#pragma once
#include "config.h"
#include "game.h"
#include "http_server.h"
#include "model.h"
#define BOOST_BEAST_USE_STD_STRING_VIEW

struct Config {
    bool timer_period_exists{false};
    uint64_t timer_period;
    std::string config_file;
    std::string static_dir_path;
    bool is_random_spawn;
};

namespace http_handler {

using namespace std::literals;
namespace url {

constexpr std::string_view SEPARATOR = "/"sv;
constexpr std::string_view API = "/api/"sv;
constexpr std::string_view GET_ALL_MAPS = "/api/v1/maps"sv;
constexpr std::string_view GET_MAP_WITH_ID = "/api/v1/maps/"sv;
constexpr std::string_view ADD_PLAYER = "/api/v1/game/join"sv;
constexpr std::string_view GET_ALL_PLAYERS = "/api/v1/game/players"sv;
constexpr std::string_view GET_STATE = "/api/v1/game/state"sv;
constexpr std::string_view PLAYER_ACTION = "/api/v1/game/player/action"sv;
constexpr std::string_view CHANGE_TIME = "/api/v1/game/tick"sv;

}  // namespace url

namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;
namespace net = boost::asio;
namespace sys = boost::system;

using namespace http_server;

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
    using Strand = net::strand<net::io_context::executor_type>;

    class Ticker : public std::enable_shared_from_this<Ticker> {
       public:
        using Strand = net::strand<net::io_context::executor_type>;
        using Handler = std::function<void(std::chrono::milliseconds delta)>;

        // Функция handler будет вызываться внутри strand с интервалом period
        Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
            : strand_{strand}, period_{period}, handler_{std::move(handler)} {}

        void Start() {
            net::dispatch(strand_, [self = this->shared_from_this()] {
                self->last_tick_ = Clock::now();
                self->ScheduleTick();
            });
        }

        //       private:
        void ScheduleTick() {
            assert(strand_.running_in_this_thread());
            timer_.expires_after(period_);
            timer_.async_wait([self = shared_from_this()](sys::error_code ec) { self->OnTick(ec); });
        }

        void OnTick(sys::error_code ec) {
            using namespace std::chrono;
            assert(strand_.running_in_this_thread());

            if (!ec) {
                auto this_tick = Clock::now();
                auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
                last_tick_ = this_tick;
                try {
                    handler_(delta);
                } catch (...) {}
                ScheduleTick();
            }
        }

        using Clock = std::chrono::steady_clock;

        Strand strand_;
        std::chrono::milliseconds period_;
        net::steady_timer timer_{strand_};
        Handler handler_;
        std::chrono::steady_clock::time_point last_tick_;
    };

   public:
    explicit RequestHandler(Strand api_strand, model::Game& game, app::Config& config)
        : game_{game}, path_to_static_folder_{config.static_dir_path}, api_strand_{api_strand} {
        using namespace std::literals;
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

        if (config.timer_period_exists) {
            timer_ = std::make_shared<Ticker>(api_strand, std::chrono::milliseconds(config.timer_period),
                                              [this, time_delta = config.timer_period](
                                                  std::chrono::milliseconds delta) { game_.ChangeTime(time_delta); });
        }
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;
    // точка входа в обработку http запроса
    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto version = req.version();
        auto keep_alive = req.keep_alive();
        try {
            if (req.target().starts_with(url::API)) {  // запрос API
                auto handle = [self = shared_from_this(), send, req = std::forward<decltype(req)>(req), version,
                               keep_alive] {
                    try {
                        assert(self->api_strand_.running_in_this_thread());
                        return send(self->HandleApiRequest(req));
                    } catch (...) {
                        // send(self->ReportServerError("Internal error"sv, "Exception while handling request"sv,
                        //                              std::move(req)));
                    }
                };
                return net::dispatch(api_strand_, handle);
            } else {  // запрос файла
                HttpResponse response = HandleFileRequest(req);
                // HttpResponse response = HandleFileRequest(std::forward<decltype(req)>(req));
                std::visit([&](auto&& arg) { send(arg); }, response);
            }
        } catch (...) {
            send(ReportServerError("Internal error"sv, "Exception while handling request"sv, req));
        }
    }
    void Init() {
        if (timer_) {
            timer_->Start();
        }
    }

   private:
    StringResponse HandleApiRequest(const StringRequest& request);
    StringResponse GetAllMaps(const StringRequest& req);
    StringResponse GetMapByID(const StringRequest& req, const std::vector<std::string>& url);
    HttpResponse HandleFileRequest(const StringRequest& req);
    StringResponse ReportServerError(std::string_view code, std::string_view msg, const StringRequest& req,
                                     http::status status = http::status::bad_request,
                                     std::string_view allowed_method = {},
                                     std::pair<http::field, std::string_view> keyValue = {});
    StringResponse AddNewPlayer(const http_server::StringRequest& req);
    StringResponse GetAllPlayers(const StringRequest& req);
    StringResponse GetGameState(const StringRequest& req);
    StringResponse ChangeGameState(const StringRequest& req);
    StringResponse ChangeTime(const StringRequest& req);

    model::Game& game_;
    const fs::path path_to_static_folder_;
    std::unordered_map<std::string, std::string> supported_file_types;
    Strand api_strand_;
    std::shared_ptr<Ticker> timer_;
};

}  // namespace http_handler
