#include "postgres.h"

#include <pqxx/pqxx>
#include <pqxx/zview.hxx>
#include <vector>

namespace {
static constexpr const double SECONDS_TO_MILLI = 1000.0;
}

namespace db {

using namespace std::literals;
using pqxx::operator"" _zv;

void Database::Save(const model::Player::PlayerInfo& player) {
    pqxx::work work{connection_};
    work.exec_params(
        R"( INSERT INTO records (id, name, score, play_time) VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE SET name=$2, score=$3, play_time=$4; )"_zv,
        PlayerId::New().ToString(), player.name_, player.score_,
        std::chrono::duration<double>(player.play_time_).count());
    work.commit();
}

std::vector<model::Player::PlayerInfo> Database::GetRecords(size_t start, size_t limit) {
    pqxx::read_transaction r(connection_);
    auto sql =
        R"( SELECT name, score, play_time 
FROM records 
ORDER BY score DESC, play_time, name OFFSET )" +
        std::to_string(start) + R"( LIMIT )" + std::to_string(limit) + R"( ;)";

    std::vector<model::Player::PlayerInfo> res;
    for (auto [name, score, play_time] : r.query<std::string, int, double>(sql)) {
        res.emplace_back(name, score, std::chrono::milliseconds(uint64_t(play_time * SECONDS_TO_MILLI)));
    }
    return res;
}

Database::Database(std::string_view db_url) : connection_(pqxx::connection{db_url.data()}) {
    pqxx::work work{connection_};
    work.exec(R"( CREATE TABLE IF NOT EXISTS records (
    id UUID CONSTRAINT player_id_constraint PRIMARY KEY,
    name varchar(100) NOT NULL,
    score INTEGER NOT NULL,
    play_time DOUBLE PRECISION NOT NULL ); )"_zv);
    work.commit();
}

}  // namespace db