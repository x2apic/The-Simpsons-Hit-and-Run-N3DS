// WatcherDll.h : main header file for the WATCHERDLL application
//

#if !defined(AFX_WATCHERDLL_H__AC22C32D_65FD_4749_8226_F1827CCB16F0__INCLUDED_)
#define AFX_WATCHERDLL_H__AC22C32D_65FD_4749_8226_F1827CCB16F0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CWatcherDllApp:
// See WatcherDll.cpp for the implementation of this class
//

class CWatcherDllApp : public CWinApp
{
public:
	CWatcherDllApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWatcherDllApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CWatcherDllApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WATCHERDLL_H__AC22C32D_65FD_4749_8226_F1827CCB16F0__INCLUDED_)
