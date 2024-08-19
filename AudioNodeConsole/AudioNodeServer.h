#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include "Rawsocket.h"
#include <condition_variable>

class AudioNodeServer {
public:
    AudioNodeServer(std::shared_ptr<CWizReadWriteSocket> sock);
    static int InitializeConnection(std::shared_ptr<CWizReadWriteSocket> socket, AudioNodeServer* server);
    void setSocket(std::shared_ptr<CWizReadWriteSocket> sock);
private:
    static void audioReader(std::vector<char>& buffer, std::mutex& bufferMutex, std::condition_variable& bufferCv, AudioNodeServer* server);
    static void handleClient(AudioNodeServer* server);
    //void audioReader(std::vector<char>& buffer, std::mutex& bufferMutex, std::condition_variable& bufferCv);
    //~AudioNodeServer();

    std::shared_ptr<CWizReadWriteSocket> socket = nullptr;
    std::vector<std::shared_ptr<CWizReadWriteSocket>> sources_;
    std::vector<std::shared_ptr<CWizReadWriteSocket>> receivers_;
    std::mutex clients_mutex_;
    int numClients = 0;
    bool is_running = false;

    std::vector<char> audioBuffer_;
    std::mutex bufferMutex_;
    std::condition_variable bufferCv_;
};
