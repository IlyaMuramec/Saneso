// LightingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NGEuserApp.h"
#include "LightingDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LIGHT_UP_DOWN_LEVELS 10

static UINT UWM_LIGHT_CONTROLLER_MSG = RegisterWindowMessage(_T("Light Controller Message"));
static void MicroControllerMessageCallback(void* pObject, CMessage *pMsg)
{
	if ((pMsg->type == MC_CONTROL_CHANGED) && (pMsg->size == 2))
	{
		CLightingDlg *pDlg = (CLightingDlg *)pObject;
		pDlg->PostMessage(UWM_LIGHT_CONTROLLER_MSG, pMsg->msgData[0], pMsg->msgData[1]);
	}
}

static UINT UWM_UPDATE_LIGHT_LEVEL = RegisterWindowMessage(_T("Update Light Level"));
static void UpdateLightLevel(void* pObject, CameraType camera, LightSide side, float val)
{
	CLightingDlg *pDlg = (CLightingDlg *)pObject;
	WPARAM wparam = ((int)camera << 16 | (int)side);
	pDlg->PostMessage(UWM_UPDATE_LIGHT_LEVEL, wparam, val);
}

static UINT UWM_SELECT_BAND_IMAGING = RegisterWindowMessage(_T("Select Band Imaging"));
static void SelectBandImaging(void* pObject, bool enable)
{
	CLightingDlg *pDlg = (CLightingDlg *)pObject;
	WPARAM wparam = enable;
	pDlg->PostMessage(UWM_SELECT_BAND_IMAGING, wparam, 0);
}
/////////////////////////////////////////////////////////////////////////////
// CLightingDlg dialog


CLightingDlg::CLightingDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLightingDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLightingDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT	
	m_bNextUpdateFromMC = false;
	m_bNextUpdateFromSlider = false;
}


void CLightingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLightingDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_LED_FRONT_DOWN, m_btnLEDFrontDown);
	DDX_Control(pDX, IDC_LED_FRONT_UP, m_btnLEDFrontUp);
	DDX_Control(pDX, IDC_LED_SIDE_DOWN, m_btnLEDSideDown);
	DDX_Control(pDX, IDC_LED_SIDE_UP, m_btnLEDSideUp);
	DDX_Control(pDX, IDC_LED_FRONT_LEVEL, m_ctlLEDFrontLevel);
	DDX_Control(pDX, IDC_LED_SIDE_LEVEL, m_ctlLEDSideLevel);
	DDX_Control(pDX, IDC_SLIDER_SHADOW_CAST_DELAY, m_sldrShadowCastDelay);
	DDX_Control(pDX, IDC_SLIDER_FRONT, m_sldrFrontValue);
	DDX_Control(pDX, IDC_SLIDER_LEFT, m_sldrLeftValue);
	DDX_Control(pDX, IDC_SLIDER_RIGHT, m_sldrRightValue);
	DDX_Control(pDX, IDC_SLIDER_TOP, m_sldrTopValue);
	DDX_Control(pDX, IDC_SLIDER_BOTTOM, m_sldrBottomValue);
	DDX_Control(pDX, IDC_SLIDER_AUTO_SETPOINT, m_sldrAutoSetpoint);
	DDX_Control(pDX, IDC_SLIDER_AUTO_GAINP, m_sldrAutoGainP);
	DDX_Control(pDX, IDC_SLIDER_AUTO_GAINI, m_sldrAutoGainI);
	DDX_Control(pDX, IDC_SLIDER_AUTO_GAIND, m_sldrAutoGainD);
}


BEGIN_MESSAGE_MAP(CLightingDlg, CDialog)
	//{{AFX_MSG_MAP(CLightingDlg)
		// NOTE: the ClassWizard will add message map macros here
	ON_WM_DRAWITEM()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR1, &CLightingDlg::OnBnClickedLedFrontColor1)
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR2, &CLightingDlg::OnBnClickedLedFrontColor2)
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR3, &CLightingDlg::OnBnClickedLedFrontColor3)
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR4, &CLightingDlg::OnBnClickedLedFrontColor4)
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR5, &CLightingDlg::OnBnClickedLedFrontColor5)
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR6, &CLightingDlg::OnBnClickedLedFrontColor6)
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR7, &CLightingDlg::OnBnClickedLedFrontColor7)
	ON_BN_CLICKED(IDC_LED_FRONT_COLOR8, &CLightingDlg::OnBnClickedLedFrontColor8)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR1, &CLightingDlg::OnBnClickedLedSideColor1)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR2, &CLightingDlg::OnBnClickedLedSideColor2)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR3, &CLightingDlg::OnBnClickedLedSideColor3)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR4, &CLightingDlg::OnBnClickedLedSideColor4)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR5, &CLightingDlg::OnBnClickedLedSideColor5)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR6, &CLightingDlg::OnBnClickedLedSideColor6)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR7, &CLightingDlg::OnBnClickedLedSideColor7)
	ON_BN_CLICKED(IDC_LED_SIDE_COLOR8, &CLightingDlg::OnBnClickedLedSideColor8)
	ON_BN_CLICKED(IDC_LED_FRONT_ON_OFF, &CLightingDlg::OnBnClickedLedFrontOnOff)
	ON_BN_CLICKED(IDC_LED_FRONT_AUTO, &CLightingDlg::OnBnClickedLedFrontAuto)
	ON_BN_CLICKED(IDC_LED_SIDE_ON_OFF, &CLightingDlg::OnBnClickedLedSideOnOff)
	ON_BN_CLICKED(IDC_LED_SIDE_AUTO, &CLightingDlg::OnBnClickedLedSideAuto)
	ON_BN_CLICKED(IDC_LED_FRONT_DOWN, &CLightingDlg::OnBnClickedLedFrontDown)
	ON_BN_CLICKED(IDC_LED_FRONT_UP, &CLightingDlg::OnBnClickedLedFrontUp)
	ON_BN_CLICKED(IDC_LED_SIDE_DOWN, &CLightingDlg::OnBnClickedLedSideDown)
	ON_BN_CLICKED(IDC_LED_SIDE_UP, &CLightingDlg::OnBnClickedLedSideUp)
	ON_BN_CLICKED(IDC_SHADOW_CAST_ON_OFF, &CLightingDlg::OnBnClickedShadowCastOnOff)
	ON_BN_CLICKED(IDC_CHECK_SHADOW_FRONT_1, &CLightingDlg::OnBnClickedCheckShadowFront1)
	ON_BN_CLICKED(IDC_CHECK_SHADOW_LEFT_1, &CLightingDlg::OnBnClickedCheckShadowLeft1)
	ON_BN_CLICKED(IDC_CHECK_SHADOW_RIGHT_1, &CLightingDlg::OnBnClickedCheckShadowRight1)
	ON_BN_CLICKED(IDC_CHECK_SHADOW_TOP_1, &CLightingDlg::OnBnClickedCheckShadowTop1)
	ON_BN_CLICKED(IDC_CHECK_SHADOW_BOTTOM_1, &CLightingDlg::OnBnClickedCheckShadowBottom1)
	ON_BN_CLICKED(IDC_CHECK_SHADOW_FRONT_2, &CLightingDlg::OnBnClickedCheckShadowFront2)
	ON_BN_CLICKED(IDC_CHECK_SHADOW_LEFT_2, &CLightingDlg::OnBnClickedCheckShadowLeft2)
	ON_BN_CLICKED(IDC_CHECK_SHADOW_RIGHT_2, &CLightingDlg::OnBnClickedCheckShadowRight2)
	ON_BN_CLICKED(IDC_CHECK_SHADOW_TOP_2, &CLightingDlg::OnBnClickedCheckShadowTop2)
	ON_BN_CLICKED(IDC_CHECK_SHADOW_BOTTOM_2, &CLightingDlg::OnBnClickedCheckShadowBottom2)
	ON_WM_VSCROLL()
	ON_REGISTERED_MESSAGE(UWM_LIGHT_CONTROLLER_MSG, &CLightingDlg::OnMicroControllerMessage)
	ON_REGISTERED_MESSAGE(UWM_UPDATE_LIGHT_LEVEL, &CLightingDlg::OnUpdateLightValue)
	ON_REGISTERED_MESSAGE(UWM_SELECT_BAND_IMAGING, &CLightingDlg::OnSelectBandImaging)
	
	ON_BN_CLICKED(IDC_AUTO_SAT_OR_AVG, &CLightingDlg::OnBnClickedAutoSatOrAvg)
	ON_BN_CLICKED(IDC_AUTO_MODE, &CLightingDlg::OnBnClickedAutoMode)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLightingDlg message handlers

BOOL CLightingDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_pLightControl = theApp.GetSystemManager()->GetLightControl();
	m_pLightControl->SetUpdateLightCallBack(UpdateLightLevel, this);
	m_pLightControl->AddSelectBandImagingCallBack(SelectBandImaging, this);

	m_btnLEDFrontDown.LoadBitmaps(IDB_BITMAP_DOWN, IDB_BITMAP_DOWN_PRESSED);
	m_btnLEDFrontUp.LoadBitmaps(IDB_BITMAP_UP, IDB_BITMAP_UP_PRESSED);
	m_btnLEDSideDown.LoadBitmaps(IDB_BITMAP_DOWN, IDB_BITMAP_DOWN_PRESSED);
	m_btnLEDSideUp.LoadBitmaps(IDB_BITMAP_UP, IDB_BITMAP_UP_PRESSED);

	m_ctlLEDFrontLevel.SetBarColor(RGB(255, 255, 255));
	m_ctlLEDFrontLevel.SetBkColor(RGB(0, 0, 0));
	m_ctlLEDFrontLevel.SetRange(0, LIGHT_UP_DOWN_LEVELS);
	m_ctlLEDFrontLevel.SetStep(1);

	m_ctlLEDSideLevel.SetBarColor(RGB(255, 255, 255));
	m_ctlLEDSideLevel.SetBkColor(RGB(0, 0, 0));
	m_ctlLEDSideLevel.SetRange(0, LIGHT_UP_DOWN_LEVELS);
	m_ctlLEDSideLevel.SetStep(1);

	m_sldrShadowCastDelay.SetRange(SHADOW_CAST_DELAY_MS_MIN, SHADOW_CAST_DELAY_MS_MAX);
	m_sldrShadowCastDelay.SetTicFreq(abs(SHADOW_CAST_DELAY_MS_MAX - SHADOW_CAST_DELAY_MS_MIN) / 20);

	m_sldrFrontValue.SetRange(0, 100);
	m_sldrFrontValue.SetTicFreq(1);

	m_sldrLeftValue.SetRange(0, 100);
	m_sldrLeftValue.SetTicFreq(1);

	m_sldrRightValue.SetRange(0, 100);
	m_sldrRightValue.SetTicFreq(1);

	m_sldrTopValue.SetRange(0, 100);
	m_sldrTopValue.SetTicFreq(1);

	m_sldrBottomValue.SetRange(0, 100);
	m_sldrBottomValue.SetTicFreq(1);


	theApp.GetSystemManager()->GetMicroController()->AddMessageCallback(MicroControllerMessageCallback, this);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CLightingDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	COLORREF color = RGB(0, 0, 0);
	COLORREF selectedFrontColor = m_pLightControl->GetColorValue(m_pLightControl->GetCurrentFrontColor());
	COLORREF selectedSideColor = m_pLightControl->GetColorValue(m_pLightControl->GetCurrentSideColor());
	if (m_pLightControl->IsSelectBandImagingEnabled())
	{
		selectedFrontColor = m_pLightControl->GetFrontSelectBandColorValue();
		selectedSideColor = m_pLightControl->GetSideSelectBandColorValue();
	}
	BOOL selected = FALSE;

	switch (nIDCtl)
	{
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
	case IDC_LED_FRONT_AUTO:
		selected = m_pLightControl->GetFrontLightsAuto();
		selected ? color = selectedFrontColor : color = RGB(240, 240, 240);
		break;
	case IDC_LED_FRONT_ON_OFF:
		selected = m_pLightControl->GetFrontLightsOn();
		selected ? color = selectedFrontColor : color = RGB(240, 240, 240);
		break;
	case IDC_LED_SIDE_AUTO:
		selected = m_pLightControl->GetSideLightsAuto();
		selected ? color = selectedSideColor : color = RGB(240, 240, 240);
		break;
	case IDC_LED_SIDE_ON_OFF:
		selected = m_pLightControl->GetSideLightsOn();
		selected ? color = selectedSideColor : color = RGB(240, 240, 240);
		break;
	case IDC_SHADOW_CAST_ON_OFF:
		selected = m_pLightControl->GetShadowCastOn();
		selected ? color = selectedFrontColor : color = RGB(240, 240, 240);
		break;
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

void CLightingDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (bShow)
	{
		ShowColorButtonSelect();

		UpdateFrontLevel();
		UpdateSideLevel();

		ClearFrontColorSelect();
		UpdateFrontColorSelect();

		ClearSideColorSelect();
		UpdateSideColorSelect();

		((CButton*)GetDlgItem(IDC_CHECK_SHADOW_FRONT_1))->SetCheck(m_pLightControl->GetShadowCastGroup1(eFront));
		((CButton*)GetDlgItem(IDC_CHECK_SHADOW_LEFT_1))->SetCheck(m_pLightControl->GetShadowCastGroup1(eLeft));
		((CButton*)GetDlgItem(IDC_CHECK_SHADOW_RIGHT_1))->SetCheck(m_pLightControl->GetShadowCastGroup1(eRight));
		((CButton*)GetDlgItem(IDC_CHECK_SHADOW_TOP_1))->SetCheck(m_pLightControl->GetShadowCastGroup1(eTop));
		((CButton*)GetDlgItem(IDC_CHECK_SHADOW_BOTTOM_1))->SetCheck(m_pLightControl->GetShadowCastGroup1(eBottom));
		((CButton*)GetDlgItem(IDC_CHECK_SHADOW_FRONT_2))->SetCheck(m_pLightControl->GetShadowCastGroup2(eFront));
		((CButton*)GetDlgItem(IDC_CHECK_SHADOW_LEFT_2))->SetCheck(m_pLightControl->GetShadowCastGroup2(eLeft));
		((CButton*)GetDlgItem(IDC_CHECK_SHADOW_RIGHT_2))->SetCheck(m_pLightControl->GetShadowCastGroup2(eRight));
		((CButton*)GetDlgItem(IDC_CHECK_SHADOW_TOP_2))->SetCheck(m_pLightControl->GetShadowCastGroup2(eTop));
		((CButton*)GetDlgItem(IDC_CHECK_SHADOW_BOTTOM_2))->SetCheck(m_pLightControl->GetShadowCastGroup2(eBottom));

		m_sldrShadowCastDelay.SetPos(m_pLightControl->GetShadowCastDelayMs());

		m_sldrFrontValue.SetPos(100 - (int)(100 * m_pLightControl->GetCameraValue(eFront, LEFT)));
		m_sldrLeftValue.SetPos(100 - (int)(100 * m_pLightControl->GetCameraValue(eLeft, LEFT)));
		m_sldrRightValue.SetPos(100 - (int)(100 * m_pLightControl->GetCameraValue(eRight, LEFT)));
		m_sldrTopValue.SetPos(100 - (int)(100 * m_pLightControl->GetCameraValue(eTop, LEFT)));
		m_sldrBottomValue.SetPos(100 - (int)(100 * m_pLightControl->GetCameraValue(eBottom, LEFT)));

		UpdateAutoControls();

		UpdateData(FALSE);
	}
	else
	{
	}
}

LRESULT CLightingDlg::OnMicroControllerMessage(WPARAM control, LPARAM value)
{
	switch (control)
	{
	case CTRL_FRONT_LIGHT_UP:
		m_bNextUpdateFromMC = true;
		m_pLightControl->SetFrontLightsLevel((float)(value + 1) / LIGHT_UP_DOWN_LEVELS, true);
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_FRONT_UP, BN_CLICKED), NULL);
		break;
	case CTRL_FRONT_LIGHT_DOWN:
		m_bNextUpdateFromMC = true;
		m_pLightControl->SetFrontLightsLevel((float)(value + 1) / LIGHT_UP_DOWN_LEVELS, true);
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_FRONT_DOWN, BN_CLICKED), NULL);
		break;
	case CTRL_FRONT_LIGHT_ON_OFF:
		m_bNextUpdateFromMC = true;
		m_pLightControl->SetFrontLightsOn(value == 1, true);
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_FRONT_ON_OFF, BN_CLICKED), NULL);
		break;
	case CTRL_FRONT_LIGHT_AUTO:
		m_bNextUpdateFromMC = true;
		m_pLightControl->SetFrontLightsAuto(value == 1, false);
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_FRONT_AUTO, BN_CLICKED), NULL);
		break;
	case CTRL_SIDE_LIGHT_UP:
		m_bNextUpdateFromMC = true;
		m_pLightControl->SetSideLightsLevel((float)(value + 1) / LIGHT_UP_DOWN_LEVELS, true);
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_SIDE_UP, BN_CLICKED), NULL);
		break;
	case CTRL_SIDE_LIGHT_DOWN:
		m_bNextUpdateFromMC = true;
		m_pLightControl->SetSideLightsLevel((float)(value + 1) / LIGHT_UP_DOWN_LEVELS, true);
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_SIDE_DOWN, BN_CLICKED), NULL);
		break;
	case CTRL_SIDE_LIGHT_ON_OFF:
		m_bNextUpdateFromMC = true;
		m_pLightControl->SetSideLightsOn((float)value == 1, true);
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_SIDE_ON_OFF, BN_CLICKED), NULL);
		break;
	case CTRL_SIDE_LIGHT_AUTO:
		m_bNextUpdateFromMC = true;
		m_pLightControl->SetSideLightsAuto((float)value == 1, false);
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_SIDE_AUTO, BN_CLICKED), NULL);
		break;
	}
	return 0;
}

void CLightingDlg::UpdateFrontLevel()
{
	float level = m_pLightControl->GetFrontLightsLevel();
	m_ctlLEDFrontLevel.SetPos(round(LIGHT_UP_DOWN_LEVELS * level));
	m_ctlLEDFrontLevel.ShowWindow(m_pLightControl->GetFrontLightsOn());
}

void CLightingDlg::UpdateSideLevel()
{
	float level = m_pLightControl->GetSideLightsLevel();
	m_ctlLEDSideLevel.SetPos(round(LIGHT_UP_DOWN_LEVELS * level));
	m_ctlLEDSideLevel.ShowWindow(m_pLightControl->GetSideLightsOn());
}

LRESULT CLightingDlg::OnUpdateLightValue(WPARAM control, LPARAM value)
{
	if (m_bNextUpdateFromSlider)
	{
		m_bNextUpdateFromSlider = false;
		return 0;
	}

	CameraType camera = (CameraType)((control & 0xFFFF0000) >> 16);
	LightSide side = (LightSide)(control & 0xFFFF);
	int val = value;

	switch (camera)
	{
	case eFront:
		if (side == LEFT)
			m_sldrFrontValue.SetPos(100 - val);

		break;
	case eLeft:
		if (side == LEFT)
			m_sldrLeftValue.SetPos(100 - val);

		break;
	case eRight:
		if (side == LEFT)
			m_sldrRightValue.SetPos(100 - val);

		break;
	case eTop:
		if (side == LEFT)
			m_sldrTopValue.SetPos(100 - val);

		break;
	case eBottom:
		if (side == LEFT)
			m_sldrBottomValue.SetPos(100 - val);

		break;
	}

	return 0;
}

LRESULT CLightingDlg::OnSelectBandImaging(WPARAM control, LPARAM value)
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

void CLightingDlg::CycleColor(bool bNext)
{
	int index = m_pLightControl->GetCurrentFrontColor();
	index += (bNext ? 1 : -1);

	int numColors = m_pLightControl->GetNumColors();
	if (index < 0)
	{
		index = numColors - 1;
	}
	if (index >= numColors)
	{
		index = 0;
	}

	switch (index)
	{
	case 0: 
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_FRONT_COLOR1, BN_CLICKED), NULL); 
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_SIDE_COLOR1, BN_CLICKED), NULL);
		break;
	case 1:
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_FRONT_COLOR2, BN_CLICKED), NULL);
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_SIDE_COLOR2, BN_CLICKED), NULL);
		break;
	case 2:
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_FRONT_COLOR3, BN_CLICKED), NULL);
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_SIDE_COLOR3, BN_CLICKED), NULL);
		break;
	case 3:
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_FRONT_COLOR4, BN_CLICKED), NULL);
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_SIDE_COLOR4, BN_CLICKED), NULL);
		break;
	case 4:
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_FRONT_COLOR5, BN_CLICKED), NULL);
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_SIDE_COLOR5, BN_CLICKED), NULL);
		break;
	case 5:
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_FRONT_COLOR6, BN_CLICKED), NULL);
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_SIDE_COLOR6, BN_CLICKED), NULL);
		break;
	case 6:
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_FRONT_COLOR7, BN_CLICKED), NULL);
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_SIDE_COLOR7, BN_CLICKED), NULL);
		break;
	case 7:
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_FRONT_COLOR8, BN_CLICKED), NULL);
		SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_SIDE_COLOR8, BN_CLICKED), NULL);
		break;
	}

}
void CLightingDlg::ToggleFrontLights()
{
	SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_FRONT_ON_OFF, BN_CLICKED), NULL);
}
void CLightingDlg::ToggleSideLights()
{
	SendMessage(WM_COMMAND, MAKEWPARAM(IDC_LED_SIDE_ON_OFF, BN_CLICKED), NULL);
}

void CLightingDlg::ShowColorButtonSelect()
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

void CLightingDlg::ClearFrontColorSelect()
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

void CLightingDlg::EnableFrontColorSelect(BOOL enable)
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

void CLightingDlg::ClearSideColorSelect()
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

void CLightingDlg::EnableSideColorSelect(BOOL enable)
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

void CLightingDlg::UpdateFrontColorSelect()
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

	m_ctlLEDFrontLevel.SetBarColor(selectedColor);
	m_ctlLEDFrontLevel.SetBkColor(RGB(0, 0, 0));

	UpdateFrontLevel();
}

void CLightingDlg::UpdateSideColorSelect()
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

	m_ctlLEDSideLevel.SetBarColor(selectedColor);
	m_ctlLEDSideLevel.SetBkColor(RGB(0, 0, 0));
	
	UpdateSideLevel();
}

void CLightingDlg::UpdateAutoControls()
{
	bool bAutoSat = m_pLightControl->GetAutoSatOrAvg();
	if (bAutoSat)
	{
		GetDlgItem(IDC_AUTO_SAT_OR_AVG)->SetWindowTextW(_T("Saturation"));

		m_sldrAutoSetpoint.SetTopBottomValues(0.25, 0.001);
		m_sldrAutoSetpoint.SetFloatValue(m_pLightControl->GetAutoSatSetpoint());
		
		m_sldrAutoGainP.SetTopBottomValues(5, 0.1);
		m_sldrAutoGainP.SetFloatValue(m_pLightControl->GetAutoSatGainP());
		m_sldrAutoGainI.SetTopBottomValues(10, 0);
		m_sldrAutoGainI.SetFloatValue(m_pLightControl->GetAutoSatGainI());
		m_sldrAutoGainD.SetTopBottomValues(0.5, 0);
		m_sldrAutoGainD.SetFloatValue(m_pLightControl->GetAutoSatGainD());
	}
	else
	{
		GetDlgItem(IDC_AUTO_SAT_OR_AVG)->SetWindowTextW(_T("Average"));

		m_sldrAutoSetpoint.SetTopBottomValues(255, 0);
		m_sldrAutoSetpoint.SetFloatValue(m_pLightControl->GetAutoAvgSetpoint());
		
		m_sldrAutoGainP.SetTopBottomValues(0.01, 0.001); 
		m_sldrAutoGainP.SetFloatValue(m_pLightControl->GetAutoAvgGainP());
		m_sldrAutoGainI.SetTopBottomValues(0.01, 0);
		m_sldrAutoGainI.SetFloatValue(m_pLightControl->GetAutoAvgGainI());
		m_sldrAutoGainD.SetTopBottomValues(0.01, 0);
		m_sldrAutoGainD.SetFloatValue(m_pLightControl->GetAutoAvgGainD());
	}

	AutoMode autoMode = m_pLightControl->GetAutoMode();
	switch (autoMode)
	{
		case LIGHTS:
			GetDlgItem(IDC_AUTO_MODE)->SetWindowTextW(_T("Lights"));
			break;
		case CAMERA_GAIN:
			GetDlgItem(IDC_AUTO_MODE)->SetWindowTextW(_T("Camera"));
			break;
		case BOTH:
			GetDlgItem(IDC_AUTO_MODE)->SetWindowTextW(_T("Both"));
			break;
	}
}

void CLightingDlg::OnBnClickedLedFrontColor1()
{
	m_pLightControl->SetCurrentFrontColor(0);

	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedFrontColor2()
{
	m_pLightControl->SetCurrentFrontColor(1);

	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedFrontColor3()
{
	m_pLightControl->SetCurrentFrontColor(2);

	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedFrontColor4()
{
	m_pLightControl->SetCurrentFrontColor(3);

	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedFrontColor5()
{
	m_pLightControl->SetCurrentFrontColor(4);

	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedFrontColor6()
{
	m_pLightControl->SetCurrentFrontColor(5);

	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedFrontColor7()
{
	m_pLightControl->SetCurrentFrontColor(6);
	
	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedFrontColor8()
{
	m_pLightControl->SetCurrentFrontColor(7);

	ClearFrontColorSelect();
	UpdateFrontColorSelect();
	Invalidate();
}

void CLightingDlg::OnBnClickedLedSideColor1()
{
	m_pLightControl->SetCurrentSideColor(0);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedSideColor2()
{
	m_pLightControl->SetCurrentSideColor(1);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedSideColor3()
{
	m_pLightControl->SetCurrentSideColor(2);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedSideColor4()
{
	m_pLightControl->SetCurrentSideColor(3);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedSideColor5()
{
	m_pLightControl->SetCurrentSideColor(4);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedSideColor6()
{
	m_pLightControl->SetCurrentSideColor(5);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedSideColor7()
{
	m_pLightControl->SetCurrentSideColor(6);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedSideColor8()
{
	m_pLightControl->SetCurrentSideColor(7);

	ClearSideColorSelect();
	UpdateSideColorSelect();
	Invalidate();
}

void CLightingDlg::OnBnClickedLedFrontOnOff()
{
	if (!m_bNextUpdateFromMC)
	{
		m_pLightControl->SetFrontLightsOn(!m_pLightControl->GetFrontLightsOn(), true);
	}
	m_bNextUpdateFromMC = false;

	UpdateFrontLevel();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedFrontAuto()
{
	bool bAutoNewState = !m_pLightControl->GetFrontLightsAuto();
	if (bAutoNewState && !m_pLightControl->GetFrontLightsOn())
	{
		m_pLightControl->SetFrontLightsOn(true, true);
	}

	if (!m_bNextUpdateFromMC)
	{
		m_pLightControl->SetFrontLightsAuto(bAutoNewState, true);
	}
	m_bNextUpdateFromMC = false;

	UpdateFrontColorSelect();
	Invalidate();
}

void CLightingDlg::OnBnClickedLedSideOnOff()
{
	if (!m_bNextUpdateFromMC)
	{
		m_pLightControl->SetSideLightsOn(!m_pLightControl->GetSideLightsOn(), true);
	}
	m_bNextUpdateFromMC = false;

	UpdateSideLevel();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedSideAuto()
{
	bool bAutoNewState = !m_pLightControl->GetSideLightsAuto();
	if (bAutoNewState && !m_pLightControl->GetSideLightsOn())
	{
		m_pLightControl->SetSideLightsOn(true, true);
	}

	if (!m_bNextUpdateFromMC)
	{
		m_pLightControl->SetSideLightsAuto(bAutoNewState, true);
	}
	m_bNextUpdateFromMC = false;
	
	UpdateSideColorSelect();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedFrontDown()
{
	if (!m_pLightControl->GetFrontLightsOn())
	{
		m_pLightControl->SetFrontLightsOn(true, true);
	}
	
	float fLevel = m_pLightControl->GetFrontLightsLevel();
	
	if (!m_bNextUpdateFromMC)
	{
		float inc = 1.0f / LIGHT_UP_DOWN_LEVELS;
		fLevel -= inc;
		m_pLightControl->SetFrontLightsLevel(fLevel, true);
	}
	m_bNextUpdateFromMC = false;

	UpdateFrontLevel();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedFrontUp()
{
	if (!m_pLightControl->GetFrontLightsOn())
	{
		m_pLightControl->SetFrontLightsOn(true, true);
	}

	float fLevel = m_pLightControl->GetFrontLightsLevel();
	if (!m_bNextUpdateFromMC)
	{
		float inc = 1.0f / LIGHT_UP_DOWN_LEVELS;
		fLevel += inc;
		m_pLightControl->SetFrontLightsLevel(fLevel, true);
	}
	m_bNextUpdateFromMC = false;

	UpdateFrontLevel();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedSideDown()
{
	if (!m_pLightControl->GetSideLightsOn())
	{
		m_pLightControl->SetSideLightsOn(true, true);
	}

	float fLevel = m_pLightControl->GetSideLightsLevel();
	if (!m_bNextUpdateFromMC)
	{
		float inc = 1.0f / LIGHT_UP_DOWN_LEVELS;
		fLevel -= inc;
		m_pLightControl->SetSideLightsLevel(fLevel, true);
	}
	m_bNextUpdateFromMC = false;

	UpdateSideLevel();
	Invalidate();
}


void CLightingDlg::OnBnClickedLedSideUp()
{
	if (!m_pLightControl->GetSideLightsOn())
	{
		m_pLightControl->SetSideLightsOn(true, true);
	}

	float fLevel = m_pLightControl->GetSideLightsLevel();
	if (!m_bNextUpdateFromMC)
	{
		float inc = 1.0f / LIGHT_UP_DOWN_LEVELS;
		fLevel += inc;
		m_pLightControl->SetSideLightsLevel(fLevel, true);
	}
	m_bNextUpdateFromMC = false;

	UpdateSideLevel();
	Invalidate();
}


void CLightingDlg::OnBnClickedShadowCastOnOff()
{
	m_pLightControl->SetShadowCastOn(!m_pLightControl->GetShadowCastOn());
	Invalidate();
}

void CLightingDlg::OnBnClickedAutoSatOrAvg()
{
	m_pLightControl->SetAutoSatOrAvg(!m_pLightControl->GetAutoSatOrAvg());
	
	UpdateAutoControls();
}


void CLightingDlg::OnBnClickedAutoMode()
{
	int mode = (int)m_pLightControl->GetAutoMode();
	mode++;
	if (mode == (int)LAST_AUTO_MODE)
		mode = 0;

	m_pLightControl->SetAutoMode((AutoMode)mode);

	UpdateAutoControls();
}

void CLightingDlg::OnBnClickedCheckShadowFront1()
{
	bool checked = (((CButton *)GetDlgItem(IDC_CHECK_SHADOW_FRONT_1))->GetCheck() == 1);
	m_pLightControl->UpdateShadowGroup1(eFront, checked);
}


void CLightingDlg::OnBnClickedCheckShadowLeft1()
{
	bool checked = (((CButton *)GetDlgItem(IDC_CHECK_SHADOW_LEFT_1))->GetCheck() == 1);
	m_pLightControl->UpdateShadowGroup1(eLeft, checked);
}


void CLightingDlg::OnBnClickedCheckShadowRight1()
{
	bool checked = (((CButton *)GetDlgItem(IDC_CHECK_SHADOW_RIGHT_1))->GetCheck() == 1);
	m_pLightControl->UpdateShadowGroup1(eRight, checked);
}


void CLightingDlg::OnBnClickedCheckShadowTop1()
{
	bool checked = (((CButton *)GetDlgItem(IDC_CHECK_SHADOW_TOP_1))->GetCheck() == 1);
	m_pLightControl->UpdateShadowGroup1(eTop, checked);
}


void CLightingDlg::OnBnClickedCheckShadowBottom1()
{
	bool checked = (((CButton *)GetDlgItem(IDC_CHECK_SHADOW_BOTTOM_1))->GetCheck() == 1);
	m_pLightControl->UpdateShadowGroup1(eBottom, checked);
}


void CLightingDlg::OnBnClickedCheckShadowFront2()
{
	bool checked = (((CButton *)GetDlgItem(IDC_CHECK_SHADOW_FRONT_2))->GetCheck() == 1);
	m_pLightControl->UpdateShadowGroup2(eFront, checked);
}


void CLightingDlg::OnBnClickedCheckShadowLeft2()
{
	bool checked = (((CButton *)GetDlgItem(IDC_CHECK_SHADOW_LEFT_2))->GetCheck() == 1);
	m_pLightControl->UpdateShadowGroup2(eLeft, checked);
}


void CLightingDlg::OnBnClickedCheckShadowRight2()
{
	bool checked = (((CButton *)GetDlgItem(IDC_CHECK_SHADOW_RIGHT_2))->GetCheck() == 1);
	m_pLightControl->UpdateShadowGroup2(eRight, checked);
}


void CLightingDlg::OnBnClickedCheckShadowTop2()
{
	bool checked = (((CButton *)GetDlgItem(IDC_CHECK_SHADOW_TOP_2))->GetCheck() == 1);
	m_pLightControl->UpdateShadowGroup2(eTop, checked);
}


void CLightingDlg::OnBnClickedCheckShadowBottom2()
{
	bool checked = (((CButton *)GetDlgItem(IDC_CHECK_SHADOW_BOTTOM_2))->GetCheck() == 1);
	m_pLightControl->UpdateShadowGroup2(eBottom, checked);
}

void CLightingDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar == (CScrollBar *)&m_sldrShadowCastDelay)
	{
		int value = m_sldrShadowCastDelay.GetPos();
		int settingMs = m_pLightControl->GetShadowCastDelayMs();
		if (settingMs != value)
		{
			m_pLightControl->SetShadowCastDelayMs(value);
		}
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrFrontValue)
	{
		m_bNextUpdateFromSlider = true;
		float value = (float)(100.0 - m_sldrFrontValue.GetPos())/100.0;
		m_pLightControl->SetCameraValue(eFront, LEFT, value, true);
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrLeftValue)
	{
		m_bNextUpdateFromSlider = true;
		float value = (float)(100.0 - m_sldrLeftValue.GetPos()) / 100.0;
		m_pLightControl->SetCameraValue(eLeft, LEFT, value, true);
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrRightValue)
	{
		m_bNextUpdateFromSlider = true;
		float value = (float)(100.0 - m_sldrRightValue.GetPos()) / 100.0;
		m_pLightControl->SetCameraValue(eRight, LEFT, value, true);
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrTopValue)
	{
		m_bNextUpdateFromSlider = true;
		float value = (float)(100.0 - m_sldrTopValue.GetPos()) / 100.0;
		m_pLightControl->SetCameraValue(eTop, LEFT, value, true);
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrBottomValue)
	{
		m_bNextUpdateFromSlider = true;
		float value = (float)(100.0 - m_sldrBottomValue.GetPos()) / 100.0;
		m_pLightControl->SetCameraValue(eBottom, LEFT, value, true);
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrAutoSetpoint)
	{
		float setpoint = m_sldrAutoSetpoint.GetFloatValue();
		if (m_pLightControl->GetAutoSatOrAvg())
		{
			m_pLightControl->SetAutoSatSetpoint(setpoint);
		}
		else
		{
			m_pLightControl->SetAutoAvgSetpoint(setpoint);
		}
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrAutoGainP)
	{
		float gain = m_sldrAutoGainP.GetFloatValue();
		if (m_pLightControl->GetAutoSatOrAvg())
		{
			m_pLightControl->SetAutoSatGainP(gain);
		}
		else
		{
			m_pLightControl->SetAutoAvgGainP(gain);
		}
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrAutoGainI)
	{
		float gain = m_sldrAutoGainI.GetFloatValue();
		if (m_pLightControl->GetAutoSatOrAvg())
		{
			m_pLightControl->SetAutoSatGainI(gain);
		}
		else
		{
			m_pLightControl->SetAutoAvgGainI(gain);
		}
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrAutoGainD)
	{
		float gain = m_sldrAutoGainD.GetFloatValue();
		if (m_pLightControl->GetAutoSatOrAvg())
		{
			m_pLightControl->SetAutoSatGainD(gain);
		}
		else
		{
			m_pLightControl->SetAutoAvgGainD(gain);
		}
	}
	else{
		CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
	}
}
