#ifndef YU_TASK_MONITOR_H
#define YU_TASK_MONITOR_H

#include <atomic>
#include <stdint.h>

class YuThread;
class YuTask;

// *************************************************************************************************

extern std::atomic<bool> gYuTaskMonitorSleeping;

void yumiStartTaskMonitor();
void yumiStopTaskMonitor();
void yumiRegisterMonitoredThread(YuThread* thread);
void yumiUnregisterMonitoredThread(YuThread* thread);
void yumiWakeTaskMonitor();
void yumiInterruptRequestAdded();
void yumiInterruptRequestRemoved();
void yumiServiceTaskInterrupts(bool canYield, uintptr_t fp, uintptr_t pc);
void yumiReportLongTask(YuThread* thread, YuTask* task, bool hasStack, uintptr_t fp = 0,
    uintptr_t pc = 0);

inline void
yumiNoteMonitoredThreadRunning() {
    if (gYuTaskMonitorSleeping.load(std::memory_order_seq_cst)) {
        yumiWakeTaskMonitor();
    }
}

#endif
