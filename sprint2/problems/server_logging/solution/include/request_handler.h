#pragma once
#include "http_server.h"
#include "model.h"
#define BOOST_BEAST_USE_STD_STRING_VIEW

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;

class RequestHandler {
   public:
    explicit RequestHandler(model::Game& game, fs::path path_to_static_folder)
        : game_{game}, path_to_static_folder_{std::move(path_to_static_folder)} {
        using namespace std::literals;
        using namespace http_server;
        supported_file_types[".htm"s] = ContentType::TEXT_HTML;
        supported_file_types[".html"s] = ContentType::TEXT_HTML;
        supported_file_types[".css"s] = ContentType::TEXT_CSS;
        supported_file_types[".txt"s] = ContentType::TEXT_PLAIN;
        supported_file_types[".js"s] = ContentType::TEXT_JS;
        supported_file_types[".json"s] = ContentType::JSON;
        supported_file_types[".xml"s] = ContentType::APP_XML;
        supported_file_types[".png"s] = ContentType::IMG_PNG;
        supported_file_types[".jpg"s] = ContentType::IMG_JPEG;
        supported_file_types[".jpe"s] = ContentType::IMG_JPEG;
        supported_file_types[".jpeg"s] = ContentType::IMG_JPEG;
        supported_file_types[".gif"s] = ContentType::IMG_GIF;
        supported_file_types[".bmp"s] = ContentType::IMG_BMP;
        supported_file_types[".ico"s] = ContentType::IMG_ICON;
        supported_file_types[".tiff"s] = ContentType::IMG_TIFF;
        supported_file_types[".tif"s] = ContentType::IMG_TIFF;
        supported_file_types[".svg"s] = ContentType::IMG_SVG;
        supported_file_types[".svgz"s] = ContentType::IMG_SVG;
        supported_file_types[".mp3"s] = ContentType::AUDIO_MPEG;
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        http_server::HttpResponse response = HandleRequest(std::forward<decltype(req)>(req));
        // Отправляем ответ с определением типа
        // response -> arg
        std::visit([&](auto&& arg) { send(arg); }, response);
    }
    http_server::HttpResponse HandleRequest(http_server::StringRequest&& req);
    http_server::HttpResponse RequestRouter(http_server::StringRequest&& req);
    http_server::HttpResponse GetAllMaps(http_server::StringRequest&& req);
    http_server::HttpResponse GetMapByID(http_server::StringRequest&& req, const std::vector<std::string>& url);
    http_server::HttpResponse GetStaticFile(http_server::StringRequest&& req);
    http_server::HttpResponse HandleBadRequest(http_server::StringRequest&& req);

   private:
    model::Game& game_;
    const fs::path path_to_static_folder_;
    std::unordered_map<std::string, std::string> supported_file_types;
};

}  // namespace http_handler
