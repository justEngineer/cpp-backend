#include "player_tokens.h"
#include <sstream>
#include "game.h"

namespace {
constexpr size_t TOKEN_SIZE = 32;
}

std::string PlayerTokens::GetToken() {
    auto hexConverter = [](const auto& value) -> std::string {
        std::stringstream sstream;
        sstream << std::hex << value;
        return sstream.str();
    };
    std::string token;
    do {
        token = hexConverter(generator1_()) + hexConverter(generator2_());
    } while (token.size() != TOKEN_SIZE);
    return token;
}