// ConsoleApplication2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <opencv2\objdetect\objdetect.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\opencv.hpp>

#include <iostream>
#include <stdio.h>

using namespace std;
using namespace cv;

static bool drag_start = false;
static bool rectangle_on = false;
static int  Pos_x = 0;
static int  Pos_y = 0;

static int  Sel_x = 0;
static int  Sel_y = 0;

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

		//if sel[2] > sel[0] and sel[3] > sel[1]:
		//patch = gray[sel[1]:sel[3], sel[0] : sel[2]]
			//result = cv.matchTemplate(gray, patch, cv.TM_CCOEFF_NORMED)
			//result = np.abs(result)**3
			//val, result = cv.threshold(result, 0.01, 0, cv.THRESH_TOZERO)
			//result8 = cv.normalize(result, None, 0, 255, cv.NORM_MINMAX, cv.CV_8U)
			//cv.imshow("result", result8)
		rectangle_on = true;
		drag_start = false;
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
		//cvWriteFrame(writer, frame);

		//cv::cvtColor(frame, gray, COLOR_BGR2GRAY);
		if (rectangle_on)
			cvRectangle(frame, cvPoint(minPosX, minPosY), cvPoint(maxPosX, maxPosY), CV_RGB(255, 0, 0), 1, 8, 0);
		cvShowImage("Webcam", frame);
		//printf("%d\n", i);

	
		char inKey = 0;
		inKey = cvWaitKey(20);
		if (inKey == 'q' || inKey == 'Q')
			break;

		switch(inKey)
		{
		//transpose 90 degree
		case 't':
		case 'T':
			frame = rotate_image(frame, 1);
			cvNamedWindow("transpose90", 1);
			cvShowImage("transpose90", frame);
			break;
		case 'f':
		case 'F':
			frame = rotate_image(frame, 2);
			cvNamedWindow("transpose180", 1);
			cvShowImage("transpose180", frame);
			break;
		case 'r':
		case 'R':
			frame = rotate_image(frame, 3);
			cvNamedWindow("transpose270", 1);
			cvShowImage("transpose270", frame);
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


