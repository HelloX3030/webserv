#pragma once

#include <stdexcept>

class SetupError : public std::runtime_error
{
  public:
    explicit SetupError(const std::string &msg)
        : std::runtime_error(msg)
    {
    }
};
