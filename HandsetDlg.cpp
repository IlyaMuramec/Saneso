// HandsetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NGEuserApp.h"
#include "HandsetDlg.h"
#include "LightingDlg.h"
#include "MainDlg.h"
#include "EnhancementDlg.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



static UINT UWM_HANDSET_CONTROLLER_MSG = RegisterWindowMessage(_T("Handset Controller Message"));
static void MicroControllerMessageCallback(void* pObject, CMessage *pMsg)
{
	if ((pMsg->type == MC_CONTROL_CHANGED) && (pMsg->size == 2))
	{
		CHandsetDlg *pDlg = (CHandsetDlg *)pObject;
		pDlg->PostMessage(UWM_HANDSET_CONTROLLER_MSG, pMsg->msgData[0], pMsg->msgData[1]);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHandsetDlg dialog


CHandsetDlg::CHandsetDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHandsetDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHandsetDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CHandsetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHandsetDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX, IDC_DIAGRAM, m_picCtrl);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_COMBO_SWITCH1, m_comboSwitch1);
	DDX_Control(pDX, IDC_COMBO_SWITCH2, m_comboSwitch2);
	DDX_Control(pDX, IDC_COMBO_SWITCH3, m_comboSwitch3);
	DDX_Control(pDX, IDC_COMBO_SWITCH4, m_comboSwitch4);
	DDX_Control(pDX, IDC_COMBO_SWITCH5, m_comboSwitch5);
}


BEGIN_MESSAGE_MAP(CHandsetDlg, CDialog)
	//{{AFX_MSG_MAP(CHandsetDlg)
	// NOTE: the ClassWizard will add message map macros here
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_SWITCH1, &CHandsetDlg::OnBnClickedSwitch1)
	ON_BN_CLICKED(IDC_SWITCH2, &CHandsetDlg::OnBnClickedSwitch2)
	ON_BN_CLICKED(IDC_SWITCH3, &CHandsetDlg::OnBnClickedSwitch3)
	ON_BN_CLICKED(IDC_SWITCH4, &CHandsetDlg::OnBnClickedSwitch4)
	ON_BN_CLICKED(IDC_SWITCH5, &CHandsetDlg::OnBnClickedSwitch5)
	ON_REGISTERED_MESSAGE(UWM_HANDSET_CONTROLLER_MSG, &CHandsetDlg::OnMicroControllerMessage)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHandsetDlg message handlers

BOOL CHandsetDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Load the logo into the picture control
	m_picCtrl.LoadFromResource(NULL, MAKEINTRESOURCE(IDB_HANDSET_DIAGRAM), _T("PNG"));

	m_AllowedActions.push_back(LIVE_FREEZE_STREAM);
	m_AllowedActions.push_back(APPLY_ENHANCEMENT_ON_OFF);
	m_AllowedActions.push_back(SELECT_BAND_IMAGING_ON_OFF);
	m_AllowedActions.push_back(STITCHED_UNSTICHED_IMAGES);
	m_AllowedActions.push_back(NEXT_COLOR);
	m_AllowedActions.push_back(PREVIOUS_COLOR);
	m_AllowedActions.push_back(FRONT_LIGHTS_ON_OFF);
	m_AllowedActions.push_back(SIDE_LIGHTS_ON_OFF);
	m_AllowedActions.push_back(APPLY_MASK_ON_OFF);
	m_AllowedActions.push_back(MARKERS_ON_OFF);
	m_AllowedActions.push_back(NEXT_ENHANCEMENT);
	m_AllowedActions.push_back(PREVIOUS_ENHANCEMENT);
	m_AllowedActions.push_back(BLENDING_ON_OFF);
	m_AllowedActions.push_back(CAPTURE_IMAGE_FILE);

	for (int i = 0; i < m_AllowedActions.size(); i++)
	{
		m_comboSwitch1.AddString(m_AllowedActions[i]);
		m_comboSwitch2.AddString(m_AllowedActions[i]);
		m_comboSwitch3.AddString(m_AllowedActions[i]);
		m_comboSwitch4.AddString(m_AllowedActions[i]);
		m_comboSwitch5.AddString(m_AllowedActions[i]);
	}

	for (int i = 1; i <= 5; i++)
	{
		m_mapButtonLookup[i] = i;
	}

	ReloadControls(DEFAULT_CONFIG);

	theApp.GetSystemManager()->GetMicroController()->AddMessageCallback(MicroControllerMessageCallback, this);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CHandsetDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (bShow)
	{
		BOOL bShowButton5 = (m_mapButtonLookup[5] > 0);
		GetDlgItem(IDC_SWITCH5)->ShowWindow(bShowButton5);
		GetDlgItem(IDC_COMBO_SWITCH5)->ShowWindow(bShowButton5);
	}
	else
	{
	}
}

void CHandsetDlg::ReloadControls(string folder)
{
	string fName = folder + "\\Handset.json";

	std::ifstream file(fName);

	Json::Value root;
	Json::Reader reader;

	bool parsingSuccessful = reader.parse(file, root);
	if (parsingSuccessful)
	{
		m_comboSwitch1.SetCurSel(root.get("Switch1", 0).asInt());
		m_comboSwitch2.SetCurSel(root.get("Switch2", 0).asInt());
		m_comboSwitch3.SetCurSel(root.get("Switch3", 0).asInt());
		m_comboSwitch4.SetCurSel(root.get("Switch4", 0).asInt());
		m_comboSwitch5.SetCurSel(root.get("Switch5", 0).asInt());
		
		for (int i = 1; i <= 5; i++)
		{
			char str[20];
			sprintf_s(str, "Translate%d", i);
			m_mapButtonLookup[i] = root.get(str, i).asInt();
		}
	}
	else
	{
		m_comboSwitch1.SetCurSel(1);
		m_comboSwitch2.SetCurSel(8);
		m_comboSwitch3.SetCurSel(3);
		m_comboSwitch4.SetCurSel(9);
		m_comboSwitch5.SetCurSel(0);
	}
}

void CHandsetDlg::SaveControls(string folder)
{
	Json::Value root;

	root["Switch1"] = m_comboSwitch1.GetCurSel();
	root["Switch2"] = m_comboSwitch2.GetCurSel();
	root["Switch3"] = m_comboSwitch3.GetCurSel();
	root["Switch4"] = m_comboSwitch4.GetCurSel();
	root["Switch5"] = m_comboSwitch5.GetCurSel();
	
	for (int i = 1; i <= 5; i++)
	{
		char str[20];
		sprintf_s(str, "Translate%d", i);
		root[str] = m_mapButtonLookup[i];
	}

	string fName = folder + "\\Handset.json";
	std::ofstream file(fName);
	Json::StreamWriterBuilder styledWriterBuilder;
	styledWriterBuilder.settings_["precision"] = 5;
	file << Json::writeString(styledWriterBuilder, root);
	file.close();
}

LRESULT CHandsetDlg::OnMicroControllerMessage(WPARAM control, LPARAM value)
{
	// Only handle button down states
	if ((m_mapButtonLookup.find(control) != m_mapButtonLookup.end())
		&& (value == 1))
	{
		int switchID = GetSwitchID(m_mapButtonLookup[control]);
		SendMessage(WM_COMMAND, MAKEWPARAM(switchID, BN_CLICKED), NULL);
	}

	return 0;
}

int CHandsetDlg::GetSwitchID(int switchNum)
{
	DWORD switchID = 0;

	switch (switchNum)
	{
	case 1:
		switchID = IDC_SWITCH1;
		break;
	case 2:
		switchID = IDC_SWITCH2;
		break;
	case 3:
		switchID = IDC_SWITCH3;
		break;
	case 4:
		switchID = IDC_SWITCH4;
		break;
	case 5:
		switchID = IDC_SWITCH5;
		break;
	}

	return switchID;
}

void CHandsetDlg::DoAction(CString action)
{
	if (action == LIVE_FREEZE_STREAM)
	{
		DisplayMode mode = theApp.GetSystemManager()->GetDisplayMode();
		if (mode != eLive)
		{
			theApp.GetSystemManager()->SetNewDisplayMode(eLive);
		}
		else
		{
			theApp.GetSystemManager()->SetNewDisplayMode(eFrozen);
		}
	}
	else if (action == APPLY_ENHANCEMENT_ON_OFF)
	{
		theApp.GetEnhancementDlg()->HandsetAction(eOnOff);
	}
	else if (action == SELECT_BAND_IMAGING_ON_OFF)
	{
		theApp.GetSystemManager()->EnableSelectBandImaging(!theApp.GetSystemManager()->IsSelectBandImagingEnabled());
	}
	else if (action == STITCHED_UNSTICHED_IMAGES)
	{
		theApp.GetSystemManager()->NextImageStitchingMode();
	}
	else if (action == NEXT_COLOR)
	{
		theApp.GetLightingDlg()->CycleColor(true);
		theApp.GetOperatorDlg()->UpdateLightColor();
	}
	else if (action == PREVIOUS_COLOR)
	{
		theApp.GetLightingDlg()->CycleColor(false);
		theApp.GetOperatorDlg()->UpdateLightColor();
	}
	else if (action == FRONT_LIGHTS_ON_OFF)
	{
		theApp.GetLightingDlg()->ToggleFrontLights();
	}
	else if (action == SIDE_LIGHTS_ON_OFF)
	{
		theApp.GetLightingDlg()->ToggleSideLights();
	}
	else if (action == APPLY_MASK_ON_OFF)
	{
		theApp.GetSystemManager()->EnableApplyMask(!theApp.GetSystemManager()->IsApplyMaskEnabled());
	}
	else if (action == MARKERS_ON_OFF)
	{
		theApp.GetSystemManager()->EnableMarkerDislay(!theApp.GetSystemManager()->IsMarkerDisplayEnabled());
	}
	else if (action == NEXT_ENHANCEMENT)
	{
		theApp.GetEnhancementDlg()->HandsetAction(eNext);
	}
	else if (action == PREVIOUS_ENHANCEMENT)
	{
		theApp.GetEnhancementDlg()->HandsetAction(ePrevious);
	}
	else if (action == BLENDING_ON_OFF)
	{
		bool enabled = theApp.GetSystemManager()->GetMosaicData()->IsBlendingEnabled();
		theApp.GetSystemManager()->GetMosaicData()->EnableBlending(!enabled);
	}
	else if (action == CAPTURE_IMAGE_FILE)
	{
		theApp.GetSystemManager()->CaptureImageFile();
	}
	else
	{
		CString msg = "Unsupported Remote Button Action!\n" + action;
		theApp.ShowMessageBox(msg, _T("Error"), MB_OK | MB_TOPMOST);
	}
}

void CHandsetDlg::OnBnClickedSwitch1()
{
	CString action;
	m_comboSwitch1.GetLBText(m_comboSwitch1.GetCurSel(), action);
	DoAction(action);
}


void CHandsetDlg::OnBnClickedSwitch2()
{
	CString action;
	m_comboSwitch2.GetLBText(m_comboSwitch2.GetCurSel(), action);
	DoAction(action);
}


void CHandsetDlg::OnBnClickedSwitch3()
{
	CString action;
	m_comboSwitch3.GetLBText(m_comboSwitch3.GetCurSel(), action);
	DoAction(action);
}


void CHandsetDlg::OnBnClickedSwitch4()
{
	CString action;
	m_comboSwitch4.GetLBText(m_comboSwitch4.GetCurSel(), action);
	DoAction(action);
}

void CHandsetDlg::OnBnClickedSwitch5()
{
	CString action;
	m_comboSwitch5.GetLBText(m_comboSwitch5.GetCurSel(), action);
	DoAction(action);
}
