#pragma once

#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

using namespace std::literals;
using sys_clock = std::chrono::system_clock;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    static constexpr std::string_view LOG_FILE_PATH = "/var/log/sample_log_"sv;
    static constexpr std::string_view LOG_FILE_EXT = ".log"sv;

    auto GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }

        return sys_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = sys_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        const auto t_c = std::chrono::system_clock::to_time_t(GetTime());
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&t_c), "%Y_%m_%d");
        return ss.str();
    };

    Logger() = default;
    Logger(const Logger&) = delete;

   public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template <class... Ts>
    void Log(const Ts&... args) {
        std::stringstream ss;
        ss << GetTimeStamp() << ": ";
        Log(ss, args...);
        const std::lock_guard<std::mutex> lock(log_file_mtx_);
        std::ofstream log_file{std::string(LOG_FILE_PATH) + GetFileTimeStamp() + std::string(LOG_FILE_EXT),
                               std::ios::app};
        log_file << ss.str() << std::endl;
    }

    // head
    template <typename T, class... Ts>
    void Log(std::stringstream& ss, T value, const Ts&... args) {
        ss << value;
        Log(ss, args...);
    }
    // tail
    void Log(std::stringstream& ss) {}

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        const std::lock_guard<std::mutex> lock(ts_mtx_);
        manual_ts_ = ts;
    }

   private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::mutex ts_mtx_;
    std::mutex log_file_mtx_;
};
