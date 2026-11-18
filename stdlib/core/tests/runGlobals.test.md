# Globals

```yumi
+ a = 42

 -- a
a = 99
foo()
 -- a

━━━━━━━━━━━━━━━━━━━━
+ do foo

-- a
a = 88
```

```output
42
99
88
```

### Constructor in global variable

```yumi
-- foo

+ foo = Foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ This
```

```output
[Object __source__:Foo]
```

### Constructor as default value of global

```yumi
-- foo

+ foo Foo

━━━━━━━━━━━━━━━━━━━━
+ Foo
```

```output
[Object __source__:Foo]
```

### Cast in global assignment

```yumi
-- foo

+ foo F64 = Foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ to F64

99.9f
```

```output
99.9
```

### Tuple in global assignment

```yumi
-- foo

+ foo Foo

━━━━━━━━━━━━━━━━━━━━
+ Foo is (a Int, b Int)
```

```output
(0, 0)
```

### Tuple with optional in global assignment

```yumi
-- foo
-- bar

+ foo = Foo(0, "hi")
+ bar = Foo(0, "hi", 9)

━━━━━━━━━━━━━━━━━━━━
+ Foo is (a Int, b String, c I32?)
```

```output
(0, hi, unknown)
(0, hi, 9)
```

### Optional in global assignment

```yumi
-- foo
-- bar

+ foo I32?
+ bar I32? = 1
```

```output
unknown
1
```

### TypeInfo in global assignment

```yumi
-- foo

+ foo = type-of 42
```

```output
[TypeInfo I64]
```

```yumi
-- foo

+ foo = I32[3]
```

```output
I32[3]
```

### Circular reference between global and argument default

```yumi crash
-- foo
-- bar()

+ foo = bar()

━━━━━━━━━━━━━━━━━━━━
+ bar
    - f = foo

f
```

```output
Exception: "foo" depends on its own value, directly or through the values it reads; break the cycle
<no file:5>
    + foo = bar()
    ^^^^^^^^^^^^^
```

### Assign to global variable before it is referenced

```yumi
foo = "boo"
-- foo

+ foo = "foobar"
```

```output
boo
```

```yumi
+ foo := goober()

-- "hi"

━━━━━━━━━━━━━━━━━━━━
+ goober

-- "goober"
42
```

```output
hi
```

```yumi
+ foo := goober()

-- "hi"
-- foo

━━━━━━━━━━━━━━━━━━━━
+ goober

-- "goober"
42
```

```output
hi
goober
42
```

### Eager globals initialize in declaration order before main, including globals nothing reads.

```yumi
-- "main starts"
-- second
-- first

+ first = noisy("first")
+ second = noisy("second")
+ unused = noisy("unused")

━━━━━━━━━━━━━━━━━━━━
+ do noisy String
    - name String

-- "initializing $name"
= name
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

```yumi
-- "main starts"
-- second
-- first

+ first := noisy("first")
+ second := noisy("second")
+ unused := noisy("unused")

━━━━━━━━━━━━━━━━━━━━
+ do noisy String
    - name String

-- "initializing $name"
= name
```

```output
main starts
initializing second
second
initializing first
first
```

### Writing a lazy global before its first read skips its initializer.

```yumi
a = 5
-- a

+ a := noisy(1)

━━━━━━━━━━━━━━━━━━━━
+ do noisy Int
    - n Int

-- "initializing"
= n
```

```output
5
```

### An initializer that directly reads a later eager global is a compile error.

```yumi crash
-- a

+ a = b.length
+ b = noisy("b")

━━━━━━━━━━━━━━━━━━━━
+ do noisy String
    - name String

= name
```

```output
Exception: Global 'b' is read before it is initialized
<no file:4>
    + a = b.length
          ^
```

### A read that reaches an uninitialized eager global through a virtual call panics.

```yumi crash
-- a

+ reader Reader = BReader()
+ a = reader.read()
+ b = noisy("b")

━━━━━━━━━━━━━━━━━━━━
+ Reader

────────────────────
+ read String

"base"

━━━━━━━━━━━━━━━━━━━━
+ BReader is Reader

────────────────────
+ read String

"saw $b"

━━━━━━━━━━━━━━━━━━━━
+ do noisy String
    - name String

= name
```

```stderr
Unhandled panic: Global 'b' is read before it is initialized
<no file:22>

Task 1

Stack trace:
  0  __source__:BReader:read no file:22:7
  1  __source__:init.__source__.globals no file:5:7
  2  __source__:brix.main no file
```

### A read that reaches an uninitialized eager global through a function call panics.

```yumi crash
-- first

+ first = read-name()
+ name = noisy("world")

━━━━━━━━━━━━━━━━━━━━
+ read-name String

name

━━━━━━━━━━━━━━━━━━━━
+ do noisy String
    - name String

= name
```

```stderr
Unhandled panic: Global 'name' is read before it is initialized
<no file:10>

Task 1

Stack trace:
  0  __source__:read-name no file:10:1
  1  __source__:init.__source__.globals no file:4:11
  2  __source__:brix.main no file
```

### A panic inside a global initializer is reported.

```yumi crash
-- z

+ z = boom()

━━━━━━━━━━━━━━━━━━━━
+ do boom Int

?! Panic("boom")
= 1
```

```stderr
Unhandled panic: boom
<no file:9>

Task 1

Stack trace:
  0  __source__:boom no file:9:1
  1  __source__:init.__source__.globals no file:4:7
  2  __source__:brix.main no file
```

### A default that runs code is evaluated by each call that leaves its argument out, and not by a call that gives it.

```yumi
-- "main"
-- bar(5)

━━━━━━━━━━━━━━━━━━━━
+ do make Int

-- "made"
= 7

━━━━━━━━━━━━━━━━━━━━
+ bar Int
    - f Int = make()

f
```

```output
main
5
```

### Defaults that run code are evaluated again by each call that leaves their arguments out, where the call is made.

```yumi
-- "main"
-- outer()
-- outer()
-- last()

━━━━━━━━━━━━━━━━━━━━
+ do make Int
    - n Int

-- "made $n"
= n

━━━━━━━━━━━━━━━━━━━━
+ outer Int
    - f Int = make(1)

f + inner()

━━━━━━━━━━━━━━━━━━━━
+ inner Int
    - f Int = make(2)

f

━━━━━━━━━━━━━━━━━━━━
+ last Int
    - f Int = make(3)

f
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

```yumi
-- "main"
-- Foo().get()
-- Foo().get(1)
base = 100
-- Foo().get()

+ base = 10

━━━━━━━━━━━━━━━━━━━━
+ do twice Int
    - n Int

-- "twice"
= n * 2

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ get Int
    - n Int = twice(base)

n
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

```yumi crash
-- "main"
-- count

+ first = bump()
+ count = start()

━━━━━━━━━━━━━━━━━━━━
+ do start Int

= 10

━━━━━━━━━━━━━━━━━━━━
+ do bump Int

count += 1
= 0
```

```stderr
Unhandled panic: Global 'count' is read before it is initialized
<no file:16>

Task 1

Stack trace:
  0  __source__:bump no file:16:1
  1  __source__:init.__source__.globals no file:5:11
  2  __source__:brix.main no file
```

### A global assigned before its value is evaluated takes its value afterwards.

```yumi
-- "main"
-- count
-- first

+ first = set()
+ count = start()

━━━━━━━━━━━━━━━━━━━━
+ do start Int

-- "start"
= 10

━━━━━━━━━━━━━━━━━━━━
+ do set Int

count = 5
= 1
```

```output
start
main
10
1
```

### A lazy global whose value panics: the trace names the global's function under the read.

```yumi crash
-- "main"
-- a

+ a := fail()

━━━━━━━━━━━━━━━━━━━━
+ do fail Int

-- "fail"
?! Panic("broken")
= 1
```

```stderr
Unhandled panic: broken
<no file:11>

Task 1

Stack trace:
  0  __source__:fail no file:11:1
  1  __source__:init.__source__.global.a no file:5:8
  2  __source__:main no file:3:4
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A lazy global with a declared type converts its value, and an assigning operator evaluates it first.

```yumi
-- "main"
-- a
a += 2
-- a
-- b

+ a Float := one()
+ b := a * 2

━━━━━━━━━━━━━━━━━━━━
+ do one I32

-- "one"
= 1
```

```output
main
one
1
3
6
```

### A global nothing reads is still given its value as the module starts.

```yumi
-- "main"

+ unused = Foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ This

-- "constructed"
```

```output
constructed
main
```

### A lazy global read by a function is evaluated once, and not at all once it is assigned.

```yumi
-- "main"
show()
show()
total = 9
show()

+ total := compute()

━━━━━━━━━━━━━━━━━━━━
+ do compute Int

-- "compute"
= 3

━━━━━━━━━━━━━━━━━━━━
+ do show

-- total
```

```output
main
compute
3
3
9
```

### A global whose value is built from a call is read and assigned by the module and by a function.

```yumi
-- greeting
-- shout()
greeting ++= "!"
-- greeting

+ greeting = "hello " ++ name()

━━━━━━━━━━━━━━━━━━━━
+ name String

"world"

━━━━━━━━━━━━━━━━━━━━
+ shout String

greeting ++ "?"
```

```output
hello world
hello world?
hello world!
```

### A lazy global assigned before it is read is never evaluated.

```yumi
a = 5
-- a

+ a := noisy(1)

━━━━━━━━━━━━━━━━━━━━
+ do noisy Int
    - n Int

-- "initializing"
= n
```

```output
5
```

### A struct or a tuple held in a global is passed to a function and is the receiver of a member.

```yumi
-- sum(at)
-- at.total()
-- "$at"
. copy = at
at = Point(7, 8)
-- sum(at)
-- copy.x
-- first(pair)
-- sum(later)
-- inside()

+ at = Point(1, 2)
+ pair = Pair(3, 4)
+ later := Point(5, 6)

━━━━━━━━━━━━━━━━━━━━
+ do inside Int

= at.total() + sum(at)

━━━━━━━━━━━━━━━━━━━━
+ sum Int
    - p Point

p.x + p.y

━━━━━━━━━━━━━━━━━━━━
+ first Int
    - p Pair

p.a

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ a Int
+ b Int

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

────────────────────
+ total Int

x + y

────────────────────
+ to String

"P($x,$y)"
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

```yumi
-- at().x
-- sum()
-- sum(Point(1, 1))

━━━━━━━━━━━━━━━━━━━━
+ at Point
    - p Point = make()

p

━━━━━━━━━━━━━━━━━━━━
+ sum Int
    - p Point = make()

p.x + p.y

━━━━━━━━━━━━━━━━━━━━
+ do make Point

= Point(3, 4)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
3
7
2
```

### The value of a class's property that reads a global with no value yet panics.

```yumi crash
-- "main"
-- first.n

+ first = Thing()
+ second = say(2)

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ n = second + 1

━━━━━━━━━━━━━━━━━━━━
+ do say Int
    - v Int

-- "say $v"
= v
```

```stderr
Unhandled panic: Global 'second' is read before it is initialized
<no file:11>

Task 1

Stack trace:
  0  __source__:Thing:Thing no file:11:7
  1  __source__:init.__source__.globals no file:5:11
  2  __source__:brix.main no file
```

### The value of a lazy global that reads a global with no value yet panics.

```yumi crash
-- "main"
-- first
-- later

+ later := second * 2
+ first = later + 1
+ second = say(2)

━━━━━━━━━━━━━━━━━━━━
+ do say Int
    - v Int

-- "say $v"
= v
```

```stderr
Unhandled panic: Global 'second' is read before it is initialized
<no file:6>

Task 1

Stack trace:
  0  __source__:init.__source__.global.later no file:6:12
  1  __source__:init.__source__.globals no file:7:11
  2  __source__:brix.main no file
```

### The value of a class's property reads a global that has its value.

```yumi
-- "main"
-- first.n
-- Thing().m

+ second = say(2)
+ first = Thing()
+ later := second * 2

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ n = second + 1
+ m = later + n

━━━━━━━━━━━━━━━━━━━━
+ do say Int
    - v Int

-- "say $v"
= v
```

```output
say 2
main
3
7
```

### A default that panics, in its own function, called by the call that leaves its argument out.

```yumi crash
-- "main"
-- bar(1)
-- bar()

+ g = one()

━━━━━━━━━━━━━━━━━━━━
+ do one Int

= 1

━━━━━━━━━━━━━━━━━━━━
+ bar Int
    - f Int = boom()

f

━━━━━━━━━━━━━━━━━━━━
+ do boom Int

-- "boom"
?! Panic("default mid")
= 2
```

```stderr
Unhandled panic: default mid
<no file:23>

Task 1

Stack trace:
  0  __source__:boom no file:23:1
  1  __source__:f-default no file:15:15
  2  __source__:main no file:4:4
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### The value of a global cannot name a local of the module's statements.

```yumi crash
. statement-local = 3
-- g

+ g = statement-local + 1
```

```output
Exception: Name "statement-local" not found
<no file:5>
    + g = statement-local + 1
          ^^^^^^^^^^^^^^^
```

### A global of a function type holds a function written as its value, and is assigned another.

```yumi
+ handler Handler := shout
+ fallback Handler = murmur

handler("one")
fallback("two")
handler = whisper
handler("three")

━━━━━━━━━━━━━━━━━━━━
+ Handler is () Void
    - text String

━━━━━━━━━━━━━━━━━━━━
+ do shout
    - text String

-- "$text!"

━━━━━━━━━━━━━━━━━━━━
+ do murmur
    - text String

-- "($text)"

━━━━━━━━━━━━━━━━━━━━
+ do whisper
    - text String

-- "...$text"
```

```output
one!
(two)
...three
```

### Globals initialized from case constants of the same module read the constants' values.

```yumi
-- "${shade.name} ${shade.number}"
-- level
-- picked

+ shade = Color.blue
+ level = Level.high
+ picked = brightest()

━━━━━━━━━━━━━━━━━━━━
+ brightest Color

Color.green

━━━━━━━━━━━━━━━━━━━━
+ Color is Tuple

| red = (0, "red")
| green = (1, "green")
| blue = (2, "blue")

+ number Int
+ name String

────────────────────
+ to String

name

━━━━━━━━━━━━━━━━━━━━
+ Level is Int

| low = 1
| high = 7
```

```output
blue 2
7
green
```

### Functions called by a global initializer see their constant argument defaults.

```yumi
-- sized
-- named
-- chosen

+ sized = grow()
+ named = label()
+ chosen = pick()

━━━━━━━━━━━━━━━━━━━━
+ grow Int
    - size = 10

size + 1

━━━━━━━━━━━━━━━━━━━━
+ label String
    - text = "ten"

"got $text"

━━━━━━━━━━━━━━━━━━━━
+ pick Level
    - level = Level.high

level

━━━━━━━━━━━━━━━━━━━━
+ Level is Int

| low = 1
| high = 7
```

```output
11
got ten
7
```

### An error that leaves the value of a lazy global, which its first read evaluates, is reported, and the program stops.

```yumi crash
_ value := thrower(1)
_ other := thrower(0)

-- "start"
-- value
-- "middle"
-- other
-- "end"

━━━━━━━━━━━━━━━━━━━━
+ do thrower I64 ?! Error
    - n I64

if n == 0
    ?! Error("bad")
= n * 10
```

```stderr
Unhandled error: bad
<no file:16>

Task 1

Stack trace:
  0  __source__:thrower no file:16:5
  1  __source__:init.__source__.global.other no file:3:12
  2  __source__:main no file:8:4
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A module that runs no code of its own gives a global no code reads its value as the program starts, though converting the items its feeds yield to a list may panic.

```yumi
./lib/items

-- "ok"
```

```yumi file=lib/items.yumi

_ items List'Int
    * 1
    * 2

```

```output
ok
```

### A lazy global of a module that runs no code of its own takes its value from another lazy global of the module.

```yumi
./lib/names

-- full
```

```yumi file=lib/names.yumi

+ first := pick()
+ full := first ?? "none"

━━━━━━━━━━━━━━━━━━━━
_ do pick String?

-- "pick"
= "first"

```

```output
pick
first
```

### A module's getter is called where its name is read alone, and its setter where its name is assigned, in the module, in a method, and in a module that imports it.

```yumi
./lib/sizes

-- greeting
. read = greeting
-- read.length
name = "a"
size = 3
-- size
Box().go()

━━━━━━━━━━━━━━━━━━━━
+ get greeting String

"hi"

━━━━━━━━━━━━━━━━━━━━
+ get name String

"name"

━━━━━━━━━━━━━━━━━━━━
+ do name = value String

-- "name = $value"

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do go

-- name
name = "b"
```

```yumi file=lib/sizes.yumi

━━━━━━━━━━━━━━━━━━━━
+ get size F64

2.5

━━━━━━━━━━━━━━━━━━━━
+ do size = value F64

-- "size = $value"

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

```yumi
./lib/sizes

count += 2
size *= 2
title ++= "!"

━━━━━━━━━━━━━━━━━━━━
+ get count Int

1

━━━━━━━━━━━━━━━━━━━━
+ do count = value Int

-- "count = $value"

━━━━━━━━━━━━━━━━━━━━
+ get title String

"hi"

━━━━━━━━━━━━━━━━━━━━
+ do title = value String

-- "title = $value"
```

```yumi file=lib/sizes.yumi

━━━━━━━━━━━━━━━━━━━━
+ get size F64

2.5

━━━━━━━━━━━━━━━━━━━━
+ do size = value F64

-- "size = $value"

```

```output
count = 3
size = 5
title = hi!
```
