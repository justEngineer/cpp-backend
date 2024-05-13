#include <logger.h>
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/json.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <string_view>

using namespace std::literals;

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace json = boost::json;

namespace logger {

inline void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    strm << rec[additional_data] << std::endl;
}

inline std::string GetLogTime() {
    boost::posix_time::ptime tm(boost::posix_time::microsec_clock::universal_time());
    return to_iso_extended_string(tm);
}

void InitLogger() {
    logging::add_common_attributes();
    logging::add_console_log(std::clog, keywords::auto_flush = true, keywords::format = &MyFormatter);
}

void LogServerIsStarted(const std::string& address, unsigned int port, const std::string& message) {
    json::object resp_object;
    resp_object["message"] = message;
    resp_object["timestamp"] = GetLogTime();

    json::object data_object;
    data_object["address"] = address;
    data_object["port"] = port;

    resp_object["data"] = data_object;

    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, json::value(resp_object));
}

void LogServerIsShuttingDown(const std::string& message, int code, const std::string& exception_msg) {
    json::object resp_object;
    resp_object["message"] = message;
    resp_object["timestamp"] = GetLogTime();

    json::object data_object;
    data_object["code"] = code;
    if (!exception_msg.empty())
        data_object["exception"] = exception_msg;

    resp_object["data"] = data_object;

    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, json::value(resp_object));
}

void LogRequest(const std::string& ip_address, const std::string& uri, std::string_view http_method) {
    json::object resp_object;
    resp_object["message"] = std::string(logger::message::REQUEST_RECEIVED);
    resp_object["timestamp"] = GetLogTime();

    json::object data_object;
    data_object["ip"] = ip_address;
    data_object["URI"] = uri;
    data_object["method"] = std::string(http_method);

    resp_object["data"] = data_object;

    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, json::value(resp_object));
}

}  // namespace logger
