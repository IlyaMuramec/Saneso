// MainDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NGEuserApp.h"
#include "NGEuserDlg.h"
#include "LightingDlg.h"
#include "MainDlg.h"
#include "SelectItemDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static UINT UWM_MAIN_CONTROLLER_MSG = RegisterWindowMessage(_T("Main Controller Message"));
static void MicroControllerMessageCallback(void* pObject, CMessage *pMsg)
{
	if ((pMsg->type == MC_CONTROL_CHANGED) && (pMsg->size == 2))
	{
		CMainDlg *pDlg = (CMainDlg *)pObject;
		pDlg->PostMessage(UWM_MAIN_CONTROLLER_MSG, pMsg->msgData[0], pMsg->msgData[1]);
	}
}

static UINT UWM_MAIN_SELECT_BAND_IMAGING = RegisterWindowMessage(_T("Select Band Imaging"));
static void SelectBandImaging(void* pObject, bool enable)
{
	CLightingDlg *pDlg = (CLightingDlg *)pObject;
	WPARAM wparam = enable;
	pDlg->PostMessage(UWM_MAIN_SELECT_BAND_IMAGING, wparam, 0);
}

/////////////////////////////////////////////////////////////////////////////
// CMainDlg dialog


CMainDlg::CMainDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMainDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMainDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMainDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMainDlg, CDialog)
	//{{AFX_MSG_MAP(CMainDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_WM_DRAWITEM()
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_BUTTON_EDIT_SETTINGS, &CMainDlg::OnBnClickedButtonEditSettings)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_BAND_IMAGING, &CMainDlg::OnBnClickedButtonSelectBandImaging)
	ON_REGISTERED_MESSAGE(UWM_MAIN_CONTROLLER_MSG, &CMainDlg::OnMicroControllerMessage)
	ON_REGISTERED_MESSAGE(UWM_MAIN_SELECT_BAND_IMAGING, &CMainDlg::OnSelectBandImaging)
	ON_MESSAGE(WM_UPDATE_MODE_MSG, &CMainDlg::OnUpdateModeMessage)
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR1, &CMainDlg::OnBnClickedLedFrontColor1)
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR2, &CMainDlg::OnBnClickedLedFrontColor2)
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR3, &CMainDlg::OnBnClickedLedFrontColor3)
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR4, &CMainDlg::OnBnClickedLedFrontColor4)
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR5, &CMainDlg::OnBnClickedLedFrontColor5)
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR6, &CMainDlg::OnBnClickedLedFrontColor6)
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR7, &CMainDlg::OnBnClickedLedFrontColor7)
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR8, &CMainDlg::OnBnClickedLedFrontColor8)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR1, &CMainDlg::OnBnClickedLedSideColor1)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR2, &CMainDlg::OnBnClickedLedSideColor2)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR3, &CMainDlg::OnBnClickedLedSideColor3)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR4, &CMainDlg::OnBnClickedLedSideColor4)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR5, &CMainDlg::OnBnClickedLedSideColor5)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR6, &CMainDlg::OnBnClickedLedSideColor6)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR7, &CMainDlg::OnBnClickedLedSideColor7)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR8, &CMainDlg::OnBnClickedLedSideColor8)
	ON_BN_CLICKED(IDC_BUTTON_LIVE_STREAM, &CMainDlg::OnBnClickedButtonLiveStream)
	ON_BN_CLICKED(IDC_BUTTON_PLAYBACK, &CMainDlg::OnBnClickedButtonPlayback)
	ON_BN_CLICKED(IDC_BUTTON_RECORD, &CMainDlg::OnBnClickedButtonRecord)
	ON_BN_CLICKED(IDC_BUTTON_LOAD_CAPTURE, &CMainDlg::OnBnClickedButtonLoadCapture)
	ON_BN_CLICKED(IDC_BUTTON_DO_CAPTURE, &CMainDlg::OnBnClickedButtonDoCapture)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT_SETTINGS, &CMainDlg::OnBnClickedButtonDefaultSettings)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainDlg message handlers

BOOL CMainDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_pLightControl = theApp.GetSystemManager()->GetLightControl();
	m_pLightControl->AddSelectBandImagingCallBack(SelectBandImaging, this);
	theApp.GetSystemManager()->GetMicroController()->AddMessageCallback(MicroControllerMessageCallback, this);
	
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CMainDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	COLORREF color = RGB(0, 0, 0);
	COLORREF colorText = RGB(0, 0, 0);
	COLORREF selectedColor = RGB(255, 255, 0);
	BOOL selected = FALSE;

	switch (nIDCtl)
	{
	case IDC_BUTTON_SELECT_BAND_IMAGING: 
		selected = theApp.GetSystemManager()->GetLightControl()->IsSelectBandImagingEnabled();
		selected ? color = selectedColor : color = RGB(240, 240, 240);
		break;
	case IDC_LED_FRONT_COLOR1: color = m_pLightControl->GetColorValue(0); break;
	case IDC_LED_FRONT_COLOR2: color = m_pLightControl->GetColorValue(1); break;
	case IDC_LED_FRONT_COLOR3: color = m_pLightControl->GetColorValue(2); break;
	case IDC_LED_FRONT_COLOR4: color = m_pLightControl->GetColorValue(3); break;
	case IDC_LED_FRONT_COLOR5: color = m_pLightControl->GetColorValue(4); break;
	case IDC_LED_FRONT_COLOR6: color = m_pLightControl->GetColorValue(5); break;
	case IDC_LED_FRONT_COLOR7: color = m_pLightControl->GetColorValue(6); break;
	case IDC_LED_FRONT_COLOR8: color = m_pLightControl->GetColorValue(7); break;
	case IDC_LED_SIDE_COLOR1: color = m_pLightControl->GetColorValue(0); break;
	case IDC_LED_SIDE_COLOR2: color = m_pLightControl->GetColorValue(1); break;
	case IDC_LED_SIDE_COLOR3: color = m_pLightControl->GetColorValue(2); break;
	case IDC_LED_SIDE_COLOR4: color = m_pLightControl->GetColorValue(3); break;
	case IDC_LED_SIDE_COLOR5: color = m_pLightControl->GetColorValue(4); break;
	case IDC_LED_SIDE_COLOR6: color = m_pLightControl->GetColorValue(5); break;
	case IDC_LED_SIDE_COLOR7: color = m_pLightControl->GetColorValue(6); break;
	case IDC_LED_SIDE_COLOR8: color = m_pLightControl->GetColorValue(7); break;
	}

	BOOL enabled = GetDlgItem(nIDCtl)->IsWindowEnabled();
	if (!enabled)
	{
		color = RGB(240, 240, 240);
	}

	if (color != 0)
	{
		CDC dc;
		RECT rect;
		dc.Attach(lpDrawItemStruct->hDC);   // Get the Button DC to CDC

		rect = lpDrawItemStruct->rcItem;     //Store the Button rect to our local rect.

		dc.Draw3dRect(&rect, RGB(255, 255, 255), RGB(0, 0, 0));
		dc.FillSolidRect(&rect, color);//Here you can define the required color to appear on the Button.
		dc.SetBkColor(color);   //Setting the Text Background color
		dc.SetTextColor(RGB(0, 0, 0));     //Setting the Text Color

		UINT state = lpDrawItemStruct->itemState;  //This defines the state of the Push button either pressed or not. 
		if ((state & ODS_SELECTED)
			|| selected)
		{
			dc.DrawEdge(&rect, EDGE_SUNKEN, BF_RECT);

			// Draw a focus rect which indicates the user 
			// that the button is focused
			int iChange = 3;
			rect.top += iChange;
			rect.left += iChange;
			rect.right -= iChange;
			rect.bottom -= iChange;
			dc.DrawFocusRect(&rect);
		}
		else
		{
			dc.DrawEdge(&rect, EDGE_RAISED, BF_RECT);
		}

		TCHAR buffer[MAX_PATH];           //To store the Caption of the button.
		ZeroMemory(buffer, MAX_PATH);     //Intializing the buffer to zero
		::GetWindowText(lpDrawItemStruct->hwndItem, buffer, MAX_PATH); //Get the Caption of Button Window 

		dc.DrawText(buffer, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);//Redraw the  Caption of Button Window 

		dc.Detach();  // Detach the Button DC
	}
	CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CMainDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (bShow)
	{
		ShowColorButtonSelect();

		ClearFrontColorSelect();
		UpdateFrontColorSelect();

		ClearSideColorSelect();
		UpdateSideColorSelect();

		OnUpdateModeMessage(NULL, NULL);

		UpdateData(FALSE);
	}
}

LRESULT CMainDlg::OnMicroControllerMessage(WPARAM control, LPARAM value)
{
	Invalidate();

	return 0;
}

LRESULT CMainDlg::OnSelectBandImaging(WPARAM control, LPARAM value)
{
	bool enabled = (control == 1);

	ClearFrontColorSelect();
	EnableFrontColorSelect(!enabled);
	UpdateFrontColorSelect();

	ClearSideColorSelect();
	EnableSideColorSelect(!enabled);
	UpdateSideColorSelect();

	Invalidate();

	return 0;
}

void CMainDlg::OnBnClickedButtonSelectBandImaging()
{
	bool enabled = theApp.GetSystemManager()->IsSelectBandImagingEnabled();
	theApp.GetSystemManager()->EnableSelectBandImaging(!enabled);

	Invalidate();
}

void CMainDlg::OnBnClickedButtonEditSettings()
{
	if (!theApp.IsLoggedIn())
	{
		if (theApp.EditSettings())
		{
			GetDlgItem(IDC_BUTTON_EDIT_SETTINGS)->SetWindowText(_T("LOG OUT"));
		}
	}
	else
	{
		theApp.LogOut();
		GetDlgItem(IDC_BUTTON_EDIT_SETTINGS)->SetWindowText(_T("EDIT SETTINGS"));
	}

}

void CMainDlg::ShowColorButtonSelect()
{
	int numColors = m_pLightControl->GetNumColors();

	GetDlgItem(IDC_LED_FRONT_COLOR1)->ShowWindow(numColors >= 1);
	GetDlgItem(IDC_LED_FRONT_COLOR2)->ShowWindow(numColors >= 2);
	GetDlgItem(IDC_LED_FRONT_COLOR3)->ShowWindow(numColors >= 3);
	GetDlgItem(IDC_LED_FRONT_COLOR4)->ShowWindow(numColors >= 4);
	GetDlgItem(IDC_LED_FRONT_COLOR5)->ShowWindow(numColors >= 5);
	GetDlgItem(IDC_LED_FRONT_COLOR6)->ShowWindow(numColors >= 6);
	GetDlgItem(IDC_LED_FRONT_COLOR7)->ShowWindow(numColors >= 7);
	GetDlgItem(IDC_LED_FRONT_COLOR8)->ShowWindow(numColors >= 8);

	GetDlgItem(IDC_LED_SIDE_COLOR1)->ShowWindow(numColors >= 1);
	GetDlgItem(IDC_LED_SIDE_COLOR2)->ShowWindow(numColors >= 2);
	GetDlgItem(IDC_LED_SIDE_COLOR3)->ShowWindow(numColors >= 3);
	GetDlgItem(IDC_LED_SIDE_COLOR4)->ShowWindow(numColors >= 4);
	GetDlgItem(IDC_LED_SIDE_COLOR5)->ShowWindow(numColors >= 5);
	GetDlgItem(IDC_LED_SIDE_COLOR6)->ShowWindow(numColors >= 6);
	GetDlgItem(IDC_LED_SIDE_COLOR7)->ShowWindow(numColors >= 7);
	GetDlgItem(IDC_LED_SIDE_COLOR8)->ShowWindow(numColors >= 8);
}

void CMainDlg::ClearFrontColorSelect()
{
	((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR1))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR2))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR3))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR4))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR5))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR6))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR7))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR8))->SetState(FALSE);
}

void CMainDlg::EnableFrontColorSelect(BOOL enable)
{
	GetDlgItem(IDC_LED_FRONT_COLOR1)->EnableWindow(enable);
	GetDlgItem(IDC_LED_FRONT_COLOR2)->EnableWindow(enable);
	GetDlgItem(IDC_LED_FRONT_COLOR3)->EnableWindow(enable);
	GetDlgItem(IDC_LED_FRONT_COLOR4)->EnableWindow(enable);
	GetDlgItem(IDC_LED_FRONT_COLOR5)->EnableWindow(enable);
	GetDlgItem(IDC_LED_FRONT_COLOR6)->EnableWindow(enable);
	GetDlgItem(IDC_LED_FRONT_COLOR7)->EnableWindow(enable);
	GetDlgItem(IDC_LED_FRONT_COLOR8)->EnableWindow(enable);
}

void CMainDlg::ClearSideColorSelect()
{
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR1))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR2))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR3))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR4))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR5))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR6))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR7))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR8))->SetState(FALSE);
}

void CMainDlg::EnableSideColorSelect(BOOL enable)
{
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR1))->EnableWindow(enable);
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR2))->EnableWindow(enable);
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR3))->EnableWindow(enable);
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR4))->EnableWindow(enable);
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR5))->EnableWindow(enable);
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR6))->EnableWindow(enable);
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR7))->EnableWindow(enable);
	((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR8))->EnableWindow(enable);
}

void CMainDlg::UpdateFrontColorSelect()
{
	COLORREF selectedColor;

	if (m_pLightControl->IsSelectBandImagingEnabled())
	{
		selectedColor = m_pLightControl->GetFrontSelectBandColorValue();
	}
	else
	{
		int colorIndex = m_pLightControl->GetCurrentFrontColor();
		switch (colorIndex)
		{
		case 0: ((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR1))->SetState(TRUE); break;
		case 1: ((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR2))->SetState(TRUE); break;
		case 2: ((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR3))->SetState(TRUE); break;
		case 3: ((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR4))->SetState(TRUE); break;
		case 4: ((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR5))->SetState(TRUE); break;
		case 5: ((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR6))->SetState(TRUE); break;
		case 6: ((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR7))->SetState(TRUE); break;
		case 7: ((CButton*)GetDlgItem(IDC_LED_FRONT_COLOR8))->SetState(TRUE); break;
		}

		selectedColor = m_pLightControl->GetColorValue(colorIndex);
	}
}

void CMainDlg::UpdateSideColorSelect()
{
	COLORREF selectedColor;

	if (m_pLightControl->IsSelectBandImagingEnabled())
	{
		selectedColor = m_pLightControl->GetSideSelectBandColorValue();
	}
	else
	{
		int colorIndex = m_pLightControl->GetCurrentSideColor();
		switch (colorIndex)
		{
		case 0: ((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR1))->SetState(TRUE); break;
		case 1: ((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR2))->SetState(TRUE); break;
		case 2: ((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR3))->SetState(TRUE); break;
		case 3: ((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR4))->SetState(TRUE); break;
		case 4: ((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR5))->SetState(TRUE); break;
		case 5: ((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR6))->SetState(TRUE); break;
		case 6: ((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR7))->SetState(TRUE); break;
		case 7: ((CButton*)GetDlgItem(IDC_LED_SIDE_COLOR8))->SetState(TRUE); break;
		}

		selectedColor = m_pLightControl->GetColorValue(colorIndex);
	}
}

void CMainDlg::OnBnClickedLedFrontColor1()
{
	m_pLightControl->SetCurrentFrontColor(0);

	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}


void CMainDlg::OnBnClickedLedFrontColor2()
{
	m_pLightControl->SetCurrentFrontColor(1);

	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}

void CMainDlg::OnBnClickedLedFrontColor3()
{
	m_pLightControl->SetCurrentFrontColor(2);

	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}

void CMainDlg::OnBnClickedLedFrontColor4()
{
	m_pLightControl->SetCurrentFrontColor(3);

	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}

void CMainDlg::OnBnClickedLedFrontColor5()
{
	m_pLightControl->SetCurrentFrontColor(4);

	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}

void CMainDlg::OnBnClickedLedFrontColor6()
{
	m_pLightControl->SetCurrentFrontColor(5);

	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}

void CMainDlg::OnBnClickedLedFrontColor7()
{
	m_pLightControl->SetCurrentFrontColor(6);

	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}

void CMainDlg::OnBnClickedLedFrontColor8()
{
	m_pLightControl->SetCurrentFrontColor(7);

	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}

void CMainDlg::OnBnClickedLedSideColor1()
{
	m_pLightControl->SetCurrentSideColor(0);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}

void CMainDlg::OnBnClickedLedSideColor2()
{
	m_pLightControl->SetCurrentSideColor(1);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}

void CMainDlg::OnBnClickedLedSideColor3()
{
	m_pLightControl->SetCurrentSideColor(2);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}

void CMainDlg::OnBnClickedLedSideColor4()
{
	m_pLightControl->SetCurrentSideColor(3);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}

void CMainDlg::OnBnClickedLedSideColor5()
{
	m_pLightControl->SetCurrentSideColor(4);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}

void CMainDlg::OnBnClickedLedSideColor6()
{
	m_pLightControl->SetCurrentSideColor(5);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}

void CMainDlg::OnBnClickedLedSideColor7()
{
	m_pLightControl->SetCurrentSideColor(6);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}

void CMainDlg::OnBnClickedLedSideColor8()
{
	m_pLightControl->SetCurrentSideColor(7);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}

void CMainDlg::OnBnClickedButtonPlayback()
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


void CMainDlg::OnBnClickedButtonDoCapture()
{
	theApp.GetSystemManager()->CaptureImageFile();
}

void CMainDlg::OnBnClickedButtonLoadCapture()
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


void CMainDlg::OnBnClickedButtonRecord()
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

void CMainDlg::OnBnClickedButtonLiveStream()
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

LRESULT CMainDlg::OnUpdateModeMessage(WPARAM control, LPARAM value)
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



void CMainDlg::OnBnClickedButtonDefaultSettings()
{
	theApp.GetSystemManager()->LoadDefaultSettings();
}

void CMainDlg::UpdateLightColor()
{
	ClearFrontColorSelect();
	ClearSideColorSelect();
	UpdateFrontColorSelect();
	UpdateSideColorSelect();

	Invalidate();
}
