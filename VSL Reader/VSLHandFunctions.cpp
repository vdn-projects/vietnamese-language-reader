#include "stdafx.h"
#include "VSLHandFunctions.h"

CHandFunctions::CHandFunctions(void)
{
	m_PresentedImg->Empty();
	m_arrModel.empty();
	timg8 = timg32 = tft = tmdl = NULL;
	handSize = cvSize(64,64);
}

CHandFunctions::~CHandFunctions(void)
{
	m_PresentedImg->ReleaseBuffer();
	
	cvReleaseMat(&tft);
	cvReleaseMat(&timg32);
	cvReleaseMat(&tmdl);
	cvReleaseMat(&timg8);
	

	ReleaseLight();
	ReleaseFeature();
	ReleaseSubspace();
}

bool CHandFunctions::InitHandFunction()
{
//Init Face Light Pre-Processing
	if(!InitLight(handSize)) return false;

//Init Face Feature
	m_ftSize = InitFeature(handSize);
	if(m_ftSize == 0) return false;

	timg8 = cvCreateMat(handSize.height, handSize.width, CV_8UC1);
	timg32 = cvCreateMat(handSize.height, handSize.width, CV_32FC1);
	tft = cvCreateMat(m_ftSize, 1, CV_32FC1);
	
	return true;
}


bool CHandFunctions::HandProcessing(CvMat *handImg8, CvMat *model)
{
	RunLightPrep(handImg8);
	cvConvertScale(handImg8, timg32, 1.0/255);
	GetFeature(timg32, tft);
	Project(tft, model);
	return true;
}

void CHandFunctions::SaveToModel( CString classId, CString name, CString path, CvMat *model )
{
	SDatabase sd;
	sd.classId = classId;
	sd.name = name;
	sd.picPath = path;
	sd.model = model;
	m_arrModel.push_back(sd);
}


void CHandFunctions::ClearModelArr()
{
	sdb_iter	iter = m_arrModel.begin();
	for (; iter != m_arrModel.end(); iter++)
	{
		cvReleaseMat(&(iter->model));
	}
	m_arrModel.clear();
}

void CHandFunctions::HandTraining(CString path2PicData)
{
	ClearModelArr();
	
	vector<SDirInfo> paths;
	ScanDirectory(path2PicData, paths);
	sdi_iter iter = paths.begin();
//	TRACE(iter->picPath);
	for (; iter != paths.end(); iter++)
	{
		CvMat *pic = cvLoadImageM(iter->picPath, 0);  //CV_LOAD_IMAGE_GRAYSCALE);
		CvMat *ft = cvCreateMat(m_ftSize, 1, CV_32FC1);		
		CvMat* picScaled = cvCreateMat(handSize.width, handSize.height, CV_8U);
		cvResize(pic, picScaled, INTER_NEAREST);

		RunLightPrep(picScaled);
		cvConvertScale(picScaled, timg32, 1.0/255);

		GetFeature(timg32, ft);
		SaveToModel( iter->classId, iter->name, iter->picPath, ft);

	//	DispCvArr(ft,"feature");
		cvReleaseMat(&pic);
		cvReleaseMat(&picScaled);
	//	cvReleaseMat(&ft);

	}
	paths.clear();

//Start training
	m_totalImg = m_arrModel.size();
	CvMat	*inputs = cvCreateMat(m_ftSize, m_totalImg, CV_32FC1);
	int		*trainIds = new int[m_totalImg];
	


	FormTrainMat(inputs, trainIds);
	//DispCvArr(inputs,"inputs1");
	m_mdlSize = CalcSubspace(inputs, trainIds);

	TrainResSave2Model(); //project all face features to subspace & save into gallary
	
	delete []trainIds;
	cvReleaseMat(&inputs);

	tmdl = cvCreateMat(m_mdlSize, 1, CV_64FC1);

}

void CHandFunctions::FormTrainMat( CvMat *inputs, int *trainIds )
{
	sdb_iter	iter = m_arrModel.begin();
	CvMat	sub, *src;
	int i = 0;
	m_totalId = 0;
	//DispCvArr(iter->model,"model 0");

	for (; iter != m_arrModel.end(); iter++)
	{
		src = iter->model;
		cvGetCol(inputs, &sub, i);
		cvCopy(src, &sub);

		bool flag = false;
		for (int j = 0; j < i; j++)
		{
			if (trainIds[j] == atoi(iter->classId))
			{
				flag = true;
				break;
			}
		}
		if (!flag) m_totalId++;
		trainIds[i++] = atoi(iter->classId);
	}
}


void CHandFunctions::TrainResSave2Model()
{
	int		mdSz = GetModelSize();
	sdb_iter	iter = m_arrModel.begin();
	for (; iter != m_arrModel.end(); iter++)
	{
		// project the feature vectors of the training samples
		CvMat *model = cvCreateMat(mdSz, 1, CV_64FC1);
		Project(iter->model, model);
		cvReleaseMat(&(iter->model));
		iter->model = model;
	}
}


void CHandFunctions::Store2Database()
{
	CvFileStorage * database;
	
	database = cvOpenFileStorage( "Database\\database.xml", 0, CV_STORAGE_WRITE );
	cvWriteInt( database, "ModelSize", m_mdlSize);
	cvWriteInt( database, "TotalImages", m_totalImg);
	cvWriteInt( database, "TotalIds", m_totalId);

	sdb_iter	iter = m_arrModel.begin();
	for (int i = 0; iter != m_arrModel.end(); iter++)
	{
		CString mdlAdding, idAdding, pathAdding, nameAdding;
				
				idAdding.Format("ID_%d", i);
				nameAdding.Format("Name_%d",i);
				pathAdding.Format("Path_%d", i);
				mdlAdding.Format("ModelVector_%d", i++ );
		
		cvWriteString(database, idAdding, iter->classId);
		cvWriteString(database, nameAdding, iter->name);
		cvWriteString(database, pathAdding, iter->picPath);
		cvWrite(database, mdlAdding, iter->model, cvAttrList(0,0));
	}
	SaveSpace(database);

	cvReleaseFileStorage(&database);
}

void CHandFunctions::LoadDatabase(CString database)
{
	ClearModelArr();
	
	CString preId, postId = "xxx";
	CvFileStorage* fileStorage;
	fileStorage = cvOpenFileStorage( database, 0, CV_STORAGE_READ );
	
	m_mdlSize = cvReadIntByName(fileStorage, 0, "ModelSize", 0);
	tmdl = cvCreateMat(m_mdlSize, 1, CV_64FC1);

	m_totalImg = cvReadIntByName(fileStorage, 0, "TotalImages", 0);
	m_totalId = cvReadIntByName(fileStorage, 0, "TotalIds", 0);
	for(int i = 0; i < m_totalImg; i++)
	{
		CString mdlAdding, idAdding, pathAdding, nameAdding;
		idAdding.Format("ID_%d", i);
		nameAdding.Format("Name_%d",i);
		pathAdding.Format("Path_%d", i);
		mdlAdding.Format("ModelVector_%d", i );

		SaveToModel(cvReadStringByName(fileStorage, 0, idAdding, 0),
					cvReadStringByName(fileStorage, 0, nameAdding, 0),
					cvReadStringByName(fileStorage, 0, pathAdding, 0),
					(CvMat*)cvReadByName(fileStorage, 0, mdlAdding, 0));

		preId = cvReadStringByName(fileStorage, 0, idAdding, 0);
		if(preId != postId)
		{
			postId = preId;
			m_PresentedImg[atoi(postId)] = cvReadStringByName(fileStorage, 0, pathAdding, 0);
		}
	}

	cvReleaseFileStorage( &fileStorage );
	LoadSpace(database);
}

void CHandFunctions::HandRecognization( Mat handImg8, SMatch *info )
{
	CvMat* handImgTmp = cvCreateMat(handSize.height, handSize.width, CV_8U);
	CvMat* matCvt = &CvMat(handImg8);
	CvMat* matCvtGr = cvCreateMat(handImg8.rows, handImg8.cols, CV_8U);
	cvCvtColor(matCvt, matCvtGr, CV_BGR2GRAY);
	cvResize(matCvtGr, handImgTmp, INTER_NEAREST);

	HandProcessing(handImgTmp, tmdl);

	SDatabase	*minpm = NULL;
	sdb_iter	iter = m_arrModel.begin();
	double	minDist = 1e9, curVal; 


	for (; iter != m_arrModel.end(); iter++)
	{
		curVal = CalcVectorDist(iter->model, tmdl);
		if (curVal < minDist)
		{
			minDist = curVal;
			minpm = &(*iter);
		}
	}

	info->classId = minpm->classId;
	info->name = minpm->name;
	info->dist = minDist;
//	info->picPath = minpm->picPath;

	info->picPath = m_PresentedImg[atoi(minpm->classId)];

	cvReleaseMat(&handImgTmp);
	cvReleaseMat(&matCvtGr);
}

CString CHandFunctions::FindName( CString fn )
{
	int a = fn.Find('_'), b = fn.ReverseFind('_');
	//return p > 0 ? fn.Left(p) : (q > 0 ? fn.Left(q) : fn);
	return fn.Mid(a + 1, b - a - 1);
}

CString CHandFunctions::FindId( CString fn )
{
	int a = fn.Find('_');
	return fn.Left(a);
}

void CHandFunctions::ScanDirectory( CString Dir, vector<SDirInfo> &DirInfo )
{
	Dir.TrimRight('\\');
	Dir += '\\';
	CFileFind search;
	BOOL fileSearch = search.FindFile(Dir + "*.*");
	while(fileSearch)
	{
		fileSearch = search.FindNextFile(); 
		if (search.IsDirectory()) continue;
		CString info = search.GetFileName();
		DirInfo.push_back(SDirInfo(FindId(info), FindName(info), Dir + info, info));
	}
}


