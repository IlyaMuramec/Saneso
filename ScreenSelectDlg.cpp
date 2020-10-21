// ProcessingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NGEuserApp.h"
#include "NGEuserDlg.h"
#include "ProcessingDlg.h"
#include "SelectItemDlg.h"
#include "ScreenSelectDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScreenSelectDlg dialog


CScreenSelectDlg::CScreenSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CScreenSelectDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CScreenSelectDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_nCurrentScreen = IDC_BUTTON_OPERATOR;
}

void CScreenSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScreenSelectDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScreenSelectDlg, CDialog)
	//{{AFX_MSG_MAP(CScreenSelectDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_WM_DRAWITEM()
	ON_WM_SHOWWINDOW()
	ON_CONTROL_RANGE(BN_CLICKED, IDC_BUTTON_OPERATOR, IDC_BUTTON_SIGNAL_OUTPUT, &CScreenSelectDlg::OnButtonClicked)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScreenSelectDlg message handlers

BOOL CScreenSelectDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CScreenSelectDlg::OnShowWindow(BOOL bShow, 
							   UINT nStatus)
{
	if (bShow)
	{
	}
	else
	{
	}
}

void CScreenSelectDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	COLORREF color = RGB(0, 0, 0);
	COLORREF colorText = RGB(0, 0, 0);
	COLORREF selectedColor = RGB(255, 255, 0);
	BOOL selected = FALSE;

	selected = (nIDCtl == m_nCurrentScreen);
	selected ? color = selectedColor : color = RGB(240, 240, 240);if (nIDCtl == m_nCurrentScreen)
	
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

void CScreenSelectDlg::SetScreenSelect(int screen)
{
	m_nCurrentScreen = screen + IDC_BUTTON_OPERATOR;
	Invalidate();
}

void CScreenSelectDlg::OnButtonClicked(UINT nID)
{
	m_nCurrentScreen = nID;
	GetParent()->PostMessage(WM_UPDATE_SCREEN, m_nCurrentScreen - IDC_BUTTON_OPERATOR, 0);
	Invalidate();
}