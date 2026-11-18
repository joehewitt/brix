# Op overrides

Binary operators

```yumi
. thing = Thing()
-- thing + 1
-- thing + thing

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a I32 = 10

────────────────────
+ this Int + n I32

a + n

────────────────────
+ this Float + n Thing

a + n.a
```

```output
11
20
```

## Incremental assignment operators.

```yumi
. thing = Thing()
thing += 1
-- thing.a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 10

────────────────────
+ do this += n

a += n
```

```output
11
```

### If incremental operator not overriden but math op is, use that

```yumi
. thing = Thing(10)
thing += 1
-- thing.a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 0

────────────────────
+ This
    - aa

a = aa

────────────────────
+ this + n

Thing(a + n)
```

```output
11
```

### Incremental operator on a property of a member

```yumi
. thing = Thing()
thing.member += 1
-- thing.member.a

━━━━━━━━━━━━━━━━━━━━
+ Member

+ a = 10

────────────────────
+ do this += n

a += n

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ member = Member()
```

```output
11
```

## Concat operator

```yumi
. thing = Thing()
-- "$thing you"

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = "me"

────────────────────
+ to String

a

────────────────────
+ this ++ other

"$a with$other"
```

```output
me you
```

## Comparison operators.

```yumi
. a = Thing(10)
. b = Thing(20)
-- a == b
-- a == a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 10

────────────────────
+ This
    - v

a = v

────────────────────
+ this == other

a == other.a
```

```output
false
true
```

```yumi
. a = Thing(10)
. b = Thing(10)
-- a == b
-- b == a
-- a != b
-- b != a
b.a = 11
-- a == b
-- b == a
-- a != b
-- b != a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 10

────────────────────
+ This
    - v

a = v

────────────────────
+ this != other Thing

a != other.a
```

```output
true
true
false
false
false
false
true
true
```

### != deduced from ==

```yumi
. a = Thing(10)
. b = Thing(10)
-- a == b
-- b == a
-- a != b
-- b != a
b.a = 11
-- a == b
-- b == a
-- a != b
-- b != a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 10

────────────────────
+ This
    - v

a = v

────────────────────
+ this == other Thing

a == other.a
```

```output
true
true
false
false
false
false
true
true
```

```yumi
. n10 = Thing(10)
. n11 = Thing(11)
-- n10 < n11
-- n10 > n11
-- n11 < n10
-- n11 > n10
n11.a = 10
-- n10 < n11
-- n10 > n11
-- n11 < n10
-- n11 > n10

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 10

────────────────────
+ This
    - v

a = v

────────────────────
+ this > other Thing

a > other.a

────────────────────
+ this == other Thing

a == other.a
```

```output
true
false
false
true
false
false
false
false
```

```yumi crash
. a = Thing(10)
. b = Thing(10)
-- a < b

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ value Int

────────────────────
+ This
    - value

────────────────────
+ this > other Thing

value > other.value
```

```output
Exception: Must implement == for Thing
<no file:4>
    -- a < b
       ^^^^^
```

```yumi
. a = Thing(10)
. b = Thing(10)
-- a == b
-- a != b

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ value Int

────────────────────
+ This
    - value

────────────────────
+ this == other Thing

value == other.value
```

```output
true
false
```

```yumi
. a = Thing(10)
. b = Thing(10)
-- a == b
-- a != b

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ value Int

────────────────────
+ This
    - value

────────────────────
+ this != other Thing

value != other.value
```

```output
true
false
```

```yumi
. foo = Foo(10)
. bar = Bar(10)
-- foo > bar
-- foo < bar
-- foo == bar
-- bar > foo
-- bar < foo
bar.value = 11
-- foo > bar
-- foo < bar
-- foo == bar
-- bar > foo
-- bar < foo

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ value Int

────────────────────
+ This
    - value

────────────────────
+ this > other

value > other.value

━━━━━━━━━━━━━━━━━━━━
+ Bar

+ value Int

────────────────────
+ This
    - value

────────────────────
+ this == other

value == other.value
```

```output
false
false
true
false
false
false
true
false
true
false
```

### Comparison ops require Boolean return value

```yumi crash
. a = Thing()
-- a > 1

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ this String > other

"hi"
```

```output
Exception: Operator must return Bool
<no file:9>
    + this String > other
    ^^^^^^^^^^^^^^^^^^^^
```

### A comparison operator answers a Bool, which text does not become unasked.

```yumi crash
. a = Thing()
-- a > 1
-- a < 1

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ this > other

"hi"

────────────────────
+ this < other

""
```

```output
Exception: Expected Bool here, but this is String, which becomes Bool only when asked; write "as Bool"
<no file:12>
    "hi"
    ^^^^

Exception: Expected Bool here, but this is String, which becomes Bool only when asked; write "as Bool"
<no file:17>
    ""
    ^^
```

```yumi
. a = Thing()
. b = Thing()
-- a is in a
-- a is in b

-- a not in a
-- a not in b

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ this has other

this == other

────────────────────
+ this has not other

this != other
```

```output
true
false
false
true
```

### "has not" deduced from "has" operator

```yumi
. a = Thing(10)
-- 10 is in a
-- 10 not in a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ value = 0

────────────────────
+ This
    - value

────────────────────
+ this has other

value == other
```

```output
true
false
```

### "has" deduced from "has not" operator

```yumi
. a = Thing(10)
-- 10 is in a
-- 10 not in a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ value = 0

────────────────────
+ This
    - value

────────────────────
+ this has not other

value != other
```

```output
true
false
```

### Error if both "is in" and "not in" not supported and you call one

```yumi crash
. a = Thing()
. b = Thing()
-- a not in b

━━━━━━━━━━━━━━━━━━━━
+ Thing
```

```output
Exception: Operator "not in" is not defined for Thing and Thing
<no file:4>
    -- a not in b
       ^^^^^^^^^^
```

## Prefix operators.

### Truth override

```yumi
. thing = Thing()
-- !thing

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ !this

true
```

```output
true
```

### Truth override in and expressions

```yumi
. a = Thing(1)
. b = Thing(11)
-- a && true
-- true && a
-- b && true
-- true && b

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 0

────────────────────
+ This
    - n
a = n

────────────────────
+ !this

a <= 10
```

```output
false
false
true
true
```

```yumi
. a = Thing(1)
. b = Thing(11)
-- a || false
-- false || a
-- b || false
-- false || b

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 0

────────────────────
+ This
    - n

a = n

────────────────────
+ !this

a <= 10
```

```output
false
false
true
true
```

### Truth override in if statement

```yumi
. a = Thing(1)
. b = Thing(11)
if a
    -- "hi"
else if b
    -- "oh"
else
    -- "bye"

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 0

────────────────────
+ This
    - n
a = n

────────────────────
+ !this

a <= 10
```

```output
oh
```

### Truth override in if expression

```yumi
. a = Thing(1)
. b = Thing(11)
-- if a then "hi a" else "bye a"
-- if b then "hi b" else "bye b"

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 0

────────────────────
+ This
    - n

a = n

────────────────────
+ !this

a <= 10
```

```output
bye a
hi b
```

### Truth override in while loop

```yumi
. thing = Thing()
while thing
    -- thing.a
    thing.a -= 1

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 3

────────────────────
+ !this

a == 0
```

```output
3
2
1
```

```yumi crash
. thing = Thing()
-- !thing

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 0

────────────────────
+ !this I32

2
```

```output
Exception: Operator must return Bool
<no file:11>
    + !this I32
    ^^^^^^^^^^
```

### An operator that answers a truth answers a Bool, which a number does not become unasked.

```yumi crash
. thing = Thing()
-- !thing
thing.a = 5
-- !thing

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 0

────────────────────
+ !this

a
```

```output
Exception: Expected Bool here, but this is I64, which becomes Bool only when asked; write "as Bool"
<no file:15>
    a
    ^
```

### An operator that answers a truth answers a Bool, which an object does not become unasked.

```yumi crash
. thing = Thing()
-- !thing
thing.state = State(false)
-- !thing

━━━━━━━━━━━━━━━━━━━━
+ State

+ truth = false

────────────────────
+ This
    - t

truth = t

────────────────────
+ !this

!truth

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ state = State(true)

────────────────────
+ !this
state
```

```output
Exception: Expected Bool here, but this is State, which becomes Bool only when asked; write "as Bool"
<no file:30>
    state
    ^^^^^
```

```yumi
. thing = Thing()
. nthing = -thing
-- nthing.a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 5

────────────────────
+ do -this

. neg = Thing()
neg.a = -a
= neg
```

```output
-5
```

## Index operators.

```yumi
. thing = Thing()
-- thing[10]

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ this[index]

index + 5
```

```output
15
```

```yumi
. thing = Thing()
thing[10] = 5
-- thing.a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 10

────────────────────
+ do this[index] = value

a = index + value
```

```output
15
```

```yumi
. thing = Thing()
-= thing[1]
-- thing.a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 10

────────────────────
+ do -= this[index]

a -= index
```

```output
9
```

## Slice operators.

```yumi
. thing = Thing()
-- thing[10 .. < 100 by 2]

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ this[range Range''T]

range.start + range.end + range.increment
```

```output
112
```

```yumi
. thing = Thing()
-- thing[10 .. < 100]

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ this[range Range''T]

range.start + range.end + range.increment
```

```output
111
```

```yumi
. thing = Thing()
thing[10 .. < 100 by 1] = 5
-- thing.a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 10

────────────────────
+ do this[range Range''T] = value

a = range.start + range.end + range.increment + value
```

```output
116
```

```yumi
. thing = Thing()
-= thing[10 .. < 100 by 1]
-- thing.a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 10

────────────────────
+ do -= this[range Range''T]

a -= range.start + range.end + range.increment
```

```output
-101
```

## Call operators

```yumi
. thing = Thing()
thing()
thing(42)

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ do this

-- "Called"

────────────────────
+ do this
    - a

-- "Called $a"
```

```output
Called
Called 42
```

## A number on the left of an operator

### A number on the left of `*` calls the `*` an extension of its type declares for the other operand.

```yumi
-- Three() * 5
-- 5 * Three()
-- 2.5 * Three()

━━━━━━━━━━━━━━━━━━━━
+ Three

────────────────────
+ this * that

3 * that

━━━━━━━━━━━━━━━━━━━━
+ extend Number

────────────────────
+ this * that Three

that * this
```

```output
15
15
7.5
```

### A number on the left of `+` calls the `+` an extension of its type declares for the other operand.

```yumi
-- Ten() + 5
-- 5 + Ten()

━━━━━━━━━━━━━━━━━━━━
+ Ten

────────────────────
+ this + that

10 + that

━━━━━━━━━━━━━━━━━━━━
+ extend Number

────────────────────
+ this + that Ten

that + this
```

```output
15
15
```

### A number assigned with `+=` calls the `+` an extension of its type declares for the other operand.

```yumi
. x = 5
x += Ten()
-- x

━━━━━━━━━━━━━━━━━━━━
+ Ten

────────────────────
+ this + that

10 + that

━━━━━━━━━━━━━━━━━━━━
+ extend Number

────────────────────
+ this + that Ten

that + this
```

```output
15
```

### The right operand is never asked for an operator the left one lacks: a number on the left needs its own.

```yumi compile-error
-- 5 * Three()

━━━━━━━━━━━━━━━━━━━━
+ Three

────────────────────
+ this * that

3 * that
```

```output
Exception: Expected I64 here, but this is Three, and there is no conversion between them
<no file:2>
    -- 5 * Three()
           ^^^^^^^
```

### A list given text to add is an error: its `+` takes an item, and the text is not joined to it reversed.

```yumi compile-error
-- [1, 2, 3] + "1"
```

```output
Exception: Operator "+" is not defined for [I64] and String
<no file:2>
    -- [1, 2, 3] + "1"
       ^^^^^^^^^^^^^^^
```

### Text on the right of a number is not added to it, nor joined to it reversed.

```yumi compile-error
-- 5 + "1"
```

```output
Exception: Expected I64 here, but this is String, and there is no conversion between them
<no file:2>
    -- 5 + "1"
           ^^^
```

### An ordering no function answers is the opposite of the ordering that negates it.

```yumi
-- "a" <= "b"
-- "b" <= "a"
-- "a" <= "a"
-- "a" >= "b"
-- "b" >= "a"
-- "a" >= "a"
```

```output
true
false
true
false
true
true
```

### `<=` deduced from `>=` and `==` holds when the two are equal.

```yumi
. a = Thing(1)
. b = Thing(1)
. c = Thing(2)
-- a <= b
-- a <= c
-- c <= a
-- a >= b
-- a < c
-- c < a
-- a < b

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ value Int

────────────────────
+ This
    - value

────────────────────
+ this >= other Thing

value >= other.value

────────────────────
+ this == other Thing

value == other.value
```

```output
true
true
false
true
true
false
false
```

### An operator a subclass declares again answers for its objects, held as the base class.

```yumi
-- (B() as A) == A()
-- A() == A()
-- (B() as A) != A()
-- A() == B()
-- (B() as A) ++ "!"
-- A() ++ "!"

━━━━━━━━━━━━━━━━━━━━
+ A

────────────────────
+ This

────────────────────
+ this == other A

true

────────────────────
+ this ++ other String

"a" ++ other

━━━━━━━━━━━━━━━━━━━━
+ B is A

────────────────────
+ This

────────────────────
+ this == other A

false

────────────────────
+ this ++ other String

"b" ++ other
```

```output
false
true
true
true
b!
a!
```

### An assigning operator on a property a method names alone calls the function the property's class declares, and reads the property as it is written.

```yumi
. box = Box()
box.grow()
box.grow()
-- box.count
-- box.label

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ n = 0

────────────────────
+ do this += other I32

n += other

━━━━━━━━━━━━━━━━━━━━
+ Box

+ held = Counter()
+ label = "box"

────────────────────
+ do grow

held += 10
label += "!"

────────────────────
+ get count

held.n
```

```output
20
box!!
```

### A value held by a name is called through the function its type declares for a call

```yumi
. parts = ["x", "y", "z"]
-- "-"(parts)
-- ""(parts)
. glue = " + "
-- glue((each * 2 in [1, 2, 3]))
. adder = Adder(10)
-- adder(5)
-- adder • factor 7
. shout = Shout()
shout("hey")

━━━━━━━━━━━━━━━━━━━━
+ Adder

+ base Int

────────────────────
+ This
    - base

────────────────────
+ this Int
    - n Int

base + n

────────────────────
+ this Int
    - factor Int

base * factor

━━━━━━━━━━━━━━━━━━━━
+ Shout

────────────────────
+ do this
    - text String

-- "$text!"
```

```output
x-y-z
xyz
2 + 4 + 6
50
70
hey!
```

### The fallback operator keeps a tuple its truth test passes and takes the other where it fails.

```yumi
-- (Reach(3, 9) ?? Reach(1, 1)).to
-- (Reach(4, 4) ?? Reach(1, 2)).to
-- wider(Reach(5, 5), Reach(6, 8)).to
-- wider(Reach(5, 7), Reach(6, 8)).to

━━━━━━━━━━━━━━━━━━━━
+ Reach is Tuple

+ from Int
+ to Int

────────────────────
+ !this

from == to

━━━━━━━━━━━━━━━━━━━━
+ wider Reach
    - reach Reach
    - other Reach

reach ?? other
```

```output
9
2
8
7
```

### A method named without a call is a function value, which has no truth to test.

```yumi crash
. box = Box()
if box.has-items
    -- "items"

━━━━━━━━━━━━━━━━━━━━
+ Box

+ items [Int]

────────────────────
+ This

────────────────────
+ has-items Bool

items.length > 0
```

```output
Exception: function has-items cannot be tested as a condition; it is a function, so call it first
<no file:3>
    if box.has-items
       ^^^^^^^^^^^^^
```

### An object of a class that does not declare `!this` has no truth to test or deny.

```yumi crash
. thing = Thing()
if thing
    -- "thing"

━━━━━━━━━━━━━━━━━━━━
+ Thing
```

```output
Exception: Thing cannot be tested as a condition, since operator "!" is not defined for it
<no file:3>
    if thing
       ^^^^^
```

```yumi crash
. thing = Thing()
-- !thing

━━━━━━━━━━━━━━━━━━━━
+ Thing
```

```output
Exception: Operator "!" is not defined for Thing
<no file:3>
    -- !thing
       ^^^^^^
```

### `!` of a function value has no truth to deny.

```yumi crash
. box = Box()
-- "${!box.has-items}"

━━━━━━━━━━━━━━━━━━━━
+ Box

+ items [Int]

────────────────────
+ This

────────────────────
+ has-items Bool

items.length > 0
```

```output
Exception: Operator "!" is not defined for function has-items; it is a function, so call it first
<no file:3>
    -- "${!box.has-items}"
          ^^^^^^^^^^^^^^
```

### A range, a tuple, a table or a builder that does not say whether it is empty cannot be tested as a condition.

```yumi crash
. r = 0 .. 3
if r
    -- "yes"
```

```output
Exception: Range'I64 cannot be tested as a condition, since operator "!" is not defined for it
<no file:3>
    if r
       ^
```

### A table has no truth of its own either.

```yumi crash
/core/table

. t = Table'Point(3)
-- "made" if t

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Exception: Table'Point cannot be tested as a condition, since operator "!" is not defined for it
<no file:5>
    -- "made" if t
       ^^^^^^^^^^^
```

### Values ordered by "<" need an operator that orders them; two lists, tuples or atoms have none.

```yumi crash
-- "a" < "b"
. xs = [1, 2]
. ys = [1, 3]
-- xs < ys
```

```output
Exception: Operator "<" is not defined for [I64] and [I64]
<no file:5>
    -- xs < ys
       ^^^^^^^
```
