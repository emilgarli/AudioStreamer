#include "Client.h"

void Client::do_connect(const tcp::resolver::results_type& endpoints) {
    boost::asio::async_connect(*socket_, endpoints,
        [this](boost::system::error_code ec, tcp::endpoint) {
            if (!ec) {
                audioData_->socket = socket_; // Assign the socket to audioData
                printf("Successfully connected to server!\n");

                // Connected to Audio Node. Tell it we're a audio source
                do_write();
            }
            else {
                printf("Failed to connect to server: %s\n", ec.message().c_str());
            }
        });
}

void Client::do_write() {
    char outBuf = 'c';
    boost::asio::async_write(*socket_, boost::asio::buffer(&outBuf, 1),
        [this](const boost::system::error_code& ec, std::size_t /* bytes_transferred */) {
            if (!ec) {
                printf("Write completed successfully!\n");
            }
            else {
                printf("Write error: %s\n", ec.message().c_str());
            }
        });
}
