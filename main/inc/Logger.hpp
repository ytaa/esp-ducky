#pragma once

#include <array>
#include <string>
#include <cstdint>

#define LOGD(...) Logger::get().log(Logger::Level::Debug, __VA_ARGS__)
#define LOGI(...) Logger::get().log(Logger::Level::Info, __VA_ARGS__)
#define LOGW(...) Logger::get().log(Logger::Level::Warning, __VA_ARGS__)
#define LOGE(...) Logger::get().log(Logger::Level::Error, __VA_ARGS__)
#define LOGC(...) Logger::get().log(Logger::Level::Critical, __VA_ARGS__);fflush(stdout);abort()

class Logger {
public:
    enum class Level : std::uint8_t {
        Debug,
        Info,
        Warning,
        Error,
        Critical,
        LevelNum
    };

    static const std::array<std::string, static_cast<size_t>(Level::LevelNum)> levelNames;

    Level level;

    static Logger& get();

    void log(Level level, const char *fmt, ...);

    void setLevel(Level newLevel);

private:
    Logger();
    Logger(const Logger&) = delete;
    ~Logger() = default;
};