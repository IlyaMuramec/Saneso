// MicroControlDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NGEuserApp.h"
#include "MicroControlDlg.h"
#include "Timers.h"
#include "SplashScreen.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static UINT UWM_MICRO_CONTROLLER_MSG = RegisterWindowMessage(_T("Micro Controller Message"));
static void MicroControllerMessageCallback(void* pObject, CMessage *pMsg)
{
	CMicroControlDlg *pDlg = (CMicroControlDlg *)pObject;

	//Make a copy fo the message and post it
	CMessage *pNewMsg = new CMessage(*pMsg);
	pDlg->PostMessage(UWM_MICRO_CONTROLLER_MSG, (WPARAM)pNewMsg);
}

/////////////////////////////////////////////////////////////////////////////
// CMicroControlDlg dialog


CMicroControlDlg::CMicroControlDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMicroControlDlg::IDD, pParent)
	, m_sVersion(_T("Not Connected"))
	, m_uLEDMUXPeriod(0)
{
	//{{AFX_DATA_INIT(CMicroControlDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CMicroControlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMicroControlDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_EDIT_VERSION, m_sVersion);
	DDX_Text(pDX, IDC_EDIT_STATUS, m_sStatus);
	DDX_Control(pDX, IDC_LIST_COM_PORTS, m_lbComPorts);
	DDX_Control(pDX, IDC_EDIT_EEPROM_BOX, m_editEEPromBox);
	DDX_Control(pDX, IDC_EDIT_EEPROM_SCOPE, m_editEEPromScope);
	DDX_Text(pDX, IDC_EDIT_LED_MUX_PERIOD, m_uLEDMUXPeriod);
}


BEGIN_MESSAGE_MAP(CMicroControlDlg, CDialog)
	//{{AFX_MSG_MAP(CMicroControlDlg)
	// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_WM_PAINT()
	ON_WM_SHOWWINDOW()
	ON_WM_TIMER()
	ON_REGISTERED_MESSAGE(UWM_MICRO_CONTROLLER_MSG, &CMicroControlDlg::OnMicroControllerMessage)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CMicroControlDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_EEPROM_BOX_READ, &CMicroControlDlg::OnBnClickedButtonEepromBoxRead)
	ON_BN_CLICKED(IDC_BUTTON_EEPROM_BOX_WRITE, &CMicroControlDlg::OnBnClickedButtonEepromBoxWrite)
	ON_BN_CLICKED(IDC_BUTTON_EEPROM_SCOPE_READ, &CMicroControlDlg::OnBnClickedButtonEepromScopeRead)
	ON_BN_CLICKED(IDC_BUTTON_EEPROM_SCOPE_WRITE, &CMicroControlDlg::OnBnClickedButtonEepromScopeWrite)
	ON_BN_CLICKED(IDC_BUTTON_EEPROM_BOX_READ_FILE, &CMicroControlDlg::OnBnClickedButtonEepromBoxReadFile)
	ON_BN_CLICKED(IDC_BUTTON_EEPROM_BOX_WRITE_FILE, &CMicroControlDlg::OnBnClickedButtonEepromBoxWriteFile)
	ON_BN_CLICKED(IDC_BUTTON_EEPROM_SCOPE_READ_FILE, &CMicroControlDlg::OnBnClickedButtonEepromScopeReadFile)
	ON_BN_CLICKED(IDC_BUTTON_EEPROM_SCOPE_WRITE_FILE, &CMicroControlDlg::OnBnClickedButtonEepromScopeWriteFile)
	ON_BN_CLICKED(IDC_BUTTON_DEMO_FEATURES, &CMicroControlDlg::OnBnClickedButtonDemoFeatures)
	ON_EN_KILLFOCUS(IDC_EDIT_LED_MUX_PERIOD, &CMicroControlDlg::OnEnKillfocusEditLedMuxPeriod)
	ON_BN_CLICKED(IDC_BUTTON_REFLASH, &CMicroControlDlg::OnBnClickedButtonReflash)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMicroControlDlg message handlers

BOOL CMicroControlDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	theApp.GetSystemManager()->GetMicroController()->AddMessageCallback(MicroControllerMessageCallback, this);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CMicroControlDlg::OnShowWindow(BOOL bShow,
	UINT nStatus)
{
	if (bShow)
	{
		m_lbComPorts.ResetContent();

		CSerialPort::EnumerateSerialPorts(m_comPortNumbers);

		for (int i = 0; i < m_comPortNumbers.GetSize(); i++)
		{
			int comPort = (int)m_comPortNumbers.ElementAt(i);

			CString str;
			str.Format(_T("COM%d"), comPort);
			m_lbComPorts.AddString(str);
		}

		int connectedPort = theApp.GetSystemManager()->GetMicroController()->GetConnectedComPort();
		if (connectedPort < 0)
		{
			m_lbComPorts.SetCurSel(-1);
		}
		else
		{
			CString str;
			str.Format(_T("COM%d"), connectedPort);
			m_lbComPorts.SelectString(-1, str);
		}
		
		m_sVersion = theApp.GetSystemManager()->GetMicroController()->GetVersion().c_str();
		m_uLEDMUXPeriod = theApp.GetSystemManager()->GetLightControl()->GetLightCalSettings()->GetMUXRate();

		UpdateData(FALSE);
	}
	else
	{
	}
}

void CMicroControlDlg::OnTimer(UINT_PTR nIDEvent)
{
	theApp.GetSystemManager()->SetMicroControllerReflashing(false);

	// Hide the splash screen
	CSplashThread* pSplashThread = theApp.GetSplashThread();
	pSplashThread->HideSplash();

	KillTimer(IDT_TIMER_REFLASH);
}

LRESULT CMicroControlDlg::OnMicroControllerMessage(WPARAM control, LPARAM value)
{
	CMessage *pMsg = (CMessage*)control;
	if ((pMsg->type == MC_STATUS_MESSAGE)
		|| (pMsg->type == GET_STATUS_RESPONSE))
	{
		m_sStatus.Format(_T("0x%02x 0x%02x 0x%02x 0x%02x"), pMsg->msgData[0], pMsg->msgData[1], pMsg->msgData[2], pMsg->msgData[3]);
		//UpdateData(FALSE);
	}

	delete pMsg;

	return 0;
}

void CMicroControlDlg::OnBnClickedButtonConnect()
{
	theApp.GetSystemManager()->GetMicroController()->Connect();

	OnShowWindow(TRUE, 0);
}


void CMicroControlDlg::OnBnClickedButtonEepromBoxRead()
{
	string value = theApp.GetSystemManager()->GetMicroController()->ReadEEPromSN(ProcessorBox);
	CString sValue = CStringA(value.c_str());
	m_editEEPromBox.SetWindowText(sValue);

	UpdateData(FALSE);
}


void CMicroControlDlg::OnBnClickedButtonEepromBoxWrite()
{
	UpdateData();

	CString sValue;
	m_editEEPromBox.GetWindowText(sValue);
	string value = CStringA(sValue);
	theApp.GetSystemManager()->GetMicroController()->WriteEEPromSN(ProcessorBox, value);
}


void CMicroControlDlg::OnBnClickedButtonEepromScopeRead()
{
	string value = theApp.GetSystemManager()->GetMicroController()->ReadEEPromSN(Scope);
	CString sValue = CStringA(value.c_str());
	m_editEEPromScope.SetWindowText(sValue);

	UpdateData(FALSE);
}


void CMicroControlDlg::OnBnClickedButtonEepromScopeWrite()
{
	UpdateData();

	CString sValue;
	m_editEEPromScope.GetWindowText(sValue);
	string value = CStringA(sValue);
	theApp.GetSystemManager()->GetMicroController()->WriteEEPromSN(Scope, value);
}


void CMicroControlDlg::OnBnClickedButtonEepromBoxReadFile()
{
	ReadEEPromToFile(ProcessorBox);
}


void CMicroControlDlg::OnBnClickedButtonEepromBoxWriteFile()
{
	WriteEEPromFromFile(ProcessorBox);
}


void CMicroControlDlg::OnBnClickedButtonEepromScopeReadFile()
{
	ReadEEPromToFile(Scope);
}


void CMicroControlDlg::OnBnClickedButtonEepromScopeWriteFile()
{
	WriteEEPromFromFile(Scope);
}


void CMicroControlDlg::ReadEEPromToFile(EEPromLocation location)
{
	string str = theApp.GetSystemManager()->GetMicroController()->ReadEEProm(location);

	CString filters = _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||");
	CFileDialog fileDlg(FALSE, _T("txt"), _T("*.txt"),
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, filters, this);

	if (fileDlg.DoModal() == IDOK)
	{
		std::ofstream file((CStringA)fileDlg.GetPathName());
		file << str;

		theApp.ShowMessageBox(_T("EEProm data read to file is complete"), _T("Info"), MB_OK | MB_TOPMOST);
	}
}

void CMicroControlDlg::WriteEEPromFromFile(EEPromLocation location)
{
	CString filters = _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||");
	CFileDialog fileDlg(TRUE, _T("txt"), _T("*.txt"),
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, filters, this);

	if (fileDlg.DoModal() == IDOK)
	{
		std::ifstream file((CStringA)fileDlg.GetPathName());
		std::string str((std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());

		theApp.GetSystemManager()->GetMicroController()->WriteEEProm(location, str);

		theApp.ShowMessageBox(_T("EEProm data write from file is complete"), _T("Info"), MB_OK | MB_TOPMOST);
	}
}

void CMicroControlDlg::OnBnClickedButtonDemoFeatures()
{
	theApp.GetSystemManager()->GetLightControl()->ToggleDemoMode();
}


void CMicroControlDlg::OnEnKillfocusEditLedMuxPeriod()
{
	UpdateData();

	theApp.GetSystemManager()->GetLightControl()->GetLightCalSettings()->SetMUXRate(m_uLEDMUXPeriod);
	theApp.GetSystemManager()->GetLightControl()->UpdateLEDMUXPeriod(m_uLEDMUXPeriod, true);
}


void CMicroControlDlg::OnBnClickedButtonReflash()
{
	int ret = theApp.ShowMessageBox(_T("Are you sure you want to reflash the MicroController?"), _T("Warning"), MB_OKCANCEL | MB_TOPMOST);
	if (ret == IDOK)
	{
		// Display the splash screen
		CSplashThread* pSplashThread = theApp.GetSplashThread();
		pSplashThread->SetText("Please reprogram with USB Bootloader now");
		pSplashThread->ShowSplash();

		theApp.GetSystemManager()->SetMicroControllerReflashing(true);
		theApp.GetSystemManager()->GetMicroController()->SendReflash();

		// Set the timer. 
		SetTimer(IDT_TIMER_REFLASH,               // timer identifier 
			30000,                     // 30-second interval 
			NULL); // timer callback
	}
}
