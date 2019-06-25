
// VSL ReaderDlg.h : header file
//
#pragma once
#include "VSLStreamRender.h"
#include "VSLSegmentation.h"
#include "VSLHandFunctions.h"
#include "SubFunctions.h"

// CVSLReaderDlg dialog
class CVSLReaderDlg : public CDialogEx
{
// Construction
public:
	CVSLReaderDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_VSLREADER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	UtilPipeline *m_pp;
	PXCSmartPtr<PXCProjection> m_projection;
	PXCPoint3DF32 *m_pos2d;       // array of depth coordinates to be mapped onto color coordinates
	PXCPointF32 *m_posc;          // array of mapped color coordinates

	PXCGesture *m_gesture;
	IplImage   *m_depthImg,
			   *m_colorImg,
			   *m_blobImg,
			   *m_nohandImg;

	
	SMatch hInfo;

	int m_ihRadius;
	bool m_binitFlag;
	bool m_bthrdStart;

	CHandFunctions *m_hand;
	CVSLSegmentation m_handSeg;
	CWinThread	*m_thrdVSL;
	UINT VSLDisplay();
	
	void TextTimeStamp(CString src, CString &txtOut, const int time);
	CString m_textTemp;
	int m_timeStp;
	CvFont m_cvFont;

	CString m_strTextOut;
	CString m_strFPS, m_strStt;
	afx_msg void OnBnClickedButtonClear();
	CString m_timeStamp;
	afx_msg void OnBnClickedButtonExit();
};

UINT RunDialog( LPVOID pParam );
