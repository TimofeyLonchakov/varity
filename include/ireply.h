#pragma once

#include <string>
#include <cstdint>

class IReply
{
public:
    virtual void onReply(uint64_t id,
                         const std::string& value) = 0;
    virtual void onReply(uint64_t id) = 0;
};
