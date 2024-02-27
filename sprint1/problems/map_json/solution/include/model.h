#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "tagged.h"

#include <boost/json.hpp>
#include "boost/json/serialize.hpp"
#include "boost/json/value_from.hpp"

namespace model {
    
namespace json = boost::json;
    
using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
};



namespace json_config {
    constexpr const char * MAPS = "maps";
    namespace map {
        constexpr const char * ID = "id";
        constexpr const char * NAME = "name";
        constexpr const char * ROADS = "roads";
        constexpr const char * BUILDINGS = "buildings";
        constexpr const char * OFFICES = "offices";
        namespace road {
            constexpr const char * START_X = "x0";
            constexpr const char * START_Y = "y0";
            constexpr const char * END_X = "x1";
            constexpr const char * END_Y = "y1";
        }
        namespace building {
            constexpr const char * POS_X = "x";
            constexpr const char * POS_Y = "y";
            constexpr const char * SIZE_W = "w";
            constexpr const char * SIZE_H = "h";
        }
        namespace office {
            constexpr const char * ID = "id";
            constexpr const char * POS_X = "x";
            constexpr const char * POS_Y = "y";
            constexpr const char * OFFSET_X = "offsetX";
            constexpr const char * OFFSET_Y = "offsetY";
        }
    }
}

namespace json_error {
    constexpr const char * CODE = "code";
    constexpr const char * MESSAGE = "message";
}


Building tag_invoke(json::value_to_tag<Building>, json::value const& json_value);

Road tag_invoke(json::value_to_tag<Road>, json::value const& json_value);

Office tag_invoke(json::value_to_tag<Office>, json::value const& json_value);


Map tag_invoke(json::value_to_tag<Map>, json::value const& json_value );


void tag_invoke(json::value_from_tag, json::value& json_value, Office const& office);

void tag_invoke(json::value_from_tag, json::value& json_value, Building const& building);

void tag_invoke(json::value_from_tag, json::value& json_value, Road const& road);

void tag_invoke(json::value_from_tag, json::value& json_value, Map const& map);

void tag_invoke(json::value_from_tag, json::value& json_value, std::vector<Map> const& maps);

struct ResponseError {
    std::string code;
    std::string message;
};

void tag_invoke(json::value_from_tag, json::value& json_value, ResponseError const& err);

}  // namespace model
