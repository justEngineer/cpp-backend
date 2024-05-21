#include "json_loader.h"
#include "json_serialization.h"
// Позволяет загрузить содержимое файла в виде строки:
#include <boost/property_tree/json_parser.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include "game.h"
#include "model.h"

namespace json_loader {

namespace json = boost::json;
using namespace std::literals;

model::AddNewPlayerRequest ParseAddNewPlayerRequest(std::string str) {
    using namespace model;
    auto parsed_config_json = json::parse(str);
    auto obj = parsed_config_json.as_object();
    model::AddNewPlayerRequest res;
    res.name = value_to<std::string>(obj.at(std::string(json_add_player_request::NAME)));
    res.map_id = value_to<std::string>(obj.at(std::string(json_add_player_request::MAP_ID)));
    return res;
}

std::string_view ParseDogMoveRequest(std::string str) {
    using namespace model;
    auto parsed_config_json = json::parse(str);
    auto obj = parsed_config_json.as_object();
    return value_to<std::string_view>(obj.at(std::string(json_move_dog_request::MOVE)));
}

std::pair<bool, uint64_t> ParseChangeTimeRequestToMilliseconds(std::string str) {
    using namespace model;
    auto parsed_config_json = json::parse(str);
    auto obj = parsed_config_json.as_object();
    if (!obj.contains(json_change_time_request::DELTA)) {
        return {true, {}};
    }
    auto res = obj.at(std::string(json_change_time_request::DELTA));
    if (!res.is_int64()) {
        return {true, {}};
    }
    return {false, value_to<uint64_t>(res)};
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    std::ifstream input_file_stream(json_path);
    std::string raw_input(std::istreambuf_iterator<char>(input_file_stream), {});
    auto obj = json::parse(raw_input).as_object();
    auto default_dog_speed = value_to<double>(obj.at(std::string(model::json_obj_game::DEFAULT_DOG_SPEED)));
    uint32_t default_bag_capacity{0};
    if (auto it = obj.find(model::json_obj_game::DEFAULT_BAG_CAPACITY); it != obj.end()) {
        default_bag_capacity = value_to<uint32_t>(it->value());
    }
    auto loot_gen_cfg = value_to<model::LootGeneratorCfg>(obj.at({(model::json_obj_game::LOOT_GENERATOR_CFG)}));
    model::Game game(default_dog_speed, std::move(loot_gen_cfg), default_bag_capacity);
    auto maps = value_to<std::vector<model::Map>>(obj.at({model::json_obj_game::MAPS}));
    for (auto& map : maps) {
        game.AddMap(std::move(map));
    }
    return game;
}

}  // namespace json_loader
