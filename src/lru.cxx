#include <iostream>
#include <list>
#include <mutex>
#include <time.h>
#include <stdio.h>
#include <cstdint>
#include <cstring>
#include "mempool.h"
#include "log.h"
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
static struct Item* EvictOneItem(int slabClassID)
{
    struct Item* item = nullptr;
    struct LruCache *lruInstance = &lru[slabClassID];

    if (!lruInstance->listSize) {
	    /*
	     * All hell breaks lose
	     */
	    LogMsg(LOG_HIGH, "Something is fishy \n");
	    return nullptr;
    }

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
        item = EvictOneItem(slabClassID);

    /*
     * Copy over the raw key and value pairs to the item structure
     */
    item->keyLength = key.length();
    std::memcpy(&item->data[0], key.c_str(), item->keyLength);

    item->dataLength = value.length();
    std::memcpy(&item->data[item->keyLength], value.c_str(), item->dataLength);


    list_add(&item->node, &lruInstance->head);
    itemOut = item;

    return 0;
}

void LruTouchItem(struct Item* item)
{
	if (!item)
		return;

	struct LruCache *lruInstance = &lru[item->slabClassID];

	/*
	 * Using a simple RAII style lock gaurd
	 */
	std::lock_guard<std::mutex> gaurd(lruInstance->cacheLock);

	/*
	 * Unlink and move the node to the head of the list
	 */
	list_del(&item->node);
	list_add(&item->node, &lruInstance->head);
}
