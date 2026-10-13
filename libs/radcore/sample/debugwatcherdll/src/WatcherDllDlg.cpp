//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


// WatcherDllDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WatcherDll.h"
#include "WatcherDllDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg :
    public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWatcherDllDlg dialog

CWatcherDllDlg::CWatcherDllDlg(CWnd* pParent /*=NULL*/) :
	CDialog(CWatcherDllDlg::IDD, pParent),
    m_uServiceTimer( 0 )
{
	//{{AFX_DATA_INIT(CWatcherDllDlg)
	m_bGameRunning = FALSE;
	m_uFrameCountCurrent = 0;
	m_uFrameRateCurrent = 0;
	m_uFrameRate = 0;
	m_bFogEnable = FALSE;
	m_uFogParamType = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWatcherDllDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWatcherDllDlg)
	DDX_Control(pDX, IDC_EDIT_FOG_PARAM_TYPE, m_edtFogParamType);
	DDX_Control(pDX, IDC_STATIC_FOG_PARAM_TYPE, m_stcFogParamType);
	DDX_Control(pDX, IDC_STATIC_FOG_PARAM, m_stcFogParam);
	DDX_Control(pDX, IDC_STATIC_FOG_COLOUR_RED, m_stcFogColourRed);
	DDX_Control(pDX, IDC_STATIC_FOG_COLOUR_GREEN, m_stcFogColourGreen);
	DDX_Control(pDX, IDC_STATIC_FOG_COLOUR_BLUE, m_stcFogColourBlue);
	DDX_Control(pDX, IDC_STATIC_FOG_COLOUR, m_stcFogColour);
	DDX_Control(pDX, IDC_STATIC_FOG, m_stcFog);
	DDX_Control(pDX, IDC_SLIDER_FOG_COLOUR_RED, m_sldFogColourRed);
	DDX_Control(pDX, IDC_SLIDER_FOG_COLOUR_GREEN, m_sldFogColourGreen);
	DDX_Control(pDX, IDC_SLIDER_FOG_COLOUR_BLUE, m_sldFogColourBlue);
	DDX_Control(pDX, IDC_CHECK_FOG_ENABLE, m_chkFogEnable);
	DDX_Control(pDX, IDC_BUTTON_FOG_PARAM_TYPE, m_butFogParamType);
	DDX_Control(pDX, IDC_EDIT_FRAME_RATE, m_edtFrameRate);
	DDX_Control(pDX, IDC_COMBO_SPEED, m_cbxGameSpeed);
	DDX_Control(pDX, IDC_STATIC_SPEED, m_stcSpeed);
	DDX_Control(pDX, IDC_STATIC_FRAME_RATE, m_stcFrameRate);
	DDX_Control(pDX, IDC_STATIC_FRAME_COUNT, m_stcFrameCount);
	DDX_Control(pDX, IDC_EDIT_FRAME_COUNT_CURRENT, m_edtFrameCount);
	DDX_Control(pDX, IDC_BUTTON_UPDATE_VECTOR, m_butUpdateVector);
	DDX_Control(pDX, IDC_TAB_SECTION, m_tabSection);
	DDX_Control(pDX, IDC_CHECK_GAME_RUNNING, m_chkGameRunning);
	DDX_Control(pDX, IDC_SLIDER_FRAME_RATE, m_sldFrameRate);
	DDX_Check(pDX, IDC_CHECK_GAME_RUNNING, m_bGameRunning);
	DDX_Text(pDX, IDC_EDIT_FRAME_COUNT_CURRENT, m_uFrameCountCurrent);
	DDX_Text(pDX, IDC_EDIT_FRAME_RATE, m_uFrameRate);
	DDX_Check(pDX, IDC_CHECK_FOG_ENABLE, m_bFogEnable);
//	DDX_Text(pDX, IDC_EDIT_FOG_PARAM_TYPE, m_uFogParamType);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWatcherDllDlg, CDialog)
	//{{AFX_MSG_MAP(CWatcherDllDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHECK_GAME_RUNNING, OnCheckGameRunning)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_SECTION, OnSelchangeTabSection)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_FRAME_RATE, OnReleasedcaptureSliderFrameRate)
	ON_BN_CLICKED(IDC_BUTTON_UPDATE_VECTOR, OnButtonUpdateVector)
	ON_CBN_SELCHANGE(IDC_COMBO_SPEED, OnSelchangeComboSpeed)
	ON_BN_CLICKED(IDC_CHECK_FOG_ENABLE, OnCheckFogEnable)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_FOG_COLOUR_BLUE, OnReleasedcaptureSliderFogColourBlue)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_FOG_COLOUR_GREEN, OnReleasedcaptureSliderFogColourGreen)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_FOG_COLOUR_RED, OnReleasedcaptureSliderFogColourRed)
	ON_BN_CLICKED(IDC_BUTTON_FOG_PARAM_TYPE, OnButtonFogParamType)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWatcherDllDlg message handlers

BOOL CWatcherDllDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
    //
    // Initialize the watch server
    //
    ::radWatchServerInitialize( NULL, 500, ConnectionStatusCallBack, OnWPChangeCallBack, OnRecieveWPValueCallBack, this );

    m_uServiceTimer = SetTimer( 1, 50, NULL );

    CString strTitle;
    strTitle.Format( "Watcher dll sample - %s Disconnected", ::radWatchServerGetTargetName( ) );

    SetWindowText( strTitle );

    m_tabSection.InsertItem( 0, "Global" );
    m_tabSection.InsertItem( 1, "game" );

    VERIFY( SetWindowLong( m_sldFrameRate, GWL_STYLE, m_sldFrameRate.GetStyle( ) | TBS_TOOLTIPS ) != 0 );

    m_stcFogParamType.EnableWindow( FALSE );
    m_stcFogParam.EnableWindow( FALSE );
    m_stcFogColourRed.EnableWindow( FALSE );
    m_stcFogColourGreen.EnableWindow( FALSE );
    m_stcFogColourBlue.EnableWindow( FALSE );
    m_stcFogColour.EnableWindow( FALSE );
    m_stcFog.EnableWindow( FALSE );
    m_sldFogColourRed.EnableWindow( FALSE );
    m_sldFogColourGreen.EnableWindow( FALSE );
    m_sldFogColourBlue.EnableWindow( FALSE );
    m_edtFogParamType.EnableWindow( FALSE );
    m_chkFogEnable.EnableWindow( FALSE );
    m_butFogParamType.EnableWindow( FALSE );

    m_stcSpeed.EnableWindow( FALSE );
    m_stcFrameRate.EnableWindow( FALSE );
    m_stcFrameCount.EnableWindow( FALSE );
    m_butUpdateVector.EnableWindow( FALSE );
    m_edtFrameCount.EnableWindow( FALSE );
    m_chkGameRunning.EnableWindow( FALSE );
    m_sldFrameRate.EnableWindow( FALSE );
    m_cbxGameSpeed.EnableWindow( FALSE );
    m_edtFrameRate.EnableWindow( FALSE );

    m_tabSection.SetCurSel( 0 );
    LRESULT lr;
    OnSelchangeTabSection( NULL, & lr );

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CWatcherDllDlg::OnSysCommand( UINT nID, LPARAM lParam )
{
	if ( ( nID & 0xFFF0 ) == IDM_ABOUTBOX )
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal( );
	}
	else
	{
		CDialog::OnSysCommand( nID, lParam );
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWatcherDllDlg::OnPaint( ) 
{
	if ( IsIconic( ) )
	{
		CPaintDC dc( this ); // device context for painting

		SendMessage( WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc( ), 0 );

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics( SM_CXICON );
		int cyIcon = GetSystemMetrics( SM_CYICON );
		CRect rect;
		GetClientRect( &rect );
		int x = (rect.Width( ) - cxIcon + 1) / 2;
		int y = (rect.Height( ) - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon( x, y, m_hIcon );
	}
	else
	{
		CDialog::OnPaint( );
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWatcherDllDlg::OnQueryDragIcon( )
{
	return (HCURSOR) m_hIcon;
}

void CWatcherDllDlg::OnTimer( UINT nIDEvent ) 
{
    if ( m_uServiceTimer == nIDEvent )
    {
        ::radWatchServerService( );
    }
	CDialog::OnTimer( nIDEvent );
}

void CWatcherDllDlg::OnDestroy( ) 
{
	CDialog::OnDestroy( );

    //
    // Terminate the watch server.
    //
    ::radWatchServerTerminate( );
}

//
// updating value responds to user input.
//
void CWatcherDllDlg::OnCheckGameRunning() 
{
    if ( m_chkGameRunning.GetCheck( ) != BST_CHECKED )
    {
        ::radWatchServerEnumerateWatchPoints( WatchPointEnumerationCallBack, this );
        radWatchServerWPValue Value;
        memset( & Value, 0, sizeof( Value ) );
        Value.m_DataType = radWatchServerWPDBoolean;
        Value.m_DataValue.m_BooleanValue = false;
        ::radWatchServerUpdateWPValue( "Game Running", & Value );
    }
}

void CWatcherDllDlg::OnReleasedcaptureSliderFrameRate(NMHDR* pNMHDR, LRESULT* pResult) 
{
    radWatchServerWPValue Value;
    memset( & Value, 0, sizeof( Value ) );
    Value.m_DataType = radWatchServerWPDUnsignedInt;
    Value.m_DataValue.m_UIntValue = m_sldFrameRate.GetPos( );
    ::radWatchServerUpdateWPValue( "\\\\Frame Rate", & Value );

    *pResult = 0;
}

void CWatcherDllDlg::OnButtonUpdateVector() 
{
    radWatchServerWPValue Value;
    memset( & Value, 0, sizeof( Value ) );
    Value.m_DataType = radWatchServerWPDFunction;
    ::radWatchServerUpdateWPValue( "\\\\Update Vector", & Value );

	Value.m_DataType = radWatchServerWPDString;
	Value.m_DataValue.m_StringValue = "Testing 123";
	::radWatchServerUpdateWPValue( "\\\\Game\\Timer\\ElaspedTime", & Value );
}

void CWatcherDllDlg::OnSelchangeComboSpeed() 
{
    radWatchServerWPValue Value;
    memset( & Value, 0, sizeof( Value ) );
    Value.m_DataType = radWatchServerWPDEnum;
    Value.m_DataValue.m_EnumValue = m_cbxGameSpeed.GetItemData( m_cbxGameSpeed.GetCurSel( ) );
    ::radWatchServerUpdateWPValue( "\\\\Speed", & Value );
}

void CWatcherDllDlg::OnCheckFogEnable() 
{
    radWatchServerWPValue Value;
    memset( & Value, 0, sizeof( Value ) );

    Value.m_DataType = radWatchServerWPDBoolean;
    Value.m_DataValue.m_BooleanValue = ( m_chkFogEnable.GetCheck( ) == 1 );
    ::radWatchServerUpdateWPValue( "\\\\game\\fog\\enable\\enable", & Value );
}

void CWatcherDllDlg::OnReleasedcaptureSliderFogColourBlue(NMHDR* pNMHDR, LRESULT* pResult) 
{
    radWatchServerWPValue Value;
    memset( & Value, 0, sizeof( Value ) );

    Value.m_DataType = radWatchServerWPDUnsignedInt;
    Value.m_DataValue.m_UIntValue = m_sldFogColourBlue.GetPos( );
    ::radWatchServerUpdateWPValue( "\\\\game\\fog\\color\\blue", & Value );
    
    *pResult = 0;
}

void CWatcherDllDlg::OnReleasedcaptureSliderFogColourGreen(NMHDR* pNMHDR, LRESULT* pResult) 
{
    radWatchServerWPValue Value;
    memset( & Value, 0, sizeof( Value ) );

    Value.m_DataType = radWatchServerWPDUnsignedInt;
    Value.m_DataValue.m_UIntValue = m_sldFogColourGreen.GetPos( );
    ::radWatchServerUpdateWPValue( "\\\\game\\fog\\color\\green", & Value );
    
	*pResult = 0;
}

void CWatcherDllDlg::OnReleasedcaptureSliderFogColourRed(NMHDR* pNMHDR, LRESULT* pResult) 
{
    radWatchServerWPValue Value;
    memset( & Value, 0, sizeof( Value ) );

    Value.m_DataType = radWatchServerWPDUnsignedInt;
    Value.m_DataValue.m_UIntValue = m_sldFogColourRed.GetPos( );
    ::radWatchServerUpdateWPValue( "\\\\game\\fog\\color\\red", & Value );
    
	*pResult = 0;
}

void CWatcherDllDlg::OnButtonFogParamType() 
{
    radWatchServerWPValue Value;
    memset( & Value, 0, sizeof( Value ) );

    Value.m_DataType = radWatchServerWPDUnsignedInt;
    CString strText;
    m_edtFogParamType.GetWindowText( strText );
    Value.m_DataValue.m_UIntValue = atoi( strText );
    ::radWatchServerUpdateWPValue( "\\\\game\\fog\\parameter\\type", & Value );
}

void CWatcherDllDlg::ConnectionStatusCallBack( bool connected, void * pUserData )
{
    CWatcherDllDlg * pThis = ( CWatcherDllDlg * )pUserData;
    if ( connected )
    {
        CString strTitle;
        strTitle.Format( "Watcher dll sample [%s] Connected.", ::radWatchServerGetTargetName( ) );
        pThis->SetWindowText( strTitle );
    }
    else
    {
        CString strTitle;
        strTitle.Format( "Watcher dll sample [%s] Disconnected.", ::radWatchServerGetTargetName( ) );
        pThis->SetWindowText( strTitle );
    }
}

//
// callback handler
//
void CWatcherDllDlg::OnWPChangeCallBack( const char * pWPName, bool isCreate, void * pUserData )
{
    CWatcherDllDlg * pThis = ( CWatcherDllDlg * )pUserData;
    if ( strcmp( pWPName, "\\\\Speed" ) == 0 )
    {
        //
        // Update Speed combobox control
        //
        if ( isCreate )
        {
            radWatchServerWPAttribute oAttribute;
            memset( & oAttribute, 0, sizeof( oAttribute ) );

            if ( radWatchServerSelectWPAttribute( "\\\\Speed", & oAttribute ) )
            {
                ASSERT( oAttribute.m_DataType == radWatchServerWPDEnum );

                pThis->m_stcSpeed.EnableWindow( TRUE );
                pThis->m_cbxGameSpeed.EnableWindow( TRUE );

                while ( pThis->m_cbxGameSpeed.GetCount( ) > 0 )
                {
                    pThis->m_cbxGameSpeed.DeleteString( 0 );
                }

                for ( unsigned int i = 0; i < oAttribute.m_DataAttrib.m_EnumAttr.m_EnumCount; i ++ )
                {
                    pThis->m_cbxGameSpeed.InsertString( i, oAttribute.m_DataAttrib.m_EnumAttr.m_ppEnumNameList[ i ] );
                    pThis->m_cbxGameSpeed.SetItemData( i, oAttribute.m_DataAttrib.m_EnumAttr.m_pEnumValueList[ i ] );
                }

                radWatchServerFreeWPAttribute( & oAttribute );
            }
        }
        else
        {
            pThis->m_stcSpeed.EnableWindow( TRUE );
            pThis->m_cbxGameSpeed.EnableWindow( FALSE );
        }
    }
    else if ( strcmp( pWPName, "\\\\Frame Rate" ) == 0 )
    {
        //
        // Update Frame Rate
        //
        if ( isCreate )
        {
            radWatchServerWPAttribute oAttribute;
            memset( & oAttribute, 0, sizeof( oAttribute ) );

            if ( radWatchServerSelectWPAttribute( "\\\\Frame Rate", & oAttribute ) )
            {
                ASSERT( oAttribute.m_DataType == radWatchServerWPDUnsignedInt );

                pThis->m_stcFrameRate.EnableWindow( TRUE );
                pThis->m_sldFrameRate.EnableWindow( TRUE );
                pThis->m_edtFrameRate.EnableWindow( TRUE );

                pThis->m_sldFrameRate.SetRange( oAttribute.m_DataAttrib.m_UIntAttr.m_Min, oAttribute.m_DataAttrib.m_UIntAttr.m_Max );
                radWatchServerFreeWPAttribute( & oAttribute );
            }
        }
        else
        {
            pThis->m_stcFrameRate.EnableWindow( TRUE );
            pThis->m_sldFrameRate.EnableWindow( FALSE );
            pThis->m_edtFrameRate.EnableWindow( FALSE );
        }
    }
    else if ( strcmp( pWPName, "\\\\Frame Count" ) == 0 )
    {
        if ( isCreate )
        {
            pThis->m_stcFrameCount.EnableWindow( TRUE );
            pThis->m_edtFrameCount.EnableWindow( TRUE );
        }
        else
        {
            pThis->m_stcFrameCount.EnableWindow( TRUE );
            pThis->m_edtFrameCount.EnableWindow( FALSE );
        }
    }
    else if ( strcmp( pWPName, "\\\\Update Vector" ) == 0 )
    {
        if ( isCreate )
        {
            pThis->m_butUpdateVector.EnableWindow( TRUE );
        }
        else
        {
            pThis->m_butUpdateVector.EnableWindow( FALSE );
        }
    }
    else if ( strcmp( pWPName, "\\\\Game Running" ) == 0 )
    {
        if ( isCreate )
        {
            pThis->m_chkGameRunning.EnableWindow( TRUE );
        }
        else
        {
            pThis->m_chkGameRunning.EnableWindow( FALSE );
        }
    }
    else if ( strcmp( pWPName, "\\\\game\\fog\\enable\\enable" ) == 0 )
    {
        if ( isCreate )
        {
            pThis->m_stcFog.EnableWindow( TRUE );
            pThis->m_chkFogEnable.EnableWindow( TRUE );
        }
        else
        {
            pThis->m_stcFog.EnableWindow( FALSE );
            pThis->m_chkFogEnable.EnableWindow( FALSE );
        }
    }
    else if ( strcmp( pWPName, "\\\\game\\fog\\color\\red" ) == 0 )
    {
        if ( isCreate )
        {
            pThis->m_stcFogColour.EnableWindow( TRUE );
            pThis->m_stcFogColourRed.EnableWindow( TRUE );
            pThis->m_sldFogColourRed.EnableWindow( TRUE );
        }
        else
        {
            pThis->m_stcFogColour.EnableWindow( FALSE );
            pThis->m_stcFogColourRed.EnableWindow( FALSE );
            pThis->m_sldFogColourRed.EnableWindow( FALSE );
        }
    }
    else if ( strcmp( pWPName, "\\\\game\\fog\\color\\green" ) == 0 )
    {
        if ( isCreate )
        {
            pThis->m_stcFogColourGreen.EnableWindow( TRUE );
            pThis->m_sldFogColourGreen.EnableWindow( TRUE );
        }
        else
        {
            pThis->m_stcFogColourGreen.EnableWindow( FALSE );
            pThis->m_sldFogColourGreen.EnableWindow( FALSE );
        }
    }
    else if ( strcmp( pWPName, "\\\\game\\fog\\color\\blue" ) == 0 )
    {
        if ( isCreate )
        {
            pThis->m_stcFogColourBlue.EnableWindow( TRUE );
            pThis->m_sldFogColourBlue.EnableWindow( TRUE );
        }
        else
        {
            pThis->m_stcFogColourBlue.EnableWindow( FALSE );
            pThis->m_sldFogColourBlue.EnableWindow( FALSE );
        }
    }
    else if ( strcmp( pWPName, "\\\\game\\fog\\parameter\\type" ) == 0 )
    {
        if ( isCreate )
        {
            pThis->m_stcFogParam.EnableWindow( TRUE );
            pThis->m_edtFogParamType.EnableWindow( TRUE );
            pThis->m_stcFogParamType.EnableWindow( TRUE );
            pThis->m_butFogParamType.EnableWindow( TRUE );
        }
        else
        {
            pThis->m_stcFogParam.EnableWindow( FALSE );
            pThis->m_edtFogParamType.EnableWindow( FALSE );
            pThis->m_stcFogParamType.EnableWindow( FALSE );
            pThis->m_butFogParamType.EnableWindow( FALSE );
        }
    }
}

void CWatcherDllDlg::OnRecieveWPValueCallBack( const char * pWPName, void * pUserData )
{
    CWatcherDllDlg * pThis = ( CWatcherDllDlg * )pUserData;

    radWatchServerWPValue value;

	AfxTrace( "WPName[%s]\n", pWPName );
    if ( strcmp( "\\\\Frame Count", pWPName ) == 0 )
    {
        memset( & value, 0, sizeof( value ) );

        if ( ::radWatchServerSelectWPValue( "\\\\Frame Count", & value ) )
        {
            pThis->m_uFrameCountCurrent = value.m_DataValue.m_UIntValue;
            ::radWatchServerFreeWPValue( & value );
            pThis->UpdateData( FALSE );
        }
    }
    else if ( strcmp( "\\\\Game Running", pWPName ) == 0 )
    {
        memset( & value, 0, sizeof( value ) );

        if ( ::radWatchServerSelectWPValue( "\\\\Game Running", & value ) )
        {
            pThis->m_bGameRunning = value.m_DataValue.m_BooleanValue;
            ::radWatchServerFreeWPValue( & value );
            pThis->UpdateData( FALSE );
        }
    }
    else if ( strcmp( "\\\\Speed", pWPName ) == 0 )
    {
        memset( & value, 0, sizeof( value ) );

        if ( ::radWatchServerSelectWPValue( "\\\\Speed", & value ) )
        {
            for ( int i = 0; i < pThis->m_cbxGameSpeed.GetCount( ); i ++ )
            {
                if ( (signed int)( pThis->m_cbxGameSpeed.GetItemData( i ) ) == value.m_DataValue.m_EnumValue )
                {
                    pThis->m_cbxGameSpeed.SetCurSel( i );
                }
            }
            ::radWatchServerFreeWPValue( & value );
            pThis->UpdateData( FALSE );
        }
    }
    else if ( strcmp( pWPName, "\\\\Frame Rate" ) == 0 )
    {
        memset( & value, 0, sizeof( value ) );

        if ( radWatchServerSelectWPValue( "\\\\Frame Rate", & value ) )
        {
            pThis->m_sldFrameRate.SetPos( value.m_DataValue.m_UIntValue );
            pThis->m_uFrameRate = value.m_DataValue.m_UIntValue;
            radWatchServerFreeWPValue( & value );
            pThis->UpdateData( FALSE );
        }
    }
    else if ( strcmp( pWPName, "\\\\game\\fog\\enable\\enable" ) == 0 )
    {
        memset( & value, 0, sizeof( value ) );

        if ( radWatchServerSelectWPValue( "\\\\game\\fog\\enable\\enable", & value ) )
        {
            pThis->m_bFogEnable = value.m_DataValue.m_BooleanValue;
            radWatchServerFreeWPValue( & value );
            pThis->UpdateData( FALSE );
        }
    }
    else if ( strcmp( pWPName, "\\\\game\\fog\\color\\red" ) == 0 )
    {
        memset( & value, 0, sizeof( value ) );

        if ( radWatchServerSelectWPValue( "\\\\game\\fog\\color\\red", & value ) )
        {
            pThis->m_sldFogColourRed.SetPos( value.m_DataValue.m_UIntValue );
            radWatchServerFreeWPValue( & value );
            pThis->UpdateData( FALSE );
        }
    }
    else if ( strcmp( pWPName, "\\\\game\\fog\\color\\green" ) == 0 )
    {
        memset( & value, 0, sizeof( value ) );

        if ( radWatchServerSelectWPValue( "\\\\game\\fog\\color\\green", & value ) )
        {
            pThis->m_sldFogColourGreen.SetPos( value.m_DataValue.m_UIntValue );
            radWatchServerFreeWPValue( & value );
            pThis->UpdateData( FALSE );
        }
    }
    else if ( strcmp( pWPName, "\\\\game\\fog\\color\\blue" ) == 0 )
    {
        memset( & value, 0, sizeof( value ) );

        if ( radWatchServerSelectWPValue( "\\\\game\\fog\\color\\blue", & value ) )
        {
            pThis->m_sldFogColourBlue.SetPos( value.m_DataValue.m_UIntValue );
            radWatchServerFreeWPValue( & value );
            pThis->UpdateData( FALSE );
        }
    }
    else if ( strcmp( pWPName, "\\\\game\\fog\\parameter\\type" ) == 0 )
    {
        memset( & value, 0, sizeof( value ) );

        if ( radWatchServerSelectWPValue( "\\\\game\\fog\\parameter\\type", & value ) )
        {
            pThis->m_uFogParamType = value.m_DataValue.m_UIntValue;
            CString strText;
            strText.Format( "%d", pThis->m_uFogParamType );
            pThis->m_edtFogParamType.SetWindowText( strText );
            radWatchServerFreeWPValue( & value );
        }
    }
	else if ( strcmp( pWPName, "\\\\Game\\Timer\\ElaspedTime" ) == 0 )
	{
        memset( & value, 0, sizeof( value ) );

        if ( radWatchServerSelectWPValue( "\\\\Game\\Timer\\ElaspedTime", & value ) )
        {
			CString strTitle;
			strTitle.Format( "Watcher dll sample - %s [%s]", ::radWatchServerGetTargetName( ), value.m_DataValue.m_StringValue );

			pThis->SetWindowText( strTitle );
            radWatchServerFreeWPValue( & value );
        }
	}
}

void CWatcherDllDlg::WatchPointEnumerationCallBack( const char * pWPName, void * pUserData )
{
    CWatcherDllDlg * pThis = (CWatcherDllDlg*)pUserData;
    AfxTrace( "%s\n", pWPName );
}

void CWatcherDllDlg::OnSelchangeTabSection(NMHDR* pNMHDR, LRESULT* pResult) 
{
    int nSelection = m_tabSection.GetCurSel( );

    m_stcFogParamType.ShowWindow( SW_HIDE );
    m_stcFogParam.ShowWindow( SW_HIDE );
    m_stcFogColourRed.ShowWindow( SW_HIDE );
    m_stcFogColourGreen.ShowWindow( SW_HIDE );
    m_stcFogColourBlue.ShowWindow( SW_HIDE );
    m_stcFogColour.ShowWindow( SW_HIDE );
    m_stcFog.ShowWindow( SW_HIDE );
    m_sldFogColourRed.ShowWindow( SW_HIDE );
    m_sldFogColourGreen.ShowWindow( SW_HIDE );
    m_sldFogColourBlue.ShowWindow( SW_HIDE );
    m_edtFogParamType.ShowWindow( SW_HIDE );
    m_chkFogEnable.ShowWindow( SW_HIDE );
    m_butFogParamType.ShowWindow( SW_HIDE );

    m_stcSpeed.ShowWindow( SW_HIDE );
    m_stcFrameRate.ShowWindow( SW_HIDE );
    m_stcFrameCount.ShowWindow( SW_HIDE );
    m_butUpdateVector.ShowWindow( SW_HIDE );
    m_edtFrameCount.ShowWindow( SW_HIDE );
    m_chkGameRunning.ShowWindow( SW_HIDE );
    m_sldFrameRate.ShowWindow( SW_HIDE );
    m_cbxGameSpeed.ShowWindow( SW_HIDE );
    m_edtFrameRate.ShowWindow( SW_HIDE );

    if ( nSelection == 0 )
    {
        m_stcSpeed.ShowWindow( SW_SHOW );
        m_stcFrameRate.ShowWindow( SW_SHOW );
        m_stcFrameCount.ShowWindow( SW_SHOW );
        m_chkGameRunning.ShowWindow( SW_SHOW );
        m_edtFrameCount.ShowWindow( SW_SHOW );
        m_sldFrameRate.ShowWindow( SW_SHOW );
        m_butUpdateVector.ShowWindow( SW_SHOW );
        m_cbxGameSpeed.ShowWindow( SW_SHOW );
        m_edtFrameRate.ShowWindow( SW_SHOW );
    }
    else if ( nSelection == 1 )
    {
        m_stcFogParamType.ShowWindow( SW_SHOW );
        m_stcFogParam.ShowWindow( SW_SHOW );
        m_stcFogColourRed.ShowWindow( SW_SHOW );
        m_stcFogColourGreen.ShowWindow( SW_SHOW );
        m_stcFogColourBlue.ShowWindow( SW_SHOW );
        m_stcFogColour.ShowWindow( SW_SHOW );
        m_stcFog.ShowWindow( SW_SHOW );
        m_sldFogColourRed.ShowWindow( SW_SHOW );
        m_sldFogColourGreen.ShowWindow( SW_SHOW );
        m_sldFogColourBlue.ShowWindow( SW_SHOW );
        m_edtFogParamType.ShowWindow( SW_SHOW );
        m_chkFogEnable.ShowWindow( SW_SHOW );
        m_butFogParamType.ShowWindow( SW_SHOW );
    }

	*pResult = 0;
}

