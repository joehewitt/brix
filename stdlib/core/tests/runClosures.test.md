# Closures

```yumi
. a = =>
    42
-- a()
```

```output
42
```

```yumi
. a = => do
    -- "hello"
a()
```

```output
hello
```

```yumi
. a = :: x => 42
-- a(1)
```

```output
42
```

### Different instances of closure with different generic argument types

```yumi
. a = :: b => do
    -- "hello $b"
a(24)
a(24.5)
```

```output
hello 24
hello 24.5
```

### Call closure passed in as argument

```yumi
. a = :: b => b
-- foo(a, 24)
-- foo(a, 24.5)

━━━━━━━━━━━━━━━━━━━━
+ foo
    - fn
    - n

fn(n)
```

```output
24
24.5
```

```yumi
. n = 42
. a = :: b => b * n * k

-- foo(a, 2)
n = 100
-- foo(a, 4)

_ k = 10

━━━━━━━━━━━━━━━━━━━━
_ foo
    - fn
    - n

fn(n)
```

```output
840
4000
```

```yumi
. n = 42
. a = :: b =>
    bn
    .. bn = b * n

-- a(2)
```

```output
84
```

### Multiple closures

```yumi
. n = 42
. d = 2
. a = :: b => b * n
. a2 = :: b => b * n * d

-- a(2)
-- a2(2)
```

```output
84
168
```

### Modify closure

```yumi
. n = 42
. mutate = :: b => do
    n *= b

mutate(2)
-- n
```

```output
84
```

### Multiple closures, one modifies, then the other can see modification

```yumi
. n = 42
. mutate = :: b => do
    n *= b
. ref = :: b => n * b

mutate(2)
-- ref(2)
```

```output
168
```

### Assign to variable declared after closure

```yumi
. mutate = => do
    n = 2
. n = 42

mutate()
-- n
```

```output
2
```

### Nested closure

```yumi
. n = 42
. a = :: b =>
    =>
        n * b

. f = a(2)
-- f()
```

```output
84
```

### Doubly nested closure

```yumi
. n = 42
. a = =>
    =>
        =>
            n

. f = a()
. b = f()
-- b()
```

```output
42
```

### Nested closure capturing lazy variable

```yumi
. n = 42
. a = =>
    :: b =>
        n * b * c
    .. c = 2

-- a()(2)
```

```output
168
```

### Nested closure mutating lazy variable

```yumi
. a = =>
    :: b => do
        c += b
        = c
    .. c = 0

. f = a()
-- f(2)
-- f(3)
```

```output
2
5
```

### Closures at same level reference each other

```yumi
. n = 42
. x = =>
    n
. a = =>
    x()

-- a()
```

```output
42
```

### Nested closure mutation

```yumi
. n = 42
. a = =>
    :: b => do
        n *= b

a()(2)
-- n
```

```output
84
```

```yumi
. n = 42
. a = =>
    =>
        :: b => do
            n *= b

a()()(2)
-- n
```

```output
84
```

### Closure in method binds to 'this'

```yumi
. thing = Thing()
. get-self = thing.get-self
-- get-self().a

. get-a = thing.get-a
-- get-a()
thing.a = 10
-- get-a()
-- get-self().a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 5

+ get-self Getter'This := => this
+ get-a Getter'Int := => a
```

```output
5
5
10
10
```

### Illegal closure in property

```yumi crash
. thing = Thing()
. get-self = thing.get-self
-- get-self()

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ get-self = cb as Getter'This
    .. cb = => this
```

```output
Exception: Illegal to reference 'this' in eager properties or properties referenced by them
<no file:10>
        .. cb = => this
                   ^^^^
```

### Refer to property in closure within method

```yumi
. thing = Thing()
-- thing.get-self()()

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 5

────────────────────
+ get-self

=>
    a
```

```output
5
```

## Closures in methods

```yumi
. thing = Thing()
. s = thing.foo()
-- s()(42)

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ foo

=>
    bar

────────────────────
+ bar
    - n I32

"bar $n"
```

```output
bar 42
```

```yumi
. thing = Thing()
. s = thing.foo()
-- s()

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ baz = 42

────────────────────
+ foo

=>
    baz
```

```output
42
```

```yumi
. thing = Thing()
. s = thing.foo()
-- s()(42)

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ foo

=>
    baz

────────────────────
+ baz
    - n I32

"baz $n"
```

```output
baz 42
```

```yumi crash
. thing = Thing()
. s = thing.foo()
-- s()(42)

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ foo

=>
    bar
```

```output
Exception: Name "bar" not found
<no file:13>
        bar
        ^^^
```

## Closures in methods of non-classes

```yumi
. thing = Thing()
. s = thing.foo()
-- s()(42)

━━━━━━━━━━━━━━━━━━━━
+ Thing is I32

────────────────────
+ foo

=>
    bar

────────────────────
+ bar
    - n I32

"bar $n"
```

```output
bar 42
```

```yumi
. thing = Thing()
. s = thing.foo()
-- s()

+ baz = 42

━━━━━━━━━━━━━━━━━━━━
+ Thing is I32

────────────────────
+ foo

=> baz
```

```output
42
```

```yumi
. thing = Thing()
. s = thing.foo()
-- s()(42)

━━━━━━━━━━━━━━━━━━━━
+ Thing is I32

────────────────────
+ foo

=> baz

────────────────────
+ baz
    - n I32

"baz $n"
```

```output
baz 42
```

```yumi crash
. thing = Thing()
. s = thing.foo()
-- s()(42)

━━━━━━━━━━━━━━━━━━━━
+ Thing is I32

────────────────────
+ foo

=> bar
```

```output
Exception: Name "bar" not found
<no file:12>
    => bar
       ^^^
```

## Closures in generators

```yumi
. gen = foo(1.5)
for f in gen
    f()

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - n

<= => do
    -- n
```

```output
1.5
```

## Casting closures to function types

```yumi
. a = => 42
. foo = a as Foo
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Int
```

```output
42
```

```yumi
. a = :: n => 42 + n
. foo = a as Foo
-- foo(1)

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Int
    - n Int
```

```output
43
```

```yumi
. a = :: n Float => 42.5 + n
. foo = a as Foo
-- foo(1)

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Int
    - n Int
```

```output
43
```

```yumi
. a = :: n Int => 42 + n
-- call1(a)

━━━━━━━━━━━━━━━━━━━━
+ Fooi is () Int
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ Foof is () Int
    - n I32

━━━━━━━━━━━━━━━━━━━━
+ call1
    - fn Fooi

call2(fn)

━━━━━━━━━━━━━━━━━━━━
+ call2
    - fn Foof

fn(10)
```

```output
52
```

```yumi
. a = :: n Int => 42 + n
. f1 = a as Fooi
. f2 = f1 as Foof
-- f2(10)

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ from other Int

Thing()

────────────────────
+ to Int

5

━━━━━━━━━━━━━━━━━━━━
+ Fooi is () Int
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ Foof is () Float
    - n Thing
```

```output
47
```

```yumi crash
. a = :: n => 42 + n
. foo = a as Foo
-- foo(1, 2)

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Int
    - n Int
```

```output
Exception: Wrong number of arguments
<no file:4>
    -- foo(1, 2)
       ^^^^^^^^^
```

```yumi crash
. a = :: n => 42 + n
. foo = a as Foo
-- foo(1, 2)

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Int
    - n Int
```

```output
Exception: Wrong number of arguments
<no file:4>
    -- foo(1, 2)
       ^^^^^^^^^
```

```yumi
. a = => do
    -- "hi"

. foo = a as Foo
foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Void
```

```output
hi
```

```yumi
. a = :: n => do
    -- n
. foo = a as Foo
foo(42)

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Void
    - n Int
```

```output
42
```

### Cast to function with different return type

```yumi
. a = => 42
. foo = a as Foo
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Float
```

```output
42
```

### Cast to function with return type with compatible converter

```yumi
. a = => 42
. foo = a as Foo
-- foo().num

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ num = 0

────────────────────
+ do from n Int

. t = Thing()
t.num = n
= t

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Thing
```

```output
42
```

### Cast to function with different arguments

```yumi crash
. a = :: n =>
    -- n
. foo = a as Foo
foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Void
```

```output
Exception: closure cannot be cast to Foo; there is no conversion between them
<no file:4>
    . foo = a as Foo
            ^^^^^^^^
```

### Cast function with no return type to function with return type

```yumi crash
. a = => do
    -- "hi"

. foo = a as Foo
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Int
```

```output
Exception: Void cannot be cast to I64; there is no conversion between them
<no file:5>
    . foo = a as Foo
            ^^^^^^^^
```

### Cast to function with incompatible return type

```yumi crash
. a = => 42
. foo = a as Foo
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ Thing

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Thing
```

```output
Exception: I64 cannot be cast to Thing; there is no conversion between them
<no file:3>
    . foo = a as Foo
            ^^^^^^^^
```

### Closure within interface function

```yumi
. thing = Thing() as Foo
-- thing
thing.foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foo

────────────────────
+ do foo

. x = 42
. a = => do
    -- x
a()
```

```output
[Interface __source__:Foo]
42
```

### Crashes because we don't correctly rewrite closure inside of a generator It crashes accessing foo because this was loaded from closure field 2 but was written to field 0

```yumi
. thing = Thing()
for a in thing.action()
    -- a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ foo = 42

────────────────────
+ do action

. getter = => foo
-- getter()
<= 1
```

```output
42
1
```

### A generic function whose parameter is an inline function type infers its type argument from the closure.

```yumi
-- apply(4, :: n => n + 1)
-- apply(4, :: n => "n = $n")

━━━━━━━━━━━━━━━━━━━━
+ apply'Result
    - n Int
    - func () Result
        - value Int

func(n)
```

```output
5
n = 4
```

### A local is one cell for the function that declares it and the closure that captures it.

```yumi
. total = 0
. add = :: amount => do
    total += amount
    = total
-- add(2)
total += 10
-- add(3)
-- total
```

```output
2
15
15
```

### Two closures share a local, and a closure in a closure reaches it two environments out.

```yumi
. count = 0
. bump = => do
    count += 1
. read = => count
bump()
bump()
-- read()
. outer = :: step => do
    . inner = :: times => do
        count += step * times
        = count
    = inner(2) + inner(3)
-- outer(10)
-- count
```

```output
2
74
52
```

### Each run of a function has its own environment, which outlives the run.

```yumi
. first = counter(10)
. second = counter(100)
-- first(1)
-- first(2)
-- second(5)
-- first(3)

━━━━━━━━━━━━━━━━━━━━
+ do counter
    - start Int

. held = start
. step = :: amount => do
    held += amount
    = held
= step
```

```output
11
13
105
16
```

### A closure whose value is a closure, called after the closure that wrote it has returned.

```yumi
. scale = 3
. make = :: base =>
    :: amount => amount * scale + base
. plus-one = make(1)
. plus-ten = make(10)
-- plus-one(2)
scale = 4
-- plus-one(2)
-- plus-ten(2)
```

```output
7
9
18
```

### A closure written in a method reads and assigns the properties of the object it was made on.

```yumi
. first = Tally()
. second = Tally()
. add = first.adder()
. other = second.adder()
-- add(2)
-- add(3)
-- other(10)
first.total = 100
-- add(1)
-- second.total

━━━━━━━━━━━━━━━━━━━━
+ Tally

+ total = 0

────────────────────
+ do adder

. add = :: amount => do
    total += amount
    = total
= add
```

```output
2
5
10
101
10
```

### A text and an object that a closure captures survive the collector.

```yumi
. label = "start"
. box = Box()
. note = :: text => do
    label = "$label/$text"
    box.count += 1
note("a")
churn()
note("b")
churn()
-- label
-- box.count

━━━━━━━━━━━━━━━━━━━━
+ Box

+ count = 0

━━━━━━━━━━━━━━━━━━━━
+ do churn

. kept = ""
. turn = 0
while turn < 20000
    kept = "text $turn"
    turn += 1
```

```output
start/a/b
2
```

### A closure written as the value of a method reads a property and calls a method.

```yumi
. thing = Thing()
. read = thing.reader()
-- read()
thing.size = 7
-- read()
-- thing.twice()()

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ size = 2

────────────────────
+ reader

=> size * 10

────────────────────
+ twice

=> double(size)

────────────────────
+ double Int
    - amount Int

amount * 2
```

```output
20
70
14
```

### A method read by its name inside a closure binds the object, and is called outside its class.

```yumi
. thing = Thing()
. s = thing.held()
. bound = s()
-- bound(42)

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ do foo

. getter = => bar
. bound = getter()
= bound(42)

────────────────────
+ do direct

. bound = bar
= bound(42)

────────────────────
+ held

=> bar

────────────────────
+ bar
    - n I32

"bar $n"
```

```output
bar 42
```

### Nested functions in the members of a number type read the number.

```yumi
. m = Meters(5)
. add = m.adder(10)
-- add(1)
-- add(2)
. twice = m.twice()
-- twice()

━━━━━━━━━━━━━━━━━━━━
+ Meters is I32

────────────────────
+ adder
    - base I32

:: n I32 => (this as I32) + base + n

────────────────────
+ twice

=> scaled(2)

────────────────────
+ scaled I32
    - by I32

(this as I32) * by
```

```output
16
17
10
```

### A nested function in a member of an enum calls a member on the case.

```yumi
. c = Color.green
. name = c.namer()
-- name(1)
. other = Color.red.namer()
-- other(2)

━━━━━━━━━━━━━━━━━━━━
+ Color is U8

| red
| green

────────────────────
+ namer

:: n Int => n + code()

────────────────────
+ code Int

if this == Color.red then 10 else 20
```

```output
21
12
```

### A nested function in one, in a member of a number type.

```yumi
. m = Meters(5)
. far = m.later()
-- far()()

━━━━━━━━━━━━━━━━━━━━
+ Meters is I32

────────────────────
+ later

=> => (this as I32) + 1
```

```output
6
```

### A nested function in a member of a type that is a text.

```yumi
. w = Word("hey")
. shout = w.shouter()
-- shout(2)
-- shout(1)

━━━━━━━━━━━━━━━━━━━━
+ Word is String

────────────────────
+ shouter

:: n Int => if n > 1 then "${this as String}!" else "${this as String}"
```

```output
hey!
hey
```

### A closure that states its argument's type is made a value of a function type that gives it a number of another type and takes another from it.

```yumi
. a = :: n Int => 42 + n
. f = a as Foof
-- f(10.7)
-- f(-3.2)

━━━━━━━━━━━━━━━━━━━━
+ Foof is () Float
    - n Float
```

```output
52
39
```

### A closure of two typed arguments that captures a local is made a value of a function type whose arguments and answer are of other types.

```yumi
. k = 3
. a = :: x Int, y Float => x * k + y
. f = a as Mix
-- f(2.9, 4)
k = 5
-- f(2.9, 4)

━━━━━━━━━━━━━━━━━━━━
+ Mix is () Int
    - x Float
    - y Int
```

```output
10
14
```

### A closure's answer is made a number from a truth, and a truth from a number, by the function type it is given to.

```yumi
. a = :: n Int => n > 2
. f = a as Num
-- f(3.5)
-- f(1.5)
. b = :: n Int => n - 3
. g = b as Truth
-- g(3)
-- g(4)

━━━━━━━━━━━━━━━━━━━━
+ Num is () Int
    - n Float

━━━━━━━━━━━━━━━━━━━━
+ Truth is () Bool
    - n Int
```

```output
1
0
false
true
```

### A closure's answer is made an optional, and a narrower number, by the function type it is given to.

```yumi
. a = :: n Int => n + 1
. f = a as Opt
-- f(3) ?? -1
. g = a as Small
-- g(300)
-- g(127)

━━━━━━━━━━━━━━━━━━━━
+ Opt is () Int?
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ Small is () I8
    - n I16
```

```output
4
45
-128
```

### A function, a closure with no type stated and a closure with one are added to a list of a function type and called in turn.

```yumi
. fns [Whole]
fns += half
fns += :: n => n * 3
. third = :: n Float => n / 3
fns += third
for fn in fns
    -- fn(9)

━━━━━━━━━━━━━━━━━━━━
+ Whole is () Float
    - n I32

━━━━━━━━━━━━━━━━━━━━
+ half
    - n Float

n / 2
```

```output
4.5
27
3
```

### A closure with a typed argument is given to a function, written where it is given, and returned as a value of a function type.

```yumi
. a = :: n Int => n + 0.5
-- run(a)
-- run(:: n Int => n * 2.0)
-- keep(a)(4)

━━━━━━━━━━━━━━━━━━━━
+ Half is () Float
    - n I32

━━━━━━━━━━━━━━━━━━━━
+ run
    - fn Half

fn(2)

━━━━━━━━━━━━━━━━━━━━
+ keep Half
    - fn Half

fn
```

```output
2.5
4
4.5
```

### Closures written where a function takes a function type state other types for their argument, or none.

```yumi
-- run(:: n Int => n + 0.5)
-- run(:: n Float => n * 2)
-- run(:: n => n * 1.5)

━━━━━━━━━━━━━━━━━━━━
+ Half is () Float
    - n I32

━━━━━━━━━━━━━━━━━━━━
+ run
    - fn Half

fn(3)
```

```output
3.5
6
4.5
```

### Closures that take a value of an interface and an object of a base class are given to function types that give an object of the class and of one derived from it.

```yumi
. a = :: label Label => label.label()
. f = a as Taker
-- f(Thing("one"))
. b = :: base Base => base.kind()
-- run(b)
. c = :: thing Thing => thing
. g = c as Shown
-- g(Thing("two")).label()

━━━━━━━━━━━━━━━━━━━━
+ Taker is () String
    - thing Thing

━━━━━━━━━━━━━━━━━━━━
+ Shown is () Label
    - thing Thing

━━━━━━━━━━━━━━━━━━━━
+ Lower is () String
    - derived Derived

━━━━━━━━━━━━━━━━━━━━
+ Label is Interface

────────────────────
+ label String

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Label

+ name = ""

────────────────────
+ This
    - name

────────────────────
+ label String

"thing $name"

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ kind String

"base"

━━━━━━━━━━━━━━━━━━━━
+ Derived is Base

────────────────────
+ kind String

"derived"

━━━━━━━━━━━━━━━━━━━━
+ run
    - fn Lower

fn(Derived())
```

```output
thing one
derived
thing two
```

### A closure with a typed argument, written where a function takes a function type, panics.

```yumi crash
. shown = run • fn :: n Int => do
    . items = [1, 2, 3]
    = items[n]
-- shown

━━━━━━━━━━━━━━━━━━━━
+ Pick is () Int
    - n I32

━━━━━━━━━━━━━━━━━━━━
+ run
    - fn Pick

fn(1) + fn(9)
```

```stderr
Unhandled panic: List index 9 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:shown no file:4:7
  2  __source__:shownCasting no file:2:14
  3  __source__:run no file:15:9
  4  __source__:main no file:2:11
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### A closure and a function that take an object are given where a function type gives a number the object's class is made from.

```yumi
. a = :: thing Thing => thing.n
. f = a as Taker
-- f(4)
-- run(show)

━━━━━━━━━━━━━━━━━━━━
+ Taker is () Int
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ show
    - thing Thing

thing.n + 2

━━━━━━━━━━━━━━━━━━━━
+ run
    - fn Taker

fn(7)

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ n = 0

────────────────────
+ do from n Int

. made = Thing()
made.n = n * 10
= made

────────────────────
+ to Int

n + 1

────────────────────
+ !this

n == 0
```

```output
40
72
```

### A closure that takes a text is given a number by a function type, and one that answers a number is cast to a type that answers text.

```yumi
. a = :: text String => text.length
. f = a as Count
-- f(42)
. b = :: n Int => n
. g = b as Text
-- g(7)

━━━━━━━━━━━━━━━━━━━━
+ Count is () Int
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ Text is () String
    - n Int
```

```output
2
7
```

### A closure that takes a number is cast to a function type that gives it text.

```yumi
. a = :: n Int => n + 1
. f = a as Texty
-- f("0")

━━━━━━━━━━━━━━━━━━━━
+ Texty is () Int
    - n String
```

```output
1
```

### A closure with a typed argument panics, called as a value of a function type that gives it another number: the stack trace has the closure and the function made for the value.

```yumi crash
. items = [1, 2, 3]
. picker = :: n Int => items[n]
. f = picker as Pick
-- f(1.9)
-- f(7.5)

━━━━━━━━━━━━━━━━━━━━
+ Pick is () Float
    - n Float
```

```stderr
Unhandled panic: List index 7 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:picker no file:3:24
  2  __source__:pickerCasting no file:4:7
  3  __source__:main no file:6:4
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A closure with a typed argument added to a list of a function type panics, called in a loop.

```yumi crash
. chosen [Pick]
. items = [1, 2, 3]
chosen += :: n Int => items[n]
for fn in chosen
    -- fn(0)
    -- fn(5)

━━━━━━━━━━━━━━━━━━━━
+ Pick is () Int
    - n I32
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:chosen no file:4:23
  2  __source__:chosenCasting no file:5:11
  3  __source__:main no file:7:8
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A value of a function type is printed as the name of its type

```yumi
. gen = numbers(3)
-- gen
-- "made $gen"
. words = texts()
-- words
. adder Mapper'Int'Int = :: value => value + 1
-- adder
. getter Getter'String = => "x"
-- getter
-- "held $getter"

━━━━━━━━━━━━━━━━━━━━
+ do numbers
    - a Int

<= a

━━━━━━━━━━━━━━━━━━━━
+ do texts Generator'String

<= "t"
```

```output
[Function Generator'I64]
made [Function Generator'I64]
[Function Generator'String]
[Function Mapper'I64'I64]
[Function Getter'String]
held [Function Getter'String]
```

### Nested functions in the values of two lazy properties read the object they were made for.

```yumi
. t = Thing()
. f = t.get-a
. g = t.get-b
-- f()
-- g()
t.a = 3
-- f() + g()
. h = :: n Int => n * 2
-- h(4)

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 5
+ b = 7

+ get-a Getter'Int := => a
+ get-b Getter'Int := => b + a
```

```output
5
12
13
8
```

### A nested function in a lazy property's value, inherited by a subclass.

```yumi
. s = Sub()
. f = s.get-a
-- f()
s.a = 9
-- f()

━━━━━━━━━━━━━━━━━━━━
+ Base

+ a = 1

+ get-a Getter'Int := => a * 10

━━━━━━━━━━━━━━━━━━━━
+ Sub is Base
```

```output
10
90
```

### A nested function in a lazy property's value of a generic class.

```yumi
. b = Box'Int(4)
. r = b.reader
-- r()
. s = Box'String("x")
. q = s.reader
-- q()

━━━━━━━━━━━━━━━━━━━━
+ Box'T

+ value T

+ reader Getter'T := => value

────────────────────
+ This
    - value T
```

```output
4
x
```

### A lazy property whose value holds a closure reads its fields by their bare names.

```yumi
. t = Thing()
-- t.total
-- t.total
. u = Thing()
u.n = 10
-- u.total

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ k = 3
+ n = 4

+ total Int := n + apply(=> k)

━━━━━━━━━━━━━━━━━━━━
+ do apply Int
    - f Getter'Int

= f()
```

```output
7
7
13
```

### Closures made in a loop read the names match patterns bind in the turn they are made in.

```yumi
. items = [Item.num(1), Item.word("ab"), Item.pair((3, "c"))]
. fs = List'Doer()
. total = 0
for item in items
    do match item
        | .num(n)
            fs += => do
                total += n
                -- "num $n"
        | .word(w)
            fs += => do
                total += w.length
                -- "word $w"
        | .pair((a, b))
            fs += => do
                total += a
                -- "pair $a $b"
for f in fs
    f()
-- total

━━━━━━━━━━━━━━━━━━━━
+ Item is Union

| num Int
| word String
| pair (Int, String)
```

```output
num 1
word ab
pair 3 c
6
```

### A closure picks a function-typed overload over one whose parameter is a bare type variable.

```yumi
-- pick(=> 1 + 1)
-- pick(3)
-- pick("s")

━━━━━━━━━━━━━━━━━━━━
+ pick'T
    - value T

"eager"

━━━━━━━━━━━━━━━━━━━━
+ pick'T
    - value Getter'T

"lazy ${value()}"
```

```output
lazy 2
eager
eager
```

### A lazy expectation runs its closure and catches the error it raises.

```yumi
. t = Checks()
t.expect("infallible", 2, => 1 + 1)
t.expect("fallible", 2, => half(4))
t.expect("failing", 2, => half(3))
t.expect("eager", 2, 2)
-- "${t.passed} passed, ${t.failed} failed, ${t.caught} caught"

━━━━━━━━━━━━━━━━━━━━
+ Checks

+ passed = 0
+ failed = 0
+ caught = 0

────────────────────
+ do expect'Expecting'Actual
    - label String
    - expected Expecting
    - actual Actual

-- "Assertion $label is eager"
passed += 1

= this

────────────────────
+ do expect'Expecting'Actual
    - label String
    - expected Expecting
    - actual Getter'Actual

. fn = actual
. actual-result = catch? (fn() as Expecting)
if actual-result
    if actual-result == expected
        passed += 1
    else
        failed += 1
else
    -- "Assertion $label caught"
    caught += 1

= this

━━━━━━━━━━━━━━━━━━━━
+ OddNumber is Error

━━━━━━━━━━━━━━━━━━━━
+ half Int ?! OddNumber
    - value Int

?! OddNumber("odd")
    if value % 2 != 0

value / 2
```

```output
Assertion failing caught
Assertion eager is eager
3 passed, 0 failed, 1 caught
```

### A nested function in a method of a generic class names the class's type parameter.

```yumi
. b = Holder'Int(5)
-- b.listed().length
-- b.cast()

━━━━━━━━━━━━━━━━━━━━
+ Holder'T

+ value T

────────────────────
+ This
    - value

────────────────────
+ do listed [T]

. g = => List'T([value, value])
= g()

────────────────────
+ do cast T

. f = => value as T
= f()
```

```output
2
5
```

### A lambda whose body is optional where its function type answers a value unwraps the body's value with `!`.

```yumi
-- ordered(3, 1)
-- compared(1, 3) • using :: a, b => (if a < b then .lesser else .greater if a > b)!

━━━━━━━━━━━━━━━━━━━━
_ do ordered Comparison
    - left Int
    - right Int

. maybe Comparison? = .greater
= compared(left, right) • using :: a, b => maybe!

━━━━━━━━━━━━━━━━━━━━
_ compared Comparison
    - left Int
    - right Int
    - using Comparer'Int

using(left, right)
```

```output
greater
lesser
```

### The unwrap of a lambda's optional result panics where the body holds no value.

```yumi crash
-- compared(1, 1) • using :: a, b => (.greater if a > b)!

━━━━━━━━━━━━━━━━━━━━
_ compared Comparison
    - left Int
    - right Int
    - using Comparer'Int

using(left, right)
```

```stderr
Unhandled panic: Optional'Comparison is null
<no file:2>

Task 1

Stack trace:
  0  __source__:anon no file:2:38
  1  __source__:anonCasting no file:2:18
  2  __source__:compared no file:10:1
  3  __source__:main no file:2:4
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A constructor argument named as an optional property of a function type takes a lambda and a method.

```yumi
. counter = Counter()
-- Holder(counter.next).run()
-- Holder(=> 7).run()

━━━━━━━━━━━━━━━━━━━━
+ Maker is () Int

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ count = 0

────────────────────
+ do next Int

count += 1
= count * 10

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ maker Maker?

────────────────────
+ This
    - maker

────────────────────
+ do run Int

= if maker then maker() else 0
```

```output
10
7
```

### A closure that raises an error its function type declares binds the type's return to what it gives.

```yumi
. got = first(:: n => half(n))
-- got + 1
. listed = gather(:: n => half(n))
-- listed[0] + 1

━━━━━━━━━━━━━━━━━━━━
+ Odd is Error

━━━━━━━━━━━━━━━━━━━━
+ do half Int ?! Odd
    - n Int

?! Odd("odd")
    if n % 2 == 1
= n / 2

━━━━━━━━━━━━━━━━━━━━
+ do first'Item Item ?! Odd
    - decode () Item ?! Odd
        - n Int

= decode(4)

━━━━━━━━━━━━━━━━━━━━
+ do gather'Item [Item] ?! Odd
    - decode () Item ?! Odd
        - n Int

. found [Item]
found += decode(6)
= found
```

```output
3
4
```

### An index assignment whose setter answers the new value stores it in a captured local or argument, from a closure or task that captures it or from the function that declares it.

```yumi
/core/bitmap

. bits = Bitmap'U8()
. read = => bits[1]
. change = => do
    bits[2] = true
change()
bits[1] = true
-- "${read()} ${bits[2]}"
flip(Bitmap'U8())

━━━━━━━━━━━━━━━━━━━━
+ do flip
    - bits Bitmap'U8

. run = do new
    bits[3] = true
    -- bits[3]
run.wait()
```

```output
true true
true
```
