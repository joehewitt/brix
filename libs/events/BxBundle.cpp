#include <BxEvents/BxEvents.h>

#include <CoreFoundation/CoreFoundation.h>

extern "C" int64_t
brixMainBundleString(const char* key, char* buffer, int64_t capacity) {
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (!key || !bundle || !CFBundleGetIdentifier(bundle)) {
        return -1;
    }
    CFStringRef name = CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8);
    if (!name) {
        return -1;
    }
    CFTypeRef value = CFBundleGetValueForInfoDictionaryKey(bundle, name);
    CFRelease(name);
    if (!value || CFGetTypeID(value) != CFStringGetTypeID()) {
        return -1;
    }
    CFStringRef text = static_cast<CFStringRef>(value);
    CFRange range = CFRangeMake(0, CFStringGetLength(text));
    CFIndex needed = 0;
    CFStringGetBytes(text, range, kCFStringEncodingUTF8, 0, false, nullptr, 0, &needed);
    if (needed <= capacity) {
        CFStringGetBytes(text, range, kCFStringEncodingUTF8, 0, false,
            reinterpret_cast<UInt8*>(buffer), capacity, nullptr);
    }
    return needed;
}
