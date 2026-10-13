// WatcherDllDlg.h : header file
//

#if !defined(AFX_WATCHERDLLDLG_H__12EB24E9_2C15_4D19_81B0_93E97AF2F58B__INCLUDED_)
#define AFX_WATCHERDLLDLG_H__12EB24E9_2C15_4D19_81B0_93E97AF2F58B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CWatcherDllDlg dialog

class CWatcherDllDlg : public CDialog
{
// Construction
public:
	CWatcherDllDlg(CWnd* pParent = NULL);	// standard constructor

    static void ConnectionStatusCallBack( bool connected, void * pUserData );

    static void OnWPChangeCallBack( const char * pWPName, bool isCreate, void * pUserData );

    static void OnRecieveWPValueCallBack( const char * pWPName, void * pUserData );

    static void WatchPointEnumerationCallBack( const char * pWPName, void * pUserData );
    
// Dialog Data
	//{{AFX_DATA(CWatcherDllDlg)
	enum { IDD = IDD_WATCHERDLL_DIALOG };
	CEdit	    m_edtFogParamType;
	CStatic	    m_stcFogParamType;
	CStatic	    m_stcFogParam;
	CStatic	    m_stcFogColourRed;
	CStatic	    m_stcFogColourGreen;
	CStatic	    m_stcFogColourBlue;
	CStatic	    m_stcFogColour;
	CStatic	    m_stcFog;
	CSliderCtrl	m_sldFogColourRed;
	CSliderCtrl	m_sldFogColourGreen;
	CSliderCtrl	m_sldFogColourBlue;
	CButton	    m_chkFogEnable;
	CButton	    m_butFogParamType;
	CEdit	    m_edtFrameRate;
	CComboBox	m_cbxGameSpeed;
	CStatic	    m_stcSpeed;
	CStatic	    m_stcFrameRate;
	CStatic	    m_stcFrameCount;
	CEdit	    m_edtFrameCount;
	CButton	    m_butUpdateVector;
	CTabCtrl	m_tabSection;
	CButton	    m_chkGameRunning;
	CSliderCtrl	m_sldFrameRate;
	BOOL	    m_bGameRunning;
	UINT	    m_uFrameCountCurrent;
	UINT	    m_uFrameRateCurrent;
	UINT	    m_uFrameRate;
	BOOL	    m_bFogEnable;
	UINT	    m_uFogParamType;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWatcherDllDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CWatcherDllDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnCheckGameRunning();
	afx_msg void OnSelchangeTabSection(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnReleasedcaptureSliderFrameRate(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonUpdateVector();
	afx_msg void OnSelchangeComboSpeed();
	afx_msg void OnCheckFogEnable();
	afx_msg void OnReleasedcaptureSliderFogColourBlue(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnReleasedcaptureSliderFogColourGreen(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnReleasedcaptureSliderFogColourRed(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonFogParamType();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

    unsigned int m_uServiceTimer;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WATCHERDLLDLG_H__12EB24E9_2C15_4D19_81B0_93E97AF2F58B__INCLUDED_)
