#include "stdafx.h"
#include "VSLStreamRender.h"




void InitSenz3D(UtilPipeline *pp){
	pp->EnableGesture();
	pp->EnableImage(PXCImage::COLOR_FORMAT_RGB32, COLOR_W, COLOR_H);
	pp->EnableImage(PXCImage::COLOR_FORMAT_DEPTH, DEPTH_W, DEPTH_H);
	pp->Init();
}

void FrameStart(UtilPipeline *pp){
	pp->AcquireFrame(true);
}

void FrameEnd(UtilPipeline *pp){
	pp->ReleaseFrame();
}

IplImage* QueryDepthImage(UtilPipeline *pp){
	IplImage *depthImg = cvCreateImageHeader(cvSize(DEPTH_W, DEPTH_H), 16, 1);
	PXCImage::ImageData depthdata;
    PXCImage *depth = pp->QueryImage(PXCImage::IMAGE_TYPE_DEPTH);
	
	depth->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::COLOR_FORMAT_DEPTH, &depthdata);
	cvSetData(depthImg, (ushort*)depthdata.planes[0], depthImg->width*sizeof(ushort));
	depth->ReleaseAccess(&depthdata);
	
	return depthImg;
}

IplImage* QueryBlobImage( PXCImage *blob){
	IplImage *blobImg = cvCreateImageHeader(cvSize(DEPTH_W, DEPTH_H), 8, 1);
	PXCImage::ImageData blobdata;

	blob->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::COLOR_FORMAT_GRAY, &blobdata);
	cvSetData(blobImg, (uchar*)blobdata.planes[0], blobImg->width*sizeof(uchar));
	blob->ReleaseAccess(&blobdata);

	return blobImg;
}

IplImage* QueryColorImage(UtilPipeline *pp){
	IplImage* colorImg = cvCreateImageHeader(cvSize(COLOR_W, COLOR_H), 8, 3);
	PXCImage::ImageData colordata;
    PXCImage *color = pp->QueryImage(PXCImage::IMAGE_TYPE_COLOR);
	
	color->AcquireAccess(PXCImage::ACCESS_READ, &colordata);
	cvSetData(colorImg, (uchar*)colordata.planes[0], colorImg->width*sizeof(uchar)*colorImg->nChannels);
	color->ReleaseAccess(&colordata);
	
	return colorImg;
}


ushort Distance(UtilPipeline *pp, int x, int y){
	PXCImage::ImageData depthdata;
    PXCImage *depth = pp->QueryImage(PXCImage::IMAGE_TYPE_DEPTH);
	
	depth->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::COLOR_FORMAT_DEPTH, &depthdata);
	ushort& dist = ((ushort*)depthdata.planes[0])[y*DEPTH_W + x];
	depth->ReleaseAccess(&depthdata);
	
	if(dist == LOW_CONF || dist == SAT_VAL) return -1;
	else return dist;
}

int FPS(double &t, int &totalMS, int &frameCnt){
	t = (double)cvGetTickCount() - t;
	frameCnt++;
	totalMS += (int)(t / (cvGetTickFrequency() * 1000.0F));
	int fps = (int)(frameCnt / ((float)totalMS / 1000.0F));
	
	if (frameCnt > 5000)
	{
		frameCnt = 0;
		totalMS = 0;
	}

	return fps;
}
	
//Map depth data to color image using UV mapping
CvPoint UVMap(UtilPipeline *pp, int x, int y) {
   PXCImage::ImageData depthdata;
   int index = y*DEPTH_W + x;

   PXCImage *depth = pp->QueryImage(PXCImage::IMAGE_TYPE_DEPTH);
   depth->AcquireAccess(PXCImage::ACCESS_READ, &depthdata);
   ushort& dist = ((ushort*)depthdata.planes[0])[index];
   float *uvmap = (float*)depthdata.planes[2];

   CvPoint MapPoint = cvPoint(-1, -1);

   if((uvmap) && (dist != LOW_CONF) && (dist != SAT_VAL) &&
	   x>=0 && x<=DEPTH_W && y>=0 && y<=DEPTH_H)
   { 
		int xx = (int)(uvmap[index*2 + 0]*COLOR_W + 0.5f);
		int yy = (int)(uvmap[index*2 + 1]*COLOR_H + 0.5f);
		if(xx>=0 && xx<=COLOR_W && yy>=0 && yy<=COLOR_H)
			MapPoint = cvPoint(xx, yy);
   }
   depth->ReleaseAccess(&depthdata);

   return MapPoint;
}

//Still in debuging
bool InitProjection(UtilPipeline *pp, PXCProjection* projection, PXCPoint3DF32 *pos2d){
	
	pxcUID prj_value;
	pxcStatus sts = pp->QueryCapture()->QueryDevice()->QueryPropertyAsUID(PXCCapture::Device::PROPERTY_PROJECTION_SERIALIZABLE,&prj_value);
	if (sts>=PXC_STATUS_NO_ERROR) {
		pp->QuerySession()->DynamicCast<PXCMetadata>()->CreateSerializable<PXCProjection>(prj_value, &projection);
		int k = 0;
		for (float y=0;y<DEPTH_H;y++)
			for (float x=0;x<DEPTH_W;x++,k++)
				pos2d[k].x=x, pos2d[k].y=y;
		printf("\nOK 2");
		return TRUE;
	} else return FALSE;

}

CvPoint Projection(UtilPipeline *pp, int x, int y, PXCPoint3DF32 *pos2d, PXCPointF32 *posc, PXCProjection* projection) {

	PXCImage::ImageData depthdata;
	CvPoint ProjectPoint = cvPoint(-1, -1);
    PXCImage *depth = pp->QueryImage(PXCImage::IMAGE_TYPE_DEPTH);
	depth->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::COLOR_FORMAT_DEPTH, &depthdata);
	
	if(x>=0 && x<DEPTH_W && y>=0 && y<DEPTH_H)
	{
		int index = y*DEPTH_W + x;
		ushort& dist = ((ushort*)depthdata.planes[0])[index];
		pos2d[index].z = dist;
		
		if(pos2d && (dist != LOW_CONF) && (dist != SAT_VAL)){
		projection->MapDepthToColorCoordinates(DEPTH_W*DEPTH_H,pos2d,posc);
		int xx= (int)(posc[index].x+0.5f); 
		int	yy= (int)(posc[index].y+0.5f);
		if(xx>0 && xx<COLOR_W && yy>0 && yy<COLOR_H)
			ProjectPoint = cvPoint(xx, yy);		
		}
	}

	depth->ReleaseAccess(&depthdata);

	return ProjectPoint;
}



/*typeImg: 0:depth, 1: color */
void DrawGeoNode(UtilPipeline *pp, IplImage* Image, PXCGesture *gesture, int typeImg)
 {
	PXCGesture::GeoNode nodes[2][11]={0};
	gesture->QueryNodeData(0,PXCGesture::GeoNode::LABEL_BODY_HAND_PRIMARY,10,nodes[0]);
	gesture->QueryNodeData(0,PXCGesture::GeoNode::LABEL_BODY_HAND_SECONDARY,10,nodes[1]);
	gesture->QueryNodeData(0,PXCGesture::GeoNode::LABEL_BODY_ELBOW_PRIMARY,&nodes[0][10]);
	gesture->QueryNodeData(0,PXCGesture::GeoNode::LABEL_BODY_ELBOW_SECONDARY,&nodes[1][10]);

	if (nodes[0][0].body > 0) 
	{
	//	printf("Hand 1: %d\n", nodes[0][0].openness);
	}
	if (nodes[1][0].body > 0) 
	{
	//	printf("Hand 2%d\n", nodes[1][0].openness);
	}

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 11; j++) {
			if (nodes[i][j].body <= 0) continue;
			int sz = (j == 0) ? 10 : ((nodes[i][j].radiusImage>5)?(int)nodes[i][j].radiusImage:5); // RADIUS 
			int x = (int)nodes[i][j].positionImage.x;
			int y = (int)nodes[i][j].positionImage.y;

			CvScalar clolor = (j<6 || j==10)?cvScalar( 255, 0, 0 ): ((j == 6)?cvScalar( 0, 255, 0 ):cvScalar( 0, 0, 255 ));	

			if(typeImg == 0) cvCircle(Image, cvPoint((int)x,(int)y), sz,  clolor, 1 );
			else if (typeImg == 1){
				cvCircle(Image, UVMap(pp, x, y), sz,  clolor, 2 );
			}
		}
	}
 }

/*
CvPoint HandSignal(PXCGesture *gesture){
	CvPoint HandPoint = cvPoint(-1, -1);
	PXCGesture::Blob bdata;
	gesture->QueryBlobData(PXCGesture::Blob::LABEL_SCENE, 0, &bdata);

	if((bdata.labelRightHand != 0xffffffff) || ((bdata.labelLeftHand != 0xffffffff))){
		PXCGesture::GeoNode palmnode[1] = {0};		
		gesture->QueryNodeData(0,PXCGesture::GeoNode::LABEL_BODY_HAND_PRIMARY, &palmnode[0]);
		if(palmnode[0].body>0)
			HandPoint = cvPoint(palmnode[0].positionImage.x, palmnode[0].positionImage.y);
	}

	return HandPoint;
}*/


char* BasicGestures(PXCGesture *gesture)
 {
	static struct { int label; char* gesture_name; } ges_array[]={
		{ PXCGesture::Gesture::LABEL_POSE_THUMB_UP,     "Thump Up"    }, 
		{ PXCGesture::Gesture::LABEL_POSE_THUMB_DOWN,	"Thump Down"  },
		{ PXCGesture::Gesture::LABEL_POSE_PEACE,        "Peace"       },
		{ PXCGesture::Gesture::LABEL_POSE_BIG5,         "Big 5"       },
		{ PXCGesture::Gesture::LABEL_HAND_WAVE,			"Hand Wave"	  },
		{ PXCGesture::Gesture::LABEL_HAND_CIRCLE,		"Circle"	  },
		{ PXCGesture::Gesture::LABEL_NAV_SWIPE_LEFT,    "Swipe Left"  },
		{ PXCGesture::Gesture::LABEL_NAV_SWIPE_RIGHT,   "Swipe Right" },
		{ PXCGesture::Gesture::LABEL_NAV_SWIPE_UP,      "Swipe Up"    },
		{ PXCGesture::Gesture::LABEL_NAV_SWIPE_DOWN,    "Swipe Down"  },
		};
	char* ges_name = " ";
	PXCGesture::Gesture gestures[1]={0};
	gesture->QueryGestureData(0,PXCGesture::GeoNode::LABEL_BODY_HAND_PRIMARY,0,&gestures[0]);
	
	if (gestures[0].body > 0)
	for (int j=0; j<10; j++) {
		if (ges_array[j].label == gestures[0].label) 
			ges_name = ges_array[j].gesture_name;
		//SetTimer(hwndDlg,gesture_panels[i],3000,0);
	}

	return ges_name;
 }

int big5signal(UtilPipeline *pp, PXCGesture *gesture, CvRect &handRegion){
	int d = 0;
	double distance = 0;
	PXCGesture::GeoNode handnodes[1][10]={0};
	PXCGesture::Gesture big5[1]={0};		
	gesture->QueryGestureData(0,PXCGesture::GeoNode::LABEL_BODY_HAND_LEFT,0, &big5[0]);
	gesture->QueryNodeData(0,PXCGesture::GeoNode::LABEL_BODY_HAND_LEFT,10,handnodes[0]);
	
	
	if(handnodes[0][0].body>0 && handnodes[0][3].body >0 && 
	   big5[0].label == PXCGesture::Gesture::LABEL_POSE_BIG5){
		CvPoint p1 = cvPoint(handnodes[0][0].positionImage.x, handnodes[0][0].positionImage.y); //palm position
		distance = Distance(pp, p1.x, p1.y);

		if(distance>= DISTANCE_MIN && distance<=DISTANCE_MAX){
			CvPoint p2 = cvPoint(handnodes[0][3].positionImage.x, handnodes[0][3].positionImage.y); //fingertip position
			d = p1.y - p2.y;
			if(d>0)	{
				CvPoint p = cvPoint((int)(p1.x - 0.9*d), (int)(p1.y - 1.3*d));
				int radius = (int)(1.8*d);
				handRegion = cvRect(p.x, p.y, radius, radius);			
			}
		}
	}

	return d;
}

CvRect GetHandRegion(UtilPipeline *pp, PXCGesture *gesture, PXCPoint3DF32 *pos2d, PXCPointF32 *posc, PXCProjection* projection, int d_size, CvRect& HandRegionDepth, CString &stt){
	CvRect HandRegionColor = {-1};

	PXCGesture::GeoNode palmnode[1] = {0};		
	gesture->QueryNodeData(0,PXCGesture::GeoNode::LABEL_BODY_HAND_PRIMARY, &palmnode[0]);

	if( palmnode[0].body>0 ){
		CvPoint HandPoint_Depth = cvPoint((int)(palmnode[0].positionImage.x), (int)(palmnode[0].positionImage.y));
		ushort dist = Distance(pp, HandPoint_Depth.x, HandPoint_Depth.y);
		
		if( dist>= DISTANCE_MIN && dist<= DISTANCE_MAX) {
			int radius = (int)(1.6*d_size);
			CvPoint p1 = cvPoint((int)(HandPoint_Depth.x - 0.7*d_size), (int)(HandPoint_Depth.y - 1.1*d_size));
			CvPoint p2 = cvPoint(p1.x + radius, p1.y + radius);
			
			CvPoint p1_ = Projection(pp, p1.x, p1.y, pos2d, posc, projection);
			CvPoint p2_ = Projection(pp, p2.x, p2.y, pos2d, posc, projection);

		//	CvPoint p1_ = UVMap(pp, p1.x, p1.y);
		//	CvPoint p2_ =  UVMap(pp, p2.x, p2.y);

			if(	p1_.x!=-1 && p2_.x!=-1 &&
				p2_.x>p1_.x && p2_.y>p1_.y)
			{
				HandRegionColor = cvRect(p1_.x, p1_.y,  
								  		 p2_.x - p1_.x, 
										 p2_.y - p1_.y);
				HandRegionDepth = cvRect(p1.x, p1.y,  
								  		 p2.x - p1.x, 
										 p2.y - p1.y);
			}
			stt = "";

		} else {
		//	printf("\nHand is too far or too close");
			if(dist < DISTANCE_MIN) stt = "Too close! Please move further";
			if(dist > DISTANCE_MAX) stt = "Too far! Please move closer";
		}
	}

	return HandRegionColor;
}


Mat HandMask(UtilPipeline *pp, CvRect handDepthRegion, CvRect handColorRegion, PXCPoint3DF32 *pos2d, PXCPointF32 *posc, PXCProjection* projection){
	PXCImage::ImageData depthdata;
	IplImage* maskImg = NULL;
	Mat maskMat;
    PXCImage* depth = pp->QueryImage(PXCImage::IMAGE_TYPE_DEPTH);
	int px = handDepthRegion.x;
	int py = handDepthRegion.y;
	int pw = handDepthRegion.width;
	int ph = handDepthRegion.height;

	depth->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::COLOR_FORMAT_DEPTH, &depthdata);
	
	for(int y = py ; y<(py + ph); y++)
		for(int x = px ; x< (px+pw); x++){
			int indexDepth = y*DEPTH_W+x;
			pos2d[indexDepth].z = ((short*)depthdata.planes[0])[indexDepth];
		}
	projection->MapDepthToColorCoordinates(DEPTH_W*DEPTH_H, pos2d, posc);

	int colorX1 = handColorRegion.x;
	int colorY1 = handColorRegion.y;

	int color_W = handColorRegion.width;
	int color_H = handColorRegion.height;

/*
	int colorX1 = (int)(posc[handDepthRegion.y*DEPTH_W+handDepthRegion.x].x + 0.5f);
	int colorY1 = (int)(posc[handDepthRegion.y*DEPTH_W+handDepthRegion.x].y + 0.5f);

	int colorX2 = (int)(posc[(handDepthRegion.y+handDepthRegion.height)*DEPTH_W+(handDepthRegion.x+handDepthRegion.width)].x + 0.5f);
	int colorY2 = (int)(posc[(handDepthRegion.y+handDepthRegion.height)*DEPTH_W+(handDepthRegion.x+handDepthRegion.width)].y + 0.5f);

	int color_W = colorX2-colorX1;
	int color_H = colorY2-colorY1;
*/

	if( color_W>0 && color_W<COLOR_W && color_H>0 && color_H<COLOR_H ){
		maskImg = cvCreateImage(cvSize(color_W, color_H), 8, 1);
		cvSet(maskImg, cvScalar(255)); //flood white color over the image
		
		for(int y = py, k=0; y < (py+ph); y++, k++)
			for(int x = px; x<(px + pw); x++){
				int indexDepth = y*DEPTH_W+x;
				int xx = (int)(posc[indexDepth].x + 0.5f);
				int yy = (int)(posc[indexDepth].y + 0.5f);

				if( xx>=0 && xx<COLOR_W && yy>=0 && yy<COLOR_H && pos2d[indexDepth].z < DEPTH_THRESHOLD &&
					pos2d[indexDepth].z!= LOW_CONF && pos2d[indexDepth].z!= SAT_VAL){
						int w = xx - colorX1;
						int h = yy - colorY1;
						if(w>=0 && w< color_W && h>=0 && h<color_H) cvSet2D(maskImg, h, w, cvScalar(0));
				}
			}

			maskMat = Mat(maskImg, true);	
			cvReleaseImage(&maskImg);	
	}
	depth->ReleaseAccess(&depthdata);
	return maskMat;
}