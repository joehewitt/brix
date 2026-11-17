
#ifndef YU_NUMBER_H
#define YU_NUMBER_H

#include <time.h>
#include <YuGC/YuGCAnnotations.h>

extern "C" YUMI_WITHOUT_GC time_t yumiGetUTCTime();
extern "C" YUMI_WITHOUT_GC char* yumiGetTimeZone();
extern "C" YUMI_WITHOUT_GC int yumiGetTimeZoneOffset();
extern "C" YUMI_WITHOUT_GC int yumiGetTimeZoneHasDST();
extern "C" struct tm* yumiBreakDownUTCTime(time_t t, struct tm* buf);
extern "C" time_t yumiMakeTime(struct tm* tm);

#endif
