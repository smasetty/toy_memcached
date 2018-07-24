#include <iostream>
#include <cstring>
#include <list>
#include <mutex>
#include <time.h>
#include "mempool.h"
#include "slabs.h"
#include "log.h"

#define MAX_PAGES_PER_SLAB  10

#define INITIAL_CHUNK_SIZE  100 //bytes
#define DEFAULT_ALIGNMENT   8
#define CHUNK_GROWTH_FACTOR 1.25

struct SlabClass {
    unsigned int chunkSize;
    unsigned int totalItems;
    void** poolPages;
    unsigned int totalPagesAlloced;
    std::list<struct Item*> freeItems;
    std::mutex slabLock;
};

static SlabClass SlabClass[MAX_SLABS];

#define ALIGN(a, x)  ALIGN_MASK(x, ((a)-1))
#define ALIGN_MASK(x, mask)  (((x) + mask)&~mask)

#define ALIGN_MOD(a, x) (((x)+a) - ((x)%a))

int GetItem(struct Item* item)
{
	return ++(item->refCount);
}

int PutItem(struct Item* item)
{
	return --(item->refCount);
}

/*
 * TODO: how does the user send us the timeout/expiration
 */
int ConstructItem(struct Item* item, uint64_t itemExpiry)
{
    INIT_LIST_HEAD(&item->node);
    /*
     * Do the timestamping here, We could either use the realtime clock or
     * the monotonic clock. The monotonic clock may(not) work during system
     * suspend state, and the realtime clock is subject to external interference
     * such as user time changes, NTP, etc. Which one to pick?
     */
    struct timespec tsCurrent;

    clock_gettime(CLOCK_MONOTONIC, &tsCurrent);
    item->timestamp = tsCurrent.tv_sec;

    /*
     * A value of '0' means that never expire. The item could still be evicted
     * if this is the least recently used
     */
    item->expiry = itemExpiry;
    item->refCount = 1;
    return 0;
}

/*
 * Each slab only gets one page at init time from the page pool.
 * After all the free items from the initial page are exhausted,
 * new pages from the pool are requested on demand basis.
 */
static int PreAllocateItems(int slabClassID, unsigned int chunkSize, void* page)
{
    struct SlabClass* slab = &SlabClass[slabClassID];
    char *ptr = (char *) page;
    unsigned int pageSize = GetPageSize();
    unsigned int remainingSpace = pageSize;

    chunkSize = std::min(chunkSize, pageSize);

    while (remainingSpace > chunkSize) {
        struct Item* item = (struct Item*)ptr;
	memset(item, 0, sizeof(struct Item));
        item->slabClassID = slabClassID;
        slab->freeItems.push_back(item);
        remainingSpace -= chunkSize;

        if (remainingSpace > chunkSize)
            ptr += chunkSize;
    }

    return 0;
}

void ReleaseItems(int slabClassID)
{
    struct SlabClass* slab = &SlabClass[slabClassID];
    std::list<struct Item*>::iterator it;

    for(it = slab->freeItems.begin(); it != slab->freeItems.end(); it++)
        //TODO: really free up?
        //Do nothing for now
        ;

    return;
}

static void FreePages(int slabClassID)
{
    struct SlabClass* slab = &SlabClass[slabClassID];

    ReleaseItems(slabClassID);

    for (int i = 0; i < (int)slab->totalPagesAlloced; i++)
        MempoolPageFree(slab->poolPages[i]);

    delete [] slab->poolPages;
}

void SlabClassDestroy()
{
    for(int i = 0; i < MAX_SLABS; i++)
        FreePages(i);

    MempoolDestroy();
    return;
}

#define MAX_MEMPOOL_PAGES 200
int SlabClassInit()
{
    int chunkSize  = INITIAL_CHUNK_SIZE;

    MempoolInit(MAX_MEMPOOL_PAGES);

    for(int i = 0; i < MAX_SLABS; i++) {
        struct SlabClass* slab = &SlabClass[i];

        chunkSize = ALIGN(DEFAULT_ALIGNMENT, chunkSize);
        slab->chunkSize = chunkSize;
        slab->poolPages = new(std::nothrow) void*[MAX_PAGES_PER_SLAB];
        if (!slab->poolPages)
            //clean up // return
            ;

        slab->poolPages[0] = MempoolPageAlloc();
        if (!slab->poolPages[0])
            //cleanup; return;
            ;

        slab->totalPagesAlloced++;

        /*
         * If we reached this far, then we are ready to setup the free item
         * store to cache the key/value pairs for this class
         */
        PreAllocateItems(i, chunkSize, slab->poolPages[0]);

        chunkSize *= CHUNK_GROWTH_FACTOR;
    }

    return 0;
}

int GetSlabClassID(int valueLength)
{
    int totalLength = valueLength + sizeof(struct Item)  + 1;
    struct SlabClass* slab;
    int i;

    for(i = 0; i < MAX_SLABS; i++) {
        slab = &SlabClass[i];

        if (totalLength < (int)slab->chunkSize)
            break;
    }

    /*
     * The Key Value pair is too large for any of our existing slabs
     */
    if (i == MAX_SLABS)
        return -1;

    return i;
}

/*
 * TODO: should be called with the slabLock held, for now this function also
 * allocates items for the newly allocated backend page.
 */
static void TryGetMorePages(int slabClassID)
{
    struct SlabClass* slab;

    if (slabClassID >= MAX_SLABS)
        return;

    slab = &SlabClass[slabClassID];

    if (slab->totalPagesAlloced == MAX_PAGES_PER_SLAB)
        return;

    slab->poolPages[slab->totalPagesAlloced] = MempoolPageAlloc();

    if (slab->poolPages[slab->totalPagesAlloced]) {
        PreAllocateItems(slabClassID, slab->chunkSize,
                slab->poolPages[slab->totalPagesAlloced]);
        slab->totalPagesAlloced++;
    }
}

struct Item* AllocItemFromSlab(int slabClassID)
{
    struct Item* item = nullptr;
    struct SlabClass* slab;

    if (slabClassID >= MAX_SLABS)
        return nullptr;

    slab = &SlabClass[slabClassID];

    slab->slabLock.lock();
    /*
     * Try to allocate a backend page from the pool if we are out of freeItems
     */
    if (!slab->freeItems.size())
        TryGetMorePages(slabClassID);

    if (!slab->freeItems.size()) {
	    slab->slabLock.unlock();
	    return nullptr;
    }

    item = slab->freeItems.front();
    slab->freeItems.pop_front();

    slab->slabLock.unlock();
    return item;
}

void FreeItem(struct Item* item, int slabClassID)
{
    struct SlabClass* slab;

    if (!item || slabClassID < 0 || slabClassID >= MAX_SLABS)
        return;

    slab = &SlabClass[slabClassID];
    /*
     * Put the item back on the free store
     */
    slab->freeItems.push_back(item);
}
