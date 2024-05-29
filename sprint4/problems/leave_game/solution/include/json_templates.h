#pragma once
#include <string>

using namespace std::literals;
namespace model {

namespace json_obj_game {
constexpr static char MAPS[] = "maps";
constexpr static char DEFAULT_DOG_SPEED[] = "defaultDogSpeed";
constexpr static char LOOT_GENERATOR_CFG[] = "lootGeneratorConfig";
constexpr static char DEFAULT_BAG_CAPACITY[] = "defaultBagCapacity";
constexpr static char DOG_RETIREMENT_TIME[] = "dogRetirementTime";
namespace loot_generator_cfg {
constexpr static char PERIOD[] = "period";
constexpr static char PROBABILITY[] = "probability";
}  // namespace loot_generator_cfg
namespace map {
constexpr static char ID[] = "id";
constexpr static char NAME[] = "name";
constexpr static char ROADS[] = "roads";
constexpr static char BUILDINGS[] = "buildings";
constexpr static char OFFICES[] = "offices";
constexpr static char DOG_SPEED[] = "dogSpeed";
constexpr static char LOOT_TYPES[] = "lootTypes";
constexpr static char BAG_CAPACITY[] = "bagCapacity";
namespace road {
constexpr static char START_X[] = "x0";
constexpr static char START_Y[] = "y0";
constexpr static char END_X[] = "x1";
constexpr static char END_Y[] = "y1";
}  // namespace road
namespace building {
constexpr static char POS_X[] = "x";
constexpr static char POS_Y[] = "y";
constexpr static char SIZE_W[] = "w";
constexpr static char SIZE_H[] = "h";
}  // namespace building
namespace office {
constexpr static char ID[] = "id";
constexpr static char POS_X[] = "x";
constexpr static char POS_Y[] = "y";
constexpr static char OFFSET_X[] = "offsetX";
constexpr static char OFFSET_Y[] = "offsetY";
}  // namespace office
}  // namespace map
}  // namespace json_obj_game

namespace json_error {
constexpr static char CODE[] = "code";
constexpr static char MESSAGE[] = "message";
constexpr static char EXCEPTION[] = "exception";
}  // namespace json_error

namespace error_code {
constexpr static char BAD_REQUEST[] = "badRequest";
constexpr static char INVALID_ARGUMENT[] = "invalidArgument";
constexpr static char INVALID_METHOD[] = "invalidMethod";
constexpr static char INVALID_TOKEN[] = "invalidToken";
constexpr static char UNKNOWN_TOKEN[] = "unknownToken";
constexpr static char MAP_NOT_FOUND[] = "mapNotFound";
constexpr static char FILE_NOT_FOUND[] = "fileNotFound";
}  // namespace error_code

namespace json_log {
// Logger
constexpr static char LOGGER_TIMESTAMP[] = "timestamp";
constexpr static char LOGGER_DATA[] = "data";
constexpr static char LOGGER_MESSAGE[] = "message";
// Server
constexpr static char SERVER_PORT[] = "port";
constexpr static char SERVER_ADDRESS[] = "address";
//Request
constexpr static char REQUEST_IP[] = "ip";
constexpr static char REQUEST_URI[] = "URI";
constexpr static char REQUEST_METHOD[] = "method";
constexpr static char RESPONSE_TIME[] = "response_time";
constexpr static char RESPONSE_CODE[] = "code";
constexpr static char RESPONSE_CONTENT_TYPE[] = "content_type";
}  // namespace json_log

namespace json_add_player_request {
constexpr static char NAME[] = "userName";
constexpr static char MAP_ID[] = "mapId";
}  // namespace json_add_player_request

namespace json_move_dog_request {
constexpr static char MOVE[] = "move";
}  // namespace json_move_dog_request

namespace json_change_time_request {
constexpr static char DELTA[] = "timeDelta";
}  // namespace json_change_time_request

namespace json_add_player_response {
constexpr static char TOKEN[] = "authToken";
constexpr static char PLAYER_ID[] = "playerId";
}  // namespace json_add_player_response

namespace json_get_players_response {
constexpr static char LIST_PLAYERS_NAME[] = "name";
}  // namespace json_get_players_response

namespace json_get_state_response {
constexpr static char PLAYERS_ARRAY[] = "players";
constexpr static char ITEMS_ARRAY[] = "lostObjects";
constexpr static char ITEM_TYPE[] = "type";
constexpr static char ITEM_POSITION[] = "pos";
}  // namespace json_get_state_response

namespace json_dog {
constexpr static char POSITION[] = "pos";
constexpr static char SPEED[] = "speed";
constexpr static char DIRECTION[] = "dir";
constexpr static char BAG[] = "bag";
constexpr static char SCORE[] = "score";
}  // namespace json_dog

namespace json_record {
constexpr static char NAME[] = "name";
constexpr static char SCORE[] = "score";
constexpr static char PLAY_TIME[] = "playTime";
}  // namespace json_record

}  // namespace model
