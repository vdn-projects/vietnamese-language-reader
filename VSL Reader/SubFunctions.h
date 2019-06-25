#pragma once
#include "stdafx.h"
#include "GlobalHeader.h"


void EmbedCvWindow(HWND pWnd, CString strWndName, int w, int h);
bool SelDirectory ( HWND hWnd, LPCTSTR strTitle, CString &strDir );
int MessageBox1(LPCTSTR lpText, UINT uType = MB_OK | MB_ICONINFORMATION, 
				LPCTSTR lpCaption = "message", HWND hWnd = NULL);

// Use TRACE to print CvArr in debug mode
// for 1D/2D 1 channel mat/img
void DispCvArr(CvArr *a, TCHAR *varName = "CvArr", bool showTranspose = false, TCHAR *format = NULL);