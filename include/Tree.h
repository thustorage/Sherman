#if !defined(_TREE_H_)
#define _TREE_H_

#include "DSM.h"
#include <atomic>
#include <city.h>
#include <functional>
#include <iostream>

class IndexCache;

struct LocalLockNode {
  std::atomic<uint64_t> ticket_lock;
  bool hand_over;
  uint8_t hand_time;
};

struct Request {
  bool is_search;
  Key k;
  Value v;
};

class RequstGen {
public:
  RequstGen() = default;
  virtual Request next() { return Request{}; }
};

using CoroFunc = std::function<RequstGen *(int, DSM *, int)>;

struct SearchResult {
  bool is_leaf;
  uint8_t level;
  GlobalAddress slibing;
  GlobalAddress next_level;
  Value val;
};

class InternalPage;
class LeafPage;
class Tree {

public:
  Tree(DSM *dsm, uint16_t tree_id = 0);

  void insert(const Key &k, const Value &v, CoroContext *cxt = nullptr,
              int coro_id = 0);
  bool search(const Key &k, Value &v, CoroContext *cxt = nullptr,
              int coro_id = 0);
  void del(const Key &k, CoroContext *cxt = nullptr, int coro_id = 0);

  uint64_t range_query(const Key &from, const Key &to, Value *buffer,
                       CoroContext *cxt = nullptr, int coro_id = 0);

  void print_and_check_tree(CoroContext *cxt = nullptr, int coro_id = 0);

  void run_coroutine(CoroFunc func, int id, int coro_cnt);

  void lock_bench(const Key &k, CoroContext *cxt = nullptr, int coro_id = 0);

  void index_cache_statistics();
  void clear_statistics();

private:
  DSM *dsm;
  uint64_t tree_id;
  GlobalAddress root_ptr_ptr; // the address which stores root pointer;

  // static thread_local int coro_id;
  static thread_local CoroCall worker[define::kMaxCoro];
  static thread_local CoroCall master;

  LocalLockNode *local_locks[MAX_MACHINE];

  IndexCache *index_cache;

  void print_verbose();

  void before_operation(CoroContext *cxt, int coro_id);

  GlobalAddress get_root_ptr_ptr();
  GlobalAddress get_root_ptr(CoroContext *cxt, int coro_id);

  void coro_worker(CoroYield &yield, RequstGen *gen, int coro_id);
  void coro_master(CoroYield &yield, int coro_cnt);

  void broadcast_new_root(GlobalAddress new_root_addr, int root_level);
  bool update_new_root(GlobalAddress left, const Key &k, GlobalAddress right,
                       int level, GlobalAddress old_root, CoroContext *cxt,
                       int coro_id);

  void insert_internal(const Key &k, GlobalAddress v, CoroContext *cxt,
                       int coro_id, int level);

  bool try_lock_addr(GlobalAddress lock_addr, uint64_t tag, uint64_t *buf,
                     CoroContext *cxt, int coro_id);
  void unlock_addr(GlobalAddress lock_addr, uint64_t tag, uint64_t *buf,
                   CoroContext *cxt, int coro_id, bool async);
  void write_page_and_unlock(char *page_buffer, GlobalAddress page_addr,
                             int page_size, uint64_t *cas_buffer,
                             GlobalAddress lock_addr, uint64_t tag,
                             CoroContext *cxt, int coro_id, bool async);
  void lock_and_read_page(char *page_buffer, GlobalAddress page_addr,
                          int page_size, uint64_t *cas_buffer,
                          GlobalAddress lock_addr, uint64_t tag,
                          CoroContext *cxt, int coro_id);

  bool page_search(GlobalAddress page_addr, const Key &k, SearchResult &result,
                   CoroContext *cxt, int coro_id, bool from_cache = false);
  void internal_page_search(InternalPage *page, const Key &k,
                            SearchResult &result);
  void leaf_page_search(LeafPage *page, const Key &k, SearchResult &result);

  void internal_page_store(GlobalAddress page_addr, const Key &k,
                           GlobalAddress value, GlobalAddress root, int level,
                           CoroContext *cxt, int coro_id);
  bool leaf_page_store(GlobalAddress page_addr, const Key &k, const Value &v,
                       GlobalAddress root, int level, CoroContext *cxt,
                       int coro_id, bool from_cache = false);
  bool leaf_page_del(GlobalAddress page_addr, const Key &k, int level,
                     CoroContext *cxt, int coro_id, bool from_cache = false);

  bool acquire_local_lock(GlobalAddress lock_addr, CoroContext *cxt,
                          int coro_id);
  bool can_hand_over(GlobalAddress lock_addr);
  void releases_local_lock(GlobalAddress lock_addr);
};

class Header {
private:
  GlobalAddress leftmost_ptr;
  GlobalAddress sibling_ptr;
  uint8_t level;
  int16_t last_index;
  Key lowest;
  Key highest;

  friend class InternalPage;
  friend class LeafPage;
  friend class Tree;
  friend class IndexCache;

public:
  Header() {
    leftmost_ptr = GlobalAddress::Null();
    sibling_ptr = GlobalAddress::Null();
    last_index = -1;
    lowest = kKeyMin;
    highest = kKeyMax;
  }

  void debug() const {
    std::cout << "leftmost=" << leftmost_ptr << ", "
              << "sibling=" << sibling_ptr << ", "
              << "level=" << (int)level << ","
              << "cnt=" << last_index + 1 << ","
              << "range=[" << lowest << " - " << highest << "]";
  }
} __attribute__((packed));
;

class InternalEntry {
public:
  Key key;
  GlobalAddress ptr;

  InternalEntry() {
    ptr = GlobalAddress::Null();
    key = 0;
  }
} __attribute__((packed));

class LeafEntry {
public:
  uint8_t f_version : 4;
  Key key;
  Value value;
  uint8_t r_version : 4;

  LeafEntry() {
    f_version = 0;
    r_version = 0;
    value = kValueNull;
    key = 0;
  }
} __attribute__((packed));

constexpr int kInternalCardinality = (kInternalPageSize - sizeof(Header) -
                                      sizeof(uint8_t) * 2 - sizeof(uint64_t)) /
                                     sizeof(InternalEntry);

constexpr int kLeafCardinality =
    (kLeafPageSize - sizeof(Header) - sizeof(uint8_t) * 2 - sizeof(uint64_t)) /
    sizeof(LeafEntry);

class InternalPage {
private:
  union {
    uint32_t crc;
    uint64_t embedding_lock;
    uint64_t index_cache_freq;
  };

  uint8_t front_version;
  Header hdr;
  InternalEntry records[kInternalCardinality];

  // uint8_t padding[3];
  uint8_t rear_version;

  friend class Tree;
  friend class IndexCache;

public:
  // this is called when tree grows
  InternalPage(GlobalAddress left, const Key &key, GlobalAddress right,
               uint32_t level = 0) {
    hdr.leftmost_ptr = left;
    hdr.level = level;
    records[0].key = key;
    records[0].ptr = right;
    records[1].ptr = GlobalAddress::Null();

    hdr.last_index = 0;

    front_version = 0;
    rear_version = 0;
  }

  InternalPage(uint32_t level = 0) {
    hdr.level = level;
    records[0].ptr = GlobalAddress::Null();

    front_version = 0;
    rear_version = 0;

    embedding_lock = 0;
  }

  void set_consistent() {
    front_version++;
    rear_version = front_version;
#ifdef CONFIG_ENABLE_CRC
    this->crc =
        CityHash32((char *)&front_version, (&rear_version) - (&front_version));
#endif
  }

  bool check_consistent() const {

    bool succ = true;
#ifdef CONFIG_ENABLE_CRC
    auto cal_crc =
        CityHash32((char *)&front_version, (&rear_version) - (&front_version));
    succ = cal_crc == this->crc;
#endif
    succ = succ && (rear_version == front_version);

    return succ;
  }

  void debug() const {
    std::cout << "InternalPage@ ";
    hdr.debug();
    std::cout << "version: [" << (int)front_version << ", " << (int)rear_version
              << "]" << std::endl;
  }

  void verbose_debug() const {
    this->debug();
    for (int i = 0; i < this->hdr.last_index + 1; ++i) {
      printf("[%lu %lu] ", this->records[i].key, this->records[i].ptr.val);
    }
    printf("\n");
  }

} __attribute__((packed));

class LeafPage {
private:
  union {
    uint32_t crc;
    uint64_t embedding_lock;
  };
  uint8_t front_version;
  Header hdr;
  LeafEntry records[kLeafCardinality];

  // uint8_t padding[1];
  uint8_t rear_version;

  friend class Tree;

public:
  LeafPage(uint32_t level = 0) {
    hdr.level = level;
    records[0].value = kValueNull;

    front_version = 0;
    rear_version = 0;

    embedding_lock = 0;
  }

  void set_consistent() {
    front_version++;
    rear_version = front_version;
#ifdef CONFIG_ENABLE_CRC
    this->crc =
        CityHash32((char *)&front_version, (&rear_version) - (&front_version));
#endif
  }

  bool check_consistent() const {

    bool succ = true;
#ifdef CONFIG_ENABLE_CRC
    auto cal_crc =
        CityHash32((char *)&front_version, (&rear_version) - (&front_version));
    succ = cal_crc == this->crc;
#endif

    succ = succ && (rear_version == front_version);

    return succ;
  }

  void debug() const {
    std::cout << "LeafPage@ ";
    hdr.debug();
    std::cout << "version: [" << (int)front_version << ", " << (int)rear_version
              << "]" << std::endl;
  }

} __attribute__((packed));

#endif // _TREE_H_
