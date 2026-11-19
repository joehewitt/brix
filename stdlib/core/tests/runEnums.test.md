# Enums

```brix
(main
  (print (. Foo bar))
  (print (. Foo baz))
  (print (== (as 0 Foo) (. Foo bar)))
  (print (== (as 1 Foo) (. Foo baz)))
  (print (== (as 0 Foo) (. Foo baz)))
)

(type + Foo
  (variant bar)
  (variant baz)
  (blank)
  ; -----
  ; + to String
  (blank)
  ; "Foo $number"
)
```

```output
[Object __source__:Foo]
[Object __source__:Foo]
true
true
false
```

```brix
(main
  (print (. Foo bar))
  (print (. Foo baz))
  (print (as 0 Foo))
)

(type + Foo (is (tuple (: a Int) (: b String)))
  (variant bar (= (tuple 1 "BAR")))
  (variant baz (= (tuple 2 "BAZ")))
  (fn-to-expr + String
    b
  )
)
```

```output
BAR
BAZ
BAR
```

```brix
(main
  (let value Foo (. Foo baz))
  (print
    (match value
      (case= (. Foo bar)
        "bar"
      )
      (case= (. Foo baz)
        "baz"
      )
      (default
        "unknown"
      )
    )
  )
)

(type + Foo (is (tuple (: a Int) (: b String)))
  (variant bar (= (tuple 1 "BAR")))
  (variant baz (= (tuple 2 "BAZ")))
  (fn-to-expr + String
    b
  )
)
```

```output
baz
```

```brix
(main
  (print (. Foo bar))
  (print (. Foo baz))
)

(type + Foo (is Int)
  (variant bar)
  (variant baz)
)
```

```output
0
1
```

```brix
(main
  (let value Foo (. Foo baz))
  (print
    (match value
      (case= .bar
        "bar"
      )
      (case= .baz
        "baz"
      )
      (case= .qux
        "qux"
      )
      (default
        "unknown"
      )
    )
  )
  (print
    (match (. Foo qux)
      (case= (. Foo bar)
        "bar"
      )
      (case= (. Foo baz)
        "baz"
      )
      (case= (. Foo qux)
        "qux"
      )
      (default
        "unknown"
      )
    )
  )
)

(type + Foo (is Int)
  (variant bar)
  (variant baz)
  (variant qux)
)
```

```output
baz
qux
```

```brix crash
(main
  (print
    (match (. Foo bar)
      (case .bar
        "union case"
      )
      (default
        "fallback"
      )
    )
  )
)

(type + Foo (is U8)
  (variant bar)
  (variant baz)
)
```

```output
Exception: Foo.bar is an enum value, not a union case; match it with "(case= .bar …)"
<no file:5>
          (case .bar
                ^^^^
```

```brix
(main
  (print (. Foo bar))
  (print (. Foo baz))
)

(type + Foo (is Float)
  (variant bar)
  (variant baz)
)
```

```output
0
1
```

```brix
(main
  (print (. Foo bar))
  (print (as 0 Foo))
  (print (== (as 0 Foo) (. Foo bar)))
)

(type + Foo
  (variant bar (= "hello"))
  (blank)
  (prop + name String)
  (fn-do + This _
    (param name String)
  )
  (fn-to-expr + String
    (str "[Foo " name "]")
  )
)
```

```output
[Foo hello]
[Foo hello]
true
```

```brix crash
(main
  (print (. Foo bar))
)

(type + Foo
  (variant bar (= "hello"))
)
```

```output
Exception: Expected Foo here, but this is String, and there is no conversion between them
<no file:7>
      (variant bar (= "hello"))
                      ^^^^^^^
```

### U8 enum values match with value patterns, including inside the enum's own methods.

```brix
(main
  (for (in kind (list (. Kind first) (. Kind second) (. Kind third)))
    (print (. kind label))
    (print (call describe kind))
  )
)

(fn-expr + describe String
  (param kind Kind)
  (match kind
    (case= .first
      "one"
    )
    (case= (. Kind second)
      "two"
    )
    (default
      "other"
    )
  )
)

(type + Kind (is U8)
  (variant first)
  (variant second)
  (variant third)
  (fn-expr + label _ :get
    (match this
      (case= .first
        "one"
      )
      (case= .second
        "two"
      )
      (default
        "other"
      )
    )
  )
)
```

```output
one
one
two
two
other
other
```

### Values of enums of a number, a tuple and a class, named, compared and matched

```brix
(main
  (print (. Level low))
  (print (. Level high))
  (print (== (. Level low) (. Level low)))
  (print (== (. Level low) (. Level high)))
  (print (call rank .low))
  (print (call rank .mid))
  (print (call rank (. Level high)))
  (let picked Level .mid)
  (print picked)
  (print (. Tone warm))
  (print (. Tone cool name))
  (print (== (. Tone warm) (. Tone cool)))
  (print (. Mark plain label))
  (print (. Mark bold label))
  (print (== (. Mark plain) (. Mark plain)))
  (print (== (. Mark plain) (. Mark bold)))
)

(fn-expr + rank Int
  (param level Level)
  (match level
    (case= .low
      1
    )
    (case= (. Level mid)
      2
    )
    (default
      3
    )
  )
)

(type + Level (is U8)
  (variant low)
  (variant mid)
  (variant high)
)

(type + Tone (is (tuple (: heat Int) (: name String)))
  (variant warm (= (tuple 30 "warm")))
  (variant cool (= (tuple 5 "cool")))
  (fn-to-expr + String
    (str "tone " name " " heat)
  )
)

(type + Mark
  (variant plain (= "p"))
  (variant bold (= "b"))
  (blank)
  (prop + label String)
  (fn-do + This _
    (param label String)
  )
)
```

```output
0
2
true
false
1
2
3
1
tone warm 30
cool
false
p
b
true
false
```

### Number converted to an enum of a class by its place

```brix
(main
  (print (as 0 Foo))
  (print (as 1 Foo))
  (print (as 2 Foo))
  (print (as -1 Foo))
  (let n 1)
  (print (as n Foo))
  (let wide I64 0)
  (print (as wide Foo))
  (let small U8 1)
  (print (as small Foo))
  (print (. (as 5 Foo) name))
  (print (== (as 0 Foo) (. Foo bar)))
  (print (call take (as 0 Foo)))
  (print (call take (as 7 Foo)))
)

(fn-expr + take String
  (param foo Foo)
  (str "took " (. foo name))
)

(type + Foo
  (variant bar (= "hello"))
  (variant baz (= "there"))
  (blank)
  (prop + name String (= "made"))
  (fn-do + This _
    (param name String (= "none"))
  )
  (fn-to-expr + String
    (str "[Foo " name "]")
  )
)
```

```output
[Foo hello]
[Foo there]
[Foo none]
[Foo none]
[Foo there]
[Foo hello]
[Foo there]
none
true
took hello
took none
```

### A number becomes an enum of a class only when asked

```brix compile-error
(main
  (let place 1)
  (let chosen Foo place)
)

(type + Foo
  (variant bar (= "hello"))
  (variant baz (= "there"))
)
```

```output
Exception: Expected Foo here, but this is I64, which becomes Foo only when asked; write "(as place Foo)"
<no file:4>
      (let chosen Foo place)
                  ^^^
```

### Number converted to an enum of a tuple by its place

```brix
(main
  (print (as 0 Pair))
  (print (as 1 Pair))
  (print (as 3 Pair))
  (print (. (as 1 Pair) b))
  (print (. (as 4 Pair) a))
)

(type + Pair (is (tuple (: a Int) (: b String)))
  (variant one (= (tuple 1 "ONE")))
  (variant two (= (tuple 2 "TWO")))
  (fn-to-expr + String
    (str a " " b)
  )
)
```

```output
1 ONE
2 TWO
0 
TWO
0
```

### Number past the values of an enum constructs it with nothing

```brix
(main
  (print (as 0 Foo))
  (print (as 2 Foo))
)

(type + Foo
  (variant bar (= "hello"))
  (variant baz (= "there"))
  (blank)
  (prop + name String)
  (fn-do + This _
    (param name String)
  )
  (fn-to-expr + String
    (str "[Foo " name "]")
  )
)
```

```output
[Foo hello]
[Foo ]
```

### Number converted to an enum whose values are never named

```brix
(main
  (print (as 0 Foo))
  (print (as 2 Foo))
)

(type + Foo
  (variant bar)
  (variant baz)
  (blank)
  (prop + name String (= "made"))
  (fn-to-expr + String
    (str "[Foo " name "]")
  )
)
```

```output
[Foo made]
[Foo made]
```

### A number an argument's call gives does not become an enum unasked.

```brix crash
(main
  (print (call take (call compute)))
)

(fn-do + compute Int
  (print "compute")
  (return 0)
)

(fn-expr + take String
  (param foo Foo)
  (str "took " (. foo name))
)

(type + Foo
  (variant bar (= "hello"))
  (variant baz (= "there"))
  (blank)
  (prop + name String)
  (fn-do + This _
    (param name String (= "none"))
  )
)
```

```output
Exception: No function "take" accepts (I64); it is declared as "take(foo Foo)"
<no file:3>
      (print (call take (call compute)))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A number converted to an enum with as where returned, declared and compared

```brix
(main
  (print (. (call pick 0) name))
  (print (. (call pick 1) name))
  (print (. (call pick 5) name))
  (print (. (call choose true) name))
  (print (. (call choose false) name))
  (print (call each))
  (let made (as (call next 0) Foo))
  (print (. made name))
  (let later (as (call next 1) Foo))
  (print (. later name))
  (print (. (as (call next 4) Foo) name))
  (let same (== (as 1 Foo) (. Foo baz)))
  (print same)
  (print (== (as 1 Foo) (as 1 Foo)))
  (print (== (as 9 Foo) (as 9 Foo)))
)

(fn-do + next Int
  (param from Int)
  (print (str "next " from))
  (return (+ from 1))
)

(fn-expr + pick Foo
  (param n Int)
  (as n Foo)
)

(fn-do + choose Foo
  (param first Bool)
  (if first
    (return (as 0 Foo))
  )
  (return (as 1 Foo))
)

(fn-do + each String
  (let text "")
  (let n 0)
  (while (< n 4)
    (let foo (as n Foo))
    (= text (++ text (. foo name) " "))
    (+= n 1)
  )
  (return text)
)

(type + Foo
  (variant bar (= "hello"))
  (variant baz (= "there"))
  (blank)
  (prop + name String)
  (fn-do + This _
    (param name String (= "none"))
  )
)
```

```output
hello
there
none
hello
there
hello there none none 
next 0
there
next 1
none
next 4
none
true
true
false
```

### Match over some values of a class is open

```brix
(main
  (let level (. Level low))
  (print
    (match level
      (case= .low
        "low"
      )
    )
  )
)

(type + Level
  (variant low)
  (variant middle)
  (variant high)
)
```

```output
low
```

### Match over the values of a class by equality

```brix
(main
  (let level (. Level low))
  (print
    (match level
      (case .low
        "low"
      )
      (case= .middle
        "middle"
      )
      (case= .high
        "high"
      )
      (case= .low
        "really low"
      )
    )
  )
)

(type + Level
  (variant low)
  (variant middle)
  (variant high)
)
```

```output
really low
```

### Match over a class with values that no arm takes

```brix crash
(main
  (let level (. Level low))
  (print
    (match level
      (case .low
        "low"
      )
      (case= .middle
        "middle"
      )
    )
  )
)

(type + Level
  (variant low)
  (variant middle)
)
```

### Match over a class with values falls through

```brix crash
(main
  (print (call name (. Level low)))
  (print (call name (. Level middle)))
  (print (call name (. Level high)))
)

(fn-expr + name String
  (param level Level)
  (match level
    (case= .low
      "low"
    )
    (case= .middle
      "middle"
    )
  )
)

(type + Level
  (variant low)
  (variant middle)
  (variant high)
)
```

```output
low
middle
```

### Arm naming a value of a number type as a case

```brix crash
(main
  (let level (. Level low))
  (print
    (match level
      (case .low
        "low"
      )
      (case= .middle
        "middle"
      )
    )
  )
)

(type + Level (is Int)
  (variant low (= 1))
  (variant middle (= 2))
)
```

```output
Exception: Level.low is an enum value, not a union case; match it with "(case= .low …)"
<no file:6>
          (case .low
                ^^^^
```

### Match over some values of a number type is closed

```brix crash
(main
  (print (call name (. Level low)))
  (print (call name (. Level high)))
)

(fn-expr + name String
  (param level Level)
  (match level
    (case= .low
      "low"
    )
    (case= .middle
      "middle"
    )
  )
)

(type + Level (is Int)
  (variant low (= 1))
  (variant middle (= 2))
  (variant high (= 3))
)
```

```output
Exception: This match on Level does not handle Level.high; add "(case= (. Level high) …)", or "(default …)" for the rest
<no file:10>
        (case= .low
        ^^^^^^^^^^
```

### A case of an enum may hold a value as large as the enum's type holds.

```brix
(main
  (let f (. Fill fill-empty))
  (print (str "f " (as f U32)))
  (print (str "v " (as (. Fill fill-variadic) U32)))
  (let g Fill .fill-variadic)
  (print (str "same " (== g (. Fill fill-variadic)) " " (== f g)))
)

(type + Fill (is U32)
  (variant fill-empty (= 4294967295))
  (variant fill-variadic (= 4294967294))
)
```

```output
f 4294967295
v 4294967294
same true false
```

### The tuple written for a case of a tuple enum fills its fields in order.

```brix
(main
  (let day (. Day monday))
  (print (str "[" (. day name) "] [" (. day short-name) "]"))
  (print (str "[" (. Day sunday name) "]"))
)

(type + Day (is Tuple)
  (variant sunday (= (tuple "sunday" "sun")))
  (variant monday (= (tuple "monday" "mon")))
  (blank)
  (prop + name _ (= ""))
  (prop + short-name _ (= ""))
)
```

```output
[monday] [mon]
[sunday]
```

### A case named by its symbol alone, given for a parameter whose enum has the case, is that enum's case, though an enum imported earlier has a case of the name too.

```brix
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (print (as (call describe .stated) U32))
  (print (as (call describe .stated "named") U32))
)

(fn-do _ describe Far
  (param kind Far)
  (param label (? String) (= null))
  (return kind)
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
20
20
```

### A choice between cases named by their symbols alone, given for a parameter whose enum has both cases, is a choice of that enum's cases, though an enum the module declares has cases of the names too.

```brix
(import ./lib/far)
(blank)

(main
  (print (as (call pick true) U32))
  (print (as (call pick false) U32))
)

(type _ Near (is U32)
  (variant first (= 3))
  (variant stated (= 7))
)

(fn-do _ pick Far
  (param first Bool)
  (return (call describe (if-expr first .first .stated)))
)

(fn-do _ describe Far
  (param kind Far)
  (return kind)
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
0
20
```

### A case named by its symbol alone that two imported enums have, on the left of an operator that only one of them declares for the other operand, is that enum's case, though the other's case is the one its name alone gives.

```brix
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (print (+ .stated "!"))
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is Union)
  (variant first)
  (variant stated)
  (fn-expr + (infix this + (param mark String)) _
    (str "far " this mark)
  )
)
```

```output
far stated!
```

### A case named by its symbol alone that two imported enums have is of neither in a local declared without a type, which casts it to nothing.

```brix crash
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (let kind .stated)
  (print (as kind U32))
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
Exception: Case '.stated' is ambiguous between 'Far', 'Near'
<no file:7>
      (let kind .stated)
                ^^^^^^^
```

### A case named by its symbol alone is of neither enum that has it in a local declared with a type that has no such case, though the case converts to it.

```brix crash
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (let kind U32 .stated)
  (print kind)
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
Exception: Case '.stated' is ambiguous between 'Far', 'Near'
<no file:7>
      (let kind U32 .stated)
                    ^^^^^^^
```

### A case named by its symbol alone is of neither enum that has it where it is assigned to a local of a type that has no such case.

```brix crash
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (let kind U32 0)
  (= kind .stated)
  (print kind)
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
Exception: Case '.stated' is ambiguous between 'Far', 'Near'
<no file:8>
      (= kind .stated)
              ^^^^^^^
```

### A case named by its symbol alone is of neither enum that has it as the default of a property of a type that has no such case.

```brix crash
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (print (. (call Box) kind))
)

(type + Box
  (prop + kind U32 (= .stated))
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
Exception: Case '.stated' is ambiguous between 'Far', 'Near'
<no file:11>
      (prop + kind U32 (= .stated))
                          ^^^^^^^
```

### A case named by its symbol alone is of neither enum that has it where a function returns it as a type that has no such case.

```brix crash
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (print (call pick))
)

(fn-do _ pick U32
  (return .stated)
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
Exception: Case '.stated' is ambiguous between 'Far', 'Near'
<no file:11>
      (return .stated)
              ^^^^^^^
```

### The case named by its symbol alone that a function written without do gives is the case of the enum it returns, though an enum imported earlier has a case of the name too.

```brix
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (print (as (call pick) U32))
)

(fn-expr _ pick Far
  .stated
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
20
```

### The cases an `if` gives by their symbols alone are of the type expected of it, though another union has a case of the name too: as the value of a function written without do, and as the value of an arm of a match.

```brix
(main
  (for (in seed (..< 0 3))
    (print (call given seed))
    (print (call armed seed))
  )
)

(type + Cap (is Union)
  (variant butt)
  (variant round)
  (variant square)
)

(type + Join (is Union)
  (variant miter)
  (variant round)
  (variant bevel)
)

(fn-expr _ given Cap
  (param seed Int)
  (if-expr (== seed 0) .round (if-expr (== seed 1) .square .butt))
)

(fn-expr _ armed Cap
  (param seed Int)
  (case? (== seed 0)
    .butt
  )
  (default
    (if-expr (== seed 1) .round .square)
  )
)
```

```output
round
butt
square
round
butt
square
```

### A case named by its symbol alone that another union has too, written alone in a block of its own below a declaration, an assignment, a property or a return, is the case of the type each states.

```brix
(main
  (let cap Cap
    (block
      .round
    )
  )
  (print cap)
  (let join Join .miter)
  (= join
    (block
      .round
    )
  )
  (print join)
  (print (. (call Brush) cap))
  (print (call ending))
)

(type + Cap (is Union)
  (variant butt)
  (variant round)
)

(type + Join (is Union)
  (variant miter)
  (variant round)
)

(type + Brush
  (prop + cap Cap
    (=
      (block
        .round
      )
    )
  )
)

(fn-do + ending Cap
  (return
    (block
      .round
    )
  )
)
```

```output
round
round
round
round
```

### A case named by its symbol alone is of neither enum that has it where a function written without do gives it as a type that has no such case.

```brix crash
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (print (call pick))
)

(fn-expr _ pick U32
  .stated
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
Exception: Case '.stated' is ambiguous between 'Far', 'Near'
<no file:11>
      .stated
      ^^^^^^^
```

### A case named by its symbol alone cast with `as` to an enum that has the case is that enum's case, though an enum imported earlier has a case of the name too.

```brix
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (print (as .stated Far U32))
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
20
```

### A case named by its symbol alone cast with `as` to a type that has no such case is of neither enum that has it, though only an explicit cast converts it.

```brix crash
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (print (as .stated U8))
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
Exception: Case '.stated' is ambiguous between 'Far', 'Near'
<no file:7>
      (print (as .stated U8))
                 ^^^^^^^
```

### A case named by its symbol alone is of neither enum that has it where it is given for a parameter of a type that has no such case.

```brix crash
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (print (call show .stated))
)

(fn-expr _ show U32
  (param value U32)
  value
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
Exception: Case '.stated' is ambiguous between 'Far', 'Near'
<no file:7>
      (print (call show .stated))
                        ^^^^^^^
```

### A case named by its symbol alone is of neither enum that has it where it is given for what a union's case holds, of a type that has no such case.

```brix crash
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (print (type-of (call (. Holder held) .stated)))
)

(type + Holder (is Union)
  (variant held (tuple (: count U32)))
  (variant empty)
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
Exception: Case '.stated' is ambiguous between 'Far', 'Near'
<no file:7>
      (print (type-of (call (. Holder held) .stated)))
                                            ^^^^^^^
```

### A case named by its symbol alone is of neither enum that has it as a field of a tuple, which casts it to nothing.

```brix crash
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (let pair (tuple .stated 1))
  (print pair)
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
Exception: Case '.stated' is ambiguous between 'Far', 'Near'
<no file:7>
      (let pair (tuple .stated 1))
                       ^^^^^^^
```

### A case named by its symbol alone compared with a value of a type that has no such case is of neither enum that has it.

```brix crash
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (let count U32 7)
  (print (== count .stated))
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
Exception: Case '.stated' is ambiguous between 'Far', 'Near'
<no file:8>
      (print (== count .stated))
                       ^^^^^^^
```

### A case named by its symbol alone that two imported number enums have is printed and made text by its enum's own method, which settles it as the case of the enum its name alone gives.

```brix
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (print .stated)
  (print (str "[" .stated "]"))
)
```

```brix file=lib/near.bx
(type + Near (is U32)
  (variant first (= 0))
  (variant stated (= 7))
)
```

```brix file=lib/far.bx
(type + Far (is U32)
  (variant first (= 0))
  (variant stated (= 20))
)
```

```output
7
[7]
```

### A case named by its symbol alone that two imported unions have, made text where the union has no method for it, is of neither.

```brix crash
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (print (str "[" .stated "]"))
)
```

```brix file=lib/near.bx
(type + Near (is Union)
  (variant first)
  (variant stated)
)
```

```brix file=lib/far.bx
(type + Far (is Union)
  (variant first)
  (variant stated)
)
```

```output
Exception: Case '.stated' is ambiguous between 'Far', 'Near'
<no file:7>
      (print (str "[" .stated "]"))
                      ^^^^^^^
```

### The type of a case named by its symbol alone that two imported classes have is read from the object, so the case is of neither.

```brix crash
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (print (type-of .stated))
)
```

```brix file=lib/base.bx
(type + Base
  (prop + name _ (= ""))
)
```

```brix file=lib/near.bx
(import ./base)

(type + Near (is Base)
  (variant first)
  (variant stated)
)
```

```brix file=lib/far.bx
(import ./base)

(type + Far (is Base)
  (variant first)
  (variant stated)
)
```

```output
Exception: Case '.stated' is ambiguous between 'Far', 'Near'
<no file:7>
      (print (type-of .stated))
                      ^^^^^^^
```

### A case named by its symbol alone that two imported classes have, joined with another class's case, is cast to their base class, which has no such case, so it is of neither.

```brix crash
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (let chosen true)
  (let kind (if-expr chosen .stated (. Far first)))
  (print (. kind name))
)
```

```brix file=lib/base.bx
(type + Base
  (prop + name _ (= ""))
)
```

```brix file=lib/near.bx
(import ./base)

(type + Near (is Base)
  (variant first)
  (variant stated)
)
```

```brix file=lib/far.bx
(import ./base)

(type + Far (is Base)
  (variant first)
  (variant stated)
)
```

```output
Exception: Case '.stated' is ambiguous between 'Far', 'Near'
<no file:8>
      (let kind (if-expr chosen .stated (. Far first)))
                                ^^^^^^^
```

### A case named by its symbol alone that two imported unions have, given where an optional is empty, joins the optional's union with its own, so it is of neither.

```brix crash
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (let held (? Far) null)
  (print (type-of (?? held .stated)))
)
```

```brix file=lib/near.bx
(type + Near (is Union)
  (variant first)
  (variant stated)
)
```

```brix file=lib/far.bx
(type + Far (is Union)
  (variant first)
  (variant stated)
)
```

```output
Exception: Case '.stated' is ambiguous between 'Far', 'Near'
<no file:8>
      (print (type-of (?? held .stated)))
                               ^^^^^^^
```

### A function with no return type that returns cases by their symbols alone returns the union that has every one of them, though its first case is also another union's.

```brix
(import /core)
(blank)

(main
  (for (in seed (..< 0 3))
    (print (call describe (call cut seed)))
  )
)

(type + Edge (is Union)
  (variant sharp)
  (variant soft)
)

(type + Blade (is Union)
  (variant sharp)
  (variant dull)
)

(fn-expr + describe String
  (param edge Edge)
  (if-expr (== edge .soft) "soft" "sharp")
)

(fn-do + cut _
  (param seed Int)
  (if (== seed 1)
    (return .sharp)
  )
  (return .soft)
)
```

```output
soft
sharp
soft
```

### A case named by its symbol alone that no enum the module imports has is the case of the type expected of it: of the other side of a comparison, a parameter, or a property stored to.

```brix
(import ./lib/rows)
(blank)

(main
  (let row (call make-row))
  (print (== (. row kind) .second))
  (print (== .second (. row kind)))
  (print (call describe .second))
  (let box (call Box))
  (= (. box kind) .second)
  (print (call describe (. box kind)))
)
```

```brix file=lib/rows.bx
(import ./kinds)

(fn-do + make-row Row
  (let row Row)
  (= (. row kind) (. Kind second))
  (return row)
)

(fn-do + describe String
  (param kind Kind)
  (return (if-expr (== kind (. Kind first)) "first" "second"))
)

(type + Box
  (prop + kind _ (= (. Kind first)))
)
```

```brix file=lib/kinds.bx
(type + Kind (is U32)
  (variant first (= 0))
  (variant second (= 1))
)

(type + Row (is Struct)
  (prop + kind Kind (= .first))
)
```

```output
true
true
second
second
```

### A case named by its symbol alone that no enum the module imports has is not found where nothing expected of it is a type with the case.

```brix crash
(import ./lib/rows)
(blank)

(main
  (let kind .second)
  (print (as kind U32))
)
```

```brix file=lib/rows.bx
(import ./kinds)

(fn-do + make-row Row
  (let row Row)
  (= (. row kind) (. Kind second))
  (return row)
)

(fn-do + describe String
  (param kind Kind)
  (return (if-expr (== kind (. Kind first)) "first" "second"))
)

(type + Box
  (prop + kind _ (= (. Kind first)))
)
```

```brix file=lib/kinds.bx
(type + Kind (is U32)
  (variant first (= 0))
  (variant second (= 1))
)

(type + Row (is Struct)
  (prop + kind Kind (= .first))
)
```

```output
Exception: Case ".second" belongs to Kind, which this module does not import, so the case cannot be named alone; write "Kind.second" and add the import ./lib/kinds
<no file:6>
      (let kind .second)
                ^^^^^^^
```

### A match that returns from every arm ends a function over an enum of a number

```brix
(main
  (print (call describe (. Kind first)))
  (print (call describe (. Kind second)))
)

(type + Kind (is U8)
  (variant first)
  (variant second)
)

(fn-do + describe String
  (param kind Kind)
  (match kind
    (case= .first
      (return "one")
    )
    (case= .second
      (return "two")
    )
  )
)
```

```output
one
two
```
