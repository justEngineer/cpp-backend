#include "json_loader.h"
//#include <boost/json.hpp>
//#include "boost/json/value_to.hpp"
// Позволяет загрузить содержимое файла в виде строки:
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <string>
#include "model.h"
#include <fstream>

namespace json = boost::json;
using namespace std::literals;

namespace model {


} // namespace model


namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    model::Game game;
    std::ifstream input_file_stream(json_path);
    std::string raw_input(std::istreambuf_iterator<char>(input_file_stream), {});
    auto obj = json::parse(raw_input).as_object();
    auto maps = value_to<std::vector<model::Map>>(obj.at({model::json_config::MAPS}));
    for (auto& map : maps) {
        game.AddMap(std::move(map));
    }
    return game;
}

}  // namespace json_loader
