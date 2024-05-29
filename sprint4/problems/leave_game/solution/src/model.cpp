#include "model.h"
#include <limits>
#include <stdexcept>
#include "game.h"
#include "logger.h"

namespace json = boost::json;

namespace {
using namespace model;
using StartEndCoord = std::pair<double, double>;

bool isPointOnRoad(Position pos, const Road& road) {
    auto ox = road.GetOxProjection();
    auto oy = road.GetOyProjection();
    return pos.x >= ox.first && pos.x <= ox.second && pos.y >= oy.first && pos.y <= oy.second;
}

std::pair<Position, DynamicCoord> GetMoveOnRoad(Position start_pos, Position end_pos, const Road& road) {
    StartEndCoord oxMove = {std::min(start_pos.x, end_pos.x), std::max(start_pos.x, end_pos.x)};
    StartEndCoord oyMove = {std::min(start_pos.y, end_pos.y), std::max(start_pos.y, end_pos.y)};

    DynamicCoord start, end;
    StartEndCoord roadProjection;

    bool isHorizontal = oxMove.second - oxMove.first > oyMove.second - oyMove.first;
    // Уходим на прямую
    if (isHorizontal) {  // Перемещение по X
        start = start_pos.x;
        end = end_pos.x;
        roadProjection = road.GetOxProjection();
    } else {  // Перемещение по Y
        start = start_pos.y;
        end = end_pos.y;
        roadProjection = road.GetOyProjection();
    }

    DynamicCoord intersect;
    if (start < end) {  // Движение вправо
        intersect = std::min(end, roadProjection.second);
    } else {  // Движение влево
        intersect = std::max(end, roadProjection.first);
    }

    DynamicCoord len = std::fabs(intersect - start);

    // Начальная точка уже точно на дороге, значит пересечение проверять не нужно

    if (isHorizontal) {  // Перемещение по X
        return {{intersect, start_pos.y}, len};
    } else {
        return {{start_pos.x, intersect}, len};
    }
}
}  // namespace

namespace model {

Position operator*(Speed speed, double dt) {
    return Position{speed.ux * dt, speed.uy * dt};
}

Position operator+(Position a, Position b) {
    return Position{a.x + b.x, a.y + b.y};
}

bool operator==(Position a, Position b) {
    const DynamicDimension eps = 1.e-14;
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) < std::numeric_limits<double>::epsilon();
}

bool operator!=(Position a, Position b) {
    return !(a == b);
}

bool Dog::SetDirection(std::string_view direction, double speed) noexcept {
    if (direction.empty()) {
        speed_ = Speed{0.0, 0.0};
        return true;
    }
    auto res = std::find_if(
        DirectionToString.begin(), DirectionToString.end(),
        [direction](const std::pair<std::string_view, Direction>& pair) { return pair.first == direction; });
    if (res == DirectionToString.end()) {
        return false;
    }
    direction_ = res->second;
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

bool Dog::TakeItem(const model::Item& item) {
    if (bag_.size() == bag_capacity_) {
        return false;
    }
    bag_.push_back(item);
    return true;
}

Position Dog::Move(std::chrono::milliseconds time_delta, Map map, double speed) {
    SetDirection(GetDirectionAsString(), speed);
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
    auto seconds = std::chrono::duration<double>(time_delta).count();
    auto start_point = GetPosition();
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
    DynamicDimension result =
        (direction_ == Direction::WEST || direction_ == Direction::NORTH) ? -*maximums.rbegin() : *maximums.rbegin();
    Position res;
    if (isHorizontalMove) {
        res = {result, position_.y};
    } else {
        res = {position_.x, result};
    }
    if (res != move_destination_point)
        SetSpeed(Speed(0.0));

    position_ = res;
    return position_;
};

void Map::AddOffice(Office&& office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (std::exception& ec) {
        BOOST_LOG_TRIVIAL(warning) << "Error while Office deserialisation , error code: " << ec.what();
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

}  // namespace model
