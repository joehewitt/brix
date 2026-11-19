# Tasks

```brix
(main
  (do-new
    (print "hi")
  )
)
```

```output
hi
```

```brix crash
(main
  (do-new
    (throw (call Panic "task boom"))
  )
)
```

```stderr
Unhandled panic: task boom
<no file:4>

Task 2, spawned by task 1 from no file:3:3

Stack trace:
  0  __source__:Fork no file:4:5
```

```brix crash
(main
  (let worker (call Worker 1))
  (do-new (worker: worker)
    (throw (call Panic "worker boom"))
  )
)
```

```stderr
Unhandled panic: worker boom
<no file:5>

Task 2 (worker), spawned by task 1 from no file:4:3

Stack trace:
  0  __source__:Fork no file:5:5
```

```brix crash
(main
  (do-new-thread
    (throw (call Panic "thread boom"))
  )
)
```

```stderr
Unhandled panic: thread boom
<no file:4>

Task 2 (thread), spawned by task 1 from no file:3:3

Stack trace:
  0  __source__:Thread no file:4:5
```

```brix
(main
  (do-new
    (inline (call foo 1))
  )
)

(fn-do + foo _
  (param n _)
  (print (str "ok " n))
)
```

```output
ok 1
```

### Single suspend point

```brix
(main
  (let waiter (call Waiter))
  (do-new
    (inline (call foo waiter))
  )
  (call (. waiter resume))
  (print "done")
)

(type + Waiter
  (prop + sem _ (= (call Semaphore)))
  (fn-do + await _
    (call (. sem wait))
  )
  (fn-do + resume _
    (call (. sem post))
  )
)

(fn-do + foo _
  (param waiter Waiter)
  (print "start")
  (call (. waiter await))
  (print "resume")
)
```

```output
start
done
resume
```

### Multiple suspend points

```brix
(main
  (let waiter (call Waiter))
  (do-new
    (inline (call foo waiter))
  )
  (call (. waiter resume))
  (call (. waiter resume))
  (call (. waiter resume))
)

(type + Waiter
  (prop + sem _ (= (call Semaphore)))
  (fn-do + await _
    (call (. sem wait))
  )
  (fn-do + resume _
    (call (. sem post))
  )
)

(fn-do + foo _
  (param waiter Waiter)
  (print "a")
  (call (. waiter await))
  (print "b")
  (call (. waiter await))
  (print "c")
  (call (. waiter await))
  (return 1)
)
```

```output
a
b
c
```

### Nested suspension

```brix
(main
  (let waiter (call Waiter))
  (do-new
    (inline (call bar waiter))
  )
  (call (. waiter resume))
  (call (. waiter resume))
)

(type + Waiter
  (prop + sem _ (= (call Semaphore)))
  (fn-do + await _
    (call (. sem wait))
  )
  (fn-do + resume _
    (call (. sem post))
  )
)

(fn-do + foo _
  (param waiter Waiter)
  (print "foo start")
  (call (. waiter await))
  (print "foo resume")
  (return 1)
)

(fn-do + bar _
  (param waiter Waiter)
  (print "bar start")
  (print (call foo waiter))
  (print "bar resume")
  (call (. waiter await))
)
```

```output
bar start
foo start
foo resume
1
bar resume
```

## Capture scope

```brix
(main
  (for (in i (..< 0 3))
    (do-new
      (inline (call foo i))
    )
  )
)

(fn-do + foo _
  (param i _)
  (print i)
)
```

```output
0
1
2
```

```brix
(main
  (do-new-thread
    (call foo 42)
  )
)

(fn-do + foo _
  (param i _)
  (print 42)
)
```

```output
42
```

### Fork inside a method which refers to and modifies properties

```brix
(main
  (let foo (call Foo))
  (call (. foo branch))
)

(type + Foo
  (prop + a _ (= 42))
  (fn-do + foo _
    (param i _)
    (print i)
  )
  (fn-do + branch _
    (do-new
      (print (. this a))
      (= (. this a) 24)
      (print (. this a))
    )
  )
)
```

```output
42
24
```

```brix
(main
  (let foo (call Foo))
  (call (. foo branch))
)

(type + Foo
  (prop + a _ (= 42))
  (fn-do + foo _
    (param i _)
    (print i)
  )
  (fn-do + branch _
    (do-new
      (print a)
      (= a 24)
      (print a)
    )
  )
)
```

```output
42
24
```

### Fork inside a method which calls a method

```brix
(main
  (let foo (call Foo))
  (call (. foo branch))
)

(type + Foo
  (prop + a _ (= 42))
  (fn-do + foo _
    (param i _)
    (print i)
  )
  (fn-do + branch _
    (do-new
      (call foo 10)
    )
  )
)
```

```output
10
```

### Fork inside a closure

```brix
(main
  (let foo
    (=> () :do
      (block
        (let b 20)
        (do-new
          (print b)
        )
      )
    )
  )
  (blank)
  (call foo)
)
```

```output
20
```

```brix
(main
  (let a 10)
  (let foo
    (=> () :do
      (block
        (do-new
          (print a)
          (= a 20)
          (print a)
        )
      )
    )
  )
  (blank)
  (call foo)
)
```

```output
10
20
```

### Fork inside a nested closure

```brix
(main
  (let a 10)
  (let foo
    (=> () :do
      (block
        (let b 20)
        (let bar
          (=> () :do
            (block
              (do-new
                (print a)
                (= a 20)
                (print a)
                (blank)
                (print b)
                (= b 30)
                (print b)
              )
            )
          )
        )
        (blank)
        (call bar)
      )
    )
  )
  (blank)
  (call foo)
)
```

```output
10
20
20
30
```

### Closure inside a fork

```brix
(main
  (let a 10)
  (do-new
    (let foo
      (=> () :do
        (block
          (print a)
        )
      )
    )
    (blank)
    (call foo)
  )
)
```

```output
10
```

### Fork in a closure captures method self and a typed list

```brix
(main
  (let app (call App))
  (call (. app submit))
)

(type + App
  (prop + values (list Int)
    (block
      (item 1)
      (item 2)
    )
  )
  (fn-do + submit _
    (let handler
      (=> () :do
        (block
          (let request (list Int) values)
          (do-new
            (inline (call stream-response request))
          )
        )
      )
    )
    (call handler)
  )
  (fn-do + stream-response _
    (param request (list Int))
    (print ([] request 0))
  )
)
```

```output
1
```

### Modifying outer variable inside fork doesn't affect outside variable

```brix
(main
  (let a 10)
  (do-new
    (let foo
      (=> () :do
        (block
          (print a)
          (= a 20)
          (print a)
        )
      )
    )
    (call foo)
  )
  (print (str "still " a))
)
```

```output
still 10
10
20
```

### Fork which references global variables

```brix
(prop _ x _ (= 1))
(blank)

(main
  (do-new-thread
    (print x)
    (= x 2)
    (print x)
  )
)
```

```output
1
2
```

### Lazy script variable first read inside a fork materializes at the spawn site, in the spawning function's frame, not the fork's

```brix
(main
  (lazy y 1)
  (let done (call Semaphore))
  (blank)
  (do-new
    (print (str "task y=" y))
    (call (. done post))
  )
  (call (. done wait))
  (print (str "main y=" y))
)
```

```output
task y=1
main y=1
```

### Lazy script variables written inside a fork mutate the fork's snapshot only; the spawning task and later forks keep intact values

```brix
(main
  (lazy y 1)
  (lazy z (* y 3))
  (let done (call Semaphore))
  (blank)
  (do-new
    (= y 2)
    (print (str "first y=" y " z=" z))
    (call (. done post))
  )
  (call (. done wait))
  (print (str "main y=" y))
  (blank)
  (do-new
    (print (str "second y=" y " z=" z))
    (call (. done post))
  )
  (call (. done wait))
  (print (str "end y=" y " z=" z))
)
```

```output
first y=2 z=3
main y=1
second y=1 z=3
end y=1 z=3
```

## Workers

```brix
(main
  (let worker (call Worker 1))
  (do-new (worker: worker)
    (call foo 1 worker)
  )
)

(fn-do + foo _
  (param n _)
  (param worker _)
  (print (str "ok " n))
)
```

```output
ok 1
```

```brix crash
(main
  (let worker (call Worker 1))
  (do-new (worker: 42)
    (print 1)
  )
)
```

```output
Exception: Fork target is not a Worker
<no file:4>
      (do-new (worker: 42)
      ^^^^^^^^^^^^^^^^^^^
```

### A task gives no value: its body ends with = alone, and one that returns a value is an error.

```brix compile-error
(main
  (do-new
    (return 5)
  )
)
```

```output
Exception: A task gives no value, so its body cannot return one with "(return …)"; end it with "(return)", or send the value on a channel
<no file:4>
        (return 5)
        ^^^^^^^^^^
```

### A task runs on a worker or on the pool's threads, not on both.

```brix compile-error
(main
  (let worker (call Worker 1))
  (do-new-thread (worker: worker)
    (print 1)
  )
)
```

```output
Exception: A task runs on a worker or on the pool's threads, not both; write "(do-new (worker: w) …)" or "(do-new-thread …)"
<no file:4>
      (do-new-thread (worker: worker)
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (let worker (call Worker 1))
  (print (. worker is-stopped))
  (call (. worker stop))
  (print (. worker is-stopped))
  (do-new (worker: worker)
    (print 1)
  )
  (print "nada")
)
```

```output
false
true
nada
```

### Stopping a worker cancels the tasks it still has, which run their cleanups before it stops.

```brix
(main
  (let worker (call Worker 1))
  (let never (channel Int))
  (let ready (channel Int))
  (do-new (worker: worker)
    (finally
      (print "cleanup")
    )
    (<- ready 1)
    (print (str "got " (<- never)))
  )
  (<- ready)
  (call (. worker stop))
  (print (str "stopped " (. worker is-stopped)))
)
```

```output
cleanup
stopped true
```

### Stopping a worker waits for a cleanup that waits on a reply from the stopping task's queue.

```brix
(import /unistd.h)
(import /stdlib.h)
(blank)

(main
  (let worker (call Worker 1))
  (let started (channel Int))
  (let waiting (channel Int))
  (let reply (channel String))
  (let box (call Box))
  (let task
    (do-new (worker: worker)
      (finally
        (do (without cancel)
          (<- waiting 1)
          (= (. box reply) (?? (<- reply) "closed"))
        )
      )
      (<- started 1)
      (call (. (call Semaphore) wait))
    )
  )
  (<- started)
  (do-new
    (<- waiting)
    (<- reply "reply")
  )
  (do-new (worker: (call Worker 1))
    (let tries 0)
    (while (and (== (. box reply) "") (< tries 300))
      (call C:usleep 10000)
      (+= tries 1)
    )
    (if (== (. box reply) "")
      (<- reply "timeout")
    )
  )
  (call (. worker stop))
  (print (str (. box reply) " finished " (. task is-finished)))
  (if (not (. task is-finished))
    (call C:exit 1)
  )
)

(type + Box
  (prop + reply _ (= ""))
)
```

```output
reply finished true
```

## Priority Tasks

```brix
(main
  (do-new
    (print "normal")
  )
  (blank)
  (do-new @priority
    (print "priority")
  )
  (blank)
  (print "main")
)
```

```output
main
priority
normal
```

```brix
(main
  (do-new @priority
    (print "priority 1")
  )
  (blank)
  (do-new @priority
    (print "priority 2")
  )
  (blank)
  (do-new
    (print "normal")
  )
  (blank)
  (print "main")
)
```

```output
main
priority 1
priority 2
normal
```

```brix
(main
  (let ready (call Semaphore))
  (let box (call TaskBox))
  (blank)
  (do-new @priority
    (print "priority start")
    (= (. box parked) (call Task))
    (call (. ready post))
    (let parked (!! (. box parked)))
    (if (call (. parked suspend))
      (throw (call TaskCancelationError))
    )
    (print "priority resumed")
  )
  (blank)
  (call (. ready wait))
  (blank)
  (do-new
    (print "normal pending")
  )
  (blank)
  (let parked (!! (. box parked)))
  (call (. parked resume-later))
  (print "main after resume")
)

(type + TaskBox
  (prop + parked (? Task))
)
```

```output
priority start
main after resume
priority resumed
normal pending
```

```brix crash
(main
  (let ready (call Semaphore))
  (blank)
  (do-new
    (finally
      (let failure (. (call Task) failure))
      (if failure
        (print (str "cleanup saw " (. failure message)))
      )
      (print "worker cleanup")
    )
    (call (. ready post))
    (call (. (call Semaphore) wait))
    (print "unexpected resume")
  )
  (blank)
  (call (. ready wait))
  (call request-process-termination 130)
)
```

```output
cleanup saw Task terminated
worker cleanup
```

### A task terminated while it waits for another task ends there, and its cleanup runs.

```brix crash
(main
  (let ready (call Semaphore))
  (let sleeper
    (do-new
      (call (. (call Semaphore) wait))
    )
  )
  (do-new
    (finally
      (let failure (. (call Task) failure))
      (if failure
        (print (str "cleanup saw " (. failure message)))
      )
      (print "worker cleanup")
    )
    (call (. ready post))
    (call (. sleeper wait))
    (print "unexpected resume")
  )
  (call (. ready wait))
  (call request-process-termination 130)
)
```

```output
cleanup saw Task terminated
worker cleanup
```

### A task terminated while it is suspended is answered that it is cancelled. One that ignores the answer runs on, and the termination ends it later, at a wait or a poll; its cleanup runs.

```brix crash
(import /time/duration)
(blank)

(main
  (do-new
    (let answer false)
    (finally
      (let failure (. (call Task) failure))
      (if failure
        (print (str "cleanup saw " (. failure message)))
      )
      (print (str "suspend answered " answer))
    )
    (= answer (call (. (call Task) suspend)))
    (call (. (call Semaphore) wait))
    (print "unexpected resume")
  )
  (blank)
  (call (. 50ms wait))
  (call request-process-termination 130)
)
```

```output
cleanup saw Task terminated
suspend answered true
```

### A task terminated while it takes a lock and gives it back does not end holding it, so its cleanup can take it.

```brix crash
(main
  (let pool (call Worker 1))
  (let ready (call Semaphore))
  (let mutex (call Mutex))
  (let count 0)
  (blank)
  (do-new (worker: pool)
    (finally
      (call (. mutex lock))
      (print "cleanup took the lock")
      (call (. mutex unlock))
    )
    (call (. ready post))
    (while true
      (do
        (use mutex)
        (+= count 1)
      )
    )
  )
  (blank)
  (call (. ready wait))
  (call request-process-termination 130)
)
```

```output
cleanup took the lock
```

```brix
(main
  (let ready (call Semaphore))
  (let done (call Semaphore))
  (let box (call TaskBox))
  (blank)
  (do-new
    (= (. box task) (call Task))
    (call (. ready post))
    (finally
      (let panic (. (call Task) failure))
      (if panic
        (print (str "recovered " panic))
      )
      (call (. done post))
    )
    (blank)
    (call (. (call Semaphore) wait))
    (print "unexpected resume")
  )
  (blank)
  (call (. ready wait))
  (let task (!! (. box task)))
  (call (. task cancel))
  (call (. done wait))
  (print "done")
)

(type + TaskBox
  (prop + task (? Task))
)
```

```output
recovered Task cancelled
done
```

```brix
(main
  (for (in i (..< 0 10))
    (do-new @priority
      (print (str "priority " i))
    )
  )
  (blank)
  (do-new
    (print "normal")
  )
  (blank)
  (print "main")
)
```

```output
main
priority 0
priority 1
priority 2
priority 3
priority 4
priority 5
priority 6
priority 7
normal
priority 8
priority 9
```

```brix
(main
  (let done (call Semaphore))
  (let worker (call Worker 1))
  (blank)
  (do-new (worker: worker)
    (do-new (worker: worker)
      (print "worker normal")
      (call (. done post))
    )
    (blank)
    (do-new (worker: worker) @priority
      (print "worker priority")
    )
    (blank)
    (print "worker parent")
  )
  (blank)
  (call (. done wait))
  (call (. worker stop))
)
```

```output
worker parent
worker priority
worker normal
```

## Threads

```brix
(main
  (do-new-thread
    (inline (call foo 1))
  )
)

(fn-do + foo _
  (param n _)
  (print (str "ok " n))
  (return n)
)
```

```output
ok 1
```

## Task Local Variables

```brix
(prop + bar _ @task (= (tuple 42 99)))
(prop + baz _ @task (= 88))
(blank)

(main
  (print (tuple bar baz))
  (= bar (tuple 100 200))
  (= baz 3)
  (print (tuple bar baz))
  (blank)
  (do-new
    (print (tuple "task 1" bar baz))
    (= bar (tuple 1 2))
    (= baz 5)
    (print (tuple "task 1" bar baz))
  )
)
```

```output
((42, 99), 88)
((100, 200), 3)
(task 1, (42, 99), 88)
(task 1, (1, 2), 5)
```

### Task-local and stack-local initializers run once in the main task and once in each spawned task; the spawned task inherits the stack-local binding.

```brix
(main
  (print (tuple "main" label depth))
  (let done (call Semaphore))
  (do-new
    (print (tuple "task" label depth))
    (call (. done post))
  )
  (call (. done wait))
  (print count)
)

(blank)
(prop + count _ (= 0))
(prop + label _ @task (= (call next-count)))
(prop + depth _ @stack (= (call next-count)))

(fn-do + next-count Int
  (+= count 1)
  (return count)
)
```

```output
(main, 1, 2)
(task, 3, 2)
4
```

### The main task initializes a module's task-locals before its eager globals.

```brix
(main
  (print (tuple first-seen later))
)

(blank)
(prop + count _ (= 0))
(prop + later _ (= (call next-count)))
(prop + first-seen _ @task (= (call next-count)))

(fn-do + next-count Int
  (+= count 1)
  (return count)
)
```

```output
(1, 2)
```

### A panic in a spawned task's task-local initializer is reported.

```brix crash
(main
  (= armed true)
  (do-new
    (print label)
  )
)

(blank)
(prop + armed _ (= false))
(prop + label _ @task (= (call pick)))

(fn-do + pick String
  (if armed
    (= armed false)
    (throw (call Panic "armed"))
  )
  (return "ok")
)
```

```stderr
Unhandled panic: armed
<no file:16>

Task 2, spawned by task 1 from no file:4:3

Stack trace:
  0  __source__:pick no file:16:5
  1  __source__:init.__source__.task-local.label no file:11:26
  2  __source__:taskinit no file
  3  __source__:Fork no file:4:3
```

### A task that native code schedules on the main queue runs the task-local initializers.

```brix
(import /BxEvents/BxEvents.h)
(blank)

(main
  (= label "main")
  (call C:brixScheduleMainTaskFromNative (as native-task NativeTaskStart) (call Pointer))
  (call (. done wait))
  (print label)
)

(blank)
(prop + done _ (= (call Semaphore)))
(prop + label _ @task (= "initialized"))

(type + NativeTaskStart (is (c-fn-type Void (param closure Pointer))))

(fn-do + native-task _
  (param closure Pointer)
  (print label)
  (call (. done post))
  (call C:brixTerminateActiveTask)
)
```

```output
initialized
main
```

### A native main task that returns ends its task, and the main queue runs on.

```brix
(import /BxEvents/BxEvents.h)
(blank)

(main
  (= label "main")
  (call C:brixScheduleMainTaskFromNative (as native-task NativeTaskStart) (call Pointer))
  (call C:brixScheduleMainTaskFromNative (as native-task NativeTaskStart) (call Pointer))
  (call (. done wait))
  (call (. done wait))
  (print label)
)

(blank)
(prop + done _ (= (call Semaphore)))
(prop + label _ @task (= "initialized"))

(type + NativeTaskStart (is (c-fn-type Void (param closure Pointer))))

(fn-do + native-task _
  (param closure Pointer)
  (print label)
  (call (. done post))
)
```

```output
initialized
initialized
main
```

### A native main task that returns ends its task without task-local initializers too.

```brix
(import /BxEvents/BxEvents.h)
(blank)

(main
  (call C:brixScheduleMainTaskFromNative (as native-task NativeTaskStart) (call Pointer))
  (call C:brixScheduleMainTaskFromNative (as native-task NativeTaskStart) (call Pointer))
  (call (. done wait))
  (call (. done wait))
  (print "main")
)

(blank)
(prop + done _ (= (call Semaphore)))

(type + NativeTaskStart (is (c-fn-type Void (param closure Pointer))))

(fn-do + native-task _
  (param closure Pointer)
  (print "native")
  (call (. done post))
)
```

```output
native
native
main
```

### Arena handles and allocation pointers may cross a quiescent task handoff.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (let value (call (of (. arena allocate) U32) 1))
  (= ([] value 0) 42u32)
  (let done (call Semaphore))
  (do-new
    (print ([] value 0))
    (let other (call (of (. arena allocate) U32) 1))
    (= ([] other 0) 43u32)
    (print ([] other 0))
    (call (. done post))
  )
  (call (. done wait))
  (call (. arena release))
)
```

```output
42
43
```

```brix
(main
  (let sem (call (of Semaphore Int)))
  (let copy sem)
  (do-new
    (print (call (. sem await)))
  )
  (call (. copy post) 42)
)
```

```output
42
```

```brix
(main
  (let sem (call (of Semaphore String)))
  (do-new
    (call (. sem post) "message")
  )
  (print (call (. sem await)))
)
```

```output
message
```

### Posts that find no task waiting wait in turn, and so do waits that find no post: each wait takes one post, the first first, and lets that poster go on before it does.

```brix
(main
  (let posts (call (of Semaphore Int)))
  (do-new
    (call (. posts post) 1)
    (print "first posted")
  )
  (do-new
    (call (. posts post) 2)
    (print "second posted")
  )
  (let gate (of SyncBufferedChannel Bool))
  (do-new
    (<- gate true)
  )
  (<- gate)
  (print (call (. posts await)))
  (print (call (. posts await)))
  (blank)
  (let waits (call (of Semaphore Int)))
  (do-new
    (print (str "first waiter got " (call (. waits await))))
  )
  (do-new
    (print (str "second waiter got " (call (. waits await))))
  )
  (do-new
    (<- gate true)
  )
  (<- gate)
  (call (. waits post) 3)
  (call (. waits post) 4)
)
```

```output
first posted
1
second posted
2
first waiter got 3
second waiter got 4
```

```brix crash
(main
  (let sem (call (of Semaphore Payload)))
  (if false
    (call (. sem post) "wrong")
  )
  (print "unexpected acceptance")
)

(type + Payload
  (prop + value Int)
)
```

```output
Exception: No method "post" of Semaphore'Payload accepts (String); it is declared as "post(value Payload)"
<no file:5>
        (call (. sem post) "wrong")
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let sem (call (of Semaphore Int)))
  (if false
    (let copy (of Semaphore F32) sem)
  )
  (print "unexpected acceptance")
)
```

```output
Exception: "copy" is declared Semaphore'F32, but its value is Semaphore'I64, which can never be Semaphore'F32; convert the value, or declare "copy" without a type
<no file:5>
        (let copy (of Semaphore F32) sem)
             ^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let sem (call (of Semaphore Int)))
  (if false
    (print (call (of (. sem await) String)))
  )
  (print "unexpected acceptance")
)
```

```output
Exception: No method "await" of Semaphore'I64 accepts (); it is declared as "await(mutex Mutex?)"
<no file:5>
        (print (call (of (. sem await) String)))
               ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let sem (call (of Semaphore Int)))
  (if false
    (call (. sem post))
  )
  (print "unexpected acceptance")
)
```

```output
Exception: No method "post" of Semaphore'I64 accepts (); it is declared as "post(value Payload)"
<no file:5>
        (call (. sem post))
        ^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let sem (call (of Semaphore Int)))
  (if false
    (call (. sem wait))
  )
  (print "unexpected acceptance")
)
```

```output
Exception: Semaphore'I64 has no method "wait"; did you mean "await"?
<no file:5>
        (call (. sem wait))
        ^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let sem (call Semaphore))
  (if false
    (call (. sem post) 42)
  )
  (print "unexpected acceptance")
)
```

```output
Exception: No method "post" of Semaphore'Void accepts (I64); it is declared as "post(value Payload)" or "post"
<no file:5>
        (call (. sem post) 42)
        ^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (let sem (call (of Semaphore Int)))
  (let mutex (call Mutex))
  (call (. mutex lock))
  (do-new
    (call (. mutex lock))
    (call (. mutex unlock))
    (call (. sem post) 7)
  )
  (print (!! (call (. sem await) mutex)))
)
```

```output
7
```

```brix
(main
  (let sem (call (of Semaphore Payload)))
  (let mutex (call Mutex))
  (let copy sem)
  (call (. mutex lock))
  (do-new
    (call (. copy post) (call Payload "hello" 7))
  )
  (let received (!! (call (. sem await) mutex)))
  (print (. received text))
  (print (. received number))
)

(type + Payload (is Tuple)
  (prop + text String)
  (prop + number Int)
  (fn-do + This _
    (param text _)
    (param number _)
  )
)
```

```output
hello
7
```

```brix
(main
  (let sem (call (of Semaphore Int)))
  (let ready (call Semaphore))
  (let done (call Semaphore))
  (let box (call TaskBox))
  (do-new
    (= (. box task) (call Task))
    (finally
      (let panic (. (call Task) failure))
      (if panic
        (print "canceled receiver")
      )
      (call (. done post))
    )
    (call (. ready post))
    (print (call (. sem await)))
    (print "unexpected receive")
  )
  (call (. ready wait))
  (let canceled (!! (. box task)))
  (call (. canceled cancel))
  (call (. done wait))
  (do-new
    (call (. sem post) 9)
  )
  (print (call (. sem await)))
)

(type + TaskBox
  (prop + task (? Task))
)
```

```output
canceled receiver
9
```

```brix
(main
  (let sem (call (of Semaphore Int)))
  (let ready (call Semaphore))
  (let done (call Semaphore))
  (let box (call TaskBox))
  (do-new
    (= (. box task) (call Task))
    (finally
      (let panic (. (call Task) failure))
      (if panic
        (print "canceled sender")
      )
      (call (. done post))
    )
    (call (. ready post))
    (call (. sem post) 17)
    (print "unexpected post")
  )
  (call (. ready wait))
  (let canceled (!! (. box task)))
  (call (. canceled cancel))
  (call (. done wait))
  (do-new
    (call (. sem post) 9)
  )
  (print (call (. sem await)))
)

(type + TaskBox
  (prop + task (? Task))
)
```

```output
canceled sender
9
```

```brix
(main
  (let sem (call (of Semaphore I64)))
  (let value I32 123)
  (do-new
    (call (. sem post) value)
  )
  (print (call (. sem await)))
)
```

```output
123
```

```brix crash
(main
  (let sem (call (of Semaphore Int)))
  (if false
    (call (. sem post) (tuple 1 2 3))
  )
  (print "unexpected acceptance")
)
```

```output
Exception: No method "post" of Semaphore'I64 accepts ((I64, I64, I64)); it is declared as "post(value Payload)"
<no file:5>
        (call (. sem post) (tuple 1 2 3))
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (let sem (call (of Semaphore Int)))
  (let ready (call Semaphore))
  (let done (call Semaphore))
  (let box (call TaskBox))
  (do-new
    (= (. box task) (call Task))
    (call (. ready post))
    (print (== (call (. sem await)) null))
    (call (. done post))
  )
  (call (. ready wait))
  (let receiver (!! (. box task)))
  (call (. receiver resume-later))
  (call (. done wait))
)

(type + TaskBox
  (prop + task (? Task))
)
```

```output
true
```

```brix
(import /BxCore/BxCore.h)
(import /core/arena)
(blank)

(main
  (let reserved-before (call C:brixArenaRuntimeReservedBytes))
  (let output (of SyncBufferedChannel Int))
  (let scratch-pool (of SyncBufferedChannel Arena))
  (let worker (call Worker 4))
  (for (in _ (..< 0 4))
    (<- scratch-pool (call Arena))
  )
  (for (in value (..< 0 128))
    (do-new (worker: worker)
      (let scratch (!! (<- scratch-pool)))
      (finally
        (<- scratch-pool scratch)
      )
      (let buffer (call (of (. scratch allocate) Int) 1))
      (= ([] buffer 0) value)
      (let resume (of SyncBufferedChannel Bool))
      (do-new
        (<- resume true)
      )
      (<- resume)
      (let copied ([] buffer 0))
      (call (. scratch reset))
      (<- output copied)
    )
  )
  (blank)
  (let total 0)
  (for (in _ (..< 0 128))
    (+= total (!! (<- output)))
  )
  (call (. worker stop))
  (call (. scratch-pool close))
  (for (in scratch scratch-pool)
    (call (. scratch release))
  )
  (print (== total 8128))
  (print (== (call C:brixArenaRuntimeReservedBytes) reserved-before))
)
```

```output
true
true
```

### Task-local variables of the main task start at their values and keep what is assigned.

```brix
(prop + depth _ @task (= 5))
(prop + scale _ @task (= 2.5))
(prop + seen Bool @task)
(blank)

(main
  (print (tuple depth scale seen))
  (+= depth 3)
  (= seen true)
  (print (tuple depth scale seen))
  (call bump)
  (print depth)
)

(fn-do + bump _
  (*= depth 2)
)
```

```output
(5, 2.5, false)
(8, 2.5, true)
16
```

### A task-local with no value written starts as its type's default value, field by field.

```brix
(main
  (print pt)
  (= pt (call Point 4 5))
  (print pt)
  (print line)
)

(blank)
(prop + pt Point @task)
(prop + line Line @task)

(type + Point (is Struct)
  (prop + x I32 (= 7))
  (prop + y I32 (= 8))
)

(type + Line (is Struct)
  (prop + a Point)
  (prop + b Point)
)
```

```output
(7, 8)
(4, 5)
((7, 8), (7, 8))
```

### An eager global may read a task-local declared after it: the main task stores task-locals first.

```brix
(main
  (print later)
  (print depth)
)

(blank)
(prop + later _ (= (+ depth 1)))
(prop + depth _ @task (= 5))
```

```output
6
5
```

### A task-local no function reads never takes its value, so what its value does never runs.

```brix
(main
  (print "start")
  (print seen)
)

(blank)
(prop + seen _ (= 0))
(prop + noisy _ @task (= (call bump)))

(fn-do + bump Int
  (+= seen 10)
  (print "bumped")
  (return seen)
)
```

```output
start
0
```

### A task-local only a spawned task reads is given its value by the main task too.

```brix
(main
  (let done (call Semaphore))
  (do-new
    (print (tuple "task" label))
    (call (. done post))
  )
  (call (. done wait))
  (print "main done")
)

(blank)
(prop + label _ @task (= (call make)))

(fn-do + make String
  (print "making")
  (return "made")
)
```

```output
making
making
(task, made)
main done
```

### Task-locals take their values in the order they are declared, in the main task and in each spawned task.

```brix
(main
  (print (tuple second first))
  (let done (call Semaphore))
  (do-new
    (print (tuple "task" first second))
    (call (. done post))
  )
  (call (. done wait))
)

(blank)
(prop + count _ (= 0))
(prop + first _ @task (= (call next "first")))
(prop + second _ @task (= (call next "second")))

(fn-do + next Int
  (param name String)
  (+= count 1)
  (print (str "init " name " " count))
  (return count)
)
```

```output
init first 1
init second 2
(2, 1)
init first 3
init second 4
(task, 3, 4)
```

### A task-local's value cannot read one declared after it, which takes its value later.

```brix crash
(main
  (print first)
)

(blank)
(prop + first _ @task (= (+ second 1)))
(prop + second _ @task (= 5))
```

```output
Exception: Global 'second' is read before it is initialized
<no file:7>
    (prop + first _ @task (= (+ second 1)))
                                ^^^^^^
```

### An error that leaves the body of a spawned task is reported as unhandled.

```brix crash
(main
  (do-new
    (throw (call IOError "lost"))
  )
)
```

```stderr
Unhandled error: lost
<no file:4>

Task 2, spawned by task 1 from no file:3:3

Stack trace:
  0  __source__:Fork no file:4:5
libc++abi: terminating
```

### An error that leaves the body of a task in a worker through a call runs the cleanups open and is reported as unhandled.

```brix crash
(main
  (let worker (call Worker 1))
  (do-new (worker: worker)
    (finally
      (print "closing")
    )
    (call fetch 2)
  )
  (call (. (call Semaphore) wait))
)

(fn-do + fetch _
  (param n Int)
  (if (> n 1)
    (throw (call IOError (str "too far " n)))
  )
)
```

```stderr
Unhandled error: too far 2
<no file:16>

Task 2 (worker), spawned by task 1 from no file:4:3

Stack trace:
  0  __source__:fetch no file:16:5
  1  __source__:Fork no file:8:5
libc++abi: terminating
```

### A return in the body of a spawned task ends that task, after the cleanups open, and the program goes on.

```brix
(main
  (let ch (of SyncChannel Int))
  (let stop true)
  (do-new
    (finally
      (print "closing")
    )
    (return-if (if stop))
    (print "not reached")
  )
  (do-new
    (return-if (if (not stop)))
    (<- ch 2)
  )
  (print (<- ch))
  (print "main goes on")
)
```

```output
closing
2
main goes on
```

### A task spawned in a closure shares a name declared further out than the closure, which copies only what the closure declares.

```brix
(main
  (let a 1)
  (let g
    (=> () :do
      (block
        (let b 2)
        (do-new
          (print (str "task a=" a " b=" b))
        )
        (= b 3)
      )
    )
  )
  (call g)
  (= a 99)
  (print (str "main " a))
)
```

```output
main 99
task a=99 b=2
```

### A task spawned in a task's body shares what that body copied.

```brix
(main
  (let a 10)
  (do-new
    (do-new
      (print (str "inner task " a))
    )
    (= a 20)
    (print (str "outer task " a))
  )
)
```

```output
outer task 20
inner task 20
```

### A task's body reads a name a match pattern binds, which it copies where it is spawned, as does a task spawned in a closure written in the arm.

```brix
(main
  (let item (call (. Item num) 5))
  (let ch (of SyncChannel Int))
  (match item
    (case (call .num n)
      (do-new
        (<- ch n)
      )
      (print (<- ch))
      (let g
        (=> () :do
          (block
            (do-new
              (<- ch (* n 2))
            )
          )
        )
      )
      (call g)
      (print (<- ch))
    )
    (case (call .word w)
      (print (str "word " w))
    )
  )
)

(type + Item (is Union)
  (variant num Int)
  (variant word String)
)
```

```output
5
10
```

### A task spawned in a closure shares a lazy local declared further out: it is evaluated at the spawn, and the task reads what was written to it after.

```brix
(main
  (lazy lz (call make))
  (let done (call Semaphore))
  (let f
    (=> () :do
      (block
        (let g
          (=> () :do
            (block
              (do-new
                (print (str "task " lz))
                (call (. done post))
              )
            )
          )
        )
        (call g)
      )
    )
  )
  (print "start")
  (call f)
  (= lz 50)
  (call (. done wait))
  (print (str "end " lz))
)

(fn-do + make Int
  (print "make")
  (return 7)
)
```

```output
start
make
task 50
end 50
```

### A task spawned in a task's body shares the lazy local the outer body copied, which that body changes.

```brix
(main
  (lazy lz (call make))
  (let done (call Semaphore))
  (do-new
    (do-new
      (print (str "inner " lz))
      (call (. done post))
    )
    (= lz 40)
    (print (str "outer " lz))
  )
  (call (. done wait))
)

(fn-do + make Int
  (print "make")
  (return 7)
)
```

```output
make
outer 40
inner 40
```

### Cleanup in a cancelled task runs to completion

```brix
(main
  (let gate (call (of SyncBufferedChannel Bool)))
  (let ready (call Semaphore))
  (let done (call Semaphore))
  (let box (call TaskBox))
  (do-new
    (= (. box task) (call Task))
    (finally
      (let panic (. (call Task) failure))
      (if panic
        (print (str "recovered " panic))
      )
      (call (. done post))
    )
    (finally
      (print "cleanup: begin")
      (let total (call busy 2000))
      (print (str "cleanup: computed " total))
    )
    (call (. ready post))
    (<- gate)
    (print "unexpected resume")
  )
  (call (. ready wait))
  (call (. (!! (. box task)) cancel))
  (call (. done wait))
  (print "done")
)

(type + TaskBox
  (prop + task (? Task))
)

(fn-do + busy Int
  (param count Int)
  (let total 0)
  (let parts (call (of List String)))
  (for (in index (..< 0 count))
    (+= parts (str "item " index))
    (+= total (. ([] parts index) length))
  )
  (return total)
)
```

```output
cleanup: begin
cleanup: computed 16890
recovered Task cancelled
done
```

### A wait inside a cancelled task's cleanup reports the cancellation, and the cleanups of the callers still run

```brix
(main
  (let gate (call (of SyncBufferedChannel Bool)))
  (let ready (call Semaphore))
  (let done (call Semaphore))
  (let box (call TaskBox))
  (do-new
    (= (. box task) (call Task))
    (finally
      (let panic (. (call Task) failure))
      (if panic
        (print (str "outer: recovered " panic))
      )
      (call (. done post))
    )
    (call middle ready gate)
  )
  (call (. ready wait))
  (call (. (!! (. box task)) cancel))
  (call (. done wait))
  (print "done")
)

(type + TaskBox
  (prop + task (? Task))
)

(fn-do + middle _
  (param ready Semaphore)
  (param gate (of SyncBufferedChannel Bool))
  (finally
    (let parts (call (of List String)))
    (for (in index (..< 0 3))
      (+= parts (str "part " index))
    )
    (print (str "middle: " (. parts length) " " ([] parts 2)))
  )
  (call inner ready gate)
)

(fn-do + inner _
  (param ready Semaphore)
  (param gate (of SyncBufferedChannel Bool))
  (finally
    (print "inner: begin")
    (call (. (call Semaphore) wait))
    (print "inner: unexpected resume")
  )
  (call (. ready post))
  (<- gate)
)
```

```output
inner: begin
middle: 3 part 2
outer: recovered Task cancelled
done
```

### A channel delivers to the next reader after a waiting reader is cancelled

```brix
(main
  (let numbers (call (of SyncBufferedChannel Int)))
  (let ready (call Semaphore))
  (let done (call Semaphore))
  (let box (call TaskBox))
  (do-new
    (= (. box task) (call Task))
    (finally
      (let panic (. (call Task) failure))
      (if panic
        (print "canceled reader")
      )
      (call (. done post))
    )
    (call (. ready post))
    (let number (<- numbers))
    (print (str "unexpected read " number))
  )
  (call (. ready wait))
  (call (. (!! (. box task)) cancel))
  (call (. done wait))
  (<- numbers 7)
  (let number (<- numbers))
  (print (str "read " number))
)

(type + TaskBox
  (prop + task (? Task))
)
```

```output
canceled reader
read 7
```

### A cancelled one-shot read of an AsyncChannel ends its subscription, so the next write does not wait

```brix
(main
  (let numbers (of AsyncChannel Int))
  (let ready (call Semaphore))
  (let done (call Semaphore))
  (let box (call TaskBox))
  (do-new
    (= (. box task) (call Task))
    (finally
      (if (. (call Task) failure)
        (print "canceled reader")
      )
      (call (. done post))
    )
    (call (. ready post))
    (let number (<- numbers))
    (print (str "unexpected read " number))
  )
  (call (. ready wait))
  (call (. (!! (. box task)) cancel))
  (call (. done wait))
  (<- numbers 1)
  (print "wrote 1")
  (do-new
    (call (. ready post))
    (print (str "read " (<- numbers)))
    (call (. done post))
  )
  (call (. ready wait))
  (<- numbers 2)
  (call (. done wait))
  (print (str "open " (not (. numbers is-closed))))
)

(type + TaskBox
  (prop + task (? Task))
)
```

```output
canceled reader
wrote 1
read 2
open true
```

### A cancelled writer's message is withdrawn from an unbuffered channel

```brix
(main
  (let numbers (call (of SyncChannel Int)))
  (let ready (call Semaphore))
  (let done (call Semaphore))
  (let box (call TaskBox))
  (do-new
    (= (. box task) (call Task))
    (finally
      (let panic (. (call Task) failure))
      (if panic
        (print "canceled writer")
      )
      (call (. done post))
    )
    (call (. ready post))
    (<- numbers 1)
    (print "unexpected write")
  )
  (call (. ready wait))
  (call (. (!! (. box task)) cancel))
  (call (. done wait))
  (do-new
    (<- numbers 2)
  )
  (let number (<- numbers))
  (print (str "read " number))
)

(type + TaskBox
  (prop + task (? Task))
)
```

```output
canceled writer
read 2
```

### A task cancelled while it waits for another task to initialize a lazy global reads the global once it is ready

```brix
(main
  (let ready (call Semaphore))
  (let done (call Semaphore))
  (let box (call TaskBox))
  (do-new
    (print (str "first reader: " slow))
    (call (. done post))
  )
  (call (. started wait))
  (do-new
    (= (. box task) (call Task))
    (finally
      (call (. done post))
    )
    (call (. ready post))
    (print (str "second reader: " slow))
  )
  (call (. ready wait))
  (call (. (!! (. box task)) cancel))
  (call (. gate post))
  (call (. done wait))
  (call (. done wait))
)

(blank)
(prop + started _ (= (call Semaphore)))
(prop + gate _ (= (call Semaphore)))
(prop + slow _ (:= (call initialize)))

(fn-do + initialize Int
  (call (. started post))
  (call (. gate wait))
  (return 42)
)

(type + TaskBox
  (prop + task (? Task))
)
```

```output
first reader: 42
second reader: 42
```

### A handle kept after its task finished is inert: it reports finished, and cancelling or resuming it leaves other tasks alone

```brix
(main
  (let box (call TaskBox))
  (let finished (call Semaphore))
  (do-new
    (= (. box task) (call Task))
    (print (str "running finished " (. (!! (. box task)) is-finished)))
    (call (. finished post))
  )
  (call (. finished wait))
  (let stale (!! (. box task)))
  (blank)
  (let started (call (of SyncBufferedChannel Int)))
  (let gate (call (of SyncBufferedChannel Bool)))
  (let done (call (of SyncBufferedChannel Int)))
  (for (in index (..< 0 4))
    (do-new
      (finally
        (let panic (. (call Task) failure))
        (if panic
          (print (str "bystander " index ": " panic))
        )
        (<- done index)
      )
      (<- started index)
      (<- gate)
      (print (str "bystander " index ": released"))
    )
  )
  (for (in index (..< 0 4))
    (<- started)
  )
  (blank)
  (print (str "stale finished " (. stale is-finished)))
  (call (. stale cancel))
  (call (. stale resume-later))
  (call (. stale resume-now))
  (print (str "stale canceled " (. stale is-canceled)))
  (call (. gate close))
  (for (in index (..< 0 4))
    (<- done)
  )
  (print "done")
)

(type + TaskBox
  (prop + task (? Task))
)
```

```output
running finished false
stale finished true
stale canceled false
bystander 0: released
bystander 1: released
bystander 2: released
bystander 3: released
done
```

```brix crash
(main
  (print greeting)
)

(blank)
(prop + name _ (= (call noisy "world")))
(prop + greeting _ @task (= (str "hello " name)))

(fn-do + noisy String
  (param name String)
  (return name)
)
```

```stderr
Unhandled panic: Global 'name' is read before it is initialized
<no file:8>

Task 1

Stack trace:
  0  __source__:init.__source__.task-local.greeting no file:8:43
  1  __source__:brix.main no file
```

### An object held only by the stack of a task that is still running survives a collection.

```brix
(import /BxGC/BxGC.h)
(blank)
(prop + stop-worker _ (= false))
(blank)

(main
  (let addresses (of SyncChannel U64))
  (let results (of SyncChannel String))
  (blank)
  (do-new
    (let node (call GCNode 616))
    (<- addresses (. (as node Pointer) address))
    (while (not stop-worker)
      (+= (. node value) 0)
    )
    (if (call C:brixGCVerifyObject (as node Pointer))
      (<- results (str "alive " (. node value)))
      (else
        (<- results "missing")
      )
    )
  )
  (blank)
  (let address (<- addresses))
  (call C:brixCollect)
  (print (call C:brixGCVerifyObject (as address Pointer)))
  (= stop-worker true)
  (print (<- results))
)

(type + GCNode
  (prop + value _ (= 0))
  (fn-do + This _
    (param v Int)
    (= value v)
  )
)
```

```output
true
alive 616
```

### An object held only by the closure of a task that has not started survives a collection.

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let results (of SyncChannel String))
  (call start-parked-task results)
  (call C:brixCollect)
  (print (<- results))
)

(fn-do + start-parked-task _
  (param results (of SyncChannel String))
  (let node (call GCNode 909))
  (do-new
    (if (call C:brixGCVerifyObject (as node Pointer))
      (<- results (str "alive " (. node value)))
      (else
        (<- results "missing")
      )
    )
  )
)

(type + GCNode
  (prop + value _ (= 0))
  (fn-do + This _
    (param v Int)
    (= value v)
  )
)
```

```output
alive 909
```

### A tuple of objects posted to a Semaphore survives collections until it is awaited.

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let sem (call (of Semaphore (tuple GCNode GCNode (list GCNode)))))
  (let ready (call Semaphore))
  (blank)
  (do-new
    (let payload (tuple (call GCNode 1) (call GCNode 2) (list (call GCNode 3) (call GCNode 4))))
    (call (. ready post))
    (call (. sem post) payload)
    (call C:brixCollect)
  )
  (blank)
  (call (. ready wait))
  (call C:brixCollect)
  (let received (!! (call (. sem await))))
  (call C:brixCollect)
  (print (call C:brixGCVerifyObject (as ([] received 0) Pointer)))
  (print (tuple (. ([] received 0) value) (. ([] received 1) value) (. ([] ([] received 2) 0) value) (. ([] ([] received 2) 1) value)))
)

(type + GCNode
  (prop + value _ (= 0))
  (fn-do + This _
    (param v Int)
    (= value v)
  )
)
```

```output
true
(1, 2, 3, 4)
```

```brix
(import /os/process)
(import /time/duration)
(blank)

(main
  (let process (call Process "sleep" (list "5")))
  (let done (call (of Semaphore Size)))
  (do-new
    (let buffer ([] Byte 16))
    (let count (call (. process read) 16 buffer))
    (call (. done post) count)
  )
  (call (. 20ms wait))
  (call (. process close))
  (print (str "read " (?? (call (. done await)) 99)))
  (call (. process kill))
  (let status (call (. process wait) null))
)
```

```output
read 0
```

### A semaphore that only its parked waiters still hold keeps working across collections.

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let results (call (of SyncChannel Int)))
  (let sem (call start-waiter results))
  (let canceled
    (do-new
      (call (. (call (of Semaphore Int)) await))
    )
  )
  (call C:brixCollect)
  (call (. sem post) 41)
  (call (. canceled cancel-later))
  (call C:brixCollect)
  (call C:brixCollect)
  (call (. canceled wait))
  (print (<- results))
)

(fn-do + start-waiter (of Semaphore Int)
  (param results (of SyncChannel Int))
  (let sem (call (of Semaphore Int)))
  (do-new
    (<- results (+ (?? (call (. sem await)) 0) 1))
  )
  (return sem)
)
```

```output
42
```

### Copies of a mutex lock it for two contending tasks after the original is dropped.

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let results (call (of SyncChannel String)))
  (call start-contenders results)
  (call C:brixCollect)
  (print (<- results))
  (print (<- results))
)

(fn-do + start-contenders _
  (param results (of SyncChannel String))
  (let m (call Mutex))
  (let first m)
  (let second m)
  (call (. first lock))
  (do-new
    (call (. second lock))
    (<- results "second")
    (call (. second unlock))
  )
  (do-new
    (call C:brixCollect)
    (<- results "first")
    (call (. first unlock))
  )
)
```

```output
first
second
```

### A task whose cancellation reaches its root ends quietly after its cleanups run.

```brix
(main
  (let ready (call Semaphore))
  (let done (call Semaphore))
  (let box (call TaskBox))
  (blank)
  (do-new
    (= (. box task) (call Task))
    (call (. ready post))
    (finally
      (call (. done post))
    )
    (blank)
    (call (. (call Semaphore) wait))
    (print "unexpected resume")
  )
  (blank)
  (call (. ready wait))
  (call (. (!! (. box task)) cancel))
  (call (. done wait))
  (print "still running")
)

(type + TaskBox
  (prop + task (? Task))
)
```

```output
still running
```

### A lazy global that tasks on several threads read at once is evaluated once, and each reads its value.

```brix
(prop + shared _ (:= (call slow-value)))
(blank)

(main
  (let pool (call Worker 8))
  (let done (of SyncBufferedChannel Int))
  (for (in _ (..< 0 8))
    (do-new (worker: pool)
      (<- done shared)
    )
  )
  (let total 0)
  (for (in _ (..< 0 8))
    (+= total (!! (<- done)))
  )
  (call (. pool stop))
  (print total)
)

(fn-do _ slow-value Int
  (print "evaluated")
  (let sum 0)
  (for (in step (..< 0 20000000))
    (+= sum (% step 3))
  )
  (return (if-expr (> sum 0) 1 0))
)
```

```output
evaluated
8
```

### A task forked onto a property's worker, where a test found it holds one, runs on it.

```brix
(main
  (let server (call Server))
  (= (. server worker) (call Worker 1))
  (call (. server start) 3)
)

(type + Server
  (prop + worker (? Worker))
  (fn-do + This _)
  (fn-do + start _
    (param value Int)
    (if worker
      (do-new (worker: worker)
        (inline (call report value))
      )
    )
  )
)

(fn-do _ report _
  (param value Int)
  (print (str "ok " value))
)
```

```output
ok 3
```

### A loop in a function that may not panic runs to its end when the task is being terminated, and the termination is raised later, where a function that may panic or the task's body checks.

```brix crash
(main
  (let last (call count-up 1000000))
  (if (== last 999999)
    (print "counted to the end")
    (else
      (print "stopped early")
    )
  )
)

(fn-do + count-up Int
  (param limit Int)
  (let last 0)
  (for (in index (..< 0 limit))
    (if (== index 10)
      (call request-process-termination 130)
    )
    (= last index)
  )
  (return last)
)
```

```output
counted to the end
```

### A loop in a function that may panic ends when the task is terminated.

```brix crash
(main
  (let box (call Box))
  (finally
    (if (< (. box last) 999999)
      (print "stopped early")
      (else
        (print "counted to the end")
      )
    )
  )
  (call count-up box 1000000)
)

(type + Box
  (prop + last Int)
)

(fn-do + count-up _
  (param box Box)
  (param limit Int)
  (for (in index (..< 0 limit))
    (if (== index 10)
      (call request-process-termination 130)
    )
    (if (< index 0)
      (throw (call Panic "negative"))
    )
    (= (. box last) index)
  )
)
```

```output
stopped early
```

### A task's finish action runs in a new main-queue task after the task's body returns.

```brix
(main
  (let log (call FinishLog))
  (do-new
    (call (. (call Task) when-finished) (. log child-finished))
    (+= (. log notes) "child body")
  )
  (call (. log done wait))
  (call (. (call Task) when-finished) (. log main-finished))
  (+= (. log notes) "main body")
)

(type + FinishLog
  (prop + notes (list String))
  (prop + done _ (= (call Semaphore)))
  (fn-do + child-finished _
    (+= notes "child finished")
    (call (. done post))
  )
  (fn-do + main-finished _
    (+= notes "main finished")
    (for (in note notes)
      (print note)
    )
  )
)
```

```output
child body
child finished
main body
main finished
```

### An action given to when-main-ends runs where main's statements end, before what main uses is closed and its do finally blocks run.

```brix
(main
  (finally
    (print "finally")
  )
  (use (call Thing))
  (call when-main-ends report)
  (print "last statement")
)

(fn-do _ report _
  (print "main ended")
)

(type + Thing
  (implements Closable)
  (fn-do + close _
    (print "closed")
  )
)
```

```output
last statement
main ended
closed
finally
```

```brix
(main
  (let label "the task")
  (do-new
    (finally
      (print "cleanup")
    )
    (throw (call Panic "boom"))
    (on-failure (: failure Panic)
      (print (str "caught " (. failure message) " in " label))
    )
  )
)
```

```output
cleanup
caught boom in the task
```

```brix
(main
  (do-new
    (let items (list 1 2))
    (print ([] items 5))
    (on-failure (: failure OutOfBoundsError)
      (print "out of bounds")
    )
    (on-failure failure
      (print "other")
    )
  )
  (do-new
    (throw (call Panic "plain"))
    (on-failure (: failure OutOfBoundsError)
      (print "out of bounds")
    )
    (on-failure failure
      (print (str "other: " (. failure message)))
    )
  )
)
```

```output
out of bounds
other: plain
```

```brix crash
(main
  (do-new
    (throw (call Panic "nobody"))
    (on-failure (: failure OutOfBoundsError)
      (print "wrong")
    )
  )
)
```

```stderr
Unhandled panic: nobody
<no file:4>

Task 2, spawned by task 1 from no file:3:3

Stack trace:
  0  __source__:Fork no file:4:5
```

```brix crash
(main
  (do-new
    (throw (call Panic "first"))
    (on-failure failure
      (throw (call Panic (str "again after " (. failure message))))
    )
  )
)
```

```stderr
Unhandled panic: again after first
<no file:6>

Task 2, spawned by task 1 from no file:3:3

Stack trace:
  0  __source__:Fork no file:6:7
```

```brix
(main
  (let channel (channel Int))
  (do-new
    (print "started")
    (call (. (call Task) cancel))
    (let value (<- channel))
    (print (str "never " value))
    (on-failure failure
      (print "handler ran")
    )
  )
  (print "main")
)
```

```output
main
started
```

```brix
(main
  (do-new
    (inline (call fails))
    (on-failure failure
      (print (str "caught " (. failure message)))
    )
  )
  (do-new-thread
    (throw (call Panic "on a thread"))
    (on-failure failure
      (print (str "thread: " (. failure message)))
    )
  )
)

(fn-do _ fails _
  (throw (call Panic "from a call"))
)
```

```output
caught from a call
thread: on a thread
```

```brix crash
(type-line + NotAPanic (is Error))
(blank)

(main
  (do-new
    (print 1)
    (on-failure (: failure NotAPanic)
      (print 2)
    )
  )
)
```

```output
Exception: A task's handler takes a panic
<no file:8>
        (on-failure (: failure NotAPanic)
                               ^^^^^^^^^
```

```brix
(main
  (print (str "main: " (call ending)))
  (do-new
    (print (str "before: " (call ending)))
    (finally
      (print (str "cleanup: " (call ending)))
    )
    (throw (call Panic "ending"))
    (on-failure failure
      (print (str "handler: " (. failure message) ", now " (call ending)))
    )
  )
)

(fn-do _ ending String
  (let failure (. (call Task) failure))
  (return (if-expr (!= failure null) (. failure message) "nothing"))
)
```

```output
main: nothing
before: nothing
cleanup: ending
handler: ending, now nothing
```

```brix
(main
  (let done (channel Int))
  (do-new @large-stack
    (<- done (call deep 100000))
  )
  (print (str "task " (<- done)))
  (do-new-thread @priority @large-stack
    (<- done (call deep 100000))
  )
  (print (str "thread " (<- done)))
  (let worker (call Worker 1))
  (do-new (worker: worker) @large-stack
    (<- done (call deep 100000))
  )
  (print (str "worker " (<- done)))
)

(fn-do _ deep Int
  (param n Int)
  (return (if-expr (== n 0) 0 (+ (call deep (- n 1)) 1)))
)
```

```output
task 100000
thread 100000
worker 100000
```

### A `do new` used as a value gives its `Task`, which tells how the task ended and can be waited for.

```brix
(main
  (let go (call Semaphore))
  (let task
    (do-new
      (call (. go wait))
      (print "task ran")
    )
  )
  (print (str "running: " (== (. task end) .running)))
  (do-new
    (call (. go post))
  )
  (call (. task wait))
  (print (str "finished: " (== (. task end) .finished)))
  (blank)
  (let one
    (do-new
      (inline (print "one step"))
    )
  )
  (call (. one wait))
  (print (str "one: " (== (. one end) .finished)))
  (blank)
  (let failing
    (do-new
      (throw (call Panic "broke"))
      (on-failure failure
        (print (str "caught " (. failure message)))
      )
    )
  )
  (call (. failing wait))
  (print (str "panicked: " (== (. failing end) .panicked)))
  (blank)
  (let blocked
    (do-new
      (let ch (channel Int))
      (let value (<- ch))
      (print (str "never " value))
    )
  )
  (do-new
    (call (. blocked cancel))
  )
  (call (. blocked wait))
  (print (str "cancelled: " (== (. blocked end) .cancelled)))
  (blank)
  (let worker (call Worker 2))
  (let pooled
    (do-new (worker: worker)
      (print "in worker")
    )
  )
  (call (. pooled wait))
  (print (str "worker: " (== (. pooled end) .finished)))
  (blank)
  (let threaded
    (do-new-thread @large-stack
      (print "on a thread")
    )
  )
  (call (. threaded wait))
  (print (str "thread: " (== (. threaded end) .finished)))
)
```

```output
running: true
task ran
finished: true
one step
one: true
caught broke
panicked: true
cancelled: true
in worker
worker: true
on a thread
thread: true
```

### A task waits for another through the `Task` that other task gave itself, and a task that waits for itself panics.

```brix
(main
  (let box (call TaskBox))
  (let ready (call Semaphore))
  (let go (call Semaphore))
  (do-new
    (= (. box task) (call Task))
    (call (. ready post))
    (call (. go wait))
    (print "task done")
  )
  (call (. ready wait))
  (let task (!! (. box task)))
  (print (str "running: " (== (. task end) .running)))
  (do-new
    (call (. go post))
  )
  (call (. task wait))
  (print (str "finished: " (== (. task end) .finished)))
  (call (. task wait))
  (print "waited again")
  (blank)
  (do-new
    (call (. (call Task) wait))
    (on-failure failure
      (print (str "self: " (. failure message)))
    )
  )
)

(type + TaskBox
  (prop + task (? Task))
)
```

```output
running: true
task done
finished: true
waited again
self: A task cannot wait for itself
```

### A panic a handler takes tells where it was raised, on what stack, and when.

```brix
(main
  (let before (call Panic "never raised"))
  (print (str "unraised: " (== (. before stack) null) " " (== (. before raised-at) null)))
  (let task
    (do-new
      (call fails)
      (on-failure failure
        (let stack (?? (. failure stack) ""))
        (print (str "stack: " (!= (call (. stack find) "Stack trace:") null) " " (!= (call (. stack find) "__source__:fails") null)))
        (let at (?? (. failure raised-at) 0))
        (print (str "recent: " (> at 1700000000.0)))
      )
    )
  )
  (call (. task wait))
)

(fn-do _ fails _
  (throw (call Panic "broke"))
)
```

```output
unraised: true true
stack: true true
recent: true
```

### Shutdown ends a busy loop at its poll, once: the task's cleanup runs its own loop to the end and reads the termination.

```brix crash
(main
  (let ready (call Semaphore))
  (do-new-thread
    (finally
      (print (str "cleanup counted " (call spin 1000)))
      (print (str "cleanup saw " (?? (?. (. (call Task) failure) message) "nothing")))
    )
    (call (. ready post))
    (let count 0)
    (while true
      (+= count 1)
    )
    (print "unexpected end")
  )
  (call (. ready wait))
  (call request-process-termination 130)
)

(fn-do _ spin Int
  (param count Int)
  (let total 0)
  (for (in index (..< 0 count))
    (+= total index)
  )
  (return total)
)
```

```output
cleanup counted 499500
cleanup saw Task terminated
```

### A cancelled task ends at its wait, and a wait in one of its cleanups ends that cleanup; the next cleanup still runs, and the task's catch does not.

```brix
(main
  (let ready (call Semaphore))
  (let parked
    (do-new
      (finally
        (print (str "outer cleanup saw " (?? (?. (. (call Task) failure) message) "nothing")))
      )
      (finally
        (print "inner cleanup begins")
        (call (. (call Semaphore) wait))
        (print "inner cleanup goes on")
      )
      (call (. ready post))
      (call (. (call Semaphore) wait))
      (print "parked goes on")
      (on-failure failure
        (print (str "catch ran: " (. failure message)))
      )
    )
  )
  (call (. ready wait))
  (call (. parked cancel))
  (call (. parked wait))
  (print (str "cancelled: " (== (. parked end) .cancelled)))
)
```

```output
inner cleanup begins
outer cleanup saw Task cancelled
cancelled: true
```

### A cancelled task blocked in a post with no payload ends there.

```brix
(main
  (let ready (call Semaphore))
  (let poster
    (do-new
      (let gate (call Semaphore))
      (finally
        (print (str "poster cleanup saw " (?? (?. (. (call Task) failure) message) "nothing")))
      )
      (call (. ready post))
      (call (. gate post))
      (print "poster goes on")
    )
  )
  (call (. ready wait))
  (call (. poster cancel))
  (call (. poster wait))
  (print (str "cancelled: " (== (. poster end) .cancelled)))
)
```

```output
poster cleanup saw Task cancelled
cancelled: true
```

### A cancelled task waiting for a process ends there, and takes its wait off the event loop, so the process exiting later resumes nothing.

```brix
(import /os/process)
(blank)

(main
  (let ready (call Semaphore))
  (let process (call Process "sleep" (list "5")))
  (let waiter
    (do-new
      (finally
        (print (str "waiter cleanup saw " (?? (?. (. (call Task) failure) message) "nothing")))
      )
      (call (. ready post))
      (let status (call (. process wait) null))
      (print (str "waiter goes on with " (?? status -1)))
    )
  )
  (call (. ready wait))
  (call (. waiter cancel))
  (call (. waiter wait))
  (print (str "cancelled: " (== (. waiter end) .cancelled)))
  (call (. process kill))
  (print (str "reaped: " (!= (call (. process wait) null) null)))
)
```

```output
waiter cleanup saw Task cancelled
cancelled: true
reaped: true
```

### Every task waiting for a process receives its status, and a later wait gives the same one.

```brix
(import /os/process)
(blank)

(main
  (let process (call Process "sh" (list "-c" "sleep 0.1; exit 3")))
  (let first
    (do-new
      (print (str "first " (?? (call (. process wait) null) -1)))
    )
  )
  (let second
    (do-new
      (print (str "second " (?? (call (. process wait) null) -1)))
    )
  )
  (call (. first wait))
  (call (. second wait))
  (print (str "usage " (?? (?. (call (. process wait-usage)) exit-code) -1)))
)
```

```output
first 3
second 3
usage 3
```

### A TaskGroup waits for every task added to it.

```brix
(main
  (let pool (call Worker 2))
  (let group (call TaskGroup))
  (let results (list 0 0 0 0))
  (for (in index (..< 0 4))
    (let task
      (do-new (worker: pool)
        (= ([] results index) (* index index))
        (on-failure failed
          (call (. group keep) failed)
        )
      )
    )
    (call (. group add) task)
  )
  (call (. group wait))
  (print (call (. ", " join) (in (str each) results)))
  (call (. pool stop))
)
```

```output
0, 1, 4, 9
```

### The catch after each task keeps its panic, and wait raises the first kept once every task has ended. settle waits the same way and leaves the panic for wait.

```brix
(main
  (let pool (call Worker 2))
  (let outer
    (do-new
      (let group (call TaskGroup))
      (let finished (list false false false))
      (for (in index (..< 0 3))
        (let task
          (do-new (worker: pool)
            (if (== index 1)
              (throw (call Panic (str "worker " index " broke")))
            )
            (= ([] finished index) true)
            (on-failure failed
              (call (. group keep) failed)
            )
          )
        )
        (call (. group add) task)
      )
      (call (. group settle))
      (print (str "settled: " (call (. ", " join) (in (str each) finished))))
      (call (. group wait))
      (print "not reached")
      (on-failure failure
        (print (str "caught: " (. failure message)))
      )
    )
  )
  (call (. outer wait))
  (call (. pool stop))
)
```

```output
settled: true, false, true
caught: worker 1 broke
```

### close cancels a task that still runs and waits for it, as use does where its block ends; a task that has ended is left as it is. cancel on a group cancels each task.

```brix
(main
  (let parked
    (do-new
      (call (. (call Semaphore) wait))
      (print "never")
    )
  )
  (call (. parked close))
  (print (== (. parked end) .cancelled))
  (let quick
    (do-new
      (print "quick ran")
    )
  )
  (call (. quick wait))
  (call (. quick close))
  (print (== (. quick end) .finished))
  (call held)
  (let group (call TaskGroup))
  (let waiting
    (do-new
      (call (. (call Semaphore) wait))
    )
  )
  (call (. group add) waiting)
  (call (. group cancel))
  (call (. group wait))
  (print (== (. waiting end) .cancelled))
)

(fn-do _ held _
  (let outside
    (do-new
      (call (. (call Semaphore) wait))
    )
  )
  (do
    (let task (use outside))
    (print "leaving")
  )
  (print (== (. outside end) .cancelled))
)
```

```output
true
quick ran
true
leaving
true
true
```

### @detached is accepted on a task and changes nothing yet: no task has an owner.

```brix
(main
  (let task
    (do-new @detached
      (print "detached ran")
    )
  )
  (call (. task wait))
  (print (== (. task end) .finished))
)
```

```output
detached ran
true
```

### A task cancelled inside a do without cancel block runs the block to its end, waits included, and the cancellation ends it at its next wait after the block. Its handle shows the request at once.

```brix
(main
  (let ready (call Semaphore))
  (let release (call Semaphore))
  (let worker
    (do-new
      (finally
        (print (str "cleanup saw " (?? (?. (. (call Task) failure) message) "nothing")))
      )
      (do (without cancel)
        (call (. ready post))
        (call (. release wait))
        (print "shielded wait finished")
      )
      (print "after the block")
      (call (. (call Semaphore) wait))
      (print "never")
    )
  )
  (call (. ready wait))
  (call (. worker cancel))
  (print (str "requested: " (. worker is-canceled) ", in force: " (. worker is-cancel-in-force)))
  (call (. release post))
  (call (. worker wait))
  (print (str "cancelled: " (== (. worker end) .cancelled)))
)
```

```output
requested: true, in force: false
shielded wait finished
after the block
cleanup saw Task cancelled
cancelled: true
```

### Blocks without cancel nest, and the inner block's end leaves the outer one's in place. A return from inside such a block leaves it too.

```brix
(main
  (let gate (call Semaphore))
  (let nested
    (do-new
      (do (without cancel)
        (do (without cancel)
          (call (. gate wait))
        )
        (print "inner block ended")
        (call (. gate wait))
        (print "outer block ended")
      )
      (call (. gate wait))
      (print "never")
    )
  )
  (call (. nested cancel))
  (call (. gate post))
  (call (. gate post))
  (call (. nested wait))
  (print (str "nested cancelled: " (== (. nested end) .cancelled)))
  (let returner
    (do-new
      (print (str "returned " (call returns-early)))
      (call (. (call Semaphore) wait))
      (print "never")
    )
  )
  (call (. returner cancel))
  (call (. returner wait))
  (print (str "returner cancelled: " (== (. returner end) .cancelled)))
)

(fn-do _ returns-early Int
  (do (without cancel)
    (return 5)
  )
  (return 6)
)
```

```output
inner block ended
outer block ended
nested cancelled: true
returned 5
returner cancelled: true
```

### A task cancelled while it runs on a worker's threads still waits in full inside a later block without cancel.

```brix
(main
  (let pool (call Worker 2))
  (let entered (call Semaphore))
  (let release (call Semaphore))
  (let worker
    (do-new (worker: pool)
      (let busy (call spin 400000))
      (do (without cancel)
        (call (. entered post))
        (call (. release wait))
        (print (str "worker got release " (> busy 0)))
      )
      (call (. (call Semaphore) wait))
      (print "never")
    )
  )
  (call (. worker cancel))
  (call (. entered wait))
  (print "main posts")
  (call (. release post))
  (call (. worker wait))
  (print (str "cancelled: " (== (. worker end) .cancelled)))
  (call (. pool stop))
)

(fn-do _ spin Int
  (param count Int)
  (let total 0)
  (for (in index (..< 0 count))
    (+= total (% index 7))
  )
  (return total)
)
```

```output
main posts
worker got release true
cancelled: true
```

### A block without cancel does not keep out termination: the task ends at its wait there.

```brix crash
(main
  (let ready (call Semaphore))
  (do-new
    (finally
      (print (str "cleanup saw " (?? (?. (. (call Task) failure) message) "nothing")))
    )
    (do (without cancel)
      (call (. ready post))
      (call (. (call Semaphore) wait))
      (print "never")
    )
  )
  (call (. ready wait))
  (call request-process-termination 130)
)
```

```output
cleanup saw Task terminated
```

### A do without cancel block in a cancelled task's cleanup waits in full there, and lets go of its hold where it ends, so the next wait ends the cleanup. Inside it the task sees the request.

```brix
(main
  (let ready (call Semaphore))
  (let flushed (call Semaphore))
  (let writer
    (do-new
      (finally
        (do (without cancel)
          (call (. flushed wait))
          (print (str "flushed, requested: " (. (call Task) is-canceled) ", in force: " (. (call Task) is-cancel-in-force)))
        )
        (print (str "after the block, in force: " (. (call Task) is-cancel-in-force)))
        (call (. (call Semaphore) wait))
        (print "never")
      )
      (call (. ready post))
      (call (. (call Semaphore) wait))
    )
  )
  (call (. ready wait))
  (call (. writer cancel))
  (call (. flushed post))
  (call (. writer wait))
  (print (str "cancelled: " (== (. writer end) .cancelled)))
)
```

```output
flushed, requested: true, in force: false
after the block, in force: true
cancelled: true
```

### In a cleanup, a break, an error and a panic that leave a do without cancel block let go of its hold too.

```brix
(main
  (let ready (call Semaphore))
  (let looper
    (do-new
      (finally
        (for (in index (..< 0 3))
          (do (without cancel)
            (if (== index 1)
              (break)
            )
          )
        )
        (print (str "after the break, in force: " (. (call Task) is-cancel-in-force)))
        (let failure (catch (call flush)))
        (print (str "after the error " (is failure Error) ", in force: " (. (call Task) is-cancel-in-force)))
      )
      (finally
        (do (without cancel)
          (throw (call Panic "inside the block"))
        )
      )
      (call (. ready post))
      (call (. (call Semaphore) wait))
      (on-failure failed
        (print (str "caught: " (. failed message)))
      )
    )
  )
  (call (. ready wait))
  (call (. looper cancel))
  (call (. looper wait))
)

(fn-do _ flush _
  (finally
    (do (without cancel)
      (throw (call Error "flush failed"))
    )
  )
)
```

```output
after the break, in force: true
after the error true, in force: true
caught: inside the block
```

### A task a method starts is its object's. The object's close cancels it as close leaves, so the body of close may still use it, and close does not wait for it. A task marked @detached outlives the close.

```brix
(main
  (let relay (call Relay))
  (call (. relay start))
  (call (. relay close))
  (call (. (!! (. relay worker)) wait))
  (print (str "worker cancelled: " (== (. (!! (. relay worker)) end) .cancelled)))
  (call (. relay gate post))
  (call (. (!! (. relay keeper)) wait))
  (print (str "keeper finished: " (== (. (!! (. relay keeper)) end) .finished)))
)

(type + Relay
  (prop + asked _ (= (call Semaphore)))
  (prop + answered _ (= (call Semaphore)))
  (prop + gate _ (= (call Semaphore)))
  (prop + worker (? Task))
  (prop + keeper (? Task))
  (fn-do + start _
    (= worker
      (do-new
        (finally
          (print "worker cleanup")
        )
        (while true
          (call (. asked wait))
          (print "worker answered")
          (call (. answered post))
        )
      )
    )
    (= keeper
      (do-new @detached
        (call (. gate wait))
        (print "keeper ran after close")
      )
    )
  )
  (fn-do + close _
    (call (. asked post))
    (call (. answered wait))
    (print "close used its worker")
  )
)
```

```output
worker answered
close used its worker
worker cleanup
worker cancelled: true
keeper ran after close
keeper finished: true
```

### A task started in a nested body of a method, a task's or a function value's, is the object's too. A task that closes its own object is not cancelled by that close, and a task started after a close is the object's until the next one.

```brix
(main
  (let done (call Semaphore))
  (let holder (call Holder done))
  (call (. holder start-nested))
  (call (. holder close))
  (call (. done wait))
  (call (. done wait))
  (print "nested tasks cancelled")
  (call (. holder start-self-closing))
  (call (. (!! (. holder task)) wait))
  (print (str "self closer cancelled by the next close: " (== (. (!! (. holder task)) end) .cancelled)))
)

(type + Holder
  (prop + done Semaphore)
  (prop + task (? Task))
  (fn-do + This _
    (param done Semaphore)
  )
  (fn-do + start-nested _
    (let signal done)
    (let ready (call Semaphore))
    (do-new @detached
      (do-new
        (finally
          (call (. signal post))
        )
        (call (. (call Semaphore) wait))
      )
      (call (. ready post))
    )
    (call (. ready wait))
    (let run
      (=> ()
        (block
          (do-new
            (finally
              (call (. signal post))
            )
            (call (. (call Semaphore) wait))
          )
          true
        )
      )
    )
    (let ran (call run))
  )
  (fn-do + start-self-closing _
    (let started (call Semaphore))
    (= task
      (do-new
        (call close)
        (print "still running after closing its object")
        (call (. started post))
        (call (. (call Semaphore) wait))
      )
    )
    (call (. started wait))
    (call close)
  )
  (fn-do + close _)
)
```

```output
nested tasks cancelled
still running after closing its object
self closer cancelled by the next close: true
```

### A method of a value type starts a task no one owns. An object's task on a worker's threads is cancelled by its close as one on the main queue is, and a task started while markup builds an element is the element's.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let done (call Semaphore))
  (call (. (call Pair 5) spawn) done)
  (call (. done wait))
  (print "the value's task ran")
  (let pool (call Worker 2))
  (let pooled (call Pooled pool))
  (call (. pooled start))
  (call (. pooled close))
  (call (. (!! (. pooled task)) wait))
  (print (str "pool task cancelled: " (== (. (!! (. pooled task)) end) .cancelled)))
  (call (. pool stop))
  (let entered (call Semaphore))
  (<> Box (id: box)
    (do-new
      (finally
        (call (. done post))
      )
      (call (. entered post))
      (call (. (call Semaphore) wait))
    )
  )
  (call (. entered wait))
  (call (. box close))
  (call (. done wait))
  (print "the element's task cancelled")
)

(type + Pair (is Tuple)
  (prop + a Int)
  (fn-do + spawn _
    (param done Semaphore)
    (do-new
      (call (. done post))
    )
  )
)

(type + Pooled
  (prop + pool Worker)
  (prop + task (? Task))
  (fn-do + This _
    (param pool Worker)
  )
  (fn-do + start _
    (let entered (call Semaphore))
    (= task
      (do-new (worker: pool)
        (finally
          (print "pool task cleanup")
        )
        (call (. entered post))
        (call (. (call Semaphore) wait))
      )
    )
    (call (. entered wait))
  )
  (fn-do + close _)
)
```

```output
the value's task ran
pool task cleanup
pool task cancelled: true
the element's task cancelled
```

### A task a state case starts in its entry rows, in a handler, or in a nested body of either belongs to the case's activation: its exit rows still use it, and it is cancelled as they end. An inner case's exit leaves the outer case's tasks. A task marked @detached is no one's, and one the exit rows start is the object's. Entering the case again makes a new activation, and an object's close leaves its machines, which cancels theirs.

```brix
(main
  (let runner (call Runner))
  (<- (. runner enter) true)
  (for (in _ (..< 0 3))
    (call (. runner entered wait))
  )
  (<- (. runner ticks) 1)
  (call (. runner entered wait))
  (<- (. runner inner) true)
  (call (. runner entered wait))
  (<- (. runner outer) true)
  (call (. (!! (. runner inner-task)) wait))
  (print (str "inner case's task cancelled: " (== (. (!! (. runner inner-task)) end) .cancelled)))
  (print (str "outer case's task running: " (== (. (!! (. runner worker)) end) .running)))
  (<- (. runner leave) true)
  (for (in task (list (!! (. runner worker)) (!! (. runner nested)) (!! (. runner closure)) (!! (. runner handler))))
    (call (. task wait))
    (print (str "cancelled: " (== (. task end) .cancelled)))
  )
  (call (. runner gate post))
  (call (. (!! (. runner loose)) wait))
  (print (str "detached finished: " (== (. (!! (. runner loose)) end) .finished)))
  (let lingering (!! (. runner after)))
  (print (str "exit rows' task running: " (== (. lingering end) .running)))
  (<- (. runner enter) true)
  (for (in _ (..< 0 3))
    (call (. runner entered wait))
  )
  (call (. runner close))
  (call (. (!! (. runner worker)) wait))
  (print (str "second worker cancelled by close: " (== (. (!! (. runner worker)) end) .cancelled)))
  (call (. lingering wait))
  (print (str "exit rows' task cancelled by close: " (== (. lingering end) .cancelled)))
  (call (. runner gate post))
  (call (. (!! (. runner loose)) wait))
)

(type + Runner
  (event + enter Bool)
  (event + leave Bool)
  (event + inner Bool)
  (event + outer Bool)
  (event + ticks Int)
  (prop + entered _ (= (call Semaphore)))
  (prop + asked _ (= (call Semaphore)))
  (prop + answered _ (= (call Semaphore)))
  (prop + gate _ (= (call Semaphore)))
  (prop + worker (? Task))
  (prop + nested (? Task))
  (prop + closure (? Task))
  (prop + handler (? Task))
  (prop + loose (? Task))
  (prop + inner-task (? Task))
  (prop + after (? Task))
  (prop + flow _
    (machine
      (state idle)
      (state active (trigger on enter)
        (= worker
          (do-new
            (finally
              (print "worker cleanup")
            )
            (call (. entered post))
            (while true
              (call (. asked wait))
              (call (. answered post))
            )
          )
        )
        (do-new @detached
          (= nested
            (do-new
              (call (. entered post))
              (call (. (call Semaphore) wait))
            )
          )
        )
        (let start
          (=> () :do
            (block
              (= closure
                (do-new
                  (call (. entered post))
                  (call (. (call Semaphore) wait))
                )
              )
              (return true)
            )
          )
        )
        (let started (call start))
        (= loose
          (do-new @detached
            (call (. gate wait))
            (print "detached ran after exit")
          )
        )
        (for (on tick ticks)
          (= handler
            (do-new
              (call (. entered post))
              (call (. (call Semaphore) wait))
            )
          )
        )
        (finally
          (call (. asked post))
          (call (. answered wait))
          (print "exit rows used the worker")
          (= after
            (do-new
              (call (. (call Semaphore) wait))
            )
          )
        )
        (blank)
        (state waiting)
        (state busy (trigger on inner)
          (= inner-task
            (do-new
              (call (. entered post))
              (call (. (call Semaphore) wait))
            )
          )
          (then waiting (trigger on outer))
        )
        (then idle (trigger on leave))
      )
    )
  )
  (fn-do + close _)
)
```

```output
inner case's task cancelled: true
outer case's task running: true
exit rows used the worker
worker cleanup
cancelled: true
cancelled: true
cancelled: true
cancelled: true
detached ran after exit
detached finished: true
exit rows' task running: true
exit rows used the worker
worker cleanup
second worker cancelled by close: true
exit rows' task cancelled by close: true
detached ran after exit
```

### A machine in a function with no receiver owns its cases' tasks as one a class holds does. A task a transition's rows start belongs to the activation of the case the transition enters.

```brix
(prop + enters (of AsyncChannel Bool))
(prop + leaves (of AsyncChannel Bool))
(prop + stops (of AsyncChannel Bool))
(prop + entered _ (= (call Semaphore)))
(prop + worker (? Task) (= null))
(prop + stopper (? Task) (= null))
(blank)

(main
  (state idle)
  (state active (trigger on enters)
    (= worker
      (do-new
        (finally
          (print "worker cleanup")
        )
        (call (. entered post))
        (call (. (call Semaphore) wait))
      )
    )
    (then stopped (trigger on stops)
      (= stopper
        (do-new
          (finally
            (print "stopper cleanup")
          )
          (call (. entered post))
          (call (. (call Semaphore) wait))
        )
      )
    )
    (then idle (trigger on leaves))
  )
  (state stopped
    (then idle (trigger on leaves))
  )
  (blank)
  (<- enters true)
  (call (. entered wait))
  (<- stops true)
  (call (. (!! worker) wait))
  (print (str "main machine task cancelled: " (== (. (!! worker) end) .cancelled)))
  (call (. entered wait))
  (print (str "stopper running in stopped: " (== (. (!! stopper) end) .running)))
  (<- leaves true)
  (call (. (!! stopper) wait))
  (print (str "stopper cancelled as stopped exits: " (== (. (!! stopper) end) .cancelled)))
)
```

```output
worker cleanup
main machine task cancelled: true
stopper running in stopped: true
stopper cleanup
stopper cancelled as stopped exits: true
```

### A subclass's refinement that adds entry rows or a handler to a case the base writes owns the tasks they start for the case's activation, cancelled as the case exits, its own exit rows first.

```brix
(main
  (let sub (call Sub))
  (call (. sub entered wait))
  (call (. sub entered wait))
  (<- (. sub leaves) true)
  (call (. (!! (. sub base-task)) wait))
  (call (. (!! (. sub sub-task)) wait))
  (print (str "base task cancelled: " (== (. (!! (. sub base-task)) end) .cancelled)))
  (print (str "sub entry task cancelled: " (== (. (!! (. sub sub-task)) end) .cancelled)))
  (<- (. sub busies) true)
  (<- (. sub ticks) 1)
  (call (. sub entered wait))
  (<- (. sub leaves) true)
  (call (. (!! (. sub handler)) wait))
  (print (str "sub handler task cancelled: " (== (. (!! (. sub handler)) end) .cancelled)))
)

(type + Thing
  (event + leaves Bool)
  (event + busies Bool)
  (event + ticks Int)
  (prop + entered _ (= (call Semaphore)))
  (prop + base-task (? Task))
  (prop + sub-task (? Task))
  (prop + handler (? Task))
  (prop + body _
    (machine
      (state free
        (= base-task
          (do-new
            (finally
              (print "base cleanup")
            )
            (call (. entered post))
            (call (. (call Semaphore) wait))
          )
        )
        (then away (trigger on leaves))
      )
      (state away
        (then busy (trigger on busies))
      )
      (state busy
        (then away (trigger on leaves))
      )
    )
  )
  (fn-do + close _)
)

(type + Sub (is Thing)
  (prop + body _
    (machine
      (state free
        (= sub-task
          (do-new
            (finally
              (print "sub cleanup")
            )
            (call (. entered post))
            (call (. (call Semaphore) wait))
          )
        )
      )
      (state busy
        (for (on n ticks)
          (= handler
            (do-new
              (call (. entered post))
              (call (. (call Semaphore) wait))
            )
          )
        )
      )
    )
  )
)
```

```output
sub cleanup
base cleanup
base task cancelled: true
sub entry task cancelled: true
sub handler task cancelled: true
```

### A task that leaves its own case, by sending what moves the machine, is not cancelled as it leaves: the case's next exit cancels it, or its object's close. A case's task that closes its own object goes on running, and the object's next close cancels it.

```brix
(main
  (let runner (call Runner))
  (<- (. runner enter) true)
  (call (. runner entered wait))
  (print (str "left by its own task: " (is (. runner flow) .idle)))
  (print (str "still running: " (== (. (!! (. runner first)) end) .running)))
  (<- (. runner enter) true)
  (call (. runner entered wait))
  (<- (. runner leave) true)
  (call (. (!! (. runner first)) wait))
  (print (str "cancelled by the next exit: " (== (. (!! (. runner first)) end) .cancelled)))
  (<- (. runner enter) true)
  (call (. runner entered wait))
  (call (. runner close))
  (call (. (!! (. runner third)) wait))
  (print (str "cancelled by close: " (== (. (!! (. runner third)) end) .cancelled)))
  (let closer (call Closer))
  (<- (. closer enter) true)
  (call (. closer entered wait))
  (call (. closer close))
  (call (. (!! (. closer worker)) wait))
  (print (str "closer cancelled by the next close: " (== (. (!! (. closer worker)) end) .cancelled)))
)

(type + Runner
  (event + enter Bool)
  (event + leave Bool)
  (prop + entered _ (= (call Semaphore)))
  (prop + entries _ (= 0))
  (prop + first (? Task))
  (prop + third (? Task))
  (prop + flow _
    (machine
      (state idle)
      (state active (trigger on enter)
        (+= entries 1)
        (let leaves (!= entries 2))
        (let task
          (do-new
            (if leaves
              (<- leave true)
              (print "left its own case")
            )
            (call (. entered post))
            (call (. (call Semaphore) wait))
          )
        )
        (if (== entries 1)
          (= first task)
          (else-if (== entries 3)
            (= third task)
          )
        )
        (then idle (trigger on leave))
      )
    )
  )
  (fn-do + close _)
)

(type + Closer
  (event + enter Bool)
  (prop + entered _ (= (call Semaphore)))
  (prop + worker (? Task))
  (prop + flow _
    (machine
      (state idle)
      (state active (trigger on enter)
        (= worker
          (do-new
            (call close)
            (print "closed its own object")
            (call (. entered post))
            (call (. (call Semaphore) wait))
          )
        )
      )
    )
  )
  (fn-do + close _)
)
```

```output
left its own case
left by its own task: true
still running: true
cancelled by the next exit: true
left its own case
cancelled by close: true
closed its own object
closer cancelled by the next close: true
```

### A task terminated while it awaits a completion leaves it abandoned, and its cleanup runs to its end: the termination is raised once.

```brix crash
(import /time/duration)
(blank)

(main
  (do-new
    (let completion (call (of Completion Int)))
    (finally
      (print (str "abandoned: " (. completion is-abandoned)))
      (let failure (. (call Task) failure))
      (if failure
        (print (str "cleanup saw " (. failure message)))
      )
      (print "cleanup done")
    )
    (call (. completion await))
    (print "unexpected resume")
  )
  (blank)
  (call (. 50ms wait))
  (call request-process-termination 130)
)
```

```output
abandoned: true
cleanup saw Task terminated
cleanup done
```

### Finishing a completion wakes only the tasks that await it: another task may await it, and its creator's unrelated wait is left alone.

```brix
(main
  (let completion (call (of Completion Int)))
  (let gate (call (of Semaphore Int)))
  (let other
    (do-new
      (print (str "other awaited " (call (. completion await))))
    )
  )
  (do-new
    (call (. completion finish) 41)
    (do-new
      (call (. gate post) 7)
    )
  )
  (print (str "gate " (call (. gate await))))
  (print (str "main awaited " (call (. completion await))))
  (call (. other wait))
)
```

```output
other awaited 41
gate 7
main awaited 41
```

### `cancel-later` cancels a task without yielding to it: the active task goes on, and the task ends at its wait once its turn comes.

```brix
(main
  (let ready (call Semaphore))
  (let task
    (do-new
      (finally
        (print "worker cleanup")
      )
      (call (. ready post))
      (call (. (call Semaphore) wait))
      (print "unexpected resume")
    )
  )
  (blank)
  (call (. ready wait))
  (call (. task cancel-later))
  (print "main goes on")
  (call (. task wait))
  (print (str "cancelled: " (== (. task end) .cancelled)))
)
```

```output
main goes on
worker cleanup
cancelled: true
```

### A task that `cancel-later` cancels before it runs runs up to its first wait, and ends there.

```brix
(main
  (let task
    (do-new
      (print "runs up to its first wait")
      (call (. (call Semaphore) wait))
      (print "unexpected resume")
    )
  )
  (blank)
  (call (. task cancel-later))
  (call (. task wait))
  (print (str "cancelled: " (== (. task end) .cancelled)))
)
```

```output
runs up to its first wait
cancelled: true
```

### A generator a task reads sees the global `do with` bound where the task was started.

```brix
(main
  (call run)
)

(fn-do + run _
  (do-with level 5
    (let s (call Session (call Provider)))
    (call (. s send))
    (call (. (!! (. s task)) wait))
  )
)

(prop + level Int @stack)

(type + Provider
  (fn-do + stream (stream Int)
    (yield level)
    (yield (+ level 1))
  )
)

(type + Session
  (prop + provider Provider)
  (prop + task (? Task))
  (fn-do + This _
    (param provider Provider)
  )
  (fn-do + send _
    (= task
      (do-new
        (for (in each (call (. provider stream)))
          (print each)
        )
      )
    )
  )
)
```

```output
5
6
```
