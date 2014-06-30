// ConsoleApplication2.cpp : Defines the entry point for the console application.
/*
*  You need install latest OpenCV binary in C:\opencv
*  Download it from http://sourceforge.net/projects/opencvlibrary/files/opencv-win/
*/

#include "stdafx.h"
#include <opencv2\objdetect\objdetect.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\opencv.hpp>

#include <iostream>
#include <stdio.h>
#include <time.h>

using namespace std;
using namespace cv;

static int  color_flag = 0; //0: RGB, 1: Grey
static int  rotation_flag = 0; //1: 90, 2: 180, 3:270

static bool drag_start = false;
static bool rectangle_on = false;
static bool verbose = false;
static bool barcodeEnable = false;
static int  Pos_x = 0;
static int  Pos_y = 0;

IplImage *frame = NULL;
IplImage *gray = NULL;
IplImage *tracking = NULL;

static int minPosX = 0;
static int minPosY = 0;
static int maxPosX = 0;
static int maxPosY = 0;

static void onMouse(int event, int x, int y, int, void*)
{
	if (event == EVENT_LBUTTONDOWN) {
		drag_start = true;
		Pos_x = x;
		Pos_y = y;
		minPosX = 0; minPosY = 0; maxPosX = 0; maxPosY = 0;
		rectangle_on = false;
	}
	else if (event == EVENT_LBUTTONUP)  {
		//Show new image
		cvSetImageROI(frame, cvRect(minPosX, minPosY, maxPosX - minPosX, maxPosY - minPosY));
		if (verbose)
			printf("x=%d,y=%d,width=%d,height=%d \n", minPosX, minPosY, maxPosX - minPosX, maxPosY - minPosY);
		CvSize cvs = cvGetSize(frame);
		if (verbose)
			printf("size width=%d, height=%d \n", cvs.width, cvs.height);
		tracking = cvCreateImage(cvGetSize(frame), frame->depth, frame->nChannels);
		cvCopy(frame, tracking, NULL);
		cvResetImageROI(frame);
		cvDestroyWindow("Tracking Image");
		cvShowImage("Tracking Image", tracking);
		drag_start = false;
		if (minPosX != 0)
			rectangle_on = true;
	}
	else if (drag_start) { //Drag start
		minPosX = min(Pos_x, x);
		minPosY = min(Pos_y, y);
		maxPosX = max(Pos_x, x);
		maxPosY = max(Pos_y, y);
		cvRectangle(frame, cvPoint(minPosX, minPosY), cvPoint(maxPosX, maxPosY), CV_RGB(255, 0, 0), 1, 8, 0);
		cvShowImage("Webcam", frame);
	}
	else {
		if (verbose)
			printf("selection is complete");
		drag_start = false;
	}
}


IplImage* transposeImage(IplImage* image) {

	IplImage *rotated = cvCreateImage(cvSize(image->height, image->width),
		IPL_DEPTH_8U, image->nChannels);
	CvPoint2D32f center;
	float center_val = (float)((image->width) - 1) / 2;
	center.x = center_val;
	center.y = center_val;
	CvMat *mapMatrix = cvCreateMat(2, 3, CV_32FC1);
	cv2DRotationMatrix(center, 90, 1.0, mapMatrix);
	cvWarpAffine(image, rotated, mapMatrix,
		CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS,
		cvScalarAll(0));
	cvReleaseMat(&mapMatrix);

	return rotated;
}

void detect_barcode(IplImage* imgco)
{
	IplImage* img = cvCreateImage(cvGetSize(imgco), 8, 1);
	IplImage* imgx = cvCreateImage(cvGetSize(img), IPL_DEPTH_16S, 1);
	IplImage* imgy = cvCreateImage(cvGetSize(img), IPL_DEPTH_16S, 1);
	IplImage* thresh = cvCreateImage(cvGetSize(img), 8, 1);

	IplImage* img2 = cvCreateImage(cvGetSize(imgco), 8, 1);
	IplImage* img3 = cvCreateImage(cvGetSize(imgco), 8, 1);

	//### Convert image to gray ###
	cvCvtColor(imgco, img, CV_BGR2GRAY);

	//### Finding horizontal and vertical gradient ###
	//When img is 8s, dst must be 16s because Sobel result has negtive value. 
	cvSobel(img, imgx, 1, 0, 3); //find horizontal gradient (x,0,3)
	cvAbs(imgx, imgx);
	cvConvertScale(imgx, img2);
	//cvShowImage("x-axi", img2);

	cvSobel(img, imgy, 0, 1, 3); //find vertical gradient (0,y,3)
	cvAbs(imgy, imgy);
	cvConvertScale(imgx, img3);
	//cvShowImage("y-axi", img3);

	//Using sub to remove some border which we don't need.
	cvSub(imgx, imgy, imgx);
	cvConvertScale(imgx, img);
	//cvShowImage("x-y img", img);

	//### Low pass filtering using GAUSSIAN Blur ###
	/*
	*   Try to blur it, parameter more big, the image become more blur.
	*   More blur image helps us to find Contour for barcode. 
	*   cvSmooth(img, img, CV_GAUSSIAN, 7, 7, 0) > cvSmooth(img, img, CV_GAUSSIAN, 3, 3, 0)
	*/
	cvSmooth(img, img, CV_GAUSSIAN, 7, 7, 0);
	//cvShowImage("7X7 CV_GAUSSIAN", img);

	//### Applying Threshold ###
	cvThreshold(img, thresh, 100, 255, CV_THRESH_BINARY);

	cvErode(thresh, thresh, NULL, 2);
	cvDilate(thresh, thresh, NULL, 5);

	//### Contour finding with max. area ###
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq *pContour = NULL;
	cvFindContours(thresh, storage, &pContour, sizeof(CvContour), CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
	double area = 0;
	CvSeq *bar = 0;
	for (; pContour != NULL; pContour = pContour->h_next)  {
		double max_area = cvContourArea(pContour);
		if (max_area > area) {
			area = max_area;
			bar = pContour;
		}
	}

	//### Draw bounding rectangles ###
	if (!bar) //cannot find.
	{
		//Release memory
		cvReleaseImage(&img);
		cvReleaseImage(&imgx);
		cvReleaseImage(&imgy);
		cvReleaseImage(&thresh);
		cvReleaseImage(&img2);
		cvReleaseImage(&img3);
		cvClearMemStorage(storage);
		cvReleaseMemStorage(&storage);
		return;
	}

	CvRect bound_rect = cvBoundingRect(bar);
	CvPoint pt1 = cvPoint(bound_rect.x, bound_rect.y);
	CvPoint pt2 = cvPoint(bound_rect.x + bound_rect.width, bound_rect.y + bound_rect.height);
	cvRectangle(imgco, pt1, pt2, CV_RGB(0, 255, 255), 2);

	//Release memory
	cvReleaseImage(&img);
	cvReleaseImage(&imgx);
	cvReleaseImage(&imgy);
	cvReleaseImage(&thresh);
	cvReleaseImage(&img2);
	cvReleaseImage(&img3);
	cvClearMemStorage(storage);
	cvReleaseMemStorage(&storage);
}

IplImage *rotate_image(IplImage *image, int _90_degrees_steps_anti_clockwise)
{
	IplImage *rotated;

	if (_90_degrees_steps_anti_clockwise != 2)
		rotated = cvCreateImage(cvSize(image->height, image->width), image->depth, image->nChannels);
	else
		rotated = cvCloneImage(image);

	if (_90_degrees_steps_anti_clockwise != 2)
		cvTranspose(image, rotated);

	if (_90_degrees_steps_anti_clockwise == 3)
		cvFlip(rotated, NULL, 1);
	else if (_90_degrees_steps_anti_clockwise == 1)
		cvFlip(rotated, NULL, 0);
	else if (_90_degrees_steps_anti_clockwise == 2)
		cvFlip(rotated, NULL, -1);

	return rotated;
}

int _tmain(int argc, const char** argv)
{
	CvCapture* capture = 0;
	capture = cvCaptureFromCAM(0); //0=default, -1=any camera, 1..99=your camera
	if (!capture) cout << "No camera detected" << endl;

	cvNamedWindow("Webcam", 1);
	setMouseCallback("Webcam", onMouse, 0);

	/*
	CvVideoWriter *writer;
	char AviFileName[] = "Output.avi";
	int AviForamt = -1;
	int FPS = 25;
	CvSize AviSize = cvSize(640, 480);
	int AviColor = 1;
	writer = cvCreateVideoWriter(AviFileName, AviForamt, FPS, AviSize, AviColor);
	*/

	//Start the clock
	time_t start, end;
	time(&start);
	int counter = 0;

	int i = 0;
	while (true)
	{
		frame = cvQueryFrame(capture);
		cv::Mat ref, gref;
		if (color_flag) {
			ref = cv::Mat(frame, 0);
			cv::cvtColor(ref, gref, CV_BGR2GRAY);
			frame = &IplImage(gref);
		}

		//cvWriteFrame(writer, frame);
		//cv::cvtColor(frame, gray, COLOR_BGR2GRAY);
		
		if (rectangle_on) {
			if (color_flag) {
				cv::Mat gtp1(tracking, 0);
				cv::Mat res(ref.rows - gtp1.rows + 1, ref.cols - gtp1.cols + 1, CV_32FC1);
				cv::threshold(res, res, 0.8, 1., CV_THRESH_TOZERO);
			}

			IplImage *result = cvCreateImage(cvSize(cvGetSize(frame).width - cvGetSize(tracking).width + 1, cvGetSize(frame).height - cvGetSize(tracking).height + 1), 32, 1);
			int match_method = CV_TM_SQDIFF;

			cvMatchTemplate(frame, tracking, result, match_method);
			cvNormalize(result, result, 0, 1, NORM_MINMAX, NULL);
			double minVal, maxVal;
			CvPoint minLoc, maxLoc, matchLoc;
			cvMinMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, NULL);
			if (match_method == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED) 
				matchLoc = minLoc;
			else
				matchLoc = maxLoc;
			if (verbose)
				printf("patter maching min=%lf, max=%lf \n", minVal, maxVal);
			cvRectangle(frame, matchLoc, Point(matchLoc.x + cvGetSize(tracking).width, matchLoc.y + cvGetSize(tracking).height), CV_RGB(255, 0, 0), 1, 8, 0);
			cvReleaseImage(&result);
		}

		if (barcodeEnable)
			detect_barcode(frame);
			
		//rotation
		if (rotation_flag == 1) 
			frame = rotate_image(frame, 1); //90
		else if (rotation_flag == 2)
			frame = rotate_image(frame, 2); //180
		else if (rotation_flag == 3)
			frame = rotate_image(frame, 3); //270

		cvShowImage("Webcam", frame);
		//Stop the clock and show FPS
		time(&end);
		++counter;
		double sec = difftime(end, start);
		double fps = counter / sec;
		if (verbose)
			printf("\n%lf", fps);
	
		char inKey = 0;
		inKey = cvWaitKey(20);
		if (inKey == 'q' || inKey == 'Q')
			break;

		switch(inKey)
		{
		case 'b':
		case 'B':
			barcodeEnable = !barcodeEnable;
			if (verbose)
				printf("Enable detect barcode\n");
			break;
		case 'g':
		case 'G':
			//transpose 90 degree
			color_flag = !color_flag;
			rectangle_on = false;
			cvDestroyWindow("Tracking Image");
			break;
		case 't':
		case 'T':
			rotation_flag = 1; //90
			break;
		case 'f':
		case 'F':
			rotation_flag = 2; //180
			break;
		case 'r':
		case 'R':
			rotation_flag = 3; //270
			break;
		case 'n':
		case 'N':
			rotation_flag = 0;
			rectangle_on = false;
			cvDestroyWindow("Tracking Image");
		case 'v':
		case 'V':
			verbose = !verbose;
			break;

		default:
			if (verbose)
				printf("%c", inKey);
			break;
		}
		i++;
	}
	cvReleaseCapture(&capture);
	//cvReleaseVideoWriter(&writer);
	cvDestroyWindow("Webcam");
	cvDestroyWindow("transpose90");
	cvDestroyWindow("transpose180");
	cvDestroyWindow("transpose270");
}


