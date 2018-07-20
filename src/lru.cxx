#include <iostream>
#include <list>
#include <mutex>
#include <time.h>
#include "mempool.h"
#include "slabs.h"

struct LruCache {
    uint64_t active_timestamp;
    std::list<struct Item*> cacheItems;
    std::mutex cacheLock;
};

struct LruCache lru[MAX_SLABS];

#define MAX_ITEMS_PER_LRU   100

void LruCacheInit()
{
    return;
}

void LruCacheDestroy()
{
    return;
}

static bool ItemExpired(struct Item* item)
{
    struct timespec tsCurrent;

    clock_gettime(CLOCK_MONOTONIC, &tsCurrent);

    return (tsCurrent.tv_sec >= item->expiry);
}

static struct Item* GetExpiredItem(uint32_t slabClassID)
{
    std::list<struct Item*>::reverse_iterator it;
    struct LruCache *lruInstance = &lru[slabClassID];

    if (!lruInstance->cacheItems.size())
        return nullptr;

    /*
     * Start from the end of the list(lru) and pluck out an expired entry
     */
    for (it = lruInstance->cacheItems.rbegin();
            it !=lruInstance->cacheItems.rend(); it++) {
        struct Item* item = *it;

        if (ItemExpired(item))
            return item;
    }

    return nullptr;
}

/*
 * slabClassID should be validated
 * This function should  be called with the lruInstance cacheLock held
 */
static bool IsLruFull(int slabClassID)
{
    struct LruCache *lruInstance = &lru[slabClassID];

    return lruInstance->cacheItems.size() == MAX_ITEMS_PER_LRU;
}

/*
 * This function should  be called with the lruInstance cacheLock held
 */
static struct Item* TryEvictItem(int slabClassID)
{
    struct Item* item = nullptr;
    struct LruCache *lruInstance = &lru[slabClassID];

    item = lruInstance->cacheItems.back();
    lruInstance->cacheItems.pop_back();

    return item;
}

int LruCacheInsert(std::string& key, std::string& value, int expiry,
		struct Item* itemOut)
{
    int slabClassID = GetSlabClassID(key.length() + value.length());
    struct Item* item = nullptr;

    if (slabClassID < 0)
        return -1;

    struct LruCache *lruInstance = &lru[slabClassID];

    /*
     * Using a simple RAII style lock gaurd
     */
    std::lock_guard<std::mutex> gaurd(lruInstance->cacheLock);

    /*
     * LRU cache Eviction and insert rules
     * a) Check for expired cache entries with the help of timestamps
     * b) if (!a) then Allocate a new Item from the slab, if not full
     * c) if (!(a||b)) then evict the least recently used entry
     * TODO: In the future, move the (a) to a worker thread to save time
     * on the hot path
     */
    item = GetExpiredItem(slabClassID);

    if (!item && !IsLruFull(slabClassID))
        item = AllocItemFromSlab(slabClassID);

    if (!item)
        item = TryEvictItem(slabClassID);

    /*
     * TODO:
     * copy over the key and the value pair to the item, need to think a bit
     * about this to handle different input types
     */

    lruInstance->cacheItems.push_front(item);
    itemOut = item;

    return 0;
}
