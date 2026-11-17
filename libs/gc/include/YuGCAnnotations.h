#ifndef YU_GC_ANNOTATIONS_H
#define YU_GC_ANNOTATIONS_H

#if defined(__clang__)
#define YUMI_WITHOUT_GC __attribute__((annotate("yumi:gc_leaf")))
#define YUMI_WITHOUT_BLOCKING __attribute__((annotate("yumi:gc_polling")))
#else
#define YUMI_WITHOUT_GC
#define YUMI_WITHOUT_BLOCKING
#endif

#endif
