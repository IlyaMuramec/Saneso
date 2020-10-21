// EnhancementDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NGEuserApp.h"
#include "EnhancementDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CEnhancementDlg dialog


CEnhancementDlg::CEnhancementDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEnhancementDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEnhancementDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bNextUpdateFromMC = false;
	m_bNextUpdateFromSlider = false;

	m_sldrContrast.SetTopBottomValues(100.0, 0.0);
	m_sldrSharpness.SetTopBottomValues(20.0, 0.0);
	m_sldrGamma.SetTopBottomValues(0, 1.0);
	m_sldrGammaSide.SetTopBottomValues(0, 1.0);
	m_sldrBrightness.SetTopBottomValues(1, 0);
	m_sldrTemporalAveraging.SetTopBottomValues(100.0, 0.0);
	m_sldrSpatialAveraging.SetTopBottomValues(10.0, 0.0);
	m_sldrFrontScaleSlope.SetTopBottomValues(1.5, 0.0);
	m_sldrScaleSlope.SetTopBottomValues(0.75, 0.01);
	m_sldrScaleOffset.SetTopBottomValues(2.5, 0.0);
}


void CEnhancementDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEnhancementDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_SLIDER_ENHANCE_CONTRAST, m_sldrContrast);
	DDX_Control(pDX, IDC_SLIDER_ENHANCE_SHARPNESS, m_sldrSharpness);
	DDX_Control(pDX, IDC_SLIDER_ENHANCE_GAMMA, m_sldrGamma);
	DDX_Control(pDX, IDC_SLIDER_ENHANCE_GAMMA_SIDE, m_sldrGammaSide);
	DDX_Control(pDX, IDC_SLIDER_ENHANCE_BRIGHTNESS, m_sldrBrightness);
	DDX_Control(pDX, IDC_SLIDER_TEMPORAL_AVERAGING, m_sldrTemporalAveraging);
	DDX_Control(pDX, IDC_SLIDER_SPATIAL_AVERAGING, m_sldrSpatialAveraging);
	DDX_Control(pDX, IDC_SLIDER_FRONT_SCALE_SLOPE, m_sldrFrontScaleSlope);
	DDX_Control(pDX, IDC_SLIDER_SCALE_SLOPE, m_sldrScaleSlope);
	DDX_Control(pDX, IDC_SLIDER_SCALE_OFFSET, m_sldrScaleOffset);
}


BEGIN_MESSAGE_MAP(CEnhancementDlg, CDialog)
	//{{AFX_MSG_MAP(CEnhancementDlg)
	// NOTE: the ClassWizard will add message map macros here
	ON_WM_DRAWITEM()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_ENHANCE_PRESET1, &CEnhancementDlg::OnBnClickedEnchancePreset1)
	ON_BN_CLICKED(IDC_ENHANCE_PRESET2, &CEnhancementDlg::OnBnClickedEnchancePreset2)
	ON_BN_CLICKED(IDC_ENHANCE_PRESET3, &CEnhancementDlg::OnBnClickedEnchancePreset3)
	ON_BN_CLICKED(IDC_ENHANCE_PRESET4, &CEnhancementDlg::OnBnClickedEnchancePreset4)
	ON_BN_CLICKED(IDC_ENHANCE_PRESET5, &CEnhancementDlg::OnBnClickedEnchancePreset5)
	ON_BN_CLICKED(IDC_ENHANCE_PRESET6, &CEnhancementDlg::OnBnClickedEnchancePreset6)
	ON_BN_CLICKED(IDC_ENHANCE_PRESET7, &CEnhancementDlg::OnBnClickedEnchancePreset7)
	ON_BN_CLICKED(IDC_ENHANCE_PRESET8, &CEnhancementDlg::OnBnClickedEnchancePreset8)
	ON_WM_VSCROLL()
	ON_BN_CLICKED(IDC_ENHANCE_CONTRAST_ON_OFF, &CEnhancementDlg::OnBnClickedEnhanceContrastOnOff)
	ON_BN_CLICKED(IDC_ENHANCE_SHARPNESS_ON_OFF, &CEnhancementDlg::OnBnClickedEnhanceSharpnessOnOff)
	ON_BN_CLICKED(IDC_TEMPORAL_AVERAGING_ON_OFF, &CEnhancementDlg::OnBnClickedTemporalAveragingOnOff)
	ON_BN_CLICKED(IDC_BUTTON_TEMPORAL_AVERAGING_TYPE, &CEnhancementDlg::OnBnClickedButtonTemporalAveragingType)
	ON_BN_CLICKED(IDC_SPATIAL_AVERAGING_ON_OFF, &CEnhancementDlg::OnBnClickedSpatialAveragingOnOff)
	ON_BN_CLICKED(IDC_BUTTON_SPATIAL_AVERAGING_TYPE, &CEnhancementDlg::OnBnClickedButtonSpatialAveragingType)
	ON_BN_CLICKED(IDC_ENHANCE_GAMMA_ON_OFF, &CEnhancementDlg::OnBnClickedEnhanceGammaOnOff)
	ON_BN_CLICKED(IDC_ENHANCE_BRIGHTNESS_ON_OFF, &CEnhancementDlg::OnBnClickedEnhanceBrightnessOnOff)
	ON_BN_CLICKED(IDC_ENHANCE_GAMMA_SIDE_ON_OFF, &CEnhancementDlg::OnBnClickedEnhanceGammaSideOnOff)
	ON_BN_CLICKED(IDC_SCALING_ON_OFF, &CEnhancementDlg::OnBnClickedScalingOnOff)
	ON_BN_CLICKED(IDC_ENHANCE_ALL_OFF, &CEnhancementDlg::OnBnClickedEnhanceAllOff)
	ON_BN_CLICKED(IDC_FRONT_SCALING_ON_OFF, &CEnhancementDlg::OnBnClickedFrontScalingOnOff)
	ON_BN_CLICKED(IDC_RADIO_UPSAMPLE1, &CEnhancementDlg::OnBnClickedRadioUpsample1)
	ON_BN_CLICKED(IDC_RADIO_UPSAMPLE2, &CEnhancementDlg::OnBnClickedRadioUpsample2)
	ON_BN_CLICKED(IDC_RADIO_UPSAMPLE3, &CEnhancementDlg::OnBnClickedRadioUpsample3)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEnhancementDlg message handlers

BOOL CEnhancementDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_pEnhancementControl = theApp.GetSystemManager()->GetEnhancementControl();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CEnhancementDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	COLORREF color = RGB(0, 0, 0);
	COLORREF colorText = RGB(0, 0, 0);
	COLORREF selectedColor = RGB(255, 255, 0);
	BOOL selected = FALSE;

	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();

	switch (nIDCtl)
	{
	case IDC_ENHANCE_PRESET1:
	case IDC_ENHANCE_PRESET2:
	case IDC_ENHANCE_PRESET3:
	case IDC_ENHANCE_PRESET4:
	case IDC_ENHANCE_PRESET5:
	case IDC_ENHANCE_PRESET6:
	case IDC_ENHANCE_PRESET7:
	case IDC_ENHANCE_PRESET8:
		IsButtonSelected(nIDCtl)? color = selectedColor : color = RGB(240, 240, 240);
		break;
	case IDC_ENHANCE_ALL_OFF:
		selected = !m_pEnhancementControl->IsEnhancementEnabled();
		selected ? color = selectedColor : color = RGB(240, 240, 240);
		break;
	case IDC_ENHANCE_CONTRAST_ON_OFF:
		selected = pSettings->IsContrastEnhancementEnabled();
		selected ? color = selectedColor : color = RGB(240, 240, 240);
		break;
	case IDC_ENHANCE_GAMMA_ON_OFF:
		selected = pSettings->IsGammaEnabled();
		selected ? color = selectedColor : color = RGB(240, 240, 240);
		break;
	case IDC_ENHANCE_GAMMA_SIDE_ON_OFF:
		selected = pSettings->IsGammaSideEnabled();
		selected ? color = selectedColor : color = RGB(240, 240, 240);
		break;
	case IDC_ENHANCE_BRIGHTNESS_ON_OFF:
		selected = pSettings->IsBrightnessEnabled();
		selected ? color = selectedColor : color = RGB(240, 240, 240);
		break;
	case IDC_ENHANCE_SHARPNESS_ON_OFF:
		selected = pSettings->IsSharpnessEnhancementEnabled();
		selected ? color = selectedColor : color = RGB(240, 240, 240);
		break;
	case IDC_TEMPORAL_AVERAGING_ON_OFF:
		selected = pSettings->IsTemporalAveragingEnabled();
		selected ? color = selectedColor : color = RGB(240, 240, 240);
		break;
	case IDC_SPATIAL_AVERAGING_ON_OFF:
		selected = pSettings->IsSpatialAveragingEnabled();
		selected ? color = selectedColor : color = RGB(240, 240, 240);
		break;
	case IDC_FRONT_SCALING_ON_OFF:
		selected = pSettings->IsFrontScaleEnabled();
		selected ? color = selectedColor : color = RGB(240, 240, 240);
		break;
	case IDC_SCALING_ON_OFF:
		selected = pSettings->IsSideScaleEnabled();
		selected ? color = selectedColor : color = RGB(240, 240, 240);
		break;
	}

	if (!GetDlgItem(nIDCtl)->IsWindowEnabled())
	{
		color = RGB(240, 240, 240);
		colorText = RGB(160, 160, 160);
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
		dc.SetTextColor(colorText);     //Setting the Text Color

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

void CEnhancementDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (bShow)
	{
		ShowPresetButtons();
		ClearPresetSelect();
		UpdatePresetSelect();

		UpdateData(FALSE);
	}
	else
	{
	}
}

bool CEnhancementDlg::IsButtonSelected(int nIDCtl)
{
	int state = ((CButton*)GetDlgItem(nIDCtl))->GetState();
	return ((state & 0x4) == 0x4);
}

void CEnhancementDlg::HandsetAction(EnhancementHandsetAction action)
{
	int index = m_pEnhancementControl->GetCurrentPreset();

	switch (action)
	{
	case eOnOff:
		if (m_pEnhancementControl->IsEnhancementEnabled())
		{
			// Click all the off button
			index = 0; // no mode change
			SendMessage(WM_COMMAND, MAKEWPARAM(IDC_ENHANCE_ALL_OFF, BN_CLICKED), NULL); 
			break;
		}
		break;
	case eNext:
		do
		{
			index += 1;
			if (index > MAX_NUM_ENHANCEMENT_PRESETS)
			{
				index = 1;
			}
		} while (!m_pEnhancementControl->ContainsPreset(index));
		
		break;
	case ePrevious:
		do
		{
			index -= 1;
			if (index < 1)
			{
				index = MAX_NUM_ENHANCEMENT_PRESETS;
			}
		} while (!m_pEnhancementControl->ContainsPreset(index));
			
		break;
	}
	
	switch (index)
	{
	case 1: SendMessage(WM_COMMAND, MAKEWPARAM(IDC_ENHANCE_PRESET1, BN_CLICKED), NULL); break;
	case 2: SendMessage(WM_COMMAND, MAKEWPARAM(IDC_ENHANCE_PRESET2, BN_CLICKED), NULL); break;
	case 3: SendMessage(WM_COMMAND, MAKEWPARAM(IDC_ENHANCE_PRESET3, BN_CLICKED), NULL); break;
	case 4: SendMessage(WM_COMMAND, MAKEWPARAM(IDC_ENHANCE_PRESET4, BN_CLICKED), NULL); break;
	case 5: SendMessage(WM_COMMAND, MAKEWPARAM(IDC_ENHANCE_PRESET5, BN_CLICKED), NULL); break;
	case 6: SendMessage(WM_COMMAND, MAKEWPARAM(IDC_ENHANCE_PRESET6, BN_CLICKED), NULL); break;
	case 7: SendMessage(WM_COMMAND, MAKEWPARAM(IDC_ENHANCE_PRESET7, BN_CLICKED), NULL); break;
	case 8: SendMessage(WM_COMMAND, MAKEWPARAM(IDC_ENHANCE_PRESET8, BN_CLICKED), NULL); break;
	}
}

void CEnhancementDlg::ShowPresetButtons()
{
	GetDlgItem(IDC_ENHANCE_PRESET1)->ShowWindow(m_pEnhancementControl->ContainsPreset(1));
	GetDlgItem(IDC_ENHANCE_PRESET2)->ShowWindow(m_pEnhancementControl->ContainsPreset(2));
	GetDlgItem(IDC_ENHANCE_PRESET3)->ShowWindow(m_pEnhancementControl->ContainsPreset(3));
	GetDlgItem(IDC_ENHANCE_PRESET4)->ShowWindow(m_pEnhancementControl->ContainsPreset(4));
	GetDlgItem(IDC_ENHANCE_PRESET5)->ShowWindow(m_pEnhancementControl->ContainsPreset(5));
	GetDlgItem(IDC_ENHANCE_PRESET6)->ShowWindow(m_pEnhancementControl->ContainsPreset(6));
	GetDlgItem(IDC_ENHANCE_PRESET7)->ShowWindow(m_pEnhancementControl->ContainsPreset(7));
	GetDlgItem(IDC_ENHANCE_PRESET8)->ShowWindow(m_pEnhancementControl->ContainsPreset(8));
}

void CEnhancementDlg::ClearPresetSelect()
{
	((CButton*)GetDlgItem(IDC_ENHANCE_PRESET1))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_ENHANCE_PRESET2))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_ENHANCE_PRESET3))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_ENHANCE_PRESET4))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_ENHANCE_PRESET5))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_ENHANCE_PRESET6))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_ENHANCE_PRESET7))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_ENHANCE_PRESET8))->SetState(FALSE);
	((CButton*)GetDlgItem(IDC_ENHANCE_ALL_OFF))->SetState(FALSE);
}

void CEnhancementDlg::EnableOptions(BOOL enable)
{
	GetDlgItem(IDC_ENHANCE_CONTRAST_ON_OFF)->EnableWindow(enable);
	GetDlgItem(IDC_ENHANCE_GAMMA_ON_OFF)->EnableWindow(enable);
	GetDlgItem(IDC_ENHANCE_GAMMA_SIDE_ON_OFF)->EnableWindow(enable);
	GetDlgItem(IDC_ENHANCE_BRIGHTNESS_ON_OFF)->EnableWindow(enable);
	GetDlgItem(IDC_ENHANCE_SHARPNESS_ON_OFF)->EnableWindow(enable);
	GetDlgItem(IDC_TEMPORAL_AVERAGING_ON_OFF)->EnableWindow(enable);
	GetDlgItem(IDC_SPATIAL_AVERAGING_ON_OFF)->EnableWindow(enable);
	GetDlgItem(IDC_FRONT_SCALING_ON_OFF)->EnableWindow(enable);
	GetDlgItem(IDC_SCALING_ON_OFF)->EnableWindow(enable);
}


void CEnhancementDlg::UpdatePresetSelect()
{
	if (m_pEnhancementControl->IsEnhancementEnabled())
	{
		int colorIndex = m_pEnhancementControl->GetCurrentPreset();
		switch (colorIndex)
		{
		case 1: ((CButton*)GetDlgItem(IDC_ENHANCE_PRESET1))->SetState(TRUE); break;
		case 2: ((CButton*)GetDlgItem(IDC_ENHANCE_PRESET2))->SetState(TRUE); break;
		case 3: ((CButton*)GetDlgItem(IDC_ENHANCE_PRESET3))->SetState(TRUE); break;
		case 4: ((CButton*)GetDlgItem(IDC_ENHANCE_PRESET4))->SetState(TRUE); break;
		case 5: ((CButton*)GetDlgItem(IDC_ENHANCE_PRESET5))->SetState(TRUE); break;
		case 6: ((CButton*)GetDlgItem(IDC_ENHANCE_PRESET6))->SetState(TRUE); break;
		case 7: ((CButton*)GetDlgItem(IDC_ENHANCE_PRESET7))->SetState(TRUE); break;
		case 8: ((CButton*)GetDlgItem(IDC_ENHANCE_PRESET8))->SetState(TRUE); break;
		}
	}
	else
	{
		((CButton*)GetDlgItem(IDC_ENHANCE_ALL_OFF))->SetState(TRUE);
	}

	EnableOptions(m_pEnhancementControl->IsEnhancementEnabled());

	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();
	m_sldrContrast.SetValue(pSettings->GetContrastEnhancement());
	m_sldrSharpness.SetValue(pSettings->GetSharpnessEnhancement());
	m_sldrGamma.SetFloatValue(pSettings->GetGamma());
	m_sldrGammaSide.SetFloatValue(pSettings->GetGammaSide());
	m_sldrBrightness.SetFloatValue(pSettings->GetBrightness());
	m_sldrTemporalAveraging.SetValue(pSettings->GetTemporalAveraging());
	m_sldrSpatialAveraging.SetValue(pSettings->GetSpatialAveraging());
	m_sldrFrontScaleSlope.SetFloatValue(pSettings->GetFrontScaleSlope());
	m_sldrScaleSlope.SetFloatValue(pSettings->GetSideScaleSlope());
	m_sldrScaleOffset.SetFloatValue(pSettings->GetSideScaleOffset());

	GetDlgItem(IDC_BUTTON_TEMPORAL_AVERAGING_TYPE)->SetWindowTextW(
		pSettings->GetTemporalMedianAveraging() ?
		_T("Median") : _T("Mean"));

	GetDlgItem(IDC_BUTTON_SPATIAL_AVERAGING_TYPE)->SetWindowTextW(
		pSettings->GetSpatialMedianAveraging() ?
		_T("Median") : _T("Mean"));

	int upsample = pSettings->GetUpsampleFactor();
	((CButton*)GetDlgItem(IDC_RADIO_UPSAMPLE1))->SetCheck(upsample == 1);
	((CButton*)GetDlgItem(IDC_RADIO_UPSAMPLE2))->SetCheck(upsample == 2);
	((CButton*)GetDlgItem(IDC_RADIO_UPSAMPLE3))->SetCheck(upsample == 3);
}


void CEnhancementDlg::OnBnClickedEnchancePreset1()
{
	m_pEnhancementControl->SetCurrentPreset(1);
	
	ClearPresetSelect();
	UpdatePresetSelect();
	Invalidate();
}


void CEnhancementDlg::OnBnClickedEnchancePreset2()
{
	m_pEnhancementControl->SetCurrentPreset(2);

	ClearPresetSelect();
	UpdatePresetSelect();
	Invalidate();
}


void CEnhancementDlg::OnBnClickedEnchancePreset3()
{
	m_pEnhancementControl->SetCurrentPreset(3);
	
	ClearPresetSelect();
	UpdatePresetSelect();
	Invalidate();
}


void CEnhancementDlg::OnBnClickedEnchancePreset4()
{
	m_pEnhancementControl->SetCurrentPreset(4);
	
	ClearPresetSelect();
	UpdatePresetSelect();
	Invalidate();
}


void CEnhancementDlg::OnBnClickedEnchancePreset5()
{
	m_pEnhancementControl->SetCurrentPreset(5);
	
	ClearPresetSelect();
	UpdatePresetSelect();
	Invalidate();
}


void CEnhancementDlg::OnBnClickedEnchancePreset6()
{
	m_pEnhancementControl->SetCurrentPreset(6);
	
	ClearPresetSelect();
	UpdatePresetSelect();
	Invalidate();
}


void CEnhancementDlg::OnBnClickedEnchancePreset7()
{
	m_pEnhancementControl->SetCurrentPreset(7);
	
	ClearPresetSelect();
	UpdatePresetSelect();
	Invalidate();
}


void CEnhancementDlg::OnBnClickedEnchancePreset8()
{
	m_pEnhancementControl->SetCurrentPreset(8);
	
	ClearPresetSelect();
	UpdatePresetSelect();
	Invalidate();
}

void CEnhancementDlg::OnBnClickedEnhanceAllOff()
{	
	m_pEnhancementControl->EnableEnhancement(false);
	
	ClearPresetSelect();
	UpdatePresetSelect();
	Invalidate();
}

void CEnhancementDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();

	if (pScrollBar == (CScrollBar *)&m_sldrContrast)
	{
		int value = m_sldrContrast.GetValue();
		int setting = pSettings->GetContrastEnhancement();
		if (setting != value)
		{
			pSettings->SetContrastEnhancement(value);
		}
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrSharpness)
	{
		int value = m_sldrSharpness.GetValue();
		int setting = pSettings->GetSharpnessEnhancement();
		if (setting != value)
		{
			pSettings->SetSharpnessEnhancement(value);
		}
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrTemporalAveraging)
	{
		int value = m_sldrTemporalAveraging.GetValue();
		int setting = pSettings->GetTemporalAveraging();
		if (setting != value)
		{
			pSettings->SetTemporalAveraging(value);
		}
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrSpatialAveraging)
	{
		int value = m_sldrSpatialAveraging.GetValue();
		int setting = pSettings->GetSpatialAveraging();
		if (setting != value)
		{
			pSettings->SetSpatialAveraging(value);
		}
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrGamma)
	{
		float value = m_sldrGamma.GetFloatValue();
		float setting = pSettings->GetGamma();
		if (setting != value)
		{
			pSettings->SetGamma(value);
		}
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrGammaSide)
	{
		float value = m_sldrGammaSide.GetFloatValue();
		float setting = pSettings->GetGammaSide();
		if (setting != value)
		{
			pSettings->SetGammaSide(value);
		}
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrBrightness)
	{
		float value = m_sldrBrightness.GetFloatValue();
		float setting = pSettings->GetBrightness();
		if (setting != value)
		{
			pSettings->SetBrightness(value);
		}
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrFrontScaleSlope)
	{
		float value = m_sldrFrontScaleSlope.GetFloatValue();
		float setting = pSettings->GetFrontScaleSlope();
		if (setting != value)
		{
			pSettings->SetFrontScaleSlope(value);
		}
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrScaleSlope)
	{
		float value = m_sldrScaleSlope.GetFloatValue();
		float setting = pSettings->GetSideScaleSlope();
		if (setting != value)
		{
			pSettings->SetSideScaleSlope(value);
		}
	}
	else if (pScrollBar == (CScrollBar *)&m_sldrScaleOffset)
	{
		float value = m_sldrScaleOffset.GetFloatValue();
		float setting = pSettings->GetSideScaleOffset();
		if (setting != value)
		{
			pSettings->SetSideScaleOffset(value);
		}
	}
	else{
		CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
	}
}

void CEnhancementDlg::OnBnClickedEnhanceContrastOnOff()
{
	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();
	pSettings->EnableContrastEnhancement(!pSettings->IsContrastEnhancementEnabled());
	Invalidate();
}


void CEnhancementDlg::OnBnClickedEnhanceSharpnessOnOff()
{
	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();
	pSettings->EnableSharpnessEnhancement(!pSettings->IsSharpnessEnhancementEnabled());
	Invalidate();
}

void CEnhancementDlg::OnBnClickedEnhanceBrightnessOnOff()
{
	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();
	pSettings->EnableBrightness(!pSettings->IsBrightnessEnabled());
	Invalidate();
}

void CEnhancementDlg::OnBnClickedEnhanceGammaOnOff()
{
	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();
	pSettings->EnableGamma(!pSettings->IsGammaEnabled());
	Invalidate();
}


void CEnhancementDlg::OnBnClickedEnhanceGammaSideOnOff()
{
	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();
	pSettings->EnableGammaSide(!pSettings->IsGammaSideEnabled());
	Invalidate();
}


void CEnhancementDlg::OnBnClickedTemporalAveragingOnOff()
{
	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();
	pSettings->EnableTemporalAveraging(!pSettings->IsTemporalAveragingEnabled());
	Invalidate();
}


void CEnhancementDlg::OnBnClickedButtonTemporalAveragingType()
{
	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();
	pSettings->SetTemporalMedianAveraging(!pSettings->GetTemporalMedianAveraging());
	UpdatePresetSelect();
}


void CEnhancementDlg::OnBnClickedSpatialAveragingOnOff()
{
	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();
	pSettings->EnableSpatialAveraging(!pSettings->IsSpatialAveragingEnabled());
	Invalidate();
}


void CEnhancementDlg::OnBnClickedButtonSpatialAveragingType()
{
	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();
	pSettings->SetSpatialMedianAveraging(!pSettings->GetSpatialMedianAveraging());
	UpdatePresetSelect();
}


void CEnhancementDlg::OnBnClickedScalingOnOff()
{
	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();
	pSettings->EnableSideScale(!pSettings->IsSideScaleEnabled());
	Invalidate();
}


void CEnhancementDlg::OnBnClickedFrontScalingOnOff()
{
	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();
	pSettings->EnableFrontScale(!pSettings->IsFrontScaleEnabled());
	Invalidate();
}

void CEnhancementDlg::OnBnClickedRadioUpsample1()
{
	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();
	pSettings->SetUpsampleFactor(1);
}


void CEnhancementDlg::OnBnClickedRadioUpsample2()
{
	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();
	pSettings->SetUpsampleFactor(2);
}


void CEnhancementDlg::OnBnClickedRadioUpsample3()
{
	CEnhancementSettings *pSettings = m_pEnhancementControl->GetCurrentSettings();
	pSettings->SetUpsampleFactor(3);
}
