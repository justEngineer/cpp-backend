#pragma once

#include <filesystem>
#include "request_handler.h"

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path, std::string_view db_url);
model::AddNewPlayerRequest ParseAddNewPlayerRequest(std::string str);
std::string_view ParseDogMoveRequest(std::string str);
std::pair<bool, std::chrono::milliseconds> ParseChangeTimeRequestToMilliseconds(std::string str);
std::pair<uint64_t, uint64_t> GetParametersFromUrl(std::string req_target);

}  // namespace json_loader
