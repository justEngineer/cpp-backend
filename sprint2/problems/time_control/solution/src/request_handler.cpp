#include "request_handler.h"
#include <boost/algorithm/string.hpp>
#include <boost/json.hpp>
#include "boost/json/serialize.hpp"
#include "boost/json/value_from.hpp"
#include "json_loader.h"
#include "model.h"

namespace json = boost::json;
namespace fs = std::filesystem;
namespace sys = boost::system;
using namespace std::literals;

namespace {

constexpr std::string_view INDEX_FILE_URL = "/"sv;
constexpr std::string_view INDEX_FILE_PATH = "./index.html"sv;
constexpr std::string_view BEARER_TOKEN_FIELD = "Bearer"sv;
constexpr std::string_view EMPTY_JSON_RESPONSE = "{}"sv;

const size_t MAP_ID = 4;
const uint8_t GET_MAP_WITH_ID_URL_LEN = 5;
const uint8_t BEARER_TOKEN_SIZE = 32;

namespace HTTPMethods {
constexpr static std::string_view ADD_PLAYER = "POST"sv;
constexpr static std::string_view GET_PLAYERS = "GET, HEAD"sv;
};  // namespace HTTPMethods

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

std::pair<bool, std::string> GetAuthToken(std::string_view auth) {
    if (auth.find_first_of(BEARER_TOKEN_FIELD) == std::string_view::npos) {
        return {true, {}};
    }
    auth.remove_prefix(BEARER_TOKEN_FIELD.size());
    size_t pos{0};
    do {
        pos = auth.find_first_of(' ');
        if (pos == 0)
            auth.remove_prefix(1);
    } while (pos != std::string_view::npos);
    if (std::distance(auth.begin(), auth.end()) != BEARER_TOKEN_SIZE) {
        return {true, {}};
    }
    return {false, std::string(auth.begin(), auth.end())};
}

}  // namespace

namespace http_handler {

StringResponse RequestHandler::AddNewPlayer(const http_server::StringRequest& req) {
    auto req_content_type = req[http::field::content_type];
    if (req_content_type != http_server::ContentType::JSON) {
        return ReportServerError(model::error_code::INVALID_ARGUMENT, "Invalid content type"sv, req,
                                 http::status::bad_request, HTTPMethods::ADD_PLAYER,
                                 {http::field::cache_control, "no-cache"sv});
    }
    if (req.method() != http::verb::post) {
        return ReportServerError(model::error_code::INVALID_METHOD, "Only POST method is expected"sv, req,
                                 http::status::method_not_allowed, HTTPMethods::ADD_PLAYER,
                                 {http::field::cache_control, "no-cache"sv});
    }
    try {
        auto parsedRequest = json_loader::ParseAddNewPlayerRequest(req.body());
        if (parsedRequest.name.empty()) {
            return ReportServerError(model::error_code::INVALID_ARGUMENT, "Invalid name"sv, req,
                                     http::status::bad_request, HTTPMethods::ADD_PLAYER,
                                     {http::field::cache_control, "no-cache"sv});
        }
        auto [err, player] = game_.AddPlayer(parsedRequest);
        if (!err.empty()) {
            return ReportServerError(model::error_code::MAP_NOT_FOUND, "Map not found"sv, req, http::status::not_found,
                                     HTTPMethods::ADD_PLAYER, {http::field::cache_control, "no-cache"sv});
        }
        auto body = boost::json::serialize(json::value_from(*player));
        auto response =
            http_server::MakeStringResponse(http::status::ok, std::move(body), req, http_server::ContentType::JSON,
                                            HTTPMethods::ADD_PLAYER, {http::field::cache_control, "no-cache"sv});
        return response;
    } catch (std::exception err) {
        return ReportServerError(model::error_code::INVALID_ARGUMENT, "request parse error"sv, req,
                                 http::status::bad_request, HTTPMethods::ADD_PLAYER,
                                 {http::field::cache_control, "no-cache"sv});
    }
    return ReportServerError(model::error_code::BAD_REQUEST, "request error"sv, req, http::status::bad_request,
                             HTTPMethods::ADD_PLAYER, {http::field::cache_control, "no-cache"sv});
}

StringResponse RequestHandler::GetAllPlayers(const http_server::StringRequest& req) {
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        return ReportServerError(model::error_code::INVALID_METHOD, "Invalid method"sv, req,
                                 http::status::method_not_allowed, HTTPMethods::GET_PLAYERS,
                                 {http::field::cache_control, "no-cache"sv});
    }
    auto [err, token] = GetAuthToken(req[http::field::authorization]);
    if (!err) {
        // Получаем список игроков в виде response_body для игрока с токеном token
        try {
            auto players = game_.GetAllPlayersInSession(token);
            if (!players) {
                auto response = ReportServerError(model::error_code::UNKNOWN_TOKEN, "Player token has not been found"sv,
                                                  req, http::status::unauthorized, HTTPMethods::GET_PLAYERS,
                                                  {http::field::cache_control, "no-cache"sv});
                return response;
            }
            if (req.method() == http::verb::head) {
                auto response = http_server::MakeStringResponse(
                    http::status::ok, "", req, http_server::ContentType::JSON, HTTPMethods::GET_PLAYERS,
                    {http::field::cache_control, "no-cache"sv});

                return response;
            } else {  // GET-method
                auto body = boost::json::serialize(json::value_from(*players));
                auto response = http_server::MakeStringResponse(
                    http::status::ok, body, req, http_server::ContentType::JSON, HTTPMethods::GET_PLAYERS,
                    {http::field::cache_control, "no-cache"sv});
                return response;
            }
        } catch (...) {
            return ReportServerError(model::error_code::BAD_REQUEST, "Unknown error"sv, req, http::status::bad_request,
                                     HTTPMethods::GET_PLAYERS, {http::field::cache_control, "no-cache"sv});
        }
    } else {
        auto response = ReportServerError(model::error_code::INVALID_TOKEN, "Authorization header is missing"sv, req,
                                          http::status::unauthorized, HTTPMethods::GET_PLAYERS,
                                          {http::field::cache_control, "no-cache"sv});
        return response;
    }
    return ReportServerError(model::error_code::BAD_REQUEST, "Unknown error"sv, req, http::status::bad_request,
                             HTTPMethods::GET_PLAYERS, {http::field::cache_control, "no-cache"sv});
}

StringResponse RequestHandler::GetGameState(const http_server::StringRequest& req) {
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        return ReportServerError(model::error_code::INVALID_METHOD, "Invalid method"sv, req,
                                 http::status::method_not_allowed, HTTPMethods::GET_PLAYERS,
                                 {http::field::cache_control, "no-cache"sv});
    }
    auto [err, token] = GetAuthToken(req[http::field::authorization]);
    if (!err) {
        // Получаем список игроков в виде response_body для игрока с токеном token
        try {
            auto players = game_.GetAllPlayersInSession(token);
            if (!players) {
                auto response = ReportServerError(model::error_code::UNKNOWN_TOKEN, "Player token has not been found"sv,
                                                  req, http::status::unauthorized, HTTPMethods::GET_PLAYERS,
                                                  {http::field::cache_control, "no-cache"sv});
                return response;
            }
            if (req.method() == http::verb::head) {
                auto response = http_server::MakeStringResponse(
                    http::status::ok, "", req, http_server::ContentType::JSON, HTTPMethods::GET_PLAYERS,
                    {http::field::cache_control, "no-cache"sv});

                return response;
            } else {  // GET-method
                auto body = boost::json::serialize(
                    json::value_from(model::GetStateWrapper<const std::vector<model::Player>>(*players)));
                auto response = http_server::MakeStringResponse(
                    http::status::ok, body, req, http_server::ContentType::JSON, HTTPMethods::GET_PLAYERS,
                    {http::field::cache_control, "no-cache"sv});
                return response;
            }
        } catch (...) {
            return ReportServerError(model::error_code::BAD_REQUEST, "Unknown error"sv, req, http::status::bad_request,
                                     HTTPMethods::GET_PLAYERS, {http::field::cache_control, "no-cache"sv});
        }
    } else {
        auto response = ReportServerError(model::error_code::INVALID_TOKEN, "Authorization header is missing"sv, req,
                                          http::status::unauthorized, HTTPMethods::GET_PLAYERS,
                                          {http::field::cache_control, "no-cache"sv});
        return response;
    }
    return ReportServerError(model::error_code::BAD_REQUEST, "Unknown error"sv, req, http::status::bad_request,
                             HTTPMethods::GET_PLAYERS, {http::field::cache_control, "no-cache"sv});
}

StringResponse RequestHandler::ChangeGameState(const http_server::StringRequest& req) {
    if (req.method() != http::verb::post) {
        return ReportServerError(model::error_code::INVALID_METHOD, "Invalid method"sv, req,
                                 http::status::method_not_allowed, HTTPMethods::ADD_PLAYER,
                                 {http::field::cache_control, "no-cache"sv});
    }
    try {
        auto [err, token] = GetAuthToken(req[http::field::authorization]);
        if (!err) {
            // Получаем список игроков в виде response_body для игрока с токеном token
            if (!game_.FindAndMovePlayersDog(token, json_loader::ParseDogMoveRequest(req.body()))) {
                return ReportServerError(model::error_code::UNKNOWN_TOKEN, "Player token has not been found"sv, req,
                                         http::status::unauthorized, HTTPMethods::ADD_PLAYER,
                                         {http::field::cache_control, "no-cache"sv});
            } else {
                auto body = boost::json::serialize(json::value_from(model::EmptyObject()));
                return http_server::MakeStringResponse(http::status::ok, body, req, http_server::ContentType::JSON,
                                                       HTTPMethods::ADD_PLAYER,
                                                       {http::field::cache_control, "no-cache"sv});
            }
        } else {
            auto response = ReportServerError(model::error_code::INVALID_TOKEN, "Authorization header is missing"sv,
                                              req, http::status::unauthorized, HTTPMethods::GET_PLAYERS,
                                              {http::field::cache_control, "no-cache"sv});
            return response;
        }
    } catch (...) {
        return ReportServerError(model::error_code::BAD_REQUEST, "Unknown error"sv, req, http::status::bad_request,
                                 HTTPMethods::GET_PLAYERS, {http::field::cache_control, "no-cache"sv});
    }
    return ReportServerError(model::error_code::BAD_REQUEST, "Unknown error"sv, req, http::status::bad_request,
                             HTTPMethods::GET_PLAYERS, {http::field::cache_control, "no-cache"sv});
}

StringResponse RequestHandler::ChangeTime(const http_server::StringRequest& req) {
    if (req.method() != http::verb::post) {
        return ReportServerError(model::error_code::INVALID_METHOD, "Invalid method"sv, req,
                                 http::status::method_not_allowed, HTTPMethods::ADD_PLAYER,
                                 {http::field::cache_control, "no-cache"sv});
    }
    try {
        auto [err, time_delta] = json_loader::ParseChangeTimeRequestToMilliseconds(req.body());
        if (err) {
            return ReportServerError(model::error_code::INVALID_ARGUMENT, "Failed to parse tick request JSON"sv, req,
                                     http::status::bad_request, HTTPMethods::GET_PLAYERS,
                                     {http::field::cache_control, "no-cache"sv});
        } else {
            game_.ChangeTime(time_delta);
            auto body = boost::json::serialize(json::value_from(model::EmptyObject()));
            return http_server::MakeStringResponse(http::status::ok, body, req, http_server::ContentType::JSON,
                                                   HTTPMethods::ADD_PLAYER, {http::field::cache_control, "no-cache"sv});
        }
    } catch (...) {
        return ReportServerError(model::error_code::INVALID_ARGUMENT, "Failed to parse tick request JSON"sv, req,
                                 http::status::bad_request, HTTPMethods::GET_PLAYERS,
                                 {http::field::cache_control, "no-cache"sv});
    }
}

StringResponse RequestHandler::GetAllMaps(const http_server::StringRequest& req) {
    auto body = json::serialize(json::value_from(game_.GetMaps()));
    return http_server::MakeStringResponse(http::status::ok, std::move(body), req, http_server::ContentType::JSON, {},
                                           {http::field::cache_control, "no-cache"sv});
}

StringResponse RequestHandler::GetMapByID(const http_server::StringRequest& req, const std::vector<std::string>& url) {
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
    return http_server::MakeStringResponse(std::move(status), std::move(body), req, http_server::ContentType::JSON, {},
                                           {http::field::cache_control, "no-cache"sv});
}

StringResponse RequestHandler::ReportServerError(std::string_view code, std::string_view msg, const StringRequest& req,
                                                 http::status status, std::string_view allowed_method,
                                                 std::pair<http::field, std::string_view> keyValue) {
    auto body = json::serialize(json::value_from(model::ResponseError{std::string(code), std::string(msg)}));
    return http_server::MakeStringResponse(status, std::move(body), req, http_server::ContentType::JSON, allowed_method,
                                           keyValue);
};

HttpResponse RequestHandler::HandleFileRequest(const http_server::StringRequest& req) {
    if (req.method() != http::verb::get) {
        return http_server::MakeStringResponse(http::status::method_not_allowed, "Invalid method"sv, req,
                                               http_server::ContentType::JSON);
    }
    fs::path full_path_to_file;
    if (req.target() == INDEX_FILE_URL) {
        full_path_to_file = path_to_static_folder_ / INDEX_FILE_PATH;
    } else {
        full_path_to_file = path_to_static_folder_ / fs::path("." + std::string(req.target()));
    }
    if (verifyRequestPath(full_path_to_file, path_to_static_folder_)) {
        http::file_body::value_type file;
        if (sys::error_code ec; file.open(full_path_to_file.c_str(), beast::file_mode::read, ec), ec) {
            auto body = json::serialize(json::value_from(model::ResponseError{"fileNotFound", "Fail not found"}));
            return http_server::MakeStringResponse(http::status::not_found, std::move(body), req,
                                                   http_server::ContentType::TEXT_PLAIN);
        } else {
            auto content_type = GetContentType(full_path_to_file, supported_file_types);
            return http_server::MakeFileResponse(http::status::ok, file, file.size(), req.version(), req.keep_alive(),
                                                 std::move(content_type));
        }
    } else {
        // Make error response
        auto body = json::serialize(json::value_from(model::ResponseError{"fileNotFound", "File Not Found"}));
        return http_server::MakeStringResponse(http::status::not_found, std::move(body), req,
                                               http_server::ContentType::TEXT_PLAIN);
    }
    return ReportServerError("badRequest"sv, "Bad request"sv, req);
}

StringResponse RequestHandler::HandleApiRequest(const StringRequest& request) {
    using namespace std::literals;
    auto target = request.target();
    //    if (request.method() == http::verb::get) {
    std::vector<std::string> url_parts;
    boost::split(url_parts, target, boost::is_any_of(url::SEPARATOR));
    // получить все карты
    if (target == url::GET_ALL_MAPS) {
        return GetAllMaps(request);
        // получить одну карту по её id
    } else if (url_parts.size() == GET_MAP_WITH_ID_URL_LEN && target.starts_with(url::GET_MAP_WITH_ID)) {
        return GetMapByID(request, url_parts);
        // запрос списка игроков
    } else if (target == url::GET_ALL_PLAYERS) {
        return GetAllPlayers(request);
    }
    // аутентификация нового игрока
    else if (target == url::ADD_PLAYER) {
        return AddNewPlayer(request);
    } else if (target == url::GET_STATE) {
        return GetGameState(request);
    } else if (target == url::PLAYER_ACTION) {
        return ChangeGameState(request);
    } else if (target == url::CHANGE_TIME) {
        return ChangeTime(request);
    }
    return ReportServerError(model::error_code::BAD_REQUEST, "Invalid method"sv, request, http::status::bad_request,
                             HTTPMethods::GET_PLAYERS, {http::field::cache_control, "no-cache"sv});
}

}  // namespace http_handler
