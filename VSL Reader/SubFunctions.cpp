#include "stdafx.h"
#include "SubFunctions.h"


void EmbedCvWindow( HWND pWnd, CString strWndName, int w, int h )
{
	cvDestroyWindow(strWndName);
	cvNamedWindow(strWndName, 0);
	HWND hWnd = (HWND) cvGetWindowHandle(strWndName);
	HWND hParent = ::GetParent(hWnd);
	::SetParent(hWnd, pWnd);
	::ShowWindow(hParent, SW_HIDE);
	::SetWindowPos(pWnd, NULL, 0,0, w,h, SWP_NOMOVE | SWP_NOZORDER);
	cvResizeWindow(strWndName, w,h);
}

int CALLBACK BrowserCallbackProc
(
 HWND   hWnd,
 UINT   uMsg,
 LPARAM   lParam,
 LPARAM   lpData
 )
{
	switch(uMsg)  
	{  
	case BFFM_INITIALIZED:  
		::SendMessage ( hWnd, BFFM_SETSELECTION, 1, lpData );  
		break;  
	default:  
		break;  
	}  
	return 0;  
}  

bool SelDirectory ( HWND hWnd, LPCTSTR strTitle, CString &strDir )  
{  
	BROWSEINFO bi;  
	char szDisplayName[MAX_PATH] = {0};  

	bi.hwndOwner = hWnd;  
	bi.pidlRoot = NULL;  
	bi.pszDisplayName = szDisplayName;  
	bi.lpszTitle = strTitle;  
	bi.ulFlags = 0;  
	bi.lpfn = BrowserCallbackProc;  
	bi.lParam = (LPARAM)(LPCTSTR)strDir;  
	bi.iImage = NULL;  

	ITEMIDLIST* piid = ::SHBrowseForFolder ( &bi );  

	if ( piid == NULL )  return false;  

	BOOL bValidPath = ::SHGetPathFromIDList ( piid, szDisplayName );  
	if ( ! bValidPath ) return false;  

	LPMALLOC lpMalloc;  
	HRESULT hr = ::SHGetMalloc ( &lpMalloc );
	assert(hr == NOERROR);  
	lpMalloc->Free ( piid );  
	lpMalloc->Release ();  

	if ( szDisplayName[0] == '\0' ) return false; 

	strDir = szDisplayName;  

	return true;
}

int MessageBox1( LPCTSTR lpText, UINT uType /*= MB_OK | MB_ICONINFORMATION*/, 
				LPCTSTR lpCaption /*= "message"*/, HWND hWnd /*= NULL*/ )
{
	return ::MessageBox(hWnd, lpText, lpCaption, uType);
}


void DispCvArr( CvArr *a, TCHAR *varName /*= "CvArr"*/, 
					  bool showTranspose /*= false*/, TCHAR *format /*= NULL*/ )
{
	/*CvMat	tmpHeader, *m = cvGetMat(a, &tmpHeader);
	int		depth = CV_MAT_DEPTH(m->type);*/
	CString str, tmpstr;
	CvSize	sz = cvGetSize(a);
	if (showTranspose) str.Format("\n%s(T) : %d x %d", varName, sz.height, sz.width);
	else str.Format("\n%s : %d x %d", varName, sz.height, sz.width);
	TRACE(str);
	TCHAR *form = "%f\t";
	if (format) form = format;
	if (!showTranspose)
		for (int i = 0; i < sz.height; i++)
		{
			str = "\n";
			for (int j = 0; j < sz.width; j++)
			{
				double d = cvGetReal2D(a, i, j);
				tmpstr.Format(form, d);
				str += tmpstr;
			}
			TRACE(str);
		}
	else
		for (int i = 0; i < sz.width; i++)
		{
			str = "\n";
			for (int j = 0; j < sz.height; j++)
			{
				double d = cvGetReal2D(a, j, i);
				tmpstr.Format(form, d);
				str += tmpstr;
			}
			TRACE(str);
		}
	
	TRACE("\n");
}