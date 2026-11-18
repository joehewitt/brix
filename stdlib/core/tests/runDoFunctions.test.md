# Do functions

### Expression functions return their final expression

```yumi
-- label(1)
-- label(2)

━━━━━━━━━━━━━━━━━━━━
+ label String
    - n Int

|? n == 1
    "one"
??
    "other"
```

```output
one
other
```

### Non-optional do returns accept an infinite loop whose reachable paths return.

```yumi
-- always()
-- always-boolean()

━━━━━━━━━━━━━━━━━━━━
+ do always String

while 1
    = "returned"

━━━━━━━━━━━━━━━━━━━━
+ do always-boolean String

while true
    = "boolean returned"
```

```output
returned
boolean returned
```

### Declared non-optional do returns cannot silently fall through to a null object.

```yumi crash
-- missing().value

━━━━━━━━━━━━━━━━━━━━
+ Item

+ value Int = 1

━━━━━━━━━━━━━━━━━━━━
+ do missing Item

Item()
```

```output
Exception: Function missing ends with a value it does not return; return it with "= value", or remove it
<no file:12>
    Item()
    ^^^^^^
```

### Expression functions reject explicit return

```yumi crash
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ foo
= 1
```

```output
Exception: Function "foo" is not declared with do, so its body is one expression and cannot return with "="; write "+ do foo"
<no file:6>
    = 1
    ^^^
```

### Expression functions reject return-if

```yumi crash
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ foo
=? 1
    if true
2
```

```output
Exception: Function "foo" is not declared with do, so its body is one expression and cannot return with "="; write "+ do foo"
<no file:6>
    =? 1
    ^^^
```

### Expression functions reject condition-only return-if

```yumi crash
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ foo
=? if true
2
```

```output
Exception: Function "foo" is not declared with do, so its body is one expression and cannot return with "="; write "+ do foo"
<no file:6>
    =? if true
    ^^^^^^^^^^
```

### Expression functions reject immediate declarations

```yumi crash
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ foo
. value = 1
value
```

```output
Exception: Function "foo" is not declared with do, so its body is one expression and cannot hold ". name = value" declarations; write "+ do foo" and return its value with "= value"
<no file:6>
    . value = 1
    ^^^^^^^^^^^
```

### Expression return-mode validation reaches helpers used by streamed constructors

```yumi crash
for item in items()
    -- item.value
    break

━━━━━━━━━━━━━━━━━━━━
+ Item

+ value String?

────────────────────
+ This

value = invalid-helper()

━━━━━━━━━━━━━━━━━━━━
_ invalid-helper

. result = "hello"
result

━━━━━━━━━━━━━━━━━━━━
+ do items <Item>

<= Item()
```

```output
Exception: Function "invalid-helper" is not declared with do, so its body is one expression and cannot hold ". name = value" declarations; write "_ do invalid-helper" and return its value with "= value"
<no file:19>
    . result = "hello"
    ^^^^^^^^^^^^^^^^^^
```

### Expression functions reject assignments outside a do block

```yumi crash
-- foo(1)

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n Int

n = 2
n
```

```output
Exception: Function "foo" is not declared with do, so its body is one expression and cannot hold assignments; put the assignment in a "do" block, or write "+ do foo" and return its value with "= value"
<no file:8>
    n = 2
    ^^^^^
```

### Expression functions run assignments in a do block before their value

```yumi
-- foo(1)

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n Int

do
    n = 2
n
```

```output
2
```

### Expression functions reject for loops

```yumi crash
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ foo

for n in 0 .. < 1
    -- n
0
```

```output
Exception: Function "foo" is not declared with do, so its body is one expression and cannot hold loops; write "+ do foo" and return its value with "= value"
<no file:7>
    for n in 0 .. < 1
       ^^^^^^^^^^^^^
```

### Expression functions reject while loops

```yumi crash
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ foo

while false
    -- "never"
0
```

```output
Exception: Function "foo" is not declared with do, so its body is one expression and cannot hold loops; write "+ do foo" and return its value with "= value"
<no file:7>
    while false
    ^^^^^^^^^^
```

### Expression functions reject if statements

```yumi crash
-- foo(3)

━━━━━━━━━━━━━━━━━━━━
+ foo Int
    - n Int

if n > 5
    -- "big"
n
```

```output
Exception: Function "foo" is not declared with do, so its body is one expression and cannot hold "if" statements; write a check as "?! error if condition", put other steps in a "do" block, or write "+ do foo Int" and return its value with "= value"
<no file:8>
    if n > 5
    ^^^^^^^
```

### Expression functions reject calls before their value

```yumi crash
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ foo Int

touch()
42

━━━━━━━━━━━━━━━━━━━━
+ do touch

-- "touched"
```

```output
Exception: Function "foo" is not declared with do, so its body is one expression and cannot run calls before its value; put the call in a "do" block, or write "+ do foo Int" and return its value with "= value"
<no file:7>
    touch()
    ^^^^^^^
```

### Expression functions name lazies after their value

```yumi crash
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ foo Int

.. base = 40
base + 2
```

```output
Exception: Function "foo" is not declared with do, so its body is one expression and cannot name lazies before its value; move the lazy below the value, or write "+ do foo Int" and return its value with "= value"
<no file:7>
    .. base = 40
    ^^^^^^^^^^^^
```

### Expression functions run checks, prints and do forms before their value

```yumi
-- foo(3)

━━━━━━━━━━━━━━━━━━━━
+ foo Int
    - n Int

?! Panic("big") if n > 5
do
    -- "first"
do finally
    -- "last"
-- "printed"
n
```

```output
first
printed
last
3
```

### Expression functions reject yield

```yumi crash
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ foo

<= 1
0
```

```output
Exception: Function "foo" is not declared with do, so its body is one expression and cannot yield; write "+ do foo" and return its value with "= value"
<no file:7>
    <= 1
    ^^^^
```

### Expression functions allow prep statements

```yumi
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ foo

?! Panic("bad")
    if false
do
    -- "prep"
do finally
    -- "cleanup"
-- "before"
42
```

```output
prep
before
cleanup
42
```

### Imperative do functions return explicitly

```yumi
-- choose(true)
-- choose(false)

━━━━━━━━━━━━━━━━━━━━
+ do choose
    - flag Bool

if flag
    = 1
= 2
```

```output
1
2
```

### Imperative do functions infer optional return on fallthrough

```yumi
-- maybe(true)
-- maybe(false)

━━━━━━━━━━━━━━━━━━━━
+ do maybe
    - flag Bool

if flag
    = 7
```

```output
7
unknown
```

### Do functions reject a value as their last step

```yumi crash
foo()
-- "done"

━━━━━━━━━━━━━━━━━━━━
+ do foo

42
```

```output
Exception: A value cannot stand alone as a step; use it, or remove it
<no file:8>
    42
    ^^
```

### Bool do functions reject an unreturned final value

```yumi crash
-- either(true, false)

━━━━━━━━━━━━━━━━━━━━
+ do either Bool
    - left Bool
    - right Bool

. together = left && right
=? true if together
left || right
```

```output
Exception: Function either ends with a value it does not return; return it with "= value", or remove it
<no file:11>
    left || right
    ^^^^^^^^^^^^^
```

### Optional do functions reject an unreturned final call

```yumi crash
-- first-even(4)

━━━━━━━━━━━━━━━━━━━━
+ do first-even Int?
    - n Int

=? null if n < 0
evens(n)

━━━━━━━━━━━━━━━━━━━━
+ evens Int
    - n Int

n * 2
```

```output
Exception: Function first-even ends with a value it does not return; return it with "= value", or remove it
<no file:9>
    evens(n)
    ^^^^^^^^
```

### Interface overrides of Bool methods reject an unreturned final value

```yumi crash
-- ready-of(Probe())

━━━━━━━━━━━━━━━━━━━━
+ Readiness is Interface

────────────────────
+ get is-ready Bool

━━━━━━━━━━━━━━━━━━━━
+ Probe
    is Readiness

+ ready = true

────────────────────
+ do get is-ready

ready

━━━━━━━━━━━━━━━━━━━━
+ ready-of
    - readiness Readiness

readiness.is-ready
```

```output
Exception: Function is-ready ends with a value it does not return; return it with "= value", or remove it
<no file:19>
    ready
    ^^^^^
```

### Class overrides of Bool methods reject an unreturned final value

```yumi crash
. check Checker = StrictChecker()
-- check.passes

━━━━━━━━━━━━━━━━━━━━
+ Checker

────────────────────
+ do get passes Bool

= false

━━━━━━━━━━━━━━━━━━━━
+ StrictChecker is Checker

+ strict = true

────────────────────
+ do get passes

strict
```

```output
Exception: Function passes ends with a value it does not return; return it with "= value", or remove it
<no file:21>
    strict
    ^^^^^^
```

### Inferred do returns reject an unreturned final value

```yumi crash
-- pick(true)

━━━━━━━━━━━━━━━━━━━━
+ do pick
    - flag Bool

if flag
    = 7
8
```

```output
Exception: Function pick ends with a value it does not return; return it with "= value", or remove it
<no file:10>
    8
    ^
```

### Bool and optional do functions fall through after final statements without values

```yumi
. counter = Counter()
-- counter.bump()
-- counter.last-even() ?? "none"
-- counter.count

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ count Int
+ label String?

────────────────────
+ do bump Bool

=? true if count > 3
count += 1

────────────────────
+ do last-even String?

=? label if count > 3
label = "odd"
count = count + 1
```

```output
false
none
2
```

### Empty imperative do functions are concrete no-op Void bodies

```yumi
empty-noop()
-- "done"

━━━━━━━━━━━━━━━━━━━━
+ do empty-noop
```

```output
done
```

### Empty non-do functions reject inferred Void

```yumi crash
empty-noop()

━━━━━━━━━━━━━━━━━━━━
+ empty-noop
```

```output
Exception: Non-do function must return a non-void value
<no file:5>
    + empty-noop
    ^^^^^^^^^^^
```

### Non-do functions reject explicit Void

```yumi crash
explicit-void()

━━━━━━━━━━━━━━━━━━━━
+ explicit-void Void
```

```output
Exception: Non-do function must return a non-void value
<no file:5>
    + explicit-void Void
    ^^^^^^^^^^^^^^^^^^^
```

### Non-do functions with a declared return reject a body that produces no value

```yumi crash
-- stored(1)

━━━━━━━━━━━━━━━━━━━━
+ stored Int
    - n Int

do
    n = 4
```

```output
Exception: Non-do function must return a non-void value
<no file:5>
    + stored Int
    ^^^^^^^^^^^
```

### This constructors are implicitly imperative

```yumi
-- Thing(5).value

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ value Int

────────────────────
+ This
    - value Int

this.value = value
```

```output
5
```

### Constructors ignore unused method results

```yumi
-- Thing().value

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ value = 0

────────────────────
+ This

initialize()

────────────────────
_ do initialize Int

value = 5
= 42
```

```output
5
```

### Imperative closures return explicitly

```yumi
. fn = => do
    = 9

-- fn()
```

```output
9
```

### Expression closures reject explicit return

```yumi crash
. fn = =>
    = 9

-- fn()
```

```output
Exception: A closure without do is one expression and cannot return with "="; write "=> do"
<no file:3>
        = 9
        ^^^
```

### Expression closures reject assignments before their value

```yumi crash
. calls = 0
. counter = =>
    calls += 1
    calls

-- counter()
```

```output
Exception: A closure without do is one expression and cannot hold assignments; put the assignment in a "do" block, or write "=> do" and return its value with "= value"
<no file:4>
        calls += 1
        ^^^^^^^^^^
```

### Expression closures run checks, prints and do forms before their value

```yumi
. calls = 0
. counter = :: n Int =>
    ?! Panic("big") if n > 5
    do
        calls += 1
    do finally
        -- "last"
    -- "printed"
    calls + n

-- counter(3)
```

```output
printed
last
4
```

### Module top-level rejects explicit return

```yumi crash
= 1
```

```output
Exception: The top level of a module cannot return a value with "="; remove the "=", or print the value with "--"
<no file:2>
    = 1
    ^^^
```

### Module top-level rejects value-return-if

```yumi crash
=? 1
    if true
```

```output
Exception: The top level of a module cannot return a value with "="; remove the "=", or print the value with "--"
<no file:2>
    =? 1
    ^^^
```

```yumi
-- choose-size'Int()
-- choose-size'(Int, Int, Int, Int, Int)()

━━━━━━━━━━━━━━━━━━━━
+ do choose-size'Item Int

if Item.size <= 16
    = 1
else
    = 2
```

```output
1
2
```

### A Void function can return a Void call

```yumi
check("abc")
check("a")
finish("done")

━━━━━━━━━━━━━━━━━━━━
+ do noted
    - text String

-- "noted $text"

━━━━━━━━━━━━━━━━━━━━
+ do check
    - text String

=? noted(text) if text.length > 2
-- "short $text"

━━━━━━━━━━━━━━━━━━━━
+ do finish
    - text String

= noted(text)
```

```output
noted abc
short a
noted done
```

### The returns of a `do` function: a value, nothing, a value under a condition, and the condition alone.

```yumi
-- pick(true)
-- pick(false)
-- small(1)
-- small(50)
show(1)
show(5)
-- first-over(3)

━━━━━━━━━━━━━━━━━━━━
+ do pick Int
    - flag Bool

=? 1 if flag
= 2

━━━━━━━━━━━━━━━━━━━━
+ do small Bool
    - n Int

=? if n > 10
= true

━━━━━━━━━━━━━━━━━━━━
+ do show
    - n Int

if n > 3
    =
-- n

━━━━━━━━━━━━━━━━━━━━
+ do first-over Int
    - limit Int

. i = 0
while true
    i += 1
    if i > limit
        = i
```

```output
1
2
true
false
1
4
```

### A `do` function that runs off its end returns nothing, or false when it returns a Bool.

```yumi
-- found(8)
-- found(2)
quiet(1)
-- "end"

━━━━━━━━━━━━━━━━━━━━
+ do found Bool
    - n Int

if n > 5
    = true

━━━━━━━━━━━━━━━━━━━━
+ do quiet
    - n Int

n += 1
```

```output
true
false
end
```

### A returned value is converted to the type the function declares.

```yumi
. x I32 = 3
-- f()
-- g(x)
-- h(x)

━━━━━━━━━━━━━━━━━━━━
+ do f Float

= 1

━━━━━━━━━━━━━━━━━━━━
+ do g I64
    - n I32

= n

━━━━━━━━━━━━━━━━━━━━
+ do h Int
    - n I32

=? 200 if n > 2
= n
```

```output
1
3
200
```

### A `do` function with no return type returns the type its returns promote to.

```yumi
-- pick(true)
-- pick(false)

━━━━━━━━━━━━━━━━━━━━
+ do pick
    - flag Bool

if flag
    = 1
= 2.5
```

```output
1
2.5
```

### The body of a `do` function has locals, branches and loops.

```yumi
-- spin(3)
-- spin(100000)
-- label(7)

━━━━━━━━━━━━━━━━━━━━
+ do spin Int
    - n Int

. total = 0
for i in 0 .. < n
    total += i % 7
= total

━━━━━━━━━━━━━━━━━━━━
+ do label String
    - n Int

. text = ""
. left = n
while left > 0
    if left % 2 == 0
        text ++= "e"
    else
        text ++= "o"
    left -= 1
= text
```

```output
3
299995
oeoeoeo
```

### A `do` function that declares a value to return cannot run off its end.

```yumi crash
-- missing()

━━━━━━━━━━━━━━━━━━━━
+ do missing Int

. a = 1
```

```output
Exception: Function missing can fall through without returning I64
<no file:5>
    + do missing Int
    ^^^^^^^^^^^^^^^
```

### A branch that does not return leaves the end of its function in reach.

```yumi crash
-- part(3)

━━━━━━━━━━━━━━━━━━━━
+ do part Float
    - n Int

if n > 2
    = 1.5
```

```output
Exception: Function part can fall through without returning F64
<no file:5>
    + do part Float
    ^^^^^^^^^^^^^^
```

### The end of a function is out of reach after a return, an endless loop, or an `if` on the constant true.

```yumi
-- after(3)
-- endless(3)
-- always(3)

━━━━━━━━━━━━━━━━━━━━
+ do after Int
    - n Int

if n > 5
    = 1
= 2
n += 1

━━━━━━━━━━━━━━━━━━━━
+ do endless Int
    - n Int

while true
    n += 1
    if n > 5
        = n

━━━━━━━━━━━━━━━━━━━━
+ do always Int
    - n Int

if true
    = n
```

```output
2
6
3
```

### A function may end on a match that must match: no arm matching is a panic, so the end is not in reach.

```yumi
-- pick(Item.left(1))
-- pick(Item.right(2))

━━━━━━━━━━━━━━━━━━━━
+ Left is Tuple

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Right is Tuple

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Item is Union

| left Left
| right Right

━━━━━━━━━━━━━━━━━━━━
+ do pick Int
    - item Item

match item
    | .left(row)
        if row.value == 1
            = 10
        = 11
    | .right(row)
        = 20
```

```output
10
20
```

### A match written as a step, whose value nothing uses, runs its arms as steps, so an arm may only print.

```yumi
step(3)
step(4)

━━━━━━━━━━━━━━━━━━━━
+ do step
    - n Int

match n
    |= 3
        -- "three"
    ??
        -- "other"
```

```output
three
other
```

### In Brix, a match on a row of its own in a body of steps runs its arms as steps: in a function's body, in an `if`, an `else` or a loop, in an arm of a match run as steps, in a closure that runs steps and in `main`. A `match?` run as steps goes on where no arm is taken, and a function may end on a match whose arms all return.

```brix
(type + Shade (is Union)
  (variant red)
  (variant blue)
)

(fn-do + show _
  (param s Shade)
  (param t Shade)
  (match s
    (case .red (print "red"))
    (case .blue
      (match t
        (case .red (print "blue, red"))
        (case .blue (print "blue, blue"))
      )
    )
  )
)

(fn-do + tally _
  (param shades (list Shade))
  (param loud Bool)
  (let reds 0)
  (for (in s shades)
    (match s
      (case .red (+= reds 1))
      (case .blue (print "blue"))
    )
  )
  (if loud
    (match reds
      (case= 0 (print "no red"))
      (default (print "some red"))
    )
    (else
      (inline
        (match? ([] shades 0)
          (case .red (print "quiet red"))
        )
      )
    )
  )
  (print (str "reds " reds))
)

(fn-do + code I64
  (param s Shade)
  (match s
    (case .red (return 1))
    (case .blue (return 2))
  )
)

(main
  (call show .red .blue)
  (call show .blue .red)
  (call tally (list (. Shade red) (. Shade blue)) true)
  (call tally (list (. Shade blue)) false)
  (print (str (call code .red) " " (call code .blue)))
  (let s Shade .blue)
  (let run (=> () :do (match s (case .red (print "closure red")) (case .blue (print "closure blue")))))
  (call run)
  (match s
    (case .red (print "main red"))
    (case .blue (print "main blue"))
  )
)
```

```output
red
blue, red
blue
some red
reds 1
blue
reds 0
1 2
closure blue
main blue
```

### A loop on the constant true can be left by a break in an arm of a match, so what follows it is in reach.

```yumi crash
-- pick(Item.left(1))

━━━━━━━━━━━━━━━━━━━━
+ Left is Tuple

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Right is Tuple

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Item is Union

| left Left
| right Right

━━━━━━━━━━━━━━━━━━━━
+ do pick Int
    - item Item

while true
    match? item
        | .left(row)
            break
    = 3
```

```output
Exception: Function pick can fall through without returning I64
<no file:21>
    + do pick Int
    ^^^^^^^^^^^^
```

### A function an interface requires and gives no body has no end to run off.

```yumi
. thing = Thing()
foo(thing)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ do bar Int

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ do bar Int

= 42

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - p Proto

-- p.bar()
```

```output
42
```

### A condition that is the constant false is its else, and a return under the constant true ends the function.

```yumi
-- first()
-- second()
-- third()

━━━━━━━━━━━━━━━━━━━━
+ do first Int

if false
    -- 0
else
    = 1

━━━━━━━━━━━━━━━━━━━━
+ do second Int

=? 4 if true

━━━━━━━━━━━━━━━━━━━━
+ do third Int

if 0
    -- 5
else
    = 6
```

```output
1
4
6
```

### A condition that is the constant false and has no else leaves the end in reach.

```yumi crash
-- pick()

━━━━━━━━━━━━━━━━━━━━
+ do pick Int

if false
    = 1
```

```output
Exception: Function pick can fall through without returning I64
<no file:5>
    + do pick Int
    ^^^^^^^^^^^^
```

### A return of a call that returns nothing leaves the function returning nothing, and a conditional return of a value makes it optional.

```yumi
check("abc")
check("a")
finish("done")
-- pick(1)
-- pick(0) ?? 9
both(true)
both(false)

━━━━━━━━━━━━━━━━━━━━
+ do noted
    - text String

-- "noted $text"

━━━━━━━━━━━━━━━━━━━━
+ do check
    - text String

=? noted(text) if text.length > 2
-- "short $text"

━━━━━━━━━━━━━━━━━━━━
+ do finish
    - text String

= noted(text)

━━━━━━━━━━━━━━━━━━━━
+ do pick
    - n Int

=? n * 2 if n > 0
-- "none"

━━━━━━━━━━━━━━━━━━━━
+ do both
    - loud Bool

if loud
    = noted("loud")
noted("quiet")
```

```output
noted abc
short a
noted done
2
none
9
noted loud
noted quiet
```

### A method written with no body gives what the type it returns makes with nothing written.

```yumi
. h = Holder()
-- "[${h.text()}]"
-- h.maybe()
-- h.point()
-- h.flag()
-- h.ratio()
-- h.items().length
-- "end"

━━━━━━━━━━━━━━━━━━━━
+ Point is (x Int, y Int)

━━━━━━━━━━━━━━━━━━━━
+ Holder

────────────────────
+ text String

────────────────────
+ maybe Int?

────────────────────
+ point Point

────────────────────
+ flag Bool

────────────────────
+ ratio Float

────────────────────
+ items [Int]
```

```output
[]
unknown
(0, 0)
false
0
0
end
```

### A function declared outside any type with no body, which no compiler intrinsic answers, has nothing to give.

```yumi compile-error
-- twice(2.0)

━━━━━━━━━━━━━━━━━━━━
+ twice F64
    - n F64
```

```output
Exception: Function "twice" has no body, and no compiler intrinsic supplies one
<no file:5>
    + twice F64
    ^^^^^^^^^^
```

### A generic method written with no body gives an object of the type it names.

```yumi
. d = Holder()
-- d.get-foo'Int()
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ Foo'T

━━━━━━━━━━━━━━━━━━━━
+ Holder

────────────────────
+ get-foo'Other Foo'Other
```

```output
[Object __source__:Foo'core/number:I64]
after
```

### Non-do functions reject a call before their value even when the body can fail

```yumi crash
-- catch pair(1)

━━━━━━━━━━━━━━━━━━━━
+ pair Int ?! Failure
    - n Int

checked(n)
checked(n + 1)

━━━━━━━━━━━━━━━━━━━━
+ do checked Int ?! Failure
    - n Int

?! Failure("negative")
    if n < 0
= n

━━━━━━━━━━━━━━━━━━━━
+ Failure is Error
```

```output
Exception: Function "pair" is not declared with do, so its body is one expression and cannot run calls before its value; put the call in a "do" block, or write "+ do pair Int ?! Failure" and return its value with "= value"
<no file:8>
    checked(n)
    ^^^^^^^^^^
```

### Expression functions reject explicit return where they are declared, though nothing calls them

```yumi crash
-- "never"

━━━━━━━━━━━━━━━━━━━━
+ foo
= 1
```

```output
Exception: Function "foo" is not declared with do, so its body is one expression and cannot return with "="; write "+ do foo"
<no file:6>
    = 1
    ^^^
```

### Getters that are not do reject immediate declarations

```yumi crash
-- Holder().name

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ value Int = 1

────────────────────
+ get name String

. text = "held"
text
```

```output
Exception: Function "name" is not declared with do, so its body is one expression and cannot hold ". name = value" declarations; write "+ do get name String" and return its value with "= value"
<no file:12>
    . text = "held"
    ^^^^^^^^^^^^^^^
```

### A `do` function that returns from inside a `do use` block returns what it gives there.

```yumi
-- make(3).size

━━━━━━━━━━━━━━━━━━━━
+ do make
    - size U32

. res = Res()
do use res
    = Box(size)

━━━━━━━━━━━━━━━━━━━━
+ Res
    is Usable'Void

────────────────────
+ begin-using

this

────────────────────
+ do end-using

-- "end"

━━━━━━━━━━━━━━━━━━━━
+ Box

+ size U32

────────────────────
+ This
    - size U32
```

```output
end
3
```

### A `do` function that ends in a loop returns an optional, as though it could run past the loop, even where the loop never ends.

```yumi crash
-- find(3) + 1

━━━━━━━━━━━━━━━━━━━━
+ do find
    - n Int

. i = 0
while true
    if i == n
        = i
    i += 1
```

```output
Exception: Operator "+" is not defined for I64? and I64
<no file:2>
    -- find(3) + 1
       ^^^^^^^^^^^
```

### A `do` function that ends in a match that must match returns what its arms return when each arm returns.

```yumi
-- size(Shape.circle(2)) + 1

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union
| circle(r Int)
| square(s Int)

━━━━━━━━━━━━━━━━━━━━
+ do size
    - shape Shape

match shape
    | .circle(r)
        = r
    | .square(s)
        = s * 2
```

```output
3
```

### Statements after one that returns are never reached, and are not typed.

```yumi
-- value()

━━━━━━━━━━━━━━━━━━━━
+ do value Int

= 1
*> each
```

```output
1
```

### `=? value` of an optional returns only where the optional holds a value, so the statements after it run where it holds none.

```yumi
-- first(null)
-- first(2)

━━━━━━━━━━━━━━━━━━━━
+ do first Int
    - value Int?

=? value
-- "none"
= 0
```

```output
none
0
2
```

### Statements after one that leaves are never bound either, so the names they use need not exist: after a return, a `continue`, and a raise under no condition.

```yumi
-- value()
-- skipped()
-- failing()

━━━━━━━━━━━━━━━━━━━━
+ do value Int

= 1
-- missing-name
. x = missing-function()

━━━━━━━━━━━━━━━━━━━━
+ do skipped Int

. total = 0
for item in [1, 2]
    if item == 1
        continue
        -- unknown-name
    total += item
= total

━━━━━━━━━━━━━━━━━━━━
+ do failing Int

. maybe Int? = 3
=? maybe
?! Panic("never")
-- after-panic
= 0
```

```output
1
2
3
```
