#include "http_server.h"
#include "logger.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {

void ReportError(beast::error_code ec, std::string_view operation) {
    BOOST_LOG_TRIVIAL(info) << "Error while operation " << operation << " , error code: " << ec;
}

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, StringRequest request,
                                  std::string_view content_type, std::string_view allowed_method,
                                  std::pair<http::field, std::string_view> keyValue) {
    StringResponse response(status, request.version());
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(request.keep_alive());
    if (status == http::status::method_not_allowed) {
        response.set(http::field::allow, allowed_method);
    }
    if (keyValue.first != http::field::unknown) {
        response.set(keyValue.first, keyValue.second);
    }
    return response;
}

HttpResponse MakeFileResponse(http::status status, http::file_body::value_type& body, size_t size,
                              unsigned http_version, bool keep_alive, std::string_view content_type) {
    FileResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = std::move(body);
    response.content_length(size);
    response.keep_alive(keep_alive);
    return response;
}

void SessionBase::Run() {
    // Вызываем метод Read, используя executor объекта stream_.
    // Таким образом вся работа со stream_ будет выполняться, используя его executor
    net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
    if (ec) {
        return ReportError(ec, "write"sv);
    }
    if (close) {
        // Семантика ответа требует закрыть соединение
        return Close(ec);
    }
    // Считываем следующий запрос
    Read();
}

void SessionBase::Read() {
    using namespace std::literals;
    // Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
    request_ = {};
    stream_.expires_after(30s);
    // Считываем request_ из stream_, используя buffer_ для хранения считанных данных
    http::async_read(stream_, buffer_, request_,
                     // По окончании операции будет вызван метод OnRead
                     beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
    using namespace std::literals;
    if (ec == http::error::end_of_stream) {
        // Нормальная ситуация - клиент закрыл соединение
        return Close(ec);
    }
    if (ec) {
        return ReportError(ec, "read"sv);
    }
    HandleRequest(std::move(request_));
}

}  // namespace http_server
