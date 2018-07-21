#include <iostream>
#include <mutex>
#include <unordered_map>
#include <string>
#include "slabs.h"
#include "log.h"

std::unordered_map<std::string, struct Item*> hash;
std::mutex hashLock;

void MemcachedDestroy()
{

}

int MemcachedInit()
{
	const int atExitRet = std::atexit(MemcachedDestroy);
	if (atExitRet)
		return -1;

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
	struct Item* item;

	item = hashGetItem(key);
	if (item) {
		//update LRU; TODO
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

	if (value->length() < item->dataLength)
		value->resize(item->dataLength);

	value->assign(&item->data[0], &item->data[0] + item->dataLength);

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
// API documentation and return types
// complete TODOs
// Item destruction and cleaning up LRU
// slab locking
// memcached init
