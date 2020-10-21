// SignalOutputDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NGEuserApp.h"
#include "SignalOutputDlg.h"
#include "SignalOutput.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSignalOutputDlg dialog


CSignalOutputDlg::CSignalOutputDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSignalOutputDlg::IDD, pParent)
	, m_uDTROutputDuration(0)
{
	//{{AFX_DATA_INIT(CSignalOutputDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSignalOutputDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSignalOutputDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_LIST_COM_PORTS, m_lbComPorts);
	DDX_Text(pDX, IDC_EDIT_DTR_OUTPUT_DURATION, m_uDTROutputDuration);
	DDV_MinMaxUInt(pDX, m_uDTROutputDuration, 1, 1000);
}


BEGIN_MESSAGE_MAP(CSignalOutputDlg, CDialog)
	//{{AFX_MSG_MAP(CSignalOutputDlg)
	// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_WM_PAINT()
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CSignalOutputDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_SEND_NOW, &CSignalOutputDlg::OnBnClickedButtonSendNow)
	ON_EN_KILLFOCUS(IDC_EDIT_TRIGGER_STRING, &CSignalOutputDlg::OnEnKillfocusEditTriggerString)
	ON_BN_CLICKED(IDC_RADIO_BREAK_SET, &CSignalOutputDlg::OnBnClickedRadioBreakSet)
	ON_BN_CLICKED(IDC_RADIO_BREAK_CLEAR, &CSignalOutputDlg::OnBnClickedRadioBreakClear)
	ON_BN_CLICKED(IDC_RADIO_DTR_SET, &CSignalOutputDlg::OnBnClickedRadioDtrSet)
	ON_BN_CLICKED(IDC_RADIO_DTR_CLEAR, &CSignalOutputDlg::OnBnClickedRadioDtrClear)
	ON_BN_CLICKED(IDC_RADIO_RTS_SET, &CSignalOutputDlg::OnBnClickedRadioRtsSet)
	ON_BN_CLICKED(IDC_RADIO_RTS_CLEAR, &CSignalOutputDlg::OnBnClickedRadioRtsClear)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSignalOutputDlg message handlers

BOOL CSignalOutputDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CSignalOutputDlg::OnShowWindow(BOOL bShow,
	UINT nStatus)
{
	if (bShow)
	{
		m_lbComPorts.ResetContent();

		CSerialPort::EnumerateSerialPorts(m_comPortNumbers);

		int mcPort = theApp.GetSystemManager()->GetMicroController()->GetConnectedComPort();

		for (int i = 0; i < m_comPortNumbers.GetSize(); i++)
		{
			int comPort = (int)m_comPortNumbers.ElementAt(i);
			if (mcPort == comPort)
			{
				// Don't include MicroController comm port in list
				continue;
			}

			CString str;
			str.Format(_T("COM%d"), comPort);
			m_lbComPorts.AddString(str);
		}

		UpdateStatus();

		m_uDTROutputDuration = theApp.GetSystemManager()->GetSignalOutput()->GetTriggerDuration();

		UpdateData(FALSE);
	}
	else
	{
	}
}

void CSignalOutputDlg::UpdateStatus()
{
	CString status;
	int connectedPort = theApp.GetSystemManager()->GetSignalOutput()->GetConnectedComPort();
	if (connectedPort < 0)
	{
		status = "Not connected";
	}
	else
	{
		status.Format(_T("Connected to COM%d"), connectedPort);
	}

	GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(status);
}

void CSignalOutputDlg::OnBnClickedButtonConnect()
{
	int nSel = m_lbComPorts.GetCurSel();
	if (nSel != LB_ERR)
	{
		CString selectedPort;
		m_lbComPorts.GetText(nSel, selectedPort);

		int comPort;
		swscanf_s(selectedPort.GetBuffer(), L"COM%d", &comPort);
		theApp.GetSystemManager()->GetSignalOutput()->Connect(comPort);

		OnShowWindow(TRUE, 0);
	}
	else
	{
		theApp.ShowMessageBox(_T("Please select an available port"), _T("Error"), MB_OK | MB_TOPMOST);
	}
}


void CSignalOutputDlg::OnBnClickedButtonSendNow()
{
	theApp.GetSystemManager()->GetSignalOutput()->SendTrigger();
}


void CSignalOutputDlg::OnEnKillfocusEditTriggerString()
{
	UpdateData();

	theApp.GetSystemManager()->GetSignalOutput()->SetTriggerDuration(m_uDTROutputDuration);
}

void CSignalOutputDlg::OnBnClickedRadioBreakSet()
{
	theApp.GetSystemManager()->GetSignalOutput()->SetBreak(true);
}


void CSignalOutputDlg::OnBnClickedRadioBreakClear()
{
	theApp.GetSystemManager()->GetSignalOutput()->SetBreak(false);
}


void CSignalOutputDlg::OnBnClickedRadioDtrSet()
{
	theApp.GetSystemManager()->GetSignalOutput()->SetDTR(true);
}


void CSignalOutputDlg::OnBnClickedRadioDtrClear()
{
	theApp.GetSystemManager()->GetSignalOutput()->SetDTR(false);
}


void CSignalOutputDlg::OnBnClickedRadioRtsSet()
{
	theApp.GetSystemManager()->GetSignalOutput()->SetRTS(true);
}


void CSignalOutputDlg::OnBnClickedRadioRtsClear()
{
	theApp.GetSystemManager()->GetSignalOutput()->SetRTS(false);
}
