#include "game.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
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
    return players_.emplace_back(current_uid_++, name, token, current_uid_, name, map_.GetSpawnPoint());
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

const std::vector<Item>& GameSession::GetAllItems() {
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

void GameSession::ChangeTime(double deltaInSeconds) {
    for (auto& player : players_) {
        player.dog_.Move(deltaInSeconds, map_);
    }
    auto new_items_count = loot_generator_.Generate(std::chrono::milliseconds{int(std::round(deltaInSeconds * 1000))},
                                                    item_id_to_index_.size(), players_.size());
    for (decltype(new_items_count) i = 0; i < new_items_count; i++) {
        auto rand_type = std::rand() % map_.GetLootTypesCount();
        items_.emplace_back(model::Item::Id(i), rand_type, map_.GetSpawnPoint());
    }
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

const std::pair<const std::vector<Player>*, const std::vector<Item>*> Game::GetPlayersAndItems(
    const std::string& token) {
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
    double seconds = static_cast<double>(deltaInMilliseconds) / 1000.0;
    for (auto& session : sessions_) {
        session->ChangeTime(seconds);
    }
}

}  // namespace model
