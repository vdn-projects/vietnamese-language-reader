#pragma once
#include "GlobalHeader.h"

using namespace cv;
using namespace std;



void InitSenz3D(UtilPipeline *pp);
void FrameStart(UtilPipeline *pp);
void FrameEnd(UtilPipeline *pp);

IplImage* QueryDepthImage(UtilPipeline *pp);
IplImage* QueryBlobImage(PXCImage *blob);
IplImage* QueryColorImage(UtilPipeline *pp);
ushort Distance(UtilPipeline *pp, int rows, int cols);
int FPS(double &t, int &totalMS, int &frameCnt);
CvPoint UVMap(UtilPipeline *pp, int x, int y);
bool InitProjection(UtilPipeline *pp, PXCProjection* projection, PXCPoint3DF32 *pos2d);
CvPoint Projection(UtilPipeline *pp, int x, int y, PXCPoint3DF32 *pos2d, 
				   PXCPointF32 *posc, PXCProjection* projection);
/*typeImg: 0:depth, 1: color */
void DrawGeoNode(UtilPipeline *pp, IplImage* Image, PXCGesture *gesture, int typeImg);
CvPoint HandTracking(UtilPipeline *pp, IplImage* Image, PXCGesture *gesture, PXCPoint3DF32 *pos2d, PXCPointF32 *posc, 
				  PXCProjection* projection, vector<CvPoint> &tipPointArr, vector<CvPoint> &tipPointArr_R, bool &pointEnd);
CvRect GetHandRegion(UtilPipeline *pp, PXCGesture *gesture, PXCPoint3DF32 *pos2d, PXCPointF32 *posc, 
					 PXCProjection* projection, int d_size, CvRect& HandRegionDepth, CString &stt);
char* BasicGestures(PXCGesture *gesture);
int big5signal(UtilPipeline *pp, PXCGesture *gesture, CvRect &handRegion);

Mat HandMask(UtilPipeline *pp, CvRect handDepthRegion, CvRect handColorRegion, PXCPoint3DF32 *pos2d, PXCPointF32 *posc, PXCProjection* projection);