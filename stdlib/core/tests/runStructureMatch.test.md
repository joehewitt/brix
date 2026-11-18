# Structure match

```yumi
-- matched-name(null)
-- matched-name(NamedMatchNode())
-- matched-name(MatchNode())

━━━━━━━━━━━━━━━━━━━━
+ MatchNode

━━━━━━━━━━━━━━━━━━━━
+ NamedMatchNode is MatchNode

+ name = "named"

━━━━━━━━━━━━━━━━━━━━
+ matched-name
    - node MatchNode?

match node
    |= null
        "null"
    | named NamedMatchNode
        named.name
    ??
        "other"
```

```output
null
named
other
```

### An arm that is a name alone takes every value, so no arm may follow it.

```yumi compile-error
-- first-name(MatchNode())

━━━━━━━━━━━━━━━━━━━━
+ MatchNode

━━━━━━━━━━━━━━━━━━━━
+ NamedMatchNode is MatchNode

+ name = "named"

━━━━━━━━━━━━━━━━━━━━
+ first-name
    - node MatchNode?

match node
    | any
        "first"
    | named NamedMatchNode
        named.name
    ??
        "other"
```

```output
Exception: Arm '| any' takes every value, so the arms after it are never reached
<no file:17>
        | any
          ^^^
```

### An arm that is a name alone binds the name even where the subject has a case of that name.

```yumi compile-error
-- describe(Light.green)

━━━━━━━━━━━━━━━━━━━━
+ Light is Union

| red
| green

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - light Light

match light
    | Light.green
        "green"
    | red
        "red"
```

```output
Exception: Arm '| red' binds a new name, not the case Light.red; match the case with '| Light.red'
<no file:17>
        | red
          ^^^
```

```yumi
-- 42 is Int
-- 42 is! String
```

```output
true
false
```

```yumi
. selected = 0
. trace = ""

match 2
    |= 1
        selected = 10
        trace += "one"
    |= 2
        selected = 20
        trace += "two"
        selected += 2
    ??
        selected = 30
        trace += "default"

-- selected
-- trace
```

```output
22
two
```

```yumi
. selected = 0

match 7
    |? it < 0
        selected = -1
    |? it > 5
        . offset = 40
        selected = it + offset
    ??
        selected = 1

-- selected
```

```output
47
```

```yumi
. selected = 4

match? 7
    |= 1
        selected = 10
    |= 2
        selected = 20

-- selected
```

```output
4
```

```yumi
do new
    match 8
        |? it > 10
            -- "unreachable"
catch failure
    -- failure
```

```output
Pattern match failed
```

```yumi
do new
    match 8
        |? it > 10
            -- "unreachable"
        ?! Panic("too much")
catch failure
    -- failure
```

```output
too much
```

```yumi
-- choose(1)
-- choose(2)

━━━━━━━━━━━━━━━━━━━━
+ do choose I32
    - value I32

match value
    |= 1
        = 10
    ??
        = 20
```

```output
10
20
```

```yumi
. total = 0

for value in [1, 2, 3, 4]
    match value
        |= 2
            continue
        |= 4
            break
        ??
            total += value

-- total
```

```output
4
```

```yumi
. counter = Counter()
. selected = 0

match counter.next()
    |= 1
        selected = 10
    |= 2
        selected = 20
    ??
        selected = 30

-- counter.count
-- selected

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ count I32

────────────────────
+ do next I32

count += 1
= 2
```

```output
1
20
```

```yumi
. event = match true
    |= true
        ..opened(7)
    ??
        ..closed
. selected = 0

match event
    | .opened(value)
        selected = value
    | .closed
        selected = -1

-- selected

━━━━━━━━━━━━━━━━━━━━
+ Event is Union

| opened I32
| closed
```

```output
7
```

```yumi
match 1
    |= 1
        42
    ??
        "unused"

-- "done"
```

```output
done
```

### A number is not a Bool, but converts to one.

```yumi
-- it | Bool~ = 4.5
-- 42 is Bool
-- 42 is~ Bool
```

```output
true
false
true
```

```yumi
-- match 42
    | Int
        it * 2
```

```output
84
```

```yumi
-- match 42
    | String!
        it * 2
    ?? 10
```

```output
10
```

```yumi crash
-- match 42
    | String!
        it * 2
```

```output
Exception: Pattern has no possible matches
<no file:3>
        | String!
        ^^^^^^^^
```

```yumi
-- match 42
    | a Int
        a * 2.0
```

```output
84
```

```yumi
. x = match? 42
    | {it > 20}
-- type-of x
-- x
```

```output
[TypeInfo Optional'I64]
42
```

```yumi
. x = match 42
    | {it > 20}
    ?? 1
-- type-of x
-- x
```

```output
[TypeInfo I64]
42
```

```yumi
-- match 42
    | Int
        "Ok"
    ??
        "Not"
```

```output
Ok
```

```yumi
-- match 42
    | 42
        "Ok"
    ??
        "Not"
```

```output
Ok
```

```yumi
-- match 42
    | {it > 5}
        "Ok"
    ??
        "Not"
```

```output
Ok
```

```yumi
-- match 42
    | Int {it > 5}
        "Ok"
    ??
        "Not"
```

```output
Ok
```

```yumi
-- match 42
    | String | {it > 5}
        "Ok"
    ??
        "Not"
```

```output
Ok
```

```yumi
-- match 42
    | {it > 5} | String
        "Ok"
    ??
        "Not"
```

```output
Ok
```

```yumi
-- match 42
    | String! | List
        "Ok"
    ?? "Not"
```

```output
Not
```

```yumi crash
-- match 42
    | String! | List
        "Ok"
```

```output
Exception: Pattern has no possible matches
<no file:3>
        | String! | List
        ^^^^^^^^^^^^^^^
```

```yumi
-- match 42
    | String!
        "Black Hole"
    | Int
        "Universe"
    ??
        "Nope"
```

```output
Universe
```

```yumi
-- test: "foo", 42
-- test: "bar", "foo"
-- test: 42, "bar"
-- test: 24

━━━━━━━━━━━━━━━━━━━━
+ test
    - expr

match expr
    | String!, Int!
        "Black Hole"
    | Int
        "Universe"
    ??
        "Nope"
```

```output
Black Hole
Nope
Nope
Universe
```

```yumi
-- match [1, 2, 3]
    | [..., last]
        last
-- match [1, 2, 3]
    | [first, ...]
        first
-- match [1, 2, 3]
    | [first, ..., last]
        first + last
```

```output
3
1
4
```

```yumi
-- match "foo", 42
    | a String, b Int
        "$a = $b"
    ??
         "Nope"
```

```output
foo = 42
```

```yumi
-- match "foo"
    | a String
        key
        .. key = a
    ?? "Nope"
```

```output
foo
```

```yumi
-- match [1, 2, 3]
    | Int
        "num"
    | x List
        "$foo"
        .. foo = ",".join(x)
    ??
        "Nope"
```

```output
1,2,3
```

```yumi
-- match "foo", 42
    | a String, b Int
        key
        .. key = "$a = $b"
    ?? "Nope"
```

```output
foo = 42
```

```yumi
-- match "foo", 42
    | a String, b {it > 20}
        key
        .. key = "$a = $b"
    ?? "Nope"
```

```output
foo = 42
```

```yumi
-- match [42]
    | a List
        key
        .. key = ", ".join(value)
        .. value = each * 2 in a
    ?? "Nope"
```

```output
84
```

```yumi
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ foo

|? 42 is List
    1
??
    y
    .. y = 2
```

```output
2
```

```yumi
-- match [42]
    | x List
        "list $x"
    | Int
        "num"
```

```output
list 42
```

```yumi
. v = 2
-- match v
    | 3
        13
    | 2
        12
    | 1
        11
    ??
        14
```

```output
12
```

```yumi
. opcode U8 = 3u8
-- match opcode
    | 0
        "load"
    | 1
        "add"
    | 2
        "store"
    | 3
        "halt"
    ??
        "unknown"
```

```output
halt
```

```yumi
. opcode U8 = 8u8
-- match opcode
    | 0
        "load"
    | 3
        "add"
    | 8
        "store"
    | 16
        "halt"
    ??
        "unknown"
```

```output
store
```

```yumi
. v = 1
-- match v
    |? it > 0
        10
    ??
        12
```

```output
10
```

```yumi
. attempt = do new
    -- check(1)
    -- check(3)
    -- check(2)
catch failure
    -- "Error: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ check
    - v

match v
    | 1
        10
    ?! Panic("Failed")
        if it == 2
    ??
        12
```

```output
10
12
Error: Failed
```

```yumi
do new
    -- match 8
        |? it > 10
            10
        ?! Panic("too much")
catch failure
    -- failure
```

```output
too much
```

```yumi
 -- ", ".join • stream in [1, 2, 3]
    | n Int
        n * 2
    ??
        0
```

```output
2, 4, 6
```

```yumi
. iter1 = in [1, 2, 3, 2, 4]
    <* {it > 2}
-- type-of iter1
-- ", "(iter1)

. iter2 = <* {it > 2} in [1, 2, 3]
-- type-of iter2
-- ", "(iter2)
```

```output
[TypeInfo Generator'I64]
3, 4
[TypeInfo Generator'I64]
3
```

### Output of each pattern goes to the next one in "and" sequence

```yumi
-- match 10foo
    | Bar~ {it > 20}

-- match 10foo
    | Bar~ {it > 20} Foo~ {it < 30}

+ Foo (foo) is F64
+ Bar (bar) is I32

+ 1bar = 0.333333333333foo
```

```output
30
9.99999999999
```

```yumi
-- match Foo2()
    | Foo2
        1
    ??
        2

-- match Foo()
    | Foo2
        1
    ??
        2

━━━━━━━━━━━━━━━━━━━━
+ Foo

━━━━━━━━━━━━━━━━━━━━
+ Foo2 is Foo
```

```output
1
2
```

## Imperative conditional matches

```yumi
if | foo {it == 42} = 42
    -- "pass $foo"
    -- "ok"
else
    -- "fail"
```

```output
pass 42
ok
```

```yumi
if | foo {it == 43} = 42
    -- "pass $foo"
else
    -- "fail"
```

```output
fail
```

```yumi
. x = 0
while | foo {it < 3} = x
    -- "pass $foo"
    x += 1
```

```output
pass 0
pass 1
pass 2
```

## Optional matching

```yumi
. foo Int? = 42
-- | Int = foo
-- | Int {it > 40} = foo
```

```output
42
42
```

## Property matching

```yumi
. ..length = "foo"
-- length
```

```output
3
```

```yumi
. x ..length = "foo"
-- x
```

```output
3
```

```yumi crash
. x ..length = "foo"
-- length
```

```output
Exception: Name "length" not found
<no file:3>
    -- length
       ^^^^^^
```

```yumi crash
. ..nada = "foo"
```

```output
Exception: Pattern has no possible matches
<no file:2>
    . ..nada = "foo"
      ^^^^^^
```

```yumi
do new
    . y (..length {it == 1}) = "f"
    -- y

    . x (..length {it == 3}) = "f"
    -- x
catch failure
    -- failure
```

```output
1
Pattern match failed
```

### Two patterns binding same variable name should not conflict

```yumi
. foo = Foo("hi")

-- match foo
    | a Int~
        a
    | a String~
        10

━━━━━━━━━━━━━━━━━━━━
+ Foo is Union

| Int
| String
```

```output
10
```

```yumi
-- match 42
    |= 4
        1
    |= 42
        2
    ??
        3
```

```output
2
```

```yumi
-- match 1
    |= 4
        1
    |= 42
        2
    ??
        3
```

```output
3
```

```yumi
-- match? 1
    |= 4
        1
    |= 42
        2
```

```output
unknown
```

### Integer match compares against constants that are not add/sub immediates

```yumi
-- few(-3)
-- few(-4)
-- few(7)
-- many(-3)
-- many(5000)
-- many(70000)
-- many(-100000)
-- many(2)
-- many(9)

━━━━━━━━━━━━━━━━━━━━
+ few String
    - status I32

match status
    |= -3
        "minus three"
    |= -4
        "minus four"
    ??
        "other"

━━━━━━━━━━━━━━━━━━━━
+ many String
    - value I64

match value
    |= -100000
        "minus hundred thousand"
    |= -3
        "minus three"
    |= 2
        "two"
    |= 5000
        "five thousand"
    |= 70000
        "seventy thousand"
    ??
        "other"
```

```output
minus three
minus four
other
minus three
five thousand
seventy thousand
minus hundred thousand
two
other
```

### A match no arm of which matches panics where its first arm is written

```yumi
do new
    . value = 6
    -- match value
        |= 4
            "four"
        |= 5
            "five"
catch failure
    -- failure
```

```output
Pattern match failed
```

### A match statement panics where no arm matches, and one with ? goes on

```yumi
do new
    . value = 6
    match value
        |= 6
            -- "six"
    match? value
        |= 4
            -- "four"
    -- "after"
    match value
        |= 4
            -- "four"
        |= 5
            -- "five"
    -- "unreachable"
catch failure
    -- failure
```

```output
six
after
Pattern match failed
```

### A type with a condition that fails leaves the match unmatched

```yumi
do new
    -- size(7)
    -- size(1)
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ size Int
    - value Int

match value
    | Int {it > 5}
        it
```

```output
7
Pattern match failed
```

### A condition in a match on a subject tests a text for emptiness

```yumi
-- label("", 5)
-- label("x", 5)
-- label("", 1)

━━━━━━━━━━━━━━━━━━━━
+ label String
    - name String
    - count Int

match count
    |? name
        "named"
    |? it > 3
        "big"
    ??
        "small"
```

```output
big
named
small
```

### A condition in a match with no subject tests a list and a text for emptiness

```yumi
. none [Int]
-- pick(none, "")
-- pick([1], "")
-- pick(none, "n")

━━━━━━━━━━━━━━━━━━━━
+ pick String
    - items [Int]
    - name String

match true
    |? items
        "items"
    |? name
        "name"
    ??
        "none"
```

```output
none
items
name
```

### A condition after a type tests what the type passed of an optional

```yumi
-- size(7)
-- size(2)
-- size(null)

━━━━━━━━━━━━━━━━━━━━
+ size Int
    - value Int?

match value
    | Int {it > 5}
        it
    | n Int
        n + 100
    |= null
        -1
```

```output
7
102
-1
```

### A condition after a type tests the case of a union the type matched

```yumi
-- describe(.count(7))
-- describe(.count(2))
-- describe(.circle(2.0))
-- describe(.circle(1.0))
-- describe(.named("a"))
-- describe(.named(""))

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| circle Float
| count Int
| named String

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - shape Shape

match shape
    | Int {it > 5}
        "big $it"
    | Int
        "small $it"
    | Float {it > 1.5}
        "wide"
    | String {it}
        "named $it"
    ??
        "other"
```

```output
big 7
small 2
wide
other
named a
other
```

### A union matches text only where it holds its case of text

```yumi
. u Shape = .circle(1.0)
-- match u
    | Float {it > 1.5}
        "wide"
    | String {it}
        "named $it"
    ?? "other"

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| circle Float
| count Int
| named String
```

```output
other
```

### A union with no case of text converts to text where it holds the case picked for text

```yumi
. u Mixed = .count(1)
-- match u
    | String~
        "s $it"
    ?? "other"
. w Mixed = .flag(true)
-- match w
    | String~
        "s $it"
    ?? "other"

━━━━━━━━━━━━━━━━━━━━
+ Mixed is Union

| count Int
| flag Bool
```

```output
other
s true
```

### The alternatives of an arm bind the same names, which the arm reads whichever of them matched.

```yumi
-- show(.a(3))
-- show(.b(4))
-- show(.c)
-- pick((1, 2))
-- pick((5, 1))

━━━━━━━━━━━━━━━━━━━━
+ show String
    - shape Shape

match shape
    | .a(n) | .b(n)
        "n is $n"
    ??
        "none"

━━━━━━━━━━━━━━━━━━━━
+ pick Int
    - pair (Int, Int)

match pair
    | (1, x) | (x, 1)
        x
    ??
        0

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| a Int
| b Int
| c
```

```output
n is 3
n is 4
none
2
5
```

### A name the alternatives of an arm bind is of one type in each.

```yumi crash
-- show(.a(3))

━━━━━━━━━━━━━━━━━━━━
+ show String
    - shape Shape

match shape
    | .a(n) | .b(n)
        "n is $n"
    ??
        "none"

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| a Int
| b String
```

```output
Exception: "n" is I64 in one alternative and String in another; bind it to one type in each
<no file:9>
        | .a(n) | .b(n)
                  ^^^^^
```

### A name one alternative of an arm binds is bound by the others too.

```yumi crash
-- show(.a(3))

━━━━━━━━━━━━━━━━━━━━
+ show String
    - shape Shape

match shape
    | .a(n) | .c
        "n is $n"
    ??
        "none"

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| a Int
| c
```

```output
Exception: "n" is bound by the alternative before this one but not by this one; bind it in each alternative
<no file:9>
        | .a(n) | .c
                  ^^
```

### A pattern binds a name once; one written twice in it is an error, not a test that the two places are equal.

```yumi crash
-- match [1, 2]
    | [a, a]
        "same $a"
    ??
        "other"
```

```output
Exception: "a" is bound twice in this pattern; give each place a name of its own
<no file:3>
        | [a, a]
              ^
```

### A match whose arms give values of types that share none gives a union of them all, however many there are and in whatever order.

```yumi
for n in 1 .. 3
    -- describe(n)
    -- weigh(n)

━━━━━━━━━━━━━━━━━━━━
+ do describe String
    - n Int

. value = match n
    |= 1
        "one"
    |= 2
        2
    ??
        2.5
= "$value"

━━━━━━━━━━━━━━━━━━━━
+ do weigh String
    - n Int

. value = match n
    |= 1
        2.5
    |= 2
        3
    ??
        "many"
= "$value"
```

```output
one
2.5
2
3
2.5
many
```

### Captures with types take the cases of a union

```yumi
-- show(.count(7))
-- show(.wide(2.5))
-- show(.named("n"))
-- show(.empty)

━━━━━━━━━━━━━━━━━━━━
+ show String
    - shape Shape

match shape
    | n Int
        "int ${n + 1}"
    | f Float
        "float ${f * 2.0}"
    | s String
        "text $s"
    ??
        "none"

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| count Int
| wide Float
| named String
| empty
```

```output
int 8
float 5
text n
none
```

### Cases named with patterns for what they hold

```yumi
-- show(.count(7))
-- show(.wide(2.5))
-- show(.named("n"))
-- show(.empty)
-- show(.pair(1, "p"))

━━━━━━━━━━━━━━━━━━━━
+ show String
    - shape Shape

match shape
    | .count(n)
        "count $n"
    | .wide(Float)
        "wide $it"
    | .named
        "named"
    | .pair(a, b)
        "pair $a $b"
    | .empty
        "empty"

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| count Int
| wide Float
| named String
| pair(a Int, b String)
| empty
```

```output
count 7
wide 2.5
named
empty
pair 1 p
```

### An optional matched against null, a capture with a condition, and a type

```yumi
-- pick(null)
-- pick(3)
-- pick(30)
-- twice(null)
-- twice(4)

━━━━━━━━━━━━━━━━━━━━
+ pick String
    - value Int?

match value
    |= null
        "nothing"
    | n Int {n > 10}
        "big $n"
    | Int
        "small $it"

━━━━━━━━━━━━━━━━━━━━
+ twice Int?
    - value Int?

match? value
    | n Int
        n * 2
```

```output
nothing
small 3
big 30
unknown
8
```

### Classes derived from the subject's class

```yumi
-- sound(Dog())
-- sound(Cat())
-- sound(Animal())

━━━━━━━━━━━━━━━━━━━━
+ sound String
    - animal Animal

match animal
    | d Dog
        d.bark
    | Cat
        it.meow
    ??
        "silence"

━━━━━━━━━━━━━━━━━━━━
+ Animal

+ legs = 4

━━━━━━━━━━━━━━━━━━━━
+ Dog is Animal

+ bark = "woof"

━━━━━━━━━━━━━━━━━━━━
+ Cat is Animal

+ meow = "meow"
```

```output
woof
meow
silence
```

### A tuple of subjects with conditions on its fields

```yumi
-- pair("foo", 42)
-- pair("foo", 2)
-- pair("bar", 42)

━━━━━━━━━━━━━━━━━━━━
+ pair String
    - name String
    - count Int

match name, count
    | a String, b {it > 20}
        "$a = $b"
    | a {it == "bar"}, n
        "bar $n"
    ?? "Nope"
```

```output
foo = 42
Nope
bar = 42
```

### Match statements, conditions on the subject, and a struct captured

```yumi
. point = Point(1, 2)
-- match point
    | p Point {p.x == 1}
        p.x + p.y
    ?? 0
. sum = 0
match point.x
    |= 1
        sum += 10
    |= 2
        sum += 20
match? point.y
    |= 7
        sum += 100
-- sum
-- grade(95)
-- grade(85)
-- grade(10)

━━━━━━━━━━━━━━━━━━━━
+ grade String
    - score Int

match score
    |? it >= 90
        "A"
    |? it >= 80
        "B"
    ??
        "F"

━━━━━━━━━━━━━━━━━━━━
+ Point is (x Int, y Int)
```

```output
3
10
A
B
F
```

### A match in an arm of a match

```yumi
-- nest(.count(7), true)
-- nest(.count(7), false)
-- nest(.named("n"), true)

━━━━━━━━━━━━━━━━━━━━
+ nest String
    - shape Shape
    - loud Bool

match shape
    | .count(n)
        match loud
            |= true
                "COUNT $n"
            ??
                "count $n"
    | .named(s)
        s

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| count Int
| named String
```

```output
COUNT 7
count 7
n
```

### What a case named passes on: the union, or what its pattern passed

```yumi
. v Shape? = Shape.count(3)
-- match v
    | .count
        "count ${type-of it}"
    ??
        "none"
. w Shape = .named("x")
-- match w
    | .count(Int)
        "count $it"
    | .named(String {it.length > 3})
        "long $it"
    | .named(s)
        "short $s $it"

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| count Int
| named String
```

```output
count [TypeInfo Shape]
short x x
```

### An arm that raises a panic for whatever no arm before it matched

```yumi
do new
    . value = 8
    . text = match value
        |= 8
            "eight"
        ?! Panic("not eight")
    -- text
    -- match value
        |= 9
            "nine"
        ?! Panic("not nine")
catch failure
    -- failure
```

```output
eight
not nine
```

### is and is not between numbers, texts and optionals hold for the value's own type, never by conversion

```yumi
-- 42 is Int
-- 42 is! String
-- 42 is Bool
-- 42 is String
-- 42 is! Int
-- 42 is~ Float
-- 42 is not Int
-- 4.5 is not String
. o Int? = 3
-- o is Int
. n Int?
-- n is Int
-- n is not Int
-- "x" is String
-- "x" is Int
```

```output
true
false
false
false
true
true
false
true
true
false
true
true
false
```

### A type named with how near a value must be to it is matched over a union and over a number, and captured.

```yumi
-- near(Num(1))
-- near(Num(2.5))
-- near(Num("x"))
-- exact(Num(1))
-- exact(Num(2.5))
-- exact(Num("x"))
-- plain(7)
-- plain(8)

━━━━━━━━━━━━━━━━━━━━
+ do near String
    - n Num

= match n
    | a Int~
        "int $a"
    | b String~
        "text $b"
    ??
        "other"

━━━━━━━━━━━━━━━━━━━━
+ do exact String
    - n Num

= match n
    | a Float!
        "float $a"
    | b Int!
        "int $b"
    ??
        "other"

━━━━━━━━━━━━━━━━━━━━
+ do plain String
    - n Int

= match n
    | a Int~ {it > 7}
        "big $a"
    | a Int!
        "small $a"

━━━━━━━━━━━━━━━━━━━━
+ Num is Union

| Int
| Float
| String
```

```output
int 1
other
text x
int 1
float 2.5
other
small 7
big 8
```

### Pattern conditions chained over the cases of a union

```yumi
. kinds [Shape] = [Shape.circle(2), Shape.square(3), Shape.dot, Shape.rect(2, 5)]
. index = 0
while index < 4
    describe(kinds[index])
    index += 1

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| dot
| circle(radius Int)
| square(side Int)
| rect(wide Int, tall Int)

━━━━━━━━━━━━━━━━━━━━
+ do describe
    - shape Shape

if | .circle(radius) = shape
    -- "circle $radius"
else if | .square(side) = shape
    -- "square $side"
else if | .rect(wide, tall) = shape
    -- "rect ${wide * tall}"
else
    -- "something else"
-- "done"
```

```output
circle 2
done
square 3
done
something else
done
rect 10
done
```

### Pattern condition that always matches

```yumi
if | x = 42
    -- "always $x"
else
    -- "never"
if | y Int = 7
    -- "int $y"
-- "end"
```

```output
always 42
int 7
end
```

### Break and continue in a pattern loop inside another loop

```yumi
. outer = 0
while outer < 3
    . inner = 0
    while | step {it < 5} = inner
        inner += 1
        if step == 1
            continue
        if step == 3
            break
        -- "outer $outer step $step"
    -- "left inner at $inner"
    outer += 1
-- "end"
```

```output
outer 0 step 0
outer 0 step 2
left inner at 4
outer 1 step 0
outer 1 step 2
left inner at 4
outer 2 step 0
outer 2 step 2
left inner at 4
end
```

### Pattern loop over an optional its body assigns

```yumi
. current Int? = 3
while | held Int = current
    -- "held $held"
    if held == 0
        current = null
    else
        current = held - 1
-- "end"
```

```output
held 3
held 2
held 1
held 0
end
```

### Return from the body of a pattern condition

```yumi
-- find(5)
-- find(12)
-- find(-1)

━━━━━━━━━━━━━━━━━━━━
+ do find String
    - number Int

if | small {it < 10 && it >= 0} = number
    = "small $small"
if | large {it >= 10} = number
    = "large $large"
= "negative"
```

```output
small 5
large 12
negative
```

### Raise from the body of a pattern condition

```yumi
-- catch? run(1) ?? -1
-- catch? run(20) ?? -1
-- catch? run(-4) ?? -1
-- catch? run(5) ?? -1

━━━━━━━━━━━━━━━━━━━━
+ do run Int ?! Failed
    - number Int

if | small {it < 10 && it >= 0} = number
    ?! Failed()
        if small == 1
    = small
if | large {it >= 10} = number
    = large * 2
= 0

━━━━━━━━━━━━━━━━━━━━
+ Failed is Error
```

```output
-1
40
0
5
```

### Pattern loop evaluates its value on each pass

```yumi
. total = 0
. count = 0
while | value {it < 20} = next(count)
    count += 1
    total += value
-- "count $count total $total"

━━━━━━━━━━━━━━━━━━━━
+ do next Int
    - at-step Int

-- "next $at-step"
= at-step * at-step
```

```output
next 0
next 1
next 2
next 3
next 4
next 5
count 5 total 30
```

### Pattern condition after a plain condition

```yumi
. flag = true
if flag == false
    -- "no"
else if | .valid(email, password) = submit("joe", "secret")
    -- "$email / $password"
else
    -- "invalid"
if | .valid(email, password) = submit("", "secret")
    -- "$email / $password"
else if | .invalid-email = submit("", "secret")
    -- "bad email"
else
    -- "invalid"

━━━━━━━━━━━━━━━━━━━━
+ Authorization is Union

| invalid-password
| invalid-email
| valid(email String, password String)

━━━━━━━━━━━━━━━━━━━━
+ do submit Authorization
    - email String
    - password String

if email.length == 0
    = Authorization.invalid-email
= Authorization.valid(email, password)
```

```output
joe / secret
bad email
```

### Pattern conditions nested with a cleanup

```yumi
. first Int? = 4
. second Int? = null
if | a Int = first
    if | b Int = second
        -- "both $a $b"
    else
        -- "only a $a"
    do finally
        -- "leaving a"
    -- "inside a"
-- "end"
```

```output
only a 4
inside a
leaving a
end
```

### Cleanup and continue in a pattern loop

```yumi
. count = 0
while | n {it < 4} = count
    do finally
        -- "after $n"
    count += 1
    if n == 2
        continue
    -- "body $n"
-- "end $count"
```

```output
body 0
after 0
body 1
after 1
after 2
body 3
after 3
end 4
```

### Pattern conditions that take a tuple apart

```yumi
. point = Point(3, 4)
if | (x, y) = point
    -- "point $x $y"
if | (x {it > 5}, y) = point
    -- "big $x $y"
else
    -- "small"
. steps = 0
while | (x {it < 6}, y) = point
    point = Point(x + 1, y * 2)
    steps += 1
-- "steps $steps ${point.x} ${point.y}"

━━━━━━━━━━━━━━━━━━━━
+ Point is (x Int, y Int)
```

```output
point 3 4
small
steps 3 6 32
```

### Pattern conditions over the types a union holds

```yumi
. value Foo = 12
if | number Int = value
    -- "number $number"
else if | text String = value
    -- "text $text"
value = "words"
if | number Int = value
    -- "number $number"
else if | text String = value
    -- "text $text"
. tries = 0
while | text String = value
    -- "still $text"
    tries += 1
    if tries == 2
        value = 5
-- "end $tries"

━━━━━━━━━━━━━━━━━━━━
+ Foo is Union

| Int
| String
```

```output
number 12
text words
still words
still words
end 2
```

### Pattern conditions over optionals

```yumi
. maybe Int? = null
if | held Int = maybe
    -- "held $held"
else
    -- "nothing"
maybe = 9
if | held Int = maybe
    -- "held $held"
else
    -- "nothing"
. text String? = "word"
while | held String = text
    -- held
    text = null
-- "end"
```

```output
nothing
held 9
word
end
```

### Break from a pattern loop inside a loop of a function

```yumi
-- count-down(3)

━━━━━━━━━━━━━━━━━━━━
+ do count-down Int
    - from Int

. left = from
. rounds = 0
while true
    while | n {it > 0} = left
        left -= 1
        rounds += 1
        if n == 2
            break
    if left == 0
        = rounds
    rounds += 10
```

```output
13
```

### Pattern conditions over anonymous cases

```yumi
. result = attempt(3)
if | .ok(n) = result
    -- "ok $n"
else if | .failed = result
    -- "failed"
. other = attempt(-1)
if | .ok(n) = other
    -- "ok $n"
else if | .failed = other
    -- "failed"
. count = 5
while | .ok(n) = attempt(count)
    -- "again $n"
    count -= 2
-- "end"

━━━━━━━━━━━━━━━━━━━━
+ attempt
    - number Int

if number > 0 then ..ok(number) else ..failed
```

```output
ok 3
failed
again 5
again 3
again 1
end
```

### Capture of a pattern condition is not named after it

```yumi crash
. value = 3
if | found {it > 1} = value
    -- "found $found"
-- found
```

```output
Exception: Name "found" not found
<no file:5>
    -- found
       ^^^^^
```

### Pattern condition naming a case of a class that has values

```yumi crash
. level = Level.low
if | .low = level
    -- "low"
else
    -- "other"

━━━━━━━━━━━━━━━━━━━━
+ Level

| low
| middle
| high
```

```output
Exception: Pattern has no possible matches
<no file:3>
    if | .low = level
    ^^^^^^^^^^^^^^^^
```

### Pattern loop naming a case of a class that has values

```yumi crash
. level = Level.low
. passes = 0
while | .low = level
    passes += 1
    if passes == 3
        level = Level.high
if | .high = level
    -- "high after $passes"
else
    -- "still low"
if | .low = level
    -- "low"
else if | .middle = level
    -- "middle"
else
    -- "neither"

━━━━━━━━━━━━━━━━━━━━
+ Level

| low
| middle
| high
```

```output
Exception: Pattern has no possible matches
<no file:4>
    while | .low = level
    ^^^^^^^^^^^^^^^^^^^
```

### Pattern condition naming a case no type has

```yumi crash
. shape = Shape.dot
if | .circle(radius) = shape
    -- "circle $radius"
else if | .missing = shape
    -- "missing"
else
    -- "other"

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| dot
| circle(radius Int)
```

```output
Exception: Case ".missing" not found in Shape
<no file:5>
    else if | .missing = shape
              ^^^^^^^^
```

### Pattern condition naming a case over an optional of a class

```yumi crash
. level Level? = Level.low
if | .low = level
    -- "low"
else
    -- "other"

━━━━━━━━━━━━━━━━━━━━
+ Level

| low
| middle
```

```output
Exception: Pattern has no possible matches
<no file:3>
    if | .low = level
    ^^^^^^^^^^^^^^^^
```

### Arm naming a case of a class that has values gives the match no type

```yumi
. level = Level.low
. x = match level
    | .low
        1
    ??
        "other"
-- x
-- type-of x

━━━━━━━━━━━━━━━━━━━━
+ Level

| low
| middle
| high
```

```output
other
[TypeInfo String]
```

### Arm naming a case of a class that has values before an arm of its type

```yumi
. level = Level.low
. x = match level
    | .low
        1.5
    | Level
        2
-- x
-- type-of x

━━━━━━━━━━━━━━━━━━━━
+ Level

| low
| middle
| high
```

```output
2
[TypeInfo I64]
```

### Arms naming cases of a class that has values match nothing

```yumi
. level = Level.low
match level
    | .low
        -- "low"
    | .middle
        -- "middle"
    ??
        -- "other"

━━━━━━━━━━━━━━━━━━━━
+ Level

| low
| middle
| high
```

```output
other
```

### Field pattern naming a case of a class that has values keeps its arm

```yumi
. pair = Pair(Level.low, 4)
. x = match pair
    | (.low, n)
        "$n"
    ??
        "other"
-- x
-- type-of x

━━━━━━━━━━━━━━━━━━━━
+ Pair is (level Level, count Int)

━━━━━━━━━━━━━━━━━━━━
+ Level

| low
| middle
```

```output
other
[TypeInfo String]
```

### Case patterns of a name two unions have

```yumi
. first = Color.red
. second = Light.red
-- describe(first)
-- match second
    | .red
        "light red"
    ??
        "light other"

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - color Color

match color
    | .red
        "red"
    | .green
        "green"

━━━━━━━━━━━━━━━━━━━━
+ Color is Union

| red
| green

━━━━━━━━━━━━━━━━━━━━
+ Light is Union

| red
| amber
```

```output
red
light red
```

### Case pattern naming a value of a class a union holds

```yumi
. value Mixed = Level.low
-- match value
    | .low
        "low"
    | Int
        "int"
    ??
        "other"
. pair = (value, 3)
-- match pair
    | (.low, n)
        "low $n"
    ??
        "other pair"

━━━━━━━━━━━━━━━━━━━━
+ Mixed is Union

| Int
| Level

━━━━━━━━━━━━━━━━━━━━
+ Level

| low
| middle
```

```output
other
other pair
```

### Match whose only arm names a case of a class that has values

```yumi crash
. level = Level.low
-- match level
    | .low
        "low"

━━━━━━━━━━━━━━━━━━━━
+ Level

| low
| middle
| high
```

```output
Exception: Pattern has no possible matches
<no file:4>
        | .low
        ^^^^^
```

### Optional match whose only arm names a case of a class that has values

```yumi crash
. level = Level.low
. answer = match? level
    | .low
        true
-- answer

━━━━━━━━━━━━━━━━━━━━
+ Level

| low
| middle
| high
```

```output
Exception: Pattern has no possible matches
<no file:4>
        | .low
        ^^^^^
```

### Arm naming a case its union lacks and no type has

```yumi crash
. shape = Shape.dot
-- match shape
    | .missing
        "missing"
    ??
        "other"

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| dot
| circle(radius Int)
```

```output
Exception: Case ".missing" not found in Shape
<no file:4>
        | .missing
          ^^^^^^^^
```

### Optional match naming a case no type has

```yumi crash
. shape = Shape.dot
. answer = match? shape
    | .missing
        "missing"
-- answer ?? "none"

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| dot
| circle(radius Int)
```

```output
Exception: Case ".missing" not found in Shape
<no file:4>
        | .missing
          ^^^^^^^^
```

### Arm naming a case over a number

```yumi crash
. number = 5
. answer = match? number
    | .low
        "low"
    | text String
        text
-- answer ?? "none"
```

```output
Exception: Case ".low" not found: I64 has no cases
<no file:4>
        | .low
          ^^^^
```

### Field pattern naming a case no type has

```yumi crash
. pair = Pair(Shape.dot, 4)
-- match pair
    | (.missing, n)
        "missing $n"
    | (.dot, n)
        "dot $n"
    ??
        "other"

━━━━━━━━━━━━━━━━━━━━
+ Pair is (shape Shape, count Int)

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| dot
| circle(radius Int)
```

```output
Exception: Case ".missing" not found in Shape
<no file:4>
        | (.missing, n)
           ^^^^^^^^
```

### Declaration that tests the name it reads

```yumi crash
first("foo")
second("foo")

━━━━━━━━━━━━━━━━━━━━
+ do first
    - text String

. ..length {it == 3} = text
-- length

━━━━━━━━━━━━━━━━━━━━
+ do second
    - text String

-- "before"
. ..length {it == 4} = text
-- "not reached $length"
```

```output
3
before
```

### Declarations that test names of different types

```yumi
. point = Point(3.5, 4.0)
. a (..length {it == 1}) = "f"
. b (..x {it > 2.5}) = point
. c (..length {it > 1}) = "four"
-- a
-- b
-- c
-- type-of b

━━━━━━━━━━━━━━━━━━━━
+ Point is (x Float, y Float)
```

```output
1
3.5
4
[TypeInfo F64]
```

### Locals declared by a name read are assigned

```yumi
. x ..length = "foo"
x = x + 1
-- x
. text = "hello"
. ..length = text
length += 10
-- length
-- text.length
```

```output
4
15
5
```

### Local declared by a name read holds a copy

```yumi
. rect = Rect()
. p ..origin = rect
. ..size = rect
rect.origin = Point(9, 9)
-- p.x
-- size.y
-- rect.origin.x

━━━━━━━━━━━━━━━━━━━━
+ Point is (x Int, y Int)

━━━━━━━━━━━━━━━━━━━━
+ Rect

+ origin = Point(1, 2)
+ size = Point(3, 4)
```

```output
1
4
9
```

### Declaration reads its name once

```yumi
. counter = Counter()
. ..next = counter
. second ..next = counter
-- next
-- second
-- counter.calls

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ calls = 0

────────────────────
+ do get next Int

calls += 1
= calls * 10
```

```output
10
20
2
```

### Declaration in a loop that tests the name it reads

```yumi crash
-- measure("ab")
-- measure("abcdef")

━━━━━━━━━━━━━━━━━━━━
+ do measure Int
    - text String

. total = 0
. round = 0
while round < 3
    . n (..length {it < 5}) = text
    total += n
    round += 1
= total
```

```output
6
```

### Declaration with two tests of the name it reads

```yumi
. attempt = do new
    . point = Point(3.5, 4.0)
    . a (..x {it > 2.5} {it < 4.0}) = point
    -- a
    . b (..y {it > 2.5} {it < 4.0}) = point
    -- b
catch failure
    -- failure
    -- "after"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Point is (x Float, y Float)
```

```output
3.5
Pattern match failed
after
```

### Declaration by a name read in a function

```yumi
show("foo")

━━━━━━━━━━━━━━━━━━━━
+ do show
    - text String

. ..length = text
-- length
```

```output
3
```

### Arm that captures a name read

```yumi
-- match "foo"
    | x ..length
        x
```

```output
3
```

### Arm that reads a name

```yumi
-- match "hey"
    | ..length
        length + 1
```

```output
4
```

### Optional match whose arm reads a name

```yumi
. n = match? "hello"
    | ..length
        length
-- n
```

```output
5
```

### Name read under a captured name is not declared

```yumi crash
. x ..length = "foo"
-- x
-- length
```

```output
Exception: Name "length" not found
<no file:4>
    -- length
       ^^^^^^
```

### Declaration reading a name its value lacks

```yumi crash
. ..missing = "foo"
-- missing
```

```output
Exception: Pattern has no possible matches
<no file:2>
    . ..missing = "foo"
      ^^^^^^^^^
```

### Arm among several that tests a name read

```yumi crash
-- match "hello"
    | ..length {it == 4}
        length
    ??
        99
```

```output
Exception: Type not found
<no file:3>
        | ..length {it == 4}
          ^^^^^^^^
```

### Arm among several that captures a tested name read

```yumi crash
-- match "hello"
    | n (..length {it == 5})
        n
    ??
        99
```

```output
Exception: Type not found
<no file:3>
        | n (..length {it == 5})
             ^^^^^^^^
```

### Arm among several that reads a name

```yumi crash
-- match "hello"
    | Int
        1
    | ..length
        length
```

```output
Exception: Type not found
<no file:5>
        | ..length
          ^^^^^^^^
```

### Arm among several that captures a name read

```yumi crash
-- match "hello"
    | x ..length
        x
    | String
        0
```

```output
Exception: Type not found
<no file:3>
        | x ..length
            ^^^^^^^^
```

### Arm reading a name under a captured name does not declare it

```yumi crash
-- match "foo"
    | x ..length
        length
```

```output
Exception: Name "length" not found
<no file:4>
            length
            ^^^^^^
```

### Match that ends a function whose arms each return

```yumi
-- Thing(10.5)
-- Thing(-2.0)
-- describe(3)
-- describe(-3)

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ foo I32
+ bar Float

────────────────────
+ do This
    - n Float

if n < 0.0
    = This(0, 0.0)
= This(1, n)

━━━━━━━━━━━━━━━━━━━━
+ do describe String
    - n Int

match n
    | {it < 0}
        = "negative"
    ??
        = "other"
```

```output
(1, 10.5)
(0, 0)
other
negative
```

### Match whose arms return with no arm for what is left

```yumi crash
-- sign(-4)
-- sign(0)
-- sign(9)

━━━━━━━━━━━━━━━━━━━━
+ do sign String
    - n Int

match n
    | {it < 0}
        = "negative"
    | 0
        = "zero"
= "positive"
```

```output
negative
zero
```

### Matches whose arms return, raise or go on

```yumi
-- catch? grade(95) ?? "failed"
-- catch? grade(40) ?? "failed"
-- name(1)
-- name(2)
-- name(3)

━━━━━━━━━━━━━━━━━━━━
+ Failed is Error

━━━━━━━━━━━━━━━━━━━━
+ do grade String ?! Failed
    - score Int

match score
    | {it >= 90}
        = "high"
    ??
        ?! Failed()

━━━━━━━━━━━━━━━━━━━━
+ do name String
    - n Int

match n
    | 1
        = "one"
    | 2
        = "two"
    ??
        -- "many is printed"
= "many"
```

```output
high
failed
one
two
many is printed
many
```

### Alternatives written in one arm are tried in turn and share the arm's body

```yumi
-- pick(3)
-- pick(7)
-- pick(20)
-- name("a")
-- name("b")
-- name("z")

━━━━━━━━━━━━━━━━━━━━
+ pick
    - v Int

match v
    | {it < 5} | {it > 10}
        "out"
    ??
        "in"

━━━━━━━━━━━━━━━━━━━━
+ name
    - v String

match v
    | "a" | "b"
        1
    ??
        0
```

```output
out
in
out
1
1
0
```

### Alternatives of one arm that name cases of a union

```yumi
-- kind(Shape.circle(2))
-- kind(Shape.square(3))
-- kind(Shape.dot)
-- size(Shape.circle(2))
-- size(Shape.square(3))
-- size(Shape.dot)

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| circle (r Int)
| square (s Int)
| dot

━━━━━━━━━━━━━━━━━━━━
+ kind
    - s Shape

match s
    | .circle | .dot
        "round"
    | .square
        "boxy"

━━━━━━━━━━━━━━━━━━━━
+ size
    - s Shape

match s
    | .dot | .circle
        0
    | .square(n)
        n * n
```

```output
round
boxy
round
0
9
0
```

### Three alternatives in one arm, and alternatives that name types of a union

```yumi
-- sort(3)
-- sort(-4)
-- sort(0)
-- sort(50)
-- word("x")
-- word(7)
-- word(true)

━━━━━━━━━━━━━━━━━━━━
+ sort
    - v Int

match v
    | 0 | {it > 40} | {it < -3}
        "edge"
    | {it > 0}
        "plus"
    ??
        "minus"

━━━━━━━━━━━━━━━━━━━━
+ Any is Union

| String
| Int
| Bool

━━━━━━━━━━━━━━━━━━━━
+ word
    - v Any

match v
    | Bool | Int
        "scalar"
    | String
        "text"
```

```output
plus
edge
edge
edge
text
scalar
scalar
```

### An arm that raises a panic where its condition holds, written among other arms

```yumi
. attempt = do new
    -- grade(50)
    -- grade(10)
    -- grade(101)
    -- grade(60)
catch failure
    -- "caught $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ grade
    - v Int

match v
    | 1000
        "top"
    ?! Panic("too big")
        if it > 100
    | {it > 40}
        "pass"
    ??
        "fail"
```

```output
pass
fail
caught too big
```

### Types named one after another in a pattern each convert what the one before made

```yumi
-- match 10foo
    | Bar~ {it > 20} Foo~ {it < 30}
-- match 30foo
    | Bar~ {it > 20} Foo~ {it < 30}
    ??
        0foo
-- match 5
    | Int {it > 3} String~ {it.length == 1}
-- match 7
    | Int {it > 3} {it < 10}
-- type-of chained(12)
-- chained(12)
-- chained(2)
-- shown(9bar)

+ Foo (foo) is F64
+ Bar (bar) is I32

+ 1bar = 0.333333333333foo

━━━━━━━━━━━━━━━━━━━━
+ chained
    - n I8

match n
    | Int~ {it > 3} String~ {it.length == 2}
    ??
        "none"

━━━━━━━━━━━━━━━━━━━━
+ shown
    - b Bar

match b
    | Foo~ {it > 2} Bar~
        "wide $it"
    ??
        "narrow"
```

```output
9.99999999999
29.99999999997
5
7
[TypeInfo String]
12
none
wide 9
```

### An arm written with a test narrows what the arms after it read, with a subject and with none.

```yumi
-- of-subject(0)
-- of-subject(5)
-- of-subject(null)
-- of-true(0)
-- of-true(5)
-- of-true(null)
-- of-number(0)
-- of-number(5)
-- of-number(null)
-- of-rest(0)
-- of-rest(5)
-- of-rest(null)

━━━━━━━━━━━━━━━━━━━━
+ of-subject String
    - value Int?

match value
    |? !it
        "absent"
    |? it
        "truthy"
    ??
        "falsy"

━━━━━━━━━━━━━━━━━━━━
+ of-true String
    - value Int?

match true
    |? !value
        "absent"
    |? value
        "truthy"
    ??
        "falsy"

━━━━━━━━━━━━━━━━━━━━
+ of-number String
    - value Int?

match 1
    |? value == null
        "absent"
    |? value
        "truthy"
    ??
        "falsy"

━━━━━━━━━━━━━━━━━━━━
+ of-rest String
    - value Int?

match 1
    |? !value
        "absent"
    ??
        if value then "truthy" else "falsy"
```

```output
falsy
truthy
absent
falsy
truthy
absent
falsy
truthy
absent
falsy
truthy
absent
```

### A match whose arms stand in a block with the lazy locals they read.

```yumi
-- Spot.first.place(10)
-- Spot.middle.place(10)
-- Spot.last.place(10)
. spot = Spot.index(7)
-- spot.place(10)

━━━━━━━━━━━━━━━━━━━━
+ Spot is Union

| first
| middle
| last
| index Int

────────────────────
+ place Int
    - count Int

match this
    `
    | Spot.first
        start
    | Spot.middle
        length / 2
    | Spot.last
        length - 1
    | Spot.index(n)
        n
    `
    .. start = 0
    .. length = count
```

```output
0
5
9
7
```

### A lazy local under the arms of a match is evaluated once, by the arm that reads it: in match, match? and a match? run as steps.

```yumi
-- Spot.first.place(10)
-- Spot.middle.place(10)
-- Spot.last.place(10)
. spot = Spot.index(7)
-- spot.place(10)
-- spot.named(3)
-- Spot.last.named(3)
spot.shown(4)
Spot.first.shown(4)

━━━━━━━━━━━━━━━━━━━━
_ do noisy Int
    - n Int

-- "noisy $n"
= n

━━━━━━━━━━━━━━━━━━━━
+ Spot is Union

| first
| middle
| last
| index Int

────────────────────
+ place Int
    - count Int

match this
    `
    | Spot.first
        start
    | Spot.middle
        length / 2
    | Spot.last
        length - 1 + length
    | Spot.index(n)
        n
    `
    .. start = noisy(0)
    .. length = noisy(count)

────────────────────
+ named String?
    - count Int

match? this
    `
    | Spot.index(n)
        "index $n of $length"
    `
    .. length = noisy(count)

────────────────────
+ do shown
    - count Int

match? this
    `
    | Spot.index(n)
        -- "index $n of $length and $length"
    | Spot.first
        -- "first"
    `
    .. length = noisy(count)
```

```output
noisy 0
0
noisy 10
5
noisy 10
19
7
noisy 3
index 7 of 3
unknown
noisy 4
index 7 of 4 and 4
first
```

### A comprehension written with the arms of a match gives what the arm an item fits gives, and passes over an item no arm fits

```yumi
. doubled = in [1, 2, 3]
    | n Int
        n * 2
    ??
        0
-- type-of doubled
-- ", ".join(doubled)
. picked = in [1, 2, 3, 4]
    | 2
        20
    | 4
        40
-- ", ".join(picked)
. lengths = in ["a", "bb", "", "dddd"]
    | ""
        99
    | text String
        text.length
-- ", ".join(lengths)
. shapes = List'Shape()
shapes += Circle(2)
shapes += Square(3)
shapes += Dot()
shapes += Circle(1)
. sizes = in shapes
    | c Circle
        c.radius * 3
    | s Square
        s.side * 4
-- ", ".join(sizes)
. named = in [(1, "x"), (2, "y"), (1, "z")]
    | (1, name)
        "one $name"
    | (count, name)
        "$count of $name"
for text in named
    -- text
-- List(scaled([1, 2, 3], 10)).length
for n in scaled([1, 2, 3], 10)
    -- n

━━━━━━━━━━━━━━━━━━━━
+ Shape

+ sides = 0

━━━━━━━━━━━━━━━━━━━━
+ Circle is Shape

+ radius Int

────────────────────
+ This
    - radius

━━━━━━━━━━━━━━━━━━━━
+ Square is Shape

+ side Int

────────────────────
+ This
    - side

━━━━━━━━━━━━━━━━━━━━
+ Dot is Shape

━━━━━━━━━━━━━━━━━━━━
+ scaled
    - items [Int]
    - by Int

in items
    | 2
        by
    | n Int
        n * by + 1
```

```output
[TypeInfo Generator'I64]
2, 4, 6
20, 40
1, 2, 99, 4
6, 12, 3
one x
2 of y
one z
3
11
10
31
```

### An arm of a comprehension that gives an optional that holds nothing passes over its item

```yumi
. halves = in [1, 2, 3, 4]
    | n Int
        half(n)
-- type-of halves
-- ", ".join(halves)
. mixed = in [1, 2, 3]
    | 1
        "one"
    | n Int
        "$n"
-- type-of mixed
for item in mixed
    -- item
. readers = List
    in [5, 6, 7]
        | 6
            reader(60)
        | n Int
            reader(n + 1)
for read in readers
    -- read()

━━━━━━━━━━━━━━━━━━━━
+ half Int?
    - n Int

|? n % 2 == 0
    n / 2
??
    null

━━━━━━━━━━━━━━━━━━━━
+ reader Getter'Int
    - n Int

=> n
```

```output
[TypeInfo Generator'I64]
1, 2
[TypeInfo Generator'String]
one
2
3
6
60
8
```

### List pattern over a base class reads by the derived class

```yumi
. base Base = Derived()
-- match base
    | [a, b]
        "two $a $b"
    ??
        "none"
-- base is [5, _]

━━━━━━━━━━━━━━━━━━━━
+ Base
    is Indexable'Int

────────────────────
+ get length Size

2

────────────────────
+ this[index Index] Int

index + 1

────────────────────
+ this[range Range'Index] Base

this

━━━━━━━━━━━━━━━━━━━━
+ Derived is Base

────────────────────
+ this[index Index] Int

index * 100 + 5
```

```output
two 5 105
true
```

### A generic class written bare is tested against numbers, texts, lists and maps

```yumi
-- 42 is List
-- [1, 2] is List
-- "text" is List
-- [1, 2] is not List
-- 42 is not List
. items = [1, 2, 3]
if items is List
    -- "a list"
. map = Map'String'Int()
-- map is Map
-- map is List
-- items is Map
```

```output
false
true
false
false
true
a list
true
false
false
```

### A generic class written bare is tested against optionals and tuples

```yumi
. maybe [Int]? = [1, 2]
-- maybe is List
. none [Int]? = null
-- none is List
. box = Box(5)
-- box is Box
-- box is List
-- 5 is Box
. pair = Pair(1, "a")
-- pair is Pair
-- pair is Box

━━━━━━━━━━━━━━━━━━━━
+ Box'Item is Tuple

+ item Item

━━━━━━━━━━━━━━━━━━━━
+ Pair'Left'Right is Tuple

+ left Left
+ right Right
```

```output
true
false
true
false
false
true
false
```

### A generic class written bare in a condition and in a match of a generic function

```yumi
-- check(42)
-- check([1, 2])
-- check("text")
-- pick(1)
-- pick([1])

━━━━━━━━━━━━━━━━━━━━
+ check String
    - value

|? value is List
    "list"
??
    "other"

━━━━━━━━━━━━━━━━━━━━
+ pick'T Int
    - value T

match value
    | List
        1
    ??
        2
```

```output
other
list
other
2
1
```

### A list written bare as an arm of a match over values and unions

```yumi
show(42)
show([1, 2])
show("text")
show(Foo(7))
show(Foo([Foo(1)]))

━━━━━━━━━━━━━━━━━━━━
+ Foo is Union

| Int
| [Foo]

━━━━━━━━━━━━━━━━━━━━
+ do show
    - value

match value
    | List
        -- "list"
    | Int
        -- "int"
    ??
        -- "other"
```

```output
int
list
other
int
list
```

### The arm of a match that takes whatever is left may itself hold a match.

```yumi
-- pick(1, 1)
-- pick(2, 1)
-- pick(2, 2)

━━━━━━━━━━━━━━━━━━━━
+ pick String
    - a Int
    - b Int

match a
    |= 1
        "first"
    ??
        match b
            |= 1
                "second"
            ??
                "neither"
```

```output
first
second
neither
```

### A case named over an optional union binds what the case holds.

```yumi
-- show(.optional("q"))
-- show(.buffer("b"))
-- show(null)

━━━━━━━━━━━━━━━━━━━━
+ Suffix is Union

| optional(name String)
| buffer(name String)

━━━━━━━━━━━━━━━━━━━━
+ show String
    - suffix Suffix?

match suffix
    | .optional(name)
        "optional $name"
    | .buffer(name)
        "buffer $name"
    ??
        "none"
```

```output
optional q
buffer b
none
```

### A match whose every arm returns or raises ends a `do` function that gives a value.

```yumi
-- size-of(3)
-- (catch? size-of(-3)) ?? 99

━━━━━━━━━━━━━━━━━━━━
+ do size-of Int ?! Error
    - value Int

match value
    | number Int {it >= 0}
        = number
    ?! Error("negative")
```

```output
3
99
```

### A comprehension over a pattern naming a case of a union keeps the items holding that case, as the union.

```yumi
. items [Pick] = [.first, .third(4), .second, .third(5)]
for pick in <* .third in items
    -- pick is .third
    -- match pick
        | .third(n)
            n
        ??
            0

━━━━━━━━━━━━━━━━━━━━
+ Pick is Union

| first
| second
| third(Int)
```

```output
true
4
true
5
```

### A match statement's arm may hold a value chosen by conditions, written as a statement.

```yumi
-- fill(2)
-- fill(5)

━━━━━━━━━━━━━━━━━━━━
+ do fill String
    - size Int

. out = Int[3]
match size
    |= 2
        for index in 0 .. < 3
            out[index] =
                |? index == 1
                    10
                ??
                    20
    ??
        out[0] = 1
= "${out[0]} ${out[1]} ${out[2]}"
```

```output
20 10 20
1 0 0
```

### A pattern that names a generic class without its arguments, with a guard, gives the guard's `it` the class the case it matches holds.

```yumi
. pair Item = ["a", "b"] as [Item]
. single Item = "c"
-- describe(pair)
-- describe(single)

━━━━━━━━━━━━━━━━━━━━
+ Item is Union

| String
| [Item]

━━━━━━━━━━━━━━━━━━━━
_ describe String
    - value Item

match value
    | items List~ {it.length == 2}
        "pair ${items.length}"
    ??
        "other"
```

```output
pair 2
other
```

### A generic class written bare matches the case of a union that is of the class, not one that only converts to it.

```yumi
. items [Value]
items += 1 as Value
-- after-data(items as Value)
-- after-data(Byte[2] as Value)
-- alone(items as Value)
-- alone(Byte[2] as Value)

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| Int
| Byte[]
| [Value]

━━━━━━━━━━━━━━━━━━━━
_ do after-data String
    - value Value

match value
    | number Int
        = "$number"
    | data Byte[]
        = "data ${data.size}"
    | list List~
        = "list ${list.length}"

━━━━━━━━━━━━━━━━━━━━
_ do alone String
    - value Value

match value
    | number Int
        = "$number"
    | list List
        = "list ${list.length}"
    ??
        = "other"
```

```output
list 1
data 2
list 1
other
```

### Arms after a null arm read the subject, named or as it, as what it holds, and so does what follows a match statement whose null arm leaves.

```yumi
-- by-it(null)
-- by-it(Holder())
-- by-name(null)
-- by-name(Holder())
-- after-tests(Holder())
-- after-statement(null)
-- after-statement(Holder())

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ count = 3

━━━━━━━━━━━━━━━━━━━━
+ by-it Int
    - held Holder?

match held
    |= null
        0
    |? it.count > 5
        1
    ??
        it.count

━━━━━━━━━━━━━━━━━━━━
+ by-name Int
    - held Holder?

match held
    |= null
        0
    ??
        held.count

━━━━━━━━━━━━━━━━━━━━
+ after-tests Int
    - held Holder?

match held
    |? it == null
        0
    |= null
        1
    |? held.count > 1
        held.count + 10
    ??
        held.count

━━━━━━━━━━━━━━━━━━━━
+ do after-statement Int
    - held Holder?

match held
    |= null
        = 0
    ??
        -- held.count
= held.count + 100
```

```output
0
3
0
3
13
0
3
103
```

### The body of a condition arm of a match on a subject reads the subject as it.

```yumi
-- larger(7)
-- larger(2) ?? 0
-- first-held(Holder(4))?.count ?? 0

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ count Int

────────────────────
+ This
    - count

━━━━━━━━━━━━━━━━━━━━
+ larger Int?
    - value Int

match? value
    |? it > 5
        it + 100

━━━━━━━━━━━━━━━━━━━━
+ first-held Holder?
    - held Holder

match? held
    |? it.count > 1
        it
```

```output
107
0
4
```

### An optional match whose one arm no subject can match gives null where an optional is expected

```yumi
-- found(Shape())?.hosted() ?? "none"
-- found(null)?.hosted() ?? "none"

━━━━━━━━━━━━━━━━━━━━
+ Host is Interface

────────────────────
+ do hosted String

━━━━━━━━━━━━━━━━━━━━
+ Shape

+ name = "shape"

━━━━━━━━━━━━━━━━━━━━
+ do found Host?
    - shape Shape?

= match? shape
    | host Host
        host
```

```output
none
none
```

### The value of an arm is that of its last row, and the rows before it run as steps, whatever they give.

```brix
(type + Shade (is Union)
  (variant red)
  (variant blue)
)

(fn-do + counted I64
  (print "counted")
  (return 3)
)

(fn-do + pick I64
  (param s Shade)
  (let total 0)
  (let x (match s
    (case .red (+= total 1) (call counted) (print "red") (+ 5 total))
    (case .blue 2)
  ))
  (return x)
)

(main
  (print (str (call pick .red)))
  (print (str (call pick .blue)))
)
```

```output
counted
red
6
2
```
