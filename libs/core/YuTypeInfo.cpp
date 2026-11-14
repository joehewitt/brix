
#include "YuCore/YuTypeInfo.h"

#include <cstdio>
#include <cstdlib>
#include <string>

// *************************************************************************************************

YuTypeInfo::YuTypeInfo(const char* name)
    : _name(name)
{
}

bool
YuTypeInfo::canCatch(const YuTypeInfo* other) const {
    // printf("Can catch %s == %s\n", _name, other->name()); fflush(stdout);
    return other == this;
}
