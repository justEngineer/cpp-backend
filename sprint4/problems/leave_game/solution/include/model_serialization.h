#pragma once
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/serialization/vector.hpp>
#include <mutex>
#include "config.h"
#include "game.h"
#include "model.h"
#include "ticker.h"

namespace net = boost::asio;

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

}  // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, model::Speed& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.ux;
    ar& vec.uy;
}

template <typename Archive>
void serialize(Archive& ar, model::Position& pos, [[maybe_unused]] const unsigned version) {
    ar& pos.x;
    ar& pos.y;
}

template <typename Archive>
void serialize(Archive& ar, model::Item& obj, [[maybe_unused]] const unsigned version) {
    ar&(*obj.id_);
    ar&(obj.type_);
}

}  // namespace model

namespace serialization {

struct ItemObj {
    ItemObj() = default;

    explicit ItemObj(const model::Item& item)
        : id_{item.id_}, type_{item.type_}, position_{item.position_}, value_{item.value_} {};

    [[nodiscard]] model::Item Deserialize() const { return model::Item{id_, type_, position_, value_}; }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar& type_;
        ar& position_;
        ar& value_;
    }
    model::Item Restore() const { return model::Item(id_, type_, position_, value_); }

    model::Item::Id id_{0};
    uint32_t type_;
    model::Position position_;
    uint32_t value_;
};

// DogObj (DogObjesentation) - сериализованное представление класса Dog
class DogObj {
   public:
    DogObj() = default;

    explicit DogObj(const model::Dog& dog)
        : id_(dog.GetId()),
          name_(dog.GetName()),
          pos_(dog.GetPosition()),
          bag_capacity_(dog.GetBagCapacity()),
          speed_(dog.GetSpeed()),
          direction_(dog.GetDirection()),
          score_(dog.GetScore()) {
        std::transform(dog.GetBag().begin(), dog.GetBag().end(), std::back_inserter(bag_content_),
                       [](model::Item item) { return ItemObj{item}; });
    }

    model::Dog Restore() const {
        model::Dog dog{id_, name_, pos_};
        dog.SetSpeed(speed_);
        dog.SetDirection(direction_);
        dog.SetScore(score_);
        for (const auto& item : bag_content_) {
            if (!dog.TakeItem(item.Deserialize())) {
                throw std::runtime_error("Failed to put bag content");
            }
        }
        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar&* name_;
        ar& pos_;
        ar& bag_capacity_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_content_;
    }

    model::Dog::Id id_ = model::Dog::Id{0u};
    model::Dog::Name name_{""};
    model::Position pos_{0, 0};
    size_t bag_capacity_ = 0;
    model::Speed speed_;
    model::Dog::Direction direction_ = model::Dog::Direction::NORTH;
    int score_ = 0;
    std::vector<ItemObj> bag_content_;
};

struct PlayerObj {
    PlayerObj() = default;
    explicit PlayerObj(const model::Player& player)
        : id_(player.id_),
          name_(player.info_.name_),
          token_(player.token_),
          dog_(player.dog_),
          idle_time_(std::chrono::duration<double>(player.idle_time_).count()),
          play_time_(std::chrono::duration<double>(player.info_.play_time_).count()) {}
    uint64_t id_{0};
    std::string name_;
    std::string token_;
    DogObj dog_;
    double play_time_;
    double idle_time_;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar& id_;
        ar& name_;
        ar& token_;
        ar& dog_;
        ar& play_time_;
        ar& idle_time_;
    }
    static constexpr const double SECONDS_TO_MILLI = 1000.0;
    model::Player Restore() const {
        using namespace std::chrono;
        using namespace std::chrono_literals;
        return model::Player(id_, name_, token_, std::chrono::milliseconds(uint64_t(play_time_ * SECONDS_TO_MILLI)),
                             std::chrono::milliseconds(uint64_t(idle_time_ * SECONDS_TO_MILLI)), dog_.Restore());
    }
};

struct PlayersObj {
    PlayersObj() = default;
    std::vector<PlayerObj> players;
    std::vector<std::string> names;
    std::vector<size_t> indexes;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar& players;
        ar& names;
        ar& indexes;
    }
};

class GameSessionObj {
   public:
    GameSessionObj() = default;

    explicit GameSessionObj(model::GameSession& session) {
        for (const auto& player : session.GetAllPlayers()) {
            players_.emplace_back(player);
        }
        for (const auto& [_, item] : session.GetItems()) {
            items_.emplace_back(item);
        }
        map_id_ = session.GetMap().GetId();
    }

    const model::Map::Id& GetMapId() const { return map_id_; }
    const std::vector<PlayerObj>& GetPlayers() const { return players_; }
    const std::vector<ItemObj>& GetItems() const { return items_; }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& players_;
        ar& items_;
        ar&* map_id_;
    }

    std::vector<PlayerObj> players_;
    std::vector<ItemObj> items_;
    model::Map::Id map_id_ = model::Map::Id{""};
};

class GameSerializer {
   public:
    using Clock = std::chrono::steady_clock;
    using milliseconds = std::chrono::milliseconds;
    using Strand = net::strand<net::io_context::executor_type>;

    GameSerializer(Strand& api_strand, model::Game& game, const app::Config& config);
    ~GameSerializer();

    void SerializeGameStateOnTimer(milliseconds deltaInMilliseconds);
    void SerializeGameStateToFile();
    void DeserializeGameStateFromFile();

   private:
    model::Game& game_;
    const std::filesystem::path path_;
    std::shared_ptr<util::Ticker> timer_;
    std::mutex mtx_;
    const bool is_state_path_exists_{false};
    bool is_store_period_exists_{false};
    milliseconds store_period_;
    milliseconds current_time_;
};

}  // namespace serialization
