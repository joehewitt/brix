#include "YuCore/YuBatchTest.h"

#include <climits>
#include <cstdio>
#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>

// *************************************************************************************************

static int savedStdout = -1;
static int savedStderr = -1;
static char savedCwd[PATH_MAX] = {0};

extern "C" int
yumiBatchStartIndex() {
    const char* value = getenv("YUMITEST_BATCH_START_INDEX");
    return value ? atoi(value) : 0;
}

extern "C" int
yumiBatchEndIndex() {
    const char* value = getenv("YUMITEST_BATCH_END_INDEX");
    return value ? atoi(value) : INT_MAX;
}

static void
yumiBatchAppendControl(
    const char* controlPath,
    const char* event,
    const char* testId,
    long long stdoutOffset,
    long long stderrOffset) {
    if (!controlPath || !controlPath[0]) {
        return;
    }

    FILE* file = fopen(controlPath, "a");
    if (!file) {
        return;
    }

    fprintf(
        file,
        "%s\t%s\t%lld\t%lld\n",
        event,
        testId ? testId : "",
        stdoutOffset,
        stderrOffset);
    fflush(file);
    fclose(file);
}

static long long
yumiBatchOutputOffset(const char* outputPath) {
    if (!outputPath || !outputPath[0]) {
        return 0;
    }

    int fd = open(outputPath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        return 0;
    }

    off_t offset = lseek(fd, 0, SEEK_END);
    close(fd);
    return offset >= 0 ? (long long)offset : 0;
}

static long long
yumiBatchFdOffset(int fd) {
    off_t offset = lseek(fd, 0, SEEK_CUR);
    return offset >= 0 ? (long long)offset : 0;
}

static void
yumiBatchRedirectFd(int fd, const char* outputPath, int* savedFd) {
    if (!outputPath || !outputPath[0]) {
        return;
    }

    if (*savedFd >= 0) {
        close(*savedFd);
        *savedFd = -1;
    }
    *savedFd = dup(fd);
    int outputFd = open(outputPath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (outputFd >= 0) {
        lseek(outputFd, 0, SEEK_END);
        dup2(outputFd, fd);
        close(outputFd);
    }
}

extern "C" void
yumiBatchTestStart(
    const char* controlPath,
    const char* testId,
    const char* workDirPath,
    const char* stdoutPath,
    const char* stderrPath) {
    fflush(stdout);
    fflush(stderr);
    long long stdoutStart = yumiBatchOutputOffset(stdoutPath);
    long long stderrStart = yumiBatchOutputOffset(stderrPath);
    yumiBatchAppendControl(controlPath, "started", testId, stdoutStart, stderrStart);

    getcwd(savedCwd, sizeof(savedCwd));
    yumiBatchRedirectFd(STDOUT_FILENO, stdoutPath, &savedStdout);
    yumiBatchRedirectFd(STDERR_FILENO, stderrPath, &savedStderr);

    if (workDirPath && workDirPath[0]) {
        chdir(workDirPath);
    }
}

extern "C" void
yumiBatchTestFinish(const char* controlPath, const char* testId) {
    fflush(stdout);
    fflush(stderr);
    long long stdoutEnd = savedStdout >= 0 ? yumiBatchFdOffset(STDOUT_FILENO) : 0;
    long long stderrEnd = savedStderr >= 0 ? yumiBatchFdOffset(STDERR_FILENO) : 0;

    const char* keepRedirects = getenv("YUMITEST_BATCH_KEEP_REDIRECTS");
    if (!keepRedirects || keepRedirects[0] != '1') {
        if (savedStdout >= 0) {
            dup2(savedStdout, STDOUT_FILENO);
            close(savedStdout);
            savedStdout = -1;
        }
        if (savedStderr >= 0) {
            dup2(savedStderr, STDERR_FILENO);
            close(savedStderr);
            savedStderr = -1;
        }
        if (savedCwd[0]) {
            chdir(savedCwd);
            savedCwd[0] = 0;
        }
    }

    yumiBatchAppendControl(controlPath, "finished", testId, stdoutEnd, stderrEnd);
}
