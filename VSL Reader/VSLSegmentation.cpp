#include "stdafx.h"
#include "VSLSegmentation.h"


CVSLSegmentation::CVSLSegmentation(void)
{
	this->se21 = NULL;
	this->se11 = NULL;
}


CVSLSegmentation::~CVSLSegmentation(void)
{
	cvReleaseStructuringElement(&se21);
	cvReleaseStructuringElement(&se11);
}


void CVSLSegmentation::cleanMask(Mat src)
{
	/// init the structuring elements if they don't exist
	if (this->se21 == NULL)
		this->se21 = cvCreateStructuringElementEx(7, 7, 4, 4, CV_SHAPE_RECT/*CV_SHAPE_ELLIPSE CV_SHAPE_RECT*/, NULL);
	if (this->se11 == NULL)
		this->se11 = cvCreateStructuringElementEx(7, 7, 4, 4, CV_SHAPE_RECT, NULL);

	// convert to the older OpenCV image format to use the algorithms below
	IplImage srcCvt = src;

	// run some morphs on the mask to get rid of noise
	cvMorphologyEx(&srcCvt, &srcCvt, 0, this->se11, CV_MOP_OPEN , 1);
	cvMorphologyEx(&srcCvt, &srcCvt, 0, this->se21, CV_MOP_CLOSE, 1);
}

Mat CVSLSegmentation::handFilter(Mat src, Rect ColorRect, IplImage* colorImg){
	Mat tempMat = src < 100 ;
	Mat rgbFrame; 
	rgbFrame.data = NULL;
	Rect hrec;
	findContours(tempMat, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE );
	
	if(contours.size() == 1){
		//drawContours( tempMat, contours, 0, Scalar(200,0,0), 1, 8, hierarchy, 0, Point() ); 
		Rect brect = boundingRect(contours[0]);
		int max_size = max(brect.height, brect.width);
		int x = cvRound((brect.x + brect.width/2) - max_size/2),
			y = tempMat.rows - max_size,
			height = max_size,
			width = max_size;

		if(x >=0 && x+width<=tempMat.cols && y>=0 && y+height<=tempMat.rows)
		hrec = Rect(x, y, width, height);

		Mat SubMsk = src(hrec); 
		Mat temp(SubMsk.rows, SubMsk.cols, CV_8UC3, Scalar(0, 0, 0));
		Rect hrec_new = Rect(ColorRect.x+hrec.x, ColorRect.y+hrec.y, hrec.width, hrec.height);
		
		rgbFrame = Mat(colorImg, true)(hrec_new);
		temp.copyTo(rgbFrame, SubMsk);
	}

	return rgbFrame;
}