#include "game_session.h"
#include <algorithm>
#include "model.h"
#include "player_tokens.h"

namespace model {

// void GameSession::AddDog(const model::Dog& dog) {
//     dogs_.push_back(dog);
// }

// std::shared_ptr<Player> GameSession::AddPlayer(const std::string player_name) {

//     auto itFind = std::find_if(players_.begin(), players_.end(), [&player_name](std::shared_ptr<Player>& player) {
//         return player->GetName() == player_name;
//     });

//     if (itFind != players_.end())
//         return *itFind;

//     PlayerTokens tk;
//     auto token = tk.CreateToken();
//     auto player = std::make_shared<Player>(player_id, player_name, token);

//     players_.push_back(player);
//     player_id++;

//     return players_.back();
// }

// bool GameSession::HasPlayerWithAuthToken(const std::string& auth_token) {
//     auto itFind = std::find_if(players_.begin(), players_.end(), [&auth_token](std::shared_ptr<Player>& player) {
//         return player->CreateToken() == auth_token;
//     });

//     if (itFind != players_.end())
//         return true;

//     return false;
// }

// std::map<unsigned int, std::string> GameSession::GetAllPlayersInfo(const std::string& auth_token) {
//     std::map<unsigned int, std::string> result;

//     if (!HasPlayerWithAuthToken(auth_token))
//         return result;

//     std::for_each(players_.begin(), players_.end(),
//                   [&result](std::shared_ptr<Player>& player) { result[player->GetId()] = player->GetName(); });

//     return result;
// }

}  // namespace model
