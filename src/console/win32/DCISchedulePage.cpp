// DCISchedulePage.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DCISchedulePage.h"
#include "InputBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDCISchedulePage property page

IMPLEMENT_DYNCREATE(CDCISchedulePage, CPropertyPage)

CDCISchedulePage::CDCISchedulePage() : CPropertyPage(CDCISchedulePage::IDD)
{
	//{{AFX_DATA_INIT(CDCISchedulePage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CDCISchedulePage::~CDCISchedulePage()
{
}

void CDCISchedulePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDCISchedulePage)
	DDX_Control(pDX, IDC_LIST_SCHEDULES, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDCISchedulePage, CPropertyPage)
	//{{AFX_MSG_MAP(CDCISchedulePage)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_MODIFY, OnButtonModify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDCISchedulePage message handlers


//
// WM_INITDIALOG message handler
//

BOOL CDCISchedulePage::OnInitDialog() 
{
   DWORD i;
   RECT rect;

	CPropertyPage::OnInitDialog();

   // Setup list control
   m_wndListCtrl.GetClientRect(&rect);
   rect.right -= GetSystemMetrics(SM_CXVSCROLL);
   m_wndListCtrl.InsertColumn(0, _T("Schedule"), LVCFMT_LEFT, rect.right);
	
   for(i = 0; i < m_dwNumSchedules; i++)
      m_wndListCtrl.InsertItem(0x7FFFFFFF, m_ppScheduleList[i]);
	
	return TRUE;
}


//
// "Add" button
//

void CDCISchedulePage::OnButtonAdd() 
{
   CInputBox dlgInput;

   dlgInput.m_strHeader = _T("Schedule");
   dlgInput.m_strTitle = _T("New schedule");
   if (dlgInput.DoModal() == IDOK)
   {
      m_ppScheduleList = (TCHAR **)realloc(m_ppScheduleList, sizeof(TCHAR *) * (m_dwNumSchedules + 1));
      m_ppScheduleList[m_dwNumSchedules] = _tcsdup((LPCTSTR)dlgInput.m_strText);
      StrStrip(m_ppScheduleList[m_dwNumSchedules]);
      m_wndListCtrl.InsertItem(0x7FFFFFFF, m_ppScheduleList[m_dwNumSchedules]);
      m_dwNumSchedules++;
   }
}


//
// "Modify" button
//

void CDCISchedulePage::OnButtonModify() 
{
   int iItem;
   CInputBox dlgInput;

   if (m_wndListCtrl.GetSelectedCount() != 1)
      return;

   iItem = m_wndListCtrl.GetSelectionMark();
   dlgInput.m_strHeader = _T("Schedule");
   dlgInput.m_strTitle = _T("Modify schedule");
   dlgInput.m_strText = m_ppScheduleList[iItem];
   if (dlgInput.DoModal() == IDOK)
   {
      free(m_ppScheduleList[iItem]);
      m_ppScheduleList[iItem] = _tcsdup((LPCTSTR)dlgInput.m_strText);
      StrStrip(m_ppScheduleList[iItem]);
      m_wndListCtrl.SetItemText(iItem, 0, m_ppScheduleList[iItem]);
   }
}
