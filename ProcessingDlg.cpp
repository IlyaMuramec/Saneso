// ProcessingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NGEuserApp.h"
#include "NGEuserDlg.h"
#include "ProcessingDlg.h"
#include "SelectItemDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProcessingDlg dialog


CProcessingDlg::CProcessingDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProcessingDlg::IDD, pParent)
	, m_nMosaicWidth(1000)
	, m_nMosaicHeight(1000)
	, m_bBlendingEnabled(FALSE)
{
	//{{AFX_DATA_INIT(CProcessingDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CProcessingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProcessingDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProcessingDlg, CDialog)
	//{{AFX_MSG_MAP(CProcessingDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_WM_SHOWWINDOW()
	ON_MESSAGE(WM_UPDATE_MODE_MSG, &CProcessingDlg::OnUpdateModeMessage)
	ON_BN_CLICKED(IDC_BUTTON_SAVE, &CProcessingDlg::OnBnClickedButtonSave)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, &CProcessingDlg::OnBnClickedButtonLoad)
	ON_BN_CLICKED(IDC_CHECK_BLEND_IMAGES, &CProcessingDlg::OnBnClickedCheckBlendImages)
	ON_EN_KILLFOCUS(IDC_EDIT_MOSAIC_WIDTH, &CProcessingDlg::OnEnKillfocusEditMosaicWidth)
	ON_EN_KILLFOCUS(IDC_EDIT_MOSAIC_HEIGHT, &CProcessingDlg::OnEnKillfocusEditMosaicHeight)
	
	ON_BN_CLICKED(IDC_BUTTON_PLAYBACK, &CProcessingDlg::OnBnClickedButtonPlayback)
	ON_BN_CLICKED(IDC_BUTTON_DO_CAPTURE, &CProcessingDlg::OnBnClickedButtonDoCapture)
	ON_BN_CLICKED(IDC_BUTTON_RECORD, &CProcessingDlg::OnBnClickedButtonRecord)
	ON_BN_CLICKED(IDC_BUTTON_LOAD_CAPTURE, &CProcessingDlg::OnBnClickedButtonLoadCapture)
	ON_BN_CLICKED(IDC_BUTTON_LIVE_STREAM, &CProcessingDlg::OnBnClickedButtonLiveStream)
	ON_BN_CLICKED(IDC_BUTTON_WRITE_TO_SCOPE, &CProcessingDlg::OnBnClickedButtonWriteToScope)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProcessingDlg message handlers

BOOL CProcessingDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CProcessingDlg::OnShowWindow(BOOL bShow, 
							   UINT nStatus)
{
	if (bShow)
	{
		MosaicData *pMosaicData = theApp.GetSystemManager()->GetMosaicData();
		m_bBlendingEnabled = pMosaicData->IsBlendingEnabled();

		cv::Size size = pMosaicData->GetSize();
		m_nMosaicWidth = size.width;
		m_nMosaicHeight = size.height;

		OnUpdateModeMessage(NULL, NULL);

		CheckWriteToScopeEnable();

		UpdateData(FALSE);
	}
	else
	{
	}
}

void CProcessingDlg::OnBnClickedButtonLoad()
{
#if 0
	char strFilter[] = { "Settings File (*.json)|*.json|" }; 
	CFileDialog FileDlg(TRUE, CString(".json"), theApp.GetSystemManager()->GetConfigFile(), 0, CString(strFilter));

	if( FileDlg.DoModal() == IDOK )
	{
		theApp.GetSystemManager()->LoadConfig(FileDlg.GetPathName());
	}
#else
	CSelectItemDlg SelectItemDlg;
	SelectItemDlg.SetTitle(_T("Select Config"));
	SelectItemDlg.IsLoading(true);
	CString configFolder = _T("C:\\NGE\\Config\\");
	SelectItemDlg.SetSearchFolder(configFolder);
	SelectItemDlg.DisplayFolders(TRUE);
	if (SelectItemDlg.DoModal() == IDOK)
	{
		CString selection = SelectItemDlg.GetSelectedItem();
		CString folder = configFolder + selection +_T("\\");
		theApp.GetSystemManager()->LoadConfig(folder);
	}

#endif
}

void CProcessingDlg::OnBnClickedButtonSave()
{
#if 0
	char strFilter[] = { "Settings File (*.json)|*.json|" }; 
	CFileDialog FileDlg(FALSE, CString(".json"), theApp.GetSystemManager()->GetConfigFile(), 0, CString(strFilter));

	if( FileDlg.DoModal() == IDOK )
	{
		theApp.GetSystemManager()->SaveConfig(FileDlg.GetPathName());
	}
#else
	CSelectItemDlg SelectItemDlg;
	SelectItemDlg.SetTitle(_T("Select Config"));
	SelectItemDlg.IsLoading(false);
	CString configFolder = _T("C:\\NGE\\Config\\");
	SelectItemDlg.SetSearchFolder(configFolder);
	SelectItemDlg.DisplayFolders(TRUE);
	if (SelectItemDlg.DoModal() == IDOK)
	{
		CString selection = SelectItemDlg.GetSelectedItem();
		CString folder = configFolder + selection + _T("\\");
		theApp.GetSystemManager()->SaveConfig(folder);
	}
#endif
}


void CProcessingDlg::OnBnClickedCheckBlendImages()
{
	UpdateData();

	MosaicData *pMosaicData = theApp.GetSystemManager()->GetMosaicData();
	pMosaicData->EnableBlending(m_bBlendingEnabled);
}


void CProcessingDlg::OnEnKillfocusEditMosaicWidth()
{
	UpdateData();

	MosaicData *pMosaicData = theApp.GetSystemManager()->GetMosaicData();
	pMosaicData->SetSize(cv::Size(m_nMosaicWidth, m_nMosaicHeight));

	theApp.GetSystemManager()->UpdateAllCameraModels();
}


void CProcessingDlg::OnEnKillfocusEditMosaicHeight()
{
	UpdateData();

	MosaicData *pMosaicData = theApp.GetSystemManager()->GetMosaicData();
	pMosaicData->SetSize(cv::Size(m_nMosaicWidth, m_nMosaicHeight));

	theApp.GetSystemManager()->UpdateAllCameraModels();
}


void CProcessingDlg::OnBnClickedButtonPlayback()
{
	CSelectItemDlg SelectItemDlg;
	SelectItemDlg.SetTitle(_T("Select Video"));
	SelectItemDlg.IsLoading(true);
	CString videoFolder = theApp.GetVideoCaptureFolder();// _T("C:\\NGE\\Captures");
	SelectItemDlg.SetSearchFolder(videoFolder);
	SelectItemDlg.SetSearchString(_T("*.avi"));
	if (SelectItemDlg.DoModal() == IDOK)
	{
		CString selection = SelectItemDlg.GetSelectedItem();
		CString file = videoFolder + "\\" + selection;
		CStringA fileA = file;
		theApp.GetMainDlg()->PlaybackFile(fileA.GetBuffer());
	}
}


void CProcessingDlg::OnBnClickedButtonDoCapture()
{
	theApp.GetSystemManager()->CaptureImageFile();
}

void CProcessingDlg::OnBnClickedButtonLoadCapture()
{
	CSelectItemDlg SelectItemDlg;
	SelectItemDlg.SetTitle(_T("Select Image"));
	SelectItemDlg.IsLoading(true);
	CString imageFolder = theApp.GetImageCaptureFolder();// _T("C:\\NGE\\Captures");
	SelectItemDlg.SetSearchFolder(imageFolder);
	SelectItemDlg.SetSearchString(_T("*.png"));
	if (SelectItemDlg.DoModal() == IDOK)
	{
		CString selection = SelectItemDlg.GetSelectedItem();
		CString file = imageFolder + "\\" + selection;
		CStringA fileA = file;
		theApp.GetMainDlg()->DisplayImage(fileA.GetBuffer());
	}
}


void CProcessingDlg::OnBnClickedButtonRecord()
{
	DisplayMode currentDisplayMode = theApp.GetSystemManager()->GetDisplayMode();
	if (currentDisplayMode == eRecording)
	{
		theApp.GetMainDlg()->StopCapture();
	}
	else
	{
		theApp.GetSystemManager()->CaptureVideoFile();
	}
}

void CProcessingDlg::OnBnClickedButtonLiveStream()
{
	DisplayMode currentDisplayMode = theApp.GetSystemManager()->GetDisplayMode();
	if (currentDisplayMode != eLive)
	{
		theApp.GetSystemManager()->SetNewDisplayMode(eLive);
	}
	else
	{
		theApp.GetSystemManager()->SetNewDisplayMode(eFrozen);
	}
}

LRESULT CProcessingDlg::OnUpdateModeMessage(WPARAM control, LPARAM value)
{
	CButton *pLive = (CButton*)GetDlgItem(IDC_BUTTON_LIVE_STREAM);
	CButton *pPlayback = (CButton*)GetDlgItem(IDC_BUTTON_PLAYBACK);
	CButton *pRecord = (CButton*)GetDlgItem(IDC_BUTTON_RECORD);
	CButton *pLoad = (CButton*)GetDlgItem(IDC_BUTTON_LOAD_CAPTURE);
	CButton *pCapture = (CButton*)GetDlgItem(IDC_BUTTON_DO_CAPTURE);

	DisplayMode currentDisplayMode = theApp.GetSystemManager()->GetDisplayMode();
	switch (currentDisplayMode)
	{
	case eLive:
		pLive->EnableWindow(TRUE);
		pPlayback->EnableWindow(TRUE);
		pRecord->EnableWindow(TRUE);
		pLoad->EnableWindow(TRUE);
		pCapture->EnableWindow(TRUE);

		pLive->SetWindowText(_T("Freeze Stream"));
		pRecord->SetWindowText(_T("Record"));
		break;
	case eFrozen:
		pLive->EnableWindow(TRUE);
		pPlayback->EnableWindow(TRUE);
		pRecord->EnableWindow(FALSE);
		pLoad->EnableWindow(TRUE);
		pCapture->EnableWindow(FALSE);

		pLive->SetWindowText(_T("Live Stream"));
		pRecord->SetWindowText(_T("Record"));
		break;
	case eRecording:
		pLive->EnableWindow(FALSE);
		pPlayback->EnableWindow(FALSE);
		pRecord->EnableWindow(TRUE);
		pLoad->EnableWindow(FALSE);
		pCapture->EnableWindow(FALSE);

		pLive->SetWindowText(_T("Live Stream"));
		pRecord->SetWindowText(_T("Stop Rec"));
		break;
	case ePlayback:
		pLive->EnableWindow(TRUE);
		pPlayback->EnableWindow(TRUE);
		pRecord->EnableWindow(FALSE);
		pLoad->EnableWindow(TRUE);
		pCapture->EnableWindow(FALSE);

		pLive->SetWindowText(_T("Live Stream"));
		pRecord->SetWindowText(_T("Record"));
		break;
	case eDisplayImage:
		pLive->EnableWindow(TRUE);
		pPlayback->EnableWindow(TRUE);
		pRecord->EnableWindow(FALSE);
		pLoad->EnableWindow(TRUE);
		pCapture->EnableWindow(FALSE);

		pLive->SetWindowText(_T("Live Stream"));
		pRecord->SetWindowText(_T("Record"));
		break;
	}

	return 0;
}

void CProcessingDlg::CheckWriteToScopeEnable()
{
	bool bSame = theApp.GetSystemManager()->IsScopeEEPromCurrent();

	GetDlgItem(IDC_BUTTON_WRITE_TO_SCOPE)->EnableWindow(!bSame);
}

void CProcessingDlg::OnBnClickedButtonWriteToScope()
{
	int answer = theApp.ShowMessageBox(_T("Are you sure you want to overwrite data in Scope EEProm?"), _T("Warning"), MB_OKCANCEL | MB_TOPMOST);

	if (answer == IDOK)
	{
		int retVal = theApp.GetSystemManager()->WriteCurrentToEEProm();

		if (retVal == 0)
			theApp.ShowMessageBox(_T("Write to Scope EEProm successful"), _T(""), MB_OK | MB_TOPMOST);
		else
			theApp.ShowMessageBox(_T("Write to Scope EEProm failed"), _T(""), MB_OK | MB_TOPMOST);

		CheckWriteToScopeEnable();
	}
}
