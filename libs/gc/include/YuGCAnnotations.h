#ifndef YU_GC_ANNOTATIONS_H
#define YU_GC_ANNOTATIONS_H

#if defined(__clang__)
#define YUMI_GC_LEAF __attribute__((annotate("yumi:gc_leaf")))
#define YUMI_GC_BLOCKING __attribute__((annotate("yumi:gc_blocking")))
#define YUMI_GC_POLLING __attribute__((annotate("yumi:gc_polling")))
#else
#define YUMI_GC_LEAF
#define YUMI_GC_BLOCKING
#define YUMI_GC_POLLING
#endif

#endif
