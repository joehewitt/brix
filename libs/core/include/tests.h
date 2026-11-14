
#ifndef YU_TESTS_H
#define YU_TESTS_H

#include <cstdio>

typedef struct {
    char* vtable;
    int uid;
    int field;
} YumiObject;

typedef struct {
    int a;
    const char* b;
} TestThingStruct;

typedef struct {
    double a;
    double b;
    double c;
} TestStructHFA;

typedef struct {
    int a;
    int b;
} TestStructSmall1;

typedef struct {
    int a;
    int b;
    int c;
    int d;
} TestStructSmall2;

typedef struct {
    int a;
    int b;
    int c;
    int d;
    int e;
} TestStructLarge;

typedef struct {
    int a;
    int b;
    int c;
    int d;
    TestStructSmall1 e;
} TestStructLargeNested;

typedef long Poopy;

typedef int (*TestCallback)(int);
typedef int (*TestMethodCallback)(void*, int);
typedef int (*TestTextCallback)(const char*);

extern "C" void ctest1();
extern "C" void ctest2a(int n);
extern "C" void ctest2aa(int n);
extern "C" void ctest2b(bool n);
extern "C" void ctest2c(double n);
extern "C" void ctest2d(long long n);
extern "C" void ctest2e(char* n);
extern "C" void ctest2f(YumiObject* n);
extern "C" void ctest2g(const char* n);
extern "C" int ctest4a();
extern "C" long long ctest4b();
extern "C" double ctest4c();
extern "C" const char* ctest4d();
extern "C" int ctest5(int a, int b);
extern "C" int ctestCallback(TestCallback cb);
extern "C" int ctestMethodCallback(TestMethodCallback cb);
extern "C" int ctestClobberedCallback(TestTextCallback cb);
extern "C" void ctest13(YumiObject* n, char* c);
extern "C" const char* ctest14();
extern "C" void ctest15(TestThingStruct* n);

extern "C" int ctestStructHFA(TestStructHFA data);
extern "C" int ctestStructHFASpill(float a, float b, float c, float d, float e, float f, TestStructHFA data);
extern "C" int ctestStructSmall1(TestStructSmall1 data);
extern "C" int ctestStructSmall2(TestStructSmall2 data);
extern "C" int ctestStructLarge(TestStructLarge data);
extern "C" int ctestStructLargeNested(TestStructLargeNested data);
extern "C" void ctestStackPacked(int a, int b, int c, int d, int e, int f, int g, int h, unsigned char i, unsigned char j, short k, int l);

extern "C" TestStructHFA ctestStructReturnHFA();
extern "C" TestStructSmall1 ctestStructReturnSmall1();
extern "C" TestStructSmall2 ctestStructReturnSmall2();
extern "C" TestStructLarge ctestStructReturnLarge();

#endif