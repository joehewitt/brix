# Loops

```yumi
. a = 3
while a > 1
    -- a
    a = a - 1
```

```output
3
2
```

```yumi
. a = 4
while a > 1
    -- a
    if a == 3
        break
    a -= 1
-- "done"
```

```output
4
3
done
```

```yumi
. a = 4
while a > 1
    if a == 3
        a -= 1
        continue

    -- a
    a -= 1
-- "done"
```

```output
4
2
done
```

```yumi
. r = 0 .. < 3
-- r.length
```

```output
3
```

```yumi
for i in 0 .. < 3
    -- i
-- "done"
```

```output
0
1
2
done
```

### A `for` with no body runs its loop for what reading its items does

```yumi
. counter = Counter()
for _ in ticks(counter, 3)
for item in [1, 2]
for i in 0 .. < 2
-- counter.value

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ value = 0

━━━━━━━━━━━━━━━━━━━━
+ do ticks <Int>
    - counter Counter
    - n Int

for i in 0 .. < n
    counter.value += 1
    <= i
```

```output
3
```

```yumi
for i in 0 .. < 4
    -- value
        .. value = i * 2
```

```output
0
2
4
6
```

```yumi crash
for item in 42
    -- item
```

```output
Exception: A for loop cannot iterate over I64; to count, loop over a range, as in "0 .. < 42"
<no file:2>
    for item in 42
                ^^
```

### Cursor iterables keep traversal state outside immutable Brix values

```yumi
. values = CursorRange(3)
. total = 0
for value in values
    -- value
    total += value
-- total

━━━━━━━━━━━━━━━━━━━━
+ CursorRange is Struct
    is CursorIterable'I32'I32

+ limit I32

────────────────────
+ get initial-iterator-state I32

0

────────────────────
+ do next-iterator (I32, I32?)
    - state I32

. none I32?
if state >= limit
    = (state, none)

= (state + 1, state + 1)
```

```output
1
2
3
6
```

```yumi
for i in 0 .. < 3
    -- i
    if i == 1
        break
-- "done"
```

```output
0
1
done
```

```yumi
for i in 0 .. < 3
    if i == 1
        continue
    -- i
-- "done"
```

```output
0
2
done
```

## Clauses

```yumi
for i in 0 .. < 10 when i == 3 || i == 5
    -- i
```

```output
3
5
```

```yumi
for i in 0 .. < 10 while i < 3
    -- i
```

```output
0
1
2
```

```yumi
for i in 0 .. < 10 until i == 3
    -- i
```

```output
0
1
2
```

## Combination iterators

```yumi
for i in 0 .. < 3
and j in 10 .. < 13
    -- i, j
```

```output
(0, 10)
(1, 11)
(2, 12)
```

```yumi
for i in 0 .. < 10 when i > 5
and j in 10 .. < 13
    -- i, j
```

```output
(6, 10)
(7, 11)
(8, 12)
```

```yumi
for i in 0 .. < 3
and j in 10 .. < 20 when j > 15
    -- i, j
```

```output
(0, 16)
(1, 17)
(2, 18)
```

```yumi
for i in 0 .. < 10 when i > 5
and j in 10 .. < 20 when j > 15
    -- i, j
```

```output
(6, 16)
(7, 17)
(8, 18)
(9, 19)
```

### Show that loop breaks when either of streams end

```yumi
for i in 0 .. < 10
and j in 10 .. < 13
    -- i, j
```

```output
(0, 10)
(1, 11)
(2, 12)
```

```yumi
for pair in zip([1,2,3], [4,5,6])
    -- pair

━━━━━━━━━━━━━━━━━━━━
+ do zip'T1'T2
    - stream1 <T1>
    - stream2 <T2>

for item1 in stream1
and item2 in stream2
    <= item1, item2
```

```output
(1, 4)
(2, 5)
(3, 6)
```

## Iterator expressions

```yumi
. iter = <* n in 1 .. 3
    n * 2
for i in iter
    -- i
```

```output
2
4
6
```

```yumi
. iter = <* n in 1 .. 6 when n % 2 == 0
    n * 2
for i in iter
    -- i
```

```output
4
8
12
```

```yumi
. iter = <* n in 1 .. 6 while n < 3
    n * 2
for i in iter
    -- i
```

```output
2
4
```

```yumi
. iter = <* n in 1 .. 6 until n == 3
    n * 2
for i in iter
    -- i
```

```output
2
4
```

## Iterator closures

```yumi
. iter = :: ns => each * 2 in ns
for i in iter([1,2,3])
    -- i
```

```output
2
4
6
```

```yumi
. iter = :: ns => each in ns when each % 2
for n in iter(0 .. < 8)
    -- n
```

```output
1
3
5
7
```

```yumi
for i in 0 .. < 3
    do finally
        -- "iter"

    -- i

    if i == 1
        break
-- "done"
```

```output
0
iter
1
iter
done
```

```yumi
. a = 4
while a > 1
    do finally
        -- "iter"

    if a == 3
        a -= 1
        continue

    -- a
    a -= 1
-- "done"
```

```output
4
iter
iter
2
iter
done
```

```yumi
-- List: each * 2 in 0 .. < 5
```

```output
0, 2, 4, 6, 8
```

### A loop header that names a derived class does not narrow its items: match them, or write as.

```yumi crash
for a Foo2 in [Foo(), Foo2(), Foo()]
    -- a

+ Foo
+ Foo2 is Foo
```

```output
Exception: Expected Foo2 here, but this is Foo, which is not always Foo2; match it with "| name Foo2", or write "as Foo2"
<no file:2>
    for a Foo2 in [Foo(), Foo2(), Foo()]
          ^^^^
```

```yumi
for | a Foo2 in [Foo(), Foo2(), Foo()]
    -- a

+ Foo
+ Foo2 is Foo
```

```output
[Object __source__:Foo2]
```

```yumi
for | a {it >= 2} in [1, 2, 3, 1, 2]
    -- a
```

```output
2
3
2
```

```yumi
. a = <* {it % 2} in 0 .. < 10
-- ", "(a)
```

```output
1, 3, 5, 7, 9
```

### An operand that does not convert is reported at the operand, even when it reads the loop's `it`.

```yumi crash
-- nested([[1, 2], [3]])

━━━━━━━━━━━━━━━━━━━━
+ do nested I32
    - rows [[I32]]

. total = 0
for row in rows
    for cell in row
        total += cell + it
= total
```

```output
Exception: Expected I32 here, but this is [I32]?, and there is no conversion between them
<no file:11>
            total += cell + it
                            ^^
```

### A range loop's variable shadows a local of the same name.

```yumi
-- sum(4)

━━━━━━━━━━━━━━━━━━━━
+ do sum U32
    - limit U32

. total U32 = 0
. index = limit
while index > 2
    index -= 1
for index in 0 .. < limit
    total += index
= total + index * 100
```

```output
206
```

### A range loop's variable shadows a member of the same name.

```yumi
-- Counter(7).sum(4)

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ index U32

────────────────────
+ This
    - index U32

────────────────────
+ do sum U32
    - limit U32

. total U32 = 0
for index in 0 .. < limit
    total += index
= total + index * 100
```

```output
706
```

### The item of a loop is not known after the loop.

```yumi crash
for i in 0 .. < 2
    -- i
-- i
```

```output
Exception: Name "i" not found
<no file:4>
    -- i
       ^
```

### `while` runs its body for as long as its condition holds. `continue` goes to the next pass and `break` leaves the nearest loop.

```yumi
. a = 0
. s = 0
while a < 10
    a += 1
    if a % 2 == 0
        continue
    if a > 7
        break
    s += a
-- s
-- a
. b = 0
while a < 14
    b = 0
    while true
        b += 1
        if b > 2
            break
    a += b
-- a
```

```output
16
9
15
```

### A `while` tests a text as its condition.

```yumi
. s = "abc"
. n = 0
while s
    n += 1
    if n == 3
        s = ""
-- n
```

```output
3
```

### What follows `break` in its block never runs.

```yumi
for i in 0 .. < 3
    -- i
    break
    -- "never"
-- "end"
```

```output
0
end
```

### A loop polls on every pass, however long it runs.

```yumi
. a = 0
while a < 100000
    a += 1
-- a
. s I64 = 0
for i in 0 .. < 400
    for j in 0 .. < 400
        s += j
-- s
```

```output
100000
31920000
```

### A local that holds text is kept through the collections a long loop causes.

```yumi
. s = ""
for i in 0 .. < 200000
    s = "a" ++ "b"
    s ++= "c"
-- s
```

```output
abc
```

### A condition that joins texts with && or ||, or that a call makes, is tested as each text is.

```yumi
. s = "a"
. t = ""
. ok = true
while s && ok
    -- s
    ok = false
. n = 0
while t ++ "x" && n < 2
    n += 1
-- n
for i in 0 .. < 3 when s || t
    -- i
for i in 0 .. < 3 until t || i == 1
    -- i
-- "done"
```

```output
a
2
0
1
2
0
done
```

### A clause that is a text is tested as the text is: when, until and while.

```yumi
. s = "a"
. t = ""
for i in 0 .. < 3 when t
    -- i
for i in 0 .. < 3 when s
    -- i
for i in 0 .. < 3 until s
    -- i
for i in 0 .. < 3 while t
    -- i
-- "done"
```

```output
0
1
2
done
```

### A clause of a loop tests whether an optional holds a value, also one a test found to hold one.

```yumi
count(0)
count(5)
count(null)
words("")
words("x")

━━━━━━━━━━━━━━━━━━━━
+ do count
    - value Int?

if !value
    -- "none"
    =
. turns = 0
for at-turn in 0 .. < 3 while value
    turns += 1
-- "while $turns"
turns = 0
for at-turn in 0 .. < 3 when value
    turns += 1
-- "when $turns"
turns = 0
for at-turn in 0 .. < 3 until value
    turns += 1
-- "until $turns"

━━━━━━━━━━━━━━━━━━━━
+ do words
    - value String?

if !value
    =
. turns = 0
for at-turn in 0 .. < 3 while value
    turns += 1
-- "while $turns"
turns = 0
for at-turn in 0 .. < 3 when value
    turns += 1
-- "when $turns"
turns = 0
for at-turn in 0 .. < 3 until value
    turns += 1
-- "until $turns"
```

```output
while 3
when 3
until 0
while 3
when 3
until 0
none
while 3
when 3
until 0
while 3
when 3
until 0
```

### A clause of a loop over an optional no test narrowed tests whether it holds a value.

```yumi
count(0)
count(5)
count(null)

━━━━━━━━━━━━━━━━━━━━
+ do count
    - value Int?

. turns = 0
for at-turn in 0 .. < 3 while value
    turns += 1
-- "while $turns"
turns = 0
for at-turn in 0 .. < 3 when value
    turns += 1
-- "when $turns"
turns = 0
for at-turn in 0 .. < 3 until value
    turns += 1
-- "until $turns"
```

```output
while 3
when 3
until 0
while 3
when 3
until 0
while 0
when 0
until 3
```

### A loop over a list: of texts, of tuples, and one that is empty.

```yumi
. names = ["ann", "bob", "cy"]
for name in names
    -- name
. pairs = [(1, "one"), (2, "two")]
for pair in pairs
    -- pair[0]
    -- pair[1]
. empty [Int]
for item in empty
    -- item
-- "done"
```

```output
ann
bob
cy
1
one
2
two
done
```

### A loop over a list with `continue` and `break` in its body, and with `when`, `while` and `until` in its header.

```yumi
. items = [1, 2, 3, 4, 5, 6]
for item in items
    if item == 2
        continue
    if item == 5
        break
    -- item
for item in items when item % 2 == 0
    -- item
for item in items while item < 3
    -- item
for item in items until item == 4
    -- item
```

```output
1
3
4
2
4
6
1
2
1
2
3
```

### A list that grows while a loop reads it is read to the length it had, and two loops nested over one list.

```yumi
. items = [1, 2, 3]
for item in items
    if item == 1
        items += 9
    -- item
-- items.length
. total = 0
for a in items
    for b in items
        total += a * b
-- total
```

```output
1
2
3
4
225
```

### A loop over a list a function is given, over one a property holds, and in a getter.

```yumi
-- sum([1, 2, 3])
-- longest(["a", "abc", "ab"])
. box = Box([4, 5])
-- box.total
box.show()

━━━━━━━━━━━━━━━━━━━━
+ do sum Int
    - items [Int]

. total = 0
for item in items
    total += item
= total

━━━━━━━━━━━━━━━━━━━━
+ do longest String
    - names [String]

. best = ""
for name in names
    if name.length > best.length
        best = name
= best

━━━━━━━━━━━━━━━━━━━━
+ Box

+ items [Int]

────────────────────
+ This
    - items [Int]

────────────────────
+ get total Int

sum(items)

────────────────────
+ do show

for item in items
    -- item
```

```output
6
abc
9
4
5
```

### A loop over a list of structs, of optionals, of objects and of floats.

```yumi
. points = [Point(1, 2), Point(3, 4)]
for point in points
    -- point.x + point.y
. maybe [Int?] = [1, null, 3]
for item in maybe
    if item
        -- item
    else
        -- "none"
. things = [Thing("a"), Thing("b")]
for thing in things
    -- thing.name
. floats = [1.5, 2.5]
for f in floats
    -- f

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ name String

────────────────────
+ This
    - name String
```

```output
3
7
1
none
3
a
b
1.5
2.5
```

### A list emptied while a loop reads it panics at the next read.

```yumi crash
. items = [1, 2, 3]
for item in items
    if item == 2
        items.clear()
    -- item
```

```output
1
2
```

### A loop over the characters of a text, with `when`, and nested in a loop over a range.

```yumi
. text = "héllo"
for char in text
    -- char
for char in text when char != 'l'
    -- char
. count = 0
for c in "abc"
    count += 1
-- count
for i in 0 .. < 3
    for c in "xy"
        -- "$i$c"
```

```output
h
é
l
l
o
h
é
o
3
0x
0y
1x
1y
2x
2y
```

### A loop over what a call gives, over a list written in its header, and over a slice of a list.

```yumi
. items = [3, 1, 2]
for item in make()
    -- item
for item in [10, 20]
    -- item
for item in items[1 .. < 3]
    -- item

━━━━━━━━━━━━━━━━━━━━
+ do make [String]

-- "made"
= ["x", "y"]
```

```output
made
x
y
10
20
1
2
```

### An error raised in the body of a loop over a list leaves the function.

```yumi crash
run([1, 0, 3])

━━━━━━━━━━━━━━━━━━━━
+ do run
    - items [Int]

for item in items
    -- item
    check(item)

━━━━━━━━━━━━━━━━━━━━
+ do check
    - item Int

?! Error("zero")
    if item == 0
```

```output
1
0
```

### A loop over a list of unions matched in its body, of texts with a loop over each, and of optional texts.

```yumi
. shapes = [Shape.circle(2), Shape.square(3), Shape.none]
for shape in shapes
    do match shape
        | .circle(r)
            -- "circle $r"
        | .square(side)
            -- "square $side"
        | .none
            -- "none"
. words = ["a", "bb"]
for word in words
    . doubled = "$word$word"
    -- doubled
    for letter in doubled
        -- letter
. opts [String?] = ["x", null]
for opt in opts
    -- opt ?? "nothing"
    if opt
        -- opt.length

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| circle Int
| square Int
| none
```

```output
circle 2
square 3
none
aa
a
a
bbbb
b
b
b
b
x
1
nothing
```

### An error a stream raises where a loop begins to read it is a panic at the loop.

```yumi crash
. bad Iterable'Int = Bad()
show(bad)

━━━━━━━━━━━━━━━━━━━━
+ do show
    - stream <Int>

for item in stream
    -- item

━━━━━━━━━━━━━━━━━━━━
+ Bad
    is Iterable'Int

────────────────────
+ <- this Int?

null

────────────────────
+ do begin-iterating ?! Error

?! Error("no start")

────────────────────
+ do end-iterating ?! Error

-- "ended"
```

```stderr
Unhandled panic: no start
<no file:9>

Task 1

Stack trace:
  0  __source__:show no file:9:4
  1  __source__:main no file:3:1
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### An error a stream raises where a loop has read it to its end is a panic at the loop.

```yumi crash
. feed Iterable'Int = Feed()
show(feed)

━━━━━━━━━━━━━━━━━━━━
+ do show
    - stream <Int>

for item in stream
    -- item
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ Feed
    is Iterable'Int

_ left = 3

────────────────────
+ do <- this Int?

if left == 0
    = null
left -= 1
= left

────────────────────
+ do begin-iterating ?! Error

-- "begun"

────────────────────
+ do end-iterating ?! Error

-- "ended"
?! Error("no end")
```

```stderr
Unhandled panic: no end
<no file:9>

Task 1

Stack trace:
  0  __source__:show no file:9:4
  1  __source__:main no file:3:1
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A loop whose pattern takes a tuple apart: written in parentheses and without, with a type written for a name, and with a name left out.

```yumi
. pairs = [(1i32, "a"), (2i32, "b")]
for (n, s) in pairs
    -- n
    -- s
for n, s in pairs
    -- "$n $s"
for (n Float, s) in pairs
    -- n
for (_, s) in pairs
    -- s
```

```output
1
a
2
b
1 a
2 b
1
2
a
b
```

### A loop that takes apart the pairs of a map, a list of tuples a function is given, and what a generator yields.

```yumi
. ages = Map'String'Int()
ages["ann"] = 3
ages["bob"] = 5
for key, value in ages
    -- "$key $value"
for (key, value) in ages
    -- key
. total = 0
for _, value in ages
    total += value
-- total
-- pairs-of([(1, 2), (3, 4)])
for a, b in walk()
    -- a + b

━━━━━━━━━━━━━━━━━━━━
_ do pairs-of Int
    - list [(Int, Int)]

. sum = 0
for (a, b) in list
    sum += a * b
= sum

━━━━━━━━━━━━━━━━━━━━
_ do walk <(Int, Int)>

for i in 1 .. 3
    <= (i, i * 10)
```

```output
ann 3
bob 5
ann
bob
8
14
11
22
33
```

### A loop that names fewer fields than the tuple has, one over tuples of three, and one whose field is itself a tuple.

```yumi
. triples = [(1, 2.5, "x"), (2, 3.5, "y")]
for a, b in triples
    -- a + b
for (a, b, c) in triples
    -- "$a $b $c"
. kept = 0
for (a Int64, _, c) in triples
    kept += a
    -- c
-- kept
. nested = [((1, 2), 3)]
for (p, q) in nested
    -- p
    -- q
```

```output
3.5
5.5
1 2.5 x
2 3.5 y
x
y
3
(1, 2)
3
```

### A loop that takes apart tuples of objects and of structs.

```yumi
. boxes = [(Box(1), 2), (Box(3), 4)]
for (box, n) in boxes
    box.size += n
    -- box.size
for (box, n) in boxes
    -- box.size * n
. points = [(Point(1, 2), "p"), (Point(3, 4), "q")]
for (point, name) in points
    -- point.x + point.y
    -- name

━━━━━━━━━━━━━━━━━━━━
+ Box

+ size Int

────────────────────
+ This
    - size Int

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
3
7
6
28
3
p
7
q
```

### A loop that takes a tuple apart with `break` and `continue`, with a type written for a name, and nested in another over a generator.

```yumi
. total = 0
for (a, b) in three()
    total += a
    if b == "stop"
        break
    if a == 1
        continue
    -- b
-- total
-- sums([(1, 2.5), (2, 4.5)])
. words = ["one", "two"]
for (index, word) in tagged(words)
    for (inner, other) in tagged(words)
        -- "$index$inner $word $other"

━━━━━━━━━━━━━━━━━━━━
_ do three <(Int, String)>

<= (1, "a")
<= (2, "b")
<= (3, "stop")
<= (4, "never")

━━━━━━━━━━━━━━━━━━━━
_ do sums Float
    - rows [(I32, Float)]

. sum = 0.0
for (count Float, each-value) in rows
    sum += count * each-value
= sum

━━━━━━━━━━━━━━━━━━━━
_ do tagged <(Int, String)>
    - items [String]

for index in 0 .. < items.length
    <= (index, items[index])
```

```output
b
6
11.5
00 one one
01 one two
10 two one
11 two two
```

### A loop that takes a tuple apart in a function that yields, and one with a clause that tests a name it takes.

```yumi
. pairs = [(1, "a"), (2, "b")]
for (n, s) in pairs
    -- n + 1
    -- s
for (n, s) in each()
    -- "$n$s"
for (n, s) in pairs when n > 1
    -- s

━━━━━━━━━━━━━━━━━━━━
_ do each <(Int, String)>

. pairs = [(1, "a"), (2, "b")]
for (n, s) in pairs
    <= (n * 2, s)
    <= (n * 3, s + s)
```

```output
2
a
3
b
2a
3aa
4b
6bb
b
```

### Generators of tuples, read by loops that take each apart: one that pairs two lists, one that numbers a list.

```yumi
for (a, b) in zip([1, 2, 3], ["x", "y", "z"])
    -- "$a$b"
for index, item in numbered(["p", "q"])
    -- "$index:$item"

━━━━━━━━━━━━━━━━━━━━
_ do zip <(Int, String)>
    - left [Int]
    - right [String]

for i in 0 .. < left.length
    . (l, r) = (left[i], right[i])
    <= (l, r)

━━━━━━━━━━━━━━━━━━━━
_ do numbered <(Int, String)>
    - items [String]

. index = 0
for item in items
    <= (index, item)
    index += 1
```

```output
1x
2y
3z
0:p
1:q
```

### A panic under a loop that takes a tuple apart.

```yumi crash
. pairs = [(1, 1), (2, 0)]
for (n, d) in pairs
    -- n / check(d)

━━━━━━━━━━━━━━━━━━━━
_ do check Int
    - d Int

if d == 0
    ?! Panic("zero")
= d
```

```stderr
Unhandled panic: zero
<no file:11>

Task 1

Stack trace:
  0  __source__:check no file:11:5
  1  __source__:main no file:4:12
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A loop over the characters of a text, and over a map by its entries and by key and value.

```yumi
for ch in "héy"
    -- ch
. map = Map'String'Int()
map["a"] = 1
map["b"] = 2
for entry in map
    -- entry
for key, value in map
    -- "$key=$value"
```

```output
h
é
y
(b, 2)
(a, 1)
b=2
a=1
```

### A loop over an array held in a local reads the local at each item: an array assigned to it in the body is read from there on.

```yumi
. arr = Array'Int(3)
. other = Array'Int(3)
other[0] = 7
other[1] = 8
other[2] = 9
for a in arr
    -- a
    arr = other
```

```output
0
8
9
```

### A loop over a range of floats held in a local steps through the range the local holds at each item.

```yumi
. r = 0.0 .. 1.0 by 0.5
for x in r
    -- x
    r = 10.0 .. 11.0 by 0.5
```

```output
0
10.5
11
```

### A loop that steps a cursor through a struct sees a field of the struct assigned in its body.

```yumi
. c = Cur(2)
for v in c
    -- v
    c.limit = 4

━━━━━━━━━━━━━━━━━━━━
+ Cur is Struct
    is CursorIterable'Int'Int

+ limit Int

────────────────────
+ get initial-iterator-state Int

0

────────────────────
+ do next-iterator (Int, Int?)
    - state Int

. none Int?
if state >= limit
    = (state, none)
= (state + 1, state * 10)
```

```output
0
10
20
30
```

### A loop over a struct that is a property of an object sees the property's field assigned, and the property assigned whole.

```yumi
. box = Holder(Cur(2))
for v in box.cur
    -- v
    box.cur.limit = 4
. other = Holder(Cur(2))
for v in other.cur
    -- v
    other.cur = Cur(4)

━━━━━━━━━━━━━━━━━━━━
+ Cur is Struct
    is CursorIterable'Int'Int

+ limit Int

────────────────────
+ get initial-iterator-state Int

0

────────────────────
+ do next-iterator (Int, Int?)
    - state Int

. none Int?
if state >= limit
    = (state, none)
= (state + 1, state * 10)

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ cur Cur

────────────────────
+ This
    - cur Cur
```

```output
0
10
20
30
0
10
20
30
```

### A loop that counts through a tuple reads the tuple assigned in its body, with the length it had when the loop began.

```yumi
. s = Span(2)
for item in s
    -- item
    s = Span(4)

━━━━━━━━━━━━━━━━━━━━
+ Span is Tuple
    is Indexable'Int

+ size Int

────────────────────
+ get length Size

size

────────────────────
+ this[index Index] Int

index * 10 + size

────────────────────
+ this[range Range'Index] Indexable'Int

this
```

```output
2
14
```

### A loop over an array that is an argument reads the argument assigned in its body.

```yumi
-- run(Array'Int(3))

━━━━━━━━━━━━━━━━━━━━
+ do run Int
    - arr Array'Int

. other = Array'Int(4)
other[1] = 8
other[2] = 9
other[3] = 10
. sum = 0
for a in arr
    sum += a
    arr = other
= sum
```

```output
17
```

### A loop over a range of whole numbers held in a local is not changed by an assignment to the local.

```yumi
. r = 0 .. 2
for x in r
    -- x
    r = 10 .. 12
. q = 0 .. < 3
for x in q
    -- x
    q = 5 .. < 9
```

```output
0
1
2
0
1
2
```

### A loop over an array a class stores, read by its name alone and through `this`, reads the property assigned in its body.

```yumi
. box = Box()
box.run()
box.swap()

━━━━━━━━━━━━━━━━━━━━
+ Box

+ items = make(1)

────────────────────
+ do run

for item in items
    -- item
    items = make(7)

────────────────────
+ do swap

for item in this.items
    -- item
    this.items = make(20)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
8
9
7
21
22
```

### A loop over an array of another object reads the property assigned in its body, and stays with the object it began with where the local is assigned another.

```yumi
. box = Box()
for item in box.items
    -- item
    box.items = make(7)
. second = Box()
for item in second.items
    -- item
    second = Box()
    second.items = make(30)

━━━━━━━━━━━━━━━━━━━━
+ Box

+ items = make(1)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
8
9
1
2
3
```

### A loop over an array that is a field of a struct sees the field assigned, and the struct assigned whole.

```yumi
. s = Wrap(make(1), 0)
for item in s.items
    -- item
    s.items = make(7)
. t = Wrap(make(1), 0)
for item in t.items
    -- item
    t = Wrap(make(40), 1)

━━━━━━━━━━━━━━━━━━━━
+ Wrap is Struct

+ items Array'Int
+ tag Int

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
8
9
1
41
42
```

### A loop over an array a closure assigns reads what the closure stored.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

. arr = make(1)
. set = => do
    arr = make(60)
for item in arr
    -- item
    set()

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
61
62
```

### A loop over an optional argument known to hold an array reads the argument assigned in its body.

```yumi
run(make(1))

━━━━━━━━━━━━━━━━━━━━
+ do run
    - arr Array'Int?

if arr
    for item in arr
        -- item
        arr = make(80)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
81
82
```

### A loop over a text or a list held in a local is not changed by an assignment to the local.

```yumi
. text = "abc"
for ch in text
    -- ch
    text = "xyz"
. list = [1, 2, 3]
for item in list
    -- item
    list = [7, 8, 9]
```

```output
a
b
c
1
2
3
```

### In a function that yields, a loop over an array keeps the array it began with.

```yumi
for v in gen()
    -- v

━━━━━━━━━━━━━━━━━━━━
+ do gen <Int>

. arr = make(1)
for item in arr
    <= item
    arr = make(90)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
2
3
```

### A loop in a closure's body over the closure's own array reads the local assigned in its body.

```yumi
. run = => do
    . arr = make(1)
    for item in arr
        -- item
        arr = make(100)
run()

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
101
102
```

### A loop over an array changed in place, and over an array that is a field of a tuple assigned whole in the body.

```yumi
. arr = make(1)
for item in arr
    -- item
    arr[2] = 99
. pair = (make(1), 5)
for item in pair[0]
    -- item
    pair = (make(30), 6)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
2
99
1
31
32
```

### A loop over a cast reads the array the cast gives.

```yumi
. text = "a\nb"
. count = 0
for byte in text as Byte[]
    if byte == 10u8
        count += 1
-- count
```

```output
1
```

### Loops over arrays one in another, each assigned in the inner body.

```yumi
. arr = make(1)
. other = make(10)
for item in arr when item > 0
    -- item
    for inner in other
        -- inner
        other = make(20)
    arr = other

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
10
21
22
21
20
21
22
22
20
21
22
```

### A loop over an array of an object with `continue` and `break`, refilled by a method in its body, and loops over a property of an object a call makes, which is called once.

```yumi
. box = Box()
for item in box.items
    if item == 9
        continue
    -- item
    box.refill(8)
    if item == 10
        break
for item in fresh().items
    -- item
for item in fresh().wrap.items
    -- item

━━━━━━━━━━━━━━━━━━━━
+ Box

+ items = make(1)
+ wrap = Wrap(make(100), 0)

────────────────────
+ do refill
    - first Int

items = make(first)

━━━━━━━━━━━━━━━━━━━━
+ Wrap is Struct

+ items Array'Int
+ tag Int

━━━━━━━━━━━━━━━━━━━━
+ do fresh Box

-- "made a box"
= Box()

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
10
made a box
1
2
3
made a box
100
101
102
```

### A loop over an array of an object the local no longer names, while its body allocates enough for collections to run.

```yumi
. second = Box()
. total = 0
for item in second.items
    total += item
    second = Box()
    second.items = make(30)
    for round in 0 .. < 2000
        . junk = [round, item, total]
        total += junk.length - 3
    force-collection()
-- total

━━━━━━━━━━━━━━━━━━━━
+ do force-collection

. keep [[Int]]
for n in 0 .. < 3000
    keep += [n, n, n, n]

━━━━━━━━━━━━━━━━━━━━
+ Box

+ items = make(1)
+ wrap = Wrap(make(100), 0)

────────────────────
+ do refill
    - first Int

items = make(first)

━━━━━━━━━━━━━━━━━━━━
+ Wrap is Struct

+ items Array'Int
+ tag Int

━━━━━━━━━━━━━━━━━━━━
+ do fresh Box

-- "made a box"
= Box()

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
6
```

### A loop that names the fields of pairs in an array, with a clause, reads the array assigned in its body.

```yumi
. pairs = Array'(Int, String)(2)
pairs[0] = (1, "one")
pairs[1] = (2, "two")
. other = Array'(Int, String)(2)
other[0] = (7, "seven")
other[1] = (8, "eight")
for (number, name) in pairs when number > 0
    -- "$number $name"
    pairs = other
```

```output
1 one
8 eight
```

### A closure in a function that yields loops over an array of that function and reads it assigned.

```yumi
for v in gen()
    -- v

━━━━━━━━━━━━━━━━━━━━
+ do gen <Int>

. arr = make(1)
. each-one = => do
    . total = 0
    for item in arr
        total += item
        arr = make(10)
    = total
<= each-one()
<= arr[0]

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
24
10
```

### A loop over a lazy local that one read evaluates keeps what the read gave: the body assigns the local it was declared with, and the loop does not see it.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

. other = make(1)
.. arr = other
for item in arr
    -- item
    other = make(70)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
2
3
```

### A loop over a lazy local the body assigns reads what was assigned at the next item.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

.. arr = make(1)
for item in arr
    -- item
    arr = make(70)
-- arr[0]

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
71
72
70
```

### A loop over a lazy local declared in a loop and read twice keeps the value its first read stored.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

for turn in 0 .. < 2
    . other = make(turn * 10)
    .. arr = other
    for item in arr
        -- item
        other = make(70)
    -- arr[0]

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
0
1
2
0
10
11
12
10
```

### A loop over a lazy local a closure assigns reads what the closure stored.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

.. arr = make(1)
. change = => arr = make(70)
for item in arr
    -- item
    change()

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
71
72
```

### A loop over a lazy local declared as a property of an object keeps the value: the body assigns the property, and the loop does not see it.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

. holder = Holder()
.. arr = holder.items
for item in arr
    -- item
    holder.items = make(70)

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ items = make(1)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
2
3
```

### A loop over a lazy range of numbers the body assigns keeps the range it began with.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

.. r = 0 .. < 3
for x in r
    -- x
    r = 5 .. < 9
-- r.start
```

```output
0
1
2
5
```

### A loop over a lazy local read again after it evaluates the value once.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

. count = 0
.. arr = noisy(1)
for item in arr
    -- item
-- arr[1]

━━━━━━━━━━━━━━━━━━━━
+ do noisy Array'Int
    - first Int

-- "made"
= make(first)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
made
1
2
3
2
```

### A loop over a lazy local declared in a loop evaluates the value once in each turn.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

for turn in 0 .. < 2
    .. arr = noisy(turn * 10)
    for item in arr
        -- item
    -- arr[1]

━━━━━━━━━━━━━━━━━━━━
+ do noisy Array'Int
    - first Int

-- "made"
= make(first)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
made
0
1
2
1
made
10
11
12
11
```

### A lazy local whose value panics is evaluated where a loop over it begins, and the program ends by the panic.

```yumi crash
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

.. arr = boom(1)
for item in arr
    -- item
-- arr[0]

━━━━━━━━━━━━━━━━━━━━
+ do boom Array'Int
    - first Int

. items = [1, 2, 3]
-- items[5]
= make(first)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

### A loop over a lazy local a closure reads evaluates the value once, for the loop and the closure.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

.. arr = noisy(1)
. peek = => arr[1]
for item in arr
    -- item
    -- peek()
-- peek()

━━━━━━━━━━━━━━━━━━━━
+ do noisy Array'Int
    - first Int

-- "made"
= make(first)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
made
1
2
2
2
3
2
2
```

### A loop over a lazy local read before it finds the value evaluated, and reads what its body assigns.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

.. arr = noisy(1)
-- arr[0]
for item in arr
    -- item
    arr = make(30)
-- arr[0]

━━━━━━━━━━━━━━━━━━━━
+ do noisy Array'Int
    - first Int

-- "made"
= make(first)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
made
1
1
31
32
30
```

### A loop over a lazy local whose body reads and assigns it.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

.. arr = make(1)
for item in arr
    -- item
    -- arr[2]
    if item == 1
        arr = make(30)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
3
31
32
32
32
```

### A loop over a lazy optional local a test found a value in reads what its body assigns.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

.. arr Array'Int? = make(1)
if arr
    for item in arr
        -- item
        arr = make(80)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
81
82
```

### Two loops over one lazy local, one in the other, each read what the outer body assigns.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

.. arr = make(1)
for item in arr
    for other in arr
        -- item * 100 + other
    arr = make(5)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
101
102
103
605
606
607
705
706
707
```

### A loop over a lazy local declared as a property keeps its value where the property is assigned, and reads what is assigned to the local.

```yumi
. box = Box()
box.run()

━━━━━━━━━━━━━━━━━━━━
+ Box

+ items = make(1)

────────────────────
+ do run

.. mine = items
for item in mine
    -- item
    items = make(20)
.. kept = items
for item in kept
    -- item
    kept = make(90)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
1
2
3
20
91
92
```

### A loop over a lazy local a branch may have read evaluates the value where it was not.

```yumi
run(true)
run(false)

━━━━━━━━━━━━━━━━━━━━
+ do run
    - early Bool

.. arr = noisy(1)
if early
    -- arr[0]
for item in arr
    -- item
    arr = make(30)

━━━━━━━━━━━━━━━━━━━━
+ do noisy Array'Int
    - first Int

-- "made"
= make(first)

━━━━━━━━━━━━━━━━━━━━
+ do make Array'Int
    - first Int

. made = Array'Int(3)
made[0] = first
made[1] = first + 1
made[2] = first + 2
= made
```

```output
made
1
1
31
32
made
1
31
32
```

### A second header with a clause of its own, over a list and a range

```yumi
. names = ["a", "b", "c"]
for name in names when name != "b"
and count in 5 .. 9 when count != 6
    -- "$name $count"
```

```output
a 5
c 7
```

### Three headers with continue and break, a range that counts down, and clauses that end the loop

```yumi
for i in 0 .. < 6
and j in 10 .. < 20
and k in ["a", "b", "c", "d", "e"]
    if i == 1
        continue
    if i == 4
        break
    -- "$i $j $k"
-- "done"
for i in 5 .. 1
and j in 1 .. 3
    -- i, j
for i in 0 .. < 4 until i == 2
and j in 0 .. < 9 while j < 5
    -- i, j
```

```output
0 10 a
2 12 c
3 13 d
done
(5, 1)
(4, 2)
(3, 3)
(0, 0)
(1, 1)
```

### What a loop of two headers reads of its containers, and in what order

```yumi
. loud = Loud("a", [1, 2, 3])
. other = Loud("b", [10, 20, 30, 40])
for x in loud
and y in other when y != 20
    -- "$x $y"
-- "end"

━━━━━━━━━━━━━━━━━━━━
+ Loud
    is Indexable'Int

_ name String
_ values List'Int

────────────────────
+ This
    - name
    - items [Int]

values = items

────────────────────
+ get length Size

-- "$name length"
values.length

────────────────────
+ this[index Index] Int

-- "$name read $index"
values[index]

────────────────────
+ this[range Range'Index] Loud

Loud(name, values[range])
```

```output
a length
b length
a read 0
b read 0
1 10
a read 1
b read 1
b read 2
2 30
a read 2
b read 3
3 40
end
```

### A loop of two headers over streams begins and ends each in order

```yumi
. first = Counter("a", 3)
. second = Counter("b", 5)
for x in first
and y in second
    -- "$x $y"
    if x == 2
        break
-- "after"
for x in Counter("c", 2)
and y in [7, 8, 9]
    -- "$x $y"
-- "end"

━━━━━━━━━━━━━━━━━━━━
+ Counter
    is Iterable'Int

_ name String
_ limit Int
_ at = 0

────────────────────
+ This
    - name
    - limit

────────────────────
+ do begin-iterating

-- "$name begins"

────────────────────
+ do end-iterating

-- "$name ends"

────────────────────
+ do <- this

if at < limit
    at += 1
    = at
```

```output
a begins
b begins
1 1
2 2
a ends
b ends
after
c begins
1 7
2 8
c ends
end
```

### Closures, a tuple taken apart and the item unnamed in loops of two headers

```yumi
. made = List'(Getter'String)()
for name in ["a", "b", "c"]
and count in [1, 2, 3] when count != 2
    made += => "$name$count"
for make in made
    -- make()
. pairs = [(1, "x"), (2, "y")]
for letter in ["p", "q"]
and number, word in pairs
    -- "$letter $number $word"
for in ["u", "v"]
and n in 0 .. < 5
    -- "$each $it $n"
```

```output
a1
b3
p 1 x
q 2 y
u u 0
v v 1
```

### Loops that match pass over what does not fit: a class, an optional, a tuple, a list

```yumi
. mixed [Shape] = [Circle(1) as Shape, Square(2) as Shape, Circle(3) as Shape]
for | c Circle in mixed
    -- c.radius
for | s Square in mixed when s.side > 1
    -- s.side
. maybe [Int?] = [1, null, 3]
for | n Int in maybe
    -- n
. pairs = [(1, "a"), (2, "b"), (3, "c")]
for | n {it != 2}, word in pairs
    -- "$n $word"
. lists = [[1, 2], [3], List'Int(), [4, 5, 6]]
for | [first, ...rest] in lists
    -- "$first ${rest.length}"
for | [_, second, ...] in lists
and tag in ["x", "y", "z"]
    -- "$second $tag"

━━━━━━━━━━━━━━━━━━━━
+ Shape is Interface

━━━━━━━━━━━━━━━━━━━━
+ Circle
    is Shape

+ radius Int

────────────────────
+ This
    - radius

━━━━━━━━━━━━━━━━━━━━
+ Square
    is Shape

+ side Int

────────────────────
+ This
    - side
```

```output
1
3
2
1
3
1 a
3 c
1 1
3 0
4 2
2 x
5 y
```

### Loops of two headers and loops that match in functions that yield

```yumi
for total in sums([1, 2, 3], [10, 20, 30, 40])
    -- total
for word in picked(["a", "bb", "ccc", "dd"])
    -- word
-- ", ".join(each * 2 in [1, 2, 3] when each != 2)

━━━━━━━━━━━━━━━━━━━━
+ do sums <Int>
    - left [Int]
    - right [Int]

for a in left
and b in right when b != 20
    <= a + b

━━━━━━━━━━━━━━━━━━━━
+ do picked <String>
    - words [String]

for | word {it.length == 2} in words
    <= word
```

```output
11
32
43
bb
dd
2, 6
```

### Closures made in a loop of two headers and in a loop that matches keep their turn

```yumi
. seen = List'String()
. callbacks = List'Doer()
for item in ["a", "b", "c"]
and size in [1, 2, 3]
    callbacks += => do seen += "$item$size"
for callback in callbacks
    callback()
-- ", ".join(seen)
. shapes [Shape] = [Circle(1) as Shape, Square(2) as Shape, Circle(3) as Shape]
. getters = List'(Getter'Int)()
for | c Circle in shapes
    getters += => c.radius
for getter in getters
    -- getter()

━━━━━━━━━━━━━━━━━━━━
+ Shape is Interface

━━━━━━━━━━━━━━━━━━━━
+ Circle
    is Shape

+ radius Int

────────────────────
+ This
    - radius

━━━━━━━━━━━━━━━━━━━━
+ Square
    is Shape

+ side Int

────────────────────
+ This
    - side
```

```output
a1, b2, c3
1
3
```

### A header over nothing ends a loop of two, and headers of other integer types

```yumi
. empty = List'Int()
for a in empty
and b in [1, 2]
    -- "never"
for a in [1, 2]
and b in empty
    -- "never"
. words = ["x", "y"]
for i U8 in 0 .. < 3
and j I64 in 5 .. 7
    -- i + 0, j
for word in words
and index in 0 .. < words.length
    -- "$index $word"
-- "end"
```

```output
(0, 5)
(1, 6)
(2, 7)
0 x
1 y
end
```

### The trace of a cast in a loop to a class an item is not of

```yumi crash
for item in [Foo2(), Foo(), Foo2()]
    -- item as Foo2

+ Foo
+ Foo2 is Foo
```

```stderr
Unhandled panic: Invalid cast to [Object __source__:Foo2]
<no file:3>

Task 1

Stack trace:
  0  __source__:main no file:3:8
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A comprehension that keeps the items a pattern fits gives the items themselves

```yumi
. odd = <* {it % 2} in 0 .. < 10
-- type-of odd
-- ", ".join(odd)
. big = <* {it > 2} in [1, 2, 3, 4]
-- type-of big
for item in big
    -- item
. words = <* {it.length > 1} in ["a", "bb", "", "ccc"]
-- type-of words
-- "/".join(words)
. none = <* {it > 9} in [1, 2, 3]
-- ", ".join(none)
-- "end"
```

```output
[TypeInfo Generator'I64]
1, 3, 5, 7, 9
[TypeInfo Generator'I64]
3
4
[TypeInfo Generator'String]
bb/ccc

end
```

### A comprehension that keeps what fits, written over lines, in a function that yields, and read by a loop

```yumi
. kept = in [5, 1, 7, 2, 9]
    <* {it > 4}
-- type-of kept
. total = 0
for item in kept
    total += item
-- total
. names = in ["ann", "bo", "cy", "dee"]
    <* {it.length == 3}
-- ", ".join(names)
-- ", ".join(picked([3, 8, 4, 10], 5))

━━━━━━━━━━━━━━━━━━━━
+ do picked <Int>
    - items [Int]
    - floor Int

. passing = <* {it > floor} in items
for item in passing
    <= item * 2
```

```output
[TypeInfo Generator'I64]
21
ann, dee
16, 20
```

### Loop over a base class begins and ends by the derived class

```yumi
. base Base = Derived()
for a in base
    -- a

━━━━━━━━━━━━━━━━━━━━
+ Base
    is Iterable'Int

_ at = 0

────────────────────
+ do begin-iterating

-- "base begins"

────────────────────
+ do end-iterating

-- "base ends"

────────────────────
+ do <- this Int?

if at < 2
    at += 1
    = at
= null

━━━━━━━━━━━━━━━━━━━━
+ Derived is Base

────────────────────
+ do begin-iterating

-- "derived begins"

────────────────────
+ do end-iterating

-- "derived ends"
```

```output
derived begins
1
2
derived ends
```

### Cursor loop over a base class steps by the derived class

```yumi
. base Base = Derived()
for a in base
    -- a

━━━━━━━━━━━━━━━━━━━━
+ Base
    is CursorIterable'Int'Int

────────────────────
+ get initial-iterator-state Int

0

────────────────────
+ do next-iterator (Int, Int?)
    - state Int

. none Int?
if state >= 2
    = (state, none)
= (state + 1, state + 1)

━━━━━━━━━━━━━━━━━━━━
+ Derived is Base

────────────────────
+ do next-iterator (Int, Int?)
    - state Int

. none Int?
if state >= 3
    = (state, none)
= (state + 1, state * 100)
```

```output
0
100
200
```

### Index loop over a base class reads by the derived class

```yumi
. base Base = Derived()
for a in base
    -- a

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
5
105
```

### each and it are the first header's item

```yumi
for in ["u", "v"]
and in [1, 2]
    -- "$each $it"
```

```output
u u
v v
```

### each and it of the first of three headers

```yumi
for in ["u", "v"]
and b in [1, 2]
and in [true, false]
    -- "$each $it $b"
```

```output
u u 1
v v 2
```

### A panic under the read of a loop's second header is placed at the first header, where the loop reads every header.

```yumi crash
drain()

━━━━━━━━━━━━━━━━━━━━
+ do a

<= 1
<= 2
<= 3

━━━━━━━━━━━━━━━━━━━━
+ do b

<= 10
?! Panic("zip panic")

━━━━━━━━━━━━━━━━━━━━
+ do drain

for x in a()
and y in b()
    -- x + y
```

```stderr
Unhandled panic: zip panic
<no file:15>

Task 1

Stack trace:
  0  __source__:b no file:15:1
  1  core/stream:Generator:<-read core/stream.yumi
  2  __source__:drain no file:20:4
  3  __source__:main no file:2:1
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### An error raised under the read of a loop's second header leaves the loop at the first header.

```yumi crash
drain()

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ do b

<= 10
?! Problem("zip same line")

━━━━━━━━━━━━━━━━━━━━
+ do drain

for x in 0 .. < 5
and y in b()
    -- x + y
```

```stderr
Unhandled error: zip same line
<no file:11>

Task 1

Stack trace:
  0  __source__:b no file:11:1
  1  core/stream:Generator:<-read core/stream.yumi
  2  __source__:drain no file:16:4
  3  __source__:main no file:2:1
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A generator read by a loop's second header that panics on a bad index is placed at the first header.

```yumi crash
drain()

━━━━━━━━━━━━━━━━━━━━
+ do b

<= 10
. list = [1]
<= list[3]

━━━━━━━━━━━━━━━━━━━━
+ do drain

for x in 0 .. < 5
and y in b()
    -- x + y
```

```stderr
Unhandled panic: List index 3 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:b no file:9:4
  2  core/stream:Generator:<-read core/stream.yumi
  3  __source__:drain no file:14:4
  4  __source__:main no file:2:1
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

```yumi
. input = [1, 2, 3]
. items-0 [Int] = <* value in input when value > 0
-- items-0.length
. items-1 [Int] = <* value in input when value > 1
-- items-1.length
. items-2 [Int] = <* value in input when value > 2
-- items-2.length
. items-3 [Int] = <* value in input when value > 3
-- items-3.length
```

```output
3
2
1
0
```

```yumi
sequence([1, 2, 3])

━━━━━━━━━━━━━━━━━━━━
+ do sequence
    - input [Int]

. items-0 [Int] = <* value in input
    checked(value)
-- items-0.length
. items-1 [Int] = <* value in input
    checked(value)
-- items-1.length
. items-2 [Int] = <* value in input
    checked(value)
-- items-2.length
. items-3 [Int] = <* value in input
    checked(value)
-- items-3.length

━━━━━━━━━━━━━━━━━━━━
+ do checked Int
    - value Int

if value < 0
    ?! Error("negative")
= value
```

```output
3
3
3
3
```

```yumi crash
sequence([-1])

━━━━━━━━━━━━━━━━━━━━
+ do sequence
    - input [Int]

. items-0 [Int] = <* value in input
    checked(value)
-- items-0.length
. items-1 [Int] = <* value in input
    checked(value)
-- items-1.length
. items-2 [Int] = <* value in input
    checked(value)
-- items-2.length
. items-3 [Int] = <* value in input
    checked(value)
-- items-3.length

━━━━━━━━━━━━━━━━━━━━
+ do checked Int
    - value Int

if value < 0
    ?! Error("negative")
= value
```

```stderr
Unhandled error: negative
<no file:26>

Task 1

Stack trace:
  0  __source__:checked no file:26:5
  1  __source__:sequence no file:9:5
  2  __source__:main no file:2:1
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

```yumi
for i in 0 .. < 5
    . v = match i
        |= 1
            -- "skip"
            continue
        |= 3
            break
        ??
            i * 10
    -- v
```

```output
0
skip
20
```

### A loop's pattern that names `each` binds the name itself, not the loop's item.

```yumi
. pairs [(String, Stored)]
pairs += ("a", Stored("first", 1))
for key, each in pairs
    -- "$key ${each.label} ${each.count}"
. stored [String = Stored]
stored["b"] = Stored("second", 2)
for key, each in stored
    -- "$key ${each.label}"

━━━━━━━━━━━━━━━━━━━━
+ Stored is Tuple

+ label String
+ count I64
```

```output
a first 1
b second
```

### The clauses of a loop are tested in the order written, and the first that decides wins

```yumi
for i in 0 .. < 10 when i % 2 == 0 while i < 7
    -- "range $i"
for i in 0 .. < 6 when i != 2 until i == 2
    -- "skip $i"
. names = ["a", "bb", "ccc", "dddd", "e"]
for name in names while name.length < 4 when name.length != 2
    -- "list $name"
```

```output
range 0
range 2
range 4
range 6
skip 0
skip 1
skip 3
skip 4
skip 5
list a
list ccc
```

### A `break` outside a loop is an error

```yumi crash
break
-- 1
```

```output
Exception: `break` outside a loop
<no file:2>
    break
    ^^^^^
```

### A `continue` outside a loop is an error

```yumi crash
-- f()

━━━━━━━━━━━━━━━━━━━━
+ do f Int

continue
= 7
```

```output
Exception: `continue` outside a loop
<no file:7>
    continue
    ^^^^^^^^
```

### A function written in a loop is a function: a `break` in it is outside a loop

```yumi crash
for i in 0 .. < 3
    . leave = => do
        break
    leave()
```

```output
Exception: `break` outside a loop
<no file:4>
            break
            ^^^^^
```
