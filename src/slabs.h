#ifndef SLABCLASS
#define SLABCLASS

#define MAX_SLABS           10

#include <atomic>
#include <string>
#include <stdio.h>
#include <stddef.h>

struct list_head {
	struct list_head *next, *prev;
};

/*
 * Need to take care of a few more things on the Item struct, but should be
 * okay for now
 */
struct Item {
	struct list_head node;
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

void LruCacheInit();

void LruCacheDestroy();

int LruCacheInsert(std::string& key, std::string& value, int expiry,
		struct Item* itemOut);

void LruTouchItem(struct Item* item);

#define typeof __typeof__

#define container_of(ptr, type, member) ({			\
		const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
		(type *)( (char *)__mptr - offsetof(type,member) );})

#define LIST_HEAD_INIT(name) { &(name), &(name) };

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name);

void list_del(struct list_head *entry);

void list_del_tail(struct list_head *head);

void list_add_tail(struct list_head *newItem, struct list_head *head);

void list_add(struct list_head *newItem, struct list_head *head);

void INIT_LIST_HEAD(struct list_head *list);

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define __list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

#endif /* ifndef SLABCLASS */
