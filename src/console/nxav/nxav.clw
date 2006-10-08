; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CSettingsDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "nxav.h"
LastPage=0

ClassCount=9
Class1=CAlarmViewApp
Class3=CMainFrame
Class4=CAboutDlg

ResourceCount=11
Resource1=IDD_REQUEST_WAIT
Resource2=IDD_ABOUTBOX
Class2=CChildView
Class5=CAlarmList
Class6=CInfoLine
Class7=CRequestProcessingDlg
Resource3=IDD_ABOUTBOX (English (U.S.))
Resource4=IDR_MAINFRAME
Class9=CSettingsDlg
Resource5=IDM_CONTEXT
Resource7=IDR_MAINFRAME (English (U.S.))
Resource8=IDD_REQUEST_WAIT (English (U.S.))
Resource9=IDD_SETTINGS (English (U.S.))
Class8=CAlarmBrowser
Resource10=IDM_CONTEXT (English (U.S.))
Resource11=IDD_SETTINGS

[CLS:CAlarmViewApp]
Type=0
HeaderFile=nxav.h
ImplementationFile=nxav.cpp
Filter=N
BaseClass=CWinApp
VirtualFilter=AC

[CLS:CChildView]
Type=0
HeaderFile=ChildView.h
ImplementationFile=ChildView.cpp
Filter=N

[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=W
BaseClass=CFrameWnd
VirtualFilter=fWC




[CLS:CAboutDlg]
Type=0
HeaderFile=nxav.cpp
ImplementationFile=nxav.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_EDIT_COPY
Command2=ID_EDIT_PASTE
Command3=ID_EDIT_UNDO
Command4=ID_EDIT_CUT
Command5=ID_VIEW_REFRESH
Command6=ID_NEXT_PANE
Command7=ID_PREV_PANE
Command8=ID_EDIT_COPY
Command9=ID_EDIT_PASTE
Command10=ID_EDIT_CUT
Command11=ID_EDIT_UNDO
CommandCount=11

[CLS:CAlarmList]
Type=0
HeaderFile=AlarmList.h
ImplementationFile=AlarmList.cpp
BaseClass=CListCtrl
Filter=W
VirtualFilter=FWC

[CLS:CInfoLine]
Type=0
HeaderFile=InfoLine.h
ImplementationFile=InfoLine.cpp
BaseClass=CWnd
Filter=W
VirtualFilter=WC
LastObject=CInfoLine

[DLG:IDD_REQUEST_WAIT]
Type=1
Class=CRequestProcessingDlg
ControlCount=2
Control1=IDC_STATIC,static,1342177283
Control2=IDC_INFO_TEXT,static,1342308352

[CLS:CRequestProcessingDlg]
Type=0
HeaderFile=RequestProcessingDlg.h
ImplementationFile=RequestProcessingDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=CRequestProcessingDlg

[MNU:IDM_CONTEXT]
Type=1
Class=?
Command1=ID_VIEW_REFRESH
Command2=ID_CMD_EXIT
Command3=ID_CMD_SETTINGS
CommandCount=3

[CLS:CAlarmBrowser]
Type=0
HeaderFile=AlarmBrowser.h
ImplementationFile=AlarmBrowser.cpp
BaseClass=CHtmlView
Filter=W
VirtualFilter=7VWC

[ACL:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
Command1=ID_EDIT_COPY
Command2=ID_EDIT_PASTE
Command3=ID_EDIT_UNDO
Command4=ID_EDIT_CUT
Command5=ID_VIEW_REFRESH
Command6=ID_NEXT_PANE
Command7=ID_PREV_PANE
Command8=ID_EDIT_COPY
Command9=ID_EDIT_PASTE
Command10=ID_EDIT_CUT
Command11=ID_EDIT_UNDO
CommandCount=11

[DLG:IDD_ABOUTBOX (English (U.S.))]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_REQUEST_WAIT (English (U.S.))]
Type=1
Class=CRequestProcessingDlg
ControlCount=2
Control1=IDC_STATIC,static,1342177283
Control2=IDC_INFO_TEXT,static,1342308352

[MNU:IDM_CONTEXT (English (U.S.))]
Type=1
Class=?
Command1=ID_VIEW_REFRESH
Command2=ID_CMD_EXIT
Command3=ID_CMD_SETTINGS
CommandCount=3

[DLG:IDD_SETTINGS (English (U.S.))]
Type=1
Class=CSettingsDlg
ControlCount=13
Control1=IDC_CHECK_AUTOLOGIN,button,1342242819
Control2=IDC_EDIT_SERVER_NAME,edit,1350631552
Control3=IDC_EDIT_USER,edit,1350631552
Control4=IDC_EDIT_PASSWD,edit,1350631584
Control5=IDC_CHECK_REPEAT,button,1342252035
Control6=IDC_CONFIGURE_SOUNDS,button,1342242816
Control7=IDOK,button,1342242817
Control8=IDCANCEL,button,1342242816
Control9=IDC_STATIC,button,1342177287
Control10=IDC_STATIC_SERVER,static,1342308352
Control11=IDC_STATIC_USER,static,1342308352
Control12=IDC_STATIC_PASSWORD,static,1342308352
Control13=IDC_STATIC,button,1342177287

[CLS:CSettingsDlg]
Type=0
HeaderFile=SettingsDlg.h
ImplementationFile=SettingsDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CSettingsDlg

[DLG:IDD_SETTINGS]
Type=1
ControlCount=13
Control1=IDC_CHECK_AUTOLOGIN,button,1342242819
Control2=IDC_EDIT_SERVER_NAME,edit,1350631552
Control3=IDC_EDIT_USER,edit,1350631552
Control4=IDC_EDIT_PASSWD,edit,1350631584
Control5=IDC_CHECK_REPEAT,button,1342252035
Control6=IDC_CONFIGURE_SOUNDS,button,1342242816
Control7=IDOK,button,1342242817
Control8=IDCANCEL,button,1342242816
Control9=IDC_STATIC,button,1342177287
Control10=IDC_STATIC_SERVER,static,1342308352
Control11=IDC_STATIC_USER,static,1342308352
Control12=IDC_STATIC_PASSWORD,static,1342308352
Control13=IDC_STATIC,button,1342177287

