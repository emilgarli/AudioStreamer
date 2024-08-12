#include "AudioNodeServer.h"
#include <portaudio.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>

struct AudioData {
    std::vector<char>* buffer;
    std::mutex* bufferMutex;
    std::condition_variable* bufferCv;
};

AudioNodeServer::AudioNodeServer(short port)
    : acceptor_(io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
    socket_(io_context_) {
    startAccept();
}

void AudioNodeServer::startAccept() {
    acceptor_.async_accept(socket_, [this](const boost::system::error_code& error) {
        handleAccept(error);
        });
}

void AudioNodeServer::handleAccept(const boost::system::error_code& error) {
    if (!error) {
        auto buffer = std::make_shared<std::array<char, 1>>();
        boost::asio::async_read(socket_, boost::asio::buffer(*buffer),
            [this, buffer](const boost::system::error_code& ec, std::size_t) {
                if (!ec) {
                    char clientType = (*buffer)[0];
                    if (clientType == 'c') {
                        printf("Audio source connected.\n");
                        if (numClients++ > 50) {
                            printf("Maximum number of clients reached. Rejecting...\n");
                            socket_.close();
                            return;
                        }
                        std::lock_guard<std::mutex> lock(clients_mutex_);
                        sources_.push_back(std::make_shared<boost::asio::ip::tcp::socket>(std::move(socket_)));
                        std::thread(&AudioNodeServer::handleClient, this, true).detach();
                    }
                    else if (clientType == 'r') {
                        printf("Audio receiver connected.\n");
                        std::lock_guard<std::mutex> lock(clients_mutex_);
                        receivers_.push_back(std::make_shared<boost::asio::ip::tcp::socket>(std::move(socket_)));
                        std::thread(&AudioNodeServer::handleClient, this, false).detach();
                    }
                    else {
                        std::cerr << "Unknown client type tried to connect." << std::endl;
                    }
                }
                else {
                    std::cerr << "Error reading client type: " << ec.message() << std::endl;
                }
            });
    }
    startAccept();
}

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
        size_t bytesToCopy = std::min(audioData->buffer->size(), framesPerBuffer * 2 * sizeof(short));
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
void AudioNodeServer::audioReaderThread(std::shared_ptr<boost::asio::ip::tcp::socket> socket, std::vector<char>& buffer, std::mutex& bufferMutex, std::condition_variable& bufferCv) {
    try {
        char tempBuffer[2048];
        while (true) {
            boost::system::error_code error;
            size_t len = socket->read_some(boost::asio::buffer(tempBuffer), error);
            if (error == boost::asio::error::eof) {
                std::cout << "Connection closed by peer." << std::endl;
                break;
            }
            else if (error) {
                std::cerr << "Socket read error: " << error.message() << std::endl;
                break;
            }

            std::lock_guard<std::mutex> lock(bufferMutex);
            buffer.insert(buffer.end(), tempBuffer, tempBuffer + len);
            bufferCv.notify_one();
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in audioReaderThread: " << e.what() << std::endl;
    }
}

void AudioNodeServer::handleClient(bool is_source) {
    PaStream* stream;
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
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return;
    }
    //And here we start the thread that will fill the stream with 
    //music data.
    std::thread readerThread(&AudioNodeServer::audioReaderThread, this, sources_.back(), std::ref(audioBuffer_), std::ref(bufferMutex_), std::ref(bufferCv_));
    readerThread.detach();
    while (true)
        Sleep(2);
    Pa_Sleep(1000); // Run for a while to demonstrate

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}

void AudioNodeServer::run() {
    is_running = true;
    io_context_.run();
}
