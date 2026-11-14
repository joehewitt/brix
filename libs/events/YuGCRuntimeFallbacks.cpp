
#include <YuGC/YuGCRuntime.h>
#include <YuGC/YuGCStackMap.h>

// Standalone event-runtime harnesses can link without the GC allocator object.
extern "C" unsigned char __attribute__((weak)) yumiInterruptRequestedFlags = 0;
extern "C" uint64_t __attribute__((weak)) yumiGCPublicControlWord = 0;

extern "C" void __attribute__((weak))
yumiGCSetActiveThreadCache(YumiGCThreadCache*) {
}

extern "C" YumiGCAllocationBuffer* __attribute__((weak))
yumiGCGetActiveAllocationBuffer(void) {
    return NULL;
}

extern "C" void __attribute__((weak))
yumiGCRuntimeFlushActiveAllocationBuffer(void) {
}

extern "C" void __attribute__((weak))
yumiGCRuntimeRevokeActiveAllocationBuffer(void) {
}

extern "C" void __attribute__((weak))
yumiGCScanContextRoots(
    const YumiGCContextSnapshot*,
    void*,
    size_t,
    YuGCStackRootVisitor,
    void*) {
}

extern "C" void __attribute__((weak))
yumiGCScanConservativeContextRoots(
    const YumiGCContextSnapshot*,
    void*,
    size_t,
    YuGCStackRootVisitor,
    void*) {
}

extern "C" size_t __attribute__((weak))
yumiGCStorageRootRecordCapacity(size_t, size_t) {
    return 0;
}

extern "C" void __attribute__((weak))
yumiGCVisitStorageRootRecords(
    YumiGCStorageRootRecordVisitor,
    void*) {
}

extern "C" void __attribute__((weak))
yumiGCVisitDescriptorPointerSlots(
    char*,
    size_t,
    void*,
    YumiGCDescriptorPointerSlotVisitor,
    void*) {
}

extern "C" bool __attribute__((weak))
yumiGCIsManagedAddress(char*) {
    return false;
}
