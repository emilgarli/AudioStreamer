/////////////////////////////////////////////////////////////////////
// Class Creator Version 2.0.000 Copyrigth (C) Poul A. Costinsky 1994
///////////////////////////////////////////////////////////////////
// Implementation File RawSocket.cpp
// class CWizRawSocket
//
// 23/07/1996 14:54                       Author: Poul, Hadas & Oren
///////////////////////////////////////////////////////////////////
#define WINVER 0x0501
#ifdef __unix__
#include "SiParamDefinitions.h"
#include "errno.h"

#include "RawSocket.h"

#define errno_t int
#define LPHOSTENT hostent *
#define HOSTENT hostent

#define TCP_NODELAY     0x0001
#define TCP_BSDURGENT   0x7000

#define SO_DONTLINGER   (u_int)(~SO_LINGER)


//#define IPPROTO_IP              0               /* dummy for IP */
//#define IPPROTO_TCP             6               /* tcp */
//#define IPPROTO_UDP             17              /* user datagram protocol */
//#define IPPROTO_RAW             255             /* raw IP packet */

#else
#include <winsock2.h>		// MFC socket extensions
#include <ws2tcpip.h>
//#include <iostream.h>

#include "RawSocket.h"
#define socklen_t int

#ifdef _UNICODE
#include <winnls.h>
#endif
#include <assert.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
//static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#if _MSC_VER < 1400
errno_t _ecvt_s(
	char* _Buffer,
	size_t _SizeInBytes,
	double _Value,
	int _Count,
	int* _Dec,
	int* _Sign
)
{
	_Buffer = _ecvt(_Value, _Count, _Dec, _Sign); return 0;
}
#endif
#endif
string GetLastSocketErrorText()
{
	string strLastError;
#ifndef __unix__
	int iLastError = ::WSAGetLastError();

	void* lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		iLastError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);

	strLastError = (char*)lpMsgBuf;

	// Free the buffer.
	LocalFree(lpMsgBuf);
#else
	strLastError = strerror(errno);
#endif
	return strLastError;

}
#ifdef __unix__
int WSAGetLastError()
{
	return errno;
}

#endif


//##ModelId=43CE16C80259
const char* CWizSyncSocket::GetErrorText()
{
	static string strLastError;
#ifndef __unix__
	if (0 == m_iLastError)
		m_iLastError = ::WSAGetLastError();

	void* lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		m_iLastError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);

	strLastError = (char*)lpMsgBuf;

	// Free the buffer.
	LocalFree(lpMsgBuf);
#else

	strLastError = strerror(errno);

#endif
	return strLastError.c_str();
}

//##ModelId=43CE16C8025A
const char* CWizSyncSocket::GetStaticErrorText()
{
	static string strLastError;
#ifndef __unix__
	int iLastError = ::WSAGetLastError();

	void* lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		iLastError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);

	strLastError = (char*)lpMsgBuf;

	// Free the buffer.
	LocalFree(lpMsgBuf);
#else
	strLastError = strerror(errno);
#endif
	return strLastError.c_str();
}

//*****************************************************************
//##ModelId=43CE16C8026E
void CWizSyncSocket::Init(SOCKET h)
{
	m_iLastError = 0;
	m_iCommStatus = 0;
	m_iUdpCommStatus = 0;
	m_hSocket = h;
	m_iPort = 0;
	m_strHostname = "";
}
//*****************************************************************
//##ModelId=43CE16C8023D
void CWizSyncSocket::Close()
{
	//	ASSERT(m_hSocket != INVALID_SOCKET);
	::closesocket(m_hSocket);
	m_hSocket = INVALID_SOCKET;
}
//*****************************************************************
//##ModelId=43CE16C8027B
int CWizSyncSocket::SetIntOption(int level, int optname, int val)
{
	return ::setsockopt(m_hSocket, level, optname, (char*)&val, sizeof(val));
}


//##ModelId=49AE4BBC023E
int CWizSyncSocket::SetNoDelay()// Disables NAGLE algorthm
{
	return SetIntOption(IPPROTO_TCP, TCP_NODELAY, 1);
}
//*****************************************************************
//##ModelId=43CE16C80278
BOOL CWizSyncSocket::InitializeSocket(int nPort, int iType)
{
	m_iLastError = 0;
	m_iPort = nPort;
	// Socket must be created with socket()

	// Make up address
	SOCKADDR_IN	SockAddr;
	memset(&SockAddr, 0, sizeof(SockAddr));
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = INADDR_ANY;
	SockAddr.sin_port = ::htons(nPort);

	// Bind to the address and port
	int r = ::bind(m_hSocket, (SOCKADDR*)&SockAddr, sizeof(SockAddr));
	if (r == 0)
	{
		if (iType != SOCK_DGRAM)
		{
			SetIntOption(SOL_SOCKET, SO_DONTLINGER, 1);
			SetIntOption(SOL_SOCKET, SO_KEEPALIVE, 1);
			// establishes a socket to listen for incoming connection
			// so Accept can be called
			r = ::listen(m_hSocket, 5);
		}
	}
	return r == 0;
}
//*****************************************************************
//##ModelId=43CE16C8023A
BOOL CWizSyncSocket::Create(int nPort, int iType)
{
	m_iLastError = 0;
	// creates a socket
	m_hSocket = ::socket(PF_INET, iType, 0);
	if (m_hSocket == INVALID_SOCKET)
		return FALSE;
	// Bind to the port
	if (!InitializeSocket(nPort, iType))
	{
		// 		Close();
		return FALSE;
	}
	return TRUE;
}
//*****************************************************************
// Create an invalid socket
//##ModelId=43CE16C8022E
CWizSyncSocket::CWizSyncSocket(SOCKET h, const char* szDummy /* = INVALID_SOCKET */)
{
	Init(h);
}

CWizSyncSocket::CWizSyncSocket() // SOCKET h = INVALID_SOCKET); // incapsulates 
{
	Init(INVALID_SOCKET);
}

//*****************************************************************
// Create a listening socket
//##ModelId=43CE16C8022B
CWizSyncSocket::CWizSyncSocket(int nPort, int iType)
{
	Init(INVALID_SOCKET);
	if (!Create(nPort, iType))
		throw XCannotCreate();
}
//*****************************************************************
//##ModelId=43CE16C80239
CWizSyncSocket::~CWizSyncSocket()
{
	if (m_hSocket != INVALID_SOCKET)
		Close();
}
//*****************************************************************
// Waits for pending connections on the port, 
// creates a new socket with the same properties as this 
// and returns a handle to the new socket
//##ModelId=43CE16C8023E
SOCKET CWizSyncSocket::Accept()
{
	SOCKET h = ::accept(m_hSocket, NULL, NULL);
	return h;
}
//*****************************************************************
// Cerates a socket and establishes a connection to a peer
// on lpszHostAddress:nHostPort 
//##ModelId=43CE16C8023F
BOOL CWizSyncSocket::Connect(LPCTSTR lpszHostAddress, UINT nHostPort, int iType)
{
	m_iLastError = 0;
	m_iPort = nHostPort;

	if (lpszHostAddress != NULL)
		m_strHostname = lpszHostAddress;
	// Create ? socket
	if (m_hSocket == INVALID_SOCKET)
	{
		m_hSocket = ::socket(PF_INET, iType, 0);
		if (m_hSocket == INVALID_SOCKET)
			return FALSE;
	}

	// Fill address machinery of sockets.
	SOCKADDR_IN sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
#ifdef _UNICODE
	char buff[500];
	WideCharToMultiByte(CP_ACP, 0, lpszHostAddress, -1, buff, 500, NULL, NULL);
	LPSTR lpszAscii = buff;
#else
	LPSTR lpszAscii = (LPSTR)lpszHostAddress;
#endif
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = inet_addr(lpszAscii);
	if (sockAddr.sin_addr.s_addr == INADDR_NONE)
	{
		LPHOSTENT lphost;
		lphost = gethostbyname(lpszAscii);
		if (lphost != NULL)
			sockAddr.sin_addr.s_addr = ((LPIN_ADDR)lphost->h_addr)->s_addr;
		else
		{
			WSASetLastError(WSAEINVAL);
			return FALSE;
		}
	}
	sockAddr.sin_port = htons((u_short)nHostPort);
	// connects to peer
	int r = ::connect(m_hSocket, (SOCKADDR*)&sockAddr, sizeof(sockAddr));
	if (r != SOCKET_ERROR)
	{
		return TRUE;
	}

	m_iLastError = ::WSAGetLastError();

	return FALSE;
}

//*****************************************************************
// read raw data
int CWizReadWriteSocket::Read(void* pData, int nLen, int iTimeout)
{
	m_iLastError = 0;

	// Check if data is available
	char* pcData = (char*)pData;
	int r1 = ::recv(m_hSocket, pcData, nLen, iTimeout);
	if (r1 == SOCKET_ERROR)
	{
		m_iLastError = WSAGetLastError();
		m_iCommStatus = m_iLastError;
		return 0;
	}
	m_iCommStatus = 0;

	if (r1 < 0)
	{
		m_iCommStatus = 0;
		return 0;
	}
	return r1;
}

//##ModelId=43CE16C8026B
int	CWizSyncSocket::ReadUdp(void* pData, int nLen)
{
	struct sockaddr fromAddress;
	socklen_t fromlen = sizeof(fromAddress);
	m_iLastError = 0;
	m_iUdpCommStatus = 0;

	// Check if data is available
	char* pcData = (char*)pData;
	int r1 = ::recvfrom(m_hSocket, pcData, nLen, 0, &fromAddress, &fromlen);
	if (r1 == SOCKET_ERROR)
	{
		m_iLastError = WSAGetLastError();
		m_iUdpCommStatus = m_iLastError;

		return 0;
	}
	else if (r1 == 0)
		return 0;
	else if (r1 < 0)
	{
		return 0;
	}
	return r1;
}

// //*****************************************************************
// // read raw data
// int	CWizReadWriteSocket::Read(void *pData, int nLen) 
// {
// 	// Check if data is available
// 	char* pcData = (char* )pData;
// 	int	n = nLen;
// 	
// // 	if (::recv (m_hSocket, pcData, n, MSG_PEEK) < 1)
// // 		return 0;
// 
// 	// if data size is bigger then network buffer
// 	// handle it nice
// 	do
// 		{
// 		int r1 = ::recv (m_hSocket, pcData, n, 0);
// 		if (r1 == SOCKET_ERROR)
// 			{
// 			m_iLastError = WSAGetLastError();
// 			ASSERT(e != WSAEWOULDBLOCK);
// 			return 0;
// 			}
// 		else if (r1 == 0)
// 			return 0; 
// 		else if (r1 < 0)
// 			{
// 			ASSERT(0);
// 			return 0;
// 			}
// 		pcData += r1;
// 		n -= r1;
// 		} while (n > 0);
// 
// 	ASSERT(n == 0);
// 	return nLen;
// }

//*****************************************************************
// write raw data
int	CWizReadWriteSocket::Write(const void* pData, int nLen)
{
	m_iLastError = 0;
	const char* pcData = (const char*)pData;
	int	n = nLen;
	// if data size is bigger then network buffer
	// handle it nice

	do
	{
		int r1 = ::send(m_hSocket, pcData, n, 0);
		if (r1 == SOCKET_ERROR)
		{
			m_iLastError = WSAGetLastError();
			m_iCommStatus = m_iLastError;
			return 0;
		}
		m_iCommStatus = 0;
		if (r1 == 0)
		{
			return 0;
		}
		else if (r1 < 0)
		{

			return 0;
		}
		pcData += r1;
		n -= r1;
	} while (n > 0);

	return nLen;
}

#ifndef __unix__
//*****************************************************************
// Reads UNICODE string from socket.
// Converts string from ANSI to UNICODE.
int CWizReadWriteSocket::ReadString(WCHAR* lpszString, int nMaxLen)
{
	// read string length
	u_long nt_Len;
	if (Read(&nt_Len, sizeof(nt_Len)) < sizeof(nt_Len))
		return 0;
	int Len = ntohl(nt_Len);
	if (Len == 0 || Len >= nMaxLen)
		return 0;

	static const int BUFF_SIZE = 2000;
	if (Len >= BUFF_SIZE)
		return 0;
	char buff[BUFF_SIZE];
	// Read ANSI string to the buffer
	if (Read(buff, Len) < Len)
		return 0;
	buff[Len] = 0;

	// Convert ANSI string to the UNICODE
	::MultiByteToWideChar(CP_ACP, 0, buff, Len, lpszString, nMaxLen * sizeof(lpszString[0]));
	return Len;
}
#endif
//*****************************************************************
// Reads ANSI string from socket.
int CWizReadWriteSocket::ReadString(char* lpszString, int nMaxLen)
{
	// read string length
	u_long nt_Len;
	if (Read(&nt_Len, sizeof(nt_Len)) < sizeof(nt_Len))
		return 0;
	int Len = ntohl(nt_Len);
	if (Len == 0 || Len >= nMaxLen)
		return 0;

	// Read ANSI string
	if (Read(lpszString, Len) < Len)
		return 0;
	lpszString[Len] = 0;
	return Len;
}
//*****************************************************************
inline int Length(const char* p)
{
	return strlen(p);
}
//*****************************************************************
#ifndef __unix__
inline int Length(const WCHAR* p)
{
	return wcslen(p);
}
#endif
#ifndef __unix__
//*****************************************************************
// Writes UNICODE string to socket,
// converts UNICODE string to ANSI.
int CWizReadWriteSocket::WriteString(const WCHAR* lpszString, int nLen /* = -1*/)
{
	if (nLen < 0)
		nLen = Length(lpszString);
	static const int BUFF_SIZE = 2000;
	if (nLen >= BUFF_SIZE * sizeof(lpszString) + sizeof(u_long))
		return FALSE;
	char buff[BUFF_SIZE];
	u_long nt_Len = htonl(nLen);
	int nSize = sizeof(nt_Len);
	memcpy(buff, &nt_Len, nSize);
	// To make one call less, the length of the string
	// copied to the buffer before the string itself
	// and the buffer sent once.
	char* ptr = buff + nSize;
	if (nLen > 0)
	{
		// Convert ANSI to UNICODE
		int s = WideCharToMultiByte(CP_ACP, 0, lpszString, nLen, ptr, BUFF_SIZE - sizeof(u_long), NULL, NULL);
		nSize += s;
	}
	return Write(buff, nSize);
}
#endif
//*****************************************************************
// Writes ANSI string to socket.
int CWizReadWriteSocket::WriteString(const char* lpszString, int nLen /* = -1*/)
{
	if (nLen < 0)
		nLen = strlen(lpszString);
	static const int BUFF_SIZE = 2000;
	if (nLen >= BUFF_SIZE * sizeof(lpszString) + sizeof(u_long))
		return FALSE;
	char buff[BUFF_SIZE];
	u_long nt_Len = htonl(nLen);
	int nSize = sizeof(nt_Len);
	memcpy(buff, &nt_Len, nSize);
	// To make one call less, the length of the string
	// copied to the buffer before the string itself
	// and the buffer sent once.
	char* ptr = buff + nSize;
	if (nLen > 0)
	{
		memcpy(ptr, lpszString, nLen);
		nSize += nLen;
	}
	return Write(buff, nSize);
}
//*****************************************************************
//##ModelId=43CE16C8024B
BOOL CWizSyncSocket::GetHostName(LPTSTR lpszAddress, size_t nAddrSize, UINT& rSocketPort)
{
	m_iLastError = 0;
	if (nAddrSize < 1)
		return FALSE;

	*lpszAddress = '\0';
	SOCKADDR_IN sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	socklen_t nSockAddrLen = sizeof(sockAddr);
	int r;

	while ((r = ::getsockname(m_hSocket, (SOCKADDR*)&sockAddr, &nSockAddrLen)) == SOCKET_ERROR)
	{
		m_iLastError = ::WSAGetLastError();
		if (m_iLastError != WSAEINPROGRESS)
			return FALSE;
	}

	rSocketPort = ::ntohs(sockAddr.sin_port);

	char    szName[64];
	struct  hostent* h;
	DWORD	dwMyAddress;

	int r1;
	while ((r1 = ::gethostname(szName, sizeof(szName))) == SOCKET_ERROR)
	{
		m_iLastError = ::WSAGetLastError();
		if (m_iLastError != WSAEINPROGRESS)
			return FALSE;
	}

	h = (struct hostent*) ::gethostbyname(szName);
	memcpy(&dwMyAddress, h->h_addr_list[0], sizeof(DWORD));
	if (dwMyAddress == INADDR_NONE)
		return FALSE;
	struct   in_addr     tAddr;
	memcpy(&tAddr, &dwMyAddress, sizeof(DWORD));
	char* ptr = ::inet_ntoa(tAddr);

#ifdef _UNICODE
	return ::MultiByteToWideChar(CP_ACP, 0, ptr, -1, lpszAddress, nAddrSize * sizeof(lpszAddress[0]));
#else
	if (size_t(lstrlen(ptr)) >= nAddrSize)
		return FALSE;
	lstrcpy(lpszAddress, ptr);
#endif

	return TRUE;
}
//*****************************************************************
//##ModelId=43CE16C8024F
BOOL CWizSyncSocket::GetPeerName(LPTSTR lpszAddress, size_t nAddrSize, UINT& rPeerPort)
{
	m_iLastError = 0;
	if (nAddrSize < 1)
		return FALSE;
	*lpszAddress = '\0';
	SOCKADDR_IN sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));

	socklen_t  nSockAddrLen = sizeof(sockAddr);
	int r;
	while ((r = ::getpeername(m_hSocket, (SOCKADDR*)&sockAddr, &nSockAddrLen)) == SOCKET_ERROR)
	{
		m_iLastError = ::WSAGetLastError();
		if (m_iLastError != WSAEINPROGRESS)
			return FALSE;
	}

	rPeerPort = ntohs(sockAddr.sin_port);
	char* pAddr = inet_ntoa(sockAddr.sin_addr);
	int len = strlen(pAddr);
#ifdef _UNICODE
	char buff[100];
	if (len >= 100 || len >= int(nAddrSize))
		return FALSE;
	memcpy(buff, pAddr, 100);
	return ::MultiByteToWideChar(CP_ACP, 0, buff, len, lpszAddress, nAddrSize * sizeof(lpszAddress[0]));
#else
	if (size_t(len) >= nAddrSize)
		return FALSE;
	memcpy(lpszAddress, pAddr, len + 1);
#endif
	return TRUE;
}
//*****************************************************************

#ifdef _DEBUG
void Foo()
{
	// 	
	// 	CWizReadWriteSocket s;
	// 	int i;
	// 	short j;
	// 	char k;
	// 	double d;

		// 	s << i << j << k << d;
		// 	s >> i >> j >> k >> d;
}

#endif
