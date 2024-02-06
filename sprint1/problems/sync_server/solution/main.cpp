#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <iostream>
#include <thread>
#include <syncstream>
#include <optional>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp> 

namespace net = boost::asio;
namespace sys = boost::system;
namespace beast = boost::beast;
namespace http = beast::http;
// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>; 
using tcp = net::ip::tcp;
using namespace std::literals;

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
} 
// Структура ContentType задаёт область видимости для констант,
// задающий значения HTTP-заголовка Content-Type
struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type = ContentType::TEXT_HTML) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

StringResponse HandleRequest(StringRequest&& req) {
    const auto text_response = [&req](http::status status, std::string_view text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive());
    };

    // Здесь можно обработать запрос и сформировать ответ, но пока всегда отвечаем: Hello
    return text_response(http::status::ok, "<strong>Hello</strong>"sv);
} 

std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer) {
    beast::error_code ec;
    StringRequest req;
    // Считываем из socket запрос req, используя buffer для хранения данных.
    // В ec функция запишет код ошибки.
    http::read(socket, buffer, req, ec);

    if (ec == http::error::end_of_stream) {
        return std::nullopt;
    }
    if (ec) {
        throw std::runtime_error("Failed to read request: "s.append(ec.message()));
    }
    return req;
} 

void DumpRequest(const StringRequest& req) {
    std::cout << "Method:" << req.method_string() << ", Target:" << req.target() << std::endl;
    // Выводим заголовки запроса
    for (const auto& header : req) {
        std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
    }
} 

template <typename RequestHandler>
void HandleConnection(tcp::socket& socket, RequestHandler&& handle_request) {
    try {
        // Буфер для чтения данных в рамках текущей сессии.
        beast::flat_buffer buffer;
        std::string body;
        // Продолжаем обработку запросов, пока клиент их отправляет
        while (auto request = ReadRequest(socket, buffer)) {
            const auto text_response = [request](http::status status, std::string_view text) {
                return MakeStringResponse(status, text, request->version(), request->keep_alive());
            };
            // Обрабатываем запрос и формируем ответ сервера
            DumpRequest(*request);
            StringResponse response;
            if(request->method() == http::verb::get) {
                auto target = request->target();
                auto pos = target.find_last_of ('/');
                body = "Hello, ";
                if (pos != std::string::npos) {
                    body.append(target.substr(pos + 1));
                }
                // Здесь можно обработать запрос и сформировать ответ, но пока всегда отвечаем: Hello
                response = text_response(http::status::ok, body);
                // Добавляем заголовок Content-Type: text/html
                response.set(http::field::content_type, "text/html"sv);
                // Формируем заголовок Content-Length, сообщающий длину тела ответа
                response.content_length(response.body().size());
                // Формируем заголовок Connection в зависимости от значения заголовка в запросе
                response.keep_alive(request->keep_alive());
            }
            else if(request->method() == http::verb::head) {
                auto target = request->target();
                auto pos = target.find_last_of ('/');
                body = "Hello, ";
                if (pos != std::string::npos) {
                    body.append(target.substr(pos + 1));
                }
                // Здесь можно обработать запрос и сформировать ответ, но пока всегда отвечаем: Hello
                response = text_response(http::status::ok, ""sv);
                // Добавляем заголовок Content-Type: text/html
                response.set(http::field::content_type, "text/html"sv);
                // Формируем заголовок Content-Length, сообщающий длину тела ответа
                response.content_length(body.size());
                // Формируем заголовок Connection в зависимости от значения заголовка в запросе
                response.keep_alive(request->keep_alive());
            }
            else {
                response = text_response(http::status::method_not_allowed, "Invalid method"sv);
                // Добавляем заголовок Content-Type: text/html
                response.set(http::field::content_type, "text/html"sv);
                response.set(http::field::allow, "GET, HEAD"sv);
                // Формируем заголовок Content-Length, сообщающий длину тела ответа
                response.content_length(response.body().size());
                // Формируем заголовок Connection в зависимости от значения заголовка в запросе
                response.keep_alive(request->keep_alive());
            }
            // Делегируем обработку запроса функции handle_request
            //StringResponse response = handle_request(*std::move(request));

            // Формируем ответ со статусом 200 и версией равной версии запроса
            // StringResponse response(http::status::ok, request->version());

            // Отправляем ответ сервера клиенту
            http::write(socket, response);

            // Прекращаем обработку запросов, если семантика ответа требует это
            if (response.need_eof()) {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    beast::error_code ec;
    // Запрещаем дальнейшую отправку данных через сокет
    socket.shutdown(tcp::socket::shutdown_send, ec);
} 

int main() {
    const unsigned num_threads = std::thread::hardware_concurrency();

    net::io_context ioc(num_threads);

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr unsigned short port = 8080;

    // Объект, позволяющий принимать tcp-подключения к сокету
    tcp::acceptor acceptor(ioc, {address, port});
    std::cout << "Server has started..."sv << std::endl;
    while (true) {
        tcp::socket socket(ioc);
        acceptor.accept(socket);
        // Запускаем обработку взаимодействия с клиентом в отдельном потоке
        std::thread t(
            // Лямбда-функция будет выполняться в отдельном потоке
            [](tcp::socket socket) {
                // Вызываем HandleConnection, передавая ей функцию-обработчик запроса
                HandleConnection(socket, HandleRequest);
            },
            std::move(socket));  // Сокет нельзя скопировать, но можно переместить

        // После вызова detach поток продолжит выполняться независимо от объекта t
        t.detach();
    }
}