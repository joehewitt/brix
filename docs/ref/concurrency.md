# Concurrency

A program can run many tasks. Tasks on the main thread take turns, running one at a time until
each waits. Tasks on other threads, in a worker or in the shared pool, run in parallel. Channels
carry values from one task to another, and subscriptions run code for each value a channel
publishes.

This chapter covers tasks, workers, channels and subscriptions. Tracked properties, events and
change rules are in Reactivity.

## Tasks

A `do-new` form starts a task that runs the rows it holds. It is a step and gives no value.

```brix
(main
  (let numbers (channel Int))
  (do-new
    (for (in n (.. 1 3))
      (print (str "send " n))
      (<- numbers n)
    )
    (call (. numbers close))
    (print "closed")
  )
  (for (in n numbers)
    (print (str "got " n))
  )
  (print "done")
)
```

```output
send 1
got 1
send 2
got 2
send 3
got 3
closed
done
```

- **Taking turns.** A task started on the main thread runs only when the task that started it
  waits, as `main` does here when it reads the channel, or ends. Each task then runs until it
  waits again. On the main thread, only one task runs at a time.
- **One call.** `(do-new (inline (call send-all items)))`{.brix} starts a task that makes one
  call.
- **Priority.** `@priority` after `do-new` puts the task ahead of tasks without it.
- **Results.** A task gives no value, and its body cannot return one. A task passes results back
  through a channel, as in the next example, or a caller waits for a function with `await`; see
  [Waiting for a result](#waiting-for-a-result).
- **Errors.** An error or a panic that leaves a task's body ends the whole program with status 1,
  after the task's cleanups run. The report names the task and the task that started it.

### Where a task runs

| Form | Runs the task |
| --- | --- |
| `(do-new (inline (call work)))`{.brix} | On the queue of the task that starts it: the main queue from `main`, or a worker's queue from a task on that worker. |
| `(do-new-thread (inline (call work)))`{.brix} | On the shared pool of threads, in parallel with other tasks. |
| `(do-new (worker: w) (inline (call work)))`{.brix} | On the worker `w`; see [Workers](#workers). |

The main queue and a worker with one thread run their tasks in turn, in the order they were
started. Tasks in the shared pool, or on a worker with several threads, run in parallel and in no
set order, so a program that needs an order waits on channels.

### What a task sees

- **Locals** of the function that starts a task are copied into the task when it starts. The
  task's assignments to them stay in the task, and later changes outside do not reach it.
- **Objects and module values** are shared. A list the task adds to is the list everyone sees.
- **No checks.** The compiler does not stop two threads from changing the same object at once.
  Share objects between threads only through channels or a `Mutex`.

### Task handles

`(call Task)`{.brix} gives the handle of the task that calls it. A handle has `id`,
`is-finished` and `is-canceled`, and these methods:

| Method | Effect |
| --- | --- |
| `cancel` | Cancels the task. The next time it waits, it raises `TaskCancelationError`, a panic, and its cleanups run. |
| `suspend` | Makes the task wait until it is resumed, and gives whether it was canceled. |
| `resume-now`, `resume-later` | Resumes a suspended task. |
| `when-finished` | Runs a function in a new task when the task ends. |

### When a program ends

When `main`'s rows end, the functions given to `when-main-ends` run, then `main`'s cleanups, and
then the tasks that are still waiting to run. The program exits when no task is left. A task that
waits forever keeps the program from exiting; there is no deadlock detection.

## Waiting for a result

`(call (of await Int) function)`{.brix} runs a function in the shared pool and waits for its
value. The task that calls it waits, so other tasks on its queue can run.

```brix
(main
  (let results (channel Int))
  (for (in n (list 3 4 5))
    (do-new
      (<- results (call square n))
    )
  )
  (let total 0)
  (for (in _ (..< 0 3))
    (+= total (!! (<- results)))
  )
  (print (str "total " total))
  (print (str "awaited " (call (of await Int) (=> () (call square 6)))))
)

(fn-expr + square Int
  (param n Int)
  (* n n)
)
```

```output
total 50
awaited 36
```

- **`await`** without a type waits for a function that gives nothing. An error the function
  raises goes to `unhandled-error`, and the caller goes on; see Errors.
- **`Semaphore`** passes one value from a task that calls `post` to one that calls `await`. Each
  `post` waits until a task takes its value.
- **`Completion`** holds a value that one task finishes with `finish` and others `await`.

## Workers

A `Worker` is a queue of tasks with threads of its own. `(call Worker 1)`{.brix} makes one with
one thread, and a `do-new` form with a `worker:` option starts a task on it, as in
`(do-new (worker: w) (inline (call work)))`{.brix}.

```brix
(main
  (let worker (call Worker 1))
  (let results (channel Int))
  (for (in _ (..< 0 2))
    (do-new (worker: worker)
      (+= calls 1)
      (<- results calls)
    )
  )
  (print (<- results))
  (print (<- results))
  (print (str "main thread calls " calls))
  (call (. worker stop))
)

(prop + calls _ @thread (= 0))
```

```output
1
2
main thread calls 0
```

- **Threads.** `(call Worker 4)`{.brix} makes a worker with four threads, whose tasks run in
  parallel. `usable-core-count` gives the number of cores the program can use.
- **Stopping.** `stop` waits for the worker's threads to finish and stops it. Tasks started on it
  afterwards never run.
- **Tasks within tasks.** A `do-new` form in a task on a worker starts the new task on the same
  worker.
- **Idle workers** do not keep a program from exiting.

### Values for each thread, task or stack

A module property can hold a value for each thread, each task or each stack of calls:

| Form | Holds |
| --- | --- |
| `(prop + calls _ @thread (= 0))`{.brix} | One value for each thread. Tasks on the same thread share it. |
| `(prop + depth _ @task (= 0))`{.brix} | One value for each task. Each task starts with the initial value. |
| `(prop + scope _ @stack (= "none"))`{.brix} | One value for each stack of calls, given for a block by a `do-with` form. A task starts with the value it was started under. |

### Locks

A `Mutex` lets one task at a time run a block: a `do-use` form with the mutex locks it for the
rows the form holds, and other tasks that reach the lock wait. See Errors for `use`.

## Channels

A channel carries values from tasks that write them to tasks that read them. `(channel Int)`{.brix}
is a channel of `Int`, the type `SyncChannel'Int`. A declaration with a channel type and no value
makes the channel.

| Type | Writing | Reading | After `close` |
| --- | --- | --- | --- |
| `SyncChannel` | Waits until a reader takes the value. | Waits for a value. | Readers get the values still being written, then `null`. Writing panics: `Can't write to closed channel`. |
| `SyncBufferedChannel` | Adds the value to a queue with no limit, and never waits. | Waits while the queue is empty. | Readers empty the queue, then get `null`. Writes are dropped. |
| `AsyncChannel` | Publishes the value to every subscriber, and never waits; a value with no subscribers is lost. | Subscribes; see [Subscriptions](#subscriptions). | Subscribers get what was published, then end. Writing panics: `Can't write to closed broadcaster`. |
| `AsyncBufferedChannel` | As `AsyncChannel`, but keeps values while there are no subscribers and gives them to the first. | Subscribes, or reads its kept values. | As `AsyncChannel`. |

```brix
(main
  (let queue (of SyncBufferedChannel String))
  (<- queue "a" "b" "c")
  (print "queued 3 with no reader")
  (call (. queue close))
  (do-new
    (for (in item queue)
      (print (str "took " item))
    )
    (print "drained")
  )
  (print "main done")
)
```

```output
queued 3 with no reader
main done
took a
took b
took c
drained
```

| Form | Effect |
| --- | --- |
| `(<- channel value)`{.brix} | Writes a value, and gives the channel. With several values, writes each in turn. |
| `(-> value channel)`{.brix}, `(*-> items channel)`{.brix}, `(<-* channel items)`{.brix} | Writes a value, or each item of a stream; see Streams. |
| `(?<- channel value)`{.brix} | Writes the value when `channel` is not `null`. |
| `(<- channel)`{.brix} | Reads the next value, or `null` once the channel is closed and empty. |
| A `for` loop over the channel | Reads values until the channel is closed and empty. |
| `(call (. channel close))`{.brix} | Closes the channel. |

- **Several readers** of one `SyncChannel` or `SyncBufferedChannel` each take different values.
- **Hand-offs.** On the main queue, a write to a `SyncChannel` that a task is waiting to read lets
  the reader run first, and the writer goes on after it.
- **Streams.** A channel is a stream, so a function that takes a stream takes a channel. A stream
  converts to `Broadcastable`, the type of `AsyncChannel`, by a task that publishes its items.

## Subscriptions

A loop written with `on` in place of `in` subscribes to a broadcaster such as an `AsyncChannel`.
It does not wait: the code after it runs at once, and the loop's body runs later, for each value
published.

```brix
(main
  (let temps (of AsyncChannel Int))
  (for (on t temps (when (> t 20)))
    (print (str "warm " t))
  )
  (for (on t temps)
    (print (str "seen " t))
    (if (== t 25)
      (break)
    )
  )
  (print "subscribed")
  (<-* temps (list 18 25 30))
  (call (. temps close))
  (print "published")
)
```

```output
subscribed
published
warm 25
warm 30
seen 18
seen 25
```

- **Delivery.** Each subscription gets the values in the order they were published, in a task of
  its own on the queue of the task that published them. The publisher does not wait for it.
- **Clauses.** `when`, `while` and `until` work as in other loops; see Control flow.
- **Ending.** `break` ends the subscription. Closing the channel ends every subscription after it
  gets the values already published. The body cannot return a value: `The body of a loop on
  "temps" cannot return with "="; skip to the next value with "continue", or stop the loop with
  "break"`.
- **Errors** a subscription's body raises go to `unhandled-error`, and later values are still
  delivered.
- **Reading once.** `(<- temps)`{.brix} on an `AsyncChannel` subscribes, waits for the next value
  published, and ends the subscription.
- **One source.** A loop subscribes to one broadcaster. Two `on` headers in one loop are an
  error: `A loop on a stream subscribes to one source, so it cannot be joined to another; write a
  loop for each source`.

The events of a class are broadcasters too, and deliver to their subscribers at once, before the
publisher goes on. A class can also subscribe in its own body. See Reactivity.
