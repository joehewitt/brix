# Subscriptions

### Basic subscriber

```brix
(prop + numbers (of AsyncChannel Int))
(blank)

(main
  (for (on n numbers)
    (print n)
  )
  (blank)
  (for (in x (.. 0 2))
    (<- numbers x)
  )
  ; numbers.close()
  (print "done")
)
```

```output
done
0
1
2
```

### A subscriber runs on the queue of the task that subscribed, whichever thread publishes.

```brix
(main
  (let worker (call Worker 1))
  (let numbers (of AsyncChannel Int))
  (let done (channel Int))
  (= place "main")
  (for (on n numbers)
    (print (str n " on " place))
    (<- done n)
  )
  (do-new (worker: worker)
    (= place "worker")
    (<- numbers 1)
  )
  (print (str "read " (<- done)))
  (<- numbers 2)
  (print (str "read " (<- done)))
  (call (. worker stop))
)

(blank)
(prop + place _ @thread (= "unset"))
```

```output
1 on main
read 1
2 on main
read 2
```

### Subscriber with condition

```brix
(prop + numbers (of AsyncChannel Int))
(blank)

(main
  (for (on n numbers (when (!= n 1)))
    (print n)
  )
  (blank)
  (<-* numbers (.. 0 2))
  ; numbers.close()
  (print "done")
)
```

```output
done
0
2
```

### Subscriber with while condition

```brix
(prop + numbers (of AsyncChannel Int))
(blank)

(main
  (for (on n numbers (while (!= n 3)))
    (print n)
  )
  (blank)
  (<-* numbers (.. 0 5))
  ; numbers.close()
  (print "done")
)
```

```output
done
0
1
2
```

```brix
(prop + numbers (of AsyncChannel Int))
(blank)

(main
  (for (on n numbers (until (== n 3)))
    (print n)
  )
  (blank)
  (<-* numbers (.. 0 5))
  ; numbers.close()
  (print "done")
)
```

```output
done
0
1
2
```

### Break to unsubscribe

```brix
(prop + numbers (of AsyncChannel Int))
(blank)

(main
  (for (on n numbers)
    (print n)
    (if (== n 1)
      (break)
    )
  )
  (blank)
  (<-* numbers (.. 0 2))
  ; numbers.close()
  (print "done")
)
```

```output
done
0
1
```

```brix crash
(prop + numbers (of AsyncChannel Int))
(blank)

(main
  (for (on foo numbers)
    (return)
  )
  (print numbers)
)
```

```output
Exception: The body of a loop on "numbers" cannot return with "="; skip to the next value with "continue", or stop the loop with "break"
<no file:7>
        (return)
        ^^^^^^^^
```

### Continue to exit subscriber

```brix
(prop + numbers (of AsyncChannel Int))
(blank)

(main
  (for (on foo numbers)
    (print "in")
    (continue)
    (print "out")
  )
  (blank)
  (<- numbers 1)
)
```

```output
in
```

### Continue ends one run of a class's event handler

```brix
(main
  (let spot (call Spot))
  (<- (. spot pings) true)
  (<- (. spot pings) false)
  (<- (. spot pings) true)
  (print (. spot taps))
)

(type + Spot
  (prop + taps Size (= 0))
  (event + pings Bool)
  (subscribe-for (on event pings)
    (if (not event)
      (continue)
    )
    (+= taps 1)
  )
)
```

```output
2
```

### Combined subscriptions

```brix crash
(prop + numbers1 (of AsyncChannel Int))
(prop + numbers2 (of AsyncChannel Int))
(blank)

(main
  (for (on foo numbers1)
    (and-for (on bar numbers2)
      (print "in")
      (continue)
      (print "out")
    )
  )
  (blank)
  (<- numbers1 1)
  (<- numbers2 2)
)
```

```output
Exception: A loop on a stream subscribes to one source, so it cannot be joined to another; write a loop for each source
<no file:7>
      (for (on foo numbers1)
           ^^^^^^^^^^^^^^^^^
```

### A loop on a stream joined to another by an or arm

```brix compile-error
(prop + numbers1 (of AsyncChannel Int))
(prop + numbers2 (of AsyncChannel Int))
(blank)

(main
  (for (on foo numbers1)
    (print "one")
    (or-for (on bar numbers2)
      (print "two")
    )
  )
)
```

```output
Exception: A loop on a stream subscribes to one source, so it cannot be joined to another; write a loop for each source
<no file:7>
      (for (on foo numbers1)
           ^^^^^^^^^^^^^^^^^
```

### Declarative subscription in module

```brix
(prop + numbers (of SyncBufferedChannel Int))
(prop + doubles _ (= (call doubler numbers)))
(blank)

(main
  (for (on n doubles)
    (print n)
  )
  (blank)
  (<-* numbers (.. 0 2))
  (call (. numbers close))
  (print "done")
)

(fn-do + task _
  (param outputs _)
  (param inputs _)
  (for (in n inputs)
    (print (str "double " n))
    (<- outputs (* n 2))
  )
  ; outputs.close()
)

(fn-do + doubler _
  (param inputs _)
  (let outputs (of AsyncChannel Int))
  (do-new
    (inline (call task outputs inputs))
  )
  (return outputs)
)
```

```output
done
double 0
double 1
double 2
0
2
4
```

### Declarative subscription in class

```brix
(main
  (let thing (call Thing))
  (<-* (. thing numbers) (.. 0 2))
  (call (. thing numbers close))
  (print "done")
)

(type + Thing
  (prop + numbers (of SyncBufferedChannel Int))
  (prop + doubles _ (= (call doubler numbers)))
  (subscribe-for (on n doubles)
    (print n)
  )
)

(fn-do + task _
  (param outputs _)
  (param inputs _)
  (for (in n inputs)
    (print (str "double " n))
    (<- outputs (* n 2))
  )
)

(fn-do + doubler _
  (param inputs _)
  (let outputs (of AsyncChannel Int))
  (do-new
    (inline (call task outputs inputs))
  )
  (return outputs)
)
```

```output
done
double 0
double 1
double 2
0
2
4
```

### A class's subscription calls a method of the class by its name alone where a function of another module has the name.

```brix
(main
  (let spot (call Spot))
  (<- (. spot pings) true)
  (print (. spot taps))
)

(type + Spot
  (prop + taps Size (= 0))
  (event + pings Bool)
  (subscribe-for (on event pings)
    (call tap)
  )
  (fn-do _ tap _
    (+= taps 1)
  )
)
```

```output
1
```

### Illegal broadcaster

```brix crash
(main
  (print "")
  (blank)
  (for (on n 42)
    (print n)
  )
)
```

```output
Exception: Object is not a broadcastable or iterable: I64
<no file:5>
      (for (on n 42)
      ^^^^^^^^^^^^^
```

### Subscriber expression

```brix
(main
  (<-* numbers (.. 0 2))
  (call (. numbers close))
  (print "done")
)

(blank)
(prop _ numbers (of SyncBufferedChannel Int))
(prop _ doubles _ (= (on (select (* (call dump n) 2) (<* n)) numbers)))
(blank)

(main
  (for (on n doubles)
    (print n)
  )
)

(fn-do + dump _
  (param n _)
  (print (str "got " n))
  (return n)
)
```

```output
done
got 0
got 1
got 2
0
2
4
```

### Subscriber expression with condition

```brix
(main
  (<-* numbers (.. 0 2))
  (call (. numbers close))
  (print "done")
)

(blank)
(prop _ numbers (of SyncBufferedChannel Int))
(prop _ doubles _
  (=
    (case-each (on n numbers (when (> n 1)))
      (inline (* (call dump n) 2))
    )
  )
)
(blank)

(main
  (for (on n doubles)
    (print n)
  )
)

(fn-do + dump _
  (param n _)
  (print (str "got " n))
  (return n)
)
```

```output
done
got 2
4
```

```brix
(main
  (<-* numbers (.. 0 8))
  (call (. numbers close))
  (print "done")
)

(blank)
(prop _ numbers (of SyncBufferedChannel Int))
(prop _ doubles _ (= (on (select (* (call dump n) 2) (<* n)) numbers (while (< n 5)))))
(blank)

(main
  (for (on n doubles)
    (print n)
  )
)

(fn-do + dump _
  (param n _)
  (print (str "got " n))
  (return n)
)
```

```output
done
got 0
got 1
got 2
got 3
got 4
got 5
0
2
4
6
8
```

### Subscribe to an iterable object

```brix
(main
  (print "before")
  (for (on n (list 1 2 3))
    (print n)
  )
  (print "after")
)
```

```output
before
after
1
2
3
```

```brix
(main
  (call Foo)
)

(type + Foo
  (prop + bar _ (= (list 1 2 3)))
  (prop + bee _ (= (list 1 2 3)))
  (subscribe (on bar)
    (print "bar")
  )
  (subscribe-for (on x bee)
    (print x)
  )
)
```

```output
bar
bar
bar
1
2
3
```

```brix
(main
  (call Foo)
)

(type + Foo (is Bar)
  (prop + a _ (= 0))
  (subscribe (on stuff)
    (print this)
    (print "Foo.stuff")
  )
)

(type + Bar
  (prop + stuff _ (= (list 1)))
  (subscribe (on stuff)
    (print this)
    (print "Bar.stuff")
  )
)
```

```output
[Object __source__:Bar]
Bar.stuff
[Object __source__:Foo]
Foo.stuff
```

```brix
(main
  (let a (of AsyncChannel Int))
  (do-new
    (for (in i (..< 0 3))
      (call yield-once)
      (<- a i)
    )
    ; a.close()
  )
  (blank)
  (let nums1 (on (select (* n 2) (<* (: n (similar Float)))) a))
  (let nums2 (on each a))
  (blank)
  (for (on n nums1)
    (print n)
  )
  (for (on n nums2)
    (print n)
  )
)

;
(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```output
0
0
2
1
4
2
```

```brix
(main
  (let a (of AsyncChannel Int))
  (do-new
    (for (in i (..< 0 3))
      (call yield-once)
      (<- a i)
    )
    ; a.close()
  )
  (blank)
  (let x 3)
  (let nums1 (on (select (* n x) (<* (: n (similar Float)))) a))
  (let nums2 (on each a))
  (blank)
  (for (on n nums1)
    (print n)
  )
  (for (on n nums2)
    (print n)
  )
)

;
(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```output
0
0
3
1
6
2
```

### This tested a messy situation involving creating a fragment without pushing a catcher inside it, resulting in a refernce to the unwind block inside the fragment referencing a block in a different function, thus crashing LLVM

```brix
(main
  (lazy stuff
    (iterate (on (..< 0 3))
      (block
        (case? true
          (. Choices choice)
        )
        (default
          (. Choices choice)
        )
      )
    )
  )
  (blank)
  (lazy filtered (on (select a (<* (: a Int))) stuff))
  (blank)
  (for (on x filtered)
    (print x)
  )
)

(type + Choices (is Union)
  (variant choice (= 1))
)
```

```output
1
1
1
```

```brix
(main
  (lazy is-open true)
  (lazy ticks 0)
  (let timer (of AsyncChannel F64))
  (blank)
  (let dump
    (=> (t) :do
      (block
        (print t)
        (+= ticks 1)
        (return t)
      )
    )
  )
  (blank)
  (let stuff
    (case-each (on t timer (whenever #is-open))
      (inline (call dump t))
    )
  )
  (blank)
  (<- timer 0.03)
  (call yield-once)
  (<- timer 0.06)
  (call yield-once)
  (print "STOP!")
  (= is-open false)
  (blank)
  (<- timer 0.07)
  (call yield-once)
  (print "START!")
  (let start-ticks ticks)
  (= is-open true)
  (call yield-once)
  (blank)
  (<- timer 0.03)
  (call yield-once)
  (<- timer 0.06)
  (call yield-once)
  (<- timer 0.09)
  (call yield-once)
  (print "STOP!")
  (= is-open false)
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```output
0.03
0.06
STOP!
START!
0.03
0.06
0.09
STOP!
```

```brix
(main
  (lazy is-open true)
  (lazy ticks 0)
  (let timer (of AsyncChannel F64))
  (blank)
  (for (on t timer (whenever #is-open))
    (print t)
    (+= ticks 1)
  )
  (blank)
  (<- timer 0.03)
  (call yield-once)
  (<- timer 0.06)
  (call yield-once)
  (print "STOP!")
  (= is-open false)
  (blank)
  (<- timer 0.07)
  (call yield-once)
  (print "START!")
  (let start-ticks ticks)
  (= is-open true)
  (call yield-once)
  (blank)
  (<- timer 0.03)
  (call yield-once)
  (<- timer 0.06)
  (call yield-once)
  (<- timer 0.09)
  (call yield-once)
  (print "STOP!")
  (= is-open false)
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```output
0.03
0.06
STOP!
START!
0.03
0.06
0.09
STOP!
```

### Illegal to refer to loop counter in whenever expressions

```brix crash
(main
  (case-each (on n (..< 0 3) (whenever (> n 0)))
    (print n)
  )
)
```

```output
Exception: Name "n" not found
<no file:3>
      (case-each (on n (..< 0 3) (whenever (> n 0)))
                                              ^
```

### whenever clauses require tracked reads

```brix crash
(main
  (lazy is-open true)
  (let timer (of AsyncChannel F64))
  (blank)
  (for (on t timer (whenever is-open))
    (print t)
  )
)
```

```output
Exception: whenever clauses must read at least one tracked property (like #is-open)
<no file:6>
      (for (on t timer (whenever is-open))
                       ^^^^^^^^^^^^^^^^^^
```

### Broadcastable readable adapter returns no message when subscription is rejected

```brix
(import /core/stream)
(blank)

(main
  (let source (call RejectingBroadcastable))
  (let message (<- source))
  (print (?? message "none"))
)

(type + RejectingBroadcastable
  (implements (of Broadcastable String))
  (fn-expr + is-closed Bool :get
    false
  )
  (fn-do + subscribe _ (?! Error)
    (param subscriber (of Subscriber String))
    (throw (call Error "subscribe rejected"))
  )
  (fn-do + unsubscribe _ (?! Error)
    (param subscriber (of Subscriber String))
  )
  (fn-do + subscribe _ (?! Error)
    (param subscriber (of Subscribable String))
    (throw (call Error "subscribe rejected"))
  )
  (fn-do + unsubscribe _ (?! Error)
    (param subscriber (of Subscribable String))
  )
)
```

```output
none
```

### A dead event keeps receiver effects but erases payload construction

```brix
(main
  (<- (. (call receiver) dead) (call make-payload))
)

(type + DeadSurfaceSource
  (event + dead Int)
)

(fn-do + receiver DeadSurfaceSource
  (print "receiver")
  (return (call DeadSurfaceSource))
)

(fn-do + make-payload Int
  (print "built")
  (return 1)
)
```

```output
receiver
```

### A subscriber reached through an escaped adapter keeps the event live

```brix
(main
  (let source (call EscapedAdapterSource))
  (let endpoint (of Broadcastable Int) (. source changed))
  (blank)
  (for (on value endpoint)
    (print (str "escaped " value))
  )
  (blank)
  (call (. source emit) 17)
)

(type + EscapedAdapterSource
  (event + changed Int)
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)
```

```output
escaped 17
```

### Event declarations publish synchronously

```brix
(main
  (let source (call EventSource))
  (blank)
  (for (on value (. source changed))
    (print (str "heard " value))
  )
  (blank)
  (print "before")
  (call (. source emit) 42)
  (print "after")
)

(type + EventSource
  (event + changed Int)
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)
```

```output
before
heard 42
after
```

### Event class subscriptions are installed base-first before external subscriptions

```brix
(main
  (let source (call DerivedEventSource))
  (blank)
  (for (on value (. source changed))
    (print (str "external " value))
  )
  (blank)
  (call (. source emit) 7)
)

(type + BaseEventSource
  (event + changed Int)
  (subscribe-for (on value changed)
    (print (str "base " value))
  )
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)

(type + DerivedEventSource (is BaseEventSource)
  (subscribe-for (on value changed)
    (print (str "derived " value))
  )
)
```

```output
base 7
derived 7
external 7
```

### Constructor-body subscriptions are delivered before class-body subscriptions

```brix
(main
  (let direct (call ConstructorOrderedEventSource))
  (call (. direct emit) 7)
  (blank)
  (let inherited (call ConstructorOrderedEventChild))
  (call (. inherited emit) 9)
)

(type + ConstructorOrderedEventSource
  (event + changed Int)
  (fn-do + This _
    (for (on value changed)
      (print (str "constructor " value))
    )
  )
  (subscribe-for (on value changed)
    (print (str "class body " value))
  )
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)

(type + ConstructorOrderedEventBase
  (event + changed Int)
  (subscribe-for (on value changed)
    (print (str "base class body " value))
  )
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)

(type + ConstructorOrderedEventChild (is ConstructorOrderedEventBase)
  (fn-do + This _
    (for (on value changed)
      (print (str "derived constructor " value))
    )
  )
)
```

```output
constructor 7
class body 7
derived constructor 9
base class body 9
```

### Event payload evaluation is lazy when there are no subscribers

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let source (call LazyEventSource))
  (let event-lane (call C:brixGCEventsSidePayloadLane))
  (let sidecars-before (call C:brixGCObjectSidePayloadCount event-lane))
  (call (. source emit))
  (print (== (call C:brixGCObjectSidePayloadCount event-lane) sidecars-before))
  (blank)
  (for (on value (. source changed))
    (print value)
  )
  (blank)
  (for (on value (. source other))
    (do
      (inline value)
    )
  )
  (blank)
  (print (== (call C:brixGCObjectSidePayloadCount event-lane) (+ sidecars-before 1)))
  (call (. source emit))
)

(type + LazyEventSource
  (event + changed Int)
  (event + other String)
  (fn-do + emit _
    (<- changed (call make-value))
  )
  (fn-do + make-value Int
    (print "made")
    (return 9)
  )
)
```

```output
true
true
made
9
```

### A value function that ends with a publish must still return

```brix crash
(main
  (let source (call PublishFallthroughSource))
  (print (call (. source compute)))
)

(type + PublishFallthroughSource
  (event + changed Int)
  (fn-do + compute Int
    (<- changed 1)
  )
)
```

```output
Exception: Function compute can fall through without returning I64
<no file:9>
      (fn-do + compute Int
      ^^^^^^^^^^^^^^^^^^^
```

### Subscriptions added during dispatch start with the next publication

```brix
(main
  (lazy installed false)
  (let source (call MutatingEventSource))
  (blank)
  (for (on value (. source changed))
    (print (str "first " value))
    (if (not installed)
      (= installed true)
      (call install-late source)
    )
  )
  (blank)
  (call (. source emit) 1)
  (call (. source emit) 2)
)

(fn-do + install-late _
  (param source MutatingEventSource)
  (for (on value (. source changed))
    (print (str "late " value))
  )
)

(type + MutatingEventSource
  (event + changed Int)
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)
```

```output
first 1
first 2
late 2
```

### A later event subscriber can break its loop without disturbing earlier subscribers

```brix
(main
  (let source (call BreakingEventSource))
  (blank)
  (for (on value (. source changed))
    (print (str "first " value))
  )
  (blank)
  (for (on value (. source changed))
    (print (str "later " value))
    (break)
  )
  (blank)
  (call (. source emit) 1)
  (call (. source emit) 2)
)

(type + BreakingEventSource
  (event + changed Int)
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)
```

```output
first 1
later 1
first 2
```

### Nested event publication is depth-first

```brix
(main
  (let source (call NestedEventSource))
  (blank)
  (for (on value (. source changed))
    (print (str "first " value))
    (if (== value 1)
      (call (. source emit) 2)
    )
  )
  (blank)
  (for (on value (. source changed))
    (print (str "second " value))
  )
  (blank)
  (call (. source emit) 1)
)

(type + NestedEventSource
  (event + changed Int)
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)
```

```output
first 1
first 2
second 2
second 1
```

### Closing an event owner detaches its event subscriptions

```brix
(main
  (let source (call ClosableEventSource))
  (blank)
  (for (on value (. source changed))
    (print value)
  )
  (blank)
  (call (. source emit) 1)
  (call (. source close))
  (call (. source emit) 2)
)

(type + ClosableEventSource
  (event + changed Int)
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
  (fn-do + close _)
)
```

```output
1
```

### Event endpoint escape is a memoized read-only Broadcastable adapter

```brix
(main
  (let source (call EscapedEventSource))
  (let first (of Broadcastable Int) (. source changed))
  (let second (of Broadcastable Int) (. source changed))
  (print (== (as first Pointer) (as second Pointer)))
  (blank)
  (for (on value first)
    (print value)
  )
  (blank)
  (call (. source emit) 5)
)

(type + EscapedEventSource
  (event + changed Int)
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)
```

```output
true
5
```

### A handler can unsubscribe a different subscriber through an escaped adapter without detaching itself

```brix
(main
  (let source (call EscapedUnsubscribeSource))
  (let endpoint (of Broadcastable Int) (. source changed))
  (blank)
  (for (on value endpoint)
    (print (str "first " value))
    (if (== value 1)
      (call (. endpoint unsubscribe) on-escaped-other)
    )
  )
  (blank)
  (call (. endpoint subscribe) on-escaped-other)
  (blank)
  (call (. source emit) 1)
  (call (. source emit) 2)
)

(fn-do + on-escaped-other _
  (param item Int)
  (param broadcaster (of Broadcastable Int))
  (print (str "other " item))
)

(type + EscapedUnsubscribeSource
  (event + changed Int)
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)
```

```output
first 1
first 2
```

### A handler can unsubscribe itself through an escaped adapter without skipping later subscribers

```brix
(main
  (let source (call SelfUnsubscribeSource))
  (let endpoint (of Broadcastable Int) (. source changed))
  (blank)
  (call (. endpoint subscribe) on-escaped-self)
  (for (on value endpoint)
    (print (str "later " value))
  )
  (blank)
  (call (. source emit) 1)
  (call (. source emit) 2)
)

(fn-do + on-escaped-self _
  (param item Int)
  (param broadcaster (of Broadcastable Int))
  (print (str "self " item))
  (catch (call (. broadcaster unsubscribe) on-escaped-self))
)

(type + SelfUnsubscribeSource
  (event + changed Int)
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)
```

```output
self 1
later 1
later 2
```

### Inherited events cannot be redeclared

```brix crash
(main
  (call DerivedRedeclaration)
)

(type + BaseRedeclaration
  (event + changed Int)
)

(type + DerivedRedeclaration (is BaseRedeclaration)
  (event + changed Int)
)
```

```output
Exception: Inherited event "changed" cannot be redeclared
<no file:11>
      (event + changed Int)
      ^^^^^^^^^^^^^^^^^^^^^
```

### A event callback can detach a later subscriber before its turn

```brix
(main
  (let source (call DetachingEventSource))
  (let controller (call DetachingController source))
  (let later (call DetachingObserver source))
  (= (. controller target) later)
  (blank)
  (call (. source emit) 1)
  (call (. source emit) 2)
)

(type + DetachingEventSource
  (event + changed Int)
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)

(type + DetachingController
  (prop + source DetachingEventSource)
  (prop + target (? DetachingObserver))
  (fn-do + This _
    (param source _)
  )
  (subscribe-for (on value (. source changed))
    (print (str "controller " value))
    (call (?. target close))
  )
)

(type + DetachingObserver
  (prop + source DetachingEventSource)
  (fn-do + This _
    (param source _)
  )
  (subscribe-for (on value (. source changed))
    (print (str "later " value))
  )
  (fn-do + close _)
)
```

```output
controller 1
controller 2
```

### A subscriber behind a closed one is still delivered when the freed slot is reused during dispatch

```brix
(main
  (let source (call ReusingEventSource))
  (let controller (call ReusingController source))
  (let later (call ReusingObserver source))
  (let last (call LastReusingObserver source))
  (= (. controller target) later)
  (blank)
  (call (. source emit) 1)
  (call (. source emit) 2)
)

(fn-do + install-reusing-late _
  (param source ReusingEventSource)
  (for (on value (. source changed))
    (print (str "late " value))
  )
)

(type + ReusingEventSource
  (event + changed Int)
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)

(type + ReusingController
  (prop + source ReusingEventSource)
  (prop + target (? ReusingObserver))
  (fn-do + This _
    (param source _)
  )
  (subscribe-for (on value (. source changed))
    (print (str "controller " value))
    (if target
      (call (. target close))
      (= target null)
      (call install-reusing-late source)
    )
  )
)

(type + ReusingObserver
  (prop + source ReusingEventSource)
  (fn-do + This _
    (param source _)
  )
  (subscribe-for (on value (. source changed))
    (print (str "later " value))
  )
  (fn-do + close _)
)

(type + LastReusingObserver
  (prop + source ReusingEventSource)
  (fn-do + This _
    (param source _)
  )
  (subscribe-for (on value (. source changed))
    (print (str "last " value))
  )
  (fn-do + close _)
)
```

```output
controller 1
last 1
controller 2
last 2
late 2
```

### Lowered self subscriptions run before later external subscribers in lexical order

```brix
(main
  (let owner (call OrderedLoweredOwner))
  (blank)
  (for (on value (. owner changed))
    (print (str "external " value))
  )
  (blank)
  (call (. owner emit) 7)
)

(type + OrderedLoweredOwner
  (event + changed Int)
  (subscribe-for (on value changed)
    (print (str "self-1 " value))
  )
  (subscribe-for (on value changed)
    (print (str "self-2 " value))
  )
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)
```

```output
self-1 7
self-2 7
external 7
```

### Dense runtime event ids isolate lowered descriptor buckets

```brix
(main
  (let owner (call BucketedLoweredOwner))
  (call (. owner emit))
)

(type + BucketedLoweredOwner
  (event + first Int)
  (event + second String)
  (subscribe-for (on value first)
    (print (str "first " value))
  )
  (subscribe-for (on value second)
    (print (str "second " value))
  )
  (fn-do + emit _
    (<- first 7)
    (<- second "eight")
  )
)
```

```output
first 7
second eight
```

### A lowered callback can remove an external subscriber before sidecar dispatch

```brix
(main
  (let owner (call RemovingLoweredOwner))
  (let later (call RemovingExternalObserver owner))
  (= (. owner later) later)
  (blank)
  (call (. owner emit) 1)
  (call (. owner emit) 2)
)

(type + RemovingLoweredOwner
  (event + changed Int)
  (prop + later (? RemovingExternalObserver))
  (subscribe-for (on value changed)
    (print (str "owner-first " value))
    (call (?. later close))
  )
  (subscribe-for (on value changed)
    (print (str "owner-second " value))
  )
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)

(type + RemovingExternalObserver
  (prop + source RemovingLoweredOwner)
  (fn-do + This _
    (param source _)
  )
  (subscribe-for (on value (. source changed))
    (print (str "external-later " value))
  )
  (fn-do + close _)
)
```

```output
owner-first 1
owner-second 1
owner-first 2
owner-second 2
```

### A breaking self subscription stays in the sidecar and unsubscribes after its first turn

```brix
(main
  (let owner (call MixedBreakingOwner))
  (call (. owner emit) 1)
  (call (. owner emit) 2)
)

(type + MixedBreakingOwner
  (event + changed Int)
  (subscribe-for (on value changed)
    (print (str "lowered " value))
  )
  (subscribe-for (on value changed)
    (print (str "breaking " value))
    (break)
  )
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)
```

```output
lowered 1
breaking 1
lowered 2
```

### A self subscription after a breaking one stays in the sidecar so lexical order is kept

```brix
(main
  (let owner (call PriorSidecarOrderOwner))
  (for (on value (. owner changed))
    (print (str "external " value))
  )
  (blank)
  (call (. owner emit) 1)
  (call (. owner emit) 2)
)

(type + PriorSidecarOrderOwner
  (event + changed Int)
  (subscribe-for (on value changed)
    (print (str "breaking " value))
    (break)
  )
  (subscribe-for (on value changed)
    (print (str "second " value))
  )
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)
```

```output
breaking 1
second 1
external 1
second 2
external 2
```

### Closing an owner in a lowered callback suppresses later descriptors and future publication

```brix
(main
  (let owner (call ClosingLoweredOwner))
  (call (. owner emit) 1)
  (call (. owner emit) 2)
)

(type + ClosingLoweredOwner
  (event + changed Int)
  (subscribe-for (on value changed)
    (print (str "closing " value))
    (call close)
  )
  (subscribe-for (on value changed)
    (print (str "too-late " value))
  )
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
  (fn-do + close _)
)
```

```output
closing 1
```

### Generic classes resolve event payload types in their class scope

```brix
(main
  (let source (call (of GenericEventSource String)))
  (blank)
  (for (on value (. source changed))
    (print value)
  )
  (blank)
  (call (. source emit) "generic")
)

(type + (of GenericEventSource Payload)
  (event + changed Payload)
  (fn-do + emit _
    (param value Payload)
    (<- changed value)
  )
)
```

```output
generic
```

### Public events can be observed across module boundaries

```brix
(import ./lib/event-source)
(blank)

(main
  (let source (call LibraryEventSource))
  (for (on value (. source changed))
    (print value)
  )
  (call (. source emit) 12)
)
```

```brix file=lib/event-source.bx
(type + LibraryEventSource
  (event + changed Int)
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)
```

```output
12
```

### Consumers can publish public events

```brix
(import ./lib/event-source)
(blank)

(main
  (let source (call LibraryEventSource))
  (for (on value (. source changed))
    (print value)
  )
  (<- (. source changed) 12)
)
```

```brix file=lib/event-source.bx
(type + LibraryEventSource
  (event + changed Int)
)
```

```output
12
```

### Private events reject observation from another module

```brix crash
(import ./lib/private-event-source)
(blank)

(main
  (let source (call PrivateEventSource))
  (for (on value (. source hidden))
    (print value)
  )
)
```

```brix file=lib/private-event-source.bx
(type + PrivateEventSource
  (event _ hidden Int)
)
```

```output
Exception: Event "hidden" is private
<lib/private-event-source.bx:2>
      (event _ hidden Int)
      ^^^^^^^^^^^^^^^^^^^^
```

### A shared event is heard in its module's file family

```brix
(import ./lib/shared-source)
(blank)

(main
  (let source (call SharedSource))
  (call (. source listen))
  (call (. source emit) 5)
)
```

```brix file=lib/shared-source.bx
(import ./shared-source.watch)

(type + SharedSource
  (event ^ pinged Int)
  (fn-do + emit _
    (param value Int)
    (<- pinged value)
  )
  (fn-do + listen _
    (call watch this)
  )
)
```

```brix file=lib/shared-source.watch.bx
(import ./shared-source)

(fn-do + watch _
  (param source SharedSource)
  (for (on value (. source pinged))
    (print (str "heard " value))
  )
)
```

```output
heard 5
```

### Shared events reject observation from outside their module's file family

```brix crash
(import ./lib/shared-event-source)
(blank)

(main
  (let source (call SharedEventSource))
  (for (on value (. source hidden))
    (print value)
  )
)
```

```brix file=lib/shared-event-source.bx
(type + SharedEventSource
  (event ^ hidden Int)
)
```

```output
Exception: Event "hidden" is shared, declared with ^, so only its own file family can use it
<lib/shared-event-source.bx:2>
      (event ^ hidden Int)
      ^^^^^^^^^^^^^^^^^^^^
```

### Event names cannot collide with stored properties

```brix crash
(main
  (call CollisionSource)
)

(type + CollisionSource
  (prop + changed _ (= 1))
  (event + changed Int)
)
```

```output
Exception: Redefinition of class member
<no file:8>
      (event + changed Int)
      ^^^^^^^^^^^^^^^^^^^^^
```

### Event names cannot collide with methods

```brix crash
(main
  (call CollisionMethodSource)
)

(type + CollisionMethodSource
  (event + changed Int)
  (fn-do + changed _
    (param value Int)
  )
)
```

```output
Exception: Redefinition of class member
<no file:8>
      (fn-do + changed _
      ^^^^^^^^^^^^^^^^^
```

### Routed events walk providers just in time and keep cancellation on the event

```brix
(import /core/intrinsics)
(blank)

(main
  (let root (call RoutedEventNode "root"))
  (let alternate (call RoutedEventNode "alternate"))
  (let mid (call RoutedEventNode "mid" root))
  (let leaf (call RoutedEventNode "leaf" mid))
  (blank)
  (for (on event (. root changed))
    (print (str "root " (. event value) " " (. event current-name)))
  )
  (blank)
  (for (on event (. alternate changed))
    (print (str "alternate " (. event value) " " (. event current-name)))
  )
  (blank)
  (for (on event (. mid changed))
    (print (str "mid-first " (. event value) " " (. event current-name)))
    (if (== (. event value) 3)
      (call (. event cancel))
      (else-if (== (. event value) 4)
        (= (. mid parent) alternate)
      )
    )
  )
  (blank)
  (for (on event (. mid changed))
    (print (str "mid-second " (. event value)))
  )
  (blank)
  (for (on event (. mid changed) (when arrival))
    (print (str "mid-arrival " (. event value)))
  )
  (blank)
  (for (on event (. leaf changed) (when arrival))
    (print (str "leaf-arrival " (. event value)))
  )
  (blank)
  (for (on event (. leaf changed))
    (print (str "leaf " (. event value) " " (. event current-name)))
    (if (== (. event value) 1)
      (call (. leaf route) 2)
      (print (str "leaf-restored " (. event current-name) " " (. event is-canceled)))
      (else-if (== (. event value) 2)
        (call (. event cancel))
      )
    )
  )
  (blank)
  (print (str "route-one-canceled " (call (. leaf route) 1)))
  (print (str "route-three-canceled " (call (. leaf route) 3)))
  (print (str "route-four-canceled " (call (. leaf route) 4)))
)

(type + TestRoutedEvent
  (implements RoutedEvent)
  (prop + value Int)
  (prop + current-name String)
  (prop + is-canceled _ (= false))
  (fn-do + This _
    (param value _)
    (param current-name _)
  )
  (fn-do + cancel _
    (= is-canceled true)
  )
  (fn-do + next-event-owner (? Pointer)
    (param raw-current Pointer)
    (let current (call (of gc-object-from-pointer RoutedEventNode) raw-current))
    (let next (. current parent))
    (if next
      (= current-name (. next name))
      (return (as next Pointer))
    )
  )
)

(type + RoutedEventNode
  (prop + name String)
  (prop + parent (? RoutedEventNode))
  (event + changed TestRoutedEvent)
  (fn-do + This _
    (param name _)
    (param parent _ (= null))
  )
  (fn-do + route Bool
    (param value Int)
    (let event (call TestRoutedEvent value name))
    (<- changed event)
    (return (. event is-canceled))
  )
)
```

```output
leaf-arrival 1
leaf 1 leaf
leaf-arrival 2
leaf 2 leaf
leaf-restored leaf false
mid-first 1 mid
mid-second 1
root 1 root
route-one-canceled false
leaf-arrival 3
leaf 3 leaf
mid-first 3 mid
route-three-canceled true
leaf-arrival 4
leaf 4 leaf
mid-first 4 mid
mid-second 4
alternate 4 alternate
route-four-canceled false
```

### Routed dispatch continues through target subscribers and bubble hops when a freed slot is reused

```brix
(import /core/intrinsics)
(blank)

(main
  (let root (call ReusingRoutedNode "root"))
  (let leaf (call ReusingRoutedNode "leaf" root))
  (let controller (call ReusingRoutedController leaf))
  (let later (call ReusingRoutedObserver leaf))
  (= (. controller target) later)
  (blank)
  (for (on event (. leaf changed))
    (print (str "last " (. event value) " " (. event current-name)))
  )
  (blank)
  (for (on event (. root changed))
    (print (str "root " (. event value) " " (. event current-name)))
  )
  (blank)
  (call (. leaf route) 1)
  (call (. leaf route) 2)
)

(fn-do + install-reusing-routed-late _
  (param source ReusingRoutedNode)
  (for (on event (. source changed))
    (print (str "late " (. event value) " " (. event current-name)))
  )
)

(type + ReusingRoutedEvent
  (implements RoutedEvent)
  (prop + value Int)
  (prop + current-name String)
  (prop + is-canceled _ (= false))
  (fn-do + This _
    (param value _)
    (param current-name _)
  )
  (fn-do + next-event-owner (? Pointer)
    (param raw-current Pointer)
    (let current (call (of gc-object-from-pointer ReusingRoutedNode) raw-current))
    (let next (. current parent))
    (if next
      (= current-name (. next name))
      (return (as next Pointer))
    )
  )
)

(type + ReusingRoutedNode
  (prop + name String)
  (prop + parent (? ReusingRoutedNode))
  (event + changed ReusingRoutedEvent)
  (fn-do + This _
    (param name _)
    (param parent _ (= null))
  )
  (fn-do + route _
    (param value Int)
    (<- changed (call ReusingRoutedEvent value name))
  )
)

(type + ReusingRoutedController
  (prop + source ReusingRoutedNode)
  (prop + target (? ReusingRoutedObserver))
  (fn-do + This _
    (param source _)
  )
  (subscribe-for (on event (. source changed))
    (print (str "controller " (. event value) " " (. event current-name)))
    (if target
      (call (. target close))
      (= target null)
      (call install-reusing-routed-late source)
    )
  )
)

(type + ReusingRoutedObserver
  (prop + source ReusingRoutedNode)
  (fn-do + This _
    (param source _)
  )
  (subscribe-for (on event (. source changed))
    (print (str "later " (. event value) " " (. event current-name)))
  )
  (fn-do + close _)
)
```

```output
controller 1 leaf
last 1 leaf
root 1 root
controller 2 leaf
last 2 leaf
late 2 leaf
root 2 root
```

### Erased routed publication removes its event constructor

```brix
(main
  (let node (call ErasedRoutedEventNode))
  (blank)
  (print (str "erased-canceled " (call (. node route))))
)

(type + ErasedRoutedEvent
  (implements RoutedEvent)
  (prop + is-canceled _ (= false))
  (fn-do + next-event-owner (? Pointer)
    (param current Pointer)
  )
)

(type + ErasedRoutedEventNode
  (event + changed ErasedRoutedEvent)
  (fn-do + route Bool
    (<- changed (call make-erased-routed-event))
    (return false)
  )
)

(fn-do + make-erased-routed-event ErasedRoutedEvent
  (print "built erased routed payload")
  (return (call ErasedRoutedEvent))
)
```

```output
erased-canceled false
```

### Legacy publish-routed calls are rejected after routed writes become type-directed

```brix crash
(main
  (let node (call RoutedArityEventNode))
  (call publish-routed (. node changed) null (call RoutedArityEvent))
)

(type + RoutedArityEvent
  (implements RoutedEvent)
  (prop + is-canceled _ (= false))
  (fn-do + next-event-owner (? Pointer)
    (param current Pointer)
  )
)

(type + RoutedArityEventNode
  (event + changed RoutedArityEvent)
)
```

```output
Exception: Function "publish-routed" not found
<no file:4>
      (call publish-routed (. node changed) null (call RoutedArityEvent))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Lowered self descriptors receive routed event objects

```brix
(main
  (let source (call RoutedDescriptorSource))
  (call (. source route))
)

(type + RoutedDescriptorEvent
  (implements RoutedEvent)
  (prop + value Int)
  (prop + is-canceled _ (= false))
  (fn-do + This _
    (param value _)
  )
  (fn-do + next-event-owner (? Pointer)
    (param current Pointer)
  )
)

(type + RoutedDescriptorSource
  (event + changed RoutedDescriptorEvent)
  (subscribe-for (on event changed)
    (print (str "descriptor " (. event value)))
  )
  (fn-do + route Bool
    (<- changed (call RoutedDescriptorEvent 7))
    (return false)
  )
)
```

```output
descriptor 7
```

### A class-body self subscription that reads a markup id stays a sidecar

```brix
(main
  (let host (call MarkupIdDescriptorHost))
  (call (. host emit) 3)
  (print (str "count " (. host count)))
)

(type + MarkupIdDescriptorNode
  (prop + value _ (= 10))
)

(type + MarkupIdDescriptorBox
  (prop + child _ (:= (call MarkupIdDescriptorNode)))
  (fn-do + (infix this += (param child MarkupIdDescriptorNode)) _
    (= (. this child) child)
    (return child)
  )
)

(type + MarkupIdDescriptorHost (is MarkupIdDescriptorBox)
  (event + changed Int)
  (prop + count _ (= 0))
  (subscribe-for (on n changed)
    (+= count (+ n (. node value)))
  )
  (content
    (<> MarkupIdDescriptorNode (id: node))
  )
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)
```

```output
count 13
```

### Keyed events address one receiver with ordered subscriptions and lazy misses

```brix
(main
  (let mailbox (call KeyedMailbox))
  (let receiver (call KeyedReceiver mailbox 7))
  (let duplicate (catch (call KeyedReceiver mailbox 7)))
  (blank)
  (print duplicate)
  (<- ([] (. mailbox messages) 7) 3)
  (<- ([] (. mailbox messages) 8) (call keyed-missed-payload))
  (let moving (call KeyedReceiver mailbox 11))
  (let occupied (call KeyedReceiver mailbox 12))
  (let collision (catch (call (. moving change-key) 12)))
  (print collision)
  (<- ([] (. mailbox messages) 11) 6)
  (call (. moving change-key) 13)
  (<- ([] (. mailbox messages) 11) (call keyed-missed-payload))
  (<- ([] (. mailbox messages) 13) 7)
  (<- ([] (. mailbox messages) 7) (call replace-keyed-receiver mailbox receiver))
  (<- ([] (. mailbox messages) 7) 5)
  (let releasing (call KeyedReceiver mailbox 17))
  (<- ([] (. mailbox messages) 17) 8)
  (call (. releasing release-address))
  (<- ([] (. mailbox messages) 17) (call keyed-missed-payload))
  (let reclaimed (call KeyedReceiver mailbox 17))
  (<- ([] (. mailbox messages) 17) 9)
  (for (in generation (..< 0 300))
    (let churn (call KeyedReceiver mailbox 18))
    (call (. churn release-address))
  )
  (let after-churn (call KeyedReceiver mailbox 18))
  (<- ([] (. mailbox messages) 18) 10)
)

(type + KeyedMailbox
  (event + messages Int U64)
)

(type + KeyedReceiver
  (prop + mailbox KeyedMailbox)
  (prop + id U64)
  (fn-do + This _
    (param mailbox _)
    (param id _)
  )
  (subscribe-for (on value ([] (. mailbox messages) id))
    (print (str "first " value))
  )
  (subscribe-for (on value ([] (. mailbox messages) id))
    (print (str "second " value))
  )
  (fn-do + close _)
  (fn-do + change-key _ (?! Error)
    (param next U64)
    (call (. ([] (. mailbox messages) id) rekey) next)
    (= id next)
  )
  (fn-do + release-address _
    (call (. ([] (. mailbox messages) id) release))
  )
)

(fn-do + keyed-missed-payload Int
  (print "built")
  (return 9)
)

(fn-do + replace-keyed-receiver Int
  (param mailbox KeyedMailbox)
  (param previous KeyedReceiver)
  (call (. previous close))
  (do
    (inline (call KeyedReceiver mailbox 7))
  )
  (print "built replacement")
  (return 4)
)
```

```output
Duplicate keyed event receiver for 7:0
first 3
second 3
Duplicate keyed event receiver for 12:0
first 6
second 6
first 7
second 7
built replacement
first 5
second 5
first 8
second 8
first 9
second 9
first 10
second 10
```

### Keyed endpoint adapters require a stable address and close with its receiver lifetime

```brix
(main
  (let mailbox (call KeyedAdapterMailbox))
  (let receiver (call KeyedAdapterReceiver mailbox 4))
  (let endpoint (call (. receiver endpoint)))
  (let second (call (. receiver endpoint)))
  (print (== (as endpoint Pointer) (as second Pointer)))
  (blank)
  (for (on value endpoint)
    (print (str "adapter " value))
  )
  (blank)
  (<- ([] (. mailbox messages) 4) 1)
  (call (. receiver close))
  (<- ([] (. mailbox messages) 4) (call keyed-adapter-missed))
)

(type + KeyedAdapterMailbox
  (event + messages Int U64)
)

(type + KeyedAdapterReceiver
  (prop + mailbox KeyedAdapterMailbox)
  (prop + id U64)
  (fn-do + This _
    (param mailbox _)
    (param id _)
  )
  (fn-do + endpoint (of Broadcastable Int)
    (return ([] (. mailbox messages) id))
  )
  (fn-do + close _)
)

(fn-do + keyed-adapter-missed Int
  (print "adapter built")
  (return 2)
)
```

```output
true
adapter 1
```

### Keyed address rekey and release compile without a publisher

```brix
(main
  (let mailbox (call NoPublishKeyedMailbox))
  (let receiver (call NoPublishKeyedReceiver mailbox 1))
  (call (. receiver change-key) 2)
  (call (. receiver release-address))
  (print "done")
)

(type + NoPublishKeyedMailbox
  (event + messages Int U64)
)

(type + NoPublishKeyedReceiver
  (prop + mailbox NoPublishKeyedMailbox)
  (prop + id U64)
  (fn-do + This _
    (param mailbox _)
    (param id _)
  )
  (subscribe-for (on value ([] (. mailbox messages) id))
    (print (str "got " value))
  )
  (fn-do + change-key _ (?! Error)
    (param next U64)
    (call (. ([] (. mailbox messages) id) rekey) next)
    (= id next)
  )
  (fn-do + release-address _
    (call (. ([] (. mailbox messages) id) release))
  )
)
```

```output
done
```

### A keyed endpoint stays claimed and can resubscribe after its last subscriber breaks

```brix
(main
  (let mailbox (call PersistentKeyedAdapterMailbox))
  (let receiver (call PersistentKeyedAdapterReceiver mailbox 4))
  (let endpoint (call (. receiver endpoint)))
  (blank)
  (for (on value endpoint)
    (print (str "first " value))
    (break)
  )
  (blank)
  (<- ([] (. mailbox messages) 4) 1)
  (<- ([] (. mailbox messages) 4) (call persistent-keyed-adapter-missed))
  (print "resubscribing")
  (blank)
  (for (on value endpoint)
    (print (str "second " value))
  )
  (blank)
  (<- ([] (. mailbox messages) 4) 3)
  (call (. receiver close))
  (blank)
  (let replacement (call PersistentKeyedAdapterReceiver mailbox 4))
  (let replacement-endpoint (call (. replacement endpoint)))
  (for (on value replacement-endpoint)
    (print (str "replacement " value))
  )
  (blank)
  (<- ([] (. mailbox messages) 4) 4)
)

(type + PersistentKeyedAdapterMailbox
  (event + messages Int U64)
)

(type + PersistentKeyedAdapterReceiver
  (prop + mailbox PersistentKeyedAdapterMailbox)
  (prop + id U64)
  (fn-do + This _
    (param mailbox _)
    (param id _)
  )
  (fn-do + endpoint (of Broadcastable Int)
    (return ([] (. mailbox messages) id))
  )
  (fn-do + close _)
)

(fn-do + persistent-keyed-adapter-missed Int
  (print "built missed payload")
  (return 2)
)
```

```output
first 1
resubscribing
second 3
replacement 4
```

### A keyed endpoint stays open after its last function subscriber unsubscribes

```brix
(main
  (let mailbox (call FunctionKeyedAdapterMailbox))
  (let receiver (call FunctionKeyedAdapterReceiver mailbox 5))
  (let endpoint (of Broadcastable Int) (call (. receiver endpoint)))
  (call (. endpoint subscribe) on-persistent-keyed-first)
  (blank)
  (<- ([] (. mailbox messages) 5) 1)
  (print "resubscribing function")
  (call (. endpoint subscribe) on-persistent-keyed-second)
  (<- ([] (. mailbox messages) 5) 2)
)

(fn-do + on-persistent-keyed-first _
  (param item Int)
  (param broadcaster (of Broadcastable Int))
  (print (str "first function " item))
  (catch (call (. broadcaster unsubscribe) on-persistent-keyed-first))
)

(fn-do + on-persistent-keyed-second _
  (param item Int)
  (param broadcaster (of Broadcastable Int))
  (print (str "second function " item))
)

(type + FunctionKeyedAdapterMailbox
  (event + messages Int U64)
)

(type + FunctionKeyedAdapterReceiver
  (prop + mailbox FunctionKeyedAdapterMailbox)
  (prop + id U64)
  (fn-do + This _
    (param mailbox _)
    (param id _)
  )
  (fn-do + endpoint (of Broadcastable Int)
    (return ([] (. mailbox messages) id))
  )
  (fn-do + close _)
)
```

```output
first function 1
resubscribing function
second function 2
```

### A keyed endpoint backed by a temporary key expression cannot escape

```brix crash
(main
  (let mailbox (call TemporaryKeyMailbox))
  (let adapter (of Broadcastable Int) ([] (. mailbox messages) (call temporary-key)))
)

(type + TemporaryKeyMailbox
  (event + messages Int U64)
)

(fn-do + temporary-key U64
  (return 1)
)
```

```output
Exception: A temporary keyed event address cannot escape; declare a stable address key and use that stable addressed lifetime
<no file:4>
      (let adapter (of Broadcastable Int) ([] (. mailbox messages) (call temporary-key)))
                   ^^^^^^^^^^^^^^^^^^^^^^
```

### An unaddressed class-body subscription to a keyed event is rejected

```brix crash
(main
  (let mailbox (call UnaddressedKeyedMailbox))
  (<- ([] (. mailbox messages) 1) 5)
  (print "done")
)

(type + UnaddressedKeyedMailbox
  (event + messages Int U64)
  (subscribe-for (on value messages)
    (print (str "got " value))
  )
)
```

```output
Exception: Keyed event "messages" must be addressed with [key]
<no file:10>
      (subscribe-for (on value messages)
                     ^^^^^^^^^^^^^^^^^^^
```

### Floating-point keyed event declarations are rejected

```brix crash
(main
  (do
    (inline (call FloatKeyMailbox))
  )
)

(type + FloatKeyMailbox
  (event + messages Int F64)
)
```

```output
Exception: Keyed event keys must conform to EventKey: integer, Char, Bool, interned identity, or an integer (index, generation) handle; Float and user-defined equality/hash keys are not supported
<no file:9>
      (event + messages Int F64)
      ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### User-defined keyed event equality is rejected

```brix crash
(main
  (do
    (inline (call CustomKeyMailbox))
  )
)

(type + CustomEventKey
  (prop + identity U64)
  (fn-expr + (infix this == (param other CustomEventKey)) _
    (== identity (. other identity))
  )
)

(type + CustomKeyMailbox
  (event + messages Int CustomEventKey)
)
```

```output
Exception: Keyed event keys must conform to EventKey: integer, Char, Bool, interned identity, or an integer (index, generation) handle; Float and user-defined equality/hash keys are not supported
<no file:16>
      (event + messages Int CustomEventKey)
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An error a subscriber raises goes to unhandled-error, and later items are still delivered.

```brix
(prop + numbers (of AsyncChannel Int))
(blank)

(main
  (= unhandled-error
    (=> (error) :do
      (block
        (print (str "handled " (. error message)))
      )
    )
  )
  (blank)
  (for (on n numbers)
    (if (== n 1)
      (throw (call IOError "one is bad"))
    )
    (print (str "got " n))
  )
  (blank)
  (for (in x (.. 0 2))
    (<- numbers x)
  )
  (print "done")
)
```

```output
done
got 0
handled one is bad
got 2
```

### An error a lowered handler raises goes to unhandled-error, and the handlers after it still run.

```brix
(main
  (let seen (call (of List String)))
  (= unhandled-error
    (=> (error) :do
      (block
        (+= seen (. error message))
      )
    )
  )
  (let owner (call FailingLoweredOwner))
  (blank)
  (for (on value (. owner changed))
    (print (str "external " value))
  )
  (blank)
  (call (. owner emit) 7)
  (print (. seen length))
  (print ([] seen 0))
)

(type + FailingLoweredOwner
  (event + changed Int)
  (subscribe-for (on value changed)
    (throw (call IOError (str "self-1 failed " value)))
  )
  (subscribe-for (on value changed)
    (print (str "self-2 " value))
  )
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)
```

```output
self-2 7
external 7
1
self-1 failed 7
```

```brix
(main
  (let source (call ChainedSource))
  (for (on value (. source first))
    (<- (. source second) value)
  )
  (for (on value (. source second))
    (print value)
  )
  (<- (. source first) 1)
)

(type + ChainedSource
  (event + first Int)
  (event + second Int)
)
```

```output
1
```

```brix
(main
  (call deliver 1)
  (print "after")
)

(fn-do + deliver _
  (param value Int)
  (let attempt
    (do-new
      (let owner (call SignalOwner))
      (call (. owner emit) value)
      (print "not reached after emit")
      (on-failure failure
        (print (str "recovered " (. failure message)))
      )
    )
  )
  (call (. attempt wait))
)

(type + SignalOwner
  (event + changed Int)
  (subscribe-for (on value changed)
    (throw (call Panic (str "callback " value)))
  )
  (fn-do + emit _
    (param value Int)
    (<- changed value)
    (print "not reached after publish")
  )
)
```

```output
recovered callback 1
after
```

### A `whenever` clause on the loop of a class section subscribes it while the clause holds

```brix
(main
  (let box (call Box))
  (<- (. box timer) 1)
  (call yield-once)
  (= (. box is-open) false)
  (<- (. box timer) 2)
  (call yield-once)
  (= (. box is-open) true)
  (<- (. box timer) 3)
  (call yield-once)
)

(type + Box
  (prop + is-open _ (= true))
  (prop + timer (of AsyncChannel Int))
  (subscribe-for (on t timer (whenever #is-open))
    (print (str "box " t))
  )
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```output
box 1
box 3
```

### A panic while a `whenever` clause is tested again is raised from the loop

```brix crash
(main
  (lazy index 0)
  (let timer (of AsyncChannel Int))
  (blank)
  (for (on t timer (whenever (call allowed #index)))
    (print (str "got " t))
  )
  (blank)
  (<- timer 1)
  (call yield-once)
  (= index 5)
)

(fn-do + allowed Bool
  (param index Int)
  (throw (call Panic "Index too far") (if (> index 2)))
  (return true)
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```stderr
Unhandled panic: Index too far
<no file:17>

Task 1

Stack trace:
  0  __source__:allowed no file:17:3
  1  __source__:whenever no file:6:30
  2  __source__:wheneverCasting no file:6:8
  3  core/tracker:LiveAction:refresh-trackers-into core/tracker.bx
  4  core/tracker:LiveAction:refresh-trackers core/tracker.bx
  5  core/tracker:LiveAction:run-now core/tracker.bx
  6  core/tracker:LiveAction:run-queued core/tracker.bx
  7  core/tracker:LiveBatch:run core/tracker.bx
  8  core/tracker:LiveBatch:leave core/tracker.bx
  9  core/tracker:property-dispatch core/tracker.bx
  10  __source__:main no file:12:3
  11  __source__:init.__source__ no file
  12  __source__:brix.main no file
```

### A panic while a `whenever` clause of a class section is tested again is raised from the section

```brix crash
(main
  (let box (call Box))
  (<- (. box timer) 1)
  (call yield-once)
  (= (. box index) 5)
)

(type + Box
  (prop + index _ (= 0))
  (prop + timer (of AsyncChannel Int))
  (subscribe-for (on t timer (whenever (call allowed #index)))
    (print (str "box " t))
  )
)

(fn-do + allowed Bool
  (param index Int)
  (throw (call Panic "Index too far") (if (> index 2)))
  (return true)
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```stderr
Unhandled panic: Index too far
<no file:19>

Task 1

Stack trace:
  0  __source__:allowed no file:19:3
  1  __source__:whenever no file:12:40
  2  __source__:wheneverCasting no file:12:3
  3  core/tracker:LiveAction:refresh-trackers-into core/tracker.bx
  4  core/tracker:LiveAction:refresh-trackers core/tracker.bx
  5  core/tracker:LiveAction:run-now core/tracker.bx
  6  core/tracker:LiveAction:run-queued core/tracker.bx
  7  core/tracker:LiveBatch:run core/tracker.bx
  8  core/tracker:LiveBatch:leave core/tracker.bx
  9  core/tracker:property-dispatch core/tracker.bx
  10  __source__:main no file:6:3
  11  __source__:init.__source__ no file
  12  __source__:brix.main no file
```

### A subscription made inside a task inside a method is owned by the method's receiver

```brix
(main
  (let source (call TaskSubscriptionSource))
  (let changes (of Broadcastable Int) (. source changed))
  (let listener (call TaskSubscriptionListener changes))
  (call (. listener start))
  (<- (. listener ready))
  (call (. source emit) 8)
  (call (. source emit) 5)
)

(type + TaskSubscriptionSource
  (event + changed Int)
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)

(type + TaskSubscriptionListener
  (prop + source (of Broadcastable Int))
  (prop + ready (of SyncChannel Bool))
  (prop _ total _ (= 0))
  (fn-do + This _
    (param source _)
  )
  (fn-do + start _
    (do-new
      (for (on value source)
        (call note value)
      )
      (<- ready true)
    )
  )
  (fn-do _ note _
    (param value Int)
    (+= total value)
    (print (str "noted " total))
  )
)
```

```output
noted 8
noted 13
```

### A case written by its symbol alone is published to an event as a case of the event's payload

```brix
(main
  (let source (call Source))
  (blank)
  (for (on result (. source results))
    (print (is result .limited))
  )
  (blank)
  (<- (. source results) .limited)
)

(type + Source
  (event + results Outcome)
)

(type + Outcome (is Union)
  (variant passed)
  (variant limited)
)
```

```output
true
```

### An event may be keyed by a type named for a tuple of an integer index and generation, which addresses one receiver.

```brix
(main
  (let mailbox (call Mailbox))
  (let receiver (call Receiver mailbox (call Handle 3 1)))
  (<- ([] (. mailbox messages) (call Handle 3 1)) 5)
  (<- ([] (. mailbox messages) (call Handle 4 1)) 6)
  (<- ([] (. mailbox messages) (call Handle 3 2)) 7)
  (print (catch (call Receiver mailbox (call Handle 3 1))))
  (call (. receiver move) (call Handle 5 9))
  (<- ([] (. mailbox messages) (call Handle 3 1)) 8)
  (<- ([] (. mailbox messages) (call Handle 5 9)) 9)
)

(type + Handle (is (tuple (: index Index) (: generation U64))))

(type + Mailbox
  (event + messages Int Handle)
)

(type + Receiver
  (prop + mailbox Mailbox)
  (prop + id Handle)
  (fn-do + This _
    (param mailbox _)
    (param id _)
  )
  (subscribe-for (on value ([] (. mailbox messages) id))
    (print (str "got " value))
  )
  (fn-do + move _
    (param next Handle)
    (call (. ([] (. mailbox messages) id) rekey) next)
    (= id next)
  )
)
```

```output
got 5
Duplicate keyed event receiver for 3:1
got 9
```

### A comprehension on an event of the object maps each publication through match arms whose conditions read `it`.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let form (call Form))
  (print (. form shown text))
  (<- (. form submitted) 3)
  (do-new
    (do-new
      (print (. form shown text))
      (<- (. form submitted) -1)
      (do-new
        (do-new
          (print (. form shown text))
        )
      )
    )
  )
)

(type + Form (is Box)
  (event + submitted Int)
  (content
    (<> Label (id: shown) (text: (str "got:" (## checked))))
    (blank)
    (lazy checked
      (iterate (on submitted)
        (block
          (case? (< it 0)
            "negative"
          )
          (default
            (str "ok " it)
          )
        )
      )
    )
  )
)
```

```output
got:
got:ok 3
got:negative
```

### A lazy declaration whose value is the latest item of a stream, read tracked in markup, gives the markup each new item.

```brix
(import /core/tests/support/markup-fixtures)
(blank)
(prop + numbers (of AsyncChannel Int))
(blank)

(main
  (lazy last (## numbers))
  (blank)
  (<> Label (id: shown) (text: (str "got " #last)))
  (blank)
  (print (. shown text))
  (do-new
    (<- numbers 4)
    (do-new
      (do-new
        (print (. shown text))
      )
    )
  )
)
```

```output
got 0
got 4
```

### The latest item of a stream of union values is the first case of the union until the stream gives one.

```brix
(import /core/tests/support/markup-fixtures)
(blank)
(prop + picks (of AsyncChannel Pick))
(blank)

(main
  (<> Label (id: shown) (text: (str "got " (## picks))))
  (blank)
  (print (. shown text))
  (<- picks .second)
  (do-new
    (print (. shown text))
  )
)

(type + Pick (is Union)
  (variant first)
  (variant second)
  (variant third (tuple Int))
)
```

```output
got first
got second
```

### A lazy declaration whose value is the latest item of a stream compiles where nothing reads it.

```brix
(main
  (let timer (call (of AsyncChannel Int)))
  (lazy latest (## timer))
  (print "ran")
)
```

```output
ran
```

### A loop in a section of a class on an event of the object runs each time the event is sent.

```brix
(main
  (let thing (call Thing))
  (<- (. thing opened) 3)
  (print (. thing total))
)

(type + Thing
  (event + opened Int)
  (prop + items _ (= (list 1 2 3)))
  (prop + total _ (= 0))
  (subscribe-for (on n opened)
    (for (in item items)
      (+= total (* item n))
    )
  )
)
```

```output
18
```

### A `break` in a loop on a stream unsubscribes and leaves the body at once

```brix
(prop + numbers (of AsyncChannel Int))
(blank)

(main
  (for (on n numbers)
    (if (== n 1)
      (break)
    )
    (print (str "n=" n))
  )
  (blank)
  (<-* numbers (.. 0 3))
  ; numbers.close()
  (print "done")
)
```

```output
done
n=0
```

### A `break` or a `continue` in a loop in the body of a loop on a stream belongs to that loop

```brix
(prop + numbers (of AsyncChannel Int))
(blank)

(main
  (for (on n numbers)
    (for (in k (..< 0 4))
      (if (== k 1)
        (continue)
      )
      (if (== k 3)
        (break)
      )
      (print (str "n=" n " k=" k))
    )
    (print (str "after n=" n))
  )
  (blank)
  (<-* numbers (.. 0 1))
  ; numbers.close()
  (print "done")
)
```

```output
done
n=0 k=0
n=0 k=2
after n=0
n=1 k=0
n=1 k=2
after n=1
```

### The clauses of a loop on a stream are tested in the order written for each item

```brix
(prop + numbers (of AsyncChannel Int))
(blank)

(main
  (for (on n numbers (when (!= n 1)) (until (== n 3)))
    (print (str "on " n))
  )
  (blank)
  (<-* numbers (.. 0 4))
  ; numbers.close()
  (print "done")
)
```

```output
done
on 0
on 2
```

### An error the unsubscribe a `break` makes raises goes to unhandled-error, and the program goes on.

```brix
(import /core/stream)
(blank)

(main
  (= unhandled-error
    (=> (error) :do
      (block
        (print (str "handled " (. error message)))
      )
    )
  )
  (blank)
  (let source (call RefusingSource))
  (for (on message source)
    (print message)
    (break)
  )
  (call (. source send) "one")
  (print "done")
)

(type + RefusingSource
  (implements (of Broadcastable String))
  (prop + held (? (of Subscriber String)))
  (fn-expr + is-closed Bool :get
    false
  )
  (fn-do + subscribe _ (?! Error)
    (param subscriber (of Subscriber String))
    (= held subscriber)
  )
  (fn-do + unsubscribe _ (?! Error)
    (param subscriber (of Subscriber String))
    (throw (call Error "unsubscribe refused"))
  )
  (fn-do + subscribe _ (?! Error)
    (param subscriber (of Subscribable String))
  )
  (fn-do + unsubscribe _ (?! Error)
    (param subscriber (of Subscribable String))
  )
  (fn-do + send _ (?! Error)
    (param message String)
    (let call held)
    (if call
      (call call message this)
    )
  )
)
```

```output
one
handled unsubscribe refused
done
```

### A `for … on` source that reads a tracked value must be a stream: one that only iterates is an error that points at `whenever`.

```brix crash
(main
  (let foo (call Foo))
  (for (on v (. foo #t))
    (print v)
  )
)

(type + Foo
  (prop + t _ (= "ab"))
)
```

```output
Exception: #t is not a stream; use `whenever #t`
<no file:4>
      (for (on v (. foo #t))
      ^^^^^^^^^^^^^^^^^^^^^
```

### A class loop on a tracked number of an element a branch builds is not on a stream

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print "constructed")
  (= (. demo shows) true)
  (= (. demo pinger size) 4)
  (print (str "seen=" (. demo seen)))
)

(type + Pinger (is Box)
  (prop + size _ (= 0))
)

(type + Demo (is Box)
  (prop + shows _ (= false))
  (prop + seen _ (= 0))
  (content
    (<> This
      (if #shows
        (<> Pinger (id: pinger))
      )
    )
  )
  (subscribe-for (on value (. pinger #size))
    (= seen value)
  )
)
```

```output
Exception: #size is not a stream; use `whenever #size`
<no file:27>
      (subscribe-for (on value (. pinger #size))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A latest-value read gets an item an AsyncChannel accepted in the turn it closes

```brix
(main
  (let numbers (call (of AsyncChannel Int)))
  (lazy latest (## numbers))
  (let started latest)
  (<- numbers 12)
  (call (. numbers close))
  (call yield-once)
  (print latest)
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```output
12
```

### A class subscription gets the items an AsyncChannel accepted in the turn it closes

```brix
(main
  (let holder (call ClosingHolder))
  (<- (. holder numbers) 1)
  (<- (. holder numbers) 2)
  (call (. holder numbers close))
  (print "closed")
  (call yield-once)
  (print "end")
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)

(type + ClosingHolder
  (prop + numbers _ (= (call (of AsyncChannel Int))))
  (subscribe-for (on n numbers)
    (print (str "on " n))
  )
)
```

```output
closed
on 1
on 2
end
```

### Closing a subscription's owner after its AsyncChannel closes drops the items still queued for it

```brix
(main
  (let holder (call DroppingHolder))
  (<- (. holder numbers) 1)
  (call (. holder numbers close))
  (call (. holder close))
  (call yield-once)
  (print "end")
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)

(type + DroppingHolder
  (prop + numbers _ (= (call (of AsyncChannel Int))))
  (subscribe-for (on n numbers)
    (print (str "on " n))
  )
  (fn-do + close _)
)
```

```output
end
```

### A subscription that breaks while its closed AsyncChannel drains gets no more items

```brix
(main
  (for (on n numbers)
    (print n)
    (if (== n 1)
      (break)
    )
  )
  (blank)
  (<-* numbers (.. 0 3))
  (call (. numbers close))
  (print "done")
)

(blank)
(prop + numbers (of AsyncChannel Int))
```

```output
done
0
1
```

### Closing an AsyncChannel delivers to both a subscription and an iterator, then detaches both

```brix
(main
  (let numbers (call (of AsyncChannel Int)))
  (for (on n numbers)
    (print (str "on " n))
  )
  (do-new
    (for (in n numbers)
      (print (str "in " n))
    )
    (print "loop ended")
  )
  (call yield-once)
  (<- numbers 1)
  (<- numbers 2)
  (call (. numbers close))
  (call yield-once)
  (call yield-once)
  (print (if-expr (not numbers) "no subscribers" "subscribed"))
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```output
on 1
on 2
in 1
in 2
loop ended
no subscribers
```

### An AsyncBufferedChannel subscription gets the items accepted in the turn it closes, not later ones

```brix
(main
  (for (on n numbers)
    (print (str "on " n))
  )
  (<- numbers 1)
  (<- numbers 2)
  (call (. numbers close))
  (<- numbers 3)
  (call yield-once)
  (print "end")
)

(blank)
(prop + numbers (of AsyncBufferedChannel Int))

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```output
on 1
on 2
end
```

### An AsyncOnDemandChannel subscription gets the items accepted in the turn it closes

```brix
(main
  (let channel (call (of AsyncOnDemandChannel Int) (call OnDemandSource)))
  (for (on n channel)
    (print (str "on " n))
  )
  (<- channel 1)
  (<- channel 2)
  (call (. channel close))
  (<- channel 3)
  (call yield-once)
  (print "end")
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)

(type + OnDemandSource
  (implements (of Usable Void))
  (fn-do + begin-using _
    (print "begin using")
  )
  (fn-do + end-using _
    (print "end using")
  )
)
```

```output
begin using
end using
on 1
on 2
end
```

### An event assigned to a property of the event's Broadcastable is the event's endpoint, as a declaration of that type takes it.

```brix
(main
  (let first (call Source))
  (let second (call Source))
  (let listener (call Listener (. first changed)))
  (call (. listener swap) second)
  (call (. listener start))
  (<- (. listener ready))
  (<- (. first changed) 4)
  (<- (. second changed) 6)
  (<- (. second changed) 7)
)

(type + Source
  (event + changed Int)
)

(type + Listener
  (prop + source (of Broadcastable Int))
  (prop + ready (of SyncChannel Bool))
  (fn-do + This _
    (param source _)
  )
  (fn-do + swap _
    (param other Source)
    (= source (. other changed))
  )
  (fn-do + start _
    (do-new
      (for (on value source)
        (print (str "got " value))
      )
      (<- ready true)
    )
  )
)
```

```output
got 6
got 7
```

### Events published on an EventNode bubble to ancestors that name them by class; arrival and private events stay at the target

```brix
(main
  (let root (call Panel "root"))
  (let list (call Picker "list" root))
  (let inner (call Picker "inner" list))
  (call (. inner pick) "a")
  (call (. list pick) "b")
  (call (. root note) "c")
)

(type + Node
  (implements EventNode)
  (prop + name String)
  (prop + up (? Node))
  (event + noted String)
  (fn-do + This _
    (param name _)
    (param up _ (= null))
  )
  (fn-do + note _
    (param text String)
    (<- noted text)
  )
  (fn-expr + event-parent (? EventNode) :get
    up
  )
)

(type + Picker (is Node)
  (event + picked String)
  (event _ quiet String)
  (fn-do + pick _
    (param text String)
    (<- picked text)
    (<- quiet text)
  )
  (subscribe-for (on value picked)
    (print (str name " own " value))
  )
  (subscribe-for (on value picked (when arrival))
    (print (str name " arrival " value))
  )
  (subscribe-for (on value quiet)
    (print (str name " quiet " value))
  )
)

(type + Panel (is Node)
  (subscribe-for (on value (. Picker picked))
    (print (str name " heard " value))
  )
  (subscribe-for (on value noted)
    (print (str name " noted " value))
  )
)
```

```output
inner own a
inner arrival a
list own a
root heard a
inner quiet a
list own b
list arrival b
root heard b
list quiet b
root noted c
```

### An event an interface declares is one event for every class that is of it

```brix
(main
  (let root (call Holder "root"))
  (let a (call PickA root))
  (let b (call PickB a))
  (call (. b pick) "x")
  (call (. a pick) "y")
)

(type + Picking (is Interface)
  (event + chosen String)
)

(type + Node
  (implements EventNode)
  (prop + up (? Node))
  (fn-do + This _
    (param up _ (= null))
  )
  (fn-expr + event-parent (? EventNode) :get
    up
  )
)

(type + PickA (is Node)
  (implements Picking)
  (fn-do + pick _
    (param text String)
    (<- chosen text)
  )
)

(type + PickB (is Node)
  (implements Picking)
  (fn-do + pick _
    (param text String)
    (<- chosen text)
  )
)

(type + Holder (is Node)
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
  (subscribe-for (on text (. Picking chosen))
    (print (str name " chose " text))
  )
)
```

```output
root chose x
root chose y
```

### A private event cannot be named by its class

```brix compile-error
(main
  (let root (call Panel))
)

(type + Picker
  (event _ quiet String)
)

(type + Panel
  (subscribe-for (on value (. Picker quiet))
    (print value)
  )
)
```

```output
Exception: Event "quiet" is not public, so it does not bubble and cannot be named by its class
<no file:11>
      (subscribe-for (on value (. Picker quiet))
                               ^^^^^^^^^^^^^^^^
```

### A shared event does not bubble, so it cannot be named by its class either

```brix compile-error
(main
  (let root (call Panel))
)

(type + Picker
  (event ^ quiet String)
)

(type + Panel
  (subscribe-for (on value (. Picker quiet))
    (print value)
  )
)
```

```output
Exception: Event "quiet" is not public, so it does not bubble and cannot be named by its class
<no file:11>
      (subscribe-for (on value (. Picker quiet))
                               ^^^^^^^^^^^^^^^^
```

### A handler in a markup element's body hears an event the element's class has from an interface

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
  (call (. panel run))
)

(type + Editing (is Interface)
  (event + edited String)
)

(type + Field (is Box)
  (implements Editing)
  (fn-do + edit _
    (param text String)
    (<- edited text)
  )
)

(type + Panel (is Box)
  (content
    (<> This
      (<> Field (id: field)
        (for (on text edited)
          (print (str "field edited " text))
        )
      )
    )
  )
  (fn-do + run _
    (call (. field edit) "x")
  )
)
```

```output
field edited x
```

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let box (call Listener))
  (call markup-mount-root box null)
  (<- (. box timer) 1)
  (call yield-once)
  (= (. box is-open) false)
  (<- (. box timer) 2)
  (call yield-once)
  (= (. box is-open) true)
  (<- (. box timer) 3)
  (call yield-once)
  (call (. box close))
  (<- (. box timer) 4)
  (call yield-once)
)

(type + Listener (is Box)
  (prop + is-open _ (= true))
  (prop + timer (of AsyncChannel Int))
  (subscribe-for (on t timer (whenever #is-open))
    (print t)
  )
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```output
1
3
```

### An event declared by a struct is rejected, as a struct's values are copied and no object keeps its subscribers.

```brix crash
(main
  (let p (call P))
  (for (on v (. p moved))
    (print (str "moved " v))
  )
  (<- (. p moved) 3)
)

(type + P (is Struct)
  (prop + x Int)
  (event + moved Int)
)
```

```output
Exception: Only a class can declare an event, which belongs to an object, and P is a struct, whose values are copied; declare P as a class
<no file:12>
      (event + moved Int)
      ^^^^^^^^^^^^^^^^^^^
```

### A stream of a value that reads nothing tracked is rejected, as it would never change.

```brix crash
(main
  (let c (call Counter))
  (let s (stream (* (. c count) 2)))
)

(type + Counter
  (prop + count _ (= 1))
)
```

```output
Exception: A stream of a value follows the reads written with "#", and this one has none, so it would never change; write "#" before what it follows, as in "<c.#count>"
<no file:4>
      (let s (stream (* (. c count) 2)))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A stream of a value in a property's value is rejected, as only a statement in a function's body makes one.

```brix crash
(main
  (let w (call Watcher))
  (print (. w feed))
)

(type + Watcher
  (prop + feed _ (= 1))
  (prop + feeds _ (= (stream #feed)))
)
```

```output
Exception: A stream of a value is made only by a statement in a function's body, as in ". values = <c.#count>"; make it there, move a machine as a value changes with a "whenever" trigger, as in "then loading whenever #feed", or publish an event from a "whenever" rule and loop on it
<no file:9>
      (prop + feeds _ (= (stream #feed)))
                         ^^^^^^^^^^^^^^
```

### An event that carries Void is rejected, as nothing could be published to it.

```brix crash
(main
  (let d (call Door))
)

(type + Door
  (event + pinged Void)
  (prop + count _ (= 0))
)
```

```output
Exception: An event gives each subscriber a value, so it cannot carry Void; an event that only signals carries Bool, as in "+ event pinged Bool"
<no file:7>
      (event + pinged Void)
                      ^^^^
```

### An event read with "<-" is pointed to a loop that hears it, as an event keeps no value to read.

```brix crash
(main
  (let d (call Door))
  (let first (<- (. d rang)))
)

(type + Door
  (event + rang Int)
)
```

```output
Exception: "rang" is an event of Door, which gives each value to its subscribers and keeps none to read with "<-"; hear the next one with "for value on d.rang", and end the loop with "break"
<no file:4>
      (let first (<- (. d rang)))
                     ^^^^^^^^^^
```
