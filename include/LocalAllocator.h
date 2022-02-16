#if !defined(_LOCAL_ALLOC_H_)
#define _LOCAL_ALLOC_H_

#include "Common.h"
#include "GlobalAddress.h"

#include <vector>

// for fine-grained shared memory alloc
// not thread safe
// now it is a simple log-structure alloctor
// TODO: slab-based alloctor
class LocalAllocator {

public:
  LocalAllocator() {
    head = GlobalAddress::Null();
    cur = GlobalAddress::Null();
  }

  GlobalAddress malloc(size_t size, bool &need_chunck, bool align = false) {

    if (align) {
    }

    GlobalAddress res = cur;
    if (log_heads.empty() ||
        (cur.offset + size > head.offset + define::kChunkSize)) {
      need_chunck = true;
    } else {
      need_chunck = false;
      cur.offset += size;
    }

    // assert(res.addr + size <= 40 * define::GB);

    return res;
  }

  void set_chunck(GlobalAddress &addr) {
    log_heads.push_back(addr);
    head = cur = addr;
  }

  void free(const GlobalAddress &addr) {
    // TODO
  }

private:
  GlobalAddress head;
  GlobalAddress cur;
  std::vector<GlobalAddress> log_heads;
};

#endif // _LOCAL_ALLOC_H_
