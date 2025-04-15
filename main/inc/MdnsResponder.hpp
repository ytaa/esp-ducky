#pragma once

#include <string>

#include "Utils.hpp"

class MdnsResponder {
public:
    MdnsResponder(std::string hostname);
    ~MdnsResponder() = default;
    
    ErrorCode start();
private:
    std::string hostname;
};
