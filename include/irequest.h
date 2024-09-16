#pragma once

#include <string>

class IRequest
{
public:
    virtual bool onRequest(const std::string& key,
                           std::string& value) = 0;
};
