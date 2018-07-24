// ClientDlg.h : header file
//

#if !defined(AFX_CLIENTDLG_H__D20E8B6B_CB8D_4297_B0FC_9D56BFCC6B53__INCLUDED_)
#define AFX_CLIENTDLG_H__D20E8B6B_CB8D_4297_B0FC_9D56BFCC6B53__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "winsock.h"
#pragma comment(lib,"ws2_32")
/////////////////////////////////////////////////////////////////////////////
// CClientDlg dialog

class CClientDlg : public CDialog
{
// Construction
public:
	CClientDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CClientDlg)
	enum { IDD = IDD_CLIENT_DIALOG };
	CEdit	m_Recv;
	CString	m_Send;
	CString	m_HostIp;
	int		m_TCPPort;
	int		m_UDPPort;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClientDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CClientDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	afx_msg void OnButton1();
	afx_msg void OnButton3();
	afx_msg void OnButton2();
	afx_msg void OnButton4();
	afx_msg void OnBtnConnect();
	afx_msg void OnBtnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	SOCKET m_Socket_TCP;
	BOOL bTCPInit;
	SOCKADDR_IN m_addrTCP_Host;
	SOCKADDR_IN m_addrTCP_Client;

	SOCKET m_Socket_UDP;
	BOOL bUDPInit;
	SOCKADDR_IN m_addrUDP_Host;
	SOCKADDR_IN m_addrUDP_Client;

	int nAddrLen;
	int ParseCMD(SOCKET s,CString sSend,CString &sCMD,CString &sName);
	void GetFile(CString sIp,CString pName,int nFileLen);
	void GetShareFileList();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLIENTDLG_H__D20E8B6B_CB8D_4297_B0FC_9D56BFCC6B53__INCLUDED_)
