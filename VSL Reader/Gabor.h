#pragma once
#include "GlobalHeader.h"


#define PI	CV_PI


/*
	you should pass the face size in and use faceImg32 
	of the same size in GaborConv, because we use fft to accelerate.
	return cvSize(angleNum,scaleNum).
*/
CvSize InitGabor( CvSize imgSize );

/* 
	calc the magnitude of a 2-channel complex array 
*/
void Magnitude(CvArr *input, CvMat *mag);

/*
	the returned pointer has scaleNum rows and angleNum cols(see Gabor.cpp),
	each element is a Gabor magnitude face with the same size of faceImg32.
	it will be allocated and released automatically.
*/
CvMat*** GaborConv( CvArr *faceImg32 );

void ShowGaborFace( CvArr *faceImg32 );

/*
	Remember call this at the end!
*/
void ReleaseGabor();


void ShowGaborFace(CvArr *faceImg32);