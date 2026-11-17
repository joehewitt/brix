#ifndef BX_GC_ANNOTATIONS_H
#define BX_GC_ANNOTATIONS_H

#if defined(__clang__)
#define BRIX_WITHOUT_GC __attribute__((annotate("brix:gc_leaf")))
#define BRIX_WITHOUT_BLOCKING __attribute__((annotate("brix:gc_polling")))
#else
#define BRIX_WITHOUT_GC
#define BRIX_WITHOUT_BLOCKING
#endif

#endif
