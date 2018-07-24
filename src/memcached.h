#ifndef MEMCACHED
#define MEMCACHED

#include <iostream>

/*
 * Top level API file
 */
int MemcachedInit();

void MemcachedCleanUp();

int MemcachedInsert(std::string& key, std::string& value, int expiry);

int MemcachedGet(std::string& key, std::string* value);

int MemcachedDelete(std::string& key);
#endif /* ifndef MEMCACHED */


