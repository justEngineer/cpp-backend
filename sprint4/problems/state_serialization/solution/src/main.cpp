#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <boost/signals2.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include "config.h"
#include "json_loader.h"
#include "logger.h"
#include "logging_request_handler.h"
#include "model_serialization.h"
#include "request_handler.h"
#include "sdk.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace fs = std::filesystem;
namespace signals = boost::signals2;

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
    try {
        std::optional<app::Config> config = app::ParseCommandLine(argc, argv);
        if (!config.has_value()) {
            std::cerr << "Usage: game_server <game-config-json> <static-data-dir-json>"sv << std::endl;
            return EXIT_SUCCESS;
        }
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(config->config_file);
        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
        logger::InitLogger();

        // strand для выполнения запросов к API
        auto api_strand = net::make_strand(ioc);
        std::unique_ptr<serialization::GameSerializer> serializer_ptr;
        if (config->is_state_path_exists) {
            serializer_ptr = std::make_unique<serialization::GameSerializer>(api_strand, game, config.value());
        }
        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
            logger::LogServerIsShuttingDown(std::string(logger::message::SERVER_STARTED), EXIT_SUCCESS);
        });
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        // Создаём обработчик запросов в куче, управляемый shared_ptr
        auto handler =
            std::make_shared<http_handler::RequestHandler>(api_strand, game, config.value(), serializer_ptr.get());
        game.SetSerializer(serializer_ptr.get());
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
