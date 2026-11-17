
#include <BxGC/BxGCRuntime.h>
#include <BxGC/BxGCStackMap.h>

// Standalone event-runtime harnesses can link without the GC allocator object.
extern "C" unsigned char __attribute__((weak)) brixInterruptRequestedFlags = 0;
extern "C" uint64_t __attribute__((weak)) brixGCPublicControlWord = 0;

extern "C" void __attribute__((weak))
brixGCSetActiveThreadCache(BrixGCThreadCache*) {
}

extern "C" BrixGCAllocationBuffer* __attribute__((weak))
brixGCGetActiveAllocationBuffer(void) {
    return NULL;
}

extern "C" void __attribute__((weak))
brixGCRuntimeFlushActiveAllocationBuffer(void) {
}

extern "C" void __attribute__((weak))
brixGCRuntimeRevokeActiveAllocationBuffer(void) {
}

extern "C" void __attribute__((weak))
brixGCScanContextRoots(
    const BrixGCContextSnapshot*,
    void*,
    size_t,
    BxGCStackRootVisitor,
    void*) {
}

extern "C" void __attribute__((weak))
brixGCScanConservativeContextRoots(
    const BrixGCContextSnapshot*,
    void*,
    size_t,
    BxGCStackRootVisitor,
    void*) {
}

extern "C" size_t __attribute__((weak))
brixGCStorageRootRecordCapacity(size_t, size_t) {
    return 0;
}

extern "C" void __attribute__((weak))
brixGCVisitStorageRootRecords(
    BrixGCStorageRootRecordVisitor,
    void*) {
}

extern "C" void __attribute__((weak))
brixGCVisitDescriptorPointerSlots(
    char*,
    size_t,
    void*,
    BrixGCDescriptorPointerSlotVisitor,
    void*) {
}

extern "C" bool __attribute__((weak))
brixGCIsManagedAddress(char*) {
    return false;
}
