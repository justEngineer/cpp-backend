#pragma once
#include "model.h"

#include <list>
#include "boost/json/serialize.hpp"
#include "boost/json/value_from.hpp"

using namespace std::literals;

namespace model {

template <typename T>
struct GetStateWrapper {
    T const& players_;
    explicit GetStateWrapper(T const& players) : players_(players) {}
};

struct ResponseError {
    std::string code;
    std::string message;
};

namespace json = boost::json;

LootGeneratorCfg tag_invoke(json::value_to_tag<LootGeneratorCfg>, json::value const& jv);

void tag_invoke(json::value_from_tag, json::value& jv, LootGeneratorCfg const& loot_cfg);

Building tag_invoke(json::value_to_tag<Building>, json::value const& json_value);

Road tag_invoke(json::value_to_tag<Road>, json::value const& json_value);

Office tag_invoke(json::value_to_tag<Office>, json::value const& json_value);

Map tag_invoke(json::value_to_tag<Map>, json::value const& json_value);

void tag_invoke(json::value_from_tag, json::value& json_value, Office const& office);

void tag_invoke(json::value_from_tag, json::value& json_value, Building const& building);

void tag_invoke(json::value_from_tag, json::value& json_value, Road const& road);

void tag_invoke(json::value_from_tag, json::value& json_value, Map const& map);

void tag_invoke(json::value_from_tag, json::value& json_value, std::vector<Map> const& maps);

void tag_invoke(json::value_from_tag, json::value& json_value, ResponseError const& err);

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Player const& result);
void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, std::list<Player> const& players);

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv,
                std::pair<const std::list<Player>*, const ItemsHash*> const& players);

struct EmptyObject {};

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, EmptyObject const& dummy);

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv,
                std::pair<std::list<Player>*, std::vector<Item>*> const& wrapper);

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, std::vector<Player::PlayerInfo> const& info);

}  // namespace model
