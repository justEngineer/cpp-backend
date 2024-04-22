#include "request_handler.h"
#include <boost/algorithm/string.hpp>
//#include <boost/json.hpp>
//#include "boost/json/serialize.hpp"
//#include "boost/json/value_from.hpp"
#include "model.h"

namespace json = boost::json;
using namespace std::literals;

namespace {
    constexpr std::string_view URL_SEPARATOR = "/"sv;
    constexpr std::string_view API_URL = "/api/"sv;
    constexpr std::string_view GET_ALL_MAPS_URL = "/api/v1/maps"sv;
    constexpr std::string_view GET_MAP_WITH_ID_URL = "/api/v1/maps/"sv;
    
    const size_t MAP_ID = 4;
    const uint8_t GET_MAP_WITH_ID_URL_LEN = 5;
}

namespace http_handler {

http_server::StringResponse RequestHandler::HandleRequest(http_server::StringRequest&& request) {
    using namespace std::literals;
    if (request.method() != http::verb::get) {
        return http_server::MakeStringResponse(http::status::method_not_allowed, "Invalid method"sv, request.version(),
                                               request.keep_alive(), http_server::ContentType::JSON);
    }
    auto target = request.target();
    std::vector<std::string> url;
    boost::split(url, target, boost::is_any_of(URL_SEPARATOR));
    auto status{http::status::bad_request};
    auto body = json::serialize(json::value_from(model::ResponseError{"badRequest","Bad request"}));
    if (target == GET_ALL_MAPS_URL) {
        body = json::serialize(json::value_from(game_.GetMaps()));
        status = http::status::ok;
    }
    else if (url.size() == GET_MAP_WITH_ID_URL_LEN && target.starts_with(GET_MAP_WITH_ID_URL)) {
        auto& maps = game_.GetMaps();
        auto it = std::find_if(maps.begin(), maps.end(), [&url](const auto& element){ return *element.GetId() == url[MAP_ID]; });
        if (it ==  maps.end()) {
            body = json::serialize(json::value_from(model::ResponseError{"mapNotFound","Map not found"}));
            status = http::status::not_found;
        }
        else {
            body = json::serialize(json::value_from(*it));
            status = http::status::ok;
        }
    } else if (target.starts_with(API_URL)) {
        body = json::serialize(json::value_from(model::ResponseError{"badRequest","Bad request"}));
        status = http::status::bad_request;
    }
    return http_server::MakeStringResponse(status, body, request.version(),
                                            request.keep_alive(), http_server::ContentType::JSON);
}

}  // namespace http_handler
