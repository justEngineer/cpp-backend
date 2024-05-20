#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>
#include <boost/json/value.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/timer/timer.hpp>
#include "logger.h"
#include "model.h"
#include "request_handler.h"

namespace http_handler {

template <class SomeRequestHandler>
class LoggingRequestHandler {

    static constexpr uint64_t NANO_TO_MILLI = 1'000'000;

   public:
    LoggingRequestHandler(SomeRequestHandler& handler) : decorated_(handler) { handler.Init(); }

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send,
                    boost::asio::ip::address&& address) {
        using namespace std::literals;
        logger::LogRequest(address.to_string(), std::string(req.target()), req.method_string());

        boost::json::object response_json;
        boost::timer::cpu_timer latency_timer;
        latency_timer.start();
        auto loggingResponse = [&send, &latency_timer, &response_json](auto&& response) {
            latency_timer.stop();
            boost::timer::cpu_times ticker = latency_timer.elapsed();
            response_json[model::json_log::LOGGER_MESSAGE] = std::string(logger::message::RESPONSE_SENT);

            boost::json::object data_object;
            data_object[model::json_log::RESPONSE_TIME] = ticker.wall / NANO_TO_MILLI;
            data_object[model::json_log::RESPONSE_CODE] = response.result_int();
            data_object[model::json_log::RESPONSE_CONTENT_TYPE] =
                std::string(response[beast::http::field::content_type]);
            response_json[model::json_log::LOGGER_DATA] = data_object;

            send(response);
        };
        decorated_(std::forward<decltype(req)>(req), std::move(loggingResponse));
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, boost::json::value(response_json));
    }

   private:
    SomeRequestHandler& decorated_;
};

}  // namespace http_handler
