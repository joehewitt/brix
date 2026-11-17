#ifndef BX_TASK_MONITOR_H
#define BX_TASK_MONITOR_H

#include <atomic>
#include <stdint.h>

class BxThread;
class BxTask;

// *************************************************************************************************

extern std::atomic<bool> gBxTaskMonitorSleeping;

void brixStartTaskMonitor();
void brixStopTaskMonitor();
void brixRegisterMonitoredThread(BxThread* thread);
void brixUnregisterMonitoredThread(BxThread* thread);
void brixWakeTaskMonitor();
void brixInterruptRequestAdded();
void brixInterruptRequestRemoved();
void brixServiceTaskInterrupts(bool canYield, uintptr_t fp, uintptr_t pc);
void brixReportLongTask(BxThread* thread, BxTask* task, bool hasStack, uintptr_t fp = 0,
    uintptr_t pc = 0);

inline void
brixNoteMonitoredThreadRunning() {
    if (gBxTaskMonitorSleeping.load(std::memory_order_seq_cst)) {
        brixWakeTaskMonitor();
    }
}

#endif
