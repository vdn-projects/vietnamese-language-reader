#include "stdafx.h"
#include "VSLHandSpace.h"
#include "SubFunctions.h"

static CFld g_fld;

int CalcSubspace( CvMat *inputs, int *trainIds )
{
	return g_fld.TrainFld(inputs,trainIds);;
}

void Project( CvMat *inputs, CvMat *results )
{
	g_fld.ProjectFld(inputs, results);
}

int GetModelSize(){return g_fld.m_postLdaDim;}

int GetFtDim(){return g_fld.m_inputDim;}

// If the dim is larger than 1, use cosine metric;
// Else, use L2 metric.
double CalcVectorDist( CvMat *target, CvMat *query )
{
	// use normalized cosine metric
	// other alternative metrics: L1, L2, Mahalanobis ...
/*	CString msg;
	msg.Format("%d", target->rows);
	MessageBox1(msg);
*/
	if (target->rows > 1)
		return (1-(cvDotProduct(target, query) / cvNorm(target) / cvNorm(query)))/2;
	else
		return cvNorm(target, query, CV_L2);
}

void SaveSpace(CvFileStorage * database)
{
	cvWriteInt( database, "dataBytes",  g_fld.W_prjT->step / g_fld.W_prjT->cols );
	cvWriteInt( database, "inputDim", g_fld.m_inputDim );
	cvWriteInt( database, "postLdaDim", g_fld.m_postLdaDim );
	cvWrite( database, "mu_total", g_fld.mu_total, cvAttrList(0,0));
	cvWrite( database, "W_prjT", g_fld.W_prjT, cvAttrList(0,0));
	
}

void LoadSpace(CString database){
	CvFileStorage * fileStorage;
	fileStorage = cvOpenFileStorage( database, 0, CV_STORAGE_READ );
	
	g_fld.m_inputDim = cvReadIntByName(fileStorage, 0, "inputDim", 0);
	g_fld.m_postLdaDim = cvReadIntByName(fileStorage, 0, "postLdaDim", 0);
	
	g_fld.mu_total = cvCreateMat(g_fld.m_inputDim, 1, CV_COEF_FC1);
	g_fld.mu_total = (CvMat *)cvReadByName(fileStorage, 0, "mu_total", 0);

	g_fld.W_prjT = cvCreateMat(g_fld.m_postLdaDim, g_fld.m_inputDim, CV_COEF_FC1);
	g_fld.W_prjT = (CvMat *)cvReadByName(fileStorage, 0, "W_prjT", 0);

	cvReleaseFileStorage( &fileStorage );
}

void ReleaseSubspace()
{
	g_fld.Release();
}