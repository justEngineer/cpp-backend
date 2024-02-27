#include "request_handler.h"
#include <boost/algorithm/string.hpp>
//#include <boost/json.hpp>
//#include "boost/json/serialize.hpp"
//#include "boost/json/value_from.hpp"
#include "model.h"

namespace json = boost::json;

namespace model {
    
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
    boost::split(url, target, boost::is_any_of("/"));
    auto status{http::status::bad_request};
    auto body = json::serialize(json::value_from(model::ResponseError{"badRequest","Bad request"}));
    if (target == "/api/v1/maps") {
        body = json::serialize(json::value_from(game_.GetMaps()));
        status = http::status::ok;
    }
    else if (url.size() == 5 && target.starts_with("/api/v1/maps/")) {
        auto& maps = game_.GetMaps();
        auto it = std::find_if(maps.begin(), maps.end(), [&url](const auto& element){ return *element.GetId() == url[4]; });
        if (it ==  maps.end()) {
            body = json::serialize(json::value_from(model::ResponseError{"mapNotFound","Map not found"}));
            status = http::status::not_found;
        }
        else {
            body = json::serialize(json::value_from(*it));
            status = http::status::ok;
        }
    } else if (target.starts_with("/api/")) {
        body = json::serialize(json::value_from(model::ResponseError{"badRequest","Bad request"}));
        status = http::status::bad_request;
    }
    return http_server::MakeStringResponse(status, body, request.version(),
                                            request.keep_alive(), http_server::ContentType::JSON);
}

}  // namespace http_handler
