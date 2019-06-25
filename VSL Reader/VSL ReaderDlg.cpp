// VSL ReaderDlg.cpp : implementation file
//
#include "stdafx.h"
#include "VSL Reader.h"
#include "VSL ReaderDlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int g_totalMS, g_frameCnt;
double g_time;

// CVSLReaderDlg dialog
CVSLReaderDlg::CVSLReaderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CVSLReaderDlg::IDD, pParent)
	, m_strStt(_T(""))
	, m_strTextOut(_T(""))
	, m_strFPS(_T(""))
	, m_textTemp(_T(""))
	, m_timeStamp(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_nohandImg = cvLoadImage("Nohand.jpg");
}

void CVSLReaderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_OUT_TEXT, m_strTextOut);
	DDX_Text(pDX, IDC_FPS, m_strFPS);
	DDX_Text(pDX, IDC_TIMESTAMP, m_timeStamp);
}

BEGIN_MESSAGE_MAP(CVSLReaderDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, &CVSLReaderDlg::OnBnClickedButtonClear)
	ON_BN_CLICKED(IDC_BUTTON_EXIT, &CVSLReaderDlg::OnBnClickedButtonExit)
END_MESSAGE_MAP()


// CVSLReaderDlg message handlers

BOOL CVSLReaderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();


	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	//Set Font
	CFont m_Font;
	m_Font.CreateFont(	10,                        // nHeight
						0,                         // nWidth
						0,                         // nEscapement
						0,                         // nOrientation
						FW_BOLD,                   // nWeight
						FALSE,                     // bItalic
						TRUE,                     // bUnderline
						0,                         // cStrikeOut
						ANSI_CHARSET,              // nCharSet
						OUT_DEFAULT_PRECIS,        // nOutPrecision
						CLIP_DEFAULT_PRECIS,       // nClipPrecision
						DEFAULT_QUALITY,           // nQuality
						DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
						_T("Time New Roman"));                 // lpszFacename
	GetDlgItem(IDC_STATIC)->SetFont(&m_Font);
	cvInitFont(&m_cvFont,  CV_FONT_HERSHEY_PLAIN, 1, 2, 0.5, 2, CV_AA);

	// TODO: Add extra initialization here
	m_pp = new UtilPipeline;
	m_gesture = NULL;
	m_ihRadius = 0;
	m_binitFlag = TRUE;
	m_timeStp = 0;


	g_totalMS = 0;
	g_frameCnt = 0;
	g_time = 0;
	
	InitSenz3D(m_pp);
	m_hand = new CHandFunctions;
	m_hand->InitHandFunction();
//	m_hand->HandTraining("Hand Signs\\");
//	m_hand->Store2Database();
	m_hand->LoadDatabase("Database\\database.xml");

	pxcUID prj_value;
	pxcStatus sts = m_pp->QueryCapture()->QueryDevice()->QueryPropertyAsUID(PXCCapture::Device::PROPERTY_PROJECTION_SERIALIZABLE,&prj_value);
	if (sts>=PXC_STATUS_NO_ERROR) {
		m_pp->QuerySession()->DynamicCast<PXCMetadata>()->CreateSerializable<PXCProjection>(prj_value, &m_projection);
		int npoints = DEPTH_W*DEPTH_H;
		m_pos2d=(PXCPoint3DF32 *)new PXCPoint3DF32[npoints];
		m_posc=(PXCPointF32 *)new PXCPointF32[npoints];
		int k = 0;
		for (float y=0;y<DEPTH_H;y++)
			for (float x=0;x<DEPTH_W;x++,k++)
				m_pos2d[k].x=x, m_pos2d[k].y=y;
		}

	m_depthImg = cvCreateImageHeader(cvSize(DEPTH_W, DEPTH_H), 16, 1);
	m_blobImg = cvCreateImageHeader(cvSize(DEPTH_W, DEPTH_H), 8, 1);
	m_colorImg = cvCreateImageHeader(cvSize(COLOR_W, COLOR_H), 8, 3);

	CWnd *pWnd1 = GetDlgItem(IDC_VIDEO);
	EmbedCvWindow(pWnd1->m_hWnd, "Camera", 640, 480);

	CWnd *pWnd2 = GetDlgItem(IDC_HANDSEG);
	EmbedCvWindow(pWnd2->m_hWnd, "Hand Segmentation", 200, 200);
	cvShowImage("Hand Segmentation", m_nohandImg);

	m_bthrdStart = TRUE;
	m_thrdVSL = ::AfxBeginThread(RunDialog, this);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CVSLReaderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialogEx::OnSysCommand(nID, lParam);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVSLReaderDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVSLReaderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

UINT RunDialog( LPVOID pParam )
{
	CVSLReaderDlg *pDlg = (CVSLReaderDlg *)pParam;
	return pDlg->VSLDisplay();
}

UINT CVSLReaderDlg::VSLDisplay(){

	while(m_bthrdStart)
	{
		g_time = (double)cvGetTickCount();
		FrameStart(m_pp);
		m_gesture = m_pp->QueryGesture();
		m_colorImg = QueryColorImage(m_pp);

		CvRect p1 = {-1}, p2 = {-1}, p3= {-1};

		if(m_binitFlag){
			m_ihRadius = big5signal(m_pp, m_gesture, p2);
			if(m_ihRadius){
				m_binitFlag = FALSE;
			}
		} else	p1 = GetHandRegion(m_pp, m_gesture, m_pos2d, m_posc, m_projection, m_ihRadius, p3, m_strStt);
		cvPutText(m_colorImg, m_strStt, cvPoint(10, 40), &m_cvFont, cvScalar(0, 0, 180));


		if(p1.x!=-1) {
			Mat MaskImg = HandMask(m_pp, p3, p1, m_pos2d, m_posc, m_projection);
			m_handSeg.cleanMask(MaskImg);
			Mat handMat = m_handSeg.handFilter(MaskImg, p1, m_colorImg);
			if(handMat.data) {
				imshow("Hand Segmentation", handMat);
				m_hand->HandRecognization(handMat, &hInfo);
				CString textOut;
				TextTimeStamp(hInfo.name, textOut, 12);
				m_strTextOut += textOut;

				SetDlgItemText(IDC_EDIT_OUT_TEXT, m_strTextOut);
			} else cvShowImage("Hand Segmentation", m_nohandImg);

			cvRectangle(m_colorImg, cvPoint(p1.x, p1.y), cvPoint(p1.x+p1.width, p1.y+p1.height), cvScalar(255,0,0), 2);
		}else {
			m_strStt = "";	
			cvShowImage("Hand Segmentation", m_nohandImg);
		}

		
		cvShowImage("Camera", m_colorImg);
		FrameEnd(m_pp);
		
		m_strFPS.Format( "%d", FPS(g_time, g_totalMS, g_frameCnt));
		SetDlgItemText(IDC_FPS, m_strFPS);
		cvWaitKey(3);
	}
	
	m_pp->Close();
	m_pp->Release();
	delete m_hand;
	cvDestroyAllWindows();
	cvReleaseImage(&m_depthImg);
	cvReleaseImage(&m_colorImg);
	cvReleaseImage(&m_blobImg);
	cvReleaseImage(&m_nohandImg);

	return 0;
}

void CVSLReaderDlg::TextTimeStamp(CString src, CString &txtOut, const int time){

	txtOut = "";
	if(m_textTemp == src) m_timeStp++;
	if(m_timeStp == time) {
		txtOut = src;
		m_timeStp = 0;
	}
	else if(m_timeStp > time) m_timeStp = 0;

	m_timeStamp.Format("%d", time - m_timeStp);
	SetDlgItemText(IDC_TIMESTAMP, m_timeStamp);
	m_textTemp = src;
}

void CVSLReaderDlg::OnBnClickedButtonClear()
{
	// TODO: Add your control notification handler code here
	m_strTextOut = " ";
	UpdateData(FALSE);
}


void CVSLReaderDlg::OnBnClickedButtonExit()
{
	// TODO: Add your control notification handler code here
	m_bthrdStart = FALSE;
	m_thrdVSL->SuspendThread();
	OnCancel();
}
