#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <cstdint>
#include <cassert>
#include <cstring>

#include <cstdio>

class BitMap {
   private:
    int n;
    uint64_t *bits;

   public:
    BitMap(int n) : n(n) {

        assert(n % 64 == 0);
        bits = new uint64_t[n / 64];
        memset(bits, 0, n / 8);
    }

    bool get(int pos) { return (bits[pos / 64] >> (pos % 64)) & 0x1; }

    void set(int pos) {
        uint64_t &v = bits[pos / 64];
        v = v | (0x1ull << (pos % 64));
    }

    void clear(int pos) {
        uint64_t &v = bits[pos / 64];
        v = v & ~(0x1ull << (pos % 64));
    }

    int setZeroPos() {
        for (int i = 0; i < n / 64; ++i) {
            uint64_t &v = bits[i];
            uint64_t b = ~v;
            if (b) {
                uint64_t pos = __builtin_ctzll(b);

                v = v | (0x1ull << pos);

                return i * 64 + pos;
            }
        }

        assert(false);
    }

    ~BitMap() { delete[] bits; }
};

#endif /* __BITMAP_H__ */
