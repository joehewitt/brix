# Units

Units

```yumi
-- 24.0foo as UnitString

+ Foo (foo) is F64
```

```output
24foo
```

```yumi
-- 24.0foo as Bar as UnitString
-- 48.0bar as Foo as UnitString

+ Foo (foo) is F64
+ Bar (bar) is 0.5foo
```

```output
48bar
24foo
```

```yumi
-- 24.0foo as Baz as UnitString
-- 192.0baz as Foo as UnitString

+ Foo (foo) is F64
+ Bar (bar) is 0.5foo
+ Baz (baz) is 0.25bar
```

```output
192baz
24foo
```

```yumi
-- 24.0foo as Bar as UnitString
-- 72bar as Foo as UnitString

+ Foo (foo) is F64
+ Bar (bar) is I32

+ 1bar = 0.333333333333foo
```

```output
72bar
23.999999999976foo
```

### Divide operator in unit

```yumi
-- 24.0foo as Bar as UnitString
-- 72bar as Foo as UnitString

+ Foo (foo) is F64
+ Bar (bar) is I32

+ 1bar = 1 / 3foo
```

```output
72bar
24foo
```

### Pow operator in unit

```yumi
-- 24.0foo as Bar as UnitString
-- 72bar as Foo as UnitString

+ Foo (foo) is F64
+ Bar (bar) is I32

+ 1bar = 2 %% 3foo
```

```output
3bar
576foo
```

### Negative unit

```yumi
-- 5foo as Bar as UnitString
-- -5bar as Foo as UnitString

+ Foo (foo) is F64
+ Bar (bar) is I32

+ 1bar = -1foo
```

```output
-5bar
5foo
```

### Multiple operations

```yumi
-- 24.0foo as Bar as UnitString
-- 72bar as Foo as UnitString

+ Foo (foo) is F64
+ Bar (bar) is I32

+ 1bar = (1 / 3) + 0foo
```

```output
72bar
24foo
```

```yumi crash
+ Foo (foo) is I32
+ Bar (bar) is I32

_ 1bar = 1foo
```

```output
Exception: Equalities must be public
<no file:5>
    _ 1bar = 1foo
    ^^^^^^^^^^^^^
```

```yumi crash
+ Foo (foo) is F64
+ Bar (bar) is I32

+ 1.0bar = 1 / 3
```

```output
Exception: Ambiguous units
<no file:5>
    + 1.0bar = 1 / 3
    ^^^^^^^^^^^^^^^^
```

```yumi crash
+ Foo (foo) is F64
+ Bar (bar) is I32

+ 1.0bar = 1foo / 3foo
```

```output
Exception: Conflicting units
<no file:5>
    + 1.0bar = 1foo / 3foo
               ^^^^^^^^^^^
```

### Argument cast to unit

```yumi
foo(24.0foo)

+ Foo (foo) is F64
+ Bar (bar) is 0.5foo

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - n Bar

-- n as UnitString
```

```output
48bar
```

### Return value cast to unit

```yumi
-- foo(24.0foo)  as UnitString

+ Foo (foo) is F64
+ Bar (bar) is 0.5foo

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n Bar

n
```

```output
48bar
```

### Units should call constructor

```yumi
-- 4foo as UnitString

━━━━━━━━━━━━━━━━━━━━
+ Foo (foo) is Int

────────────────────
+ This
    - n

= 99
```

```output
99i
```

### Allow casting numbers without equalities to each other

```yumi
-- (300foo as Bar) as UnitString

+ Foo (foo) is I64
+ Bar (bar) is I64
```

```output
300bar
```

### Do not allow casting numbers that have equalities to other types

```yumi crash
-- (300foo as Bar) as UnitString

+ Foo (foo) is I64
+ Bar (bar) is I64
+ Baz (baz) is I64
+ Baz2 (baz2) is I64

+ 1baz = 1bar
+ 1baz2 = 1foo
```

```output
Exception: Foo cannot be cast to Bar; there is no conversion between them
<no file:2>
    -- (300foo as Bar) as UnitString
        ^^^^^^^^^^^^^
```

### A number of one unit becomes one of another by the ratio between them, in casts and arithmetic.

```yumi
-- 72bar as Foo
-- 72foo as Bar
-- 1foo + 2bar
-- 2bar + 1foo
-- (1foo + 2bar) as UnitString
-- (2bar + 1foo) as UnitString
-- 3foo
-- 1.5foo
-- 3foo as UnitString
-- 3foo * 2foo
-- 3foo - 5foo
-- 10foo / 4foo
-- 5km as M
-- 1500m as Km
-- (2km + 500m) as UnitString
-- (500m + 2km) as UnitString
-- 3km < 5km
-- 3km < 500m
-- 3km == 3000m
-- 72bar < 40foo
-- 72bar == 36foo
-- 5m as I32
-- 5m as F64

━━━━━━━━━━━━━━━━━━━━
+ Foo (foo) is F64

━━━━━━━━━━━━━━━━━━━━
+ Bar (bar) is 0.5foo

━━━━━━━━━━━━━━━━━━━━
+ M (m) is I32

━━━━━━━━━━━━━━━━━━━━
+ Km (km) is 1000m
```

```output
36
144
2
4
2foo
4bar
3
1.5
3foo
6
-2
2.5
5000
1
2km
2500m
true
false
true
true
true
5
5
```

### A unit declared through bitwise operators.

```yumi
-- 8bar as Foo
-- (1bar + 4foo) as UnitString
-- 3baz as Foo
-- 16qux as Foo

+ Foo (foo) is I32
+ Bar (bar) is I32
+ Baz (baz) is I32
+ Qux (qux) is I32

+ 1bar = 2foo /lshift/ 1
+ 1baz = 6foo /and/ 3
+ 8qux = 16foo /rshift/ 2
```

```output
32
2bar
6
8
```

### A unit declared through an operator that gives no number.

```yumi crash
+ Foo (foo) is I32
+ Bar (bar) is I32

+ 1bar = 2foo < 1
```

```output
Exception: Invalid constant expression
<no file:5>
    + 1bar = 2foo < 1
             ^^^^^^^^
```

### Arithmetic on units as the operand of another operator.

```yumi
-- (1foo + 2bar + 3foo) as UnitString
-- (1foo + 2bar) * 2foo
-- 1foo + 2bar < 3foo
-- 1foo + 2bar == 2foo
-- (2bar + 1foo + 2bar) as UnitString
-- (1foo + 2bar) as Bar
-- 3foo - (1foo + 2bar)
-- 5km + 500m + 1km
-- (5km + 500m + 1km) as M
-- 2km + 500m < 3km

━━━━━━━━━━━━━━━━━━━━
+ Foo (foo) is F64

━━━━━━━━━━━━━━━━━━━━
+ Bar (bar) is 0.5foo

━━━━━━━━━━━━━━━━━━━━
+ M (m) is I32

━━━━━━━━━━━━━━━━━━━━
+ Km (km) is 1000m
```

```output
5foo
4
true
true
6bar
4
1
6
6000
true
```

### An assigning operator between units converts by their ratio, as its operator does.

```yumi
. x = 3foo
x += 2bar
-- x as UnitString
x += 1
-- x as UnitString
x *= 2foo
-- x as UnitString
. f = 1.5
f += 2bar
-- f
. y = 2bar
y += 6foo
-- y as UnitString
. i = 3.0
i += 2bar
-- i

+ Foo (foo) is F64
+ Bar (bar) is F64

+ 1bar = 3foo
```

```output
9foo
10foo
20foo
3.5
4bar
5
```

### A unit that names no type is an error at the unit.

```yumi crash
-- 5zz
```

```output
Exception: Unit "zz" not found
<no file:2>
    -- 5zz
        ^^
```

### A number written with a unit whose class declares a constructor is what that constructor returns.

```yumi
-- 4foo as UnitString
-- 4foo + 1
-- 7bar as UnitString

━━━━━━━━━━━━━━━━━━━━
+ Foo (foo) is Int

────────────────────
+ This
    - n

= 99

━━━━━━━━━━━━━━━━━━━━
+ Bar (bar) is Int
```

```output
99i
100
7bar
```

### A default written with a unit whose class declares a constructor is evaluated once, as its module starts.

```yumi
-- "start"
-- big(3foo)
-- twice(1bar)

━━━━━━━━━━━━━━━━━━━━
+ Foo (foo) is Int

────────────────────
+ This
    - n

-- "foo $n"
= n + 100

━━━━━━━━━━━━━━━━━━━━
+ Bar (bar) is Float

────────────────────
+ This
    - n

-- "bar $n"
= n * 2

━━━━━━━━━━━━━━━━━━━━
+ big Foo
    - f Foo = 9foo

f

━━━━━━━━━━━━━━━━━━━━
+ twice
    - f = 9bar

f * 2
```

```output
foo 9
bar 9
start
foo 3
103
bar 1
4
```

### A number type derived from an integer picks the integer overload over a float one.

```yumi
-- half(7 as Ticks)
-- half(7.0)
-- half(7ticks) + 1

━━━━━━━━━━━━━━━━━━━━
+ Ticks (ticks) is I64

━━━━━━━━━━━━━━━━━━━━
+ half
    - value I32

"I32 ${value / 2}"

━━━━━━━━━━━━━━━━━━━━
+ half
    - value I64

value / 2

━━━━━━━━━━━━━━━━━━━━
+ half
    - value F32

"F32 ${value / 2}"

━━━━━━━━━━━━━━━━━━━━
+ half
    - value F64

value / 2
```

```output
3
3.5
4
```

### A number of one unit becomes a whole number of another exactly, through a chain of units.

```yumi
-- 2880minutes as Days as UnitString
-- 2days as Minutes as UnitString
-- 10080minutes as Weeks as UnitString
-- 1weeks as Minutes as UnitString
-- 90minutes as Hours as UnitString
-- 4320minutes as Days

+ Minutes (minutes) is F64
+ Hours (hours) is 60minutes
+ Days (days) is 24hours
+ Weeks (weeks) is 7days
```

```output
2days
2880minutes
1weeks
10080minutes
1.5hours
3
```

### A whole number of one unit in another stays exact where smaller units lie below both.

```yumi
-- 36inches as Feet as UnitString
-- 3ft as Inches as UnitString
-- 30inches as Feet

+ Millimeters (mm) is F64
+ Centimeters (cm) is 10mm
+ Inches (inches) is 2.54cm
+ Feet (ft) is 12inches
```

```output
3ft
36inches
2.5
```

### Subtracting, dividing or multiplying across units converts the right side to the unit of the left.

```yumi
-- (8hours - 30minutes) as UnitString
-- (8hours / 120minutes) as UnitString
-- (4hours * 120minutes) as UnitString

+ Minutes (minutes) is F64
+ Hours (hours) is 60minutes
```

```output
7.5hours
4hours
8hours
```

### A unit declared on a number type of its own is one of that type, so a value held as the type keeps its size.

```yumi
. holder = Holder()
-- holder.span as Small
-- take(3bg)
-- (2bg as Base) as Big
-- 2bg as F64

+ Base is F64
+ Small (sm) is Base
+ Big (bg) is 1000sm

━━━━━━━━━━━━━━━━━━━━
+ take
    - n Base

n as Small

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ span Base = 2bg
```

```output
2000
3000
2
2
```

### Arithmetic between a unit and a plain number keeps the unit, on either side of a multiplication.

```yumi
. scaled = 2km * 2
-- scaled as UnitString
-- (2 * 2km) as UnitString
-- (6km / 2) as UnitString
-- meters(scaled)
-- meters(2 * 2km)
-- meters(2km * 2 + 1km)

━━━━━━━━━━━━━━━━━━━━
+ meters F64
    - distance Foo

distance as F64

━━━━━━━━━━━━━━━━━━━━
+ Foo (foo) is F64

━━━━━━━━━━━━━━━━━━━━
+ Km (km) is 1000foo
```

```output
4km
4km
3km
4000
4000
5000
```

### The distance units convert by their metric and imperial factors.

```yumi
/units/distance

-- 1meters as Millimeters
-- 1km as Meters
-- 1µm as Nanometers
-- 1miles as Meters
-- 1inches as Millimeters
```

```output
1000
1000
1000
1609.34
25.4
```

### An integer written with a unit that its number type cannot hold is rejected, as a plain one is.

```yumi crash
+ Tk (tk) is U8

-- 300tk
```

```output
Exception: Integer literal 300 does not fit Tk
<no file:4>
    -- 300tk
       ^^^^^
```

### A symbol written after a number that names no unit is named in the error.

```yumi crash
-- 25%
```

```output
Exception: Unit "%" not found
<no file:2>
    -- 25%
         ^
```

### A number with a fraction written in a unit that holds whole numbers is rejected.

```yumi crash
+ M (m) is I32

-- 3.7m
```

```output
Exception: M holds whole numbers, as I32 does, so 3.7m cannot be written; write a whole number
<no file:4>
    -- 3.7m
       ^^^^
```

### A number written in the unit of a class that is no number and has no constructor is rejected.

```yumi crash
+ Name (nm) is String

-- 5nm
```

```output
Exception: Name is no number and declares no constructor that takes one, so no number can be written in its unit; declare Name as a number type
<no file:4>
    -- 5nm
       ^^^
```

### A unit declared as a number of a unit that names nothing is reported at the declaration.

```yumi crash
-- 2bar

━━━━━━━━━━━━━━━━━━━━
+ Bar (bar) is 2zz
```

```output
Exception: Unit "zz" not found
<no file:5>
    + Bar (bar) is 2zz
                    ^^
```

### A unit declared as zero of another is rejected, as no number of it would convert.

```yumi crash
-- 2bar as Foo as UnitString

━━━━━━━━━━━━━━━━━━━━

+ Foo (foo) is F64
+ Bar (bar) is 0foo
```

```output
Exception: A unit cannot be declared as zero of another, which no number of it would convert from; write a number other than zero
<no file:7>
    + Bar (bar) is 0foo
                   ^^^^
```

### An equality with a side in a number type of the language is rejected.

```yumi crash
-- 2bar as F64

━━━━━━━━━━━━━━━━━━━━

+ Bar (bar) is F64
+ 1bar = 3f
```

```output
Exception: An equality relates units a program declares, and F64 is a number type of the language; write each side in a unit the program declares
<no file:7>
    + 1bar = 3f
    ^^^^^^^^^^^
```

### A unit suffix a module declares twice is rejected.

```yumi crash
-- 2q

━━━━━━━━━━━━━━━━━━━━

+ A (q) is F64
+ B (q) is I32
```

```output
Exception: "q" already names A; give B a suffix of its own
<no file:7>
    + B (q) is I32
               ^^^
```

### A unit suffix that a class of an imported module has is rejected, as it would hide that class's.

```yumi crash
-- 'a'

━━━━━━━━━━━━━━━━━━━━

+ C (c) is F64
```

```output
Exception: "c" already names Char; give C a suffix of its own
<no file:6>
    + C (c) is F64
               ^^^
```

### Two equalities between the same units that disagree are rejected.

```yumi crash
-- 1bar as Foo as UnitString

━━━━━━━━━━━━━━━━━━━━

+ Foo (foo) is F64
+ Bar (bar) is 2foo
+ 1bar = 3foo
```

```output
Exception: Equalities between Bar and Foo disagree: one makes 1 Bar 2 Foo, another 3 Foo; keep one of them
<no file:8>
    + 1bar = 3foo
    ^^^^^^^^^^^^^
```

### An equality that adds a number with no unit to one in a unit is rejected, as it is no ratio.

```yumi crash
-- 1bar as Foo as UnitString

━━━━━━━━━━━━━━━━━━━━

+ Foo (foo) is F64
+ Bar (bar) is F64
+ 1bar = 2foo + 5
```

```output
Exception: An equality relates two units by a ratio alone, so a number with no unit cannot be added to or taken from one; write the ratio as one number in the unit
<no file:8>
    + 1bar = 2foo + 5
                    ^
```
