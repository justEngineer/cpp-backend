#pragma once
#include <chrono>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "collision_detector.h"
#include "json_templates.h"
#include "loot_generator.h"
#include "tagged.h"

#include <boost/json.hpp>
#include "boost/json/serialize.hpp"
#include "boost/json/value_from.hpp"

namespace {
constexpr uint32_t SEC_TO_MILLISEC = 1000;
}

using namespace std::literals;
namespace model {

struct LootGeneratorCfg {
    LootGeneratorCfg(const double period_in_seconds, const double probability)
        : period_in_seconds_(period_in_seconds), probability_(probability) {}
    std::chrono::milliseconds GetPeriodInMilliseconds() const noexcept {
        return std::chrono::milliseconds{int(std::round(period_in_seconds_ * SEC_TO_MILLISEC))};
    }
    const double period_in_seconds_;
    const double probability_;
};

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
    auto operator<=>(const Speed&) const = default;
};

using StartEndCoord = std::pair<DynamicDimension, DynamicDimension>;

Position operator*(Speed speed, double dt);

Position operator+(Position a, Position b);

bool operator==(Position a, Position b);

bool operator!=(Position a, Position b);

class Road {
    struct HorizontalTag {
        HorizontalTag() = default;
    };

    struct VerticalTag {
        VerticalTag() = default;
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

    Map(Id id, std::string name, double dogSpeed, json::array loot_types, uint32_t bag_capacity) noexcept
        : id_(std::move(id)),
          name_(std::move(name)),
          dog_speed_(dogSpeed),
          loot_types_(loot_types),
          bag_capacity_(bag_capacity) {}

    const Id& GetId() const noexcept { return id_; }
    const std::string& GetName() const noexcept { return name_; }
    const Buildings& GetBuildings() const noexcept { return buildings_; }
    const Roads& GetRoads() const noexcept { return roads_; }
    const Offices& GetOffices() const noexcept { return offices_; }
    void AddRoad(Road&& road) { roads_.push_back(std::move(road)); }
    void AddBuilding(Building&& building) { buildings_.push_back(std::move(building)); }
    void AddOffice(Office&& office);
    uint32_t GetBagCapacity() const { return bag_capacity_; }
    const std::vector<collision_detector::Rectangle>& GetOfficesCollisions() const;
    double GetSpeed() const { return dog_speed_; }
    const json::array& GetLootTypes() const { return loot_types_; }
    size_t GetLootTypesCount() const { return loot_types_.size() == 0 ? 1 : loot_types_.size(); }
    Position GetSpawnPoint() const;

   private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    double dog_speed_{0.0};
    json::array loot_types_;
    uint32_t bag_capacity_{0};
    mutable std::vector<collision_detector::Rectangle> offices_collisions_;
};

struct Item {
    using Id = util::Tagged<std::uint32_t, Item>;

    Item(Id id, uint32_t type, Position position, uint32_t value = 10) noexcept
        : id_(std::move(id)), type_(type), position_(position), value_(value) {}
    const std::string GetIdAsString() const noexcept { return std::to_string(*id_); }

    const Id id_;
    const uint32_t type_;
    const Position position_;
    const uint32_t value_;

    static constexpr double width_ = 0.0;
};

using ItemIdHasher = util::TaggedHasher<Item::Id>;
using ItemsHash = std::unordered_map<Item::Id, Item, ItemIdHasher>;

class Dog {

   public:
    using Id = util::Tagged<std::uint32_t, Dog>;
    using Name = util::Tagged<std::string, Dog>;
    enum Direction : uint16_t { NORTH = 0, SOUTH = 1, WEST = 2, EAST = 3 };

    constexpr static std::array<std::pair<std::string_view, Direction>, 4> DirectionToString{
        std::make_pair<std::string_view, Direction>("U"sv, Direction::NORTH),
        std::make_pair<std::string_view, Direction>("D"sv, Direction::SOUTH),
        std::make_pair<std::string_view, Direction>("L"sv, Direction::WEST),
        std::make_pair<std::string_view, Direction>("R"sv, Direction::EAST)};

    Dog(Id id, Name name, Position position, Direction direction = Direction::NORTH) noexcept
        : id_{std::move(id)}, name_{std::move(name)}, position_{position}, direction_{direction} {}

    Dog(const Dog& dog) noexcept
        : id_{dog.id_}, name_{dog.name_}, position_{dog.position_}, direction_{dog.direction_} {
        std::move(dog.bag_.begin(), dog.bag_.end(), std::back_inserter(bag_));
    }

    const Id& GetId() const noexcept { return id_; }
    const Name& GetName() const noexcept { return name_; }
    const Position& GetPosition() const noexcept { return position_; }
    const Speed& GetSpeed() const noexcept { return speed_; }
    void SetSpeed(Speed speed) noexcept { speed_ = speed; }
    double GetWidth() const noexcept { return width_; }
    const std::vector<model::Item>& GetBag() const { return bag_; }
    size_t GetBagSize() const noexcept { return bag_.size(); }
    void ClearBag() noexcept { return bag_.clear(); }
    size_t GetBagCapacity() const noexcept { return bag_capacity_; }
    bool TakeItem(const model::Item& item);
    int GetScore() const noexcept { return score_; }
    void SetScore(int score) noexcept { score_ = score; }
    std::string_view GetDirectionAsString() const noexcept { return DirectionToString[direction_].first; }
    Direction GetDirection() const noexcept { return direction_; }
    bool SetDirection(std::string_view direction, double speed) noexcept;
    void SetDirection(Direction direction) noexcept { direction_ = direction; };
    void AddScore(int score) noexcept { score_ += score; }
    Position Move(std::chrono::milliseconds time_delta, Map map, double speed);

   private:
    static constexpr double width_ = 0.6;
    Id id_;
    Name name_;
    Position position_;
    Speed speed_{0.0, 0.0};
    Direction direction_;
    std::vector<model::Item> bag_;
    size_t bag_capacity_{10};
    int score_{0};
};

struct AddNewPlayerRequest {
    std::string name;
    std::string map_id;
};

class Player {
   public:
    struct PlayerInfo {
        std::string name_;
        int score_{0};
        std::chrono::milliseconds play_time_{0};
    };
    explicit Player(uint64_t id, const std::string& name, const std::string& token, const uint64_t dog_id,
                    const std::string& dog_name, Position pos)
        : id_(id), token_(token), dog_(Dog::Id(dog_id), Dog::Name(dog_name), pos) {
        info_ = {name, 0, std::chrono::milliseconds(0)};
    }
    explicit Player(uint64_t id, const std::string& name, const std::string& token, std::chrono::milliseconds play_time,
                    std::chrono::milliseconds idle_time, Dog dog)
        : id_(id), token_(token), dog_(dog), idle_time_(idle_time) {
        info_ = {name, dog.GetScore(), play_time};
    }
    explicit Player(const Player& player) : id_(player.id_), token_(player.token_), dog_(player.dog_) {
        info_ = {player.info_.name_, player.dog_.GetScore(), player.info_.play_time_};
    }
    Player& operator=(const Player& other) {
        info_ = {other.info_.name_, other.dog_.GetScore(), other.info_.play_time_};
        return *this;
    }
    Position Move(std::chrono::milliseconds time_delta, Map map, double speed);
    std::chrono::milliseconds GetIdleTime() const { return idle_time_; }
    std::chrono::milliseconds GetPlayTime() const { return info_.play_time_; }
    void CountAndSetScore();
    uint64_t id_{0};
    std::string token_;
    Dog dog_;
    PlayerInfo info_;
    std::chrono::milliseconds idle_time_{std::chrono::milliseconds(0)};
};

}  // namespace model
