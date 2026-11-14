#pragma once
#include <YuEvents/YuTask.h>
class YuTaskQueue {
public:
    void scheduleTask(YuTask* task, bool = false) { task->completed.store(true); }
};
