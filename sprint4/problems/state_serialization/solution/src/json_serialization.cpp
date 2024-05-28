#include "json_serialization.h"
#include <algorithm>
namespace json = boost::json;

namespace model {

Building tag_invoke(json::value_to_tag<Building>, json::value const& jason_value) {
    auto obj = jason_value.as_object();
    return Building({value_to<Coord>(obj.at({json_obj_game::map::building::POS_X})),
                     value_to<Coord>(obj.at({json_obj_game::map::building::POS_Y})),
                     value_to<Dimension>(obj.at({json_obj_game::map::building::SIZE_W})),
                     value_to<Dimension>(obj.at({json_obj_game::map::building::SIZE_H}))});
}

Road tag_invoke(json::value_to_tag<Road>, json::value const& jason_value) {
    auto obj = jason_value.as_object();
    Coord x0(value_to<Coord>(obj.at({json_obj_game::map::road::START_X})));
    Coord y0(value_to<Coord>(obj.at({json_obj_game::map::road::START_Y})));
    if (obj.contains({json_obj_game::map::road::END_X})) {
        Coord end(value_to<Coord>(obj.at({json_obj_game::map::road::END_X})));
        return Road{Road::HORIZONTAL, {x0, y0}, end};
    } else {
        Coord end(value_to<Coord>(obj.at({json_obj_game::map::road::END_Y})));
        return Road{Road::VERTICAL, {x0, y0}, end};
    }
}

Office tag_invoke(json::value_to_tag<Office>, json::value const& jason_value) {
    json::object const& obj = jason_value.as_object();
    return Office{Office::Id(value_to<std::string>(obj.at({json_obj_game::map::office::ID}))),
                  {value_to<Coord>(obj.at({json_obj_game::map::office::POS_X})),
                   value_to<Coord>(obj.at({json_obj_game::map::office::POS_Y}))},
                  {value_to<Dimension>(obj.at({json_obj_game::map::office::OFFSET_X})),
                   value_to<Dimension>(obj.at({json_obj_game::map::office::OFFSET_Y}))}};
}

LootGeneratorCfg tag_invoke(json::value_to_tag<LootGeneratorCfg>&, json::value const& jv) {
    json::object const& obj = jv.as_object();
    return LootGeneratorCfg{value_to<double>(obj.at({(json_obj_game::loot_generator_cfg::PERIOD)})),
                            value_to<double>(obj.at({(json_obj_game::loot_generator_cfg::PROBABILITY)}))};
}

void tag_invoke(json::value_from_tag, json::value& jv, LootGeneratorCfg const& loot_cfg) {
    json::object obj;
    obj[json_obj_game::loot_generator_cfg::PERIOD] = loot_cfg.period_in_seconds_;
    obj[json_obj_game::loot_generator_cfg::PROBABILITY] = loot_cfg.probability_;
    jv.emplace_object() = obj;
}

Map tag_invoke(json::value_to_tag<Map>, json::value const& jason_value) {
    auto obj = jason_value.as_object();
    double dog_speed{0.0};
    uint32_t bag_capacity{0};
    if (auto it = obj.find(json_obj_game::map::DOG_SPEED); it != obj.end()) {
        dog_speed = value_to<double>(it->value());
    }
    if (auto it = obj.find(json_obj_game::map::BAG_CAPACITY); it != obj.end()) {
        bag_capacity = value_to<uint32_t>(it->value());
    }
    Map map{Map::Id(value_to<std::string>(obj.at({json_obj_game::map::ID}))),
            value_to<std::string>(obj.at({json_obj_game::map::NAME})), dog_speed,
            obj.at(std::string(json_obj_game::map::LOOT_TYPES)).as_array(), bag_capacity};

    auto roads = value_to<std::vector<Road>>(obj.at({json_obj_game::map::ROADS}));
    for (auto& road : roads) {
        map.AddRoad(std::move(road));
    }
    auto buildings = value_to<std::vector<Building>>(obj.at({json_obj_game::map::BUILDINGS}));
    for (auto& building : buildings) {
        map.AddBuilding(std::move(building));
    }
    auto offices = value_to<std::vector<Office>>(obj.at({json_obj_game::map::OFFICES}));
    for (auto& office : offices) {
        map.AddOffice(std::move(office));
    }
    return map;
}

void tag_invoke(json::value_from_tag, json::value& jason_value, Office const& office) {
    auto id = *office.GetId();
    auto pos = office.GetPosition();
    auto offset = office.GetOffset();
    jason_value = {{json_obj_game::map::office::ID, id},
                   {json_obj_game::map::office::POS_X, pos.x},
                   {json_obj_game::map::office::POS_Y, pos.y},
                   {json_obj_game::map::office::OFFSET_X, offset.dx},
                   {json_obj_game::map::office::OFFSET_Y, offset.dy}};
}

void tag_invoke(json::value_from_tag, json::value& jason_value, Building const& building) {
    auto bounds = building.GetBounds();
    jason_value = {{json_obj_game::map::building::POS_X, bounds.position.x},
                   {json_obj_game::map::building::POS_Y, bounds.position.y},
                   {json_obj_game::map::building::SIZE_W, bounds.size.width},
                   {json_obj_game::map::building::SIZE_H, bounds.size.height}};
}

void tag_invoke(json::value_from_tag, json::value& jason_value, Road const& road) {
    auto start = road.GetStart();
    auto end = road.GetEnd();
    if (start.y == end.y) {
        jason_value = {{json_obj_game::map::road::START_X, start.x},
                       {json_obj_game::map::road::START_Y, start.y},
                       {json_obj_game::map::road::END_X, end.x}};
    } else {
        jason_value = {{json_obj_game::map::road::START_X, start.x},
                       {json_obj_game::map::road::START_Y, start.y},
                       {json_obj_game::map::road::END_Y, end.y}};
    }
}

void tag_invoke(json::value_from_tag, json::value& jason_value, Map const& map) {
    auto form_array = [](auto container) {
        json::array arr;
        std::transform(container.begin(), container.end(), std::back_inserter(arr),
                       [](const auto& arg) { return json::value_from(arg); });
        return arr;
    };
    json::object object;
    object[json_obj_game::map::ID] = *map.GetId();
    object[json_obj_game::map::NAME] = map.GetName();
    object[json_obj_game::map::ROADS] = form_array(map.GetRoads());
    object[json_obj_game::map::BUILDINGS] = form_array(map.GetBuildings());
    object[json_obj_game::map::OFFICES] = form_array(map.GetOffices());
    object[json_obj_game::map::LOOT_TYPES] = map.GetLootTypes();
    jason_value.emplace_object() = object;
}

void tag_invoke(json::value_from_tag, json::value& jason_value, std::vector<Map> const& maps) {
    auto form_array = [](auto container) {
        json::array arr;
        for (const auto& item : container) {
            json::object object;
            object[json_obj_game::map::ID] = *item.GetId();
            object[json_obj_game::map::NAME] = item.GetName();
            arr.push_back(object);
        }
        return arr;
    };
    jason_value = form_array(maps);
}

void tag_invoke(json::value_from_tag, json::value& json_value, ResponseError const& err) {
    json_value = {{json_error::CODE, err.code}, {json_error::MESSAGE, err.message}};
}

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, model::Player const& join_result) {
    json::object object;
    object[json_add_player_response::TOKEN] = join_result.token_;
    object[json_add_player_response::PLAYER_ID] = join_result.id_;
    jv.emplace_object() = object;
}

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, std::vector<Player> const& players) {
    json::object object;
    for (const auto& player : players) {
        json::object object_name;
        object_name[json_get_players_response::LIST_PLAYERS_NAME] = player.name_;
        object[std::to_string(player.id_)] = object_name;
    }
    jv.emplace_object() = object;
}

void tag_invoke(json::value_from_tag, json::value& jv, Position const& pos) {
    json::array res = {pos.x, pos.y};
    jv = res;
}

void tag_invoke(json::value_from_tag, json::value& jv, Speed const& speed) {
    json::array res = {speed.ux, speed.uy};
    jv = res;
}

void tag_invoke(json::value_from_tag, json::value& jv, Dog const& dog) {
    jv = {{json_dog::POSITION, json::value_from(dog.GetPosition())},
          {json_dog::SPEED, json::value_from(dog.GetSpeed())},
          {json_dog::DIRECTION, json::value_from(dog.GetDirectionAsString())},
          {json_dog::BAG, json::value_from(dog.GetBag())},
          {json_dog::SCORE, json::value_from(dog.GetScore())}};
}

void tag_invoke(json::value_from_tag, json::value& jv, Item const& item) {
    jv = {{json_get_state_response::ITEM_TYPE, json::value_from(item.type_)},
          {json_get_state_response::ITEM_POSITION, json::value_from(item.position_)}};
}

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv,
                std::pair<const std::vector<Player>*, const ItemsHash*> const& wrapper) {
    auto [players, items] = wrapper;
    json::object object_result;
    if (players) {
        json::object object_player_info;
        for (const auto& player : *players) {
            object_player_info[std::to_string(player.id_)] = json::value_from(player.dog_);
        }
        object_result[json_get_state_response::PLAYERS_ARRAY] = object_player_info;
    }
    if (items) {
        json::object object_item_info;
        for (const auto& [_, item_info] : *items) {
            object_item_info[item_info.GetIdAsString()] = json::value_from(item_info);
        }
        object_result[json_get_state_response::ITEMS_ARRAY] = object_item_info;
    }
    jv.emplace_object() = object_result;
}

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, EmptyObject const& /*dummy*/) {
    json::object object;
    object.clear();
    jv.emplace_object() = object;
}

}  // namespace model
