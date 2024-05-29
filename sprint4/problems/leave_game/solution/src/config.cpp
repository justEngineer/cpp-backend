#include "config.h"
#include <chrono>
#include <filesystem>
#include <iostream>

using namespace std::literals;

namespace app {

std::optional<Config> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;
    po::options_description desc{"All options"s};
    Config cfg;
    uint32_t timer_period = 0;
    uint32_t save_state_period = 0;
    desc.add_options()("help,h", "Show help")("tick-period,t", po::value(&timer_period)->value_name("milliseconds"s),
                                              "set tick period")(
        "config-file,c", po::value(&cfg.config_file)->value_name("file"s), "set config file path")(
        "www-root,w", po::value(&cfg.static_dir_path)->value_name("dir"s), "set static files root")(
        "randomize-spawn-points", po::bool_switch(&cfg.is_random_spawn), "spawn dogs at random positions")(
        "state-file,w", po::value(&cfg.state_path)->value_name("file"s), "set game state file path")(
        "save-state-period,w", po::value(&save_state_period)->value_name("milliseconds"s),
        "set game state save period");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }
    if (!std::filesystem::exists(cfg.config_file)) {
        std::cout << "Config file doesn' exists, path: " << cfg.config_file << std::endl;
        return std::nullopt;
    }

    if (vm.contains("tick-period")) {
        cfg.timer_period = std::chrono::milliseconds(timer_period);
    }
    if (vm.contains("save-state-period")) {
        cfg.save_state_period = std::chrono::milliseconds(save_state_period);
    }
    cfg.timer_period_exists = vm.contains("tick-period"s);
    cfg.is_state_path_exists = vm.contains("state-file"s);
    cfg.is_save_state_period_exists = vm.contains("save-state-period"s);

    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Config file path have not been specified"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Static files root is not specified"s);
    }
    const char* db_url = std::getenv("GAME_DB_URL");
    if (!db_url) {
        throw std::runtime_error("DB URL is not specified");
    }
    cfg.db_url = db_url;
    return cfg;
}

}  // namespace app
