#if !defined(_CACHE_ENTRY_H_)
#define _CACHE_ENTRY_H_

#include "Common.h"
#include "Tree.h"

struct CacheEntry {
  Key from;
  Key to; // [from, to]
  mutable InternalPage *ptr;
}
 __attribute__((packed));

static_assert(sizeof(CacheEntry) == 2 * sizeof(Key) + sizeof(uint64_t), "XXX");

inline std::ostream &operator<<(std::ostream &os, const CacheEntry &obj) {
  os << "[" << (int)obj.from << ", " << obj.to + 1 << ")";
  return os;
}

inline static CacheEntry Decode(const char *val) { return *(CacheEntry *)val; }

struct CacheEntryComparator {
  typedef CacheEntry DecodedType;

  static DecodedType decode_key(const char *b) { return Decode(b); }

  int cmp(const DecodedType a_v, const DecodedType b_v) const {
    if (a_v.to < b_v.to) {
      return -1;
    }

    if (a_v.to > b_v.to) {
      return +1;
    }

    if (a_v.from < b_v.from) {
      return +1;
    } else if (a_v.from > b_v.from) {
      return -1;
    } else {
      return 0;
    }
  }

  int operator()(const char *a, const char *b) const {
    return cmp(Decode(a), Decode(b));
  }

  int operator()(const char *a, const DecodedType b) const {
    return cmp(Decode(a), b);
  }
};

#endif // _CACHE_ENTRY_H_
