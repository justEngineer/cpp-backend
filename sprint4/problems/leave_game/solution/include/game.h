#pragma once

#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "model.h"
#include "postgres.h"
#include "tagged.h"

namespace serialization {
class GameSerializer;
}
namespace model {

class GameSession {
   public:
    explicit GameSession(const Map& map, loot_gen::LootGenerator& loot_gen, double dog_speed)
        : map_(map), loot_generator_(loot_gen), dog_speed_(dog_speed) {}
    const Player& AddPlayer(const std::string player_name, double default_dog_speed);
    void AddPlayer(Player player) { players_.push_back(player); };
    void AddItem(Item item) { items_.emplace(item.id_, item); };
    const Map& GetMap() { return map_; }
    bool isTokenAlreadyExists(const std::string_view auth_token);
    std::list<Player>& GetAllPlayers();
    const ItemsHash& GetAllItems();
    void ChangeTime(std::chrono::milliseconds deltaInMilliseconds);
    void GenerateItems(std::chrono::milliseconds deltaInMilliseconds);
    void HandleEvents(std::vector<collision_detector::Gatherer>&, std::vector<collision_detector::Item>&,
                      const std::vector<collision_detector::Rectangle>&);
    std::vector<collision_detector::Item>& GetItemsCollision();
    const ItemsHash& GetItems() const noexcept { return items_; }

   private:
    std::list<Player> players_;
    const Map& map_;
    uint64_t player_uid_{0};
    uint64_t item_uid_{0};
    loot_gen::LootGenerator& loot_generator_;
    ItemsHash items_;
    std::vector<collision_detector::Gatherer> dogs_collisions_;
    std::vector<collision_detector::Item> items_collision_;
    const double dog_speed_;
};

class Game {
    static constexpr const double SECONDS_TO_MILLI = 1000.0;

   public:
    using Maps = std::vector<Map>;
    explicit Game(double defaultDogSpeed, LootGeneratorCfg&& loot_gen_cfg, uint32_t bag_capacity,
                  double player_inactive_timeout, std::string_view db_url)
        : defaultDogSpeed_(defaultDogSpeed),
          loot_generator_(loot_gen_cfg.GetPeriodInMilliseconds(), loot_gen_cfg.probability_),
          default_bag_capacity_(bag_capacity),
          player_inactive_timeout_(std::chrono::milliseconds(uint64_t(player_inactive_timeout * SECONDS_TO_MILLI))),
          db_(db_url){};
    void AddMap(Map&& map);
    const Maps& GetMaps() const noexcept { return maps_; }
    void CheckExpiredPlayers();
    void SavePlayerDataToDB(const model::Player& player);
    std::vector<model::Player::PlayerInfo> GetInfoFromDB(uint64_t start, uint64_t limit);
    const Map* FindMap(const Map::Id& id) const noexcept;
    std::pair<std::string, const Player*> AddPlayer(AddNewPlayerRequest player);
    GameSession* GetSession(const Map::Id& id);
    const std::vector<GameSession*>& GetSessions() { return sessions_; };
    const std::pair<const std::list<Player>*, const ItemsHash*> GetPlayersAndItems(const std::string_view token);
    bool FindAndMovePlayersDog(const std::string_view token, std::string_view command);
    void ChangeTime(std::chrono::milliseconds deltaInMilliseconds);
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
    const std::chrono::milliseconds player_inactive_timeout_{std::chrono::milliseconds(60000)};
    db::Database db_;
};

}  // namespace model
