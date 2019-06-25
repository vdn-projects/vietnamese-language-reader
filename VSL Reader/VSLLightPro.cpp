/*
	The strategy of Tan's method is: Gamma Correction, Difference of Gaussian (DoG) Filtering, (Masking,) Contrast Equalization.
	We do not use mask here.
	ref: Tan's "Enhanced Local Texture Feature Sets for Face Recognition under Difficult Lighting Conditions"
*/
#include "stdafx.h"
#include "VSLLightPro.h"

static CvMat	*g_tfaceImg32; // temp variables
static CvMat	*g_tfaceImg32_1;
static CvMat	*g_tfaceImg32_2;
static CvMat	*g_h; // the DoG bandpass kernel


void InitFilterKernel();


bool InitLight( CvSize faceSz )
{
	g_tfaceImg32 = cvCreateMat(faceSz.height, faceSz.width, CV_32FC1);
	g_tfaceImg32_1 = cvCreateMat(faceSz.height, faceSz.width, CV_32FC1);
	g_tfaceImg32_2 = cvCreateMat(faceSz.height, faceSz.width, CV_32FC1);
	InitFilterKernel();
	return true;
}

void GenFilterKernel(CvMat *h, int filterType, /*bool bHighpass,*/ double d0, int order/* = 1*/)
{
	CvSize sz = cvGetSize(h);
	assert(d0 > 0/* && d0 < __min(sz.width/2, sz.height/2)*/);
	assert(order > 0);
	assert(filterType >= 0 && filterType <= 3);

	double	i,j;
	double	r,z;
	for (int y = 0; y < sz.height; y++)
	{
		for (int x = 0; x < sz.width; x++)
		{
			i = x - double(sz.width - 1)/2;
			j = y - double(sz.height - 1)/2;
			r = sqrt(i*i+j*j);
			switch(filterType)
			{
			case 0:
				z = (r < d0);
				break;
			case 1:
				z = exp(-r*r/(2*d0*d0));
				break;
			case 2:
				z = 1/(1+pow(r/d0, 2*filterType));
			}
			//if (bHighpass) z = 1-z; // incorrect for spatial domain
			cvmSet(h, y, x, z);
		}
	}
	CvScalar s = cvSum(h);
	cvScale(h, h, 1./s.val[0]);
}


void InitFilterKernel()
{
	double 
		s0 = 0.5, // the smaller, the more hi-freq to keep
		s1 = 2; // the smaller, the more low-freq to subtract
	int hRadius = cvRound(s1*2);
	g_h = cvCreateMat(hRadius*2+1, hRadius*2+1, CV_32FC1);
	CvMat *hTmp = cvCloneMat(g_h);
	GenFilterKernel(g_h, 1, s0);
	GenFilterKernel(hTmp, 1, s1);
	cvSub(g_h,hTmp,g_h);
	//DispCvArr(g_h, "g_h");
	cvReleaseMat(&hTmp);
}

double 
	gamma = 0.5, 
	alpha = .3,
	tau = 12.3;

void RunLightPrep( CvMat *faceImg8 )
{
	cvConvertScale(faceImg8, g_tfaceImg32, 1.0/255);

	cvPow(g_tfaceImg32,g_tfaceImg32, gamma); /* gamma correction */
	cvFilter2D(g_tfaceImg32,g_tfaceImg32, g_h); /* DoG */

	/* Contrast Equalization */

	// img = img/(mean2(abs(img).^alpha)^(1/alpha));
	cvAbs(g_tfaceImg32, g_tfaceImg32_1);
	cvPow(g_tfaceImg32_1, g_tfaceImg32_1, alpha);
	double m = pow(cvAvg(g_tfaceImg32_1).val[0], 1/alpha);
	cvScale(g_tfaceImg32, g_tfaceImg32, 1/m);

	// img = img/(mean2(min(tau,abs(img)).^alpha)^(1/alpha));
	cvAbs(g_tfaceImg32, g_tfaceImg32_1);
	cvMinS(g_tfaceImg32_1, tau, g_tfaceImg32_1);
	cvPow(g_tfaceImg32_1, g_tfaceImg32_1, alpha);
	m = pow(cvAvg(g_tfaceImg32_1).val[0], 1/alpha);
	cvScale(g_tfaceImg32, g_tfaceImg32, 1/m/tau);

	// tanh
	cvExp(g_tfaceImg32, g_tfaceImg32_1);
	cvSubRS(g_tfaceImg32, cvScalar(0), g_tfaceImg32);
	cvExp(g_tfaceImg32, g_tfaceImg32_2);
	cvSub(g_tfaceImg32_1, g_tfaceImg32_2, g_tfaceImg32);
	cvAdd(g_tfaceImg32_1, g_tfaceImg32_2, g_tfaceImg32_1);
	cvDiv(g_tfaceImg32, g_tfaceImg32_1, g_tfaceImg32);
	
	cvNormalize(g_tfaceImg32, g_tfaceImg32, 0,1, CV_MINMAX);

	cvConvertScale(g_tfaceImg32, faceImg8, 255);
}

void ReleaseLight()
{
	cvReleaseMat(&g_h);
	cvReleaseMat(&g_tfaceImg32);
	cvReleaseMat(&g_tfaceImg32_1);
	cvReleaseMat(&g_tfaceImg32_2);
}