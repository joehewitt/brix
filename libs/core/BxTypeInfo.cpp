
#include "BxCore/BxTypeInfo.h"

#include <cstdio>
#include <cstdlib>
#include <string>

// *************************************************************************************************

BxTypeInfo::BxTypeInfo(const char* name)
    : _name(name)
{
}

bool
BxTypeInfo::canCatch(const BxTypeInfo* other) const {
    // printf("Can catch %s == %s\n", _name, other->name()); fflush(stdout);
    return other == this;
}
