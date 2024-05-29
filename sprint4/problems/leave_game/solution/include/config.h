#pragma once

#include <boost/program_options.hpp>
#include <chrono>
#include <optional>

namespace app {

struct Config {
    bool timer_period_exists{false};
    std::chrono::milliseconds timer_period;
    std::string config_file;
    std::string static_dir_path;
    bool is_random_spawn;
    std::string state_path;
    bool is_state_path_exists{false};
    std::chrono::milliseconds save_state_period;
    bool is_save_state_period_exists{false};
    std::string db_url;
};

std::optional<Config> ParseCommandLine(int argc, const char* const argv[]);

}  // namespace app
