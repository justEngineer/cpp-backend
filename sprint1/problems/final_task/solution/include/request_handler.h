#pragma once
#include "http_server.h"
#include "model.h"
#define BOOST_BEAST_USE_STD_STRING_VIEW

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        send(HandleRequest(std::forward<decltype(req)>(req)));
    }
    http_server::StringResponse HandleRequest(http_server::StringRequest&& req);

private:
    model::Game& game_;
};

}  // namespace http_handler
