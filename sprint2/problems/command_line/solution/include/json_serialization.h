#pragma once
#include "model.h"

#include <boost/json.hpp>
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
void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, std::vector<Player> const& players);

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv,
                GetStateWrapper<const std::vector<Player>> const& players);

struct EmptyObject {};

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, EmptyObject const& dummy);

}  // namespace model
