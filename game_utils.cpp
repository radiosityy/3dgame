#include "game_utils.h"
#include <print>
#include <fstream>
#include <format>

void log(std::string_view msg, std::source_location srcl)
{
    static std::ofstream logfile("game.log");

    const std::string log_msg = std::format("{}({}:{}) {}: {}", srcl.file_name(), srcl.line(), srcl.column(), srcl.function_name(), msg);

    logfile << log_msg << std::endl;
#if DEBUG
    std::println("{}", log_msg);
#endif
}

void error(std::string_view msg, std::source_location srcl)
{
    const auto error_msg = std::format("[ERROR]: {}", msg);
    log(error_msg, srcl);
    throw std::runtime_error(msg.data());
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
void messageBox(std::string_view msg)
{
    MessageBox(NULL, msg.data(), "ERROR", MB_OK);
}
#elif defined(__linux__)
void messageBox(std::string_view msg)
{
   std::println("{}", msg);
}
#endif
