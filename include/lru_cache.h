#pragma once

#include <cstdint>
#include <string>
#include <mutex>
#include <list>
#include <map>
#include <set>
#include <vector>

class IRequest;
class IReply;

class LruCache
{
public:
    LruCache();

public:
    void setIRequest(IRequest* request);
    void setIReply(IReply* reply);

public:
    uint64_t onRequest(const std::string& key);
    bool onProcessNext();

private:
    bool getFromCache(const std::string& key,
                      std::string& value,
                      bool& processed) const;
    void putIntoCache(const std::string& key,
                      const std::string& value,
                      bool processed);

private:
    IRequest*   irequest;
    IReply*     ireply;

private:
    std::mutex  mutexId;
    uint64_t    lastId;

private:
    std::mutex                                  mutexSrcData;
    std::list<std::string>                      srcDataKeys;
    std::map<std::string, std::set<uint64_t>>   srcDataIds;

private:
    std::map<std::string, int>                                          cacheIndexes;
    std::vector<std::pair<std::string, std::pair<bool, std::string>>>   cache;
    int                                                                 cacheIndex;
    static const int                                                    cacheMaxSize;
};
