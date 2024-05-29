#pragma once
#include "sdk.h"
// boost.beast будет использовать std::string_view вместо boost::string_view
//#define BOOST_NO_CXX17_HDR_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <filesystem>
#include <iostream>
#include <variant>

namespace http_server {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;

using tcp = net::ip::tcp;
using namespace std::literals;

void ReportError(beast::error_code ec, std::string_view operation);

class SessionBase {
   public:
    using HttpRequest = http::request<http::string_body>;
    // Запрещаем копирование и присваивание объектов SessionBase и его наследников
    SessionBase(const SessionBase&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;
    void Run();

   protected:
    explicit SessionBase(tcp::socket&& socket) : stream_(std::move(socket)) {}

    template <typename Body, typename Fields>
    void Write(http::response<Body, Fields>&& response) {
        // Запись выполняется асинхронно, поэтому response перемещаем в область кучи
        auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));
        auto self = GetSharedThis();
        http::async_write(stream_, *safe_response,
                          [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
                              self->OnWrite(safe_response->need_eof(), ec, bytes_written);
                          });
    }

   private:
    void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written);
    /* Асинхронное чтение запроса */
    void Read();
    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read);

    void Close(beast::error_code ec) { stream_.socket().shutdown(tcp::socket::shutdown_send, ec); }

    virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;
    virtual void HandleRequest(HttpRequest&& request) = 0;

    // tcp_stream содержит внутри себя сокет и добавляет поддержку таймаутов
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    HttpRequest request_;
};

template <typename RequestHandler>
class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
   public:
    using HttpRequest = http::request<http::string_body>;
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& request_handler)
        : SessionBase(std::move(socket)), request_handler_(std::forward<Handler>(request_handler)) {}

   private:
    std::shared_ptr<SessionBase> GetSharedThis() override { return this->shared_from_this(); }
    void HandleRequest(HttpRequest&& request) override {
        // Захватываем умный указатель на текущий объект Session в лямбде,
        // чтобы продлить время жизни сессии до вызова лямбды.
        // Используется generic-лямбда функция, способная принять response произвольного типа
        request_handler_(std::move(request),
                         [self = this->shared_from_this()](auto&& response) { self->Write(std::move(response)); });
    }

    RequestHandler request_handler_;
};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
   public:
    template <typename Handler>
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
        : ioc_(ioc),
          // Обработчики асинхронных операций acceptor_ будут вызываться в своём strand
          acceptor_(net::make_strand(ioc)),
          request_handler_(std::forward<Handler>(request_handler)) {
        // Открываем acceptor, используя протокол (IPv4 или IPv6), указанный в endpoint
        acceptor_.open(endpoint.protocol());
        // После закрытия TCP-соединения сокет некоторое время может считаться занятым,
        // чтобы компьютеры могли обменяться завершающими пакетами данных.
        // Однако это может помешать повторно открыть сокет в полузакрытом состоянии.
        // Флаг reuse_address разрешает открыть сокет, когда он "наполовину закрыт"
        acceptor_.set_option(net::socket_base::reuse_address(true));
        // Привязываем acceptor к адресу и порту endpoint
        acceptor_.bind(endpoint);
        // Переводим acceptor в состояние, в котором он способен принимать новые соединения
        // Благодаря этому новые подключения будут помещаться в очередь ожидающих соединений
        acceptor_.listen(net::socket_base::max_listen_connections);
    }
    void Run() { DoAccept(); }

   private:
    void DoAccept() {
        acceptor_.async_accept(
            // Передаём последовательный исполнитель, в котором будут вызываться обработчики
            // асинхронных операций сокета
            net::make_strand(ioc_),
            // С помощью bind_front_handler создаём обработчик, привязанный к методу OnAccept
            // текущего объекта.
            // Так как Listener — шаблонный класс, нужно подсказать компилятору, что
            // shared_from_this — метод класса, а не свободная функция.
            // Для этого вызываем его, используя this
            // Этот вызов bind_front_handler аналогичен
            // namespace ph = std::placeholders;
            // std::bind(&Listener::OnAccept, this->shared_from_this(), ph::_1, ph::_2)
            beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
    }

    // Метод socket::async_accept создаст сокет и передаст его передан в OnAccept
    void OnAccept(sys::error_code ec, tcp::socket socket) {
        using namespace std::literals;
        if (ec) {
            return ReportError(ec, "accept"sv);
        }
        // Асинхронно обрабатываем сессию
        AsyncRunSession(std::move(socket));
        // Принимаем новое соединение
        DoAccept();
    }
    void AsyncRunSession(tcp::socket&& socket) {
        std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)->Run();
    }

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandler request_handler_;
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    // При помощи decay_t исключим ссылки из типа RequestHandler,
    // чтобы Listener хранил RequestHandler по значению
    using MyListener = Listener<std::decay_t<RequestHandler>>;
    std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
}

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;
// Ответ, тело которого представлено в виде бинарной последовательности
using FileResponse = http::response<http::file_body>;
using HttpResponse = std::variant<FileResponse, StringResponse>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view JSON = "application/json"sv;
    constexpr static std::string_view APP_OCT_STREAM = "application/octet-stream"sv;
    constexpr static std::string_view APP_XML = "application/xml"sv;
    constexpr static std::string_view AUDIO_MPEG = "audio/mpeg"sv;
    constexpr static std::string_view IMG_BMP = "image/bmp"sv;
    constexpr static std::string_view IMG_GIF = "image/gif"sv;
    constexpr static std::string_view IMG_ICON = "image/vnd.microsoft.icon"sv;
    constexpr static std::string_view IMG_JPEG = "image/jpeg"sv;
    constexpr static std::string_view IMG_PNG = "image/png"sv;
    constexpr static std::string_view IMG_SVG = "image/svg+xml"sv;
    constexpr static std::string_view IMG_TIFF = "image/tiff"sv;
    constexpr static std::string_view TEXT_CSS = "text/css"sv;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view TEXT_JS = "text/javascript"sv;
    constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
};

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, StringRequest request,
                                  std::string_view content_type = ContentType::TEXT_PLAIN,
                                  std::string_view allowed_method = {},
                                  std::pair<http::field, std::string_view> keyValue = {});

HttpResponse MakeFileResponse(http::status status, http::file_body::value_type& body, size_t size,
                              unsigned http_version, bool keep_alive, std::string_view content_type);

}  // namespace http_server
