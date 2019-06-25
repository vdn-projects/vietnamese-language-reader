#pragma once
#include "opencv2/opencv.hpp"
using namespace cv;


bool InitLight(CvSize imgSz);

// input is 8u, calls HomographicFilter and HistNorm
void RunLightPrep(CvMat *faceImg8);

/*
	return a filter kernel
	filterType: 0:ideal, 1:gaussian, 2:butterworth
	bHighpass: true: highpass;(not supported) false: lowpass
	d0: cut-off freq, independent to kernel size
	order: for butterworth
	h: the output kernel(normalized)
*/
void GenFilterKernel(CvMat *h, int filterType, /*bool bHighpass,*/ double d0, int order = 1);

void ReleaseLight();
