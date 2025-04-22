#include <stdio.h>
#include <stdarg.h>
#include <ctime>

#include "Logger.hpp"
#include "Utils.hpp"

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define RESET "\x1B[0m"

const std::array<std::string, static_cast<size_t>(Logger::Level::LevelNum)> Logger::levelNames{
    GRN "DBG" RESET,
    BLU "INF" RESET,
    YEL "WRN" RESET,
    RED "ERR" RESET,
    MAG "CRI" RESET
};

Logger::Logger() : level(Logger::Level::Debug) {}

Logger& Logger::get() {
    static Logger *instance = new Logger();
    return *instance;
}

void Logger::abort() {
    // FLush the output buffer to ensure all logs are printed before aborting
    fflush(stdout);

    // Enter infinite loop to halt the program
    while (true) {
        // Delay added to avoid busy-waiting
        Utils::delay(1000u); // 1 second delay
    }
}

void Logger::log(Logger::Level level, const char *fmt, ...) {
    if(level < this->level) {
        return;
    }
    
    va_list args;
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    if(timeinfo != nullptr) {
        printf("[%02d:%02d:%02d] ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    } else {
        printf("[--:--:--] ");
    }

    printf("[%s] ", levelNames[static_cast<size_t>(level)].c_str());
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void Logger::setLevel(Logger::Level newLevel) {
    if (newLevel >= Logger::Level::LevelNum) {
        LOGE("Invalid log level: %u\n", static_cast<unsigned int>(newLevel));
        return;
    }

    level = newLevel;
}
