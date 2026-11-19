# Objects

```brix
(main
  (let thing (call Thing))
  (print (call foo thing))
)

(type + Proto (is Interface)
  (fn-expr + bar Int
    0
  )
)

(type + Thing
  (implements Proto)
  (fn-expr + bar _
    42
  )
)

(fn-expr + foo _
  (param p Proto)
  (call (. p bar))
)
```

```output
42
```

```brix
(main
  (let foo (call Foo))
  (let bar (call Bar))
  (print (if-expr true foo bar))
)

(type + Base)

(type + Foo (is Base))

(type + Bar (is Base))
```

```output
[Object __source__:Base]
```

### A class constructor may return early; field initializers still apply and later statements are skipped.

```brix
(main
  (print (. (call EarlyState (call (of List I32))) describe))
  (print (. (call EarlyState (list 3 4)) describe))
)

(type + EarlyState
  (prop + values (list I32) (= (call (of List I32))))
  (prop + total I32 (= 0))
  (prop + mixed _ (= false))
  (fn-do + This _
    (param source (list I32))
    (= values source)
    (return-if (if (not (. values length))))
    (= total ([] values 0))
    (for (in index (..< 1 (. values length)))
      (if (!= ([] values index) total)
        (= mixed true)
      )
      (+= total ([] values index))
    )
  )
  (fn-expr + describe _ :get
    (str (. values length) " " total " " mixed)
  )
)
```

```output
0 0 false
2 7 true
```

### A property that holds an object and has no value is made with the defaults its constructor declares

```brix
(main
  (let holder (call Holder))
  (print (. holder counter count))
)

(type + Counter
  (prop + count _ (= 1))
  (fn-do + This _
    (param start _ (= 40))
    (= count (+ start 2))
  )
)

(type + Holder
  (prop + counter Counter)
)
```

```output
42
```

### A Weak reads null once its target has been collected.

```brix
(import /core/gc)
(blank)
(import /BxGC/BxGC.h)
(blank)

(main
  (let weak (call weak-node 71))
  (call C:brixCollect)
  (print (== (. weak get) null))
)

(fn-do + weak-node (of Weak GCNode)
  (param value Int)
  (let target (call GCNode value))
  (return (call Weak target))
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
```

### A target read through a Weak during a mark survives that cycle, and the Weak clears in the next.

```brix
(import /core/gc)
(blank)
(import /BxGC/BxGCTest.h)
(blank)

(main
  (let weak (call weak-node 72))
  (print (call loaded-during-mark weak))
  (call C:brixCollect)
  (print (== (. weak get) null))
)

(fn-do + loaded-during-mark String
  (param weak (of Weak GCNode))
  (call C:brixGCStartConcurrentMark)
  (let loaded (. weak get))
  (call C:brixGCFinishConcurrentMark)
  (if (not loaded)
    (return "empty")
  )
  (call C:brixCollect)
  (if (not (call C:brixGCVerifyObject (as loaded Pointer)))
    (return "lost")
  )
  (return (str "alive " (. loaded value)))
)

(fn-do + weak-node (of Weak GCNode)
  (param value Int)
  (let target (call GCNode value))
  (return (call Weak target))
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
alive 72
true
```

### An object made from a range of slots taken just before the collection its taking starts, held only by the stack, survives that collection.

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (call C:brixCollect)
  (let prior-goal (call C:brixGCHeapGoal))
  (call C:brixGCSetHeapGoal (+ (call C:brixGCLiveByteCount) 1))
  (print (call lost-below 0 3000))
  (call C:brixGCSetHeapGoal prior-goal)
)

(fn-do + lost-below Int
  (param depth Int)
  (param limit Int)
  (let node (call Wide depth))
  (let lost (if-expr (< (+ depth 1) limit) (call lost-below (+ depth 1) limit) (call collected)))
  (if (and (call C:brixGCVerifyObject (as node Pointer)) (== (. node a) depth))
    (return lost)
  )
  (return (+ lost 1))
)

(fn-do + collected Int
  (call C:brixCollect)
  (return 0)
)

(type + Wide
  (prop + a Int)
  (prop + b Int (= 0))
  (prop + c Int (= 0))
  (prop + d Int (= 0))
  (prop + e Int (= 0))
  (prop + f Int (= 0))
  (prop + g Int (= 0))
  (prop + h Int (= 0))
  (fn-do + This _
    (param value Int)
    (= a value)
  )
)
```

```output
0
```

### A constructor that returns an object of its class on only some ways through its body makes one first, which it gives where the body ends.

```brix
(main
  (let high (call Holder (list 1 7)))
  (print (. high amount))
  (let low (call Holder (list 1 2)))
  (print (. low amount))
)

(type + Holder
  (prop + amount I64 (= (call noted)))
  (fn-do + This _
    (param items (list I64))
    (for (in item items)
      (if (> item 5)
        (return (call This item))
      )
    )
    (print "fell through")
  )
  (fn-do + This _
    (param amount I64)
    (= (. this amount) amount)
  )
)

(fn-do + noted I64
  (print "initializer")
  (return 1)
)
```

```output
initializer
7
initializer
fell through
1
```

### A constructor's initializer reads a field declared after the one it gives.

```brix
(import /core/arena)
(import /core/map)
(blank)

(main
  (let h (call Holder 3))
  (+= (. h items) 5)
  (= ([] (. h cache) 1) 2)
  (print (str "items " (. h items length) " cache " (?? ([] (. h cache) 1) 0) " n " (. h n)))
)

(type + Holder
  (prop + n Int)
  (prop + items (of List Int ArenaBacking))
  (prop + cache (of Map Int Int ArenaBacking))
  (prop _ arena Arena)
  (fn-do + This _
    (param n Int)
    (lazy items (call (of List Int) arena))
    (lazy cache (call (of Map Int Int) arena))
  )
)
```

```output
items 1 cache 2 n 3
```

### Field defaults and a constructor's initializers run in the order the fields are declared, before the constructor's body, and one that reads a field not yet given gives that field its value at the read.

```brix
(main
  (let e (call E))
  (print "--")
  (let f (call F))
  (print "--")
  (let g (call G 5))
)

(type + E
  (prop + a _ (= (call note "e.a")))
  (prop + b Int)
  (prop + c _ (= (call note "e.c")))
  (prop + d Int)
  (fn-do + This _
    (print "e.ctor")
    (lazy d (call note "e.d"))
    (lazy b (+ (call note "e.b") c))
  )
)

(type + F
  (prop + a _ (= (+ (call note "f.a") b)))
  (prop + b Int)
  (prop + c _ (= (call note "f.c")))
  (fn-do + This _
    (lazy b (call note "f.b"))
  )
)

(type + G
  (prop + a _ (= (call note "g.a")))
  (prop + b Int)
  (prop + c Int)
  (fn-do + This _
    (param c Int)
    (lazy b (+ (call note "g.b") c))
  )
)

(fn-do + note Int
  (param tag String)
  (print tag)
  (return 1)
)
```

```output
e.a
e.b
e.c
e.d
e.ctor
--
f.a
f.b
f.c
--
g.a
g.b
```

### A property of a class with no value written is made by the class's constructor with its defaults, which convert null to optionals of several types.

```brix
(main
  (let code (call Code null (list "a")))
  (let turn (call Turn code))
  (print (?? (?. (. turn owner tools) length) 0))
  (print (?? (. turn owner name) "none"))
)

(type + Code
  (prop + name (? String))
  (prop + tools (? (list String)))
  (prop + safe Bool)
  (fn-do + This _
    (param name (? String) (= null))
    (param tools (? (list String)) (= null))
    (param safe _ (= false))
    (print (str "made " (?? (?. tools length) 9)))
  )
)

(type + Turn
  (prop + owner Code)
  (fn-do + This _
    (param given Code)
    (= owner given)
  )
)
```

```output
made 1
made 9
1
none
```

### A tuple of another module with no constructor of its own, made where it is called: a field it constructs with no value written converts the default of the argument that construction leaves out.

```brix
(import ./lib/pool)
(blank)

(main
  (let box (call Box))
  (print (. box pool capacity))
)
```

```brix file=lib/pool.bx
(type + Pool (is Tuple)
  (prop + capacity Size)
  (fn-do + This _
    (param initial Size (= 0))
    (= capacity (+ initial 1))
  )
)

(type + Box (is Tuple)
  (prop + pool Pool)
)
```

```output
1
```

### A constructor stores an either-or whose first side holds

```brix
(main
  (print (. (call Mark "*") is-many))
)

(type + Mark
  (prop + is-many Bool)
  (fn-do + This _
    (param text _)
    (= is-many (or (== text "*") (== text "+")))
  )
)
```

```output
true
```
