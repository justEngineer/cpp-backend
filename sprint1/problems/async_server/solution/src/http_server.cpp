#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {
    
void ReportError(beast::error_code ec, std::string_view operation) {
    std::cout << "Error while operation " << operation << " , error code: " << ec << std::endl;
}

StringResponse OnRequest(StringRequest&& request) {
    StringResponse response;
    std::string body;
    if (request.method() == http::verb::get || request.method() == http::verb::head) {
        auto target = request.target();
        if (request.method() == http::verb::get)
        {
            auto pos = target.find_last_of('/');
            body = "Hello, ";
            if (pos != std::string::npos) {
                body.append(target.substr(pos + 1));
            }
        }
        else {
            body.clear();
        }
        // Здесь можно обработать запрос и сформировать ответ, но пока всегда отвечаем: Hello
        response = MakeStringResponse(http::status::ok, body, request.version(), request.keep_alive());
    } else {
        response = MakeStringResponse(http::status::method_not_allowed, "Invalid method"sv, request.version(), request.keep_alive());
        response.set(http::field::allow, "GET, HEAD"sv);
    }
    // Здесь можно обработать запрос и сформировать ответ, но пока всегда отвечаем: Hello
    return response;
}

}  // namespace http_server
