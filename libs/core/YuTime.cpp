#include <time.h>

#include <YuCore/YuTime.h>

// *************************************************************************************************

extern "C" time_t
yumiGetUTCTime() {
    return time(NULL);
}

extern "C" char*
yumiGetTimeZone() {
    tzset();
    return tzname[0];
}

extern "C" int
yumiGetTimeZoneOffset() {
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);
    return (int)local.tm_gmtoff;
}

extern "C" int
yumiGetTimeZoneHasDST() {
    tzset();
    return daylight;
}

extern "C" struct tm*
yumiBreakDownUTCTime(time_t t, struct tm* buf) {
    buf[0] = *gmtime(&t);
    return buf;
}

extern "C" time_t
yumiMakeTime(struct tm* tm) {
    return timegm(tm);
}
