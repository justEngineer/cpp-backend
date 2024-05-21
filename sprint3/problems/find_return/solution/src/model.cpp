#include "model.h"
#include <stdexcept>
#include "game.h"

namespace json = boost::json;

namespace model {

Position operator*(Speed speed, double dt) {
    return Position{speed.ux * dt, speed.uy * dt};
}

Position operator+(Position a, Position b) {
    return Position{a.x + b.x, a.y + b.y};
}

bool operator==(Position a, Position b) {
    const DynamicDimension eps = 1.e-14;
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) < eps;
}

bool operator!=(Position a, Position b) {
    return !(a == b);
}

bool Dog::SetDirection(std::string_view direction, double speed) noexcept {
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
Position Dog::Move(double seconds, Map map) {
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
    DynamicDimension result =
        (direction_ == Direction::WEST || direction_ == Direction::NORTH) ? -*maximums.rbegin() : *maximums.rbegin();
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
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

}  // namespace model
