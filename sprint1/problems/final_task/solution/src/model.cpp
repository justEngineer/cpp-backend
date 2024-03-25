#include "model.h"
#include <stdexcept>


namespace json = boost::json;

namespace model {


Building tag_invoke(json::value_to_tag<Building>, json::value const& jason_value)
{
    auto obj = jason_value.as_object();
    return Building ({
        value_to<Coord>(obj.at({json_config::map::building::POS_X})),
        value_to<Coord>(obj.at({json_config::map::building::POS_Y})),
        value_to<Dimension>(obj.at({json_config::map::building::SIZE_W})),
        value_to<Dimension>(obj.at({json_config::map::building::SIZE_H}))
    });
}

Road tag_invoke(json::value_to_tag<Road>, json::value const& jason_value)
{
    auto obj = jason_value.as_object();
    Coord x0(value_to<Coord>(obj.at({json_config::map::road::START_X})));
    Coord y0(value_to<Coord>(obj.at({json_config::map::road::START_Y})));
    if (obj.contains({json_config::map::road::END_X})) {
        Coord end(value_to<Coord>(obj.at({json_config::map::road::END_X})));
        return Road{Road::HORIZONTAL, {x0, y0}, end};
    } else {
        Coord end(value_to<Coord>(obj.at({json_config::map::road::END_Y})));
        return Road{Road::VERTICAL, {x0, y0}, end};
    }
}

Office tag_invoke(json::value_to_tag<Office>, json::value const& jason_value)
{
    json::object const& obj = jason_value.as_object();
    return Office {
        Office::Id(value_to<std::string>(obj.at({json_config::map::office::ID}))),
        {
            value_to<Coord>(obj.at({json_config::map::office::POS_X})),
            value_to<Coord>(obj.at({json_config::map::office::POS_Y}))
        },
        {
            value_to<Dimension>(obj.at({json_config::map::office::OFFSET_X})),
            value_to<Dimension>(obj.at({json_config::map::office::OFFSET_Y}))
        }
    };
}


Map tag_invoke(json::value_to_tag<Map>, json::value const& jason_value )
{
    auto obj = jason_value.as_object();
    Map map{
        Map::Id(value_to<std::string>(obj.at({json_config::map::ID}))),
        value_to<std::string>(obj.at({json_config::map::NAME}))
    };
    auto roads = value_to<std::vector<Road>>(obj.at({json_config::map::ROADS}));
    for (auto& road : roads) {
        map.AddRoad(std::move(road));
    }
    auto buildings = value_to<std::vector<Building>>(obj.at({json_config::map::BUILDINGS}));
    for (auto& building : buildings) {
        map.AddBuilding(std::move(building));
    }
    auto offices = value_to<std::vector<Office>>(obj.at({json_config::map::OFFICES}));
    for (auto& office : offices) {
        map.AddOffice(std::move(office));
    }
    return map;
}


void tag_invoke(json::value_from_tag, json::value& jason_value, Office const& office)
{
    auto id = *office.GetId();
    auto pos = office.GetPosition();
    auto offset = office.GetOffset();
    jason_value = {
        {json_config::map::office::ID, id},
        {json_config::map::office::POS_X, pos.x},
        {json_config::map::office::POS_Y, pos.y},
        {json_config::map::office::OFFSET_X, offset.dx},
        {json_config::map::office::OFFSET_Y, offset.dy}
    };
}

void tag_invoke(json::value_from_tag, json::value& jason_value, Building const& building)
{
    auto bounds = building.GetBounds();
    jason_value = {
        {json_config::map::building::POS_X, bounds.position.x},
        {json_config::map::building::POS_Y, bounds.position.y},
        {json_config::map::building::SIZE_W, bounds.size.width},
        {json_config::map::building::SIZE_H, bounds.size.height}
    };
}

void tag_invoke(json::value_from_tag, json::value& jason_value, Road const& road)
{
    auto start = road.GetStart();
    auto end = road.GetEnd();
    if (start.y == end.y) {
        jason_value = {
            {json_config::map::road::START_X, start.x},
            {json_config::map::road::START_Y, start.y},
            {json_config::map::road::END_X, end.x}
        };
    }
    else  {
        jason_value = {
            {json_config::map::road::START_X, start.x},
            {json_config::map::road::START_Y, start.y},
            {json_config::map::road::END_Y, end.y}
        };
    } 
}

void tag_invoke(json::value_from_tag, json::value& jason_value, Map const& map)
{
    auto form_array = [](auto container){
        json::array arr;
        for (const auto& item : container) {
            arr.push_back(json::value_from(item));
        }
        return arr;
    };
    json::object object;
    object[json_config::map::ID] = *map.GetId();
    object[json_config::map::NAME] = map.GetName();
    object[json_config::map::ROADS] = form_array(map.GetRoads());
    object[json_config::map::BUILDINGS] = form_array(map.GetBuildings());;
    object[json_config::map::OFFICES] = form_array(map.GetOffices());
    jason_value.emplace_object() = object;
}

void tag_invoke(json::value_from_tag, json::value& jason_value, std::vector<Map> const& maps)
{
    auto form_array = [](auto container){
        json::array arr;
        for (const auto& item : container) {
            json::object object;
            object[json_config::map::ID] = *item.GetId();
            object[json_config::map::NAME] = item.GetName();
            arr.push_back(object);
        }
        return arr;
    };
    jason_value = form_array(maps);
}


void tag_invoke(json::value_from_tag, json::value& json_value, ResponseError const& err)
{
    json_value = {
        {json_error::CODE, err.code},
        {json_error::MESSAGE, err.message}
    };
}



using namespace std::literals;

void Map::AddOffice(Office&& office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(Map&& map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.push_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

}  // namespace model
