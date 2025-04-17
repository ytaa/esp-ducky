#include <vector>
#include <any>
#include <tuple>
#include <string>
#include <regex>
#include <functional>

#include "Utils.hpp"

class ScriptParser
{
    constexpr static std::size_t EXPRESSIONS_NUM = 10;
    enum class Command : std::uint8_t {
        WriteString
    };

    struct ExpressionHandler {
        const char * regexStr;
        std::function<ErrorCode(std::smatch&, std::vector<std::tuple<Command, std::any>>&)> process;
    };

    static std::array<ExpressionHandler, EXPRESSIONS_NUM> expressionHandlers;

public:
    ScriptParser() = default;
    ~ScriptParser() = default;

    ErrorCode parse(const std::string &script, std::vector<std::tuple<Command, std::any>> &actionList);
};