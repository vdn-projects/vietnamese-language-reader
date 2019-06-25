#pragma once
#include "Gabor.h"


#define PI			CV_PI
// define the precision of features, depth = 32 or 64
#define CV_FT_FC1	CV_32FC1 // 32 is faster than 64, while the correct rate seems a little higher
#define CV_FT_FC2	CV_32FC2

typedef float	ft_float; // ft_float = float or double

// Feature length is returned for init feature vectors.
// If error occurs, 0 is returned. 
int InitFeature(CvSize imgSize);

// ft32 is floating-point, and is a column vector
void GetFeature(CvArr *faceImg32, CvMat *ft32);

void ReleaseFeature();