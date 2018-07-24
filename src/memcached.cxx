#include <iostream>
#include <mutex>
#include <unordered_map>
#include <string>
#include "slabs.h"
#include "log.h"

std::unordered_map<std::string, struct Item*> hash;
std::mutex hashLock;
std::mutex globalLock;
static int initialized;

void MemcachedCleanUp()
{
	globalLock.lock();

	if (!initialized) {
		globalLock.unlock();
		return;
	}

	LruCacheDestroy();

	SlabClassDestroy();

	globalLock.unlock();
}

int MemcachedInit()
{
	globalLock.lock();

	if (initialized)  {
		globalLock.unlock();
		return -1;
	}

	const int atExitRet = std::atexit(MemcachedCleanUp);
	if (atExitRet)
		return -1;

	/*
	 * Initialize a rudimentary slab allocator, backed by a mempool
	 * of pages
	 */
	SlabClassInit();

	/*
	 * Implementation of the "The Least Recently Used" part of the cache
	 */
	LruCacheInit();

	globalLock.unlock();

	return 0;
}

static struct Item* hashGetItem(std::string& key)
{
	hashLock.lock();

	auto it =  hash.find(key);
	if (it != hash.end()) {
		GetItem(it->second);
		hashLock.unlock();
		return it->second;
	}

	hashLock.unlock();

	return nullptr;
}

static inline int hashInsert(std::string& key, struct Item* item)
{
	hashLock.lock();
	hash[key] = item;
	hashLock.unlock();
	//TODO: need to fix this
	return 0;
}

int MemcachedInsert(std::string& key, std::string& value, int expiry)
{
	struct Item* item  = nullptr;

	item = hashGetItem(key);
	if (item) {
		LruTouchItem(item);
		PutItem(item);
		return 0;
	}

	int ret = LruCacheInsert(key, value, expiry, item);
	if (ret)
		return ret;

	hashInsert(key, item);

 	return 0;
}

int MemcachedGet(std::string& key, std::string* value)
{
	if (!value)
		return -1;

	struct Item* item = hashGetItem(key);
	if (!item) {
		LogMsg(LOG_HIGH, "%s: Item not in Hash", __func__);
		return -1;
	}

	/*
	 * On Item access move the item to the top of LRU, this will help
	 * keep least frequently used elements at the rear end of the list
	 * and will make the eviction process simpler
	 */
	LruTouchItem(item);

	if (value->length() < item->dataLength)
		value->resize(item->dataLength);

	value->assign(&item->data[item->keyLength],
			&item->data[item->keyLength] + item->dataLength);

	PutItem(item);
	return 0;
}

int MemcachedDelete(std::string& key)
{
	hashLock.lock();

	auto it = hash.find(key);
	if (it != hash.end()) {
		if (!PutItem(it->second))
			hash.erase(it);
		hashLock.unlock();
		return 0;
	}

	hashLock.unlock();

	return 0;
}

// TODO
// Logging
// Add custom doubly list implementation to fix LRU code
// The actualy copy of the Key-Value pairs to memcached

// API documentation and return types
// complete TODOs
// Item destruction and cleaning up LRU
// slab locking
// memcached init
