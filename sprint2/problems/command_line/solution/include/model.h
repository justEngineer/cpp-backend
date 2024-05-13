#pragma once
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "json_templates.h"
#include "tagged.h"

#include <boost/json.hpp>
#include "boost/json/serialize.hpp"
#include "boost/json/value_from.hpp"

using namespace std::literals;
namespace model {

template <typename T>
bool IsInInterval(T point, std::pair<T, T> interval) {
    return point >= interval.first && point <= interval.second;
}

template <typename T, typename U>
std::optional<T> GetMaxMoveOnSegment(T move_start, T move_end, U road_beg, U road_end) {
    if (move_start < road_beg || move_start > road_end) {
        return std::nullopt;
    }
    auto res_beg = std::max(move_start, road_beg);
    auto res_end = std::min(move_end, road_end);
    if (res_beg <= res_end) {
        return res_end;
    }
    return std::nullopt;
}

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

using DynamicDimension = double;
using DynamicCoord = DynamicDimension;

struct Position {
    DynamicCoord x, y;
};

struct Speed {
    DynamicDimension ux, uy;
};
using StartEndCoord = std::pair<DynamicDimension, DynamicDimension>;

Position operator*(Speed speed, double dt);

Position operator+(Position a, Position b);

bool operator==(Position a, Position b);

bool operator!=(Position a, Position b);

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

    Road(HorizontalTag, Point start, Coord end_x) noexcept : start_{start}, end_{end_x, start.y} {}

    Road(VerticalTag, Point start, Coord end_y) noexcept : start_{start}, end_{start.x, end_y} {}

    bool IsHorizontal() const noexcept { return start_.y == end_.y; }

    bool IsVertical() const noexcept { return start_.x == end_.x; }

    Point GetStart() const noexcept { return start_; }

    Point GetEnd() const noexcept { return end_; }

    StartEndCoord GetOxProjection() const noexcept {
        return {std::min(start_.x, end_.x) - d_width, std::max(start_.x, end_.x) + d_width};
    }

    StartEndCoord GetOyProjection() const noexcept {
        return {std::min(start_.y, end_.y) - d_width, std::max(start_.y, end_.y) + d_width};
    }

   private:
    // Половина ширины дороги или расстояние, на которое можно отойти от оси дороги
    static constexpr DynamicDimension d_width = 0.4;
    Point start_;
    Point end_;
};

class Building {
   public:
    explicit Building(Rectangle bounds) noexcept : bounds_{bounds} {}

    const Rectangle& GetBounds() const noexcept { return bounds_; }

   private:
    Rectangle bounds_;
};

class Office {
   public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept : id_{std::move(id)}, position_{position}, offset_{offset} {}

    const Id& GetId() const noexcept { return id_; }

    Point GetPosition() const noexcept { return position_; }

    Offset GetOffset() const noexcept { return offset_; }

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

    Map(Id id, std::string name, double dogSpeed) noexcept
        : id_(std::move(id)), name_(std::move(name)), dogSpeed_(dogSpeed) {}

    const Id& GetId() const noexcept { return id_; }
    const std::string& GetName() const noexcept { return name_; }
    const Buildings& GetBuildings() const noexcept { return buildings_; }
    const Roads& GetRoads() const noexcept { return roads_; }
    const Offices& GetOffices() const noexcept { return offices_; }
    void AddRoad(Road&& road) { roads_.push_back(std::move(road)); }
    void AddBuilding(Building&& building) { buildings_.push_back(std::move(building)); }
    void AddOffice(Office&& office);
    double GetSpeed() const { return dogSpeed_; }
    Position GetSpawnPoint() const {
        size_t road_index = 0;
        auto& road = roads_[road_index];
        model::Position res;
        res.x = road.GetStart().x;
        res.y = road.GetEnd().y;
        return res;
    }

   private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    double dogSpeed_{0.0};
};

class Dog {

    enum Direction : uint16_t { NORTH = 0, SOUTH = 1, WEST = 2, EAST = 3 };

   public:
    using Id = util::Tagged<std::uint32_t, Dog>;
    using Name = util::Tagged<std::string, Dog>;

    Dog(Id id, Name name, Position position, Direction direction = Direction::NORTH) noexcept
        : id_{std::move(id)}, name_{std::move(name)}, position_{position}, direction_{direction} {}

    const Id& GetId() const noexcept { return id_; }
    const Name& GetName() const noexcept { return name_; }
    const Position& GetPosition() const noexcept { return position_; }
    const Speed& GetSpeed() const noexcept { return speed_; }
    std::string_view GetDirection() const noexcept { return DirectionToString[direction_]; }
    bool SetDirection(std::string_view direction, double speed) noexcept;
    void Move(double seconds, Map map);

   private:
    constexpr static std::array<std::string_view, 4> DirectionToString{"U"sv, "D"sv, "L"sv, "R"sv};
    Id id_;
    Name name_;
    Position position_;
    Speed speed_{0.0, 0.0};
    Direction direction_;
};

struct AddNewPlayerRequest {
    std::string name;
    std::string map_id;
};

class Player {
   public:
    Player(uint64_t id, const std::string& name, const std::string& token, const uint64_t dog_id,
           const std::string& dog_name, Position pos)
        : id_(id), name_(name), token_(token), dog_(Dog::Id(dog_id), Dog::Name(dog_name), pos) {}
    const uint64_t id_{0};
    const std::string name_;
    const std::string token_;
    Dog dog_;
};

}  // namespace model
