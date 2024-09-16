#include "lru_cache.h"

#include "irequest.h"
#include "ireply.h"

#include <utility>

const int LruCache::cacheMaxSize = 20;  // aka 'N'

LruCache::LruCache()
    : irequest(nullptr)
    , ireply(nullptr)
    , lastId(0)
    , cacheIndex(0)
{
    cache.reserve(cacheMaxSize);
}

void LruCache::setIRequest(IRequest* request)
{
    irequest = request;
}

void LruCache::setIReply(IReply* reply)
{
    ireply = reply;
}

uint64_t LruCache::onRequest(const std::string& key)
{
    if((nullptr == irequest) || (nullptr == ireply))
    {
        return 0;
    }

    mutexId.lock();
    uint64_t id = ++lastId;
    mutexId.unlock();

    std::string value;
    bool processed = false;

    mutexSrcData.lock();

    if(getFromCache(key, value, processed))
    {
        if(processed)
        {
            ireply->onReply(id, value);
        }
        else
        {
            ireply->onReply(id);
        }
    }
    else
    {
        std::map<std::string, std::set<uint64_t>>::iterator itSrc =
                srcDataIds.find(key);
        if(srcDataIds.end() == itSrc)
        {
            srcDataKeys.insert(srcDataKeys.end(), key);
            srcDataIds.insert(std::make_pair(key, std::set<uint64_t>{id}));
        }
        else
        {
            itSrc->second.insert(id);
        }
    }

    mutexSrcData.unlock();

    return id;
}

bool LruCache::onProcessNext()
{
    if((nullptr == irequest) || (nullptr == ireply))
    {
        return false;
    }

    mutexSrcData.lock();

    if(srcDataKeys.empty())
    {
        mutexSrcData.unlock();
        return false;
    }

    std::string key;
    key.swap(*srcDataKeys.begin());
    srcDataKeys.pop_front();

    mutexSrcData.unlock();

    std::string value;
    const bool processed = irequest->onRequest(key, value);

    mutexSrcData.lock();

    putIntoCache(key, value, processed);

    std::map<std::string, std::set<uint64_t>>::iterator itSrcIds =
            srcDataIds.find(key);
    std::set<uint64_t> ids;
    ids.swap(itSrcIds->second);
    srcDataIds.erase(itSrcIds);

    mutexSrcData.unlock();

    for(std::set<uint64_t>::const_iterator citId = ids.cbegin() ;
        ids.cend() != citId ; ++citId)
    {
        if(processed)
        {
            ireply->onReply(*citId, value);
        }
        else
        {
            ireply->onReply(*citId);
        }
    }

    return true;
}

bool LruCache::getFromCache(const std::string& key,
                            std::string& value,
                            bool& processed) const
{
    mutexCache.lock();

    std::map<std::string, int>::const_iterator citIndex =
            cacheIndexes.find(key);
    const bool found = (cacheIndexes.cend() != citIndex);
    if(found)
    {
        processed = cache[citIndex->second].second.first;
        value = cache[citIndex->second].second.second;
    }

    mutexCache.unlock();

    return found;
}

void LruCache::putIntoCache(const std::string& key,
                            const std::string& value,
                            bool processed)
{
    mutexCache.lock();

    if(cache.size() < cacheMaxSize)
    {
        cacheIndexes.insert(std::make_pair(key, cache.size()));
        cache.push_back(std::make_pair(key, std::make_pair(processed, value)));
    }
    else
    {
        cacheIndexes.erase(cache[cacheIndex].first);
        cacheIndexes.insert(std::make_pair(key, cacheIndex));
        cache[cacheIndex] = std::make_pair(key, std::make_pair(processed, value));
        ++cacheIndex;
        if(cacheIndex >= cacheMaxSize)
        {
            cacheIndex = 0;
        }
    }

    mutexCache.unlock();
}
