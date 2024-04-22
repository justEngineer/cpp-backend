#include "audio.h"
#include <boost/asio.hpp>
#include <array>
#include <iostream>
#include <string>
#include <string_view>

namespace net = boost::asio;
// TCP больше не нужен, импортируем имя UDP
using net::ip::udp;
using namespace std::literals;

void startUdpClient(uint16_t port) {
    static const size_t max_buffer_size = 65000*8;
    Recorder recorder(ma_format_u8, 1);
    std::string ipAddress;
    while (true) {
        std::cout << "Press Enter to record message..." << std::endl;
        std::getline(std::cin, ipAddress);
        auto rec_result = recorder.Record(65000, 1.5s);
        std::cout << "Recording done" << std::endl;
        auto bytesCount = rec_result.data.size() * recorder.GetFrameSize();
            std::cout << "Recorded " << bytesCount << " bytes" << std::endl;
        try {
            net::io_context io_context;
            // Перед отправкой данных нужно открыть сокет. 
            // При открытии указываем протокол (IPv4 или IPv6) вместо endpoint.
            udp::socket socket(io_context, udp::v4());
            boost::system::error_code ec;
            auto endpoint = udp::endpoint(net::ip::make_address(ipAddress, ec), port);
            //auto endpoint = udp::endpoint(net::ip::make_address("127.0.0.1", ec), port);
            socket.send_to(net::buffer(rec_result.data), endpoint);
            // Получаем данные и endpoint.
            // std::array<char, max_buffer_size> recv_buf;
            // udp::endpoint sender_endpoint;
            // size_t size = socket.receive_from(net::buffer(recv_buf), sender_endpoint);
            // std::cout << "Server responded "sv << std::string_view(recv_buf.data(), size) << std::endl;
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

void startUdpServer(uint16_t port) {
    static const size_t max_buffer_size = 65000;
    Player player(ma_format_u8, 1);
    try {
        boost::asio::io_context io_context;
        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));
        // Запускаем сервер в цикле, чтобы можно было работать со многими клиентами
        // std::array<char, max_buffer_size> recv_buf;
        Recorder::RecordingResult recv_buf;
        for (;;) {
            // Создаём буфер достаточного размера, чтобы вместить датаграмму.
            udp::endpoint remote_endpoint;
            // Получаем не только данные, но и endpoint клиента
            auto size = socket.receive_from(boost::asio::buffer(recv_buf.data), remote_endpoint);
            auto framesCount = recv_buf.data.size() / player.GetFrameSize();
            std::cout << "Recieved " << recv_buf.data.size() << " bytes" << std::endl;
            std::cout << "Recieved " << framesCount << " frames" << std::endl;
            player.PlayBuffer(recv_buf.data.data(), recv_buf.frames, 1.5s);
            std::cout << "Playing done" << std::endl;
            recv_buf.data.clear();
            // std::cout << "Client said "sv << std::string_view(recv_buf.data(), size) << std::endl;
            // Отправляем ответ на полученный endpoint, игнорируя ошибку.
            // На этот раз не отправляем перевод строки: размер датаграммы будет получен автоматически.
            // boost::system::error_code ignored_error;
            // socket.send_to(boost::asio::buffer("Hello from UDP-server"sv), remote_endpoint, 0, ignored_error);
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

int main(int argc, char** argv) {
    std::cout << "command arguments: " << argc << std::endl;
    if(argc != 3) {
        std::cout << "Error, command arguments (server/client, port) not provided" << std::endl;
        return 1;
    }
    try {
        int port{std::stoi(argv[2])};
        std::cout << "command argument 1: " << argv[1] << std::endl;
        if(port < 0) {
            std::cout << "Error, invalid command argument port" << std::endl;
            return 1;
        }
        if(std::string(argv[1]) == "client"sv) {
            startUdpClient(port);
        }
        else if (std::string(argv[1]) == "server"sv) {
            startUdpServer(port);
        }
        else {
            std::cout << "Error, command arguments (server/client, port) not provided" << std::endl;
            return 1;
        }
    }
    catch (std::invalid_argument const& ex) {
        std::cout << "std::invalid_argument::what(): " << ex.what() << '\n';
    }
    catch (std::out_of_range const& ex) {
        std::cout << "std::out_of_range::what(): " << ex.what() << '\n';
    }
    return 0;
}
