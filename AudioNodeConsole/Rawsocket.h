///////////////////////////////////////////////////////////////////
// Header File RawSocket.h
// class CWizSyncSocket & CWizReadWriteSocket
//
// 23/07/1996 14:54                             Author: Poul A. Costinsky
///////////////////////////////////////////////////////////////////

#ifndef __CWizRawSocket_H
#define __CWizRawSocket_H

#include <string>
using namespace std;


#include <WinSock2.h>

#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define LPTSTR  char *
#define LPSTR  char *
#define LPIN_ADDR in_addr *
#define WSABASEERR              10000
/*
 * Windows Sockets definitions of regular Microsoft C error constants
 */
#define WSAEINTR                (WSABASEERR+4)
#define WSAEBADF                (WSABASEERR+9)
#define WSAEACCES               (WSABASEERR+13)
#define WSAEFAULT               (WSABASEERR+14)
#define WSAEINVAL               (WSABASEERR+22)
#define WSAEMFILE               (WSABASEERR+24)
#define WSAEWOULDBLOCK          (WSABASEERR+35)
#define WSAEINPROGRESS          (WSABASEERR+36)
#define WSAEALREADY             (WSABASEERR+37)
#define WSAENOTSOCK             (WSABASEERR+38)

#define WSASetLastError


#if _MSC_VER < 1400

#define strcpy_s(a,b,c)		strcpy(a,c)
#define errno_t int

errno_t _ecvt_s(
	char* _Buffer,
	size_t _SizeInBytes,
	double _Value,
	int _Count,
	int* _Dec,
	int* _Sign
);
#endif

string GetLastSocketErrorText();

/////////////////////////////////////////////////////////////////////////////
// class CWizSyncSocket
// Simple encapsulation of the SOCKET handle and 
// simple operations with it.
//##ModelId=43CE16C8022A
class CWizSyncSocket
{
public:
	// Exception class if createsocket fails in constructor 
	//##ModelId=43CE16C80297
	struct XCannotCreate {};
	// Constructors
	//##ModelId=43CE16C8022B
	// NB: Remove ambiguous constructor with only one argument and default SOCK_STREAM. Need to specify SOCK_STREAM when used.
	CWizSyncSocket(int nPort, int iType);// = SOCK_STREAM);	// Connects for listening 
	// on the port nPort
//##ModelId=43CE16C8022E
	CWizSyncSocket(SOCKET h, const char* szDummy); //  = INVALID_SOCKET); // incapsulates 
	// ready socket handle
	CWizSyncSocket(); // SOCKET h = INVALID_SOCKET); // incapsulates 
	// Destructor
	//##ModelId=43CE16C80239
	~CWizSyncSocket();

	//##ModelId=43CE16C8023A
	BOOL	Create(int nPort, int iType = SOCK_STREAM);	// Creates listening socket
	//##ModelId=43CE16C8023D
	void	Close();			// Closes socket
	//##ModelId=43CE16C8023E
	SOCKET	Accept();			// waits to accept connection
	// and returns a descriptor 
	// for the accepted packet
//##ModelId=43CE16C8023F
	BOOL	Connect(LPCTSTR lpszHostAddress, UINT nHostPort, int iType = SOCK_STREAM);
	// establishes a connection to a peer

// 	operator =(SOCKET socket){ m_hSocket = socket; }
	//##ModelId=43CE16C80243
	void SetSocket(SOCKET socket) { m_hSocket = socket; }
	//##ModelId=43CE16C80249
	SOCKET	H() const { return m_hSocket; }
	// returns a handle of the socket
//##ModelId=43CE16C8024B
	BOOL GetHostName(LPTSTR lpszAddress, size_t nAddrSize, UINT& rSocketPort);
	//##ModelId=43CE16C8024F
	BOOL GetPeerName(LPTSTR lpszAddress, size_t nAddrSize, UINT& rPeerPort);

	//##ModelId=43CE16C80253
	void SetPort(int iPort) { m_iPort = iPort; }
	//##ModelId=43CE16C80255
	int  GetPort() { return m_iPort; }

	//##ModelId=43CE16C80256
	const char* GetHostName() { return m_strHostname.c_str(); }

	//##ModelId=43CE16C80259
	const char* GetErrorText();
	//##ModelId=43CE16C8025A
	static const char* GetStaticErrorText();
	//##ModelId=43CE16C8025C
	//##ModelId=43CE16C8025C
	int GetLastError() { return m_iLastError; }

	//##ModelId=43CE16C8025D
	int GetCommStatus() { return m_iCommStatus; }
	//##ModelId=43CE16C8025E
	void SetCommStatus(int iCommStatus) { m_iCommStatus = iCommStatus; }

	//##ModelId=43CE16C80268
	int GetUdpCommStatus() { return m_iUdpCommStatus; }
	//##ModelId=43CE16C80269
	void SetUdpCommStatus(int iUdpCommStatus) { m_iUdpCommStatus = iUdpCommStatus; }

	//##ModelId=43CE16C8026B
	int	ReadUdp(void* pData, int nLen);

	//##ModelId=49AE4BBC023E
	int SetNoDelay(); // Disables NAGLE algorthm

protected:
	//##ModelId=43CE16C8026E
	void Init(SOCKET h);		// initialize data members
	//##ModelId=43CE16C80278
	BOOL InitializeSocket(int nPort, int iType);
	// associates a local address 
	// with a socket and establishes
	// a socket to listen for incoming connection
//##ModelId=43CE16C8027B
	int	 SetIntOption(int level, int optname, int val);
	// sets a socket option
//##ModelId=43CE16C80280
	SOCKET	m_hSocket;			// socket handle

	//##ModelId=43CE16C80284
	int 	m_iLastError;
	//##ModelId=43CE16C80288
	int 	m_iPort;
	//##ModelId=43CE16C8028A
	string	m_strHostname;
	//##ModelId=43CE16C8028E
	int 	m_iCommStatus;
	//##ModelId=43CE16C8028F
	int 	m_iUdpCommStatus;
};

/////////////////////////////////////////////////////////////////////////////
// class CWizReadWriteSocket
// Class provides synchronous I/O for the socket
class CWizReadWriteSocket : public CWizSyncSocket
{
public:
	//##ModelId=43CE16C80299
	struct X {};
	//##ModelId=43CE16C802A7
	struct XRead : public X {};
	//##ModelId=43CE16C802AA
	struct XWrite : public X {};
public:
	// constructor usually receives a handle from Accept
	CWizReadWriteSocket(SOCKET h = INVALID_SOCKET)
		: CWizSyncSocket(h, "Dummy") {}

	// Read/write any data. Both functions return 
	// an actual number of bytes read/written,
	// accept pointer to data and number of bytes.
	int Read(void* pData, int nLen, int iTimeout = 0);
	int	Write(const void* pData, int nLen);
	// Read/write strings. nLen is in characters,
	// not bytes.
	int ReadString(char* lpszString, int nLen);
	// If nLen == -1, the actual length is calculated
	// assuming lpszString is zero-terminated.
	int WriteString(const char* lpszString, int nLen = -1);
#ifndef __unix__
	int ReadString(WCHAR* lpszString, int nLen);
	int WriteString(const WCHAR* lpszString, int nLen = -1);
#endif
};

/////////////////////////////////////////////////////////////////////////////
template<class T>
BOOL	RawRead(CWizReadWriteSocket& rSocket, T& data)
{
	return (rSocket.Read(&data, sizeof(T)) == sizeof(T));
}

template<class T>
BOOL	RawWrite(CWizReadWriteSocket& rSocket, const T& data)
{
	return (rSocket.Write(&data, sizeof(T)) == sizeof(T));
}

/////////////////////////////////////////////////////////////////////////////
template<class T>
void	RawReadX(CWizReadWriteSocket& rSocket, T& data)
{
	if (!RawRead(rSocket, data))
		throw CWizReadWriteSocket::XRead();
}

template<class T>
void	RawWriteX(CWizReadWriteSocket& rSocket, const T& data)
{
	if (!RawWrite(rSocket, data))
		throw CWizReadWriteSocket::XWrite();
}

/////////////////////////////////////////////////////////////////////////////

inline CWizReadWriteSocket& operator>>(CWizReadWriteSocket& rSocket, int& i)
{
	int ni;
	RawReadX(rSocket, ni);
	i = ntohl(ni);
	return rSocket;
}

inline CWizReadWriteSocket& operator<<(CWizReadWriteSocket& rSocket, int i)
{
	int ni = htonl(i);
	RawWriteX(rSocket, ni);
	return rSocket;
}

/////////////////////////////////////////////////////////////////////////////
inline CWizReadWriteSocket& operator>>(CWizReadWriteSocket& rSocket, char& i)
{
	RawReadX(rSocket, i);
	return rSocket;
}

inline CWizReadWriteSocket& operator<<(CWizReadWriteSocket& rSocket, char i)
{
	RawWriteX(rSocket, i);
	return rSocket;
}

/////////////////////////////////////////////////////////////////////////////
inline CWizReadWriteSocket& operator>>(CWizReadWriteSocket& rSocket, short& i)
{
	short ni;
	RawReadX(rSocket, ni);
	i = ntohs(ni);
	return rSocket;
}

inline CWizReadWriteSocket& operator<<(CWizReadWriteSocket& rSocket, short i)
{
	short ni = htons(i);
	RawWriteX(rSocket, ni);
	return rSocket;
}
/////////////////////////////////////////////////////////////////////////////
inline CWizReadWriteSocket& operator>>(CWizReadWriteSocket& rSocket, double& d)
{
	const int MAX_CHAR = 30;
	char buffer[MAX_CHAR];

	if (!rSocket.ReadString(buffer, MAX_CHAR))
		throw CWizReadWriteSocket::XRead();

	d = atof(buffer);
	return rSocket;
}

inline CWizReadWriteSocket& operator<<(CWizReadWriteSocket& rSocket, double d)
{
	int     decimal, sign;
	int     precision = 10;

	char buffer[100];
	errno_t errCode = _ecvt_s(buffer, 100, d, precision, &decimal, &sign);

	if (errCode != 0 || NULL == buffer || 0 == *buffer)
		strcpy_s(buffer, 1, "0");

	if (!rSocket.WriteString(buffer))
		throw CWizReadWriteSocket::XWrite();

	return rSocket;
}

/////////////////////////////////////////////////////////////////////////////
inline CWizReadWriteSocket& operator>>(CWizReadWriteSocket& rSocket, u_long& ul)
{
	u_long l;

	RawReadX(rSocket, l);
	ul = ntohl(l);
	return rSocket;
}

inline CWizReadWriteSocket& operator<<(CWizReadWriteSocket& rSocket, u_long ul)
{
	const u_long l = htonl(ul);
	RawWriteX(rSocket, l);
	return rSocket;
}

/////////////////////////////////////////////////////////////////////////////
#endif // __CWizRawSocket_H


