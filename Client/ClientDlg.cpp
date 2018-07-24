// ClientDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Client.h"
#include "ClientDlg.h"
#include "io.h"
#include "windows.h"
#define   _WIN32_WINNT   0x0400 
#include "Wincrypt.h"
#pragma comment(lib,"Advapi32.lib")


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

struct FILELIST
{
	char	pName[256];
	char	pHash[41];
	int		nFileLen;
};
FILELIST *filelist = NULL;
int nShareFileNum = 0;
CString sShareRoot;
CString sDownload;


#define BUFFLEN 60000
char pSendBuff[BUFFLEN];
char pRecvBuff[BUFFLEN];

UINT LISTENTHREAD(LPVOID);


// 计算Hash，成功返回0，失败返回GetLastError()
//  CONST BYTE *pbData, // 输入数据
//  DWORD dwDataLen,     // 输入数据字节长度
//  ALG_ID algId       // Hash 算法：CALG_MD5,CALG_SHA
//  LPTSTR pszHash,  // 输出16进制Hash字符串，MD5长度为32+1, SHA长度为40+1
DWORD GetHash(BYTE *pbData, DWORD dwDataLen, ALG_ID algId, LPTSTR pszHash);


class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClientDlg dialog

CClientDlg::CClientDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CClientDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CClientDlg)
	m_Send = _T("");
	m_HostIp = _T("172.16.0.239");//_T("127.0.0.1");
	m_TCPPort = 5000;
	m_UDPPort = 5001;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CClientDlg)
	DDX_Control(pDX, IDC_EDIT2, m_Recv);
	DDX_Text(pDX, IDC_EDIT1, m_Send);
	DDX_Text(pDX, IDC_EDIT_IP, m_HostIp);
	DDX_Text(pDX, IDC_EDIT_TCPPORT, m_TCPPort);
	DDX_Text(pDX, IDC_EDIT_UDPPORT, m_UDPPort);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CClientDlg, CDialog)
	//{{AFX_MSG_MAP(CClientDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	ON_BN_CLICKED(IDC_BUTTON3, OnButton3)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	ON_BN_CLICKED(IDC_BUTTON4, OnButton4)
	ON_BN_CLICKED(IDC_BTN_CONNECT, OnBtnConnect)
	ON_BN_CLICKED(IDC_BTN_HELP, OnBtnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClientDlg message handlers

BOOL CClientDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	bTCPInit = 0;
	bUDPInit = 0;
	((CButton*)GetDlgItem(IDC_BUTTON1))->EnableWindow(0);
	
	GetShareFileList();
	CreateThread(0,0,(LPTHREAD_START_ROUTINE)LISTENTHREAD,0,0,0);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CClientDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CClientDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CClientDlg::OnOK() 
{

}
void CClientDlg::GetShareFileList()
{
	char pCurr[256] = {0};
	GetCurrentDirectory(256,pCurr);
	sShareRoot = pCurr;
	sShareRoot += "\\ShareFiles\\";
	sDownload = pCurr;
	sDownload += "\\Download\\";
	CreateDirectory(sDownload,0);

	nShareFileNum = 0;

	BYTE *pBuff;
	FILE *fp;

	CFileFind fl;
	BOOL bDone;
	int i;
	CString sSearch = sShareRoot + "*.*";
	SetCurrentDirectory(sShareRoot);
	bDone = fl.FindFile(sSearch);
	if(!bDone)
	{
		SetCurrentDirectory(pCurr);
		return;
	}
	while(bDone)
	{
		bDone = fl.FindNextFile();
		if(fl.IsDots())
			continue;
		nShareFileNum++;
	}
	fl.Close();

	filelist = new FILELIST[nShareFileNum];
	i = 0;
	bDone = fl.FindFile(sSearch);
	if(!bDone)
	{
		SetCurrentDirectory(pCurr);
		return;
	}
	while(bDone)
	{
		bDone = fl.FindNextFile();
		if(fl.IsDots())
			continue;
		strcpy(filelist[i].pName,fl.GetFileName());
		_strlwr(filelist[i].pName);
		fp = fopen(filelist[i].pName,"rb");
		if(fp)
		{
			filelist[i].nFileLen = _filelength(_fileno(fp));
			pBuff = new BYTE[filelist[i].nFileLen];
			fread(pBuff,1,filelist[i].nFileLen,fp);
			fclose(fp);
			GetHash(pBuff,filelist[i].nFileLen,CALG_SHA,filelist[i].pHash);
			delete pBuff;
		}
		else
		{
			filelist[i].nFileLen = 0;
			memset(filelist[i].pHash,0,41);
		}
		i++;
	}
	fl.Close();
	SetCurrentDirectory(pCurr);
}


///tcp
void CClientDlg::OnButton1() 
{
	UpdateData();
	if( m_HostIp.IsEmpty() )//|| m_TCPPort == 0 )
	{
		MessageBox("请输入命令");
		return;
	}
	int nRet;
	CString sCommand,sName;
	char *pRecv = NULL;
	int nRecvBuffLen;
	CString ss,s1;
	int nBuffLen;
	CString sIp;
	int nRequestFileLen;

	nRecvBuffLen = ParseCMD(m_Socket_TCP,m_Send,sCommand,sName);
	ss = "CMD>";
	ss += m_Send;
	ss += "\r\n";
	m_Send = "";
	UpdateData(FALSE);

	memset(pRecvBuff,0,BUFFLEN);
	nRet = recv(m_Socket_TCP,pRecvBuff,BUFFLEN,0);
	if( nRet <= 0 )
	{
		closesocket(m_Socket_TCP);
		MessageBox("链接关闭");
		((CButton*)GetDlgItem(IDC_BUTTON1))->EnableWindow(0);
		((CButton*)GetDlgItem(IDC_BTN_CONNECT))->EnableWindow(1);
		bTCPInit = 0;
		return;
	}

	sCommand.MakeUpper();
	if(sCommand == "ADD" || sCommand == "DELETE" || sCommand == "QUIT")
	{
		ss += pRecvBuff;
	}
	else if(sCommand == "REQUEST")
	{
		if(!memcmp(pRecvBuff,"ERR NO FILE",11))
		{
			s1 = "没有找到资源";
			ss += s1;	
			sIp = "";
		}
		else
		{
			memcpy(&nRequestFileLen,pRecvBuff+4,4);
			sIp.Format("%d.%d.%d.%d",(BYTE)pRecvBuff[0],(BYTE)pRecvBuff[1],(BYTE)pRecvBuff[2],(BYTE)pRecvBuff[3]);
			s1.Format("Peer:%s  FileLength:%d",sIp,nRequestFileLen);
			ss += s1;	
		}
	}
	else if(sCommand == "LIST")
	{
		//number 4byte|namelen 1byte|name|filelen 4byte|
		//            |...|
		memcpy(&nBuffLen,pRecvBuff,4);
		pRecv = new char[nBuffLen];
		send(m_Socket_TCP,"OK",2,0);

		int nHasRecv = 0;
		int nOffset;
		int nNum = 0;
		int i;
		char pName[256];
		BYTE nNameLen;
		int nFileLen;
		while(nHasRecv < nBuffLen)
		{
			nRet = recv(m_Socket_TCP,pRecv+nHasRecv,nBuffLen-nHasRecv,0);
			if( nRet <= 0 )
			{
				MessageBox("链接已断开");
				closesocket(m_Socket_TCP);
				bTCPInit = 0;
				((CButton*)GetDlgItem(IDC_BUTTON1))->EnableWindow(0);
				((CButton*)GetDlgItem(IDC_BTN_CONNECT))->EnableWindow(1);
				return;
			}
			nHasRecv += nRet;
		}
		memcpy(&nNum,pRecv,4);
		nOffset = 4;
		for(i=0;i<nNum;i++)
		{
			nNameLen = (BYTE)pRecv[nOffset];
			nOffset++;
			memset(pName,0,256);
			memcpy(pName,pRecv+nOffset,nNameLen);
			nOffset += nNameLen;
			memcpy(&nFileLen,pRecv+nOffset,4);
			nOffset += 4;

			s1.Format("%s  FileLen = %d\r\n",pName,nFileLen);
			ss += s1;
		}
		delete pRecv;
	}
	m_Recv.SetWindowText(ss);	
	if(sCommand == "REQUEST" && !sIp.IsEmpty() && nRequestFileLen > 0)
	{
		GetFile(sIp,sName,nRequestFileLen);
	}
	((CButton*)GetDlgItem(IDC_BUTTON1))->EnableWindow();
}
int CClientDlg::ParseCMD(SOCKET s,CString sSend,CString &sCMD,CString &sName)
{
	int nRet = 0;
	int i;
	CString sHash,sLen;
	int nOffset = 0;	
//  "ADD [FileName] [SHA HASH] [FileLen]\n";
// 	"DELETE [FileName] [SHA HASH]\n";
// 	"LIST\n";
// 	"QUIT\n";
// 	"REQUEST [FileName]\n";
	i = sSend.Find(' ');
	if(i==-1)
		sCMD = sSend;
	else
		sCMD = sSend.Left(i);
	sSend = sSend.Right(sSend.GetLength() - i - 1);
	sName = "";
	sCMD.MakeLower();
	if( sCMD != "add" && sCMD != "delete" && sCMD != "list" && sCMD != "quit" && sCMD != "request" )
	{
		MessageBox("输入的命令不存在");
		return 0;
	}
	sCMD.MakeUpper();
	memcpy(pSendBuff,sCMD.GetBuffer(sCMD.GetLength()),sCMD.GetLength());
	nOffset += sCMD.GetLength();
	sCMD.MakeLower();

	if( sCMD == "list" || sCMD == "quit" )
	{
		send(s,pSendBuff,nOffset,0);
		return 0;
	}
	if(sSend.IsEmpty())
	{
		MessageBox("输入的命令格式不对");
		return 0;
	}

	if(sCMD == "request")
		sName = sSend;
	else
	{
		i = sSend.Find(" ");
		if(i==-1)
		{
			MessageBox("输入的命令格式不对");
			return 0;
		}
		sName = sSend.Left(i);
		sSend = sSend.Right(sSend.GetLength() - i - 1);
	}

	pSendBuff[nOffset] = (BYTE)sName.GetLength();
	nOffset++;
	memcpy(pSendBuff+nOffset,sName.GetBuffer(sName.GetLength()),sName.GetLength());
	nOffset += sName.GetLength();
	if(sCMD == "request")
	{
		send(s,pSendBuff,nOffset,0);
		return 0;
	}

	if(sSend.IsEmpty())
	{
		MessageBox("输入的命令格式不对");
		return 0;
	}
	if(sCMD == "delete")
		sHash = sSend;
	else
	{
		i = sSend.Find(" ");
		if(i==-1)
		{
			MessageBox("输入的命令格式不对");
			return 0;
		}
		sHash = sSend.Left(i);
		sSend = sSend.Right(sSend.GetLength() - i - 1);
	}
	
	memcpy(pSendBuff+nOffset,sHash.GetBuffer(sHash.GetLength()),40);
	nOffset += sHash.GetLength();
	if(sCMD == "delete")
	{
		send(s,pSendBuff,nOffset,0);
		return 0;
	}
	if(sSend.IsEmpty())
	{
		MessageBox("输入的命令格式不对");
		return 0;
	}

	sLen = sSend;
	int nLen;
	nLen = atoi(sLen);
	memcpy(pSendBuff+nOffset,&nLen,4);
	nOffset += 4;

	send(s,pSendBuff,nOffset,0);

	return nRet;
}
void CClientDlg::GetFile(CString sIp,CString pName,int nFileLen)
{
	WSADATA wsaData;
	SOCKET ss;
	BYTE nLen;
	char *pBuff;
	CString sFile;
	FILE *fp;
	int nHasRecv;
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRet != 0)
	{
		MessageBox("初始化失败");
		return;
	}
	
	// 创建TCP套接字
	ss = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ss == INVALID_SOCKET)
	{
		MessageBox("创建套接字失败");
		return ;
	}
	m_addrTCP_Host.sin_family = AF_INET;
	m_addrTCP_Host.sin_port = htons(6000);
	m_addrTCP_Host.sin_addr.s_addr = inet_addr(sIp);
	nAddrLen = sizeof(SOCKADDR_IN);
	nRet = connect(ss,(sockaddr *)&m_addrTCP_Host,nAddrLen);
	if(nRet == INVALID_SOCKET)
	{
		MessageBox("链接失败");
		return;
	}
	
	send(ss,"CONNECT!",8,0);
	nRet = recv(ss,pRecvBuff,BUFFLEN,0);
	if( nRet == 6 && !memcmp(pRecvBuff,"ACCEPT",6) )
	{
		
	}
	else
	{
		MessageBox("验证身份失败");
		closesocket(ss);
	}

	//|GETFILE|LEN 1|NAME|filelen 4|
	memcpy(pSendBuff,"GETFILE",7);
	nLen = (BYTE)pName.GetLength();
	pSendBuff[7] = nLen;
	memcpy(pSendBuff+8,pName.GetBuffer(nLen),nLen);
	memcpy(pSendBuff+8+nLen,&nFileLen,4);
	send(ss,pSendBuff,12+nLen,0);

	pBuff = new char[nFileLen];
	nHasRecv = 0;
	while(nHasRecv < nFileLen)
	{
		nRet = recv(ss,pBuff+nHasRecv,nFileLen-nHasRecv,0);
		if(nRet <= 0)
		{
			closesocket(ss);
			m_Recv.GetWindowText(sFile);
			sFile += "\r\n操作失败";
			m_Recv.SetWindowText(sFile);
			return;
		}
		nHasRecv += nRet;
	}

	sFile = sDownload + pName;
	fp = fopen(sFile,"wb");
	if(fp)
	{
		fwrite(pBuff,1,nFileLen,fp);
		fclose(fp);
		m_Recv.GetWindowText(sFile);
		sFile += "\r\n文件下载完毕";
		m_Recv.SetWindowText(sFile);
	}
	closesocket(ss);
	
	m_Recv.GetWindowText(sFile);
	sFile += "\r\n操作完毕";
	m_Recv.SetWindowText(sFile);
}
void CClientDlg::OnButton3() 
{
	if(!bTCPInit)
	{
		MessageBox("没链接，关啥子呦");
		return;
	}
	bTCPInit = 0;
	closesocket(m_Socket_TCP);
	MessageBox("链接已关闭");
}
///udp
void CClientDlg::OnButton2() 
{
	UpdateData();
	if( m_HostIp.IsEmpty() || m_TCPPort == 0 )
	{
		MessageBox("请输入参数");
		return;
	}
	int nRet;
	((CButton*)GetDlgItem(IDC_BUTTON2))->EnableWindow(FALSE);
	if(!bUDPInit)
	{
		WSADATA wsaData;
		nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (nRet != 0)
		{
			MessageBox("初始化失败");
			return;
		}
		
		// 创建UDP套接字
		m_Socket_UDP = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_Socket_UDP == INVALID_SOCKET)
		{
			MessageBox("创建套接字失败");
			return ;
		}
		m_addrUDP_Host.sin_family = AF_INET;
		m_addrUDP_Host.sin_port = htons(m_UDPPort);
		m_addrUDP_Host.sin_addr.s_addr = inet_addr(m_HostIp);


		bUDPInit = 1;
		((CButton*)GetDlgItem(IDC_BUTTON2))->EnableWindow();
		MessageBox("UDP已建立，可以发送消息了");
		return;		
	}
	if(!m_Send.IsEmpty())
	{
		nAddrLen = sizeof(SOCKADDR_IN);
		sendto(m_Socket_UDP, m_Send, m_Send.GetLength(), 0, (sockaddr*)&m_addrUDP_Host, nAddrLen);
		memset(pRecvBuff,0,BUFFLEN);
		nRet = recvfrom(m_Socket_UDP, pRecvBuff, BUFFLEN, 0, (sockaddr*)&m_addrUDP_Client, &nAddrLen);
		if( nRet <= 0 )
		{
			closesocket(m_Socket_UDP);
			MessageBox("关闭UDP");
			((CButton*)GetDlgItem(IDC_BUTTON2))->EnableWindow();
			bUDPInit = 0;
			return;
		}
		m_Send = "";
		UpdateData(FALSE);
		m_Recv.SetWindowText(pRecvBuff);
	}

	((CButton*)GetDlgItem(IDC_BUTTON2))->EnableWindow();
}

void CClientDlg::OnButton4() 
{
	if(!bUDPInit)
		return;
	bUDPInit = 0;
	closesocket(m_Socket_UDP);
	MessageBox("UDP已关闭");
}

void CClientDlg::OnBtnConnect() 
{
	UpdateData();
	if( m_HostIp.IsEmpty() )//|| m_TCPPort == 0 )
	{
		MessageBox("请输入命令");
		return;
	}
	WSADATA wsaData;
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRet != 0)
	{
		MessageBox("初始化失败");
		return;
	}
	
	// 创建TCP套接字
	m_Socket_TCP = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_Socket_TCP == INVALID_SOCKET)
	{
		MessageBox("创建套接字失败");
		return ;
	}
	m_addrTCP_Host.sin_family = AF_INET;
	m_addrTCP_Host.sin_port = htons(5000);
	m_addrTCP_Host.sin_addr.s_addr = inet_addr(m_HostIp);
	nAddrLen = sizeof(SOCKADDR_IN);
	nRet = connect(m_Socket_TCP,(sockaddr *)&m_addrTCP_Host,nAddrLen);
	if(nRet == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		MessageBox("链接失败");
		return;
	}

	send(m_Socket_TCP,"CONNECT!",8,0);
	nRet = recv(m_Socket_TCP,pRecvBuff,BUFFLEN,0);
	if( nRet == 6 && !memcmp(pRecvBuff,"ACCEPT",6) )
	{

	}
	else
	{
		MessageBox("验证身份失败");
		closesocket(m_Socket_TCP);
	}

	int nOffset = 0;
	int i;
	BYTE nLen;
	char pRecv[100];
	for(i=0;i<nShareFileNum;i++)
	{
		memcpy(pSendBuff,"ADD",3);
		nOffset = 3;
		nLen = (BYTE)strlen(filelist[i].pName);
		pSendBuff[nOffset] = nLen;
		nOffset ++;
		memcpy(pSendBuff+nOffset,filelist[i].pName,nLen);
		nOffset += nLen;
		memcpy(pSendBuff+nOffset,filelist[i].pHash,40);
		nOffset += 40;
		memcpy(pSendBuff+nOffset,&filelist[i].nFileLen,4);
		nOffset += 4;
		send(m_Socket_TCP,pSendBuff,nOffset,0);
		nRet = recv(m_Socket_TCP,pRecv,100,0);
	}
	
	bTCPInit = 1;
	((CButton*)GetDlgItem(IDC_BTN_CONNECT))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(IDC_BUTTON1))->EnableWindow();
	MessageBox("服务器已链接");
}
DWORD GetHash(BYTE *pbData, DWORD dwDataLen, ALG_ID algId, LPTSTR pszHash)
{
	
	DWORD dwReturn = 0;
	HCRYPTPROV hProv;
	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		return (dwReturn = GetLastError());
	
	HCRYPTHASH hHash;
	//Alg Id:CALG_MD5,CALG_SHA
	if(!CryptCreateHash(hProv, algId, 0, 0, &hHash))
	{
		dwReturn = GetLastError();
		CryptReleaseContext(hProv, 0);
		return dwReturn;
	}
	
	if(!CryptHashData(hHash, pbData, dwDataLen, 0))
	{
		dwReturn = GetLastError();
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
		return dwReturn;
	}
	
	DWORD dwSize;
	DWORD dwLen = sizeof(dwSize);
	CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)(&dwSize), &dwLen, 0);
	
	BYTE* pHash = new BYTE[dwSize];
	dwLen = dwSize;
	CryptGetHashParam(hHash, HP_HASHVAL, pHash, &dwLen, 0);
	
	lstrcpy(pszHash, _T(""));
	TCHAR szTemp[3];
	for (DWORD i = 0; i < dwLen; ++i)
	{
		//wsprintf(szTemp, _T("%X%X"), pHash[i] >> 4, pHash[i] & 0xf);
		wsprintf(szTemp, "%02X", pHash[i]);
		lstrcat(pszHash, szTemp);
	}
	delete [] pHash;
	
	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
	return dwReturn;
	
	
}
UINT LISTENTHREAD(LPVOID)
{
	CString sFile;
    WSADATA wsaData;
    int nRet,i;
	SOCKET ss,sClient;
	SOCKADDR_IN m_addr_Host,m_addr_client;
	BYTE nLen;
	char pName[256];
	BYTE *pBuff;
	int nFileLen;
	FILE *fp;
    nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (nRet != 0)
    {
        return -1;
    }
	
	// 创建TCP套接字
    ss = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ss == INVALID_SOCKET)
    {
        return -1;
    }
    m_addr_Host.sin_family = AF_INET;
    m_addr_Host.sin_port = htons(6000);
    m_addr_Host.sin_addr.s_addr = INADDR_ANY;
    nRet = bind( ss, (sockaddr*)&m_addr_Host, sizeof(sockaddr_in) );
    if( nRet == SOCKET_ERROR )
    {
        MessageBox(NULL,"绑定6000端口失败","",MB_OK);
        closesocket(ss);
        WSACleanup();
        return -1;
    }

	nRet = listen(ss, 100);
    if(nRet == SOCKET_ERROR)
    {
        MessageBox(NULL,"监听6000端口失败","",MB_OK);
        closesocket(ss);
        WSACleanup();
        return -1;
    }
	while(1)
	{
		int clientSocketLen = sizeof(SOCKADDR_IN);
		sClient = accept(ss, (sockaddr*)&m_addr_client, &clientSocketLen);
		if(sClient == WSAEWOULDBLOCK)
		{
			printf("Non-blocking, accept cannot use.\nPlease change socket mode to block.\n");
			continue;
		}
		else if(sClient == INVALID_SOCKET)
		{
			printf("Accept failed: %d\n", WSAGetLastError());
			continue;
		}
		
		///////////////////验证客户端身份
		memset(pRecvBuff,0,BUFFLEN);
		nRet = recv(sClient,pRecvBuff,BUFFLEN,0);
		if(nRet <= 0)
		{
			closesocket(sClient);
			continue;
		}
		if( memcmp(pRecvBuff,"CONNECT!",8) )
		{
			closesocket(sClient);
			continue;
		}
		send(sClient,"ACCEPT",6,0);

		while(1)
		{
			nRet = recv(sClient,pRecvBuff,BUFFLEN,0);
			if(nRet <= 0)
			{
				closesocket(sClient);
				break;
			}
			if(!memcmp(pRecvBuff,"GETFILE",7))//|GETFILE|LEN 1|NAME|filelen 4|
			{
				memset(pName,0,256);
				nLen = pRecvBuff[7];
				memset(pName,0,nLen);
				memcpy(pName,pRecvBuff+8,nLen);
				memcpy(&nFileLen,pRecvBuff+8+nLen,4);
				for(i=0;i<nShareFileNum;i++)
				{
					if(nFileLen == filelist[i].nFileLen && strlen(pName) == strlen(filelist[i].pName) && !memcmp(filelist[i].pName,_strlwr(pName),strlen(pName)))
						break;
				}
				if( i == nShareFileNum )
				{
					closesocket(sClient);
					break;
				}
				sFile = sShareRoot + pName;
				fp = fopen(sFile,"rb");
				if(!fp)
				{
					closesocket(sClient);
					break;
				}
				pBuff = new BYTE[nFileLen];
				fread(pBuff,1,nFileLen,fp);
				fclose(fp);

				send(sClient,(char*)pBuff,nFileLen,0);
				closesocket(sClient);
				delete pBuff;
				break;
			}
		}

	}

	return 1;
}

void CClientDlg::OnBtnHelp() 
{
	CString ss = "";
	ss += "命令帮助\n";
	ss += "ADD [FileName] [SHA HASH] [FileLen]\n";
	ss += "DELETE [FileName] [SHA HASH]\n";
	ss += "LIST\n";
	ss += "QUIT\n";
	ss += "REQUEST [FileName]\n";
	ss += "\n";

	MessageBox(ss);
}
