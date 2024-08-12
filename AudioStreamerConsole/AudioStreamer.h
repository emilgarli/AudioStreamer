#pragma once
#pragma once

#include <iostream>
#include <sndfile.h>
#include <boost/asio.hpp>

class WavFileReader {
public:
    WavFileReader(const std::string& filePath);
    ~WavFileReader();
    bool open();
    int readFrames(short* buffer, int frames);
    int getSampleRate() const;
    int getChannels() const;
    int getFormat() const;
    void streamWavFile(const std::string& host, const std::string& port);

    void async_stream_audio(boost::asio::ip::tcp::socket& socket);

private:
    std::string filePath;
    SNDFILE* file;
    SF_INFO sfinfo;
};