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

using namespace std;
using namespace cv;

static int  color_flag = 0; //0: RGB, 1: Grey
static int  rotation_flag = 0; //1: 90, 2: 180, 3:270

static bool drag_start = false;
static bool rectangle_on = false;
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
		printf("x=%d,y=%d,width=%d,height=%d \n", minPosX, minPosY, maxPosX - minPosX, maxPosY - minPosY);
		CvSize cvs = cvGetSize(frame);
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
		//printf("selection is complete");
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

	CvVideoWriter *writer;
	char AviFileName[] = "Output.avi";
	int AviForamt = -1;
	int FPS = 25;
	//CvSize AviSize = cvSize(640, 480);
	//int AviColor = 1;
	//writer = cvCreateVideoWriter(AviFileName, AviForamt, FPS, AviSize, AviColor);

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
			cvMatchTemplate(frame, tracking, result, CV_TM_SQDIFF);

			double minVal, maxVal;
			CvPoint minLoc, maxLoc;
			cvMinMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, 0);
			printf("patter maching min=%lf, max=%lf \n", minVal, maxVal);
			cvRectangle(frame, minLoc, maxLoc, CV_RGB(255, 0, 0), 1, 8, 0);
		}
			
		//rotation
		if (rotation_flag == 1) 
			frame = rotate_image(frame, 1); //90
		else if (rotation_flag == 2)
			frame = rotate_image(frame, 2); //180
		else if (rotation_flag == 3)
			frame = rotate_image(frame, 3); //270

		cvShowImage("Webcam", frame);
		//printf("%d\n", i);

	
		char inKey = 0;
		inKey = cvWaitKey(20);
		if (inKey == 'q' || inKey == 'Q')
			break;

		switch(inKey)
		{
		//transpose 90 degree
		case 'g':
		case 'G':
			color_flag = !color_flag;
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
			break;

		default:
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


