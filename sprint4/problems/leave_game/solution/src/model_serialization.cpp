
#include "model_serialization.h"

namespace net = boost::asio;

namespace serialization {

using Clock = std::chrono::steady_clock;
using milliseconds = std::chrono::milliseconds;
using Strand = net::strand<net::io_context::executor_type>;

GameSerializer::GameSerializer(Strand& api_strand, model::Game& game, const app::Config& config)
    : game_(game), path_(config.state_path), is_state_path_exists_(config.is_state_path_exists) {
    if (is_state_path_exists_) {
        if (std::filesystem::exists(config.state_path)) {
            DeserializeGameStateFromFile();
        }
        if (config.is_save_state_period_exists) {
            is_store_period_exists_ = true;
            store_period_ = std::chrono::milliseconds(config.save_state_period);
        }
    }
}

GameSerializer::~GameSerializer() {
    if (is_state_path_exists_) {
        SerializeGameStateToFile();
    }
}

void GameSerializer::SerializeGameStateOnTimer(milliseconds deltaInMilliseconds) {
    if (!is_store_period_exists_) {
        return;
    }
    current_time_ += deltaInMilliseconds;
    if (current_time_ >= store_period_) {
        current_time_ = {};
        SerializeGameStateToFile();
    }
}

void GameSerializer::SerializeGameStateToFile() {
    std::lock_guard lk(mtx_);
    auto tmp_file = std::string(path_) + ".tmp";
    std::ofstream out{tmp_file, std::ios_base::binary};
    //boost::archive::binary_oarchive ar{out};
    boost::archive::text_oarchive ar{out};

    // 1. кол-во сессий
    ar << game_.GetSessions().size();
    for (const auto& session : game_.GetSessions()) {
        // 2. сессия
        ar << GameSessionObj(*session);
    }
    std::rename(tmp_file.c_str(), path_.c_str());
}

void GameSerializer::DeserializeGameStateFromFile() {
    std::lock_guard lk(mtx_);
    std::ifstream in{std::string(path_), std::ios_base::binary};
    //boost::archive::binary_oarchive ar{out};
    boost::archive::text_iarchive ar{in};

    size_t sessions_to_deserialize;
    // 1. кол-во сессий
    ar >> sessions_to_deserialize;
    for (size_t i = 0; i < sessions_to_deserialize; ++i) {
        GameSessionObj session_repr;
        // 2. сессия
        ar >> session_repr;
        auto session = game_.GetSession(session_repr.GetMapId());
        for (const auto& player_repr : session_repr.GetPlayers()) {
            session->AddPlayer(player_repr.Restore());
        }
        for (const auto& item_repr : session_repr.GetItems()) {
            session->AddItem(item_repr.Restore());
        }
    }
}

}  // namespace serialization
