#include "lru_cache.h"
#include "irequest.h"
#include "ireply.h"

#include <iostream>
#include <cstdint>
#include <set>
#include <mutex>
#include <unistd.h>
#include <thread>

#define THREAD_COUNT 4

const std::string requests[] =
{
    "a",
    "aa",
    "aaa",
    "aaaa",
    "b",
    "bb",
    "bbb",
    "bbbb",
    "h",
    "aa",   // <<--
    "aka",
    "akka",
    "b",
    "bb",   // <<--
    "bbb",
    "bbbb"  // <<--
};

std::mutex mtxOutput;

class Request : public IRequest
{
public:
    Request(LruCache& cache)
        : lruCache(cache) {
    }

public:
    bool onRequest(const std::string& key,
                   std::string& value)
    {
        usleep(200000); // 0.2(sec)

        if(0 != (key.length() % 2))
        {
            return false;
        }

        value = key + key;

        return true;
    }

public:
    void run()
    {
        for(unsigned int req = 0 ; req < sizeof(requests) / sizeof(requests[0]) ; ++req)
        {
            uint64_t id = lruCache.onRequest(requests[req]);

            mtxOutput.lock();
            if(0 == id)
            {
                std::cout << "Key=\'" << requests[req] << "\' rejected.\n";
            }
            else
            {
                mtxIds.lock();
                std::cout << "Key=\'" << requests[req] << "\' accepted, ID=" << id << ", "
                          << ((ids.cend() != ids.find(id)) ? "cached" : "non-cached (yet)") << ".\n";
                mtxIds.unlock();
            }
            mtxOutput.unlock();

            usleep(300000);
        }
    }

public:
    void addProcessedId(uint64_t id)
    {
        mtxIds.lock();
        ids.insert(id);
        mtxIds.unlock();
    }

private:
    std::mutex          mtxIds;
    LruCache&           lruCache;
    std::set<uint64_t>  ids;
};

class Reply : public IReply
{
public:
    Reply(Request& r)
        : request(r) {
    }

public:
    void onReply(uint64_t id,
                 const std::string& value)
    {
        request.addProcessedId(id);

        mtxOutput.lock();

        std::cout << "ID=" << id << ": processing passed, value=\'" << value << "\'.\n";

        mtxOutput.unlock();
    }
    void onReply(uint64_t id)
    {
        request.addProcessedId(id);

        mtxOutput.lock();

        std::cout << "ID=" << id << ": processing failed.\n";

        mtxOutput.unlock();
    }

private:
    Request&    request;
};

LruCache* g_lruCache = nullptr;

void processing()
{
    int successCount = 0;

    for(unsigned int req = 0 ; req < sizeof(requests) / sizeof(requests[0]) ; ++req)
    {
        if(g_lruCache->onProcessNext())
        {
            ++successCount;
        }
        else
        {
            usleep(100000);
        }

        usleep(300000);
    }

    mtxOutput.lock();

    std::cout << "Processed: " << successCount << " key(s).\n";

    mtxOutput.unlock();
}

int main()
{
    LruCache lruCache;
    Request request(lruCache);
    Reply reply(request);

    lruCache.setIRequest(&request);
    lruCache.setIReply(&reply);

    g_lruCache = &lruCache;

    std::thread processingThreads[THREAD_COUNT];

    for(int th = 0; th < THREAD_COUNT ; ++th) {
        processingThreads[th] = std::thread(processing);
    }

    request.run();

    for(int th = 0; th < THREAD_COUNT ; ++th) {
        processingThreads[th].join();
    }

    return 0;
}
