#include <iostream>
#include <list>
#include <mutex>
#include <time.h>
#include "mempool.h"
#include "slabs.h"

struct LruCache {
    struct list_head head;
    int listSize;
    uint64_t active_timestamp;
    //std::list<struct Item*> cacheItems;
    std::mutex cacheLock;
};

struct LruCache lru[MAX_SLABS];

#define MAX_ITEMS_PER_LRU   100

void LruCacheInit()
{
    for (int i = 0; i < MAX_SLABS; i++) {
        struct LruCache *lruInstance = &lru[i];
        INIT_LIST_HEAD(&lruInstance->head);
        lruInstance->listSize = 0;
    }

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
	struct list_head* pos;
	struct Item* item;
    int count = 5;

	if (!lruInstance->listSize)
		return nullptr;

	list_for_each_prev(pos, &lruInstance->head) {
		item = list_entry(pos, struct Item, node);

		if (ItemExpired(item))
			return item;

        if (!(--count))
            break;
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

    return lruInstance->listSize == MAX_ITEMS_PER_LRU;
}

/*
 * This function should  be called with the lruInstance cacheLock held
 */
static struct Item* TryEvictItem(int slabClassID)
{
    struct Item* item = nullptr;
    struct LruCache *lruInstance = &lru[slabClassID];

	item = list_last_entry(&lruInstance->head, struct Item, node);
    list_del_tail(&lruInstance->head);

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

	list_add(&item->node, &lruInstance->head);
    itemOut = item;

    return 0;
}
