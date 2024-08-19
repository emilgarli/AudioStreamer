#include <Rawsocket.h>
#include <afxwin.h>
#include <iostream>
#include <portaudio.h>
#include <sndfile.h>
#include <vector>
#include <thread>

CWizReadWriteSocket* cSocket = NULL;
struct AudioData {
    SNDFILE* file;
    SF_INFO sfinfo;
    sf_count_t framesLeft;
};
// Callback function called by PortAudio to fill the output buffer
static int playCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    AudioData* audioData = (AudioData*)userData;
    short* out = (short*)outputBuffer;
    (void)inputBuffer; // Prevent unused variable warnings.
    (void)timeInfo;
    (void)statusFlags;

    if (audioData->framesLeft == 0) {
        return paComplete;
    }

    // Determine how many frames to read
    sf_count_t framesToRead = framesPerBuffer;
    if (audioData->framesLeft < framesPerBuffer) {
        framesToRead = audioData->framesLeft;
    }
    // Read audio data directly into the output buffer
    sf_count_t framesRead = sf_readf_short(audioData->file, out, framesToRead);
    audioData->framesLeft -= framesRead;

    // Send the exact audio data being played over the socket
    cSocket->Write(out, framesRead * audioData->sfinfo.channels * sizeof(short));
    // If less data was read than requested, zero out the remaining part of the output buffer
    if (framesRead < framesPerBuffer) {
        memset(out + framesRead * audioData->sfinfo.channels, 0,
            (framesPerBuffer - framesRead) * audioData->sfinfo.channels * sizeof(short));
    }
    memset(out, 0, framesPerBuffer * audioData->sfinfo.channels * sizeof(short));
    return (audioData->framesLeft > 0) ? paContinue : paComplete;
}



int startPlayback(const char* filePath) {
    LPCTSTR ipAddress = "127.0.0.1";
    bool Connected = false;
    SF_INFO sfinfo;
    SNDFILE* file = sf_open(filePath, SFM_READ, &sfinfo);
    if (!file) {
        std::cerr << "Failed to open file: " << filePath << "\n";
        return 1;
    }

    auto audioData = std::make_shared<AudioData>();
    audioData->file = file;
    audioData->sfinfo = sfinfo;
    audioData->framesLeft = sfinfo.frames;
    WSADATA wsaData;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    cSocket = new CWizReadWriteSocket();
    while (!Connected) {
        Connected = cSocket->Connect(ipAddress, 17598);
        if (!Connected){
            std::cout << "Unable to find server at port 17598. Retrying..." << std::endl;
        }
    }
    if (WSAGetLastError() != 0)
        std::cerr << "Listen failed: " << GetLastSocketErrorText() << std::endl;
    int written = 0;
    while (written < 1) {
        written = cSocket->Write("c", 1);
        if (written < 0)
            std::cout << "Error writing to server" << std::endl;
    }
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << "\n";
        return 1;
    }

    PaStream* stream;
    err = Pa_OpenDefaultStream(&stream,
        0, // No input channels
        1, // Output channels sfinfo.channels
        paInt16, // Sample format
        44100,
        1024,
        playCallback,
        audioData.get());
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << "\n";
        return 1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << "\n";
        return 1;
    }

    std::cout << "Playing audio...\n";
    while (Pa_IsStreamActive(stream) == 1) {
        Pa_Sleep(100);
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << "\n";
        return 1;
    }

    Pa_Terminate();
    sf_close(file);
    return 0;
}

int main() {
    std::cout << "Enter path to audio file: ";
    std::string filePath = "";
    std::getline(std::cin, filePath);
    if (filePath == ""){
        filePath = "C:\\Users\\Emil\\Music\\AnnaBlanton_Waves_Full\\AnnaBlanton_Waves_Full\\06_AcousticGtrDI.wav";
        std::cout << "No path entered. Using default path." << std::endl;
    }
    startPlayback(filePath.c_str());
    std::cout << "Finished playing.\n";
    return 0;
}
