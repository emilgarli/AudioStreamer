#pragma once
#include <sndfile.h>
#include <vector>
#include <memory>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512

struct AudioData {
    SNDFILE* file;
    SF_INFO sfinfo;
    short buffer[FRAMES_PER_BUFFER * 2];
    size_t framesLeft;
    std::shared_ptr<boost::asio::ip::tcp::socket> socket;
};

using boost::asio::ip::tcp;

class Client {
public:
    Client(boost::asio::io_context& io_context, const std::string& host, const std::string& port, std::shared_ptr<AudioData> audioData)
        : io_context_(io_context), socket_(std::make_shared<tcp::socket>(io_context)), audioData_(audioData) {
        tcp::resolver resolver(io_context_);
        auto endpoints = resolver.resolve(host, port);
        do_connect(endpoints);
    }

private:
    void do_connect(const tcp::resolver::results_type& endpoints);

    void do_write();

    boost::asio::io_context& io_context_;
    std::shared_ptr<tcp::socket> socket_;
    std::shared_ptr<AudioData> audioData_;
};