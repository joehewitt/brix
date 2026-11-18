# Inline interfaces

```yumi
/core
-- interface-caller(InlineInterfacePair(20, 22))
-- interface-join-caller(InlineInterfacePair(20, 22), InlineInterfacePair(3, 4), true)
-- interface-join-caller(InlineInterfacePair(20, 22), InlineInterfacePair(3, 4), false)
-- interface-mixed-caller(InlineInterfacePair(20, 22), InlineInterfaceSingle(9), true)
-- interface-mixed-caller(InlineInterfacePair(20, 22), InlineInterfaceSingle(9), false)
-- inline-type-name(InlineInterfacePair(20, 22))
-- inline-concrete-name(InlineInterfacePair(20, 22))
-- inline-concrete-name(choose-interface(InlineInterfacePair(20, 22), InlineInterfaceSingle(9), true))
-- inline-concrete-name(choose-interface(InlineInterfacePair(20, 22), InlineInterfaceSingle(9), false))
-- inline-inlined-name(InlineInterfacePair(20, 22))
-- inline-inlined-name(choose-interface(InlineInterfacePair(20, 22), InlineInterfaceSingle(9), true))
-- inline-inlined-name(choose-interface(InlineInterfacePair(20, 22), InlineInterfaceSingle(9), false))
-- ordinary-inlined-name(InlineInterfacePair(20, 22))
-- unknown-inlined-name(InlineInterfacePair(20, 22))
-- (type-of 42).inlined-name
-- partly-known-inlined-name(InlineInterfacePair(20, 22), InlineInterfaceSingle(9), true)
-- partly-known-inlined-name(InlineInterfacePair(20, 22), InlineInterfaceSingle(9), false)

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Tuple
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfaceSingle is Tuple
    is InlineMeasure

+ a Int

────────────────────
+ total Int @inline

a

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total()

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair

interface-total(value)

━━━━━━━━━━━━━━━━━━━━
+ do choose-interface InlineMeasure @inline
    - first InlineMeasure
    - second InlineMeasure
    - choose Bool

. result = first
if choose
    result = second
= result

━━━━━━━━━━━━━━━━━━━━
+ interface-join-caller Int
    - first InlineInterfacePair
    - second InlineInterfacePair
    - choose Bool

interface-total(choose-interface(first, second, choose))

━━━━━━━━━━━━━━━━━━━━
+ inline-type-name String @inline
    - value InlineMeasure

(type-of value).name

━━━━━━━━━━━━━━━━━━━━
+ inline-concrete-name String @inline
    - value InlineMeasure

(type-of value).concrete-name

━━━━━━━━━━━━━━━━━━━━
+ inline-inlined-name String @inline
    - value InlineMeasure

(type-of value).inlined-name

━━━━━━━━━━━━━━━━━━━━
+ unknown-inlined-name String
    - value InlineMeasure

inline-inlined-name(value)

━━━━━━━━━━━━━━━━━━━━
+ partly-known-inlined-name String
    - first InlineInterfacePair
    - second InlineMeasure
    - choose Bool

inline-inlined-name(choose-interface(first, second, choose))

━━━━━━━━━━━━━━━━━━━━
+ ordinary-inlined-name String
    - value InlineMeasure

(type-of value).inlined-name

━━━━━━━━━━━━━━━━━━━━
+ interface-mixed-caller Int
    - first InlineInterfacePair
    - second InlineInterfaceSingle
    - choose Bool

interface-total(choose-interface(first, second, choose))
```

```output
42
7
42
9
42
InlineMeasure
InlineInterfacePair
InlineInterfaceSingle
InlineInterfacePair
InlineInterfacePair
InlineInterfaceSingle
InlineInterfacePair
InlineMeasure
InlineMeasure
I64
InlineMeasure
InlineInterfacePair
```

```yumi
/core
-- interface-caller(InlineInterfacePair(20, 22))
-- inline-inlined-name(InlineInterfacePair(20, 22))

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Struct
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total()

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair

interface-total(value)

━━━━━━━━━━━━━━━━━━━━
+ inline-inlined-name String @inline
    - value InlineMeasure

(type-of value).inlined-name
```

```output
42
InlineInterfacePair
```

```yumi
/core
. pair = InlineInterfacePair(20, 22)
-- interface-caller(pair)
-- pair.a
-- interface-total(pair)
-- pair.a

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Struct
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ total Int @inline

do
    a += 1
a + b

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total() + value.total()

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair

interface-total(value)
```

```output
86
20
86
20
```

```yumi
/core
-- saved-info-name(InlineInterfacePair(20, 22))
-- returned-info-name(InlineInterfacePair(20, 22))

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Tuple
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ inline-saved-name String @inline
    - value InlineMeasure

info.inlined-name

.. info = type-of value

━━━━━━━━━━━━━━━━━━━━
+ inline-type-info @inline
    - value InlineMeasure

type-of value

━━━━━━━━━━━━━━━━━━━━
+ saved-info-name String
    - value InlineInterfacePair

inline-saved-name(value)

━━━━━━━━━━━━━━━━━━━━
+ returned-info-name String
    - value InlineInterfacePair

(inline-type-info(value)).inlined-name
```

```output
InlineInterfacePair
InlineInterfacePair
```

```yumi crash
/core
-- interface-caller(InlineInterfacePair(20, 22))

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Tuple
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ total Int @inline

a + b

────────────────────
+ only-concrete Int

a

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.only-concrete()

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair

interface-total(value)
```

```output
Exception: InlineMeasure has no method "only-concrete"
<no file:32>
    value.only-concrete()
    ^^^^^^^^^^^^^^^^^^^^^
```

```yumi
/core
-- mixed-use-caller(InlineInterfacePair(20, 22), false)
-- mixed-use-caller(InlineInterfacePair(20, 22), true)

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Tuple
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ mixed-use-caller Int
    - value InlineInterfacePair
    - escape Bool

mixed-interface-use(value, escape)

━━━━━━━━━━━━━━━━━━━━
+ mixed-interface-use Int @inline
    - value InlineMeasure
    - escape Bool

|? escape
    opaque-interface(value) + opaque-interface(value)
??
    value.total()

━━━━━━━━━━━━━━━━━━━━
+ opaque-interface Int
    - value InlineMeasure

value.total()
```

```output
42
84
```

```yumi
/core
-- mixed-use-caller(InlineInterfacePair(20, 22), false)
-- mixed-use-caller(InlineInterfacePair(20, 22), true)

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Struct
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ mixed-use-caller Int
    - value InlineInterfacePair
    - escape Bool

mixed-interface-use(value, escape)

━━━━━━━━━━━━━━━━━━━━
+ mixed-interface-use Int @inline
    - value InlineMeasure
    - escape Bool

|? escape
    opaque-interface(value) + opaque-interface(value)
??
    value.total()

━━━━━━━━━━━━━━━━━━━━
+ opaque-interface Int
    - value InlineMeasure

value.total()
```

```output
42
84
```

```yumi
/core
-- interface-mixed-caller(InlineInterfacePair(20, 22), InlineInterfaceSingle(9), true)
-- interface-mixed-caller(InlineInterfacePair(20, 22), InlineInterfaceSingle(9), false)

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Tuple
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfaceSingle is Tuple
    is InlineMeasure

+ a Int

────────────────────
+ total Int @inline

a

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total()

━━━━━━━━━━━━━━━━━━━━
+ do choose-interface InlineMeasure @inline
    - first InlineMeasure
    - second InlineMeasure
    - choose Bool

. result = first
if choose
    result = second
= result

━━━━━━━━━━━━━━━━━━━━
+ interface-mixed-caller Int
    - first InlineInterfacePair
    - second InlineMeasure
    - choose Bool

interface-total(choose-interface(first, second, choose))
```

```output
9
42
```

```yumi
/core
-- interface-caller(42 as InlineInterfacePair)
-- interface-join-caller(42 as InlineInterfacePair, 7 as InlineInterfacePair, true)
-- interface-join-caller(42 as InlineInterfacePair, 7 as InlineInterfacePair, false)

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is I32
    is InlineMeasure

────────────────────
+ total Int @inline

this as Int

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total()

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair

interface-total(value)

━━━━━━━━━━━━━━━━━━━━
+ do choose-interface InlineMeasure @inline
    - first InlineMeasure
    - second InlineMeasure
    - choose Bool

. result = first
if choose
    result = second
= result

━━━━━━━━━━━━━━━━━━━━
+ interface-join-caller Int
    - first InlineInterfacePair
    - second InlineInterfacePair
    - choose Bool

interface-total(choose-interface(first, second, choose))
```

```output
42
7
42
```

```yumi
/core
-- interface-caller(InlineInterfacePair(20, 22))
-- interface-join-caller(InlineInterfacePair(20, 22), InlineInterfacePair(3, 4), true)
-- interface-join-caller(InlineInterfacePair(20, 22), InlineInterfacePair(3, 4), false)

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ This
    - a
    - b

────────────────────
+ total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total()

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair

interface-total(value)

━━━━━━━━━━━━━━━━━━━━
+ do choose-interface InlineMeasure @inline
    - first InlineMeasure
    - second InlineMeasure
    - choose Bool

. result = first
if choose
    result = second
= result

━━━━━━━━━━━━━━━━━━━━
+ interface-join-caller Int
    - first InlineInterfacePair
    - second InlineInterfacePair
    - choose Bool

interface-total(choose-interface(first, second, choose))
```

```output
42
7
42
```

```yumi
/core
-- interface-caller(InlineInterfacePair'Int(20, 22))
-- interface-join-caller(InlineInterfacePair'Int(20, 22), InlineInterfacePair'Int(3, 4), true)
-- interface-join-caller(InlineInterfacePair'Int(20, 22), InlineInterfacePair'Int(3, 4), false)

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair'T is Tuple
    is InlineMeasure

+ a T
+ b T

────────────────────
+ total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total()

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair'Int

interface-total(value)

━━━━━━━━━━━━━━━━━━━━
+ do choose-interface InlineMeasure @inline
    - first InlineMeasure
    - second InlineMeasure
    - choose Bool

. result = first
if choose
    result = second
= result

━━━━━━━━━━━━━━━━━━━━
+ interface-join-caller Int
    - first InlineInterfacePair'Int
    - second InlineInterfacePair'Int
    - choose Bool

interface-total(choose-interface(first, second, choose))
```

```output
42
7
42
```

```yumi
/core
-- interface-caller(InlineInterfacePair(20, 22))
-- interface-join-caller(InlineInterfacePair(20, 22), InlineInterfacePair(3, 4), true)
-- interface-join-caller(InlineInterfacePair(20, 22), InlineInterfacePair(3, 4), false)

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Tuple
    is InlineMeasure

+ a Int
+ b Int
+ text String = "rooted"

────────────────────
+ total Int @inline

a + b + text.size - 6

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total()

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair

interface-total(value)

━━━━━━━━━━━━━━━━━━━━
+ do choose-interface InlineMeasure @inline
    - first InlineMeasure
    - second InlineMeasure
    - choose Bool

. result = first
if choose
    result = second
= result

━━━━━━━━━━━━━━━━━━━━
+ interface-join-caller Int
    - first InlineInterfacePair
    - second InlineInterfacePair
    - choose Bool

interface-total(choose-interface(first, second, choose))
```

```output
42
7
42
```

```yumi
/core
-- interface-caller(InlineInterfacePair(20, 22))
-- interface-join-caller(InlineInterfacePair(20, 22), InlineInterfacePair(3, 4), true)
-- interface-join-caller(InlineInterfacePair(20, 22), InlineInterfacePair(3, 4), false)

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Tuple
    is InlineMeasure

+ a Int
+ b Int
+ nested InlineNested = InlineNested(1)

────────────────────
+ total Int @inline

a + b + nested.last + nested.field79 - 80

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total()

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair

interface-total(value)

━━━━━━━━━━━━━━━━━━━━
+ do choose-interface InlineMeasure @inline
    - first InlineMeasure
    - second InlineMeasure
    - choose Bool

. result = first
if choose
    result = second
= result

━━━━━━━━━━━━━━━━━━━━
+ interface-join-caller Int
    - first InlineInterfacePair
    - second InlineInterfacePair
    - choose Bool

interface-total(choose-interface(first, second, choose))

━━━━━━━━━━━━━━━━━━━━
+ InlineNested is Tuple

+ last Int
+ field0 Int = 0
+ field1 Int = 1
+ field2 Int = 2
+ field3 Int = 3
+ field4 Int = 4
+ field5 Int = 5
+ field6 Int = 6
+ field7 Int = 7
+ field8 Int = 8
+ field9 Int = 9
+ field10 Int = 10
+ field11 Int = 11
+ field12 Int = 12
+ field13 Int = 13
+ field14 Int = 14
+ field15 Int = 15
+ field16 Int = 16
+ field17 Int = 17
+ field18 Int = 18
+ field19 Int = 19
+ field20 Int = 20
+ field21 Int = 21
+ field22 Int = 22
+ field23 Int = 23
+ field24 Int = 24
+ field25 Int = 25
+ field26 Int = 26
+ field27 Int = 27
+ field28 Int = 28
+ field29 Int = 29
+ field30 Int = 30
+ field31 Int = 31
+ field32 Int = 32
+ field33 Int = 33
+ field34 Int = 34
+ field35 Int = 35
+ field36 Int = 36
+ field37 Int = 37
+ field38 Int = 38
+ field39 Int = 39
+ field40 Int = 40
+ field41 Int = 41
+ field42 Int = 42
+ field43 Int = 43
+ field44 Int = 44
+ field45 Int = 45
+ field46 Int = 46
+ field47 Int = 47
+ field48 Int = 48
+ field49 Int = 49
+ field50 Int = 50
+ field51 Int = 51
+ field52 Int = 52
+ field53 Int = 53
+ field54 Int = 54
+ field55 Int = 55
+ field56 Int = 56
+ field57 Int = 57
+ field58 Int = 58
+ field59 Int = 59
+ field60 Int = 60
+ field61 Int = 61
+ field62 Int = 62
+ field63 Int = 63
+ field64 Int = 64
+ field65 Int = 65
+ field66 Int = 66
+ field67 Int = 67
+ field68 Int = 68
+ field69 Int = 69
+ field70 Int = 70
+ field71 Int = 71
+ field72 Int = 72
+ field73 Int = 73
+ field74 Int = 74
+ field75 Int = 75
+ field76 Int = 76
+ field77 Int = 77
+ field78 Int = 78
+ field79 Int = 79
```

```output
42
7
42
```

```yumi
/core
-- inline-is(InlineInterfacePair(20, 22))
-- inline-is(InlineInterfaceSingle(9))
-- interface-caller(InlineInterfacePair(20, 22))

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Tuple
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfaceSingle is Tuple
    is InlineMeasure

+ a Int

────────────────────
+ total Int @inline

a

━━━━━━━━━━━━━━━━━━━━
+ inline-is Bool @inline
    - value InlineMeasure

value is InlineInterfacePair

━━━━━━━━━━━━━━━━━━━━
+ inline-downcast Int @inline
    - value InlineMeasure

point.a + point.b

.. point = value as InlineInterfacePair

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair

inline-downcast(value)
```

```output
true
false
42
```

```yumi
/core
-- interface-caller(InlineInterfacePair(InlineInner(20, 22)))
-- interface-join-caller(InlineInterfacePair(InlineInner(20, 22)), InlineInterfacePair(InlineInner(3, 4)), true)
-- interface-join-caller(InlineInterfacePair(InlineInner(20, 22)), InlineInterfacePair(InlineInner(3, 4)), false)

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Tuple

as InlineMeasure for
+ inner InlineInner

━━━━━━━━━━━━━━━━━━━━
+ InlineInner is Tuple
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total()

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair

interface-total(value)

━━━━━━━━━━━━━━━━━━━━
+ do choose-interface InlineMeasure @inline
    - first InlineMeasure
    - second InlineMeasure
    - choose Bool

. result = first
if choose
    result = second
= result

━━━━━━━━━━━━━━━━━━━━
+ interface-join-caller Int
    - first InlineInterfacePair
    - second InlineInterfacePair
    - choose Bool

interface-total(choose-interface(first, second, choose))
```

```output
42
7
42
```

```yumi
/core
-- interface-caller(InlineInterfacePair(20, 22))

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineChild is Interface
    is InlineMeasure

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Tuple
    is InlineChild

+ a Int
+ b Int

────────────────────
+ total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total()

━━━━━━━━━━━━━━━━━━━━
+ child-total Int @inline
    - value InlineChild

interface-total(value as InlineMeasure)

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair

child-total(value)
```

```output
42
```

```yumi
/core
-- interface-join-caller(InlineInterfacePair(20, 22), InlineInterfacePair(3, 4), true)
-- interface-join-caller(InlineInterfacePair(20, 22), InlineInterfacePair(3, 4), false)

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Tuple
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total()

━━━━━━━━━━━━━━━━━━━━
+ do choose-interface InlineMeasure @inline
    - first InlineMeasure
    - second InlineMeasure
    - choose Bool

. result = first
. count = 0
while count < 2
    if choose
        result = second
    count += 1
= result

━━━━━━━━━━━━━━━━━━━━
+ interface-join-caller Int
    - first InlineInterfacePair
    - second InlineInterfacePair
    - choose Bool

interface-total(choose-interface(first, second, choose))
```

```output
7
42
```

```yumi
/core
/BxGC/BxGC.h

-- managed-caller(ManagedPayload(make-text(42)))

━━━━━━━━━━━━━━━━━━━━
+ Measured is Interface

────────────────────
+ size Int

━━━━━━━━━━━━━━━━━━━━
+ ManagedPayload is Tuple
    is Measured

+ text String

────────────────────
+ do size Int @inline

C:brixGCCollect()
= text.size

━━━━━━━━━━━━━━━━━━━━
+ make-text String
    - value Int

"number $value"

━━━━━━━━━━━━━━━━━━━━
+ measured-size Int @inline
    - value Measured

value.size()

━━━━━━━━━━━━━━━━━━━━
+ managed-caller Int
    - value ManagedPayload

measured-size(value)
```

```output
9
```

```yumi
/core
-- identity-caller(InlineInterfacePair(20, 22))
-- interface-caller(InlineInterfacePair(20, 22))

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Tuple
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total()

━━━━━━━━━━━━━━━━━━━━
+ InterfaceHolder is Tuple

+ value InlineMeasure

━━━━━━━━━━━━━━━━━━━━
+ same-interface Bool @inline
    - first InlineMeasure
    - second InlineMeasure

first as Pointer == second as Pointer

━━━━━━━━━━━━━━━━━━━━
+ identity-caller Bool
    - value InlineInterfacePair

same-interface(saved, saved) && !same-interface(value, value)

.. saved = value as InlineMeasure

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair

interface-total(InterfaceHolder(value).value)
```

```output
true
42
```

```yumi
/core
-- identity-caller(InlineInterfacePair(20, 22))
-- interface-caller(InlineInterfacePair(20, 22))

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Tuple
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total()

━━━━━━━━━━━━━━━━━━━━
+ InterfaceHolder is Struct

+ value InlineMeasure

━━━━━━━━━━━━━━━━━━━━
+ same-interface Bool @inline
    - first InlineMeasure
    - second InlineMeasure

first as Pointer == second as Pointer

━━━━━━━━━━━━━━━━━━━━
+ identity-caller Bool
    - value InlineInterfacePair

same-interface(saved, saved) && !same-interface(value, value)

.. saved = value as InlineMeasure

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair

interface-total(InterfaceHolder(value).value)
```

```output
true
42
```

```yumi
/core
-- interface-caller(InlineInterfacePair(20, 22))
-- interface-join-caller(InlineInterfacePair(20, 22), InlineInterfacePair(3, 4), true)
-- interface-join-caller(InlineInterfacePair(20, 22), InlineInterfacePair(3, 4), false)

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ get total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Tuple
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ get total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair

interface-total(value)

━━━━━━━━━━━━━━━━━━━━
+ do choose-interface InlineMeasure @inline
    - first InlineMeasure
    - second InlineMeasure
    - choose Bool

. result = first
if choose
    result = second
= result

━━━━━━━━━━━━━━━━━━━━
+ interface-join-caller Int
    - first InlineInterfacePair
    - second InlineInterfacePair
    - choose Bool

interface-total(choose-interface(first, second, choose))
```

```output
42
7
42
```

```yumi
/core
-- interface-caller(InlineInterfacePair(20, 22))

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Struct
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ total Int @inline

do
    a += 1
a + b

━━━━━━━━━━━━━━━━━━━━
+ interface-total Int @inline
    - value InlineMeasure

value.total() + opaque-total(value) + value.total()

━━━━━━━━━━━━━━━━━━━━
+ interface-caller Int
    - value InlineInterfacePair

interface-total(value)

━━━━━━━━━━━━━━━━━━━━
+ do opaque-total Int
    - value InlineMeasure

if value is InlineInterfacePair
    . point = value as InlineInterfacePair
    point.a += 10
= value.total()
```

```output
129
```

```yumi
/core
-- catch checked-total(InlineInterfacePair(20, 22), true)
-- catch checked-total(InlineInterfacePair(20, 22), false)

━━━━━━━━━━━━━━━━━━━━
+ InlineMeasure is Interface

────────────────────
+ total Int

━━━━━━━━━━━━━━━━━━━━
+ InlineInterfacePair is Tuple
    is InlineMeasure

+ a Int
+ b Int

────────────────────
+ total Int @inline

a + b

━━━━━━━━━━━━━━━━━━━━
+ InlineFailure is Error

━━━━━━━━━━━━━━━━━━━━
+ do checked-total Int ?! InlineFailure @inline
    - value InlineMeasure
    - succeeds Bool

do finally
    -- "cleanup ${value.total()}"
if succeeds
    = value.total()
?! InlineFailure("failed")
```

```output
cleanup 42
42
cleanup 42
failed
```

```yumi
/core
-- forwarded(0 .. < 0)
-- ordinary(0 .. < 0)
-- forwarded(2 .. 2)
-- ordinary(2 .. 2)
-- forwarded(2 .. < 7)
-- ordinary(2 .. < 7)
-- forwarded(7 .. < 2)
-- ordinary(7 .. < 2)
-- forwarded(2 .. 7 by 2)
-- ordinary(2 .. 7 by 2)
-- forwarded(7 .. 2 by 2)
-- ordinary(7 .. 2 by 2)
-- forwarded(2 .. < 7 by 2)
-- ordinary(2 .. < 7 by 2)
-- forwarded(7 .. < 2 by 2)
-- ordinary(7 .. < 2 by 2)
-- forwarded(-4 .. 3 by 2)
-- ordinary(-4 .. 3 by 2)
-- forwarded(2u64 .. 7u64 by 2u64)
-- ordinary(2u64 .. 7u64 by 2u64)
-- forwarded(7u64 .. < 2u64 by 2u64)
-- ordinary(7u64 .. < 2u64 by 2u64)
-- forwarded(2i64 .. 7i64 by 2i64)
-- ordinary(2i64 .. 7i64 by 2i64)
-- forwarded(0.0 .. < 1.0 by 0.25)
-- ordinary(0.0 .. < 1.0 by 0.25)
-- forwarded(0.0 .. < 1.0 by 0.1)
-- ordinary(0.0 .. < 1.0 by 0.1)
-- forwarded(0f32 .. < 1f32 by 0.1f32)
-- ordinary(0f32 .. < 1f32 by 0.1f32)
-- forwarded(2147483646 .. 2147483647)
-- ordinary(2147483646 .. 2147483647)
-- forwarded(-2147483647 .. -2147483648)
-- ordinary(-2147483647 .. -2147483648)
-- stop-early(1 .. 10)

━━━━━━━━━━━━━━━━━━━━
+ do ordinary'T String
    - source Indexable'T

. text = "["
for item in source
    text += "$item,"
= text + "]"

━━━━━━━━━━━━━━━━━━━━
+ do specialized'T String @inline
    - source Indexable'T

. text = "["
for item in source
    text += "$item,"
= text + "]"

━━━━━━━━━━━━━━━━━━━━
+ forwarded'T String @inline
    - source Indexable'T

specialized(source)

━━━━━━━━━━━━━━━━━━━━
+ do stop-early Int @inline
    - source Indexable'Int

for item in source
    if item == 3
        = item
= 0
```

```output
[]
[]
[2,]
[2,]
[2,3,4,5,6,]
[2,3,4,5,6,]
[7,6,5,4,3,]
[7,6,5,4,3,]
[2,4,6,]
[2,4,6,]
[7,5,3,]
[7,5,3,]
[2,4,6,]
[2,4,6,]
[7,5,3,]
[7,5,3,]
[-4,-2,0,2,]
[-4,-2,0,2,]
[2,4,6,]
[2,4,6,]
[7,5,3,]
[7,5,3,]
[2,4,6,]
[2,4,6,]
[0,0.25,0.5,0.75,]
[0,0.25,0.5,0.75,]
[0,0.1,0.2,0.30000000000000004,0.4,0.5,0.6000000000000001,0.7000000000000001,0.8,0.9,]
[0,0.1,0.2,0.30000000000000004,0.4,0.5,0.6000000000000001,0.7000000000000001,0.8,0.9,]
[0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.90000004,]
[0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.90000004,]
[2147483646,2147483647,]
[2147483646,2147483647,]
[-2147483647,-2147483648,]
[-2147483647,-2147483648,]
3
```

```yumi crash
/core
-- hidden(0 .. 3)

━━━━━━━━━━━━━━━━━━━━
+ hidden @inline
    - source Indexable'Int

source.start
```

```output
Exception: Indexable'I64 has no property "start"
<no file:9>
    source.start
    ^^^^^^^^^^^^
```
