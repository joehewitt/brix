#pragma once
#include <atomic>
class YuTaskQueue;
class YuTask {
public:
    std::atomic<bool> completed {false};
    YuTaskQueue* getTaskQueue();
};
