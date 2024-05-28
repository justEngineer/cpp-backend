#include "config.h"
#include <iostream>

using namespace std::literals;

namespace app {

std::optional<Config> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;
    po::options_description desc{"All options"s};
    Config cfg;
    desc.add_options()("help,h", "Show help")(
        "tick-period,t", po::value(&cfg.timer_period)->value_name("milliseconds"s), "set tick period")(
        "config-file,c", po::value(&cfg.config_file)->value_name("file"s), "set config file path")(
        "www-root,w", po::value(&cfg.static_dir_path)->value_name("dir"s), "set static files root")(
        "randomize-spawn-points", po::bool_switch(&cfg.is_random_spawn), "spawn dogs at random positions")(
        "state-file,w", po::value(&cfg.state_path)->value_name("file"s), "set game state file path")(
        "save-state-period,w", po::value(&cfg.save_state_period)->value_name("milliseconds"s),
        "set game state save period");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
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
    return cfg;
}

}  // namespace app
