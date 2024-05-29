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

const Player& GameSession::AddPlayer(const std::string name) {
    auto itFindPlayer =
        std::find_if(players_.begin(), players_.end(), [&name](const Player& player) { return player.name_ == name; });
    if (itFindPlayer != players_.end())
        return *itFindPlayer;
    PlayerTokens tk;
    auto token = tk.GetToken();
    return players_.emplace_back(player_uid_++, name, token, player_uid_, name, map_.GetSpawnPoint());
}

bool GameSession::isTokenAlreadyExists(const std::string& token) {
    auto itFindPlayer =
        std::find_if(players_.begin(), players_.end(), [&token](Player& player) { return player.token_ == token; });
    if (itFindPlayer != players_.end())
        return true;
    return false;
}

const std::vector<Player>& GameSession::GetAllPlayers() {
    return players_;
}

const ItemsHash& GameSession::GetAllItems() {
    return items_;
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
        const size_t index = sessions_.size();
        if (auto [it, inserted] = map_id_to_session_index_.emplace(map_id, index); !inserted) {
            return nullptr;
        } else {
            GameSession* new_session = new GameSession(*map, loot_generator_);
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
            auto& player = players_.at(event.gatherer_id_);
            if (player.dog_.GetBagSize() < map_.GetBagCapacity() && !collected_items.contains(collected_item_id)) {
                collected_items.insert(collected_item_id);
                player.dog_.TakeItem(item);
            }
        }
    }

    collision_detector::VectorOfficeSaveProvider office_provider(offices, dogs);
    auto item_delivered_events = collision_detector::FindOfficeSaveEvents(office_provider);
    for (auto event : item_delivered_events) {
        auto& player = players_.at(event.gatherer_id_);
        player.dog_.CountAndSetScore();
    }
    for (auto item_id : collected_items) {
        items_.erase(item_id);
    }
}

void GameSession::GenerateItems(uint64_t deltaInMilliseconds) {
    auto new_items_count =
        loot_generator_.Generate(std::chrono::milliseconds{deltaInMilliseconds}, items_.size(), players_.size());
    for (decltype(new_items_count) i = 0; i < new_items_count; i++) {
        uint32_t rand_type = std::rand() % map_.GetLootTypesCount();
        items_.insert(
            {model::Item::Id(item_uid_), model::Item{model::Item::Id(item_uid_), rand_type, map_.GetSpawnPoint()}});
        ++item_uid_;
    }
}

void GameSession::ChangeTime(uint64_t deltaInMilliseconds) {
    dogs_collisions_.clear();
    double deltaInSeconds = static_cast<double>(deltaInMilliseconds) / 1000.0;
    for (auto& player : players_) {
        auto prev_position = player.dog_.GetPosition();
        auto current_position = player.dog_.Move(deltaInSeconds, map_);
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
            auto& player = session->AddPlayer(req.name);
            return {"", &player};
        } catch (std::invalid_argument& err) {
            return {"InvalidName", nullptr};
        }
    }
    return {"InvalidMap", nullptr};
}

const std::pair<const std::vector<Player>*, const ItemsHash*> Game::GetPlayersAndItems(const std::string& token) {
    auto itFindSession = std::find_if(sessions_.begin(), sessions_.end(),
                                      [&token](GameSession* session) { return session->isTokenAlreadyExists(token); });
    if (itFindSession == sessions_.end())
        return {nullptr, nullptr};
    auto& players = (*itFindSession)->GetAllPlayers();
    auto& items = (*itFindSession)->GetAllItems();
    return {&players, &items};
}

bool Game::FindAndMovePlayersDog(const std::string& token, std::string_view command) {
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
    return dog.SetDirection(command, map_speed);
}

void Game::ChangeTime(uint64_t deltaInMilliseconds) {
    if (serializer_) {
        serializer_->SerializeGameStateOnTimer(std::chrono::milliseconds(deltaInMilliseconds));
    }
    for (auto& session : sessions_) {
        session->ChangeTime(deltaInMilliseconds);
    }
}

}  // namespace model
