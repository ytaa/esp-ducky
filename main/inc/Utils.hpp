#pragma once

#include <cstdint>

enum class ErrorCode : std::uint8_t {
    Success,
    GeneralError,
    InvalidArgument,
    NotImplemented,
};

namespace Utils {
    void delay(uint32_t ms);
}