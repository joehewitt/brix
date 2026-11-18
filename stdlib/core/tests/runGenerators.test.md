# Generators

```yumi
. gen = stuff(42)
-- gen

━━━━━━━━━━━━━━━━━━━━
+ do stuff
    - a

<= a
```

```output
[Function Generator'I64]
```

```yumi
. source = EntrySource()
. names = each.text in source.entries when each.text != ""
-- tally(names)

━━━━━━━━━━━━━━━━━━━━
+ Entry

+ text String

────────────────────
+ This
    - text String

━━━━━━━━━━━━━━━━━━━━
+ EntryProblem is Error

━━━━━━━━━━━━━━━━━━━━
+ EntrySource

────────────────────
+ do get entries <Entry>

?! EntryProblem("failed")
    if false
<= Entry("a")

━━━━━━━━━━━━━━━━━━━━
+ do tally'Item Int
    - stream Iterable'Item

. count = 0
for item in stream
    count += 1
= count
```

```output
1
```

### A raising generator passed as an Iterable panics when it raises, since the interface carries no error type.

```yumi crash
. source = EntrySource()
-- tally(source.entries)

━━━━━━━━━━━━━━━━━━━━
+ EntryProblem is Error

━━━━━━━━━━━━━━━━━━━━
+ EntrySource

────────────────────
+ do get entries <String>

<= "a"
?! EntryProblem("failed")

━━━━━━━━━━━━━━━━━━━━
+ do tally'Item Int
    - stream Iterable'Item

. count = 0
for item in stream
    -- item
    count += 1
= count
```

```output
a
```

### A generator can publish an event between yields.

```yumi
. source = PublishingGeneratorSource()
for value on source.changed
    -- "got $value"
for n in source.numbers()
    -- "n $n"
-- "done"

━━━━━━━━━━━━━━━━━━━━
+ PublishingGeneratorSource

+ event changed Int

────────────────────
+ do numbers <Int>

changed <- 1
<= 10
changed <- 2
<= 20
```

```output
got 1
n 10
got 2
n 20
done
```

### A generator survives erased event publications between yields.

```yumi
. source = ErasedPublishingGeneratorSource()
for n in source.numbers()
    -- "n $n"
-- "done"

━━━━━━━━━━━━━━━━━━━━
+ ErasedPublishingGeneratorSource

+ event changed Int

────────────────────
+ do numbers <Int>

changed <- 1
<= 10
changed <- 2
<= 20
```

```output
n 10
n 20
done
```

### Virtual generator overrides preserve the base method's error-result ABI.

```yumi
_ generator-cleanups = 0

. source EventSource = QuietEventSource()
. result = catch collect-events(source)
if result is [Int]
    . values = result as [Int]
    -- "count ${values.length}"
    -- "first ${values[0]}"
else
    -- "unexpected error"

. failed = catch collect-events(EventSource())
-- "base error ${failed is EventProblem}"

for value in cleanup-items()
    break
-- "cleanup count $generator-cleanups"

━━━━━━━━━━━━━━━━━━━━
+ EventProblem is Error

━━━━━━━━━━━━━━━━━━━━
+ EventSource

────────────────────
+ do events <Int> ?! EventProblem

<= 1
?! EventProblem("base")

━━━━━━━━━━━━━━━━━━━━
+ QuietEventSource is EventSource

────────────────────
+ do events <Int>

<= 2

━━━━━━━━━━━━━━━━━━━━
_ do collect-events [Int] ?! EventProblem
    - source EventSource

. values [Int] = List'Int()
for value in source.events()
    values += value
= values

━━━━━━━━━━━━━━━━━━━━
_ do cleanup-items <Int>

do finally
    generator-cleanups += 1
<= 1
<= 2
```

```output
count 1
first 2
base error true
cleanup count 1
```

```yumi
. gen = stuff(42)
-- gen
-- gen()
-- gen()
-- gen()
-- gen()

━━━━━━━━━━━━━━━━━━━━
+ do stuff
    - a

<= a
-- "hi"
<= a + 1
-- "bye"
```

```output
[Function Generator'I64]
42
hi
43
bye
unknown
unknown
```

```yumi
. gen = stuff()
-- gen()
-- gen()
-- gen()

━━━━━━━━━━━━━━━━━━━━
+ do stuff

. a = 1
. n = 10 + a
<= n
n = 20 + a
<= n
```

```output
11
21
unknown
```

```yumi
. mapper = map(3) • cb each * 2
-- mapper()
-- mapper()
-- mapper()
-- mapper()

━━━━━━━━━━━━━━━━━━━━
+ do map
    - n
    - cb

. index = 0
while index < n
    <= cb(index)
    index += 1
```

```output
0
2
4
unknown
```

```yumi
for a in gen()
    -- a

━━━━━━━━━━━━━━━━━━━━
+ do gen

<= 1
<= 2
<= 3
```

```output
1
2
3
```

```yumi
. genny = gen()
-- type-of genny
for a in gen()
    -- a

━━━━━━━━━━━━━━━━━━━━
+ do gen <Int>

<= 1
<= 2
<= 3
```

```output
[TypeInfo Iterable'I64]
1
2
3
```

```yumi
for a in gen() while a < 3
    -- a

━━━━━━━━━━━━━━━━━━━━
+ do gen

<= 1
<= 2
<= 3
```

```output
1
2
```

```yumi
for a in gen() when a != 2
    -- a

━━━━━━━━━━━━━━━━━━━━
+ do gen

<= 1
<= 2
<= 3
```

```output
1
3
```

```yumi
for a in gen2()
    -- a

━━━━━━━━━━━━━━━━━━━━
+ do gen1

<= 1
<= 2
<= 3

━━━━━━━━━━━━━━━━━━━━
+ do gen2

for a in gen1()
    <= a + 1
```

```output
2
3
4
```

```yumi
for a in foo(bar())
    -- a

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - stream

. accum = 1
for item in stream
    <= 1
accum = 2

━━━━━━━━━━━━━━━━━━━━
+ do bar

<= 1
```

```output
1
```

### Generator as method

```yumi
. thing = Thing()
for a in thing.foo(1)
    -- a

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ do foo
    - n

<= n + 1
-- this
<= n + 2
```

```output
2
[Object __source__:Thing]
3
```

### Generator as closure

```yumi
. outer = 1
. gen = => do
    <= outer
    <= outer

for a in gen()
    -- a
    outer += 1
```

```output
1
2
```

```yumi
. outer = 1
. gen = :: n => do
    <= outer + n

for a in gen(1)
    -- a
```

```output
2
```

```yumi
. gen = :: iterable => do
    for i in iterable
        <= i

for a in gen([1,2,3])
    -- a
```

```output
1
2
3
```

### Generator with do finally

```yumi
for a in foo()
    -- a

━━━━━━━━━━━━━━━━━━━━
+ do foo

do finally
    -- "bye"

<= 1
<= 2
```

```output
1
2
bye
```

### Generator with explicit return

```yumi
for a in foo()
    -- a

━━━━━━━━━━━━━━━━━━━━
+ do foo

do finally
    -- "bye"

<= 1
=
<= 3
```

```output
1
bye
```

### Generator can return early and return expression is evaluated but discarded

```yumi
for n in foo()
    -- n

━━━━━━━━━━━━━━━━━━━━
+ do foo

<= 1
= say()

━━━━━━━━━━━━━━━━━━━━
+ do say

-- "say"
= 42
```

```output
1
say
```

### Generator with explicit return in nested scope

```yumi
for a in foo(1)
    -- a

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - a

do finally
    -- "bye"
<= 1
if a
    =
<= 3
```

```output
1
bye
```

```yumi
. f = foo()
. g = f as Iterable'Int
for n in g
    -- n

━━━━━━━━━━━━━━━━━━━━
+ do foo

<= 1
```

```output
1
```

```yumi
. thing = Thing'Int()
for n in thing
    -- n

━━━━━━━━━━━━━━━━━━━━
+ Thing'T

────────────────────
+ do to Iterable'T

<= 1
```

```output
1
```

```yumi
. thing = Thing'Int(1)
for n in thing
    -- n

━━━━━━━━━━━━━━━━━━━━
+ Thing'T is Tuple

+ start T

────────────────────
+ do to Iterable'T

<= this.start
```

```output
1
```

```yumi
for i in foo(10 .. < 15)
    -- i

━━━━━━━━━━━━━━━━━━━━
+ do foo'T
    - stream <T>

<= 1
<<= stream
```

```output
1
10
11
12
13
14
```

```yumi
. names = ["a", "b", "c"]
. sizes = [1, 0, 3]
. labels = in names
and size in sizes
    "$each=$size"
for label in labels
    -- label
```

```output
a=1
b=0
c=3
```

### A generator read by two loops, the first left early, and one read by `<-` past its end.

```yumi
. g = gen()
for a in g
    -- a
    break
for a in g
    -- a
-- "end"
. h = gen()
-- <-h
-- <-h
-- <-h
-- <-h

━━━━━━━━━━━━━━━━━━━━
+ do gen

-- "start"
<= 1
-- "middle"
<= 2
-- "last"
```

```output
start
1
end
start
1
middle
2
last
unknown
unknown
```

### A local of a generator keeps its value from one yield to the next.

```yumi
for a in gen()
    -- a

━━━━━━━━━━━━━━━━━━━━
+ do gen

. count = 3
<= 1
count += 1
<= count
```

```output
1
4
```

### Generators that yield in loops: over a range up and down, over a list, over a text, and in two loops nested.

```yumi
for a in count(3)
    -- a
for a in down(3)
    -- a
for a in evens([1, 2, 3, 4, 5, 6])
    -- a
for a in letters("héy")
    -- a
for a in nested(2)
    -- a
. total = 0
for a in count(1000)
    total += a
-- total

━━━━━━━━━━━━━━━━━━━━
+ do count
    - limit Int

for i in 0 .. < limit
    <= i

━━━━━━━━━━━━━━━━━━━━
+ do down
    - from Int

for i in from .. 1
    <= i
<= 0

━━━━━━━━━━━━━━━━━━━━
+ do evens
    - items [Int]

for item in items
    if item % 2 == 0
        <= item

━━━━━━━━━━━━━━━━━━━━
+ do letters
    - text String

for char in text
    <= char

━━━━━━━━━━━━━━━━━━━━
+ do nested
    - size Int

for i in 0 .. < size
    for j in 0 .. < size
        <= i * 10 + j
```

```output
0
1
2
3
2
1
0
2
4
6
h
é
y
0
1
10
11
499500
```

### A generator that reads another, one that stops reading early, a member that yields, and generators of texts and of tuples.

```yumi
for a in doubled(count(3))
    -- a
for a in firsts(count(100), 2)
    -- a
. c = Counter(2)
for a in c.steps()
    -- a
for a in c.steps()
    -- a
-- c.made
for word in words()
    -- word
for pair in pairs()
    -- pair[0]
    -- pair[1]

━━━━━━━━━━━━━━━━━━━━
+ do count
    - limit Int

. i = 0
while i < limit
    <= i
    i += 1

━━━━━━━━━━━━━━━━━━━━
+ do doubled
    - source Generator'Int

for item in source
    <= item * 2

━━━━━━━━━━━━━━━━━━━━
+ do firsts
    - source Generator'Int
    - limit Int

. taken = 0
for item in source
    if taken == limit
        break
    taken += 1
    <= item

━━━━━━━━━━━━━━━━━━━━
+ do words

<= "one"
<= "two"

━━━━━━━━━━━━━━━━━━━━
+ do pairs

<= (1, "a")
<= (2, "b")

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ limit Int
+ made = 0

────────────────────
+ This
    - limit Int

────────────────────
+ do steps

for i in 0 .. < limit
    made += 1
    <= made * 100 + i
```

```output
0
2
4
0
1
100
201
300
401
4
one
two
1
a
2
b
```

### A panic raised between two yields of a generator leaves the loop that reads it.

```yumi crash
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

for a in gen(2)
    -- a

━━━━━━━━━━━━━━━━━━━━
+ do gen
    - limit Int

<= 1
check(limit)
<= 2

━━━━━━━━━━━━━━━━━━━━
+ do check
    - n Int

?! Panic("bad")
    if n == 2
```

```output
1
```

### A generator left by `break` goes on where it stopped; one read by `<-` and then by a loop; a return from inside a loop over one.

```yumi
. g = count(5)
for a in g
    -- a
    if a == 1
        break
-- "between"
for a in g
    -- a
-- "after"
. h = count(3)
-- <-h
for a in h
    -- a
-- outer()

━━━━━━━━━━━━━━━━━━━━
+ do count
    - limit Int

for i in 0 .. < limit
    <= i

━━━━━━━━━━━━━━━━━━━━
+ do outer Int

for a in count(10)
    if a == 3
        = a * 2
= 0
```

```output
0
1
between
after
0
1
2
6
```

### A generator given to a function that takes a stream, read by a loop there, left early, and made a list.

```yumi
show(gen(3))
-- total(gen(4))
. list = List'Int(gen(3))
-- list.length, list[0], list[2]

━━━━━━━━━━━━━━━━━━━━
+ do show
    - stream <Int>

for item in stream
    -- item

━━━━━━━━━━━━━━━━━━━━
+ do total Int
    - stream <Int>

. sum = 0
for item in stream
    if item == 3
        break
    sum += item
= sum

━━━━━━━━━━━━━━━━━━━━
+ do gen <Int>
    - n Int

. i = 0
while i < n
    <= i
    i += 1
```

```output
0
1
2
3
(3, 0, 2)
```

### A loop over a stream in a constructor, left early.

```yumi
. bag = Bag(gen(4))
-- bag.count, bag.sum
. other = Bag(gen(0))
-- other.count, other.sum
. third = Bag(gen(9))
-- third.count, third.sum

━━━━━━━━━━━━━━━━━━━━
+ Bag

+ count Int
+ sum Int

────────────────────
+ This
    - stream <Int>

for item in stream
    if item == 5
        break
    count += 1
    sum += item

━━━━━━━━━━━━━━━━━━━━
+ do gen <Int>
    - n Int

. i = 0
while i < n
    <= i
    i += 1
```

```output
(4, 6)
(0, 0)
(5, 10)
```

### A panic in the body of a loop over a generator runs the cleanup the generator has open.

```yumi crash
. bag = Bag(gen(4))
-- bag.count

━━━━━━━━━━━━━━━━━━━━
+ Bag

+ count Int

────────────────────
+ This
    - stream <Int>

for item in stream
    count += check(item)

━━━━━━━━━━━━━━━━━━━━
+ do check Int
    - item Int

if item == 2
    ?! Panic("two")
= 1

━━━━━━━━━━━━━━━━━━━━
+ do gen <Int>
    - n Int

do finally
    -- "closed"
. i = 0
while i < n
    <= i
    i += 1
```

```output
closed
```

### A generator an interface declares, run for an object through a value of the interface and read by `<-`.

```yumi
. dog Pet = Dog("rex")
for line in dog.lines(2)
    -- line
. cat = Dog("tom")
for line in cat.lines(1)
    -- line
. lines = dog.lines(3)
-- <-lines
-- <-lines

━━━━━━━━━━━━━━━━━━━━
+ Pet is Interface

────────────────────
+ get name String

────────────────────
+ do lines <String>
    - n Int

. i = 0
while i < n
    <= "$name $i"
    i += 1

━━━━━━━━━━━━━━━━━━━━
+ Dog
    is Pet

_ called String

────────────────────
+ This
    - called

────────────────────
+ get name

called
```

```output
rex 0
rex 1
tom 0
rex 0
rex 1
```

### A panic in a generator an interface declares.

```yumi crash
. dog Pet = Dog("rex")
for line in dog.lines(4)
    -- line

━━━━━━━━━━━━━━━━━━━━
+ Pet is Interface

────────────────────
+ get name String

────────────────────
+ do lines <String>
    - n Int

. i = 0
while i < n
    if i == 2
        ?! Panic("at two")
    <= "$name $i"
    i += 1

━━━━━━━━━━━━━━━━━━━━
+ Dog
    is Pet

_ called String

────────────────────
+ This
    - called

────────────────────
+ get name

called
```

```output
rex 0
rex 1
```

### A stream read by `<-` in a function that does nothing else, once and twice in a tuple.

```yumi
. g = gen()
-- first(g)
-- first(g)
-- both(g)

━━━━━━━━━━━━━━━━━━━━
+ do first Int?
    - stream <Int>

= <- stream

━━━━━━━━━━━━━━━━━━━━
+ both (Int?, Int?)
    - stream <Int>

<- stream, <- stream

━━━━━━━━━━━━━━━━━━━━
+ do gen <Int>

<= 1
<= 2
<= 3
```

```output
1
2
(3, unknown)
```

### A yield of an optional, `<=?`: only a value that is there is yielded.

```yumi
for each in evens([1, 2, 3, 4, 5, 6])
    -- each
for each in firsts(["ab", "", "cd"])
    -- each
for each in both(3)
    -- each

━━━━━━━━━━━━━━━━━━━━
_ do evens <Int>
    - items [Int]

for item in items
    <=? even(item)

━━━━━━━━━━━━━━━━━━━━
_ do even Int?
    - n Int

if n % 2 == 0
    = n
= null

━━━━━━━━━━━━━━━━━━━━
_ do firsts <String>
    - words [String]

for word in words
    <=? head(word)
    <= "-"

━━━━━━━━━━━━━━━━━━━━
_ do head String?
    - word String

if word.length == 0
    = null
= word

━━━━━━━━━━━━━━━━━━━━
_ do both <Int>
    - n Int

<=? even(n)
<= n
<=? even(n + 1)
```

```output
2
4
6
ab
-
-
cd
-
3
4
```

### A yield converted to the type of the generator's items: a whole number to a float and to a wider one, and a tuple.

```yumi
for each in halves()
    -- each
for each in wide()
    -- each
for (index, word) in tagged(["one", "two"])
    -- "$index $word"

━━━━━━━━━━━━━━━━━━━━
_ do halves <Float>

<= 1
<= 2.5
. n I32 = 3
<= n

━━━━━━━━━━━━━━━━━━━━
_ do wide <Int64>

. n = 7
<= n
<= 8

━━━━━━━━━━━━━━━━━━━━
_ do tagged <(Int, String)>
    - items [String]

for index in 0 .. < items.length
    <= (index, items[index])
```

```output
1
2.5
3
7
8
0 one
1 two
```

### A yield of every item of a stream, `<<=`: of a generator and of a list.

```yumi
for each in all()
    -- each
for each in twice([1, 2])
    -- each

━━━━━━━━━━━━━━━━━━━━
_ do evens <Int>
    - items [Int]

for item in items
    if item % 2 == 0
        <= item

━━━━━━━━━━━━━━━━━━━━
_ do all <Int>

<= 0
<<= evens([10, 11, 12])
<<= [20, 21]
<= 99

━━━━━━━━━━━━━━━━━━━━
_ do twice <Int>
    - items [Int]

<<= items
<<= items
```

```output
0
10
12
20
21
99
1
2
1
2
```

### A generator declared to return a stream that yields every item of a call to itself.

```yumi
for value in leaves(3)
    -- "$value"

━━━━━━━━━━━━━━━━━━━━
_ do leaves <Int>
    - depth Int

if depth > 0
    <<= leaves(depth - 1)
<= depth
```

```output
0
1
2
3
```

### A method generator that yields every item of a call to itself, first reached by a bare call inside its class and then called on an object.

```yumi
. node = Node("a")
node.kids += Node("b")
-- node.first()?.id ?? "none"
for each in node.all()
    -- each.id

━━━━━━━━━━━━━━━━━━━━
+ Node

+ id String
+ kids [Node]

────────────────────
+ This
    - id

────────────────────
+ do all <Node>

<= this
for kid in kids
    <<= kid.all()

────────────────────
+ first Node?

<- all()
```

```output
a
a
b
```

### A generator that yields every item of another generator, in a program that asks to inline nothing.

```yumi
for each in outer()
    -- each

━━━━━━━━━━━━━━━━━━━━
_ do inner <String>

<= "a"

━━━━━━━━━━━━━━━━━━━━
_ do outer <String>

<<= inner()
```

```output
a
```

### A comprehension written as a block, over a range: with a name for the item, and with `when`, `while` and `until`.

```yumi
. iter = <* n in 1 .. 3
    n * 2
for each in iter
    -- each
. evens = <* n in 1 .. 6 when n % 2 == 0
    n
for each in evens
    -- each
. early = <* n in 1 .. 6 while n < 3
    n + 100
for each in early
    -- each
. late = <* n in 1 .. 6 until n == 3
    n - 100
for each in late
    -- each
```

```output
2
4
6
2
4
6
101
102
-99
-98
```

### A comprehension over a range that reads a local of the function that writes it, one in a function with a clause, and one of floats.

```yumi
. k = 10
for each in (each * k in 1 .. 3)
    -- each
-- total(4)
. scaled = (it + 0.5 in 0 .. < 3)
for each in scaled
    -- each

━━━━━━━━━━━━━━━━━━━━
_ do total Int
    - n Int

. sum = 0
for each in (each * each in 1 .. n when each % 2 == 0)
    sum += each
= sum
```

```output
10
20
30
20
0.5
1.5
2.5
```

### A comprehension a method returns, which reads the receiver and an argument.

```yumi
. box = Box(3)
for each in box.steps()
    -- each
for each in box.doubled(2)
    -- each

━━━━━━━━━━━━━━━━━━━━
+ Box

+ size Int

────────────────────
+ This
    - size Int

────────────────────
+ do steps <Int>

= (each + size in 0 .. < size)

────────────────────
+ do doubled <Int>
    - by Int

= (each * by * size in 1 .. 2)
```

```output
3
4
5
6
12
```

### A comprehension reads a local as it is when the items are read; one never read, one left by `break`, one given to a function as a stream, and an empty one.

```yumi
. k = 2
. later = (each * k in 1 .. 3)
k = 5
for each in later
    -- each
. unread = (each in 1 .. 3)
for each in (each + 1 in 1 .. 10)
    if each > 3
        break
    -- each
-- sum-of((each * each in 1 .. 4))
-- count((it in 0 .. < 0))

━━━━━━━━━━━━━━━━━━━━
_ do sum-of Int
    - items <Int>

. sum = 0
for item in items
    sum += item
= sum

━━━━━━━━━━━━━━━━━━━━
_ do count Int
    - items <Int>

. n = 0
for item in items
    n += 1
= n
```

```output
5
10
15
2
3
30
0
```

### A comprehension in a function that yields, one a nested function returns, and one read inside the loop of another.

```yumi
for each in outer(3)
    -- each
. twice = :: n => (each * n in 1 .. 2)
for each in twice(7)
    -- each
for row in (each * 10 in 1 .. 2)
    for cell in (each + 100 in 1 .. 2)
        -- cell + row

━━━━━━━━━━━━━━━━━━━━
_ do outer <Int>
    - n Int

for each in (each * n in 1 .. 2)
    <= each
<<= (each - n in 1 .. 2)
```

```output
3
6
-2
-1
7
14
111
112
121
122
```

### A generator given where an iterable of another type of item is wanted, each item converted: from a function that yields and from a comprehension.

```yumi
-- total(walk())
-- wide(walk())
-- total((each * 2 in 1i32 .. 3))
. kept Iterable'Float = walk()
for each in kept
    -- each

━━━━━━━━━━━━━━━━━━━━
_ do walk <I32>

<= 1
<= 2

━━━━━━━━━━━━━━━━━━━━
_ do total Float
    - items Iterable'Float

. sum = 0.0
for item in items
    sum += item
= sum

━━━━━━━━━━━━━━━━━━━━
_ do wide Int64
    - items Iterable'Int64

. sum = 0i64
for item in items
    sum += item
= sum
```

```output
3
3
12
1
2
```

### A panic under a comprehension written in the header of a loop.

```yumi crash
. items = [1, 0]
for each in (10 / check(each) in items)
    -- each

━━━━━━━━━━━━━━━━━━━━
_ do check Int
    - d Int

if d == 0
    ?! Panic("zero")
= d
```

```output
10
```

### A panic under a comprehension kept in a local and read by a loop of the same function.

```yumi crash
-- total()

━━━━━━━━━━━━━━━━━━━━
_ do total Int

. items = [1, 0]
. sum = 0
. halves = (10 / check(each) in items)
for each in halves
    sum += each
= sum

━━━━━━━━━━━━━━━━━━━━
_ do check Int
    - d Int

if d == 0
    ?! Panic("zero")
= d
```

### A panic under a comprehension read through `join`.

```yumi crash
. items = [1, 0]
. halves = (10 / check(each) in items)
-- ", ".join(halves)

━━━━━━━━━━━━━━━━━━━━
_ do check Int
    - d Int

if d == 0
    ?! Panic("zero")
= d
```

### A panic under the second of two comprehensions over one name.

```yumi crash
run(2)

━━━━━━━━━━━━━━━━━━━━
+ do run
    - bad Int

. items = [1, 2, 3]
. first = (check(each, 9) in items)
for item in first
    -- item
. second = (check(each, bad) in items)
for item in second
    -- item

━━━━━━━━━━━━━━━━━━━━
+ do check Int
    - n Int
    - bad Int

if n == bad
    ?! Panic("no")
= n
```

```output
1
2
3
1
```

### A panic under a comprehension over a range, written in the header of a loop.

```yumi crash
run(2)

━━━━━━━━━━━━━━━━━━━━
+ do run
    - bad Int

for item in (check(each, bad) in 1 .. 3)
    -- item

━━━━━━━━━━━━━━━━━━━━
+ do check Int
    - n Int
    - bad Int

if n == bad
    ?! Panic("no")
= n
```

```output
1
```

### A panic under the second of two comprehensions over a property, read through `join`.

```yumi crash
run(2)

━━━━━━━━━━━━━━━━━━━━
+ do run
    - bad Int

. box = Box()
-- ", ".join((check(each, 9) in box.items))
-- ", ".join((check(each, bad) in box.items))

━━━━━━━━━━━━━━━━━━━━
+ Box

+ items = [1, 2, 3]

━━━━━━━━━━━━━━━━━━━━
+ do check Int
    - n Int
    - bad Int

if n == bad
    ?! Panic("no")
= n
```

```output
1, 2, 3
```

### A class read by `<-` through a value of an interface panics in a later read: the loop that reads it ends, and the program ends by the panic.

```yumi crash
. f Iterable'Int = Feed()
. sum = 0
. item = <-f
while item
    sum += item
    item = <-f
-- sum

━━━━━━━━━━━━━━━━━━━━
+ Feed
    is Iterable'Int

_ n = 0

────────────────────
+ do <- this Int?

n += 1
if n == 2
    ?! Panic("feed")
= n
```

### Yields of two tuples, neither the wider: the items have the tuple of what the fields at each place join to.

```yumi
for item in gen()
    -- item

━━━━━━━━━━━━━━━━━━━━
+ do gen

<= (1, "a")
<= (2.5, "b")
```

```output
(1, a)
(2.5, b)
```

### Three yields of pairs with a float on either side: every item has a float at both places.

```yumi
for item in gen()
    -- item

━━━━━━━━━━━━━━━━━━━━
+ do gen

<= (2.5, 1)
<= (1, 2.5)
<= (7, 7)
```

```output
(2.5, 1)
(1, 2.5)
(7, 7)
```

### Yields of tuples that hold tuples join place by place, within the tuples they hold too.

```yumi
for item in gen()
    -- item

━━━━━━━━━━━━━━━━━━━━
+ do gen

<= (1, (2, 3))
<= (1.5, (2.5, 3))
<= (1, (2, 3.5))
```

```output
(1, (2, 3))
(1.5, (2.5, 3))
(1, (2, 3.5))
```

### Yields of two lists give a list both convert to.

```yumi
for item in gen()
    -- item

━━━━━━━━━━━━━━━━━━━━
+ do gen

<= [1, 2]
<= [2.5]
```

```output
1, 2
2.5
```

### A struct yielded before a tuple of its fields: the tuple is converted to the struct.

```yumi
for item in gen()
    -- item

━━━━━━━━━━━━━━━━━━━━
+ do gen

<= P(1, 2)
<= (3, 4)

━━━━━━━━━━━━━━━━━━━━
+ P is Struct

+ x Int
+ y Int

────────────────────
+ to String

"P$x$y"
```

```output
P12
P34
```

### A generator of optional items yields an item that holds nothing without ending

```yumi
for item in gaps()
    -- item ?? -1
-- "collected"
. all = List(gaps())
-- all.length
. texts = List(named())
-- texts.length
for text in named()
    -- text ?? "(none)"
. kept =
    ** gaps()
    ?> each != null
    *> each ?? 0
    |> List
-- ", ".join(kept)

━━━━━━━━━━━━━━━━━━━━
+ do gaps

. missing Int?
<= 1
<= missing
<= 3
<= missing

━━━━━━━━━━━━━━━━━━━━
+ do named <String?>

. missing String?
<= "a"
<= missing
<= "c"
```

```output
1
-1
3
-1
collected
4
3
a
(none)
c
1, 3
```

### A function that yields and is declared to return a list returns the list of what it yields

```yumi
. shelf = Shelf()
shelf.add("b")
shelf.add("a")
. titles = shelf.titles
-- type-of titles
-- titles.length
-- ", ".join(titles)
shelf.add("c")
-- titles.length
-- shelf.titles.length
-- squares(4)
-- squares(0).length
-- type-of pairs()
for (n, name) in pairs()
    -- "$n=$name"
. things = shelf.things
-- type-of things
for thing in things
    -- thing.label

━━━━━━━━━━━━━━━━━━━━
+ Labelled is Interface

────────────────────
+ get label String

━━━━━━━━━━━━━━━━━━━━
+ Book
    is Labelled

+ label String

────────────────────
+ This
    - label

━━━━━━━━━━━━━━━━━━━━
+ Shelf
    is Labelled

+ label = "shelf"
_ books [Book]

────────────────────
+ do add
    - title String

books += Book(title)

────────────────────
+ do get titles [String]

for book in books
    <= book.label

────────────────────
+ do get things [Labelled]

<= this
for book in books
    <= book

━━━━━━━━━━━━━━━━━━━━
+ do squares [Int]
    - count Int

for n in 0 .. < count
    <= n * n

━━━━━━━━━━━━━━━━━━━━
+ do pairs [(Int, String)]

<= (1, "one")
<= (2, "two")
```

```output
[TypeInfo List'String'GCBacking]
2
b, a
2
3
0, 1, 4, 9
0
[TypeInfo List'Tuple'GCBacking]
1=one
2=two
[TypeInfo List'Labelled'GCBacking]
shelf
b
a
c
```

### A function that yields cannot be declared to return what no generator converts to

```yumi crash
-- bad()

━━━━━━━━━━━━━━━━━━━━
+ do bad Int

<= 1
```

```output
Exception: Generator return type does not match declared return type
<no file:5>
    + do bad Int
    ^^^^^^^^^^^
```

### A generator that is made and never stepped runs nothing of its body

```yumi
. unused = counted(2)
-- "made"
. held = counted(3)
-- held
show(counted(1))
-- kind(counted(5))
. first = held()
-- first ?? -1

━━━━━━━━━━━━━━━━━━━━
+ do counted
    - count Int

-- "started"
for n in 0 .. < count
    <= n

━━━━━━━━━━━━━━━━━━━━
+ do show'T
    - items <T>

-- T()

━━━━━━━━━━━━━━━━━━━━
+ kind <Int>
    - thing

thing
```

```output
made
[Function Generator'I64]
0
[Interface core/stream:Iterable'core/number:I64]
started
0
```

### A return in a function that yields evaluates its value, runs the cleanups and finishes

```yumi
for n in until(3)
    -- n
for n in until(0)
    -- n
for n in nested()
    -- n
for n in tidy()
    -- n

━━━━━━━━━━━━━━━━━━━━
+ do until
    - stop Int

for n in 0 .. < 10
    if n == stop
        = announce("stop at $n")
    <= n
-- "never"

━━━━━━━━━━━━━━━━━━━━
+ do nested

for a in 1 .. 3
    for b in 1 .. 3
        if a * b == 4
            = announce("found")
        <= a * 10 + b

━━━━━━━━━━━━━━━━━━━━
+ do tidy

do finally
    -- "tidied"
<= 1
= announce("leaving")

━━━━━━━━━━━━━━━━━━━━
+ do announce Int
    - text String

-- text
= 0
```

```output
0
1
2
stop at 3
stop at 0
11
12
13
21
found
1
leaving
tidied
```

### A member of a tuple that yields keeps the tuple it was called on

```yumi
. counter = Counter(3)
for n in counter.steps
    -- n
    counter = Counter(10)
-- counter.count
. second = Counter(2)
. held = second.steps
second = Counter(5)
for n in held
    -- n
-- second.count
for n in Counter(2).scaled(7)
    -- n
for text in Pair("a", 2)
    -- text

━━━━━━━━━━━━━━━━━━━━
+ Counter is Tuple

+ count Int

────────────────────
+ do get steps

for index in 0 .. < count
    <= index * 100 + count

────────────────────
+ do scaled'T
    - by T

for index in 0 .. < count
    <= (by, index)

━━━━━━━━━━━━━━━━━━━━
+ Pair'T is Tuple

+ item T
+ times Int

────────────────────
+ do to Iterable'T

for index in 0 .. < times
    <= item
```

```output
3
103
203
10
2
102
5
(7, 0)
(7, 1)
a
a
```

### List pattern declared in a function that yields

```yumi
for n in gen()
    -- n

━━━━━━━━━━━━━━━━━━━━
+ do gen

. [a, b] = [1, 2]
<= a + b
```

```output
3
```

### List pattern with a named rest declared in a function that yields

```yumi
for n in gen()
    -- n

━━━━━━━━━━━━━━━━━━━━
+ do gen

. [a, ...rest] = [1, 2, 3]
<= a
<= rest.length
<= a + rest[0]
```

```output
1
2
3
```

### List pattern declared in a closure that yields

```yumi
. g = :: items => do
    . [a, b] = items
    <= a
    <= b
for n in g([7, 8])
    -- n
```

```output
7
8
```

### List pattern declared in a function that yields a list

```yumi
-- squares()

━━━━━━━━━━━━━━━━━━━━
+ do squares [Int]

. [a, b] = [3, 4]
<= a * a
<= b * b
```

```output
9, 16
```

### each and it of the first header in a function that yields

```yumi
for text in walk()
    -- text

━━━━━━━━━━━━━━━━━━━━
+ do walk

for in ["u", "v"]
and in [1, 2]
    <= "$each $it"
```

```output
u u
v v
```

### A step that raises is read by <- as a call that raises: the error leaves, or catch holds it.

```yumi
. gen = items(true)
-- gen
. first = <- gen
-- first
. second = catch <- gen
-- second is Problem

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ do items
    - fail Bool

<= 1
if fail
    ?! Problem("bad")
<= 2
```

```output
[Function Generator'I64]
1
true
```

### A loop over a generator that raises leaves by the error, running the cleanups of the loop's body and of the generator; one that does not raise runs to the end.

```yumi
_ cleanups = 0

. r = catch total(items(true))
-- r is Problem
-- total(items(false))
-- "cleanups $cleanups"

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ do items
    - fail Bool

do finally
    cleanups += 1
<= 1
if fail
    ?! Problem("bad")
<= 2

━━━━━━━━━━━━━━━━━━━━
+ do total Int
    - stream <Int>

. sum = 0
for item in stream
    do finally
        -- "after $item"
    sum += item
= sum
```

```output
after 1
true
after 1
after 2
3
cleanups 2
```

### A generator that loops over one that raises raises too, and so does a comprehension over it.

```yumi
. r = catch sum-all()
-- r is Problem
. s = catch outer()
-- s is Problem

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ do inner

<= 1
?! Problem("inner")

━━━━━━━━━━━━━━━━━━━━
+ do relay

for x in inner()
    <= x * 10

━━━━━━━━━━━━━━━━━━━━
+ do outer Int

. n = 0
for x in relay()
    -- x
    n += x
= n

━━━━━━━━━━━━━━━━━━━━
+ do sum-all Int

. values = each + 1 in relay()
. n = 0
for v in values
    -- v
    n += v
= n
```

```output
11
true
10
true
```

### An override of a generator whose base states errors carries them, raising or not, and a loop through the base's table leaves by the error.

```yumi
. src Source = Loud()
. r = catch drain(src)
-- r is Problem
. q = catch drain(Source())
-- q

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ Source

────────────────────
+ do numbers <Int> ?! Problem

<= 1
<= 2

━━━━━━━━━━━━━━━━━━━━
+ Loud is Source

────────────────────
+ do numbers <Int>

<= 5
?! Problem("loud")

━━━━━━━━━━━━━━━━━━━━
+ do drain Int ?! Problem
    - source Source

. n = 0
for x in source.numbers()
    -- x
    n += x
= n
```

```output
5
true
1
2
3
```

### An error raised by a generator leaves nested loops at once.

```yumi
. r = catch run()
-- r is Problem
-- "end"

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ do items

<= "a"
?! Problem("x")

━━━━━━━━━━━━━━━━━━━━
+ do run

for a in items()
    for b in 0 .. < 2
        -- "$a$b"
```

```output
a0
a1
true
end
```

### A list of bullets whose bullet raises is a generator that raises.

```yumi
. r = catch run()
-- r is Problem

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ do check Int ?! Problem
    - n Int

if n > 2
    ?! Problem("big")
= n

━━━━━━━━━━━━━━━━━━━━
+ do run

. values =
    * 1
    * check(2)
    * check(3)
for v in values
    -- v
```

```output
1
2
true
```

### A comprehension that raises, read by a loop whose turns make closures.

```yumi
. r = catch run()
-- r is Problem

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ do check Int ?! Problem
    - n Int

if n > 2
    ?! Problem("big")
= n

━━━━━━━━━━━━━━━━━━━━
+ do run

. callbacks = List'Doer()
for v in check(each) in [1, 2, 3]
    callbacks += => do -- "cb $v"
    -- v
for c in callbacks
    c()
```

```output
1
2
true
```

```yumi crash
-- "start"
for x in items()
    -- x

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ Other is Error

━━━━━━━━━━━━━━━━━━━━
+ do items <Int> ?! Problem

<= 1
?! Other("bad")
```

```output
Exception: Other can leave generator items, which does not declare it; declare it with ?! Other, or catch it here
<no file:16>
    ?! Other("bad")
    ^^^^^^^^^^^^^^^
```

```yumi crash
-- describe(items())
. g = items()
-- "made"

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ do items

<= 1
?! Problem("bad")

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - g Generator'Int

"gen"
```

```output
Exception: No function "describe" accepts (Generator'I64); it is declared as "describe(g Generator'Int)"
<no file:2>
    -- describe(items())
       ^^^^^^^^^^^^^^^^^
```

### A generator that raises, made and never read, is made.

```yumi
. gen = items()
-- "made"

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ do items

<= 1
?! Problem("text")
```

```output
made
```

### A generator that raises, kept as an Iterable'Int that is never read, is made.

```yumi
. it Iterable'Int = items()
-- "as iterable only"

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ do items

<= 1
?! Problem("text")
```

```output
as iterable only
```

### catch? of a step of a generator that raises gives the item, or nothing where the step raised.

```yumi
. gen = items()
. a = catch? <- gen
-- a ?? -1
. b = catch? <- gen
-- b ?? -2

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ do items

<= 1
?! Problem("bad")
```

```output
1
-2
```

### A generator reads a name a match pattern binds after it yields.

```yumi
for x in gen([Item.num(1), Item.word("b"), Item.pair((3, "c"))])
    -- x

━━━━━━━━━━━━━━━━━━━━
+ do gen Iterable'String
    - items [Item]

for item in items
    do match item
        | .num(n)
            <= "n $n"
            <= "n again $n"
        | .word(w)
            <= "w $w"
            <= "w again $w"
        | .pair((a, b))
            <= "p $a"
            <= "p $b"

━━━━━━━━━━━━━━━━━━━━
+ Item is Union

| num Int
| word String
| pair (Int, String)
```

```output
n 1
n again 1
w b
w again b
p 3
p c
```

```yumi
/BxGC/BxGC.h

. source = kept-values("live")
-- source()
churn()
-- source()
churn()
-- source()

+ TextPair is (left String, right String)

━━━━━━━━━━━━━━━━━━━━
_ do kept-values
    - seed String

. pair = TextPair("${seed}-left", "${seed}-right")
do finally
    -- "cleanup ${pair.right}"
for value in [pair.left, pair.right]
    <= value

━━━━━━━━━━━━━━━━━━━━
_ do churn

for index in 0 .. < 8
    . pair = TextPair("garbage-$index", "discard-$index")
    C:brixCollect()
```

```output
live-left
live-right
cleanup live-right
unknown
```

```yumi
/BxGC/BxGC.h

for value in kept-values("early")
    -- value
    churn()
    break

+ TextPair is (left String, right String)

━━━━━━━━━━━━━━━━━━━━
_ do kept-values
    - seed String

. pair = TextPair("${seed}-left", "${seed}-right")
do finally
    -- "cleanup ${pair.right}"
for value in [pair.left, pair.right]
    <= value

━━━━━━━━━━━━━━━━━━━━
_ do churn

for index in 0 .. < 8
    . pair = TextPair("garbage-$index", "discard-$index")
    C:brixCollect()
```

```output
early-left
cleanup early-right
```

```yumi
/BxGC/BxGC.h

. left = kept-iterator(2)
. right = kept-iterator(5)
-- left()
-- right()
churn()
-- left()
-- right()
churn()
-- left()
-- right()
-- left()

+ TextPair is (left String, right String)

━━━━━━━━━━━━━━━━━━━━
_ do kept-iterator
    - seed Int

for value in [seed, seed + 1, seed + 2]
    <= value

━━━━━━━━━━━━━━━━━━━━
_ do churn

for index in 0 .. < 8
    . pair = TextPair("garbage-$index", "discard-$index")
    C:brixCollect()
```

```output
2
5
3
6
4
7
unknown
```

```yumi
/BxGC/BxGC.h

. captured = capture-generator(10)
-- captured()
churn()
-- captured()
-- captured()

+ TextPair is (left String, right String)

━━━━━━━━━━━━━━━━━━━━
_ do capture-generator
    - seed Int

. count = seed
. produce = => do
    <= count
    count += 1
    <= count
= produce()

━━━━━━━━━━━━━━━━━━━━
_ do churn

for index in 0 .. < 8
    . pair = TextPair("garbage-$index", "discard-$index")
    C:brixCollect()
```

```output
10
11
unknown
```

```yumi
/BxGC/BxGC.h

. result = catch collect-failing()
-- result is GeneratorTrouble

+ TextPair is (left String, right String)

━━━━━━━━━━━━━━━━━━━━
+ GeneratorTrouble is Error

━━━━━━━━━━━━━━━━━━━━
_ do failing-values
    - seed String

. value = "$seed value"
do finally
    -- "cleanup $value"
<= value
?! GeneratorTrouble("expected")

━━━━━━━━━━━━━━━━━━━━
_ do collect-failing Int ?! GeneratorTrouble

for value in failing-values("failure")
    -- value
    churn()
= 0

━━━━━━━━━━━━━━━━━━━━
_ do churn

for index in 0 .. < 8
    . pair = TextPair("garbage-$index", "discard-$index")
    C:brixCollect()
```

```output
failure value
cleanup failure value
true
```

```yumi
/BxGC/BxGC.h

. total = 0
for value in borrowed-values()
    value.count += 10
    churn()
    total += value.count
-- total

+ TextPair is (left String, right String)

━━━━━━━━━━━━━━━━━━━━
+ YieldRecord is Struct

+ count Int

━━━━━━━━━━━━━━━━━━━━
_ do borrowed-values

. value = YieldRecord(3)
<= value
value.count += 1
<= value

━━━━━━━━━━━━━━━━━━━━
_ do churn

for index in 0 .. < 8
    . pair = TextPair("garbage-$index", "discard-$index")
    C:brixCollect()
```

```output
27
```

```yumi
. source = EntrySource()
. names = each.text in source.entries when each.text != ""
. collected [String] = collect(names)
-- "${collected.length} ${" ".join(collected)}"

━━━━━━━━━━━━━━━━━━━━
+ Entry

+ text String

────────────────────
+ This
    - text String

━━━━━━━━━━━━━━━━━━━━
+ EntrySource

────────────────────
+ do get entries <Entry>

?! Error("failed")
    if false
<= Entry("a.txt")
<= Entry("")
<= Entry("b.txt")

━━━━━━━━━━━━━━━━━━━━
+ collect'Item
    - stream <Item>

List(stream)
```

```output
2 a.txt b.txt
```

```yumi
for x in pairs("a1")
    -- x

━━━━━━━━━━━━━━━━━━━━
+ do pairs <String>
    - text Indexable'Char

<= "start"
. got = match text
    ~ w [a-z] n [0-9]
        "$n$w"
<= got ?? "none"
```

```output
start
1a
```

### A function that yields reads a lazy local declared under the value it yields.

```yumi
for line in format(3)
    -- line
-- "\n".join(format(0))

━━━━━━━━━━━━━━━━━━━━
+ do format
    - width U32

<= "first"
if width
    <= "    $indent^"
        .. indent = "-" * width
```

```output
first
    ---^
first
```

### A return written with only its condition ends what a function that yields makes.

```yumi
for n in numbers(0)
    -- n
for n in numbers(3)
    -- n

━━━━━━━━━━━━━━━━━━━━
+ do numbers <Int>
    - count Int

<= 1
=? if count == 0
<= count
```

```output
1
1
3
```

### An override that yields and states no errors raises those of the base's generator when called through the base: an error of a class above them is cast to one as it leaves.

```yumi
do new
    . sources [Source] = [Source(), Counter(2), Counter(0)]
    for source in sources
        -- catch? total(source) ?? -1
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ do total Int ?! Error
    - source Source

. sum = 0
for value in source.values()
    sum += value
= sum

━━━━━━━━━━━━━━━━━━━━
+ Failed is Error

━━━━━━━━━━━━━━━━━━━━
+ Source

────────────────────
+ do values <Int>

if false
    <= 0
?! Failed("not here")

━━━━━━━━━━━━━━━━━━━━
+ Counter is Source

+ count Int

────────────────────
+ This
    - count

super()

────────────────────
+ do values <Int>

for step in 0 .. < 3
    <= step
if count == 0
    ?! Error("plain")
if count == 1
    ?! Failed("counted")
```

```output
-1
3
Invalid cast to [Object __source__:Failed]
```

### A `use` in a generator ends where the generator ends, and where a loop leaves it after a yield.

```yumi
for c in letters()
    -- "first $c"
    break
-- "mid"
for c in letters()
    -- "all $c"
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ do letters <Char>

. text = use Holder("xy")
for c in text
    <= c

━━━━━━━━━━━━━━━━━━━━
+ Holder
    is Usable'String

+ text String

────────────────────
+ This
    - text String

────────────────────
+ do begin-using String

-- "begin"
= text

────────────────────
+ do end-using

-- "end"
```

```output
begin
first x
end
mid
begin
all x
all y
end
after
```

### A method that yields and raises nothing, in the place of a base's whose generator raises, takes the base's errors.

```yumi
. providers [Provider] = [Provider(), Demo()]
for provider in providers
    -- catch? run(provider) ?? "failed"

━━━━━━━━━━━━━━━━━━━━
+ ProviderError is Error

━━━━━━━━━━━━━━━━━━━━
+ Provider

────────────────────
+ do stream <String>
    - n I64

if false
    <= "never"
?! ProviderError("not implemented")

━━━━━━━━━━━━━━━━━━━━
+ Demo is Provider

────────────────────
+ do stream <String>
    - n I64

<= "one $n"
<= "two $n"

━━━━━━━━━━━━━━━━━━━━
_ do run String ?! ProviderError
    - provider Provider

. text = ""
for word in provider.stream(3)
    text += "$word,"
= text
```

```output
failed
one 3,two 3,
```

### A comprehension over an index span.

```yumi
/core/arena
/core/array-builder
/core/table

. arena = Arena()
. items = ArrayBuilder'U32(arena)
items.append(4)
items.append(5)
items.append(6)
. ids = IndexSpan'U32(items, 1 .. < 3)
-- ", ".join("$each" in ids)
```

```output
5, 6
```

### A yield keeps what a test found of a property.

```yumi
for line in lines(Release("1.2"))
    -- line

━━━━━━━━━━━━━━━━━━━━
_ do lines
    - release Release

if release.version
    <= "version"
    <= "${release.version.length}"

━━━━━━━━━━━━━━━━━━━━
+ Release is Tuple

+ version String?
```

```output
version
3
```

### A generator's variadic tail with a default takes no items, one item, or several.

```yumi
for n in count()
    -- n
for n in count(10)
    -- n
for n in count(10, 20)
    -- n

━━━━━━━━━━━━━━━━━━━━
+ do count <Size>
    ... items [Int] = List'Int()

<= items.length
```

```output
0
1
2
```

### A stream has no truth to test, so a generator of one Bool does not become that Bool, with or without `as`, and a condition over it is an error too.

```yumi compile-error
-- found([1, 2, 3], 2) as Bool

━━━━━━━━━━━━━━━━━━━━
+ do found <Bool>
    - items [Int]
    - wanted Int

<= items.length > 0 && items[0] == wanted
```

```output
Exception: Iterable'Bool cannot become Bool: it has no truth to test, and "as" does not read the values it gives; read them instead
<no file:2>
    -- found([1, 2, 3], 2) as Bool
       ^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```yumi compile-error
. answer Bool = found([1, 2, 3], 2)
-- answer

━━━━━━━━━━━━━━━━━━━━
+ do found <Bool>
    - items [Int]
    - wanted Int

<= items.length > 0 && items[0] == wanted
```

```output
Exception: Iterable'Bool cannot become Bool: it has no truth to test, and "as" does not read the values it gives; read them instead
<no file:2>
    . answer Bool = found([1, 2, 3], 2)
             ^^^^
```

```yumi compile-error
-- if found([1, 2, 3], 2) then "found" else "missing"

━━━━━━━━━━━━━━━━━━━━
+ do found <Bool>
    - items [Int]
    - wanted Int

<= items.length > 0 && items[0] == wanted
```

```output
Exception: Iterable'Bool has no truth to test; read the values it gives instead
<no file:2>
    -- if found([1, 2, 3], 2) then "found" else "missing"
          ^^^^^^^^^^^^^^^^^^^
```
