#include "game.h"
#include <algorithm>
#include <stdexcept>
#include "game.h"
#include "player_tokens.h"
#include "request_handler.h"
//#include "model.h"

namespace model {

using namespace std::literals;

/*
void GameSession::AddDog(const model::Dog& dog) 
{ 
	dogs_.push_back(dog);
}
*/
const Player& GameSession::AddPlayer(const std::string name) {
    auto itFind =
        std::find_if(players_.begin(), players_.end(), [&name](const Player& player) { return player.name_ == name; });
    if (itFind != players_.end())
        return *itFind;
    PlayerTokens tk;
    auto token = tk.GetToken();
    return players_.emplace_back(current_uid_++, name, token);
}

bool GameSession::isTokenAlreadyExists(const std::string& token) {
    auto itFind =
        std::find_if(players_.begin(), players_.end(), [&token](Player& player) { return player.token_ == token; });
    if (itFind != players_.end())
        return true;
    return false;
}

const std::vector<Player>* GameSession::GetAllPlayers() {
    return &players_;
}

void Game::AddMap(Map&& map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.push_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

GameSession* Game::GetSession(const Map::Id& map_id) {
    // В данной реализации одна карта (MapId) соответсвует одной сессии
    // При необходимости можно будет переделать, чтобы на одной карте было несколько сессий.
    // Для этого нужно будет добавить критерий "переполнения" сессии и создания новой, например, по количеству
    // игроков в текущей найденной сессии
    if (auto it = map_id_to_session_index_.find(map_id); it != map_id_to_session_index_.end()) {
        return sessions_.at(it->second);
    } else {
        // Если не нашли сессию для игрока, пробуем создать новую

        //  Найдём карту, к которой хотим подключиться
        auto map = FindMap(map_id);
        if (!map) {
            return nullptr;
            //throw std::invalid_argument("Map with id "s + *map_id + " isn`t exists"s);
        }

        // Карта есть. Пробуем добавить новую сессию
        const size_t index = sessions_.size();
        if (auto [it, inserted] = map_id_to_session_index_.emplace(map_id, index); !inserted) {
            // Сессия есть для карты с таким Id. Заново создавать нельзя!
            throw std::invalid_argument("Session for map with id "s + *map_id + " already exists"s);
        } else {
            // Создаём новую сессию, привязанную к указанной карте
            GameSession* new_session = new GameSession(*map->GetId());
            try {
                sessions_.emplace_back(new_session);
            } catch (...) {
                // Не получилось. Откатываем изменения в map_id_to_map_index_
                map_id_to_session_index_.erase(it);
                delete new_session;
                //throw;
                return nullptr;
            }
        }
        return sessions_.back();
    }
}

std::pair<std::string, const Player*> Game::AddPlayer(AddNewPlayerRequest req) {
    // model::Dog::Name name_str(*name);
    // if (!isValidName(name)) {
    //     throw JoinGameError{JoinGameErrorReason::InvalidName};
    // }
    if (!FindMap(model::Map::Id{req.map_id})) {
        return {"InvalidMap", nullptr};
    }
    if (auto session = GetSession(model::Map::Id{req.map_id})) {
        //auto spawn_point = GetRandomPointOnMap();
        try {
            //auto dog = session->AddDog(spawn_point, std::move(name_str));
            //auto& player = players_.Add(/*dog,*/ *session);
            auto& player = session->AddPlayer(req.name);
            return {"", &player};
        } catch (std::invalid_argument err) {
            return {"InvalidName", nullptr};
        }
    }
    return {"InvalidMap", nullptr};
}

const std::vector<Player>* Game::GetAllPlayersInSession(const std::string& token) {
    auto itFind = std::find_if(sessions_.begin(), sessions_.end(),
                               [&token](GameSession* session) { return session->isTokenAlreadyExists(token) == true; });
    if (itFind == sessions_.end())
        return nullptr;
    auto players = (*itFind)->GetAllPlayers();
    return players;
}

}  // namespace model
