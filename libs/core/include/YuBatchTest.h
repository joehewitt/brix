#ifndef YU_BATCH_TEST_H
#define YU_BATCH_TEST_H

// *************************************************************************************************

extern "C" int yumiBatchStartIndex();
extern "C" int yumiBatchEndIndex();
extern "C" void yumiBatchTestStart(
    const char* controlPath,
    const char* testId,
    const char* workDirPath,
    const char* stdoutPath,
    const char* stderrPath);
extern "C" void yumiBatchTestFinish(const char* controlPath, const char* testId);

#endif
