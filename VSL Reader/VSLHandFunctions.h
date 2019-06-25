#pragma once
#include "VSLLightPro.h"
#include "VSLHandFeature.h"
#include "VSLHandSpace.h"
#include "SubFunctions.h"

//define struct
struct SDatabase // used in the face database
{
	CString	classId;
	CString	name;
	CString	picPath;
	CvMat	*model;
};

struct SMatch // used when matching a face
{
	CString	classId;
	CString	name;
	CString	picPath;
	double	dist;
};

struct SDirInfo // Struct of File Info£¬used to store file info when training
{
	CString	classId;
	CString name;
	CString picPath;
	CString fileName;
	SDirInfo(CString id, CString name, CString path, CString fileName):classId(id), name(name), picPath(path), fileName(fileName){}
};

typedef vector<SDatabase>::iterator	sdb_iter;
typedef vector<SDirInfo>::iterator	sdi_iter;
typedef vector<SMatch>::iterator	smt_iter;


class CHandFunctions
{
public:
	CHandFunctions(void);
	~CHandFunctions(void);

//Intial processing
	bool InitHandFunction();
	
	bool HandProcessing(CvMat *handImg8, CvMat *model);
	
/*
	path2PicData: path to face pics
	saveFace: Save processed faces into a folder
	faceProcessed: all faces in face directory have been processed
*/
	void HandTraining(CString path2PicData);
	void HandRecognization(Mat handImg8, SMatch *info );

	void SaveToModel(CString classId, CString name, CString path, CvMat *model);
	
	void FormTrainMat(CvMat *inputs, int *trainIds);// generate the matrix for training "inputs", and trainIds, according to m_lstModel 
	void TrainResSave2Model();
	void Store2Database();
	void LoadDatabase(CString database);
	//Check 
	void	ClearModelArr();

	CvSize handSize;
	int m_ftSize, m_mdlSize;
	int m_totalImg, m_totalId;
	CvRect m_faceRec;
	CString m_PresentedImg[300];

	//temp
	vector<SDatabase>	m_arrModel;
	CvMat *timg8, *timg32, *tft, *tmdl;

	//Work with directory
	CString FindName(CString fn);
	CString FindId(CString fn);
	void ScanDirectory(CString Dir, vector<SDirInfo> &DirInfo);
	
};

