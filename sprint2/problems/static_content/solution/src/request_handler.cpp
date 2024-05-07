#include "request_handler.h"
#include <boost/algorithm/string.hpp>
//#include <boost/json.hpp>
//#include "boost/json/serialize.hpp"
//#include "boost/json/value_from.hpp"
#include "model.h"

namespace json = boost::json;
namespace fs = std::filesystem;
namespace sys = boost::system;
using namespace std::literals;

namespace {

constexpr std::string_view URL_SEPARATOR = "/"sv;
constexpr std::string_view API_URL = "/api/"sv;
constexpr std::string_view GET_ALL_MAPS_URL = "/api/v1/maps"sv;
constexpr std::string_view GET_MAP_WITH_ID_URL = "/api/v1/maps/"sv;
constexpr std::string_view INDEX_FILE_URL = "/"sv;
constexpr std::string_view INDEX_FILE_PATH = "./index.html";

const size_t MAP_ID = 4;
const uint8_t GET_MAP_WITH_ID_URL_LEN = 5;

bool verifyRequestPath(fs::path path, fs::path root) {
    path = fs::weakly_canonical(path);
    root = fs::weakly_canonical(root);
    for (auto b = root.begin(), p = path.begin(); b != root.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

std::string GetContentType(const std::string& file_name,
                           const std::unordered_map<std::string, std::string>& file_types) {
    auto it = file_types.find(file_name.substr(file_name.rfind('.')));
    if (it != file_types.end()) {
        return it->second;
    } else {
        return std::string(http_server::ContentType::APP_OCT_STREAM);
    }
}

}  // namespace

namespace http_handler {

//RequestRouter

http_server::HttpResponse RequestHandler::GetAllMaps(http_server::StringRequest&& req) {
    auto body = json::serialize(json::value_from(game_.GetMaps()));
    return http_server::MakeStringResponse(http::status::ok, std::move(body), req.version(), req.keep_alive(),
                                           http_server::ContentType::JSON);
}

http_server::HttpResponse RequestHandler::GetMapByID(http_server::StringRequest&& req,
                                                     const std::vector<std::string>& url) {
    auto& maps = game_.GetMaps();
    http::status status;
    std::string body;
    auto it =
        std::find_if(maps.begin(), maps.end(), [&url](const auto& element) { return *element.GetId() == url[MAP_ID]; });
    if (it == maps.end()) {
        body = json::serialize(json::value_from(model::ResponseError{"mapNotFound", "Map not found"}));
        status = http::status::not_found;
    } else {
        body = json::serialize(json::value_from(*it));
        status = http::status::ok;
    }
    return http_server::MakeStringResponse(std::move(status), std::move(body), req.version(), req.keep_alive(),
                                           http_server::ContentType::JSON);
}

http_server::HttpResponse RequestHandler::HandleBadRequest(http_server::StringRequest&& req) {
    auto body = json::serialize(json::value_from(model::ResponseError{"badRequest", "Bad request"}));
    return http_server::MakeStringResponse(http::status::bad_request, std::move(body), req.version(), req.keep_alive(),
                                           http_server::ContentType::JSON);
};

http_server::HttpResponse RequestHandler::GetStaticFile(http_server::StringRequest&& req) {
    fs::path full_path_to_file;
    if (req.target() == INDEX_FILE_URL) {
        full_path_to_file = path_to_static_folder_ / INDEX_FILE_PATH;
    } else {
        full_path_to_file = path_to_static_folder_ / fs::path("." + std::string(req.target()));
    }
    if (verifyRequestPath(full_path_to_file, path_to_static_folder_)) {
        http::file_body::value_type file;
        if (sys::error_code ec; file.open(full_path_to_file.c_str(), beast::file_mode::read, ec), ec) {
            std::cerr << "Failed to open file "sv << full_path_to_file << std::endl;
            auto body = json::serialize(json::value_from(model::ResponseError{"mapNotFound", "Map not found"}));
            return http_server::MakeStringResponse(http::status::not_found, std::move(body), req.version(),
                                                   req.keep_alive(), http_server::ContentType::TEXT_PLAIN);
        } else {
            auto content_type = GetContentType(full_path_to_file, supported_file_types);
            return http_server::MakeFileResponse(http::status::ok, file, file.size(), req.version(), req.keep_alive(),
                                                 std::move(content_type));
        }
    } else {
        // Make error response
        auto body = json::serialize(json::value_from(model::ResponseError{"fileNotFound", "File Not Found"}));
        return http_server::MakeStringResponse(http::status::not_found, std::move(body), req.version(),
                                               req.keep_alive(), http_server::ContentType::TEXT_PLAIN);
    }
}

http_server::HttpResponse RequestHandler::HandleRequest(http_server::StringRequest&& request) {
    using namespace std::literals;
    if (request.method() != http::verb::get) {
        return http_server::MakeStringResponse(http::status::method_not_allowed, "Invalid method"sv, request.version(),
                                               request.keep_alive(), http_server::ContentType::JSON);
    }
    auto target = request.target();
    std::vector<std::string> url_parts;
    boost::split(url_parts, target, boost::is_any_of(URL_SEPARATOR));
    // получить все карты
    if (target == GET_ALL_MAPS_URL) {
        return GetAllMaps(std::move(request));
        // получить одну карту по её id
    } else if (url_parts.size() == GET_MAP_WITH_ID_URL_LEN && target.starts_with(GET_MAP_WITH_ID_URL)) {
        return GetMapByID(std::move(request), url_parts);
        // запрос статического файла
    } else if (!target.starts_with(API_URL)) {
        return GetStaticFile(std::move(request));
    }
    return HandleBadRequest(std::move(request));
}

}  // namespace http_handler
