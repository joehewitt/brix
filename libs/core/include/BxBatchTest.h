#ifndef BX_BATCH_TEST_H
#define BX_BATCH_TEST_H

// *************************************************************************************************

extern "C" int brixBatchStartIndex();
extern "C" int brixBatchEndIndex();
extern "C" void brixBatchTestStart(
    const char* controlPath,
    const char* testId,
    const char* workDirPath,
    const char* stdoutPath,
    const char* stderrPath);
extern "C" void brixBatchTestFinish(const char* controlPath, const char* testId);

#endif
