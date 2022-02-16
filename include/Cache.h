#if !defined(_CACHE_H_)
#define _CACHE_H_

#include "Config.h"
#include "HugePageAlloc.h"

class Cache {

public:
  Cache(const CacheConfig &cache_config);

  uint64_t data;
  uint64_t size;

private:
};

#endif // _CACHE_H_
