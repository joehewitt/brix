# Globals

```brix
(prop + a _ (= 42))
(blank)

(main
  (print a)
  (= a 99)
  (call foo)
  (print a)
)

(fn-do + foo _
  (print a)
  (= a 88)
)
```

```output
42
99
88
```

### Constructor in global variable

```brix
(main
  (print foo)
)

(blank)
(prop + foo _ (= (call Foo)))

(type + Foo
  (fn-do + This _)
)
```

```output
[Object __source__:Foo]
```

### Constructor as default value of global

```brix
(main
  (print foo)
)

(blank)
(prop + foo Foo)

(type + Foo)
```

```output
[Object __source__:Foo]
```

### Cast in global assignment

```brix
(main
  (print foo)
)

(blank)
(prop + foo F64 (= (call Foo)))

(type + Foo
  (fn-to-expr + F64
    99.9f
  )
)
```

```output
99.9
```

### Tuple in global assignment

```brix
(main
  (print foo)
)

(blank)
(prop + foo Foo)

(type + Foo (is (tuple (: a Int) (: b Int))))
```

```output
(0, 0)
```

### Tuple with optional in global assignment

```brix
(main
  (print foo)
  (print bar)
)

(blank)
(prop + foo _ (= (call Foo 0 "hi")))
(prop + bar _ (= (call Foo 0 "hi" 9)))

(type + Foo (is (tuple (: a Int) (: b String) (: c (? I32)))))
```

```output
(0, hi, unknown)
(0, hi, 9)
```

### Optional in global assignment

```brix
(main
  (print foo)
  (print bar)
)

(blank)
(prop + foo (? I32))
(prop + bar (? I32) (= 1))
```

```output
unknown
1
```

### TypeInfo in global assignment

```brix
(main
  (print foo)
)

(blank)
(prop + foo _ (= (type-of 42)))
```

```output
[TypeInfo I64]
```

```brix
(main
  (print foo)
)

(blank)
(prop + foo _ (= ([] I32 3)))
```

```output
I32[3]
```

### Circular reference between global and argument default

```brix crash
(main
  (print foo)
  (print (call bar))
)

(blank)
(prop + foo _ (= (call bar)))

(fn-expr + bar _
  (param f _ (= foo))
  f
)
```

```output
Exception: "foo" depends on its own value, directly or through the values it reads; break the cycle
<no file:8>
    (prop + foo _ (= (call bar)))
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Assign to global variable before it is referenced

```brix
(main
  (= foo "boo")
  (print foo)
)

(blank)
(prop + foo _ (= "foobar"))
```

```output
boo
```

```brix
(prop + foo _ (:= (call goober)))
(blank)

(main
  (print "hi")
)

(fn-expr + goober _
  (before
    (print "goober")
  )
  42
)
```

```output
hi
```

```brix
(prop + foo _ (:= (call goober)))
(blank)

(main
  (print "hi")
  (print foo)
)

(fn-expr + goober _
  (before
    (print "goober")
  )
  42
)
```

```output
hi
goober
42
```

### Eager globals initialize in declaration order before main, including globals nothing reads.

```brix
(main
  (print "main starts")
  (print second)
  (print first)
)

(blank)
(prop + first _ (= (call noisy "first")))
(prop + second _ (= (call noisy "second")))
(prop + unused _ (= (call noisy "unused")))

(fn-do + noisy String
  (param name String)
  (print (str "initializing " name))
  (return name)
)
```

```output
initializing first
initializing second
initializing unused
main starts
second
first
```

### Lazy globals initialize on their first read and never when nothing reads them.

```brix
(main
  (print "main starts")
  (print second)
  (print first)
)

(blank)
(prop + first _ (:= (call noisy "first")))
(prop + second _ (:= (call noisy "second")))
(prop + unused _ (:= (call noisy "unused")))

(fn-do + noisy String
  (param name String)
  (print (str "initializing " name))
  (return name)
)
```

```output
main starts
initializing second
second
initializing first
first
```

### Writing a lazy global before its first read skips its initializer.

```brix
(main
  (= a 5)
  (print a)
)

(blank)
(prop + a _ (:= (call noisy 1)))

(fn-do + noisy Int
  (param n Int)
  (print "initializing")
  (return n)
)
```

```output
5
```

### An initializer that directly reads a later eager global is a compile error.

```brix crash
(main
  (print a)
)

(blank)
(prop + a _ (= (. b length)))
(prop + b _ (= (call noisy "b")))

(fn-do + noisy String
  (param name String)
  (return name)
)
```

```output
Exception: Global 'b' is read before it is initialized
<no file:7>
    (prop + a _ (= (. b length)))
                      ^
```

### A read that reaches an uninitialized eager global through a virtual call panics.

```brix crash
(main
  (print a)
)

(blank)
(prop + reader Reader (= (call BReader)))
(prop + a _ (= (call (. reader read))))
(prop + b _ (= (call noisy "b")))

(type + Reader
  (fn-expr + read String
    "base"
  )
)

(type + BReader (is Reader)
  (fn-expr + read String
    (str "saw " b)
  )
)

(fn-do + noisy String
  (param name String)
  (return name)
)
```

```stderr
Unhandled panic: Global 'b' is read before it is initialized
<no file:19>

Task 1

Stack trace:
  0  __source__:BReader:read no file:19:17
  1  __source__:init.__source__.globals no file:8:16
  2  __source__:brix.main no file
```

### A read that reaches an uninitialized eager global through a function call panics.

```brix crash
(main
  (print first)
)

(blank)
(prop + first _ (= (call read-name)))
(prop + name _ (= (call noisy "world")))

(fn-expr + read-name String
  name
)

(fn-do + noisy String
  (param name String)
  (return name)
)
```

```stderr
Unhandled panic: Global 'name' is read before it is initialized
<no file:11>

Task 1

Stack trace:
  0  __source__:read-name no file:11:3
  1  __source__:init.__source__.globals no file:7:20
  2  __source__:brix.main no file
```

### A panic inside a global initializer is reported.

```brix crash
(main
  (print z)
)

(blank)
(prop + z _ (= (call boom)))

(fn-do + boom Int
  (throw (call Panic "boom"))
  (return 1)
)
```

```stderr
Unhandled panic: boom
<no file:10>

Task 1

Stack trace:
  0  __source__:boom no file:10:3
  1  __source__:init.__source__.globals no file:7:16
  2  __source__:brix.main no file
```

### A default that runs code is evaluated by each call that leaves its argument out, and not by a call that gives it.

```brix
(main
  (print "main")
  (print (call bar 5))
)

(fn-do + make Int
  (print "made")
  (return 7)
)

(fn-expr + bar Int
  (param f Int (= (call make)))
  f
)
```

```output
main
5
```

### Defaults that run code are evaluated again by each call that leaves their arguments out, where the call is made.

```brix
(main
  (print "main")
  (print (call outer))
  (print (call outer))
  (print (call last))
)

(fn-do + make Int
  (param n Int)
  (print (str "made " n))
  (return n)
)

(fn-expr + outer Int
  (param f Int (= (call make 1)))
  (+ f (call inner))
)

(fn-expr + inner Int
  (param f Int (= (call make 2)))
  f
)

(fn-expr + last Int
  (param f Int (= (call make 3)))
  f
)
```

```output
main
made 1
made 2
3
made 1
made 2
3
made 3
3
```

### A default of a method reads a global as each call that leaves its argument out is made.

```brix
(main
  (print "main")
  (print (call (. (call Foo) get)))
  (print (call (. (call Foo) get) 1))
  (= base 100)
  (print (call (. (call Foo) get)))
)

(blank)
(prop + base _ (= 10))

(fn-do + twice Int
  (param n Int)
  (print "twice")
  (return (* n 2))
)

(type + Foo
  (fn-expr + get Int
    (param n Int (= (call twice base)))
    n
  )
)
```

```output
main
twice
20
1
twice
200
```

### An assigning operator reads the global it assigns, and panics when the global has no value yet.

```brix crash
(main
  (print "main")
  (print count)
)

(blank)
(prop + first _ (= (call bump)))
(prop + count _ (= (call start)))

(fn-do + start Int
  (return 10)
)

(fn-do + bump Int
  (+= count 1)
  (return 0)
)
```

```stderr
Unhandled panic: Global 'count' is read before it is initialized
<no file:16>

Task 1

Stack trace:
  0  __source__:bump no file:16:7
  1  __source__:init.__source__.globals no file:8:20
  2  __source__:brix.main no file
```

### A global assigned before its value is evaluated takes its value afterwards.

```brix
(main
  (print "main")
  (print count)
  (print first)
)

(blank)
(prop + first _ (= (call set)))
(prop + count _ (= (call start)))

(fn-do + start Int
  (print "start")
  (return 10)
)

(fn-do + set Int
  (= count 5)
  (return 1)
)
```

```output
start
main
10
1
```

### A lazy global whose value panics: the trace names the global's function under the read.

```brix crash
(main
  (print "main")
  (print a)
)

(blank)
(prop + a _ (:= (call fail)))

(fn-do + fail Int
  (print "fail")
  (throw (call Panic "broken"))
  (return 1)
)
```

```stderr
Unhandled panic: broken
<no file:12>

Task 1

Stack trace:
  0  __source__:fail no file:12:3
  1  __source__:init.__source__.global.a no file:8:17
  2  __source__:main no file:4:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A lazy global whose value panicked is evaluated again by its next read.

```brix
(main
  (do-new
    (print answer)
    (on-failure failure
      (print (str "first read: " (. failure message)))
      (print answer)
      (print tries)
    )
  )
)

(prop + tries _ (= 0))
(prop + answer _ (:= (call fail)))

(fn-do + fail Int
  (+= tries 1)
  (throw (call Panic "not yet") (if (== tries 1)))
  (return 7)
)
```

```output
first read: not yet
7
2
```

### A lazy global with a declared type converts its value, and an assigning operator evaluates it first.

```brix
(main
  (print "main")
  (print a)
  (+= a 2)
  (print a)
  (print b)
)

(blank)
(prop + a Float (:= (call one)))
(prop + b _ (:= (* a 2)))

(fn-do + one I32
  (print "one")
  (return 1)
)
```

```output
main
one
1
3
6
```

### A global nothing reads is still given its value as the module starts.

```brix
(main
  (print "main")
)

(blank)
(prop + unused _ (= (call Foo)))

(type + Foo
  (fn-do + This _
    (print "constructed")
  )
)
```

```output
constructed
main
```

### A lazy global read by a function is evaluated once, and not at all once it is assigned.

```brix
(main
  (print "main")
  (call show)
  (call show)
  (= total 9)
  (call show)
)

(blank)
(prop + total _ (:= (call compute)))

(fn-do + compute Int
  (print "compute")
  (return 3)
)

(fn-do + show _
  (print total)
)
```

```output
main
compute
3
3
9
```

### A global whose value is built from a call is read and assigned by the module and by a function.

```brix
(main
  (print greeting)
  (print (call shout))
  (++= greeting "!")
  (print greeting)
)

(blank)
(prop + greeting _ (= (++ "hello " (call name))))

(fn-expr + name String
  "world"
)

(fn-expr + shout String
  (++ greeting "?")
)
```

```output
hello world
hello world?
hello world!
```

### A lazy global assigned before it is read is never evaluated.

```brix
(main
  (= a 5)
  (print a)
)

(blank)
(prop + a _ (:= (call noisy 1)))

(fn-do + noisy Int
  (param n Int)
  (print "initializing")
  (return n)
)
```

```output
5
```

### A struct or a tuple held in a global is passed to a function and is the receiver of a member.

```brix
(main
  (print (call sum at))
  (print (call (. at total)))
  (print (str at))
  (let copy at)
  (= at (call Point 7 8))
  (print (call sum at))
  (print (. copy x))
  (print (call first pair))
  (print (call sum later))
  (print (call inside))
)

(blank)
(prop + at _ (= (call Point 1 2)))
(prop + pair _ (= (call Pair 3 4)))
(prop + later _ (:= (call Point 5 6)))

(fn-do + inside Int
  (return (+ (call (. at total)) (call sum at)))
)

(fn-expr + sum Int
  (param p Point)
  (+ (. p x) (. p y))
)

(fn-expr + first Int
  (param p Pair)
  (. p a)
)

(type + Pair (is Tuple)
  (prop + a Int)
  (prop + b Int)
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
  (fn-expr + total Int
    (+ x y)
  )
  (fn-to-expr + String
    (str "P(" x "," y ")")
  )
)
```

```output
3
3
P(1,2)
15
1
3
11
30
```

### A default that runs code and gives a struct is read by the call that leaves it out.

```brix
(main
  (print (. (call at) x))
  (print (call sum))
  (print (call sum (call Point 1 1)))
)

(fn-expr + at Point
  (param p Point (= (call make)))
  p
)

(fn-expr + sum Int
  (param p Point (= (call make)))
  (+ (. p x) (. p y))
)

(fn-do + make Point
  (return (call Point 3 4))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
3
7
2
```

### The value of a class's property that reads a global with no value yet panics.

```brix crash
(main
  (print "main")
  (print (. first n))
)

(blank)
(prop + first _ (= (call Thing)))
(prop + second _ (= (call say 2)))

(type + Thing
  (prop + n _ (= (+ second 1)))
)

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```stderr
Unhandled panic: Global 'second' is read before it is initialized
<no file:12>

Task 1

Stack trace:
  0  __source__:Thing:Thing no file:12:21
  1  __source__:init.__source__.globals no file:8:20
  2  __source__:brix.main no file
```

### The value of a lazy global that reads a global with no value yet panics.

```brix crash
(main
  (print "main")
  (print first)
  (print later)
)

(blank)
(prop + later _ (:= (* second 2)))
(prop + first _ (= (+ later 1)))
(prop + second _ (= (call say 2)))

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```stderr
Unhandled panic: Global 'second' is read before it is initialized
<no file:9>

Task 1

Stack trace:
  0  __source__:init.__source__.global.later no file:9:24
  1  __source__:init.__source__.globals no file:10:23
  2  __source__:brix.main no file
```

### The value of a class's property reads a global that has its value.

```brix
(main
  (print "main")
  (print (. first n))
  (print (. (call Thing) m))
)

(blank)
(prop + second _ (= (call say 2)))
(prop + first _ (= (call Thing)))
(prop + later _ (:= (* second 2)))

(type + Thing
  (prop + n _ (= (+ second 1)))
  (prop + m _ (= (+ later n)))
)

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```output
say 2
main
3
7
```

### A default that panics, in its own function, called by the call that leaves its argument out.

```brix crash
(main
  (print "main")
  (print (call bar 1))
  (print (call bar))
)

(blank)
(prop + g _ (= (call one)))

(fn-do + one Int
  (return 1)
)

(fn-expr + bar Int
  (param f Int (= (call boom)))
  f
)

(fn-do + boom Int
  (print "boom")
  (throw (call Panic "default mid"))
  (return 2)
)
```

```stderr
Unhandled panic: default mid
<no file:22>

Task 1

Stack trace:
  0  __source__:boom no file:22:3
  1  __source__:f-default no file:16:19
  2  __source__:main no file:5:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### The value of a global cannot name a local of the module's statements.

```brix crash
(main
  (let statement-local 3)
  (print g)
)

(blank)
(prop + g _ (= (+ statement-local 1)))
```

```output
Exception: Name "statement-local" not found
<no file:8>
    (prop + g _ (= (+ statement-local 1)))
                      ^^^^^^^^^^^^^^^
```

### The value of a global binds the names its patterns capture and the locals its blocks declare, eager or lazy, read or not, beside statements whose function captures their own local.

```brix
(prop + value Int
  (=
    (match (call maybe)
      (case (: n Int)
        (+ n 1)
      )
      (default
        0
      )
    )
  )
)

(prop + name String
  (=
    (match "name:ada\n"
      (~ (seq "name:" (: found (+ (chars "a-z"))) "\n")
        found
      )
      (default
        "none"
      )
    )
  )
)

(prop + next Int
  (=
    (block
      (let n 5)
      (+ n 1)
    )
  )
)

(prop + later Int
  (:=
    (match (call maybe)
      (case (: n Int)
        (* n 2)
      )
      (default
        0
      )
    )
  )
)

(prop _ unread Int
  (=
    (match (call maybe)
      (case (: n Int)
        (do (print (str "unread " n)))
        n
      )
      (default
        0
      )
    )
  )
)

(main
  (let base 10)
  (let add (=> (x) (+ x base)))
  (print (call add value))
  (print name)
  (print next)
  (print later)
)

(fn-expr + maybe (? Int) 5)
```

```output
unread 5
16
ada
6
10
```

### The values of the globals of a module that runs no code of its own bind the names they capture and declare.

```brix
(import ./lib/values)
(blank)

(main
  (print value)
  (print later)
)
```

```brix file=lib/values.bx
(prop + value Int
  (=
    (match (call maybe)
      (case (: n Int)
        (+ n 1)
      )
      (default
        0
      )
    )
  )
)

(prop + later Int
  (:=
    (block
      (let n 5)
      (* n 2)
    )
  )
)

(fn-expr _ maybe (? Int) 5)
```

```output
6
10
```

### A global of a function type holds a function written as its value, and is assigned another.

```brix
(prop + handler Handler (:= shout))
(prop + fallback Handler (= murmur))
(blank)

(main
  (call handler "one")
  (call fallback "two")
  (= handler whisper)
  (call handler "three")
)

(type + Handler (is (fn-type Void (param text String))))

(fn-do + shout _
  (param text String)
  (print (str text "!"))
)

(fn-do + murmur _
  (param text String)
  (print (str "(" text ")"))
)

(fn-do + whisper _
  (param text String)
  (print (str "..." text))
)
```

```output
one!
(two)
...three
```

### Globals initialized from case constants of the same module read the constants' values.

```brix
(main
  (print (str (. shade name) " " (. shade number)))
  (print level)
  (print picked)
)

(blank)
(prop + shade _ (= (. Color blue)))
(prop + level _ (= (. Level high)))
(prop + picked _ (= (call brightest)))

(fn-expr + brightest Color
  (. Color green)
)

(type + Color (is Tuple)
  (variant red (= (tuple 0 "red")))
  (variant green (= (tuple 1 "green")))
  (variant blue (= (tuple 2 "blue")))
  (blank)
  (prop + number Int)
  (prop + name String)
  (fn-to-expr + String
    name
  )
)

(type + Level (is Int)
  (variant low (= 1))
  (variant high (= 7))
)
```

```output
blue 2
7
green
```

### Functions called by a global initializer see their constant argument defaults.

```brix
(main
  (print sized)
  (print named)
  (print chosen)
)

(blank)
(prop + sized _ (= (call grow)))
(prop + named _ (= (call label)))
(prop + chosen _ (= (call pick)))

(fn-expr + grow Int
  (param size _ (= 10))
  (+ size 1)
)

(fn-expr + label String
  (param text _ (= "ten"))
  (str "got " text)
)

(fn-expr + pick Level
  (param level _ (= (. Level high)))
  level
)

(type + Level (is Int)
  (variant low (= 1))
  (variant high (= 7))
)
```

```output
11
got ten
7
```

### An error that leaves the value of a lazy global, which its first read evaluates, is reported, and the program stops.

```brix crash
(prop _ value _ (:= (call thrower 1)))
(prop _ other _ (:= (call thrower 0)))
(blank)

(main
  (print "start")
  (print value)
  (print "middle")
  (print other)
  (print "end")
)

(fn-do + thrower I64 (?! Error)
  (param n I64)
  (if (== n 0)
    (throw (call Error "bad"))
  )
  (return (* n 10))
)
```

```stderr
Unhandled error: bad
<no file:17>

Task 1

Stack trace:
  0  __source__:thrower no file:17:5
  1  __source__:init.__source__.global.other no file:3:21
  2  __source__:main no file:10:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A module that runs no code of its own gives a global no code reads its value as the program starts, though converting the items its feeds yield to a list may panic.

```brix
(import ./lib/items)
(blank)

(main
  (print "ok")
)
```

```brix file=lib/items.bx
(prop _ items (of List Int)
  (block
    (item 1)
    (item 2)
  )
)
```

```output
ok
```

### A lazy global of a module that runs no code of its own takes its value from another lazy global of the module.

```brix
(import ./lib/names)
(blank)

(main
  (print full)
)
```

```brix file=lib/names.bx
(prop + first _ (:= (call pick)))
(prop + full _ (:= (?? first "none")))

(fn-do _ pick (? String)
  (print "pick")
  (return "first")
)
```

```output
pick
first
```

### A module's getter is called where its name is read alone, and its setter where its name is assigned, in the module, in a method, and in a module that imports it.

```brix
(import ./lib/sizes)
(blank)

(main
  (print greeting)
  (let read greeting)
  (print (. read length))
  (= name "a")
  (= size 3)
  (print size)
  (call (. (call Box) go))
)

(fn-expr + greeting String :get
  "hi"
)

(fn-expr + name String :get
  "name"
)

(fn-do + name _ :set (param value String)
  (print (str "name = " value))
)

(type + Box
  (fn-do + go _
    (print name)
    (= name "b")
  )
)
```

```brix file=lib/sizes.bx
(fn-expr + size F64 :get
  2.5
)

(fn-do + size _ :set (param value F64)
  (print (str "size = " value))
)
```

```output
hi
2
name = a
size = 3
2.5
name
name = b
```

### An assigning operator on a module's getter and setter reads the value through the getter and stores the new one through the setter, in the module and in a module that imports it.

```brix
(import ./lib/sizes)
(blank)

(main
  (+= count 2)
  (*= size 2)
  (++= title "!")
)

(fn-expr + count Int :get
  1
)

(fn-do + count _ :set (param value Int)
  (print (str "count = " value))
)

(fn-expr + title String :get
  "hi"
)

(fn-do + title _ :set (param value String)
  (print (str "title = " value))
)
```

```brix file=lib/sizes.bx
(fn-expr + size F64 :get
  2.5
)

(fn-do + size _ :set (param value F64)
  (print (str "size = " value))
)
```

```output
count = 3
size = 5
title = hi!
```
