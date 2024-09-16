#include "AudioNodeServer.h"
#include <portaudio.h>
#include <iostream>
#include <vector>

static int numClients = 0;
static int iter = 0;
static int socketiter = 0;

struct AudioData {
    std::vector<char>* buffer;
    std::mutex* bufferMutex;
    std::condition_variable* bufferCv;
};

//Constructor. Param: A connected socket
AudioNodeServer::AudioNodeServer(std::shared_ptr<CWizReadWriteSocket> sock) {
    socket = sock;
}

int AudioNodeServer::InitializeConnection(std::shared_ptr<CWizReadWriteSocket> socket) {
    char readBuf[10] = {};
    int iRead = 0;
    iRead = socket->Read(readBuf, sizeof(readBuf));
    if (iRead <= 0) {
        std::cout << "Failed to read socket type" << std::endl;
        if (WSAGetLastError() != 0)
           std::cerr << "Read socket type: " << GetLastSocketErrorText() << std::endl;
        return -1;
    }

    std::lock_guard<std::mutex> lock(clients_mutex_);

    if (readBuf[0] == 'c') {
        if (numClients > 1) {
            std::cout << "Max number of clients reached. Rejecting..." << std::endl;
            return 1;
        }
        sources_.push_back(socket);
        std::cout << "Audio source connected." << std::endl;
    }
    else if (readBuf[0] == 'r') {
        receivers_.push_back(socket);
        std::cout << "Audio receiver connected." << std::endl;
    }
    else {
        std::cout << "Unknown client type." << std::endl;
        return -1;
    }

    setSocket(socket);
    handleClient();
    return 0;
}

void AudioNodeServer::setSocket(std::shared_ptr<CWizReadWriteSocket> sock)
{
    socket = sock;
}

int AudioNodeServer::paCallback(const void* inputBuffer, 
    void* outputBuffer, 
    unsigned long framesPerBuffer, 
    const PaStreamCallbackTimeInfo* timeInfo, 
    PaStreamCallbackFlags statusFlags, 
    void* userData)
{
    if (!userData) {
        std::cerr << "Error: userData is null" << std::endl;
        return paAbort;
    }
    AudioData* audioData = static_cast<AudioData*>(userData);
    if (!audioData->buffer || !audioData->bufferMutex || !audioData->bufferCv) {
        std::cerr << "Error: Invalid AudioData structure" << std::endl;
        return paAbort;
    }
    short* out = static_cast<short*>(outputBuffer);
    std::unique_lock<std::mutex> lock(*audioData->bufferMutex);
    if (audioData->buffer->empty()) {
        std::fill(out, out + framesPerBuffer, 0); // Fill with silence if no data
        audioData->bufferCv->wait(lock); // Wait for new data
    }
    if (!audioData->buffer->empty()) {
        size_t bytesToCopy = min(audioData->buffer->size(), framesPerBuffer * sizeof(short));
        std::copy(audioData->buffer->begin(), audioData->buffer->begin() + bytesToCopy, reinterpret_cast<char*>(out));
        audioData->buffer->erase(audioData->buffer->begin(), audioData->buffer->begin() + bytesToCopy);
        /*
        if (bytesToCopy < framesPerBuffer * sizeof(short)) {
            std::fill(out + bytesToCopy / sizeof(short), out + framesPerBuffer * 2, 0); // Fill remaining with silence
        }*/
    }
    return paContinue;
}

//This thread method is responsible for reading the incomming socket data from the source
//and putting it in buffer that is shared with the playback function
void AudioNodeServer::audioReader(std::vector<char>& buffer,
                                  std::mutex& bufferMutex, 
                                  std::condition_variable& bufferCv)
{
    try {
        char tempBuffer[2048] = {};
        int iRead = 0;
        while (true) {
            iRead = socket->Read(tempBuffer, sizeof(tempBuffer));
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
            socketiter++;
        }
    }
    catch (std::exception& e) {
        std::cout << "Exception in audioReaderThread: " << e.what() << std::endl;
    }

    catch (...) {
        std::cout << "Unknown exception detected." << std::endl;
    }
    
}

void AudioNodeServer::handleClient() {
    PaStream* stream = nullptr;
    AudioData audioData;
    audioData.buffer = &audioBuffer_;
    audioData.bufferMutex = &bufferMutex_;
    audioData.bufferCv = &bufferCv_;

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
        paFramesPerBufferUnspecified,       // frames per buffer
        paCallback, // callback function
        &audioData  // user data
    );
    cout << "Stream is pointing to: " << stream << endl;
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
    std::thread readerThread(&AudioNodeServer::audioReader,
                            this,
                            std::ref(audioBuffer_),
                            std::ref(bufferMutex_),
                            std::ref(bufferCv_));
    readerThread.join();
    // This will make sure the stream runs until the end of the buffer.
    /*
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    */
    std::cout << "Done playing. Exiting... " << std::endl;
    cout << "Stream is pointing to: " << stream << endl;
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}
