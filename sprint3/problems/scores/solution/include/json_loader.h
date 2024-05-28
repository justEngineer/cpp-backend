#pragma once

#include <filesystem>
//#include "game.h"
#include "request_handler.h"

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path);
model::AddNewPlayerRequest ParseAddNewPlayerRequest(std::string str);
std::string_view ParseDogMoveRequest(std::string str);
std::pair<bool, uint64_t> ParseChangeTimeRequestToMilliseconds(std::string str);

}  // namespace json_loader
