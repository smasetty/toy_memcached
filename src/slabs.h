#ifndef SLABCLASS
#define SLABCLASS

#define MAX_SLABS           10

#include <atomic>
#include <string>

/*
 * Need to take care of a few more things on the Item struct, but should be
 * okay for now
 */
struct Item {
    uint64_t expiry;
    uint64_t timestamp;
    int slabClassID;
    std::atomic<int> refCount;
    uint32_t keyLength;
    uint32_t dataLength;
    char data[0];
};

int SlabClassInit();

void SlabClassDestroy();

int GetSlabClassID(int valueLength);

int ConstructItem(struct Item* item, uint64_t itemExpiry);

int GetItem(struct Item* item);

int PutItem(struct Item* item);

struct Item* AllocItemFromSlab(int slabClassID);

int LruCacheInsert(std::string& key, std::string& value, int expiry,
		struct Item* itemOut);

#endif /* ifndef SLABCLASS */
