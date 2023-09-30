#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#include <string>
#include <source_location>

void log(std::string_view msg, std::source_location = std::source_location::current());
void error(std::string_view msg, std::source_location = std::source_location::current());

#endif // GAME_UTILS_H
