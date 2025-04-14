#pragma once

#include <string>

#include "Utils.hpp"

class MDNSResponder {
public:
    MDNSResponder(std::string hostname);
    ~MDNSResponder() = default;
    
    ErrorCode start();
private:
    std::string hostname;
};
