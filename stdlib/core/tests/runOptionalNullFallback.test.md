# Optional null fallback

```yumi
. omitted = Box() if !mixed()
-- mixed()
-- !mixed()
-- omitted == null
. context = Context()
context.report(omitted)
-- context.fix == null

━━━━━━━━━━━━━━━━━━━━
+ Box

+ value Int = 1

━━━━━━━━━━━━━━━━━━━━
+ Context

+ fix Box?

────────────────────
+ do report
    - fix Box? = null

this.fix = fix

━━━━━━━━━━━━━━━━━━━━
+ do mixed Bool

= true
```

```output
true
false
true
true
```

```yumi
. box = Box()
. present Box? = box
. absent Box? = null
-- (present ?? null) == box
-- (absent ?? null) == null
. number Int? = null
-- (number ?? null) == null
number = 7
-- retain(number)!

━━━━━━━━━━━━━━━━━━━━
+ Box

+ value Int = 1

━━━━━━━━━━━━━━━━━━━━
+ retain
    - value Int?

value ?? null
```

```output
true
true
true
7
```

```yumi
. box = Box()
. present Box? = box
. absent Box? = null
-- (present ?? fallback()) == box
-- (absent ?? fallback()) == null

━━━━━━━━━━━━━━━━━━━━
+ Box

+ value Int = 1

━━━━━━━━━━━━━━━━━━━━
+ do fallback

-- "fallback"
= null
```

```output
true
fallback
true
```

```yumi
. half F64 = 7.5
. none F64 = 0
-- if !is-set() then 0 else half
-- if is-set() then half else 0
-- if !is-set() then 0 else 7.5
-- none ?? 2.5
-- half ?? 0

━━━━━━━━━━━━━━━━━━━━
+ do is-set Bool

= true
```

```output
7.5
7.5
7.5
2.5
7.5
```

### A conditional with a null fallback yields null on every loop iteration where its condition fails.

```yumi
. values = [1.5, 2.5, 3.5]
. present = [true, false, false]
for index in 0 .. < values.length
    . value F64? = values[index] if present[index]
    -- value == null
for index in 0 .. < values.length
    . pair Pair? = Pair(index, values[index]) if present[index]
    -- pair == null
for index in 0 .. < values.length
    . inferred = values[index] if present[index]
    -- inferred == null

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ index Index
+ value F64
```

```output
false
true
true
false
true
true
false
true
true
```

### A declared optional capture keeps null when its initializer converts from a different optional type.

```yumi
. flag = off()
. a F64? = if flag then 1.5 else null
-- a == null
. b F64? = 1.5 if flag
-- b == null
. c Int? = if flag then 3 else null
-- c == null
. d = if flag then 1.5 else null
-- d == null
. e F64? = if flag then 1.5 else (2.5 if flag)
-- e == null
. f F64? = if flag then 1.5 else 2.5
-- f!

━━━━━━━━━━━━━━━━━━━━
+ do off Bool

= false
```

```output
true
true
true
true
true
2.5
```

### A null branch before a concrete fallback unifies to the fallback's optional type.

```yumi
. owner U64 = 7
-- pick(true, owner) == null
-- pick(false, owner)!
-- pick-do(true, owner) == null
-- pick-do(false, owner)!
. text = if owner == 7 then null else "seven"
-- text == null

━━━━━━━━━━━━━━━━━━━━
+ pick U64?
    - hide Bool
    - value U64

if hide then null else value

━━━━━━━━━━━━━━━━━━━━
+ do pick-do U64?
    - hide Bool
    - value U64

. kept = value
= if hide then null else kept
```

```output
true
7
true
7
true
```

### An optional holding zero is present.

```yumi
. zero Index? = 0
-- if zero then "present" else "absent"
-- !zero
. none Index? = null
-- if none then "present" else "absent"
```

```output
present
false
absent
```

```yumi
. three = Box()
three.value = 3
. five = Box()
five.value = 5
. missing = Box()
missing.value = -1
. boxes = List'Box()
boxes += three
boxes += five
. entries = Map'Int'[Box]()
entries[1] = boxes
-- (cached(entries, 1, 5) ?? missing).value
-- (cached(entries, 1, 9) ?? missing).value
-- (cached(entries, 2, 5) ?? missing).value

━━━━━━━━━━━━━━━━━━━━
+ Box

+ value Int = 1

━━━━━━━━━━━━━━━━━━━━
+ do cached
    - entries Map'Int'[Box]
    - id Int
    - value Int

. list = entries[id]
=? null if !list
for entry in list
    if entry.value == value
        = entry
= null
```

```output
5
-1
-1
```

```yumi
. five = Box()
five.value = 5
. missing = Box()
missing.value = -1
-- (loop-only([five], 5) ?? missing).value
-- (loop-only([five], 9) ?? missing).value
-- (null-first([five], 9) ?? missing).value
-- (null-first(List'Box(), 5) ?? missing).value

━━━━━━━━━━━━━━━━━━━━
+ Box

+ value Int = 1

━━━━━━━━━━━━━━━━━━━━
+ do loop-only
    - boxes [Box]
    - value Int

for entry in boxes
    if entry.value == value
        = entry
= null

━━━━━━━━━━━━━━━━━━━━
+ do null-first
    - boxes [Box]
    - value Int

=? null if !boxes.length
if boxes[0].value == value
    = boxes[0]
= null
```

```output
5
-1
-1
-1
```

### null where a condition holds or else a value, which is evaluated only when it is taken

```yumi
. a Int? = if flag() then null else 5
-- a ?? -1
. b = if !flag() then null else "text"
-- b ?? "none"
. c = if flag() then null else make()
-- c ?? 0
-- (if !flag() then null else make()) ?? 0
. d = if !flag() then null else a
-- d ?? 7
. e = if !flag() then null else Point(1, 2)
-- e!.y
-- hidden(true) ?? "hidden"
-- hidden(false) ?? "hidden"

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ do flag Bool

= true

━━━━━━━━━━━━━━━━━━━━
+ do make Int

-- "made"
= 9

━━━━━━━━━━━━━━━━━━━━
+ hidden String?
    - hide Bool

if hide then null else "shown"
```

```output
-1
text
0
made
9
7
2
hidden
shown
```

### A number that falls back to a number of another type joins with it

```yumi
. a F64 = 0
. b I64 = 0
. c U8 = 3
. d I32 = 12
-- a ?? 1
-- b ?? 2
-- c ?? 4
-- a ?? d
-- d ?? 0.5
-- a ?? next()
-- c ?? next()

━━━━━━━━━━━━━━━━━━━━
+ do next I32

-- "next"
= 8
```

```output
1
2
3
12
12
next
8
3
```
