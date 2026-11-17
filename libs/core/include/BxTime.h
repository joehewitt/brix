
#ifndef BX_NUMBER_H
#define BX_NUMBER_H

#include <time.h>
#include <BxGC/BxGCAnnotations.h>

extern "C" BRIX_WITHOUT_GC time_t brixGetUTCTime();
extern "C" BRIX_WITHOUT_GC char* brixGetTimeZone();
extern "C" BRIX_WITHOUT_GC int brixGetTimeZoneOffset();
extern "C" BRIX_WITHOUT_GC int brixGetTimeZoneHasDST();
extern "C" struct tm* brixBreakDownUTCTime(time_t t, struct tm* buf);
extern "C" time_t brixMakeTime(struct tm* tm);

#endif
