/*
	blockwise-Fisherface method with normalized cosine metric on each block, and then compute the weighted sum of the distances.
	use this method together with FaceFeature_GSF or FaceFeature_LGBP.
	ref:
	S. Shan, W. Zhang, et. al., ¡°Ensemble of piecewise FDA based on spatial histograms of local (Gabor) binary 
	patterns for face recognition,¡± in Proc. of the 18th ICPR, vol. 4, pp. 606¨C609, Hong Kong, August 2006.
	and Ke Yan, Youbin Chen and David Zhang, 
	"Gabor Surface Feature for Face Recognition," to be published in First Asian Conference on 
	Pattern Recognition (ACPR'11), Beijing, Nov. 2011
*/
#include "stdafx.h"
#include "VSLHandSpace.h"
#include "SubFunctions.h"

static CFld *g_fld;
static int	g_blkCnt; // number of blocks
static int	g_totalPrjDim;
static int	g_totalInputDim, g_blkInputDim;
static double	*g_blkWeights;

int CalcSubspace( CvMat *inputs, int *trainIds )
{
	ReleaseSubspace();

	g_blkCnt = 40; // must be identical with the feature extraction module !!!
	int maxPrjDim = 200*g_blkCnt;
	g_totalInputDim = inputs->rows;
	g_blkInputDim = g_totalInputDim / g_blkCnt;

	CvMat sub;
	g_fld = new CFld[g_blkCnt];
	int dim1 = 0;

	for (int i = 0; i < g_blkCnt; i++)
	{
		cvGetSubRect(inputs, &sub, cvRect(0, i*g_blkInputDim, inputs->width, g_blkInputDim));
		//DispCvArr(&sub,"inputs1");
		dim1 += g_fld[i].TrainFld(&sub, trainIds);
	}

	g_blkWeights = new double[g_blkCnt];

	/*The following strategy to calculate weights works on MATLAB, but doesn't work well here.
	 *The reason may be small training samples, or the use of LAPACK.
	 *So instead of using this, we use a static weight calculated with the same algorithm from MATLAB and FERET database.
	 */
/*
	if (dim1 <= maxPrjDim)
	// use the same dim in each block, but assign different weights for diff blocks, 
	// the weight is calculated by the sum of the eigenvalues generated in FLD
	{
		g_totalPrjDim = dim1;
		double normer = cvSum(g_fld[0].fldEigenVals).val[0];
		g_blkWeights[0] = 1;
		for (int i = 1; i < g_blkCnt; i++)
		{
			//DispCvArr(g_fld[i].fldEigenVals,"ev1");
			g_blkWeights[i] = cvSum(g_fld[i].fldEigenVals).val[0];
			g_blkWeights[i] /= normer; // do some normalization
		}
		CvMat tmp = cvMat(1,g_blkCnt,CV_64FC1, g_blkWeights);
		//DispCvArr(&tmp,"weights");
	}
	else
	// use diff dim in diff blocks, the weight is the dim of each block.
	// the dim is calculated by setting a threshold to the eigenvalues generated in FLD
	{
		CvMat *evs = cvCreateMat(1, dim1, CV_64FC1);
		int idx = 0;
		for (int i = 0; i < g_blkCnt; i++)
		{
			cvGetSubRect(evs, &sub, cvRect(idx, 0, g_fld[i].m_postLdaDim, 1));
			cvCopy(g_fld[i].fldEigenVals, &sub);
			idx += g_fld[i].m_postLdaDim;
		}

		cvSort(evs,evs,NULL,CV_SORT_DESCENDING | CV_SORT_EVERY_ROW);
		//DispCvArr(evs,"evs");
		double evThres = cvGetReal1D(evs,maxPrjDim-1);
		g_totalPrjDim = 0;

		for (int i = 0; i < g_blkCnt; i++)
		{
			CvMat *ev1 = g_fld[i].fldEigenVals;
			double *pEv = ev1->data.db + ev1->cols - 1;
			int subDim1 = ev1->cols;
			while(*pEv < evThres && subDim1 > 0)
			{
				subDim1--;
				pEv--;
			}
			if (subDim1 > 0)
				g_fld[i].SetFldPrjDim(subDim1);
			g_blkWeights[i] = subDim1;
			g_totalPrjDim += subDim1;
		}
	}
*/	
	double weight[40]= { 
		0.91,0.88,0.88,0.91,
		0.97,0.98,0.98,0.97,
		0.93,1.00,1.00,0.93,
		0.89,0.97,0.97,0.89,
		0.84,0.95,0.95,0.84,
		0.74,0.91,0.91,0.74,
		0.77,0.89,0.89,0.77,
		0.80,0.85,0.85,0.80,
		0.81,0.82,0.82,0.81,
		0.77,0.80,0.80,0.77};
	for (int i=0; i<g_blkCnt;i++) g_blkWeights[i] = weight[i];
	g_totalPrjDim = dim1;

	return g_totalPrjDim;
}

void Project( CvMat *inputs, CvMat *results )
{
	CvMat subIn, subOut;
	int prjCnt = 0;
	for (int i = 0; i < g_blkCnt; i++)
	{
		cvGetRows(inputs, &subIn, i*g_blkInputDim, (i+1)*g_blkInputDim);
		cvGetRows(results, &subOut, prjCnt, prjCnt+g_fld[i].m_postLdaDim);
		prjCnt += g_fld[i].m_postLdaDim;
		g_fld[i].ProjectFld(&subIn, &subOut);
	}
}

int GetModelSize(){return g_totalPrjDim;}

int GetFtDim(){return g_totalInputDim;}

double CalcVectorDist( CvMat *target, CvMat *query )
{
	// use normalized cosine metric on each block
	// then fuse them in score level, with the weight of subspace dim of each block.
	double score = 0;
	CvMat subT, subQ;
	int subDim, prjCnt = 0;
	for (int i = 0; i < g_blkCnt; i++)
	{
		subDim = g_fld[i].m_postLdaDim;
		cvGetRows(target, &subT, prjCnt, prjCnt+subDim);
		cvGetRows(query, &subQ, prjCnt, prjCnt+subDim);
		prjCnt += subDim;
		score -= (cvDotProduct(&subT, &subQ) / cvNorm(&subT) / cvNorm(&subQ) * g_blkWeights[i]);
	}

	//return score/prjCnt;
	return (1+score/g_blkCnt)/2;
}

void SaveSpace(CvFileStorage * database)
{
	cvWriteInt( database, "dataBytes", g_fld[0].W_prjT->step / g_fld[0].W_prjT->cols );
	cvWriteInt( database, "blockCount", g_blkCnt );
	cvWriteInt( database, "totalInputFeatureDim", g_totalInputDim );
	cvWriteInt( database, "totalProjectDim", g_totalPrjDim );
	
	for (int i = 0; i < g_blkCnt; i++)
	{
		//cvWriteInt( database, "blockIndex", i );

		CString str_inputDim, str_postLdaDim, str_mu_total, str_W_prjT;

		str_inputDim.Format("inputDim_%d",i);
		str_postLdaDim.Format("postLdaDim_%d",i);
		str_mu_total.Format("mu_total_%d",i);
		str_W_prjT.Format("W_prjT_%d",i);

		cvWriteInt( database, str_inputDim, g_fld[i].m_inputDim );
		cvWriteInt( database, str_postLdaDim, g_fld[i].m_postLdaDim );

		cvWrite( database, str_mu_total, g_fld[i].mu_total, cvAttrList(0,0));
		cvWrite( database, str_W_prjT, g_fld[i].W_prjT, cvAttrList(0,0));
	}
}

void LoadSpace(CString database){
	CvFileStorage * fileStorage;
	fileStorage = cvOpenFileStorage( database, 0, CV_STORAGE_READ );
	
//	cvReleaseMat(&mu_total);
//	cvReleaseMat(&W_prjT);
	cvReadIntByName(fileStorage, 0, "dataBytes", 0);
	g_blkCnt = cvReadIntByName(fileStorage, 0, "blockCount", 0);
	g_totalInputDim = cvReadIntByName(fileStorage, 0, "totalInputFeatureDim", 0);
	g_totalPrjDim = cvReadIntByName(fileStorage, 0, "totalProjectDim", 0);
	
	g_fld = new CFld[g_blkCnt];
	for (int i = 0; i < g_blkCnt; i++)
	{
		//cvReadIntByName(fileStorage, 0, "blockIndex", 0);
		CString str_inputDim, str_postLdaDim, str_mu_total, str_W_prjT;

		str_inputDim.Format("inputDim_%d",i);
		str_postLdaDim.Format("postLdaDim_%d",i);
		str_mu_total.Format("mu_total_%d",i);
		str_W_prjT.Format("W_prjT_%d",i);
		
		g_fld[i].m_inputDim = cvReadIntByName(fileStorage, 0, str_inputDim, 0);
		g_fld[i].m_postLdaDim = cvReadIntByName(fileStorage, 0, str_postLdaDim, 0);

		g_fld[i].mu_total = cvCreateMat(g_fld[i].m_inputDim, 1, CV_COEF_FC1);
		g_fld[i].mu_total = (CvMat *)cvReadByName(fileStorage, 0, str_mu_total, 0);

		g_fld[i].W_prjT = cvCreateMat(g_fld[i].m_postLdaDim, g_fld[i].m_inputDim, CV_COEF_FC1);
		g_fld[i].W_prjT = (CvMat *)cvReadByName(fileStorage, 0, str_W_prjT, 0);
	}
	double weight[40]= {    
		0.91,0.88,0.88,0.91,
		0.97,0.98,0.98,0.97,
		0.93,1.00,1.00,0.93,
		0.89,0.97,0.97,0.89,
		0.84,0.95,0.95,0.84,
		0.74,0.91,0.91,0.74,
		0.77,0.89,0.89,0.77,
		0.80,0.85,0.85,0.80,
		0.81,0.82,0.82,0.81,
		0.77,0.80,0.80,0.77};

	g_blkWeights = new double[g_blkCnt];
	for (int i=0; i<g_blkCnt;i++) g_blkWeights[i] = weight[i];
	g_blkInputDim = g_totalInputDim / g_blkCnt;

	cvReleaseFileStorage( &fileStorage );
}

void ReleaseSubspace()
{
	if (g_fld) delete []g_fld;
	if (g_blkWeights) delete []g_blkWeights;
	g_blkCnt = g_totalPrjDim = g_totalInputDim = g_blkInputDim = 0;
	g_fld = NULL;
	g_blkWeights = NULL;
}
