#ifndef __WRLOCK_H__
#define __WRLOCK_H__

#include <atomic>

class WRLock {

private:
  std::atomic<uint16_t> l;
  const static uint16_t UNLOCKED = 0;
  const static uint16_t LOCKED = 1;

public:
  WRLock() { init(); }

  bool is_unlock() {
    return l.load() == UNLOCKED;
  }

  void init() { l.store(UNLOCKED); }

  void wLock() {
    while (true) {
      while (l.load(std::memory_order_relaxed) != UNLOCKED) {
        ;
      }

      uint16_t f = UNLOCKED;
      if (l.compare_exchange_strong(f, LOCKED)) {
        break;
      }
    }
  };

  bool try_wLock() {
    if (l.load(std::memory_order_relaxed) != UNLOCKED)
      return false;

    uint16_t f = UNLOCKED;
    return l.compare_exchange_strong(f, LOCKED);
  }

  void rLock() {
    while (true) {
      uint16_t v;
      while ((v = l.load(std::memory_order_relaxed)) == LOCKED) {
        ;
      }

      uint16_t b = v + 2;

      if (l.compare_exchange_strong(v, b)) {
        break;
      }
    }
  }

  bool try_rLock() {
  retry:
    uint16_t v = l.load(std::memory_order_relaxed);
    if (v == LOCKED)
      return false;

    uint16_t b = v + 2;
    if (!l.compare_exchange_strong(v, b)) {
      goto retry; // concurrent reader;
    }

    return true;
  }

  void rUnlock() {
    while (true) {
      uint16_t v = l.load();
      uint16_t b = v - 2;

      if (l.compare_exchange_strong(v, b)) {
        break;
      }
    }
  }

  void wUnlock() { l.store(UNLOCKED, std::memory_order_release); }
};

#endif /* __FAIRWRLOCK_H__ */
