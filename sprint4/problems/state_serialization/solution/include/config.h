#pragma once

#include <boost/program_options.hpp>
#include <optional>
#define BOOST_BEAST_USE_STD_STRING_VIEW

namespace app {

struct Config {
    bool timer_period_exists{false};
    uint64_t timer_period;
    std::string config_file;
    std::string static_dir_path;
    bool is_random_spawn;
    std::string state_path;
    bool is_state_path_exists{false};
    uint64_t save_state_period;
    bool is_save_state_period_exists{false};
};

std::optional<Config> ParseCommandLine(int argc, const char* const argv[]);

}  // namespace app
