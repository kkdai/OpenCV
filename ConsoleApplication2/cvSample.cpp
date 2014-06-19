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
	IplImage *frame;
	capture = cvCaptureFromCAM(0); //0=default, -1=any camera, 1..99=your camera
	if (!capture) cout << "No camera detected" << endl;

	cvNamedWindow("Webcam", 1);
	
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


