#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>
#include "model.h"
#include "tagged_uuid.h"

namespace detail {

struct PlayerTag {};

}  // namespace detail

namespace db {

using PlayerId = util::TaggedUUID<detail::PlayerTag>;

class Database {
   public:
    explicit Database(std::string_view db_url);
    void Save(const model::Player::PlayerInfo& player);
    std::vector<model::Player::PlayerInfo> GetRecords(size_t start, size_t limit);

   private:
    pqxx::connection connection_;
};

}  // namespace db