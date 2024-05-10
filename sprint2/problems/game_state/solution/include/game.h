#pragma once
// #include <boost/uuid/uuid.hpp>             // uuid class
// #include <boost/uuid/uuid_generators.hpp>  // generators
// #include <boost/uuid/uuid_io.hpp>          // streaming operators etc.
#include <string>
#include <unordered_map>
#include <vector>
#include "model.h"
#include "player_tokens.h"
#include "tagged.h"

namespace model {

class GameSession {
   public:
    GameSession(const std::string& map_id) : map_(map_id) {}
    //	void AddDog(const Dog& dog);
    const Player& AddPlayer(const std::string player_name);
    const std::string& GetMap() { return map_; }
    bool isTokenAlreadyExists(const std::string& auth_token);
    const std::vector<Player>* GetAllPlayers();

   private:
    std::vector<Dog> dogs_;
    std::vector<Player> players_;
    std::string map_;
    uint64_t current_uid_{0};
    //boost::uuids::random_generator uid_generator_;
    //PlayerTokens token_generator_;
};

class Game {
   public:
    using Maps = std::vector<Map>;

    void AddMap(Map&& map);

    const Maps& GetMaps() const noexcept { return maps_; }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }
    std::pair<std::string, const Player*> AddPlayer(AddNewPlayerRequest player);
    GameSession* GetSession(const Map::Id& id);
    const std::vector<Player>* GetAllPlayersInSession(const std::string& token);

   private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;

    std::vector<GameSession*> sessions_;
    MapIdToIndex map_id_to_session_index_;

    std::vector<Player> players_;
    //PlayerTokens player_tokens_;
};

}  // namespace model
