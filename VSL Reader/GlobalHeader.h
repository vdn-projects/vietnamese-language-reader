#include <vector>
#include <fstream>
#include <windows.h>
#include <iostream>
#include <assert.h>

#include "util_pipeline.h"
#include "pxcmetadata.h"
#include "pxcprojection.h"
#include "util_render.h"
#include "pxcgesture.h"

#include "cv.h"
#include "highgui.h"
#include "cvaux.h"
#include "opencv2/opencv.hpp"

#define PI 3.14159265359
#define DEPTH_H 240
#define DEPTH_W 320
#define COLOR_H 480
#define COLOR_W 640
#define LOW_CONF 3002
#define SAT_VAL 3001
#define DISTANCE_MIN 300
#define DISTANCE_MAX 500
#define DEPTH_THRESHOLD 500

using namespace std;
using namespace cv;

