# Panic

```yumi crash
?! Panic("boom")
```

```yumi crash
gc-free-panic()

━━━━━━━━━━━━━━━━━━━━
+ do gc-free-panic without gc

?! Panic("gc-free boom")
```

```stderr
Unhandled panic: gc-free boom
<no file:7>

Task 1

Stack trace:
  0  __source__:gc-free-panic no file:7:1
  1  __source__:main no file:2:1
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

```yumi crash
outer()

━━━━━━━━━━━━━━━━━━━━
+ do outer

middle()

━━━━━━━━━━━━━━━━━━━━
+ do middle

inner()

━━━━━━━━━━━━━━━━━━━━
+ do inner

?! Panic("boom")
```

```stderr
Unhandled panic: boom
<no file:17>

Task 1

Stack trace:
  0  __source__:inner no file:17:1
  1  __source__:middle no file:12:1
  2  __source__:outer no file:7:1
  3  __source__:main no file:2:1
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

```yumi crash
foo()

━━━━━━━━━━━━━━━━━━━━
+ do foo

do finally
    -- "cleanup"

?! Panic("boom")
```

```output
cleanup
```

```yumi
. attempt = do new
    foo()
catch failure SpecialPanic
    -- "special $failure"
catch failure
    -- "other $failure"
attempt.wait()
-- "done"

━━━━━━━━━━━━━━━━━━━━
+ SpecialPanic is Panic

━━━━━━━━━━━━━━━━━━━━
+ do foo

?! SpecialPanic("typed")
```

```output
special typed
done
```

```yumi
. attempt = do new
    outer()
catch failure
    -- "caught $failure"
attempt.wait()
-- "done"

━━━━━━━━━━━━━━━━━━━━
+ do outer

inner()
-- "after outer"

━━━━━━━━━━━━━━━━━━━━
+ do inner

do finally
    -- "inner cleanup sees ${Task().failure}"
    ?! Panic("wrapped")

?! Panic("inner")
-- "after inner"
```

```output
inner cleanup sees inner
caught wrapped
done
```

```yumi
. attempt = do new
    foo()
catch
    --
attempt.wait()
-- "done"

━━━━━━━━━━━━━━━━━━━━
+ do foo

do finally
    -- "first cleanup"

do finally
    -- "middle cleanup sees ${Task().failure}"

do finally
    -- "last cleanup"

?! Panic("boom")
-- "after"
```

```output
last cleanup
middle cleanup sees boom
first cleanup
done
```

```yumi
. attempt = do new
    outer()
catch failure
    -- "caught $failure"
attempt.wait()
-- "done"

━━━━━━━━━━━━━━━━━━━━
+ do outer

middle()
-- "after outer"

━━━━━━━━━━━━━━━━━━━━
+ do middle

inner()
-- "after middle"

━━━━━━━━━━━━━━━━━━━━
+ do inner

-- "before inner"
?! Panic("deep")
-- "after inner"
```

```output
before inner
caught deep
done
```

```yumi
. attempt = do new
    outer()
catch
    --
attempt.wait()
-- "done"

━━━━━━━━━━━━━━━━━━━━
+ do outer

do finally
    -- "outer cleanup sees ${Task().failure}"

inner()

━━━━━━━━━━━━━━━━━━━━
+ do inner

do finally
    -- "inner cleanup 1"

do finally
    -- "inner cleanup 2"

?! Panic("inner")
```

```output
inner cleanup 2
inner cleanup 1
outer cleanup sees inner
done
```

```yumi
. attempt = do new
    foo()
catch failure
    -- "$failure: ${failure.source}"
attempt.wait()
-- "done"

━━━━━━━━━━━━━━━━━━━━
+ do foo

?! Panic("manual", ErrorSource("manual.yumi", 99, 7))
```

```output
manual: manual.yumi:99:7
done
```

### A panic unwinding through an implicit Result return still reaches the caller.

```yumi crash
propagating()
-- "unreachable"

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError
    - ok Bool

if ok
    = 7
else
    ?! IOError("disk")

━━━━━━━━━━━━━━━━━━━━
+ do propagating

. value = read(true)
-- "value $value"
?! Panic("boom after result")
```

```output
value 7
```

```yumi
attempt(0)
attempt(1)
attempt(2)

━━━━━━━━━━━━━━━━━━━━
+ do attempt
    - which Int

. task = do new
    choose(which)
catch failure
    -- "${failure.message}: ${failure.source?.line}:${failure.source?.column}"
task.wait()

━━━━━━━━━━━━━━━━━━━━
+ do choose
    - which Int

if which == 0
    . missing Int?
    -- missing!
else if which == 1
    . missing Int?
    -- missing!
else
    ?! Panic("existing")
```

```output
Optional'core/number:I64 is null: 22:8
Optional'core/number:I64 is null: 25:8
existing: 27:5
```

### An unhandled panic is reported with the text its own class converts it to.

```yumi crash
?! MyPanic()

━━━━━━━━━━━━━━━━━━━━
+ MyPanic is Panic

────────────────────
+ to String

"custom text"
```

```stderr
Unhandled panic: custom text
<no file:2>

Task 1

Stack trace:
  0  __source__:main no file:2:1
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A core error made with no message has the text of its own class.

```yumi
-- Panic()
-- OutOfBoundsError()
-- TaskCancelationError()
-- IOError()
-- TimeoutError()
```

```output
Panic
Out of bounds
Task cancelled
IO error
Timed out
```

### A raise under a condition that does not hold raises nothing.

```yumi
. x = 3
?! Panic("big")
    if x > 5
-- "after"
```

```output
after
```

### A raise under a condition that holds raises.

```yumi crash
. x = 9
?! Panic("big")
    if x > 5
-- "never"
```

### A panic raised under a text is raised when the text is not empty.

```yumi
. t = ""
?! Panic("empty")
    if t
-- "after"
```

```output
after
```

### A panic raised below a recursive call leaves through every caller, and nothing after a call runs.

```yumi crash
down(3)
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ do down
    - n Int

if n > 0
    down(n - 1)
    -- n
    =
?! Panic("bottom")
```

```stderr
Unhandled panic: bottom
<no file:13>

Task 1

Stack trace:
  0  __source__:down no file:13:1
  1  __source__:down no file:10:5
  2  __source__:down no file:10:5
  3  __source__:down no file:10:5
  4  __source__:main no file:2:1
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### A panic leaves through two functions that call each other.

```yumi crash
a(2)
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ do a
    - n Int

b(n)
-- "a $n"

━━━━━━━━━━━━━━━━━━━━
+ do b
    - n Int

if n > 0
    a(n - 1)
    -- "b $n"
    =
?! Panic("bottom")
```

```stderr
Unhandled panic: bottom
<no file:20>

Task 1

Stack trace:
  0  __source__:b no file:20:1
  1  __source__:a no file:9:1
  2  __source__:b no file:17:5
  3  __source__:a no file:9:1
  4  __source__:b no file:17:5
  5  __source__:a no file:9:1
  6  __source__:main no file:2:1
  7  __source__:init.__source__ no file
  8  __source__:brix.main no file
```

### A call that raises nothing goes on, and one that raises ends the statement it is in.

```yumi crash
-- check(1)
-- check(-1) + check(2)
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ do check Int
    - n Int

?! Panic("negative")
    if n < 0
= n
```

```stderr
Unhandled panic: negative
<no file:10>

Task 1

Stack trace:
  0  __source__:check no file:10:1
  1  __source__:main no file:3:4
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A panic raised under the constant true ends its function, which needs no return after it.

```yumi crash
-- pick()

━━━━━━━━━━━━━━━━━━━━
+ do pick Int

?! Panic("always")
    if true
```

```stderr
Unhandled panic: always
<no file:7>

Task 1

Stack trace:
  0  __source__:pick no file:7:1
  1  __source__:main no file:2:4
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A cleanup runs once at each way out of its scope: an iteration that ends, a panic, and a return whose value was made before it.

```yumi
. attempt = do new
    foo()
catch failure
    -- "caught $failure"
attempt.wait()
-- "done"
-- value()

━━━━━━━━━━━━━━━━━━━━
+ do foo

for i in 0 .. < 3
    do finally
        -- "iter $i"
    if i == 1
        ?! Panic("boom")
    -- i

━━━━━━━━━━━━━━━━━━━━
+ do value Int

. n = 1
do finally
    n = 9
    -- "cleanup $n"

if n == 1
    do finally
        -- "inner"
    = n + 1
= 0
```

```output
0
iter 0
iter 1
caught boom
done
inner
cleanup 9
2
```

### A return gives the value its expression had before the cleanups ran.

```yumi
-- scalar()
-- fresh()
-- field().a
-- text()

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ a = 1

━━━━━━━━━━━━━━━━━━━━
+ do scalar Int

. n = 1
do finally
    n = 9
= n

━━━━━━━━━━━━━━━━━━━━
+ do fresh (Int, Int)

. made = (1, 2)
do finally
    made = (7, 8)
= (made[0] + 1, made[1] + 1)

━━━━━━━━━━━━━━━━━━━━
+ do field Holder

. held = Holder()
do finally
    held.a = 9
= held

━━━━━━━━━━━━━━━━━━━━
+ do text String

. said = "before"
do finally
    said = "after"
= said
```

```output
1
(2, 3)
9
before
```

### A panic raised in a cleanup as its block is left by its end, a `break`, a `continue` or a return leaves the function once the cleanups have run.

```yumi
watch(1)
watch(2)
watch(3)
watch(4)
watch(5)
-- "end"

━━━━━━━━━━━━━━━━━━━━
+ do watch
    - way Int

. attempt = do new
    -- "left ${leave(way)}"
catch failure
    -- "way $way: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do leave Int
    - way Int

. count = 0
while count < 3
    count += 1
    do finally
        -- "cleanup $count"
        if count == way
            ?! Panic("pass $count")
    if count == 1
        continue
    if count == 2
        break
if way == 4
    do finally
        ?! Panic("at the return")
    = 4
do
    do finally
        if way == 3
            ?! Panic("at the end of a block")
    -- "block"
= count
```

```output
cleanup 1
way 1: pass 1
cleanup 1
cleanup 2
way 2: pass 2
cleanup 1
cleanup 2
block
way 3: at the end of a block
cleanup 1
cleanup 2
way 4: at the return
cleanup 1
cleanup 2
block
left 2
end
```

### A panic raised in a cleanup takes the place of one raised before it, and the cleanups of the blocks around still run.

```yumi
run()
watch()
-- "end"

━━━━━━━━━━━━━━━━━━━━
+ do run

. attempt = do new
    do
        do finally
            ?! Panic("one")
        do finally
            ?! Panic("two")
        -- "body"
    -- "not reached"
catch failure
    -- "run sees $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do watch

. attempt = do new
    -- first(true)
catch failure
    -- "watch sees $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do first Int
    - deep Bool

do finally
    -- "outer cleanup"
do
    do finally
        -- "middle cleanup"
        ?! Panic("middle")
    if deep
        do finally
            -- "inner cleanup"
        = 5
= 6
```

```output
body
run sees one
inner cleanup
middle cleanup
outer cleanup
watch sees middle
end
```

### A returned value is converted to the type the function returns before the cleanups run, which a panic the conversion raises runs.

```yumi
-- wrapped()
-- described()
check()
-- "end"

━━━━━━━━━━━━━━━━━━━━
+ do wrapped Wrapped

. s = "s"
do finally
    -- "cleanup"
    s = "changed"
= s

━━━━━━━━━━━━━━━━━━━━
+ do described String

. t = Thing()
do finally
    t.n = 9
= "$t"

━━━━━━━━━━━━━━━━━━━━
+ do check

. attempt = do new
    -- held()
catch failure
    -- "check sees $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do held Int

. o Int?
do finally
    . failure = Task().failure
    if failure
        -- "cleanup sees $failure"
    else
        -- "no panic"
= o!

━━━━━━━━━━━━━━━━━━━━
+ Wrapped is (text String)

────────────────────
+ from other String

-- "from $other"
Wrapped("$other!")

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ n = 1

────────────────────
+ to String

"thing $n"
```

```output
from s
cleanup
(s!)
thing 1
cleanup sees Optional'core/number:I64 is null
check sees Optional'core/number:I64 is null
end
```

### A panic raised in a cleanup where its function ends is reported when nothing recovers it.

```yumi crash
go()
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ do go

do finally
    ?! Panic("in cleanup")
-- "body"
```

```output
body
```

### A panic names a tuple with no name by its fields.

```yumi
plain()
named()
-- "end"

━━━━━━━━━━━━━━━━━━━━
+ do plain

. attempt = do new
    . t (Int, String)?
    -- t!
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do named

. attempt = do new
    . t (a Int, b (Float, Bool))?
    -- t!
catch failure
    -- failure
attempt.wait()
```

```output
Optional'Tuple:(core/number:I64, core/string:String) is null
Optional'Tuple:(a: core/number:I64, b: Tuple:(core/number:F64, core/number:Bool)) is null
end
```

### A panic raised in a closure ends the program after what was printed.

```yumi crash
. limit = 3
. gate-direct = :: amount => do
    ?! Panic("direct past $limit")
        if amount > limit
    = amount * 11
-- gate-direct(2)
-- gate-direct(9)
```

```output
22
```

### A panic raised in a closure called through a function value.

```yumi crash
. limit = 4
. gate-valued = :: amount => do
    ?! Panic("valued past $limit")
        if amount > limit
    = amount * 12
-- apply-valued(gate-valued, 2)
. kept ValuedCheck = gate-valued
-- kept(3)
-- apply-valued(gate-valued, 9)

━━━━━━━━━━━━━━━━━━━━
+ ValuedCheck is () Int
    - amount Int

━━━━━━━━━━━━━━━━━━━━
+ do apply-valued Int
    - check ValuedCheck
    - amount Int

= check(amount) + 100
```

```output
124
36
```

### A panic raised in a function bound and kept as a function value.

```yumi crash
. check BoundCheck = limit-bound(5, ...)
-- check(2)
-- apply-bound(check, 9)
-- settle-bound(1)

━━━━━━━━━━━━━━━━━━━━
+ BoundCheck is () Int
    - amount Int

━━━━━━━━━━━━━━━━━━━━
+ do apply-bound Int
    - check BoundCheck
    - amount Int

= check(amount) + 200

━━━━━━━━━━━━━━━━━━━━
+ do limit-bound Int
    - limit Int
    - amount Int

?! Panic("bound past $limit")
    if amount > limit
= amount * 13

━━━━━━━━━━━━━━━━━━━━
+ do settle-bound Int
    - amount Int

= amount + 201
```

```output
26
```

### A panic raised under one of three closures passed as function values.

```yumi crash
. gate-one = :: amount => fail-each(amount, "one")
. gate-two = :: amount => fail-each(amount + 1, "two")
. gate-three = :: amount => fail-each(amount + 2, "three")
-- apply-each(gate-one, 2)
-- apply-each(gate-two, 2)
-- apply-each(gate-three, 9)

━━━━━━━━━━━━━━━━━━━━
+ EachCheck is () Int
    - amount Int

━━━━━━━━━━━━━━━━━━━━
+ do apply-each Int
    - check EachCheck
    - amount Int

= check(amount) + 300

━━━━━━━━━━━━━━━━━━━━
+ do fail-each Int
    - amount Int
    - text String

?! Panic("each $text")
    if amount > 6
= amount * 14
```

```output
328
342
```

### A panic raised in a closure written in a closure.

```yumi crash
. limit = 7
. gate-outer = :: amount => do
    . gate-inner = :: value => do
        ?! Panic("inner past $limit")
            if value > limit
        = value * 15
    = gate-inner(amount) + 500
-- gate-outer(1)
-- gate-outer(9)
```

```output
515
```

### A panic raised in a closure assigned to an optional property of a function type.

```yumi crash
. holder = HeldGate()
-- holder.apply-held(5)
. gate-held = :: amount => do
    ?! Panic("held past 8")
        if amount > 8
    = amount * 16
holder.action = gate-held
-- holder.apply-held(1)
-- holder.apply-held(9)

━━━━━━━━━━━━━━━━━━━━
+ HeldCheck is () Int
    - amount Int

━━━━━━━━━━━━━━━━━━━━
+ HeldGate

+ action HeldCheck?

────────────────────
+ do apply-held Int
    - amount Int

. step = action
if step
    = step(amount) + 600
= -1
```

```output
-1
616
```

### A panic raised in a closure a method returns, called after the method.

```yumi crash
. gate = PassGate()
gate.limit = 9
. passed = gate.passer()
-- passed(2)
-- passed(10)

━━━━━━━━━━━━━━━━━━━━
+ PassGate

+ limit = 0

────────────────────
+ do passer

. gate-passed = :: amount => do
    ?! Panic("passed past $limit")
        if amount > limit
    = amount * 17
= gate-passed
```

```output
34
```

```yumi crash
choose(0)

━━━━━━━━━━━━━━━━━━━━
+ do choose
    - which Int

if which == 0
    . missing Int?
    -- missing!
else if which == 1
    . missing Int?
    -- missing!
else
    ?! Panic("existing")
```

```stderr
Unhandled panic: Optional'core/number:I64 is null
<no file:10>

Task 1

Stack trace:
  0  __source__:choose no file:10:8
  1  __source__:main no file:2:1
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

```yumi crash
choose(1)

━━━━━━━━━━━━━━━━━━━━
+ do choose
    - which Int

if which == 0
    . missing Int?
    -- missing!
else if which == 1
    . missing Int?
    -- missing!
else
    ?! Panic("existing")
```

```stderr
Unhandled panic: Optional'core/number:I64 is null
<no file:13>

Task 1

Stack trace:
  0  __source__:choose no file:13:8
  1  __source__:main no file:2:1
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

```yumi crash
choose(2)

━━━━━━━━━━━━━━━━━━━━
+ do choose
    - which Int

if which == 0
    . missing Int?
    -- missing!
else if which == 1
    . missing Int?
    -- missing!
else
    ?! Panic("existing")
```

```stderr
Unhandled panic: existing
<no file:15>

Task 1

Stack trace:
  0  __source__:choose no file:15:5
  1  __source__:main no file:2:1
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

```yumi crash
/core/markup
/core/tests/support/markup-fixtures

<Panel panel

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ level I64 = boom()

────────────────────

<Box

━━━━━━━━━━━━━━━━━━━━
+ do boom I64

?! Panic("boom")
```

```stderr
Unhandled panic: boom
<no file:19>

Task 1

Stack trace:
  0  __source__:boom no file:19:1
  1  __source__:Panel:Panel no file:10:15
  2  __source__:Panel:Panel no file:8:1
  3  __source__:main no file:5:1
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A panic raised making a value of an enum that main reads first ends the program before main runs.

```yumi crash
-- "before"
-- Scale.minor.title

━━━━━━━━━━━━━━━━━━━━
+ Scale is (title String, notes [Note])

| minor = ("Minor", [Note(3)])
| major = ("Major", [Note(1)])

━━━━━━━━━━━━━━━━━━━━
+ Note

+ pitch Int

────────────────────
+ do This
    - pitch Int

?! Panic("no pitch $pitch")
    if pitch > 2
this.pitch = pitch
```

```stderr
Unhandled panic: no pitch 3
<no file:20>

Task 1

Stack trace:
  0  __source__:Note:This no file:20:1
  1  __source__:init.__source__.statics no file:8:22
  2  __source__:brix.main no file
```
