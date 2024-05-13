#pragma once
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "tagged.h"

#include <boost/json.hpp>
#include "boost/json/serialize.hpp"
#include "boost/json/value_from.hpp"

using namespace std::literals;
namespace model {

namespace {

template <typename T>
std::optional<std::pair<T, T>> GetIntersection(std::pair<T, T> a, std::pair<T, T> b) {
    bool reverse_move = false;
    if (a.first > a.second) {
        std::swap(a.first, a.second);
        reverse_move = true;
    }

    if (a.first > b.second || a.second < b.first) {
        return std::nullopt;
    }

    std::pair<T, T> res{max(a.first, b.first), min(a.second, b.second)};

    if (reverse_move) {
        std::swap(res.first, res.second);
    }
    return res;
}

template <typename T>
bool IsInInterval(T point, std::pair<T, T> interval) {
    return point >= interval.first && point <= interval.second;
}

template <typename T, typename U>
std::optional<T> GetMaxMoveOnSegment(T move_start, T move_end, U road_beg, U road_end) {
    // Стартуем вне дороги - такого быть не может
    if (move_start < road_beg || move_start > road_end) {
        return std::nullopt;
    }

    auto res_beg = std::max(move_start, road_beg);
    auto res_end = std::min(move_end, road_end);

    // Если полученный отрезок существует, возвращаем его
    if (res_beg <= res_end) {
        return res_end;
    }

    return std::nullopt;
}

}  // namespace

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
    double GetSpeed() const { return dogSpeed_; };

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

    Dog(Id id, Name name, Position position, double speed, Direction direction = Direction::NORTH) noexcept
        : id_{std::move(id)}, name_{std::move(name)}, position_{position}, direction_{direction} {
        //SetDirection(GetDirection(), speed);
    }

    const Id& GetId() const noexcept { return id_; }
    const Name& GetName() const noexcept { return name_; }
    const Position& GetPosition() const noexcept { return position_; }
    const Speed& GetSpeed() const noexcept { return speed_; }
    std::string_view GetDirection() const noexcept { return DirectionToString[direction_]; }
    bool SetDirection(std::string_view direction, double speed) noexcept {
        auto res = std::find(DirectionToString.begin(), DirectionToString.end(), direction);
        if (res == DirectionToString.end()) {
            return false;
        }
        uint16_t idx = std::distance(DirectionToString.begin(), res);
        direction_ = Direction(uint16_t(std::distance(DirectionToString.begin(), res)));
        switch (direction_) {
            case Direction::NORTH: {
                speed_.ux = 0.0;
                speed_.uy = -speed;
                break;
            }
            case Direction::SOUTH: {
                speed_.ux = 0.0;
                speed_.uy = speed;
                break;
            }
            case Direction::WEST: {
                speed_.ux = -speed;
                speed_.uy = 0.0;
                break;
            }
            case Direction::EAST: {
                speed_.ux = speed;
                speed_.uy = 0.0;
                break;
            }
        }
        return true;
    }
    void Move(double seconds, Map map) {
        auto NormalizeCoord = [](Direction direction_, Position pos) {
            switch (direction_) {
                case Direction::NORTH:
                    return -pos.y;
                case Direction::SOUTH:
                    return pos.y;
                case Direction::WEST:
                    return -pos.x;
                case Direction::EAST:
                    return pos.x;
            }
            return DynamicCoord{0.0};
        };
        auto move_destination_point = GetPosition() + GetSpeed() * seconds;
        bool isHorizontalMove = (direction_ == Direction::WEST || direction_ == Direction::EAST);
        std::set<DynamicDimension> maximums;
        for (const auto& road : map.GetRoads()) {
            auto ox_projection = road.GetOxProjection();
            auto oy_projection = road.GetOyProjection();
            StartEndCoord projection;
            if (isHorizontalMove) {
                if (road.IsHorizontal() && !IsInInterval(position_.y, oy_projection)) {
                    continue;
                }
                if (direction_ == Direction::WEST) {
                    projection = {-ox_projection.first, -ox_projection.second};
                } else {
                    projection = {ox_projection.first, ox_projection.second};
                }
                projection = {std::minmax(projection.first, projection.second)};
            } else {
                if (road.IsVertical() && !IsInInterval(position_.x, ox_projection)) {
                    continue;
                }
                if (direction_ == Direction::NORTH) {
                    projection = {-oy_projection.first, -oy_projection.second};
                } else {
                    projection = {oy_projection.first, oy_projection.second};
                }
                projection = {std::minmax(projection.first, projection.second)};
            }
            auto res = GetMaxMoveOnSegment(NormalizeCoord(direction_, position_),
                                           NormalizeCoord(direction_, move_destination_point), projection.first,
                                           projection.second);
            if (res) {
                maximums.insert(*res);
            }
        }
        DynamicDimension result = (direction_ == Direction::WEST || direction_ == Direction::NORTH)
                                      ? -*maximums.rbegin()
                                      : *maximums.rbegin();
        Position res;
        if (isHorizontalMove) {
            res = {result, position_.y};
        } else {
            res = {position_.x, result};
        }
        if (move_destination_point != res) {
            speed_.ux = 0.0;
            speed_.uy = 0.0;
        }
        position_ = res;
    };

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
           const std::string& dog_name, Position pos, double speed)
        : id_(id), name_(name), token_(token), dog_(Dog::Id(dog_id), Dog::Name(dog_name), pos, speed) {}
    const uint64_t id_{0};
    const std::string name_;
    const std::string token_;
    Dog dog_;
};

namespace json_config {
constexpr const char* MAPS = "maps";
constexpr const char* DEFAULT_DOG_SPEED = "defaultDogSpeed";
namespace map {
constexpr const char* ID = "id";
constexpr const char* NAME = "name";
constexpr const char* ROADS = "roads";
constexpr const char* BUILDINGS = "buildings";
constexpr const char* OFFICES = "offices";
constexpr const char* DOG_SPEED = "dogSpeed";
namespace road {
constexpr const char* START_X = "x0";
constexpr const char* START_Y = "y0";
constexpr const char* END_X = "x1";
constexpr const char* END_Y = "y1";
}  // namespace road
namespace building {
constexpr const char* POS_X = "x";
constexpr const char* POS_Y = "y";
constexpr const char* SIZE_W = "w";
constexpr const char* SIZE_H = "h";
}  // namespace building
namespace office {
constexpr const char* ID = "id";
constexpr const char* POS_X = "x";
constexpr const char* POS_Y = "y";
constexpr const char* OFFSET_X = "offsetX";
constexpr const char* OFFSET_Y = "offsetY";
}  // namespace office
}  // namespace map
}  // namespace json_config

namespace json_error {
constexpr const char* CODE = "code";
constexpr const char* MESSAGE = "message";
constexpr const char* EXCEPTION = "exception";
}  // namespace json_error

namespace error_code {
constexpr static char BAD_REQUEST[] = "badRequest";
constexpr static char INVALID_ARGUMENT[] = "invalidArgument";
constexpr static char INVALID_METHOD[] = "invalidMethod";
constexpr static char INVALID_TOKEN[] = "invalidToken";
constexpr static char UNKNOWN_TOKEN[] = "unknownToken";
constexpr static char MAP_NOT_FOUND[] = "mapNotFound";
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
}  // namespace json_get_state_response

namespace json_dog {
constexpr static char POSITION[] = "pos";
constexpr static char SPEED[] = "speed";
constexpr static char DIRECTION[] = "dir";
}  // namespace json_dog

Building tag_invoke(json::value_to_tag<Building>, json::value const& json_value);

Road tag_invoke(json::value_to_tag<Road>, json::value const& json_value);

Office tag_invoke(json::value_to_tag<Office>, json::value const& json_value);

Map tag_invoke(json::value_to_tag<Map>, json::value const& json_value);

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

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Player const& result);
void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, std::vector<Player> const& players);

template <typename T>
struct GetStateWrapper {
    T const& players_;
    GetStateWrapper(T const& players) : players_(players) {}
};

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv,
                GetStateWrapper<const std::vector<Player>> const& players);

struct EmptyObject {};

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, EmptyObject const& dummy);

}  // namespace model
