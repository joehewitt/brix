
#ifndef YU_NUMBER_H
#define YU_NUMBER_H

#include <YuGC/YuGCAnnotations.h>
#include <YuCore/YuString.h>

extern "C" YUMI_GC_POLLING const YumiStringData* yumiIntToString(int num, int padding);
extern "C" YUMI_GC_POLLING const YumiStringData* yumiUintToString(unsigned int num, int padding);
extern "C" YUMI_GC_POLLING const YumiStringData* yumiLonglongToString(long long num, int padding);
extern "C" YUMI_GC_POLLING const YumiStringData* yumiUlonglongToString(
    unsigned long long num, int padding);
extern "C" YUMI_GC_POLLING const YumiStringData* yumiFloatToString(float num);
extern "C" YUMI_GC_POLLING const YumiStringData* yumiDoubleToString(double num);
extern "C" YUMI_GC_POLLING const YumiStringData* yumiDoubleToFixedString(
    double num, int digits, int trims);
extern "C" YUMI_GC_POLLING const YumiStringData* yumiToHex(unsigned long long num, int size);
extern "C" YUMI_GC_LEAF double yumiModf64(double n, double* rem);

#endif
