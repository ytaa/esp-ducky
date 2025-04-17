#pragma once

#include <cstdint>

enum class ErrorCode : std::uint8_t {
    Success,
    GeneralError,
    InvalidArgument,
};
