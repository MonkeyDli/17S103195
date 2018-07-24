// MultiProtoclServer.cpp: implementation of the CMultiProtoclServer class.
//
//////////////////////////////////////////////////////////////////////

#include "MultiProtoclServer.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMultiProtoclServer::CMultiProtoclServer()
{

}

CMultiProtoclServer::~CMultiProtoclServer()
{

}

BOOL CMultiProtoclServer::InitServer()
{
    WSADATA wsaData;
    int err;
    err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0)
    {
        printf("WSAStartup failed with error: %d\n", err);
        return -1;
    }

	// 创建TCP套接字
    m_Socket_Tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_Socket_Tcp == INVALID_SOCKET)
    {
        err = WSAGetLastError();
        printf("\"socket\" error! error code is %d\n", err);
        return -1;
    }
    m_addr_Host.sin_family = AF_INET;
    m_addr_Host.sin_port = htons(TCP_PORT);
    m_addr_Host.sin_addr.s_addr = INADDR_ANY;
    printf("TCP listen port: %s:%d\n", inet_ntoa(m_addr_Host.sin_addr), TCP_PORT);
    err = bind( m_Socket_Tcp, (sockaddr*)&m_addr_Host, sizeof(sockaddr_in) );
    if( err == SOCKET_ERROR )
    {
        printf("TCP bind failed: %d\n", WSAGetLastError());
        closesocket(m_Socket_Tcp);
        WSACleanup();
        return -1;
    }
	
	// 创建UDP套接字
    m_Socket_UDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_Socket_UDP == INVALID_SOCKET)
    {
        err = WSAGetLastError();
        printf("\"socket\" error! error code is %d\n", err);
        return -1;
    }
    m_addr_Host.sin_family = AF_INET;
    m_addr_Host.sin_port = htons(UDP_PORT);
    m_addr_Host.sin_addr.s_addr = INADDR_ANY;
    printf("UDP listen port: %s:%d\n", inet_ntoa(m_addr_Host.sin_addr), UDP_PORT);
    err = bind( m_Socket_UDP, (sockaddr*)&m_addr_Host, sizeof(sockaddr_in) );
    if( err == SOCKET_ERROR )
    {
        printf("UDP bind failed: %d\n", WSAGetLastError());
        closesocket(m_Socket_UDP);
        WSACleanup();
        return -1;
    }
	
// 设定为非阻塞模式
    DWORD nMode = 1;
    err = ioctlsocket(m_Socket_Tcp, FIONBIO, &nMode);
    if(err == SOCKET_ERROR)
    {
        printf("TCP ioctlsocket failed: %d\n", WSAGetLastError());
        closesocket(m_Socket_Tcp);
        WSACleanup();
        return -1;
    }

    nMode= 1;
    err = ioctlsocket(m_Socket_UDP, FIONBIO, &nMode);
    if (err == SOCKET_ERROR)
    {
        printf("UDP ioctlsocket failed: %d\n", WSAGetLastError());
        closesocket(m_Socket_UDP);
        WSACleanup();
        return -1;
    }

    err = listen(m_Socket_Tcp, TCP_CLIENT_NUM);
    if(err == SOCKET_ERROR)
    {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(m_Socket_Tcp);
        WSACleanup();
        return -1;
    }

	////////
    m_tv.tv_sec = 60;
    m_tv.tv_usec = 0;
    FD_ZERO(&m_Read);
    m_tcpClient_CurrentNum = 0;
    for (int i = 0; i < TCP_CLIENT_NUM; i++)
    {
        memset(&m_tcpClientList[i],0,sizeof(SOCKETINFO));
    }
	head = new PCLIST;
	memset(head,0,sizeof(PCLIST));
	tail = head;
	////////
    printf("Init OK!\nReady to accept connection......\n");
    return 0;

}
void CMultiProtoclServer::SetFDRead()
{
	int i;
	FD_ZERO(&m_Read);
	FD_SET(m_Socket_Tcp, &m_Read);
	FD_SET(m_Socket_UDP, &m_Read);
	
	for(i = 0; i < TCP_CLIENT_NUM; ++i)
	{
		if(m_tcpClientList[i].bUse != 0)
			FD_SET(m_tcpClientList[i].s, &m_Read);
	}
}
void CMultiProtoclServer::CheckCommand(SOCKET s,char *p,int nPacketLen)
{
	int nOffset;
	PCLIST *ptr_pc;
	FILELIST *ptr_file,*pNew_file;
	BOOL bFind = 0;

	////////查找PC
	ptr_pc = head;
	while(ptr_pc->next)
	{
		ptr_pc = ptr_pc->next;

		if(ptr_pc->s == s)
		{
			bFind = 1;
			break;
		}
	}
	if(!bFind)
		return;

	if( !memcmp(p,"ADD",3) )// |ADD 3byte | filenamelen 1byte| filename| hash 40byte| filelen 4byte|
	{
		printf("CMD>ADD ");
		pNew_file = new FILELIST;
		memset(pNew_file,0,sizeof(FILELIST));

		nOffset = 3;
		BYTE nLen = (BYTE)p[nOffset];
		nOffset++;
		memcpy(pNew_file->pName,p+nOffset,nLen);
		nOffset += nLen;
		memcpy(&pNew_file->pHash,p+nOffset,40);
		nOffset += 40;
		memcpy(&pNew_file->nFileLen,p+nOffset,4);

		printf("%s,Length = %d,SHA HASH = %s\n",pNew_file->pName,pNew_file->nFileLen,pNew_file->pHash);

		_strlwr(pNew_file->pName);
		////////////////查重
		FILELIST *pTmp;
		bFind = 0;
		ptr_file = ptr_pc->file_head;
		while(ptr_file->next)
		{
			bFind = 0;
			pTmp = ptr_file->next;
			if(strlen(pNew_file->pName) == strlen(pTmp->pName) && !memcmp(pNew_file->pName,pTmp->pName,strlen(pNew_file->pName)) && !memcmp(pNew_file->pHash,pTmp->pHash,40) )
			{
				bFind = 1;
				break;
			}
			ptr_file = ptr_file->next;
		}
		if(!bFind)
		{
			ptr_pc->file_tail->next = pNew_file;
			ptr_pc->file_tail = pNew_file;
		}
		else
			delete pNew_file;
		/////////////////////

		send(s,"OK",2,0);
		printf("OK\n");
	}
	else if( !memcmp(p,"DELETE",6) )// |DELETE 6byte | filenamelen 1byte| filename| hash 40byte|
	{
		printf("CMD>DELETE ");
		char pName[256] = {0};
		BYTE nLen;
		char pHash[41] = {0};
		nOffset = 6;
		nLen = (BYTE)p[nOffset];
		nOffset++;
		memcpy(pName,p+nOffset,nLen);
		nOffset += nLen;
		memcpy(pHash,p+nOffset,40);
		printf("%d  SHA HASH:%s\n",pName,pHash);
		
		_strlwr(pName);

		FILELIST *pTmp;
		ptr_file = ptr_pc->file_head;
		while(ptr_file->next)
		{
			bFind = 0;
			pTmp = ptr_file->next;
			if(strlen(pName) == strlen(pTmp->pName) && !memcmp(pName,pTmp->pName,strlen(pName)) && !memcmp(pHash,pTmp->pHash,40) )
			{
				bFind = 1;
				break;
			}
			ptr_file = ptr_file->next;
		}
		if(bFind)
		{
			ptr_file->next = pTmp->next;
			delete pTmp;
			send(s,"OK",2,0);
			printf("OK\n");
		}
		else
		{
			send(s,"ERR NO FILE",11,0);
			printf("没有找到文件记录\n");
		}

	}
	else if( !memcmp(p,"LIST",4) )
	{
		printf("CMD>LIST\n");
		int nNum = 0;
		BYTE nLen;
		int nBuffLen = 0;

		nOffset = 4;
		PCLIST *pTmp;
		pTmp = head;
		while(pTmp->next)
		{
			pTmp = pTmp->next;
			
// 			if( pTmp == ptr_pc )
// 				continue;
			ptr_file = pTmp->file_head;
			while(ptr_file->next)
			{
				ptr_file = ptr_file->next;
				nNum++;

				nLen = (BYTE)strlen(ptr_file->pName);
				nBuffLen = nBuffLen + nLen + 1 + 4;
			}
		}
		nBuffLen += 4;

		char *ppp = new char[nBuffLen];
		memcpy(ppp,&nNum,4);
		nOffset = 4;
		//number 4byte|namelen 1byte|name|filelen 4byte|
		//            |...|
		pTmp = head;
		while(pTmp->next)
		{
			pTmp = pTmp->next;
			
// 			if( pTmp == ptr_pc )
// 				continue;
			ptr_file = pTmp->file_head;
			while(ptr_file->next)
			{
				ptr_file = ptr_file->next;
				
				nLen = (BYTE)strlen(ptr_file->pName);
				ppp[nOffset] = nLen;
				nOffset++;
				memcpy(ppp+nOffset,ptr_file->pName,nLen);
				nOffset += nLen;
				memcpy(ppp+nOffset,&ptr_file->nFileLen,4);
				nOffset += 4;
			}
		}

		/////发送长度
		memcpy(pSendBuff,&nBuffLen,4);
		send(s,pSendBuff,4,0);
		memset(pRecvBuff,0,BUFFLEN);
		int i = 0;
		while(1)
		{
			Sleep(500);
			nRet = recv(s,pRecvBuff,BUFFLEN,0);
			if(nRet > 0)
				break;
			i++;
			if(i == 20)
			{
				printf("Link Error! Close\n");
				for(int i = 0; i < TCP_CLIENT_NUM; ++i)
				{
					if( m_tcpClientList[i].s == s )
					{
						FD_CLR(m_tcpClientList[i].s, &m_Read);
						m_tcpClientList[i].bUse = 0;
						m_tcpClient_CurrentNum--;
					}
				}
				closesocket(s);
				ptr_pc->s = 0;
			}
		}
		if(nRet == 2 && pRecvBuff[0] == 'O' && pRecvBuff[1] == 'K')
		{
			send(s,ppp,nBuffLen,0);
			printf("List OK\n");
		}
		else
		{
			send(s,"ERR VERIFY ERROR",16,0);
			printf("List ERR:验证失败\n");
		}
	}
	else if( !memcmp(p,"QUIT",4) )
	{
		printf("CMD>QUIT\n");

		for(int i = 0; i < TCP_CLIENT_NUM; ++i)
		{
			if( m_tcpClientList[i].s == s )
			{
				FD_CLR(m_tcpClientList[i].s, &m_Read);
				m_tcpClientList[i].bUse = 0;
				m_tcpClient_CurrentNum--;
			}
		}
		closesocket(s);
		ptr_pc->s = 0;
		printf("OK\n");
	}
	else if( !memcmp(p,"REQUEST",7) )// |REQUEST 7byte | filenamelen 1byte| filename|
	{
		printf("CMD>REQUEST ");
		char pName[256] = {0};
		BYTE nLen;
		nOffset = 7;
		nLen = (BYTE)p[nOffset];
		nOffset++;
		memcpy(pName,p+nOffset,nLen);		
		_strlwr(pName);
		printf("%s\n",pName);
		
		PCLIST *pTmp;
		pTmp = head;
		bFind = 0;
		while(pTmp->next)
		{
			pTmp = pTmp->next;

// 			if( pTmp == ptr_pc )
// 				continue;
			ptr_file = pTmp->file_head;
			while(ptr_file->next)
			{
				ptr_file = ptr_file->next;
				if(strlen(pName) == strlen(ptr_file->pName) && !memcmp(pName,ptr_file->pName,strlen(pName)))
				{
					bFind = 1;
					break;
				}
			}
			if(bFind)
				break;
		}
		if(bFind)
		{
			memset(pSendBuff,0,BUFFLEN);
			memcpy(pSendBuff,pTmp->pIp,4);
			memcpy(pSendBuff+4,&ptr_file->nFileLen,4);
			send(s,pSendBuff,8,0);
			printf("OK\n");
		}
		else
		{
			send(s,"ERR NO FILE",11,0);
			printf("没有找到文件资源\n");
		}
	}
	else
	{
		send(s,"NO THIS COMMAND",15,0);
		printf("未知命令\n");
	}
}
void CMultiProtoclServer::CheckClientList()
{
    int nRet;
    printf("Checking Socket........\n");
    for(int i = 0; i < TCP_CLIENT_NUM; ++i)
    {
		//将已经关闭的SOCKET从FD集中删除
        if(FD_ISSET(m_tcpClientList[i].s, &m_Read))
        {
 			memset(pRecvBuff,0,BUFFLEN);
            nRet = recv(m_tcpClientList[i].s, pRecvBuff, BUFFLEN, 0);
            if(nRet <= 0)
            {
                printf("Client Has Closed.\n");
                closesocket(m_tcpClientList[i].s );
                FD_CLR(m_tcpClientList[i].s, &m_Read);
                m_tcpClientList[i].bUse = 0;
                m_tcpClient_CurrentNum--;
            }
            else
            {
				memset(pSendBuff,0,BUFFLEN);
				printf("Message From TCP Client %s : %s\n", inet_ntoa(m_addr_TcpClient.sin_addr),pRecvBuff);
				CheckCommand(m_tcpClientList[i].s,pRecvBuff,nRet);
                
// 				int BuffLength = send( m_tcpClientList[i].s, pSendBuff, strlen(pSendBuff), 0 );
//                 if (SOCKET_ERROR == BuffLength)
//                 {
//                     nRet = WSAGetLastError();
//                     printf("Send error! Error code is %d\n", nRet);
//                     system("pause");
//                 }
            }
        }
    }
}
void CMultiProtoclServer::StartWork()
{
	int i;
	while(1)
	{
        printf("Listen......\n");
		SetFDRead();

		// 调用select模式进行监听
        nRet = select(0, &m_Read, NULL, NULL, &m_tv);
        if(nRet == 0)
        {
            printf("timeout\n");
            continue;
        }
        else if(nRet < 0)
        {
            printf("Select failed: %d\n", WSAGetLastError());
            break;
        }
        /* 检查所有的可用SOCKET*/
        CheckClientList();
		//检查UDP socket连接
        if(FD_ISSET(m_Socket_UDP, &m_Read))
        {
			memset(pRecvBuff,0,BUFFLEN);
			memset(pSendBuff,0,BUFFLEN);
            int clientSocketLen = sizeof(m_addr_UdpClient);
            if (recvfrom(m_Socket_UDP, pRecvBuff, BUFFLEN, 0, (sockaddr*)&m_addr_UdpClient, &clientSocketLen) != SOCKET_ERROR)
            {
                printf("Message From UDP Client %s : %s\n",inet_ntoa(m_addr_UdpClient.sin_addr), pRecvBuff);
                sprintf(pSendBuff, "I am %d.%d.%d.%d, I know you are %d.%d.%d.%d", 
					m_addr_Host.sin_addr.S_un.S_un_b.s_b1,m_addr_Host.sin_addr.S_un.S_un_b.s_b2,
					m_addr_Host.sin_addr.S_un.S_un_b.s_b3,m_addr_Host.sin_addr.S_un.S_un_b.s_b4,
					m_addr_UdpClient.sin_addr.S_un.S_un_b.s_b1,m_addr_UdpClient.sin_addr.S_un.S_un_b.s_b2,
					m_addr_UdpClient.sin_addr.S_un.S_un_b.s_b3,m_addr_UdpClient.sin_addr.S_un.S_un_b.s_b4);
				
                sendto(m_Socket_UDP, pSendBuff, strlen(pSendBuff), 0, (sockaddr*)&m_addr_UdpClient, clientSocketLen);
            }
        }
		//检查TCP socket连接,并accept处理
        if(FD_ISSET(m_Socket_Tcp, &m_Read))
        {
            int clientSocketLen = sizeof(m_addr_TcpClient);
            m_Socket_Client = accept(m_Socket_Tcp, (sockaddr*)&m_addr_TcpClient, &clientSocketLen);
            if(m_Socket_Client == WSAEWOULDBLOCK)
            {
                printf("Non-blocking, accept cannot use.\nPlease change socket mode to block.\n");
                continue;
            }
            else if(m_Socket_Client == INVALID_SOCKET)
            {
                printf("Accept failed: %d\n", WSAGetLastError());
                continue;
            }

			///////////////////验证客户端身份
			memset(pRecvBuff,0,BUFFLEN);
			nRet = recv(m_Socket_Client,pRecvBuff,BUFFLEN,0);
			if(nRet <= 0)
			{
                printf("Link Error,Accept failed: %d\n", WSAGetLastError());
				closesocket(m_Socket_Client);
                continue;
			}
			if( memcmp(pRecvBuff,"CONNECT!",8) )
			{
                printf("Verification Error!\n");
				closesocket(m_Socket_Client);
                continue;
			}
			send(m_Socket_Client,"ACCEPT",6,0);

			///////////////////////////////////////检查服务器资源链表
			PCLIST *ptr = head;
			BOOL bFind = 0;
			while(ptr->next)
			{
				ptr = ptr->next;

				if( !memcmp(ptr->pIp,&m_addr_TcpClient.sin_addr.S_un.S_un_b,4) )
				{
					ptr->s = m_Socket_Client;
					bFind = 1;
					break;
				}
			}
			
			if( !bFind )
			{
				PCLIST *pNew = new PCLIST;
				memset(pNew,0,sizeof(PCLIST));
				memcpy(pNew->pIp,&m_addr_TcpClient.sin_addr.S_un.S_un_b,4);
				pNew->file_head = new FILELIST;
				memset(pNew->file_head,0,sizeof(FILELIST));
				pNew->file_tail = pNew->file_head;
				pNew->s = m_Socket_Client;
				tail->next = pNew;
				tail = pNew;
			}
			/////////////////////////////////////

			//新的连接可以使用,查看待决处理队列
            if(m_tcpClient_CurrentNum < TCP_CLIENT_NUM)
            {
                for(i = 0; i < TCP_CLIENT_NUM; ++i)
                {
                    if(m_tcpClientList[i].bUse == 0)
                    {
						m_tcpClientList[i].bUse = 1;
                        m_tcpClientList[i].s = m_Socket_Client;
						memcpy(&m_tcpClientList[i].addr,&m_addr_TcpClient,sizeof(SOCKADDR_IN));
                        break;
                    }
                }
                m_tcpClient_CurrentNum++;
                printf("There is a new TCP connection:[%d] %s:%d\n", m_Socket_Client, inet_ntoa(m_addr_TcpClient.sin_addr), ntohs(m_addr_TcpClient.sin_port));
            }
            else
            {
                memset(pRecvBuff,0,BUFFLEN);
                sprintf(pRecvBuff, "Connection is upto %d, cannot setup new connection: %d\n", TCP_CLIENT_NUM, m_Socket_Client);
				printf("%s",pRecvBuff);
                send(m_Socket_Client, pRecvBuff, strlen(pRecvBuff), 0);
                closesocket(m_Socket_Client);
            }
        }
	}
}


void CMultiProtoclServer::EndWork()
{
    closesocket(m_Socket_Tcp);
    closesocket(m_Socket_UDP);
    WSACleanup();
}
