#include <time.h>

#include <BxCore/BxTime.h>

// *************************************************************************************************

extern "C" time_t
brixGetUTCTime() {
    return time(NULL);
}

extern "C" char*
brixGetTimeZone() {
    tzset();
    return tzname[0];
}

extern "C" int
brixGetTimeZoneOffset() {
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);
    return (int)local.tm_gmtoff;
}

extern "C" int
brixGetTimeZoneHasDST() {
    tzset();
    return daylight;
}

extern "C" struct tm*
brixBreakDownUTCTime(time_t t, struct tm* buf) {
    buf[0] = *gmtime(&t);
    return buf;
}

extern "C" time_t
brixMakeTime(struct tm* tm) {
    return timegm(tm);
}
