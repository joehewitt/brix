
#ifndef BX_NUMBER_H
#define BX_NUMBER_H

#include <BxGC/BxGCAnnotations.h>
#include <BxCore/BxString.h>

extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixIntToString(int num, int padding);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixUintToString(
    unsigned int num,
    int padding
);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixLonglongToString(
    long long num,
    int padding
);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixUlonglongToString(
    unsigned long long num, int padding);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixFloatToString(float num);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixDoubleToString(double num);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixDoubleToFixedString(
    double num, int digits, int trims);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixToHex(unsigned long long num, int size);
extern "C" BRIX_WITHOUT_GC double brixModf64(double n, double* rem);

#endif
