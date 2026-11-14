
#ifndef YU_NUMBER_H
#define YU_NUMBER_H

#include <time.h>
#include <YuGC/YuGCAnnotations.h>

extern "C" YUMI_GC_LEAF time_t yumiGetUTCTime();
extern "C" YUMI_GC_LEAF char* yumiGetTimeZone();
extern "C" YUMI_GC_LEAF int yumiGetTimeZoneOffset();
extern "C" YUMI_GC_LEAF int yumiGetTimeZoneHasDST();
extern "C" struct tm* yumiBreakDownUTCTime(time_t t, struct tm* buf);
extern "C" time_t yumiMakeTime(struct tm* tm);

#endif
