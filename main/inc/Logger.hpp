#include <array>
#include <string>
#include <cstdint>

#define LOGD(...) Logger::get().log(Logger::Level::DEBUG, __VA_ARGS__)
#define LOGI(...) Logger::get().log(Logger::Level::INFO, __VA_ARGS__)
#define LOGW(...) Logger::get().log(Logger::Level::WARNING, __VA_ARGS__)
#define LOGE(...) Logger::get().log(Logger::Level::ERROR, __VA_ARGS__)
#define LOGC(...) Logger::get().log(Logger::Level::CRITICAL, __VA_ARGS__)

class Logger {
public:
    enum class Level : std::uint8_t {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL,
        LEVEL_COUNT
    };

    static const std::array<std::string, static_cast<size_t>(Level::LEVEL_COUNT)> levelNames;

    Level level;

    static Logger& get();

    void log(Level level, const char *fmt, ...);

    void setLevel(Level newLevel);

private:
    Logger();
    Logger(const Logger&) = delete;
    ~Logger() = default;
};