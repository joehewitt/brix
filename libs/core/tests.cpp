
#include <YuCore/tests.h>

extern "C" void ctest1() {
    printf("received!");
}

extern "C" void ctest2a(int n) {
    printf("received %d", n);
}

extern "C" void ctest2aa(int n) {
    printf("received aa %d", n);
}

extern "C" void ctest2b(bool n) {
    printf("received %d", n);
}

extern "C" void ctest2c(double n) {
    printf("received %f", n);
}

extern "C" void ctest2d(long long n) {
    printf("received %lld", n);
}

extern "C" void ctest2e(char* n) {
    printf("received '%s'", n);
}

extern "C" void ctest2f(YumiObject* n) {
    printf("received %d", n->field);
}

extern "C" void ctest2g(const char* n) {
    if (n) {
        printf("received '%s'\n", n);
    } else {
        printf("received null\n");
    }
}

extern "C" int ctest4a() {
    return 42;
}

extern "C" long long ctest4b() {
    return 9223372036854775807L;
}

extern "C" double ctest4c() {
    return 42.1234;
}

extern "C" const char* ctest4d() {
    return "foo";
}

extern "C" int ctest5(int a, int b) {
    return a + b;
}

extern "C" int ctestCallback(TestCallback cb) {
    return cb(42);
}

extern "C" int ctestMethodCallback(TestMethodCallback cb) {
    return cb(NULL, 42);
}

extern "C" int ctestClobberedCallback(TestTextCallback cb) {
    static const char text[] = "clobbered";
#if defined(__aarch64__)
    long result;
    asm volatile(
        "mov x27, #6\n"
        "mov x0, %1\n"
        "blr %2\n"
        "mov %0, x0\n"
        : "=r"(result)
        : "r"(text), "r"(cb)
        : "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12",
          "x13", "x14", "x15", "x16", "x17", "x27", "x30", "v0", "v1", "v2", "v3", "v4", "v5",
          "v6", "v7", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25",
          "v26", "v27", "v28", "v29", "v30", "v31", "cc", "memory");
    return (int)result;
#else
    return cb(text);
#endif
}

extern "C" void ctest13(YumiObject* n, char* c) {
    printf("received %d %s", !!n, c);
}

extern "C" const char* ctest14() {
    return NULL;
}

extern "C" void ctest15(TestThingStruct* n) {
    n->a = 42;
    n->b = "hi there";
}

extern "C" int ctestStructHFA(TestStructHFA data) {
    printf("a %f b %f c %f\n", data.a, data.b, data.c);
    return 0;
// /    data->a = 42;
}

extern "C" int ctestStructHFASpill(float a, float b, float c, float d, float e, float f, TestStructHFA data) {
    printf("a %f b %f c %f d %f e %f f %f\n", a, b, c, d, e, f);
    printf("a %f b %f c %f\n", data.a, data.b, data.c);
    return 0;
// /    data->a = 42;
}

extern "C" int ctestStructSmall1(TestStructSmall1 data) {
    printf("a %d b %d\n", data.a, data.b);
    return 0;
// /    data->a = 42;
}

extern "C" int ctestStructSmall2(TestStructSmall2 data) {
    printf("a %d b %d c %d d %d\n", data.a, data.b, data.c, data.d);
    return 0;
// /    data->a = 42;
}

extern "C" int ctestStructLarge(TestStructLarge data) {
    printf("a %d b %d c %d d %d e %d\n", data.a, data.b, data.c, data.d, data.e);
    data.a = 42;
    return 0;
}

extern "C" int ctestStructLargeNested(TestStructLargeNested data) {
    printf("a %d b %d c %d d %d\n", data.a, data.b, data.c, data.d);
    printf("e.a %d e.b %d\n", data.e.a, data.e.b);
    data.a = 42;
    return 0;
}

extern "C" void ctestStackPacked(int a, int b, int c, int d, int e, int f, int g, int h, unsigned char i, unsigned char j, short k, int l) {
    printf("h %d i %d j %d k %d l %d\n", h, i, j, k, l);
}

extern "C" TestStructHFA ctestStructReturnHFA() {
    TestStructHFA x;
    x.a = 1.2;
    x.b = 2.3;
    x.c = 3.4;
    return x;
}

extern "C" TestStructSmall1 ctestStructReturnSmall1() {
    TestStructSmall1 x;
    x.a = 1;
    x.b = 2;
    return x;
}

extern "C" TestStructSmall2 ctestStructReturnSmall2() {
    TestStructSmall2 x;
    x.a = 1;
    x.b = 2;
    x.c = 3;
    x.d = 4;
    return x;
}

extern "C" TestStructLarge ctestStructReturnLarge() {
    TestStructLarge x;
    x.a = 1;
    x.b = 2;
    x.c = 3;
    x.d = 4;
    x.e = 5;
    return x;
}
