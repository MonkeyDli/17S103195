// MultiProtoclServer.h: interface for the CMultiProtoclServer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MULTIPROTOCLSERVER_H__6B18F21E_95D0_452C_AB3D_EFC635064689__INCLUDED_)
#define AFX_MULTIPROTOCLSERVER_H__6B18F21E_95D0_452C_AB3D_EFC635064689__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdio.h"
#include "winsock.h"
#pragma comment (lib , "ws2_32")

#define TCP_PORT 5000
#define UDP_PORT 5001
#define TCP_CLIENT_NUM	100
#define BUFFLEN  60000

struct SOCKETINFO
{
	BOOL bUse;
	SOCKET s;
	SOCKADDR_IN addr;
};


struct FILELIST
{
	char	pName[256];
	char	pHash[41];
	int		nFileLen;

	FILELIST *next;
};

struct PCLIST
{
	BYTE pIp[4];
	SOCKET s;

	FILELIST *file_head;
	FILELIST *file_tail;

	PCLIST *next;
};



class CMultiProtoclServer  
{
public:
	CMultiProtoclServer();
	virtual ~CMultiProtoclServer();



	SOCKET m_Socket_Tcp;
	SOCKET m_Socket_UDP;
	SOCKET m_Socket_Client;
	SOCKADDR_IN  m_addr_Host;
	SOCKADDR_IN  m_addr_TcpClient;
	SOCKADDR_IN  m_addr_UdpClient;


	timeval m_tv;							// 设定超时时间
	SOCKETINFO m_tcpClientList[TCP_CLIENT_NUM];	// 待处理所有的连接在线的TCP客户端列表
	int m_tcpClient_CurrentNum;				// 现在时刻连接在线的TCP客户端个数
	fd_set m_Read;

	int nRet;
    char pRecvBuff[BUFFLEN];
	char pSendBuff[BUFFLEN];

	PCLIST *head;
	PCLIST *tail;


	BOOL InitServer();
	void StartWork();
	void EndWork();
	void SetFDRead();
	void CheckClientList();
	void CheckCommand(SOCKET s,char *p,int nLen);

};

#endif // !defined(AFX_MULTIPROTOCLSERVER_H__6B18F21E_95D0_452C_AB3D_EFC635064689__INCLUDED_)
