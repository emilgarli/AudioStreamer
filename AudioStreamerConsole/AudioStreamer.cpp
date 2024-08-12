#include <iostream>
#include <portaudio.h>
#include <sndfile.h>
#include <vector>
#include <memory>
#include <thread>
#include "client.h"

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

    sf_count_t framesToRead = framesPerBuffer;
    if (audioData->framesLeft < framesPerBuffer) {
        framesToRead = audioData->framesLeft;
    }

    sf_count_t framesRead = sf_readf_short(audioData->file, out, framesToRead);
    audioData->framesLeft -= framesRead;

       
    
    if (audioData->socket && audioData->socket->is_open()) {
        boost::asio::async_write(*audioData->socket,
            boost::asio::buffer(out, framesRead * audioData->sfinfo.channels * sizeof(short)),
            [](const boost::system::error_code& ec, std::size_t) {
                if (ec) {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                }
            });
    }
  
    memset(out, 0, framesPerBuffer * audioData->sfinfo.channels * sizeof(short));

    // If we read less than requested, zero out the remaining buffer
    if (framesRead < framesPerBuffer) {
        memset(out + framesRead * audioData->sfinfo.channels, 0,
            (framesPerBuffer - framesRead) * audioData->sfinfo.channels * sizeof(short));
    }

    return paContinue;
}

int startPlayback(const char* filePath) {
    
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

    boost::asio::io_context io_context;
    Client client(io_context, "127.0.0.1", "17598", audioData);

    std::thread io_thread([&io_context]() {
        io_context.run();
        });

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

    io_thread.join();
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
