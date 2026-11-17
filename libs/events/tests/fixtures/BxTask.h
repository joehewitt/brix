#pragma once
#include <atomic>
class BxTaskQueue;
class BxTask {
public:
    std::atomic<bool> completed {false};
    BxTaskQueue* getTaskQueue();
};
