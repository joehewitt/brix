# Worker Pools

Yumi runs tasks in two ways.

## Default Queues

`do new` puts a task on the queue of the task that spawned it. The main queue and a one-thread
`Worker` run one task at a time on one thread. A task runs until it suspends: on a channel, a
socket, a timer or a `Mutex`. Code on a default queue never sees another task change shared data
between two suspensions, so it needs no locks. GUI code depends on this.

Set `YUMI_LONG_TASK_WARNINGS=1` to find tasks that hold a default queue. With it set, a task that
runs for 100 ms without suspending while other tasks wait on its queue prints a warning on
`stderr`. It names the task, where it was spawned, and the stack at its next loop poll. If the task
is in native code instead, the warning prints when the task next suspends, without a stack. The
task keeps running. The warning is off by default.

## Pools

`Worker(n)` with `n` above 1 is a pool. `do (pool) new` starts a task in it, and `do new` inside a
pool task starts another task in the same pool. `do new thread` uses the runtime's shared pool.

Pool tasks run on any of the pool's threads and move between them at every suspension. Each
thread keeps its own run queue. A task woken on a pool thread is queued on that thread. An idle
thread steals half of another thread's queue. A new task wakes one idle thread, not all of them.

A pool task that runs for 10 ms while other pool tasks wait is preempted at its next loop poll and
goes to the back of the pool's shared queue. Preemption never happens on a default queue, in code
kept `without suspend` or `without gc`, or while native code that called back into Yumi is on the
task's stack. Loop polls cost nothing more for this: the runtime handles preemption in the same call that
answers a GC handshake.

Run only code written for threads in a pool. Pool tasks share memory with no implicit locking.

## What Is Safe To Share Across Pool Threads

- `SyncChannel` and `SyncBufferedChannel`.
- `Mutex`. A contended `lock` parks the task, not the thread. The lock belongs to the task, so a
  task may suspend or move threads while it holds it, and unlock it from any thread.
- `Semaphore`.

Everything else is unsynchronized. Do not share these between pool tasks without a `Mutex`:

- `AsyncChannel`, `AsyncBufferedChannel`, `AsyncOnDemandChannel` and `OneShotChannel`.
- Broadcasters, subscriptions and `for … on` streams.
- Lists, maps, sets, tables and every other collection.
- Objects whose fields more than one task writes.

## HTTPServer

`HTTPServer` runs on its spawning task's queue by default, one task per connection, as in Node.
Set `worker` to a pool before `start` to run the accept loop and every connection in it. The
server guards its connection list with a `Mutex` either way. With a pool, the service and every
handler run on many threads at once, so they must follow the rules above.

```text
. pool = Worker(8)
. server = HTTPServer()
server.worker = pool
server.start()
```
