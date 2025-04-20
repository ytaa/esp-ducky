#pragma once

#include <vector>
#include <any>
#include <tuple>
#include <string>
#include <regex>
#include <functional>
#include <optional>

#include "UsbDevice.hpp"
#include "Utils.hpp"

class Script
{

    // Constants ===

    constexpr static std::size_t EXPRESSIONS_NUM = 10u;
    constexpr static std::size_t SPECIAL_KEY_NUM = 50u;
    constexpr static uint8_t ASCII_CHAR_NUM = 128u;
    constexpr static uint8_t ASCII_CONV_TABLE_DIM_NUM = 2u;


    // Types ===

    enum class Command : std::uint8_t {
        StringWrite,
        KeyStroke,
        Delay
    };

    struct ExpressionHandler {
        std::string regexStr;
        std::function<ErrorCode(std::smatch&, std::vector<std::tuple<Command, std::any>>&)> process;
    };
    struct
     SpecialKey {
        std::string name;
        uint8_t keyCode;
    };

    // Static members ===
    
    static std::array<ExpressionHandler, EXPRESSIONS_NUM> expressionHandlers;
    static std::array<SpecialKey, SPECIAL_KEY_NUM> specialKeys;
    static const uint8_t asciiToKeycodeConvTable[ASCII_CHAR_NUM][ASCII_CONV_TABLE_DIM_NUM];

    static ErrorCode parseAscii(const char chr, std::vector<uint8_t> &keyCodes);
    static ErrorCode parseKeyStroke(const std::string &keyName, std::vector<uint8_t> &keyList);

    // Non-static members ===

    std::vector<std::tuple<Command, std::any>> actionList;
public:
    Script(std::vector<std::tuple<Command, std::any>> &actionList);
    Script(std::vector<std::tuple<Command, std::any>> &&actionList);
    ~Script() = default;

    ErrorCode run(UsbDevice &usbDevice);

    static std::optional<Script> parse(std::string input);
};
