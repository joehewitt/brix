# Units

Units

```brix
(main
  (print (as 24.0foo UnitString))
)

(blank)
(type-line + Foo (alias foo) (is F64))
```

```output
24foo
```

```brix
(main
  (print (as 24.0foo Bar UnitString))
  (print (as 48.0bar Foo UnitString))
)

(blank)
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is 0.5foo))
```

```output
48bar
24foo
```

```brix
(main
  (print (as 24.0foo Baz UnitString))
  (print (as 192.0baz Foo UnitString))
)

(blank)
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is 0.5foo))
(type-line + Baz (alias baz) (is 0.25bar))
```

```output
192baz
24foo
```

```brix
(main
  (print (as 24.0foo Bar UnitString))
  (print (as 72bar Foo UnitString))
)

(blank)
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is I32))
(blank)
(unit + 1bar 0.333333333333foo)
```

```output
72bar
23.999999999976foo
```

### Divide operator in unit

```brix
(main
  (print (as 24.0foo Bar UnitString))
  (print (as 72bar Foo UnitString))
)

(blank)
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is I32))
(blank)
(unit + 1bar (/ 1 3foo))
```

```output
72bar
24foo
```

### Pow operator in unit

```brix
(main
  (print (as 24.0foo Bar UnitString))
  (print (as 72bar Foo UnitString))
)

(blank)
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is I32))
(blank)
(unit + 1bar (%% 2 3foo))
```

```output
3bar
576foo
```

### Negative unit

```brix
(main
  (print (as 5foo Bar UnitString))
  (print (as -5bar Foo UnitString))
)

(blank)
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is I32))
(blank)
(unit + 1bar -1foo)
```

```output
-5bar
5foo
```

### Multiple operations

```brix
(main
  (print (as 24.0foo Bar UnitString))
  (print (as 72bar Foo UnitString))
)

(blank)
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is I32))
(blank)
(unit + 1bar (+ (/ 1 3) 0foo))
```

```output
72bar
24foo
```

```brix crash
(type-line + Foo (alias foo) (is I32))
(type-line + Bar (alias bar) (is I32))
(blank)
(unit _ 1bar 1foo)
```

```output
Exception: Equalities must be public
<no file:5>
    (unit _ 1bar 1foo)
    ^^^^^^^^^^^^^^^^^^
```

```brix crash
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is I32))
(blank)
(unit + 1.0bar (/ 1 3))
```

```output
Exception: Ambiguous units
<no file:5>
    (unit + 1.0bar (/ 1 3))
    ^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is I32))
(blank)
(unit + 1.0bar (/ 1foo 3foo))
```

```output
Exception: Conflicting units
<no file:5>
    (unit + 1.0bar (/ 1foo 3foo))
                   ^^^^^^^^^^^^^
```

### Argument cast to unit

```brix
(main
  (call foo 24.0foo)
)

(blank)
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is 0.5foo))

(fn-do + foo _
  (param n Bar)
  (print (as n UnitString))
)
```

```output
48bar
```

### Return value cast to unit

```brix
(main
  (print (as (call foo 24.0foo) UnitString))
)

(blank)
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is 0.5foo))

(fn-expr + foo _
  (param n Bar)
  n
)
```

```output
48bar
```

### Units should call constructor

```brix
(main
  (print (as 4foo UnitString))
)

(type + Foo (alias foo) (is Int)
  (fn-do + This _
    (param n _)
    (return 99)
  )
)
```

```output
99i
```

### Allow casting numbers without equalities to each other

```brix
(main
  (print (as (as 300foo Bar) UnitString))
)

(blank)
(type-line + Foo (alias foo) (is I64))
(type-line + Bar (alias bar) (is I64))
```

```output
300bar
```

### Do not allow casting numbers that have equalities to other types

```brix crash
(main
  (print (as (as 300foo Bar) UnitString))
)

(blank)
(type-line + Foo (alias foo) (is I64))
(type-line + Bar (alias bar) (is I64))
(type-line + Baz (alias baz) (is I64))
(type-line + Baz2 (alias baz2) (is I64))
(blank)
(unit + 1baz 1bar)
(unit + 1baz2 1foo)
```

```output
Exception: Foo cannot be cast to Bar; there is no conversion between them
<no file:3>
      (print (as (as 300foo Bar) UnitString))
                 ^^^^^^^^^^^^^^^
```

### A number of one unit becomes one of another by the ratio between them, in casts and arithmetic.

```brix
(main
  (print (as 72bar Foo))
  (print (as 72foo Bar))
  (print (+ 1foo 2bar))
  (print (+ 2bar 1foo))
  (print (as (+ 1foo 2bar) UnitString))
  (print (as (+ 2bar 1foo) UnitString))
  (print 3foo)
  (print 1.5foo)
  (print (as 3foo UnitString))
  (print (* 3foo 2foo))
  (print (- 3foo 5foo))
  (print (/ 10foo 4foo))
  (print (as 5km M))
  (print (as 1500m Km))
  (print (as (+ 2km 500m) UnitString))
  (print (as (+ 500m 2km) UnitString))
  (print (< 3km 5km))
  (print (< 3km 500m))
  (print (== 3km 3000m))
  (print (< 72bar 40foo))
  (print (== 72bar 36foo))
  (print (as 5m I32))
  (print (as 5m F64))
)

(type + Foo (alias foo) (is F64))

(type + Bar (alias bar) (is 0.5foo))

(type + M (alias m) (is I32))

(type + Km (alias km) (is 1000m))
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

```brix
(main
  (print (as 8bar Foo))
  (print (as (+ 1bar 4foo) UnitString))
  (print (as 3baz Foo))
  (print (as 16qux Foo))
)

(blank)
(type-line + Foo (alias foo) (is I32))
(type-line + Bar (alias bar) (is I32))
(type-line + Baz (alias baz) (is I32))
(type-line + Qux (alias qux) (is I32))
(blank)
(unit + 1bar (/lshift/ 2foo 1))
(unit + 1baz (/and/ 6foo 3))
(unit + 8qux (/rshift/ 16foo 2))
```

```output
32
2bar
6
8
```

### A unit declared through an operator that gives no number.

```brix crash
(type-line + Foo (alias foo) (is I32))
(type-line + Bar (alias bar) (is I32))
(blank)
(unit + 1bar (< 2foo 1))
```

```output
Exception: Invalid constant expression
<no file:5>
    (unit + 1bar (< 2foo 1))
                 ^^^^^^^^^^
```

### Arithmetic on units as the operand of another operator.

```brix
(main
  (print (as (+ 1foo 2bar 3foo) UnitString))
  (print (* (+ 1foo 2bar) 2foo))
  (print (< (+ 1foo 2bar) 3foo))
  (print (== (+ 1foo 2bar) 2foo))
  (print (as (+ 2bar 1foo 2bar) UnitString))
  (print (as (+ 1foo 2bar) Bar))
  (print (- 3foo (+ 1foo 2bar)))
  (print (+ 5km 500m 1km))
  (print (as (+ 5km 500m 1km) M))
  (print (< (+ 2km 500m) 3km))
)

(type + Foo (alias foo) (is F64))

(type + Bar (alias bar) (is 0.5foo))

(type + M (alias m) (is I32))

(type + Km (alias km) (is 1000m))
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

```brix
(main
  (let x 3foo)
  (+= x 2bar)
  (print (as x UnitString))
  (+= x 1)
  (print (as x UnitString))
  (*= x 2foo)
  (print (as x UnitString))
  (let f 1.5)
  (+= f 2bar)
  (print f)
  (let y 2bar)
  (+= y 6foo)
  (print (as y UnitString))
  (let i 3.0)
  (+= i 2bar)
  (print i)
)

(blank)
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is F64))
(blank)
(unit + 1bar 3foo)
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

```brix crash
(main
  (print 5zz)
)
```

```output
Exception: Unit "zz" not found
<no file:3>
      (print 5zz)
              ^^
```

### A number written with a unit whose class declares a constructor is what that constructor returns.

```brix
(main
  (print (as 4foo UnitString))
  (print (+ 4foo 1))
  (print (as 7bar UnitString))
)

(type + Foo (alias foo) (is Int)
  (fn-do + This _
    (param n _)
    (return 99)
  )
)

(type + Bar (alias bar) (is Int))
```

```output
99i
100
7bar
```

### A default written with a unit whose class declares a constructor is evaluated once, as its module starts.

```brix
(main
  (print "start")
  (print (call big 3foo))
  (print (call twice 1bar))
)

(type + Foo (alias foo) (is Int)
  (fn-do + This _
    (param n _)
    (print (str "foo " n))
    (return (+ n 100))
  )
)

(type + Bar (alias bar) (is Float)
  (fn-do + This _
    (param n _)
    (print (str "bar " n))
    (return (* n 2))
  )
)

(fn-expr + big Foo
  (param f Foo (= 9foo))
  f
)

(fn-expr + twice _
  (param f _ (= 9bar))
  (* f 2)
)
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

```brix
(main
  (print (call half (as 7 Ticks)))
  (print (call half 7.0))
  (print (+ (call half 7ticks) 1))
)

(type + Ticks (alias ticks) (is I64))

(fn-expr + half _
  (param value I32)
  (str "I32 " (/ value 2))
)

(fn-expr + half _
  (param value I64)
  (/ value 2)
)

(fn-expr + half _
  (param value F32)
  (str "F32 " (/ value 2))
)

(fn-expr + half _
  (param value F64)
  (/ value 2)
)
```

```output
3
3.5
4
```

### A number of one unit becomes a whole number of another exactly, through a chain of units.

```brix
(main
  (print (as 2880minutes Days UnitString))
  (print (as 2days Minutes UnitString))
  (print (as 10080minutes Weeks UnitString))
  (print (as 1weeks Minutes UnitString))
  (print (as 90minutes Hours UnitString))
  (print (as 4320minutes Days))
)

(blank)
(type-line + Minutes (alias minutes) (is F64))
(type-line + Hours (alias hours) (is 60minutes))
(type-line + Days (alias days) (is 24hours))
(type-line + Weeks (alias weeks) (is 7days))
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

```brix
(main
  (print (as 36inches Feet UnitString))
  (print (as 3ft Inches UnitString))
  (print (as 30inches Feet))
)

(blank)
(type-line + Millimeters (alias mm) (is F64))
(type-line + Centimeters (alias cm) (is 10mm))
(type-line + Inches (alias inches) (is 2.54cm))
(type-line + Feet (alias ft) (is 12inches))
```

```output
3ft
36inches
2.5
```

### A ratio written as a decimal stays exact through a chain of units.

```brix
(main
  (print (as 1ft M UnitString))
  (print (as 10ft M UnitString))
)

(blank)
(type-line + Mm (alias mm) (is F64))
(type-line + Cm (alias cm) (is 10mm))
(type-line + M (alias m) (is 100cm))
(type-line + Inches (alias inches) (is F64))
(type-line + Feet (alias ft) (is 12inches))
(unit + 1inches 25.4mm)
```

```output
0.3048m
3.048m
```

### A unit converts to any unit its declarations reach, however long the chain, in arithmetic as in a cast.

```brix
(main
  (print (as 1q10 Q0 UnitString))
  (print (as (+ 1q0 1q10) UnitString))
)

(blank)
(type-line + Q0 (alias q0) (is F64))
(type-line + Q1 (alias q1) (is 2q0))
(type-line + Q2 (alias q2) (is 2q1))
(type-line + Q3 (alias q3) (is 2q2))
(type-line + Q4 (alias q4) (is 2q3))
(type-line + Q5 (alias q5) (is 2q4))
(type-line + Q6 (alias q6) (is 2q5))
(type-line + Q7 (alias q7) (is 2q6))
(type-line + Q8 (alias q8) (is 2q7))
(type-line + Q9 (alias q9) (is 2q8))
(type-line + Q10 (alias q10) (is 2q9))
```

```output
1024q0
1025q0
```

### Subtracting, dividing or multiplying across units converts the right side to the unit of the left.

```brix
(main
  (print (as (- 8hours 30minutes) UnitString))
  (print (as (/ 8hours 120minutes) UnitString))
  (print (as (* 4hours 120minutes) UnitString))
)

(blank)
(type-line + Minutes (alias minutes) (is F64))
(type-line + Hours (alias hours) (is 60minutes))
```

```output
7.5hours
4hours
8hours
```

### A unit declared on a number type of its own is one of that type, so a value held as the type keeps its size.

```brix
(main
  (let holder (call Holder))
  (print (as (. holder span) Small))
  (print (call take 3bg))
  (print (as (as 2bg Base) Big))
  (print (as 2bg F64))
)

(blank)
(type-line + Base (is F64))
(type-line + Small (alias sm) (is Base))
(type-line + Big (alias bg) (is 1000sm))

(fn-expr + take _
  (param n Base)
  (as n Small)
)

(type + Holder
  (prop + span Base (= 2bg))
)
```

```output
2000
3000
2
2
```

### Units declared from a number type of the language join a family only by their equalities, so each converts by its own ratio.

```brix
(main
  (print (as (+ 8hours 30minutes) UnitString))
  (print (as (+ 30minutes 8hours) UnitString))
  (print (as 2hour Minutes UnitString))
  (print (as 120minute Hour UnitString))
  (print (as 7200second Hour UnitString))
)

(blank)
(type-line + Seconds (alias seconds) (is F64))
(type-line + Minutes (alias minutes) (is 60seconds))
(type-line + Hours (alias hours) (is 60minutes))
(type-line + Second (alias second) (is U64))
(type-line + Minute (alias minute) (is U64))
(type-line + Hour (alias hour) (is U64))
(unit + 1second 1seconds)
(unit + 1minute 1minutes)
(unit + 1hour 1hours)
```

```output
8.5hours
510minutes
120minutes
2hour
2hour
```

### Arithmetic between a unit and a plain number keeps the unit, on either side of a multiplication.

```brix
(main
  (let scaled (* 2km 2))
  (print (as scaled UnitString))
  (print (as (* 2 2km) UnitString))
  (print (as (/ 6km 2) UnitString))
  (print (call meters scaled))
  (print (call meters (* 2 2km)))
  (print (call meters (+ (* 2km 2) 1km)))
)

(fn-expr + meters F64
  (param distance Foo)
  (as distance F64)
)

(type + Foo (alias foo) (is F64))

(type + Km (alias km) (is 1000foo))
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

```brix
(import /units/distance)
(blank)

(main
  (print (as 1meters Millimeters))
  (print (as 1km Meters))
  (print (as 1µm Nanometers))
  (print (as 1miles Meters))
  (print (as 1inches Millimeters))
)
```

```output
1000
1000
1000
1609.34
25.4
```

### An integer written with a unit that its number type cannot hold is rejected, as a plain one is.

```brix crash
(type-line + Tk (alias tk) (is U8))
(blank)

(main
  (print 300tk)
)
```

```output
Exception: Integer literal 300 does not fit Tk
<no file:6>
      (print 300tk)
             ^^^^^
```

### A symbol written after a number that names no unit is named in the error.

```brix crash
(main
  (print 25%)
)
```

```output
Exception: Unit "%" not found
<no file:3>
      (print 25%)
               ^
```

### A number with a fraction written in a unit that holds whole numbers is rejected.

```brix crash
(type-line + M (alias m) (is I32))
(blank)

(main
  (print 3.7m)
)
```

```output
Exception: M holds whole numbers, as I32 does, so 3.7m cannot be written; write a whole number
<no file:6>
      (print 3.7m)
             ^^^^
```

### A number written in the unit of a class that is no number and has no constructor is rejected.

```brix crash
(type-line + Name (alias nm) (is String))
(blank)

(main
  (print 5nm)
)
```

```output
Exception: Name is no number and declares no constructor that takes one, so no number can be written in its unit; declare Name as a number type
<no file:6>
      (print 5nm)
             ^^^
```

### A unit declared as a number of a unit that names nothing is reported at the declaration.

```brix crash
(main
  (print 2bar)
)

(type + Bar (alias bar) (is 2zz))
```

```output
Exception: Unit "zz" not found
<no file:6>
    (type + Bar (alias bar) (is 2zz))
                                 ^^
```

### A unit declared as zero of another is rejected, as no number of it would convert.

```brix crash
(main
  (print (as 2bar Foo UnitString))
)

(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is 0foo))
```

```output
Exception: A unit cannot be declared as zero of another, which no number of it would convert from; write a number other than zero
<no file:7>
    (type-line + Bar (alias bar) (is 0foo))
                                     ^^^^
```

### An equality with a side in a number type of the language is rejected.

```brix crash
(main
  (print (as 2bar F64))
)

(type-line + Bar (alias bar) (is F64))
(unit + 1bar 3f)
```

```output
Exception: An equality relates units a program declares, and F64 is a number type of the language; write each side in a unit the program declares
<no file:7>
    (unit + 1bar 3f)
    ^^^^^^^^^^^^^^^^
```

### A unit suffix a module declares twice is rejected.

```brix crash
(main
  (print 2q)
)

(type-line + A (alias q) (is F64))
(type-line + B (alias q) (is I32))
```

```output
Exception: "q" already names A; give B a suffix of its own
<no file:7>
    (type-line + B (alias q) (is I32))
                                 ^^^
```

### A unit suffix that a class of an imported module has is rejected, as it would hide that class's.

```brix crash
(main
  (print 'a')
)

(type-line + C (alias c) (is F64))
```

```output
Exception: "c" already names Char; give C a suffix of its own
<no file:6>
    (type-line + C (alias c) (is F64))
                                 ^^^
```

### Two equalities between the same units that disagree are rejected.

```brix crash
(main
  (print (as 1bar Foo UnitString))
)

(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is 2foo))
(unit + 1bar 3foo)
```

```output
Exception: Equalities between Bar and Foo disagree: one makes 1 Bar 2 Foo, another 3 Foo; keep one of them
<no file:8>
    (unit + 1bar 3foo)
    ^^^^^^^^^^^^^^^^^^
```

### An equality that disagrees with a path through other units is rejected.

```brix compile-error
(main
  (print (as 1baz Bar UnitString))
)

(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is 2foo))
(type-line + Baz (alias baz) (is F64))
(unit + 1baz 1foo)
(unit + 1baz 1bar)
```

```output
Exception: Equalities between Baz and Bar disagree: this one makes 1 Baz 1 Bar, but through Foo they make 1 Baz 0.5 Bar; keep every path between two units in agreement
<no file:10>
    (unit + 1baz 1bar)
    ^^^^^^^^^^^^^^^^^^
```

### An equality between a unit and itself is rejected unless it makes one of one.

```brix compile-error
(main
  (print (as 1foo Foo UnitString))
)

(type-line + Foo (alias foo) (is F64))
(unit + 2foo 1foo)
```

```output
Exception: An equality between Foo and itself makes 1 Foo 0.5 Foo, but a unit is always 1 of itself; relate Foo to another unit
<no file:7>
    (unit + 2foo 1foo)
    ^^^^^^^^^^^^^^^^^^
```

### Units declared from one number type of a module's own are one another, so equalities that give them different ratios elsewhere are rejected where the module is imported.

```brix compile-error
(import ./lib/clock)

(main
  (print (as (+ 8hours 30minutes) UnitString))
)
```

```brix file=lib/clock/clock.bx
(type-line + Seconds (alias seconds) (is F64))
(type-line + Minutes (alias minutes) (is 60seconds))
(type-line + Hours (alias hours) (is 60minutes))
(type-line + Reading (is U64))
(type-line + Minute (alias minute) (is Reading))
(type-line + Hour (alias hour) (is Reading))
(unit + 1hour 1hours)
(unit + 1minute 1minutes)
```

```output
Exception: Equalities between Minute and Minutes disagree: this one makes 1 Minute 1 Minutes, but through Reading, Hour and Hours they make 1 Minute 60 Minutes; keep every path between two units in agreement
<lib/clock/clock.bx:8>
    (unit + 1minute 1minutes)
    ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An equality that adds a number with no unit to one in a unit is rejected, as it is no ratio.

```brix crash
(main
  (print (as 1bar Foo UnitString))
)

(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is F64))
(unit + 1bar (+ 2foo 5))
```

```output
Exception: An equality relates two units by a ratio alone, so a number with no unit cannot be added to or taken from one; write the ratio as one number in the unit
<no file:8>
    (unit + 1bar (+ 2foo 5))
                         ^
```
