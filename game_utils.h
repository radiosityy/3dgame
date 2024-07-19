#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#include <string_view>
#include <source_location>

void log(std::string_view msg, std::source_location = std::source_location::current());
void error(std::string_view msg, std::source_location = std::source_location::current());

//TODO:move this to a different file as it's platform specific?
void messageBox(std::string_view msg);

#endif // GAME_UTILS_H
