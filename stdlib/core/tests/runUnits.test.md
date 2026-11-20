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
(unit + 1bar (/ 1foo 3))
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
(unit + 1bar (* (%% 2 3) 1foo))
```

```output
3bar
576foo
```

### A side that divides by a unit declares a unit per that unit, and one that raises a unit to a whole power declares that power of it.

```brix
(main
  (print (as (* 6bar 2foo) UnitString))
  (print (as (/ 2 1foo) UnitString))
  (print (as (* 3foo 4foo) UnitString))
)

(blank)
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is F64))
(type-line + Square (alias sq) (is F64))
(blank)
(unit + 1bar (/ 1 3foo))
(unit + 1sq (%% 1foo 2))
```

```output
4f
6bar
12sq
```

### A unit is raised only to a whole number with no unit.

```brix compile-error
(main
  (print (as 1bar UnitString))
)

(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is F64))
(unit + 1bar (%% 2 3foo))
```

```output
Exception: A unit is raised only to a whole number with no unit, as in "(%% 1meters 2)"
<no file:8>
    (unit + 1bar (%% 2 3foo))
                       ^^^^
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
(unit + 1bar (+ (/ 1foo 3) 0foo))
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
(unit + 1 (/ 1 3))
```

```output
Exception: Ambiguous units
<no file:5>
    (unit + 1 (/ 1 3))
    ^^^^^^^^^^^^^^^^^^
```

```brix crash
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is I32))
(blank)
(unit + 1.0bar (+ 1foo 3bar))
```

```output
Exception: Conflicting units
<no file:5>
    (unit + 1.0bar (+ 1foo 3bar))
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
  (print (* 3foo 2))
  (print (- 3foo 5foo))
  (print (/ 10foo 4foo))
  (print (as 5km M))
  (print (as 1500m Km))
  (print (as (+ 2km 3000m) UnitString))
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
5km
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
  (print (* (+ 1foo 2bar) 2))
  (print (< (+ 1foo 2bar) 3foo))
  (print (== (+ 1foo 2bar) 2foo))
  (print (as (+ 2bar 1foo 2bar) UnitString))
  (print (as (+ 1foo 2bar) Bar))
  (print (- 3foo (+ 1foo 2bar)))
  (print (+ 5km 2000m 1km))
  (print (as (+ 5km 2000m 1km) M))
  (print (< (+ 500m 2km) 3km))
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
8
8000
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
  (*= x 2)
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

### Subtracting across units converts the right side to the unit of the left, and dividing gives a plain number.

```brix
(main
  (print (as (- 8hours 30minutes) UnitString))
  (print (as (/ 8hours 120minutes) UnitString))
)

(blank)
(type-line + Minutes (alias minutes) (is F64))
(type-line + Hours (alias hours) (is 60minutes))
```

```output
7.5hours
4f
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

### A module's equalities are joined after those of the modules it imports, so an equality that disagrees with an imported one is rejected in the module that imports it.

```brix compile-error
(import ./lib/lengths)

(unit + 1yard 1meter)

(main
  (print (as 1yard Meter UnitString))
)
```

```brix file=lib/lengths/lengths.bx
(type-line + Meter (alias meter) (is F64))
(type-line + Yard (alias yard) (is F64))
(unit + 1yard 0.9144meter)
```

```output
Exception: Equalities between Yard and Meter disagree: one makes 1 Yard 0.9144 Meter, another 1 Meter; keep one of them
<no file:4>
    (unit + 1yard 1meter)
    ^^^^^^^^^^^^^^^^^^^^^
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

### A unit stored as a whole number takes another on its own only where no fraction is lost, and `as` drops the fraction toward zero.

```brix
(main
  (let c Cents 2dollars)
  (print (as c UnitString))
  (print (as (+ 250cents 1dollars) UnitString))
  (let total Cents 1cents)
  (+= total 1dollars)
  (print (as total UnitString))
  (let d Dollars 300cents)
  (print (as d UnitString))
  (print (as (as 250cents Dollars) UnitString))
  (print (as (as -250cents Dollars) UnitString))
  (let h Hour 120minutes)
  (print (as h UnitString))
  (let n Hour -120minutes)
  (print (as n UnitString))
)

(type-line + Cents (alias cents) (is I64))
(type-line + Dollars (alias dollars) (is 100cents))
(type-line + Minutes (alias minutes) (is F64))
(type-line + Hour (alias hour) (is I64))
(unit + 1hour 60minutes)
```

```output
200cents
350cents
101cents
3dollars
2dollars
-2dollars
2hour
-2hour
```

### A literal whose amount comes out whole is taken by a parameter or an operator in a unit stored as whole numbers, and a generic function keeps the literal's own unit.

```brix
(main
  (print (call wait 120minutes))
  (print (call wait -60minutes))
  (print (as (call same 90minutes) UnitString))
  (let m Minutes 30minutes)
  (print (as (call same m) UnitString))
  (let start (call Stamp 5hour))
  (print (as (. (- start 120minutes) at) UnitString))
  (print (as (. (- start 1hour) at) UnitString))
)

(type-line + Minutes (alias minutes) (is F64))
(type-line + Hour (alias hour) (is I64))
(unit + 1hour 60minutes)

(type + Stamp (is Tuple)
  (prop + at Hour)
  (fn-expr + (infix this - (param other Hour)) _
    (call Stamp (- at other))
  )
)

(fn-expr + wait String
  (param h Hour)
  (as h UnitString)
)

(fn-expr + same T
  (param x ''T)
  x
)
```

```output
2hour
-1hour
90minutes
30minutes
3hour
4hour
```

### A literal whose amount is no whole number is rejected by a parameter in a unit stored as whole numbers, though the same call took a whole one.

```brix compile-error
(main
  (print (call wait 120minutes))
  (print (call wait 90minutes))
)

(type-line + Minutes (alias minutes) (is F64))
(type-line + Hour (alias hour) (is I64))
(unit + 1hour 60minutes)

(fn-expr + wait String
  (param h Hour)
  (as h UnitString)
)
```

```output
Exception: No function "wait" accepts (Minutes); it is declared as "wait(h Hour)"
<no file:4>
      (print (call wait 90minutes))
             ^^^^^^^^^^^^^^^^^^^^^
```

### A comparison whose left unit would drop the right's fraction compares in the right's unit.

```brix
(main
  (print (< 3dollars 350cents))
  (print (== 1dollars 150cents))
  (print (> 350cents 3dollars))
  (print (== 1dollars 100cents))
  (print (< 1hour 90minutes))
)

(type-line + Cents (alias cents) (is I64))
(type-line + Dollars (alias dollars) (is 100cents))
(type-line + Minutes (alias minutes) (is F64))
(type-line + Hour (alias hour) (is I64))
(unit + 1hour 60minutes)
```

```output
true
false
true
true
true
```

### A literal whose amount is no whole number of a unit stored as one is rejected unasked.

```brix compile-error
(main
  (let d Dollars 250cents)
  (print d)
)

(type-line + Cents (alias cents) (is I64))
(type-line + Dollars (alias dollars) (is 100cents))
```

```output
Exception: Expected Dollars here, but this is Cents, which becomes Dollars only when asked, since Dollars holds whole numbers and drops any fraction; write "(as 250cents Dollars)"
<no file:3>
      (let d Dollars 250cents)
             ^^^^^^^
```

### The right operand of arithmetic is rejected where the left's unit would drop its fraction.

```brix compile-error
(main
  (print (+ 1dollars 250cents))
)

(type-line + Cents (alias cents) (is I64))
(type-line + Dollars (alias dollars) (is 100cents))
```

```output
Exception: Expected Dollars here, but this is Cents, which becomes Dollars only when asked, since Dollars holds whole numbers and drops any fraction; write "(as 250cents Dollars)"
<no file:3>
      (print (+ 1dollars 250cents))
                         ^^^^^^^^
```

### An assigning operator rejects a value whose fraction the unit of its place would drop.

```brix compile-error
(main
  (let total Dollars 1dollars)
  (let change Cents 250cents)
  (+= total change)
  (print total)
)

(type-line + Cents (alias cents) (is I64))
(type-line + Dollars (alias dollars) (is 100cents))
```

```output
Exception: Expected Dollars here, but this is Cents, which becomes Dollars only when asked, since Dollars holds whole numbers and drops any fraction; write "(as change Dollars)"
<no file:5>
      (+= total change)
                ^^^^^^
```

### A unit stored as a float is rejected unasked where a unit stored as a whole number is wanted.

```brix compile-error
(main
  (let wait Minutes 90minutes)
  (let h Hour wait)
  (print h)
)

(type-line + Minutes (alias minutes) (is F64))
(type-line + Hour (alias hour) (is I64))
(unit + 1hour 60minutes)
```

```output
Exception: Expected Hour here, but this is Minutes, which becomes Hour only when asked, since Hour holds whole numbers and drops any fraction; write "(as wait Hour)"
<no file:4>
      (let h Hour wait)
             ^^^^
```

### A return whose fraction the declared unit would drop is rejected.

```brix compile-error
(main
  (print (call back 250cents))
)

(type-line + Cents (alias cents) (is I64))
(type-line + Dollars (alias dollars) (is 100cents))

(fn-expr + back Dollars
  (param amount Cents)
  amount
)
```

```output
Exception: Expected Dollars here, but this is Cents, which becomes Dollars only when asked, since Dollars holds whole numbers and drops any fraction; write "(as amount Dollars)"
<no file:11>
      amount
      ^^^^^^
```

### A unit that no declaration relates to another keeps its unit through `+`, `-` and `%` and through scaling by a plain number, and one stored as an integer keeps it only where the arithmetic stays in integers.

```brix
(main
  (print (as (+ 3em 2em) UnitString))
  (print (as (- 3em 2em) UnitString))
  (print (as (% 7em 2em) UnitString))
  (print (as (* 3em 0.5) UnitString))
  (print (as (* 2 3em) UnitString))
  (print (as (/ 3em 2) UnitString))
  (print (as (* 3em 2em) UnitString))
  (print (as (+ 3tk 2tk) UnitString))
  (print (as (* 3tk 2) UnitString))
  (print (as (* 3tk 0.5) UnitString))
  (let width Em 10em)
  (+= width 5)
  (print (as width UnitString))
  (let scale F64 1.0)
  (+= scale 2em)
  (print (as scale UnitString))
)

(type-line + Em (alias em) (is F64))
(type-line + Tk (alias tk) (is I32))
```

```output
5em
1em
1em
1.5em
6em
1.5em
6f
5tk
6tk
1.5f
15em
3f
```

### A unit of a family stored as an integer keeps its unit only where the arithmetic stays in integers, so a plain fraction gives a plain number.

```brix
(main
  (let half 0.5)
  (print (as (+ 3dollars 0.5) UnitString))
  (print (as (* 3dollars 0.5) UnitString))
  (print (as (* 0.5 3dollars) UnitString))
  (print (as (/ 3dollars half) UnitString))
  (print (as (* 3dollars 2) UnitString))
  (print (as (/ 3dollars 2) UnitString))
)

(type-line + Cents (alias cents) (is I64))
(type-line + Dollars (alias dollars) (is 100cents))
```

```output
3.5f
1.5f
1.5f
6f
6dollars
1dollars
```

### A plain number on the left of `+`, `-` or `%` is taken in the unit on the right.

```brix
(main
  (print (as (+ 30 90secs) UnitString))
  (print (as (- 100 1secs) UnitString))
  (print (as (% 7 2secs) UnitString))
  (print (as (/ 10 2secs) UnitString))
  (print (as (+ 2 2dollars) UnitString))
  (print (as (+ 1.5 2dollars) UnitString))
)

(type-line + Secs (alias secs) (is F64))
(type-line + Cents (alias cents) (is I64))
(type-line + Dollars (alias dollars) (is 100cents))
```

```output
120secs
99secs
1secs
5f
4dollars
3.5f
```

### Adding units of two families that no equality joins is rejected, as comparing them is.

```brix compile-error
(main
  (print (+ 100meters 20secs))
)

(type-line + Meters (alias meters) (is F64))
(type-line + Secs (alias secs) (is F64))
```

```output
Exception: Expected Meters here, but this is Secs, and there is no conversion between them
<no file:3>
      (print (+ 100meters 20secs))
                          ^^^^^^
```

### An assigning operator rejects a value in a unit of another family than its place's.

```brix compile-error
(main
  (let width Em 1em)
  (-= width 2tk)
  (print width)
)

(type-line + Em (alias em) (is F64))
(type-line + Tk (alias tk) (is I32))
```

```output
Exception: Expected Em here, but this is Tk, and there is no conversion between them
<no file:4>
      (-= width 2tk)
                ^^^
```

### Multiplying or dividing units of two families that no equality joins gives a plain number.

```brix
(main
  (print (as (* 100meters 20secs) UnitString))
  (print (as (/ 3em 2secs) UnitString))
)

(type-line + Meters (alias meters) (is F64))
(type-line + Secs (alias secs) (is F64))
(type-line + Em (alias em) (is F64))
```

```output
2000f
1.5f
```

### Dividing a unit by another of its family gives a plain number, worked out in whichever unit holds the other with no fraction lost.

```brix
(main
  (print (as (/ 500ms 2s) UnitString))
  (print (as (/ 30minutes 2hours) UnitString))
  (print (as (/ 4hours 2hours) UnitString))
  (print (as (/ 300cents 1dollars) UnitString))
  (print (as (/ 3dollars 50cents) UnitString))
  (print (as (/ 1dollars 300cents) UnitString))
  (print (as (* (/ 500ms 2s) 4) UnitString))
  (print (as (/ 7ms 2) UnitString))
  (print (as (% 7ms 2ms) UnitString))
)

(type-line + Ms (alias ms) (is F64))
(type-line + S (alias s) (is 1000ms))
(type-line + Minutes (alias minutes) (is 60s))
(type-line + Hours (alias hours) (is 60minutes))
(type-line + Cents (alias cents) (is I64))
(type-line + Dollars (alias dollars) (is 100cents))
```

```output
0.25f
0.25f
2f
3i
6i
0i
1f
3.5ms
1ms
```

### An assigning divide by a unit of its place's family keeps the place's unit.

```brix
(main
  (let wait S 10s)
  (/= wait 500ms)
  (print (as wait UnitString))
)

(type-line + Ms (alias ms) (is F64))
(type-line + S (alias s) (is 1000ms))
```

```output
20s
```

### An equality between a unit and a plain number makes the unit a pure number, which scales what it multiplies or divides by its value as a number.

```brix
(main
  (print (as (* 50pc 50pc) UnitString))
  (print (as (* 50pc 200px) UnitString))
  (print (as (* 200px 50pc) UnitString))
  (print (as (/ 200px 50pc) UnitString))
  (print (as (/ 50pc 25pc) UnitString))
  (print (as (/ 1 50pc) UnitString))
  (print (as (* 2pt 50pc) UnitString))
)

(type-line + Share (alias pc) (is F64))
(type-line + Part (alias pt) (is F64))
(type-line + Pixels (alias px) (is F64))
(unit + 100pc 1)
(unit + 1pt 1)
```

```output
25pc
100px
100px
400px
2f
2f
1pt
```

### A pure number keeps its unit through a plain number, which is taken as written, and becomes a plain number by its value in its own unit.

```brix
(main
  (let q (/ 1.0 2.0))
  (print (as (* 100pc q) UnitString))
  (print (as (* 50pc 2) UnitString))
  (print (as (+ 50pc 1) UnitString))
  (print (as (as 0.25 Share) UnitString))
  (let f F64 (* 2.0 50pc))
  (print f)
)

(type-line + Share (alias pc) (is F64))
(unit + 100pc 1)
```

```output
50pc
100pc
51pc
0.25pc
100
```

### An assigning multiply or divide by a pure number scales the place's value.

```brix
(main
  (let width Pixels 200px)
  (*= width 50pc)
  (print (as width UnitString))
  (/= width 25pc)
  (print (as width UnitString))
  (let share Share 50pc)
  (*= share 50pc)
  (print (as share UnitString))
  (let plain F64 1.0)
  (/= plain 50pc)
  (print plain)
)

(type-line + Share (alias pc) (is F64))
(type-line + Pixels (alias px) (is F64))
(unit + 100pc 1)
```

```output
100px
400px
25pc
2
```

### A pure number scaling a unit stored as an integer gives a plain number.

```brix
(main
  (print (as (* 10ct 50pc) UnitString))
  (print (as (* 50pc 10ct) UnitString))
)

(type-line + Share (alias pc) (is F64))
(type-line + Count (alias ct) (is I64))
(unit + 100pc 1)
```

```output
5f
5f
```

### An assigning multiply by a pure number into a unit stored as an integer is rejected.

```brix compile-error
(main
  (let count Count 10ct)
  (*= count 50pc)
)

(type-line + Share (alias pc) (is F64))
(type-line + Count (alias ct) (is I64))
(unit + 100pc 1)
```

```output
Exception: Expected Count here, but this is F64, which becomes Count only when asked; write "(as (*= count 50pc) Count)"
<no file:4>
      (*= count 50pc)
      ^^^^^^^^^^^^^^^
```

### Equalities that give a pure number two values are rejected.

```brix compile-error
(main
  (print (as 1pc UnitString))
)

(type-line + Share (alias pc) (is F64))
(unit + 100pc 1)
(unit + 10pc 1)
```

```output
Exception: Equalities between Share and plain numbers disagree: one makes 1 Share 0.01, another 0.1; keep one of them
<no file:8>
    (unit + 10pc 1)
    ^^^^^^^^^^^^^^^
```

### An equality that makes a unit a pure number apart from a path through another is rejected.

```brix compile-error
(main
  (print (as 1pt UnitString))
)

(type-line + Share (alias pc) (is F64))
(type-line + Part (alias pt) (is 0.1pc))
(unit + 100pc 1)
(unit + 1pt 1)
```

```output
Exception: Equalities between Part and plain numbers disagree: this one makes 1 Part 1, but through Share they make 1 Part 0.001; keep every path between two units in agreement
<no file:9>
    (unit + 1pt 1)
    ^^^^^^^^^^^^^^
```

### A unit declared as zero of a plain number is rejected.

```brix compile-error
(main
  (print (as 1pc UnitString))
)

(type-line + Share (alias pc) (is F64))
(unit + 1pc 0)
```

```output
Exception: A unit cannot be declared as zero, which no number of it would convert from; write a number other than zero
<no file:7>
    (unit + 1pc 0)
                ^
```

### Ratios and percentages from `/units` are pure numbers.

```brix
(import /units)

(main
  (print (as (* 50% 50%) UnitString))
  (print (as (* 50% 200px) UnitString))
  (print (as (* 200px 50%) UnitString))
  (print (as (/ 50% 25%) UnitString))
  (print (as (* 25% 2%%) UnitString))
  (print (as (/ 1 50%) UnitString))
  (print (as (+ 50% 1) UnitString))
  (print (as (as 0.25 Percent) UnitString))
  (print (as (as 25% Ratio) UnitString))
)

(type-line + Pixels (alias px) (is F64))
```

```output
25%
100px
100px
2f
50%
2f
51%
0.25%
0.25%%
```

### Multiplying or dividing units gives the unit an equality declares as their product or quotient, converted to its scale.

```brix
(import /time/duration)
(import /units)

(main
  (print (as (* 50kph 2hours) UnitString))
  (print (as (* 50kph 30minutes) UnitString))
  (print (as (/ 100km 2hours) UnitString))
  (print (as (/ 100meters 8s) UnitString))
  (print (as (/ 100km 50kph) UnitString))
  (print (as (* 2mps 1minutes) UnitString))
  (print (as (* 50hz 2s) UnitString))
  (print (as (/ 1 500ms) UnitString))
)

(type-line + Kph (alias kph) (is F64))
(type-line + Mps (alias mps) (is F64))
(type-line + Hertz (alias hz) (is F64))
(unit + 1kph (/ 1km 1hours))
(unit + 1mps (/ 1meters 1s))
(unit + 1hz (/ 1 1s))
```

```output
100km
25km
50kph
12.5mps
2hours
120meters
100f
2hz
```

### Units declared as amounts of the same units are one family, whichever module declares each, and a quotient takes the one declared from its operands' units.

```brix
(import /time/duration)
(import /units)
(import ./lib/speeds)

(main
  (print (as (as 1mps Kph) UnitString))
  (print (as (+ 1mps 36kph) UnitString))
  (print (as (/ 100km 2hours) UnitString))
  (print (as (/ 100meters 8s) UnitString))
)

(type-line + Kph (alias kph) (is F64))
(unit + 1kph (/ 1km 1hours))
```

```brix file=lib/speeds/speeds.bx
(import /time/duration)
(import /units)

(type-line + Mps (alias mps) (is F64))
(unit + 1mps (/ 1meters 1s))
```

```output
3.6kph
11mps
50kph
12.5mps
```

### A product or a quotient in a unit stored as a whole number is a plain number.

```brix
(import /time/duration)
(import /units)

(main
  (print (as (/ 30km 2hours) UnitString))
)

(type-line + Kmh (alias kmh) (is I32))
(unit + 1kmh (/ 1km 1hours))
```

```output
15f
```

### Multiplying two units of a family is rejected where no unit is declared as their product.

```brix compile-error
(import /time/duration)

(main
  (print (as (* 4hours 120minutes) UnitString))
)
```

```output
Exception: No unit is declared as Hours times Hours; declare one with an equality, or make one side a plain number
<no file:5>
      (print (as (* 4hours 120minutes) UnitString))
                 ^^^^^^^^^^^^^^^^^^^^^
```

### Multiplying units of two families is rejected where no unit is declared as their product.

```brix compile-error
(import /time/duration)
(import /units)

(main
  (print (as (* 3meters 2s) UnitString))
)
```

```output
Exception: No unit is declared as Meters times Seconds; declare one with an equality, or make one side a plain number
<no file:6>
      (print (as (* 3meters 2s) UnitString))
                 ^^^^^^^^^^^^^^
```

### A quotient that two declared units measure, neither from its operands' units, is rejected.

```brix compile-error
(import /time/duration)
(import /units)

(main
  (print (as (/ 1km 2s) UnitString))
)

(type-line + Kph (alias kph) (is F64))
(type-line + Mps (alias mps) (is F64))
(unit + 1kph (/ 1km 1hours))
(unit + 1mps (/ 1meters 1s))
```

```output
Exception: Kilometers per Seconds could be Kph or Mps; convert an operand to the units one of them is declared from
<no file:6>
      (print (as (/ 1km 2s) UnitString))
                 ^^^^^^^^^^
```

### An assigning operator rejects a product or a quotient that its place's unit cannot hold.

```brix compile-error
(import /time/duration)
(import /units)

(main
  (let distance Meters 100meters)
  (/= distance 2s)
)

(type-line + Mps (alias mps) (is F64))
(unit + 1mps (/ 1meters 1s))
```

```output
Exception: Dividing Meters by Seconds gives Mps, which a place of Meters cannot hold
<no file:7>
      (/= distance 2s)
                   ^^
```

### A unit declared from a unit of its own family is rejected.

```brix compile-error
(import /time/duration)
(import /units)

(main
  (print (as 1mps UnitString))
)

(type-line + Mps (alias mps) (is F64))
(unit + 1mps 1meters)
(unit + 1mps (/ 1meters 1s))
```

```output
Exception: Mps is declared from Meters, a unit of its own family; declare it from units of other families
<no file:11>
    (unit + 1mps (/ 1meters 1s))
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Units declared from each other are rejected.

```brix compile-error
(import /units)

(main
  (print (as 1mps UnitString))
)

(type-line + Mps (alias mps) (is F64))
(type-line + Kph (alias kph) (is F64))
(unit + 1mps (/ 1meters 1kph))
(unit + 1kph (/ 1km 1mps))
```

```output
Exception: Mps is declared from units that are declared from Mps; declare one of them from units of other families
<no file:10>
    (unit + 1mps (/ 1meters 1kph))
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An equality with no single unit on either side is rejected.

```brix compile-error
(import /time/duration)
(import /units)

(main
  (print 1)
)

(unit + 2 (/ 1meters 1s))
```

```output
Exception: An equality declares one unit, alone on one side, as an amount of others; write one side as a single unit, as in "(unit + 1mps (/ 1meters 1s))"
<no file:9>
    (unit + 2 (/ 1meters 1s))
    ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A product or a quotient that two units are declared as alike is rejected.

```brix compile-error
(import /time/duration)
(import /units)

(main
  (print (as (/ 100km 2hours) UnitString))
)

(type-line + Kph (alias kph) (is F64))
(type-line + Kmh (alias kmh) (is F64))
(unit + 1kph (/ 1km 1hours))
(unit + 1kmh (/ 1km 1hours))
```

```output
Exception: Kph and Kmh are both declared as Kilometers per Hours; keep one of them where both can be named
<no file:6>
      (print (as (/ 100km 2hours) UnitString))
                 ^^^^^^^^^^^^^^^^
```

### An equality that disagrees with units an imported module declares as a quotient is rejected in the module that imports it.

```brix compile-error
(import ./lib/speeds)

(main
  (print 1)
)

(unit + 1mps 3kph)
```

```brix file=lib/speeds/speeds.bx
(import /time/duration)
(import /units)

(type-line + Kph (alias kph) (is F64))
(type-line + Mps (alias mps) (is F64))
(unit + 1kph (/ 1km 1hours))
(unit + 1mps (/ 1meters 1s))
```

```output
Exception: Equalities between Mps and Kph disagree: one makes 1 Mps 3.6 Kph, another 3 Kph; keep one of them
<no file:8>
    (unit + 1mps 3kph)
    ^^^^^^^^^^^^^^^^^^
```

### An assigning multiply by a unit of the place's family is rejected where no unit is declared as their product.

```brix compile-error
(import /time/duration)

(main
  (let wait Seconds 10s)
  (*= wait 2s)
)
```

```output
Exception: No unit is declared as Seconds times Seconds; declare one with an equality, or make one side a plain number
<no file:6>
      (*= wait 2s)
               ^^
```

### Raising a unit to a whole number gives the unit an equality declares as that power of it, converted to its scale, and a square root gives the unit whose square it is.

```brix
(import /math)
(import /time/duration)
(import /units)

(main
  (print (as (%% 3meters 2) UnitString))
  (print (as (%% 3km 2) UnitString))
  (print (as (%% 500ms -1) UnitString))
  (print (as (%% 2meters 1) UnitString))
  (print (as (%% 2meters 0) UnitString))
  (print (as (call sqrt 9sqm) UnitString))
  (print (as (call sqrt 4000000sqm) UnitString))
)

(type-line + SquareMeters (alias sqm) (is F64))
(type-line + Hertz (alias hz) (is F64))
(unit + 1sqm (* 1meters 1meters))
(unit + 1hz (/ 1 1s))
```

```output
9sqm
9000000sqm
2hz
2meters
1f
3meters
2000meters
```

### A square root a value is piped into gives the unit whose square it is, as one written as an argument does.

```brix
(import /math)
(import /units)

(main
  (print (as (|> 9sqm (call sqrt)) UnitString))
  (print (as (|> 4000000sqm (call sqrt)) UnitString))
  (print (as (|> 25% (call sqrt)) UnitString))
)

(type-line + SquareMeters (alias sqm) (is F64))
(unit + 1sqm (* 1meters 1meters))
```

```output
3meters
2000meters
50%
```

### A pure number raised to any power or rooted keeps its unit, worked out by its value as a number.

```brix
(import /math)
(import /units)

(main
  (let power 2)
  (let share 50%)
  (%%= share 2)
  (print (as (%% 50% 2) UnitString))
  (print (as (%% 50% power) UnitString))
  (print (as (call sqrt 25%) UnitString))
  (print (as share UnitString))
)
```

```output
25%
25%
50%
25%
```

### Raising a unit to a whole number is rejected where no unit is declared as that power of it.

```brix compile-error
(import /units)

(main
  (print (as (%% 3meters 3) UnitString))
)
```

```output
Exception: No unit is declared as Meters times Meters times Meters; declare one with an equality, or raise a plain number
<no file:5>
      (print (as (%% 3meters 3) UnitString))
                 ^^^^^^^^^^^^^^
```

### A unit is raised only to a whole number written in place, as the number decides the unit.

```brix compile-error
(import /units)

(main
  (let power 2)
  (print (as (%% 3meters power) UnitString))
)
```

```output
Exception: A unit is raised only to a whole number written in place, as in "(%% 3meters 2)", which decides the unit it gives; make it a plain number to raise it to another power
<no file:6>
      (print (as (%% 3meters power) UnitString))
                             ^^^^^
```

### A unit is raised only to a number with no unit.

```brix compile-error
(import /time/duration)
(import /units)

(main
  (print (as (%% 3meters 2s) UnitString))
)
```

```output
Exception: A unit is raised to a plain number, which has no unit
<no file:6>
      (print (as (%% 3meters 2s) UnitString))
                             ^^
```

### The square root of a unit is rejected where no unit times itself is it.

```brix compile-error
(import /math)
(import /time/duration)

(main
  (print (as (call sqrt 9s) UnitString))
)
```

```output
Exception: No unit times itself is Seconds, so its square root has no unit; take the root of a plain number
<no file:6>
      (print (as (call sqrt 9s) UnitString))
                 ^^^^^^^^^^^^^^
```

### An assigning `%%` rejects a power that its place's unit cannot hold.

```brix compile-error
(import /units)

(main
  (let length 3meters)
  (%%= length 2)
)

(type-line + SquareMeters (alias sqm) (is F64))
(unit + 1sqm (* 1meters 1meters))
```

```output
Exception: Expected Meters here, but this is SquareMeters, and there is no conversion between them
<no file:6>
      (%%= length 2)
      ^^^^^^^^^^^^^^
```

### A number type's own arithmetic operators answer it in place of the builtin ones, a number written with no unit converts to the operator's parameter, and an operator it does not declare stays builtin.

```brix
(main
  (print (as (+ 23hr 2hrs) UnitString))
  (print (as (+ 23hr 2) UnitString))
  (print (as (- 1hr 3hr) UnitString))
  (print (as (- 1hr 3hrs) UnitString))
  (print (as (* 2 3hr) UnitString))
)

(type-line + Hrs (alias hrs) (is F64))
(type + Hr (alias hr) (is UInt)
  (fn-expr + (infix this + (param amount Hrs)) Hr
    (as (call wrap (+ (as this F64) (as amount F64))) Hr)
  )
  (fn-expr + (infix this - (param amount Hrs)) Hr
    (as (call wrap (- (as this F64) (as amount F64))) Hr)
  )
  (fn-expr + (infix this - (param other Hr)) Hrs
    (as (- (as this F64) (as other F64)) Hrs)
  )
)

(fn-expr _ wrap F64
  (param hours F64)
  (% (+ (% hours 24.0) 24.0) 24.0)
)
```

```output
1hr
1hr
-2hrs
22hr
6hr
```

### A number type's own `+` or `*` answers with its operands the other way around when the number type is on the right alone.

```brix
(main
  (print (as (+ 2 23hr) UnitString))
  (print (as (+ 2hrs 23hr) UnitString))
)

(type-line + Hrs (alias hrs) (is F64))
(type + Hr (alias hr) (is UInt)
  (fn-expr + (infix this + (param amount Hrs)) Hr
    (as (% (+ (as this F64) (as amount F64)) 24.0) Hr)
  )
)
```

```output
1hr
1hr
```

### An assigning operator answers by the number type's own operator.

```brix
(main
  (let h 22hr)
  (+= h 3hrs)
  (print (as h UnitString))
  (-= h 2hrs)
  (print (as h UnitString))
)

(type-line + Hrs (alias hrs) (is F64))
(type + Hr (alias hr) (is UInt)
  (fn-expr + (infix this + (param amount Hrs)) Hr
    (as (call wrap (+ (as this F64) (as amount F64))) Hr)
  )
  (fn-expr + (infix this - (param amount Hrs)) Hr
    (as (call wrap (- (as this F64) (as amount F64))) Hr)
  )
)

(fn-expr _ wrap F64
  (param hours F64)
  (% (+ (% hours 24.0) 24.0) 24.0)
)
```

```output
1hr
23hr
```

### A number type that declares an operator rejects operands its own operator does not take.

```brix compile-error
(main
  (print (as (+ 23hr 2hr) UnitString))
)

(type-line + Hrs (alias hrs) (is F64))
(type + Hr (alias hr) (is UInt)
  (fn-expr + (infix this + (param amount Hrs)) Hr
    (as (% (+ (as this F64) (as amount F64)) 24.0) Hr)
  )
)
```

```output
Exception: Operator "+" is not defined for Hr and Hr
<no file:3>
      (print (as (+ 23hr 2hr) UnitString))
                 ^^^^^^^^^^^^
```

### A number type's own `-` does not answer with the number type on the right alone.

```brix compile-error
(main
  (print (as (- 5 3hr) UnitString))
)

(type-line + Hrs (alias hrs) (is F64))
(type + Hr (alias hr) (is UInt)
  (fn-expr + (infix this - (param other Hr)) Hrs
    (as (- (as this F64) (as other F64)) Hrs)
  )
)
```

```output
Exception: Operator "-" is not defined for I64 and Hr
<no file:3>
      (print (as (- 5 3hr) UnitString))
                 ^^^^^^^^^
```

### An assigning operator rejects operands the place's number type's own operator does not take.

```brix compile-error
(main
  (let h 22hr)
  (+= h 2hr)
  (print (as h UnitString))
)

(type-line + Hrs (alias hrs) (is F64))
(type + Hr (alias hr) (is UInt)
  (fn-expr + (infix this + (param amount Hrs)) Hr
    (as (% (+ (as this F64) (as amount F64)) 24.0) Hr)
  )
)
```

```output
Exception: Operator "+=" is not defined for Hr and Hr
<no file:4>
      (+= h 2hr)
      ^^^^^^^^^^
```

### A number type's declared conversion is used in place of a cast of its number, with `as` and on its own.

```brix
(import /units)

(main
  (print (as (as 100cel Fahr) UnitString))
  (print (call show 100cel))
  (print (as (as 0.75%% Hr) UnitString))
)

(type + Cel (alias cel) (is F64)
  (fn-to-expr + Fahr
    (as (+ (/ (* (as this F64) 9.0) 5.0) 32.0) Fahr)
  )
)
(type-line + Fahr (alias fahr) (is F64))
(type + Hr (alias hr) (is UInt)
  (fn-from-expr + (: ratio Ratio)
    (as (* (as ratio F64) 24.0) Hr)
  )
)

(fn-expr _ show String
  (param f Fahr)
  (as f UnitString)
)
```

```output
212fahr
212fahr
18hr
```
