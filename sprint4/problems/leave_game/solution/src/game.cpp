#include "game.h"
#include <algorithm>
#include <cmath>
#include <ranges>
#include <stdexcept>
#include "model_serialization.h"
#include "player_tokens.h"
#include "request_handler.h"

namespace model {

using namespace std::literals;

const Player& GameSession::AddPlayer(const std::string name, double default_dog_speed) {
    auto itFindPlayer = std::find_if(players_.begin(), players_.end(),
                                     [&name](const Player& player) { return player.info_.name_ == name; });
    if (itFindPlayer != players_.end())
        return *itFindPlayer;
    auto map_speed = GetMap().GetSpeed();
    if (map_speed == 0.0) {
        map_speed = default_dog_speed;
    }
    PlayerTokens tk;
    auto token = tk.GetToken();
    return players_.emplace_back(player_uid_++, name, token, player_uid_, name, map_.GetSpawnPoint());
}

bool GameSession::isTokenAlreadyExists(const std::string_view token) {
    auto itFindPlayer =
        std::find_if(players_.begin(), players_.end(), [&token](Player& player) { return player.token_ == token; });
    if (itFindPlayer != players_.end())
        return true;
    return false;
}

std::list<Player>& GameSession::GetAllPlayers() {
    return players_;
}

const ItemsHash& GameSession::GetAllItems() {
    return items_;
}

std::vector<collision_detector::Item>& GameSession::GetItemsCollision() {
    items_collision_.clear();
    for (const auto& [item_id, item] : items_) {
        items_collision_.emplace_back(
            collision_detector::Item{.position_ = {item.position_.x, item.position_.y}, .width_ = item.width_});
    }
    return items_collision_;
}

void Game::AddMap(Map&& map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.push_back(std::move(map));
        } catch (std::exception& err) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

GameSession* Game::GetSession(const Map::Id& map_id) {
    if (auto it = map_id_to_session_index_.find(map_id); it != map_id_to_session_index_.end()) {
        return sessions_.at(it->second);
    } else {
        auto map = FindMap(map_id);
        if (!map) {
            return nullptr;
        }
        auto map_speed = map->GetSpeed();
        if (map_speed == 0.0) {
            map_speed = defaultDogSpeed_;
        }
        const size_t index = sessions_.size();
        if (auto [it, inserted] = map_id_to_session_index_.emplace(map_id, index); !inserted) {
            return nullptr;
        } else {
            GameSession* new_session = new GameSession(*map, loot_generator_, map_speed);
            try {
                sessions_.emplace_back(new_session);
            } catch (std::exception& err) {
                map_id_to_session_index_.erase(it);
                delete new_session;
                return nullptr;
            }
        }
        return sessions_.back();
    }
}

void GameSession::HandleEvents(std::vector<collision_detector::Gatherer>& dogs,
                               std::vector<collision_detector::Item>& items,
                               const std::vector<collision_detector::Rectangle>& offices) {
    collision_detector::VectorItemGathererProvider item_provider(items, dogs);
    auto gather_events = collision_detector::FindGatherEvents(item_provider);
    std::set<Item::Id> collected_items;
    for (auto event : gather_events) {
        auto collected_item_id = Item::Id(event.item_id_);
        if (auto search = items_.find(collected_item_id); search != items_.end()) {
            auto& item = search->second;
            // auto itFindPlayer = std::find_if(players_.begin(), players_.end(),
            //                                  [&name](const Player& player) { return player.info_.name_ == name; });
            // if (itFindPlayer != players_.end())
            auto itFindPlayer = players_.begin();
            std::advance(itFindPlayer, event.gatherer_id_);
            // auto& player = *(std::advance(players_.begin(), event.gatherer_id_));
            auto& player = *itFindPlayer;
            if (player.dog_.GetBagSize() < map_.GetBagCapacity() && !collected_items.contains(collected_item_id)) {
                collected_items.insert(collected_item_id);
                player.dog_.TakeItem(item);
            }
        }
    }

    collision_detector::VectorOfficeSaveProvider office_provider(offices, dogs);
    auto item_delivered_events = collision_detector::FindOfficeSaveEvents(office_provider);
    for (auto event : item_delivered_events) {
        auto itFindPlayer = players_.begin();
        std::advance(itFindPlayer, event.gatherer_id_);
        auto& player = *itFindPlayer;
        player.CountAndSetScore();
    }
    for (auto item_id : collected_items) {
        items_.erase(item_id);
    }
}

void GameSession::GenerateItems(std::chrono::milliseconds deltaInMilliseconds) {
    auto new_items_count = loot_generator_.Generate(deltaInMilliseconds, items_.size(), players_.size());
    for (decltype(new_items_count) i = 0; i < new_items_count; i++) {
        uint32_t rand_type = std::rand() % map_.GetLootTypesCount();
        items_.insert(
            {model::Item::Id(item_uid_), model::Item{model::Item::Id(item_uid_), rand_type, map_.GetSpawnPoint()}});
        ++item_uid_;
    }
}

void GameSession::ChangeTime(std::chrono::milliseconds deltaInMilliseconds) {
    dogs_collisions_.clear();
    for (auto& player : players_) {
        auto prev_position = player.dog_.GetPosition();
        auto current_position = player.Move(deltaInMilliseconds, map_, dog_speed_);
        dogs_collisions_.push_back(
            {{prev_position.x, prev_position.y}, {current_position.x, current_position.y}, player.dog_.GetWidth()});
    }
    HandleEvents(dogs_collisions_, GetItemsCollision(), map_.GetOfficesCollisions());
    GenerateItems(deltaInMilliseconds);
}

std::pair<std::string, const Player*> Game::AddPlayer(AddNewPlayerRequest req) {
    if (!FindMap(model::Map::Id{req.map_id})) {
        return {"InvalidMap", nullptr};
    }
    if (auto session = GetSession(model::Map::Id{req.map_id})) {
        try {
            auto& player = session->AddPlayer(req.name, defaultDogSpeed_);
            return {"", &player};
        } catch (std::invalid_argument& err) {
            return {"InvalidName", nullptr};
        }
    }
    return {"InvalidMap", nullptr};
}

const std::pair<const std::list<Player>*, const ItemsHash*> Game::GetPlayersAndItems(const std::string_view token) {
    auto itFindSession = std::find_if(sessions_.begin(), sessions_.end(),
                                      [&token](GameSession* session) { return session->isTokenAlreadyExists(token); });
    if (itFindSession == sessions_.end())
        return {nullptr, nullptr};
    auto& players = (*itFindSession)->GetAllPlayers();
    auto& items = (*itFindSession)->GetAllItems();
    return {&players, &items};
}

bool Game::FindAndMovePlayersDog(const std::string_view token, std::string_view command) {
    auto itFindSession = std::find_if(sessions_.begin(), sessions_.end(), [&token](GameSession* session) {
        return session->isTokenAlreadyExists(token) == true;
    });
    if (itFindSession == sessions_.end()) {
        return false;
    }
    auto& session = **itFindSession;
    auto& players = session.GetAllPlayers();
    auto itFindPlayer = std::find_if(players.begin(), players.end(),
                                     [&token](const model::Player& player) { return player.token_ == token; });
    if (itFindPlayer == players.end()) {
        return false;
    }
    auto& dog = const_cast<model::Dog&>(itFindPlayer->dog_);
    auto map_speed = session.GetMap().GetSpeed();
    if (map_speed == 0.0) {
        map_speed = defaultDogSpeed_;
    }
    if (!command.empty()) {
        itFindPlayer->idle_time_ = std::chrono::milliseconds(0);
    }
    return dog.SetDirection(command, map_speed);
}

void Game::ChangeTime(std::chrono::milliseconds deltaInMilliseconds) {
    if (serializer_) {
        serializer_->SerializeGameStateOnTimer(deltaInMilliseconds);
    }
    for (auto& session : sessions_) {
        session->ChangeTime(deltaInMilliseconds);
    }
    CheckExpiredPlayers();
}

void Game::CheckExpiredPlayers() {
    for (auto& session : sessions_) {
        auto& players = const_cast<std::list<model::Player>&>(session->GetAllPlayers());
        for (auto playerIt = players.begin(); playerIt != players.end();) {
            if (playerIt->idle_time_ >= player_inactive_timeout_) {
                SavePlayerDataToDB(*playerIt);
                playerIt = players.erase(playerIt);
            } else {
                playerIt++;
            }
        }
    }
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

void Game::SavePlayerDataToDB(const model::Player& player) {
    db_.Save(player.info_);
}

std::vector<model::Player::PlayerInfo> Game::GetInfoFromDB(uint64_t start, uint64_t limit) {
    return db_.GetRecords(start, limit);
}

Position Map::GetSpawnPoint() const {
    auto generate_random = [](double min, double max) {
        std::random_device rd;  // used to generate a seed
        std::mt19937 generator(rd());
        double nextafter_max = std::nextafter(max, std::numeric_limits<decltype(max)>::max());
        std::uniform_real_distribution<> distr(min, max);
        return std::round(distr(generator));
    };
    size_t road_index = 0;
    if (roads_.size() > 1) {
        std::random_device rd;                                          // obtain a random number from hardware
        std::mt19937 gen(rd());                                         // seed the generator
        std::uniform_int_distribution<> distr(0, (roads_.size() - 1));  // define the range
        road_index = distr(gen);
    }
    auto& road = roads_[road_index];
    model::Position res;
    model::Point start = road.GetStart();
    model::Point end = road.GetEnd();
    res.x = start.x;
    res.y = start.y;
    if (road.IsHorizontal()) {
        res.y = start.y;
        res.x = generate_random(start.x, end.x);
    } else {
        res.x = start.x;
        res.y = generate_random(start.y, end.y);
    }
    return res;
}

const std::vector<collision_detector::Rectangle>& Map::GetOfficesCollisions() const {
    if (offices_collisions_.empty()) {
        for (auto office : offices_) {
            offices_collisions_.emplace_back(office.GetPosition().x, office.GetPosition().y, office.GetOffset().dx,
                                             office.GetOffset().dy);
        }
    }
    return offices_collisions_;
};

Position Player::Move(std::chrono::milliseconds time_delta, Map map, double speed) {
    info_.play_time_ += time_delta;
    if (dog_.GetSpeed() == Speed{0.0, 0.0}) {
        idle_time_ += time_delta;
        return dog_.GetPosition();
    } else {
        idle_time_ = std::chrono::milliseconds(0);
    }
    return dog_.Move(time_delta, map, speed);
}

void Player::CountAndSetScore() {
    for (const auto& item : dog_.GetBag()) {
        info_.score_ += item.value_;
    }
    dog_.SetScore(info_.score_);
    dog_.ClearBag();
}

}  // namespace model
