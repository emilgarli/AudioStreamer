#include "AudioNodeServer.h"

int main() {
	
	AudioNodeServer* server = new AudioNodeServer(17598);
	printf("Starting server...\n");
	server->run();
	while (true) {
		Sleep(1);
	};
	return 0;
}