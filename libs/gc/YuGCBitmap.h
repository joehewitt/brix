#ifndef YU_GC_BITMAP_H
#define YU_GC_BITMAP_H

#include "YuGCCommon.h"

namespace YuGC {

struct GCBitmap {
    size_t bitCount = 0;
    std::vector<unsigned char> bytes;

    GCBitmap();

    explicit GCBitmap(size_t count);

    bool get(size_t index) const;

    void set(size_t index);

    void clear(size_t index);

    void clearAll();

    bool findFrom(bool value, size_t start, size_t& result) const;

    bool findFirstConsecutive(size_t count, bool value, size_t start, size_t& result) const;

    bool findBestFitConsecutive(size_t count, bool value, size_t& result) const;
};

} // namespace YuGC

#endif
