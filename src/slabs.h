#ifndef SLABCLASS
#define SLABCLASS

#define MAX_SLABS           10

/*
 * Need to take care of a few more things on the Item struct, but should be
 * okay for now
 */
struct Item {
    uint64_t expiry;
    uint64_t timestamp;
    uint64_t key;
    int slabClassID;
    char data[0];
};

int SlabClassInit();

void SlabClassDestroy();

int GetSlabClassID(int valueLength);

int ConstructItem(struct Item* item, uint64_t itemExpiry);

struct Item* AllocItemFromSlab(int slabClassID);

#endif /* ifndef SLABCLASS */
