#include "Cache.h"

Cache::Cache(const CacheConfig &cache_config) {
    size = cache_config.cacheSize;
    data = (uint64_t)hugePageAlloc(size * define::GB);
}