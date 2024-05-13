#include "sdk.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>
#include "json_loader.h"
#include "logger.h"
#include "logging_request_handler.h"
#include "request_handler.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace fs = std::filesystem;

namespace {

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

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <static-data-dir-json>"sv << std::endl;
        return EXIT_FAILURE;
    }
    try {
        // 1. Загружаем карту из файла и построить модель игры
        // model::Game game = json_loader::LoadGame(
        //     "/home/alex/cpp-backend/backend-repo/sprint2/problems/server_logging/solution/data/config.json");
        model::Game game = json_loader::LoadGame(argv[1]);
        fs::path static_files_dir_path{std::string(argv[2])};
        // fs::path static_files_dir_path{
        //     "/home/alex/cpp-backend/backend-repo/sprint2/problems/server_logging/solution/static"};
        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
        logger::InitLogger();
        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
            logger::LogServerIsShuttingDown(std::string(logger::message::SERVER_STARTED), EXIT_SUCCESS);
        });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        // strand для выполнения запросов к API
        auto api_strand = net::make_strand(ioc);
        // Создаём обработчик запросов в куче, управляемый shared_ptr
        auto handler = std::make_shared<http_handler::RequestHandler>(api_strand, game, static_files_dir_path
                                                                      /*прочие параметры, нужные RequestHandler*/);
        http_handler::LoggingRequestHandler logging_handler(*handler);
        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port}, [&logging_handler, &address](auto&& req, auto&& send) {
            logging_handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send),
                            std::forward<decltype(address)>(address));
        });

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        logger::LogServerIsStarted(address.to_string(), port, std::string(logger::message::SERVER_STARTED));
        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] { ioc.run(); });
    } catch (const std::exception& ex) {
        logger::LogServerIsShuttingDown(std::string(logger::message::SERVER_STARTED), EXIT_FAILURE, ex.what());
        return EXIT_FAILURE;
    }
    return 0;
}
