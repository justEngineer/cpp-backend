#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <game.h>
using namespace std;

//class Dog;

namespace model {

// class Player {
//    public:
//     Player(unsigned int id, const std::string& name, const std::string& token) : id_(id), name_(name), token_(token) {}
//     const std::string& GetToken() { return token_; }
//     const std::string& GetName() { return name_; }
//     unsigned int GetId() { return id_; }

//    private:
//     std::string name_;
//     std::string token_;
//     unsigned int id_{0};
// };

// class GameSession {
//    public:
//     GameSession(const std::string& map_id) : map_(map_id) {}
//     //    void AddDog(const Dog& dog);
//     std::shared_ptr<Player> AddPlayer(const std::string player_name);
//     const std::string& GetMap() { return map_; }
//     bool HasPlayerWithAuthToken(const std::string& auth_token);
//     std::map<unsigned int, std::string> GetAllPlayersInfo(const std::string& auth_token);

//    private:
//     //vector<Dog> dogs_;
//     vector<std::shared_ptr<Player>> players_;
//     std::string map_;
//     unsigned int player_id = 0;
// };

}  // namespace model
