#include "AudioNodeServer.h"
#include "Rawsocket.h"
#include <afxwin.h>
#include <iostream>

bool b_shutdown = false;

void doListenThread() {
	WSADATA wsaData;
	int iResult;
	char readBuf[10] = {};
	int iRead = 0;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return;
	}
	//Create listening socket
	CWizSyncSocket* serversocket = new CWizSyncSocket(17598, SOCK_STREAM);

	if(WSAGetLastError()!=0)
		std::cerr << "Listen failed: " << GetLastSocketErrorText() << std::endl;

	std::cout << "Listening for incomming connections..." << std::endl;

	//Accept incomming connections
	while (!b_shutdown) {
		SOCKET sock = serversocket->Accept();
		if (sock == INVALID_SOCKET) {
			std::cout << "Failed to accept connection" << std::endl;
			continue;
		}
		auto socket = std::make_shared<CWizReadWriteSocket>();
		socket->SetSocket(sock);
		AudioNodeServer* server = new AudioNodeServer(socket);
		if (WSAGetLastError() != 0)
			std::cerr << "Listen failed: " << GetLastSocketErrorText() << std::endl;
		std::cout << "Incomming connection accepted! Starting coms thread" << std::endl;
		std::thread comThread(AudioNodeServer::InitializeConnection, socket, server);
		comThread.detach();
	}
}

int main() {
	std::cout << "Starting server..." << std::endl;
	doListenThread();
	std::thread listenThread = std::thread{ doListenThread };
	listenThread.join();
	return 0;
}

