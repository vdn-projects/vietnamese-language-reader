#pragma once
#include "GlobalHeader.h"

class CVSLSegmentation
{
public:
	CVSLSegmentation(void);
	~CVSLSegmentation(void);

	void cleanMask(Mat src);
	Mat CVSLSegmentation::handFilter(Mat src, Rect ColorRect, IplImage* colorImg);

	private:
	IplConvKernel *se21;
	IplConvKernel *se11;
	vector<vector<Point>> contours;
};

