#pragma once
#include <BxEvents/BxTask.h>
class BxTaskQueue {
public:
    void scheduleTask(BxTask* task, bool = false) { task->completed.store(true); }
};
