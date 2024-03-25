#include "sdk.h"
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "http_server.h"

namespace {

namespace net = boost::asio;
namespace sys = boost::system;
namespace http = boost::beast::http;

using namespace std::literals;

// Запускает функцию function на thread_num потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned thread_num, const Fn& function) {
    thread_num = std::max(1u, thread_num);
    std::vector<std::jthread> workers;
    workers.reserve(thread_num - 1);
    // Запускаем thread_num-1 рабочих потоков, выполняющих функцию function
    while (--thread_num) {
        workers.emplace_back(function);
    }
    function();
}


}  // namespace

int main() {
    const unsigned num_threads = std::thread::hardware_concurrency();

    net::io_context ioc(num_threads);

    // Подписываемся на сигналы и при их получении завершаем работу сервера
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
        if (!ec) {
            ioc.stop();
        }
    });

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr net::ip::port_type port = 8080;
    http_server::ServeHttp(ioc, {address, port},
                           [](auto&& req, auto&& sender) { sender(http_server::OnRequest(std::forward<decltype(req)>(req))); });

    // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
    std::cout << "Server has started..."sv << std::endl;

    RunWorkers(num_threads, [&ioc] { ioc.run(); });
}