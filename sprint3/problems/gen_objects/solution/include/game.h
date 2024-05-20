#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "model.h"
#include "tagged.h"

namespace model {

class GameSession {
   public:
    explicit GameSession(const Map& map, loot_gen::LootGenerator& loot_gen) : map_(map), loot_generator_(loot_gen) {}
    const Player& AddPlayer(const std::string player_name);
    const Map& GetMap() { return map_; }
    bool isTokenAlreadyExists(const std::string& auth_token);
    const std::vector<Player>& GetAllPlayers();
    const std::vector<Item>& GetAllItems();
    void ChangeTime(double deltaInSeconds);

   private:
    using ItemIdHasher = util::TaggedHasher<Item::Id>;
    using ItemIdToIndex = std::unordered_map<Item::Id, size_t, ItemIdHasher>;
    std::vector<Player> players_;
    const Map& map_;
    uint64_t current_uid_{0};
    loot_gen::LootGenerator& loot_generator_;
    ItemIdToIndex item_id_to_index_;
    std::vector<Item> items_;
};

class Game {
   public:
    using Maps = std::vector<Map>;
    explicit Game(double defaultDogSpeed, LootGeneratorCfg&& loot_gen_cfg)
        : defaultDogSpeed_(defaultDogSpeed),
          loot_generator_(loot_gen_cfg.GetPeriodInMilliseconds(), loot_gen_cfg.probability_){};
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
    const std::pair<const std::vector<Player>*, const std::vector<Item>*> GetPlayersAndItems(const std::string& token);
    bool FindAndMovePlayersDog(const std::string& token, std::string_view command);
    void ChangeTime(uint64_t deltaInMilliseconds);

   private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;

    std::vector<GameSession*> sessions_;
    MapIdToIndex map_id_to_session_index_;

    double defaultDogSpeed_{0.0};
    loot_gen::LootGenerator loot_generator_;
};

}  // namespace model
