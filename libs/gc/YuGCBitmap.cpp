#include "YuGCBitmap.h"

namespace YuGC {

GCBitmap::GCBitmap() = default;

GCBitmap::GCBitmap(size_t count)
    : bitCount(count),
      bytes((count + 7) / 8, 0) {}

bool
GCBitmap::get(size_t index) const {
    if (index >= bitCount) {
        return false;
    }
    return (bytes[index / 8] & (1u << (index % 8))) != 0;
}

void
GCBitmap::set(size_t index) {
    if (index < bitCount) {
        bytes[index / 8] |= static_cast<unsigned char>(1u << (index % 8));
    }
}

void
GCBitmap::clear(size_t index) {
    if (index < bitCount) {
        bytes[index / 8] &= static_cast<unsigned char>(~(1u << (index % 8)));
    }
}

void
GCBitmap::clearAll() {
    std::fill(bytes.begin(), bytes.end(), 0);
}

bool
GCBitmap::findFrom(bool value, size_t start, size_t& result) const {
    if (bitCount == 0) {
        return false;
    }
    if (start >= bitCount) {
        start = 0;
    }
    for (size_t index = start; index < bitCount; ++index) {
        if (get(index) == value) {
            result = index;
            return true;
        }
    }
    for (size_t index = 0; index < start; ++index) {
        if (get(index) == value) {
            result = index;
            return true;
        }
    }
    return false;
}

bool
GCBitmap::findConsecutiveFrom(size_t count, bool value, size_t start, size_t& result) const {
    if (count == 0 || count > bitCount) {
        return false;
    }
    if (start >= bitCount) {
        start = 0;
    }
    for (size_t scanned = 0; scanned < bitCount; ++scanned) {
        size_t index = start + scanned;
        if (index >= bitCount) {
            index -= bitCount;
        }
        if (index + count > bitCount) {
            continue;
        }
        bool found = true;
        for (size_t offset = 0; offset < count; ++offset) {
            if (get(index + offset) != value) {
                found = false;
                break;
            }
        }
        if (found) {
            result = index;
            return true;
        }
    }
    return false;
}

bool
GCBitmap::findBestFitConsecutive(size_t count, bool value, size_t& result) const {
    if (count == 0 || count > bitCount) {
        return false;
    }
    size_t bestLength = bitCount + 1;
    size_t index = 0;
    while (index < bitCount) {
        if (get(index) != value) {
            ++index;
            continue;
        }
        size_t start = index;
        while (index < bitCount && get(index) == value) {
            ++index;
        }
        size_t length = index - start;
        if (length >= count && length < bestLength) {
            result = start;
            bestLength = length;
            if (length == count) {
                break;
            }
        }
    }
    return bestLength <= bitCount;
}

} // namespace YuGC
