#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "model.h"
#include "tagged.h"

namespace serialization {
class GameSerializer;
}
namespace model {

class GameSession {
   public:
    explicit GameSession(const Map& map, loot_gen::LootGenerator& loot_gen) : map_(map), loot_generator_(loot_gen) {}
    const Player& AddPlayer(const std::string player_name);
    void AddPlayer(Player player) { players_.emplace_back(player); };
    void AddItem(Item item) { items_.emplace(item.id_, item); };
    const Map& GetMap() { return map_; }
    bool isTokenAlreadyExists(const std::string& auth_token);
    const std::vector<Player>& GetAllPlayers();
    const ItemsHash& GetAllItems();
    void ChangeTime(uint64_t deltaInMilliseconds);
    void GenerateItems(uint64_t deltaInMilliseconds);
    void HandleEvents(std::vector<collision_detector::Gatherer>&, std::vector<collision_detector::Item>&,
                      const std::vector<collision_detector::Rectangle>&);
    std::vector<collision_detector::Item>& GetItemsCollision() {
        items_collision_.clear();
        for (const auto& [item_id, item] : items_) {
            items_collision_.emplace_back(
                collision_detector::Item{.position_ = {item.position_.x, item.position_.y}, .width_ = item.width_});
        }
        return items_collision_;
    }
    const ItemsHash& GetItems() const noexcept { return items_; }

   private:
    std::vector<Player> players_;
    const Map& map_;
    uint64_t player_uid_{0};
    uint64_t item_uid_{0};
    loot_gen::LootGenerator& loot_generator_;
    ItemsHash items_;
    std::vector<collision_detector::Gatherer> dogs_collisions_;
    std::vector<collision_detector::Item> items_collision_;
};

class Game {
   public:
    using Maps = std::vector<Map>;
    explicit Game(double defaultDogSpeed, LootGeneratorCfg&& loot_gen_cfg, uint32_t bag_capacity)
        : defaultDogSpeed_(defaultDogSpeed),
          loot_generator_(loot_gen_cfg.GetPeriodInMilliseconds(), loot_gen_cfg.probability_),
          default_bag_capacity_(bag_capacity){};
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
    const std::vector<GameSession*>& GetSessions() { return sessions_; };
    const std::pair<const std::vector<Player>*, const ItemsHash*> GetPlayersAndItems(const std::string& token);
    bool FindAndMovePlayersDog(const std::string& token, std::string_view command);
    void ChangeTime(uint64_t deltaInMilliseconds);
    void SetSerializer(serialization::GameSerializer* serializer) { serializer_ = serializer; }

   private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;

    std::vector<GameSession*> sessions_;
    MapIdToIndex map_id_to_session_index_;

    double defaultDogSpeed_{0.0};
    loot_gen::LootGenerator loot_generator_;
    uint32_t default_bag_capacity_{0};
    serialization::GameSerializer* serializer_;
};

}  // namespace model
