#include <boost/asio.hpp>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

class AudioNodeServer {
public:
    AudioNodeServer(short port);
    void run();

private:
    void startAccept();
    void handleAccept(const boost::system::error_code& error);
    void handleClient(bool is_source);
    void audioReaderThread(std::shared_ptr<boost::asio::ip::tcp::socket> socket, std::vector<char>& buffer, std::mutex& bufferMutex, std::condition_variable& bufferCv);
    ~AudioNodeServer();
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;

    std::vector<std::shared_ptr<boost::asio::ip::tcp::socket>> sources_;
    std::vector<std::shared_ptr<boost::asio::ip::tcp::socket>> receivers_;
    std::mutex clients_mutex_;
    int numClients = 0;
    bool is_running = false;

    std::vector<char> audioBuffer_;
    std::mutex bufferMutex_;
    std::condition_variable bufferCv_;
};
