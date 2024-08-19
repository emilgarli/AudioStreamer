#include "AudioNodeServer.h"
#include <portaudio.h>
#include <iostream>
#include <vector>

struct AudioData {
    std::vector<char>* buffer;
    std::mutex* bufferMutex;
    std::condition_variable* bufferCv;
};

//Constructor. Param: A connected socket
AudioNodeServer::AudioNodeServer(std::shared_ptr<CWizReadWriteSocket> sock){
    socket = sock;
}

int AudioNodeServer::InitializeConnection(std::shared_ptr<CWizReadWriteSocket> socket, AudioNodeServer* server) {
    char readBuf[10] = {};
    int iRead = 0;
    iRead = socket->Read(readBuf, sizeof(readBuf));
    if (iRead <= 0) {
        std::cout << "Failed to read socket type" << std::endl;
        if (WSAGetLastError() != 0)
           std::cerr << "Listen failed: " << GetLastSocketErrorText() << std::endl;
        return -1;
    }

    //std::lock_guard<std::mutex> lock(clients_mutex_);

    if (readBuf[0] == 'c') {
        //sources_.push_back(socket);
        std::cout << "Audio source connected." << std::endl;
    }
    else if (readBuf[0] == 'r') {
        //receivers_.push_back(socket);
        std::cout << "Audio receiver connected." << std::endl;
    }
    else {
        std::cout << "Unknown client type." << std::endl;
        return -1;
    }

    server->setSocket(socket);
    server->handleClient(server);
    return 0;
}

void AudioNodeServer::setSocket(std::shared_ptr<CWizReadWriteSocket> sock)
{
    socket = sock;
}


/*
AudioNodeServer::~AudioNodeServer() {

}
*/
static int paCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData) {
    AudioData* audioData = static_cast<AudioData*>(userData);
    short* out = static_cast<short*>(outputBuffer);

    std::unique_lock<std::mutex> lock(*audioData->bufferMutex);
    if (audioData->buffer->empty()) {
        std::fill(out, out + framesPerBuffer * 2, 0); // Fill with silence if no data
        audioData->bufferCv->wait(lock); // Wait for new data
    }

    if (!audioData->buffer->empty()) {
        size_t bytesToCopy = min(audioData->buffer->size(), framesPerBuffer * 2 * sizeof(short));
        std::copy(audioData->buffer->begin(), audioData->buffer->begin() + bytesToCopy, reinterpret_cast<char*>(out));
        audioData->buffer->erase(audioData->buffer->begin(), audioData->buffer->begin() + bytesToCopy);

        if (bytesToCopy < framesPerBuffer * 2 * sizeof(short)) {
            std::fill(out + bytesToCopy / sizeof(short), out + framesPerBuffer * 2, 0); // Fill remaining with silence
        }
    }
    return paContinue;
}
//This thread method is responsible for reading the incomming socket data from the source
//and putting it in buffer that is shared with the playback function
void AudioNodeServer::audioReader(std::vector<char>& buffer,
                                  std::mutex& bufferMutex, 
                                  std::condition_variable& bufferCv,
                                  AudioNodeServer* server)
{
    try {
        char tempBuffer[2048] = {};
        int iRead = 0;
        while (true) {
            iRead = server->socket->Read(tempBuffer,sizeof(tempBuffer));
            if (iRead <= 0) {
                std::cout << "Error reading socket data: " << GetLastSocketErrorText()  << std::endl;
                break;
            }
            //Lock the shared buffer
            std::lock_guard<std::mutex> lock(bufferMutex);
            //Write to the shared buffer
            buffer.insert(buffer.end(), tempBuffer, tempBuffer + iRead);
            //And finally, notify the waiting playback thread that the buffer is open for reading
            bufferCv.notify_one();
        }
    }
    catch (std::exception& e) {
        std::cout << "Exception in audioReaderThread: " << e.what() << std::endl;
    }
}

void AudioNodeServer::handleClient(AudioNodeServer* server) {
    PaStream* stream;
    AudioData audioData;
    audioData.buffer = &server->audioBuffer_;
    audioData.bufferMutex = &server->bufferMutex_;
    audioData.bufferCv = &server->bufferCv_;

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return;
    }

    err = Pa_OpenDefaultStream(&stream,
        0,          // no input channels
        1,          // stereo output
        paInt16,    // 16-bit integer output
        44100,      // sample rate
        2048,       // frames per buffer
        paCallback, // callback function
        &audioData  // user data
    );

    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return;
    }
    //Here we start the music stream
    std::cout << "Starting audio playback." << std::endl;
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return;
    }
    
    //And here we start the thread that will fill the stream with 
    //music data.
    std::thread readerThread(AudioNodeServer::audioReader,
                            std::ref(server->audioBuffer_),
                            std::ref(server->bufferMutex_),
                            std::ref(server->bufferCv_),
                            server);
    readerThread.join();
    
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}
