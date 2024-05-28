#pragma once
#include <boost/json.hpp>
#include <boost/log/trivial.hpp>                         // для BOOST_LOG_TRIVIAL
#include <boost/log/utility/manipulators/add_value.hpp>  // Вывод в поток манипулятора (logging::add_value)
#include <boost/log/utility/setup/file.hpp>              // add_file_log()
#include <iostream>

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

BOOST_LOG_ATTRIBUTE_KEYWORD(file, "File", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(line, "Line", int)

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value)

namespace logger {

namespace message {

using namespace std::literals;
constexpr std::string_view SERVER_STARTED = "server started"sv;
constexpr std::string_view SERVER_STOPPED = "server exited"sv;
constexpr std::string_view REQUEST_RECEIVED = "request received"sv;
constexpr std::string_view RESPONSE_SENT = "response sent"sv;

}  // namespace message

void InitLogger();
void LogServerIsStarted(const std::string& address, unsigned int port, const std::string& message);
void LogServerIsShuttingDown(const std::string& message, int code, const std::string& exception_descr = "");
void LogRequest(const std::string& ip_address, const std::string& uri, std::string_view http_method);
}  // namespace logger
