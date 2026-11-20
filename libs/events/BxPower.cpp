#include <BxEvents/BxEvents.h>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#include <IOKit/ps/IOPowerSources.h>
#endif
#endif

extern "C" bool
brixPowerIsLimited() {
#if defined(__APPLE__) && TARGET_OS_OSX
    return IOPSGetTimeRemainingEstimate() != kIOPSTimeRemainingUnlimited;
#else
    return false;
#endif
}
