# Inline interfaces

```brix
(import /core)

(main
  (print (call interface-caller (call InlineInterfacePair 20 22)))
  (print (call interface-join-caller (call InlineInterfacePair 20 22) (call InlineInterfacePair 3 4) true))
  (print (call interface-join-caller (call InlineInterfacePair 20 22) (call InlineInterfacePair 3 4) false))
  (print (call interface-mixed-caller (call InlineInterfacePair 20 22) (call InlineInterfaceSingle 9) true))
  (print (call interface-mixed-caller (call InlineInterfacePair 20 22) (call InlineInterfaceSingle 9) false))
  (print (call inline-type-name (call InlineInterfacePair 20 22)))
  (print (call inline-concrete-name (call InlineInterfacePair 20 22)))
  (print (call inline-concrete-name (call choose-interface (call InlineInterfacePair 20 22) (call InlineInterfaceSingle 9) true)))
  (print (call inline-concrete-name (call choose-interface (call InlineInterfacePair 20 22) (call InlineInterfaceSingle 9) false)))
  (print (call inline-inlined-name (call InlineInterfacePair 20 22)))
  (print (call inline-inlined-name (call choose-interface (call InlineInterfacePair 20 22) (call InlineInterfaceSingle 9) true)))
  (print (call inline-inlined-name (call choose-interface (call InlineInterfacePair 20 22) (call InlineInterfaceSingle 9) false)))
  (print (call ordinary-inlined-name (call InlineInterfacePair 20 22)))
  (print (call unknown-inlined-name (call InlineInterfacePair 20 22)))
  (print (. (type-of 42) inlined-name))
  (print (call partly-known-inlined-name (call InlineInterfacePair 20 22) (call InlineInterfaceSingle 9) true))
  (print (call partly-known-inlined-name (call InlineInterfacePair 20 22) (call InlineInterfaceSingle 9) false))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (+ a b)
  )
)

(type + InlineInterfaceSingle (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (fn-expr + total Int @inline
    a
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (call (. value total))
)

(fn-expr + interface-caller Int
  (param value InlineInterfacePair)
  (call interface-total value)
)

(fn-do + choose-interface InlineMeasure @inline
  (param first InlineMeasure)
  (param second InlineMeasure)
  (param choose Bool)
  (let result first)
  (if choose
    (= result second)
  )
  (return result)
)

(fn-expr + interface-join-caller Int
  (param first InlineInterfacePair)
  (param second InlineInterfacePair)
  (param choose Bool)
  (call interface-total (call choose-interface first second choose))
)

(fn-expr + inline-type-name String @inline
  (param value InlineMeasure)
  (. (type-of value) name)
)

(fn-expr + inline-concrete-name String @inline
  (param value InlineMeasure)
  (. (type-of value) concrete-name)
)

(fn-expr + inline-inlined-name String @inline
  (param value InlineMeasure)
  (. (type-of value) inlined-name)
)

(fn-expr + unknown-inlined-name String
  (param value InlineMeasure)
  (call inline-inlined-name value)
)

(fn-expr + partly-known-inlined-name String
  (param first InlineInterfacePair)
  (param second InlineMeasure)
  (param choose Bool)
  (call inline-inlined-name (call choose-interface first second choose))
)

(fn-expr + ordinary-inlined-name String
  (param value InlineMeasure)
  (. (type-of value) inlined-name)
)

(fn-expr + interface-mixed-caller Int
  (param first InlineInterfacePair)
  (param second InlineInterfaceSingle)
  (param choose Bool)
  (call interface-total (call choose-interface first second choose))
)
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

```brix
(import /core)

(main
  (print (call interface-caller (call InlineInterfacePair 20 22)))
  (print (call inline-inlined-name (call InlineInterfacePair 20 22)))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Struct)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (+ a b)
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (call (. value total))
)

(fn-expr + interface-caller Int
  (param value InlineInterfacePair)
  (call interface-total value)
)

(fn-expr + inline-inlined-name String @inline
  (param value InlineMeasure)
  (. (type-of value) inlined-name)
)
```

```output
42
InlineInterfacePair
```

```brix
(import /core)

(main
  (let pair (call InlineInterfacePair 20 22))
  (print (call interface-caller pair))
  (print (. pair a))
  (print (call interface-total pair))
  (print (. pair a))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Struct)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (before
      (do
        (+= a 1)
      )
    )
    (+ a b)
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (+ (call (. value total)) (call (. value total)))
)

(fn-expr + interface-caller Int
  (param value InlineInterfacePair)
  (call interface-total value)
)
```

```output
86
20
86
20
```

```brix
(import /core)

(main
  (print (call saved-info-name (call InlineInterfacePair 20 22)))
  (print (call returned-info-name (call InlineInterfacePair 20 22)))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (+ a b)
  )
)

(fn-expr + inline-saved-name String @inline
  (param value InlineMeasure)
  (. info inlined-name)
  (where
    (lazy info (type-of value))
  )
)

(fn-expr + inline-type-info _ @inline
  (param value InlineMeasure)
  (type-of value)
)

(fn-expr + saved-info-name String
  (param value InlineInterfacePair)
  (call inline-saved-name value)
)

(fn-expr + returned-info-name String
  (param value InlineInterfacePair)
  (. (call inline-type-info value) inlined-name)
)
```

```output
InlineInterfacePair
InlineInterfacePair
```

```brix crash
(import /core)

(main
  (print (call interface-caller (call InlineInterfacePair 20 22)))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (+ a b)
  )
  (fn-expr + only-concrete Int
    a
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (call (. value only-concrete))
)

(fn-expr + interface-caller Int
  (param value InlineInterfacePair)
  (call interface-total value)
)
```

```output
Exception: InlineMeasure has no method "only-concrete"
<no file:26>
      (call (. value only-concrete))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(import /core)

(main
  (print (call mixed-use-caller (call InlineInterfacePair 20 22) false))
  (print (call mixed-use-caller (call InlineInterfacePair 20 22) true))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (+ a b)
  )
)

(fn-expr + mixed-use-caller Int
  (param value InlineInterfacePair)
  (param escape Bool)
  (call mixed-interface-use value escape)
)

(fn-expr + mixed-interface-use Int @inline
  (param value InlineMeasure)
  (param escape Bool)
  (case? escape
    (+ (call opaque-interface value) (call opaque-interface value))
  )
  (default
    (call (. value total))
  )
)

(fn-expr + opaque-interface Int
  (param value InlineMeasure)
  (call (. value total))
)
```

```output
42
84
```

```brix
(import /core)

(main
  (print (call mixed-use-caller (call InlineInterfacePair 20 22) false))
  (print (call mixed-use-caller (call InlineInterfacePair 20 22) true))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Struct)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (+ a b)
  )
)

(fn-expr + mixed-use-caller Int
  (param value InlineInterfacePair)
  (param escape Bool)
  (call mixed-interface-use value escape)
)

(fn-expr + mixed-interface-use Int @inline
  (param value InlineMeasure)
  (param escape Bool)
  (case? escape
    (+ (call opaque-interface value) (call opaque-interface value))
  )
  (default
    (call (. value total))
  )
)

(fn-expr + opaque-interface Int
  (param value InlineMeasure)
  (call (. value total))
)
```

```output
42
84
```

```brix
(import /core)

(main
  (print (call interface-mixed-caller (call InlineInterfacePair 20 22) (call InlineInterfaceSingle 9) true))
  (print (call interface-mixed-caller (call InlineInterfacePair 20 22) (call InlineInterfaceSingle 9) false))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (+ a b)
  )
)

(type + InlineInterfaceSingle (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (fn-expr + total Int @inline
    a
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (call (. value total))
)

(fn-do + choose-interface InlineMeasure @inline
  (param first InlineMeasure)
  (param second InlineMeasure)
  (param choose Bool)
  (let result first)
  (if choose
    (= result second)
  )
  (return result)
)

(fn-expr + interface-mixed-caller Int
  (param first InlineInterfacePair)
  (param second InlineMeasure)
  (param choose Bool)
  (call interface-total (call choose-interface first second choose))
)
```

```output
9
42
```

```brix
(import /core)

(main
  (print (call interface-caller (as 42 InlineInterfacePair)))
  (print (call interface-join-caller (as 42 InlineInterfacePair) (as 7 InlineInterfacePair) true))
  (print (call interface-join-caller (as 42 InlineInterfacePair) (as 7 InlineInterfacePair) false))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is I32)
  (implements InlineMeasure)
  (fn-expr + total Int @inline
    (as this Int)
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (call (. value total))
)

(fn-expr + interface-caller Int
  (param value InlineInterfacePair)
  (call interface-total value)
)

(fn-do + choose-interface InlineMeasure @inline
  (param first InlineMeasure)
  (param second InlineMeasure)
  (param choose Bool)
  (let result first)
  (if choose
    (= result second)
  )
  (return result)
)

(fn-expr + interface-join-caller Int
  (param first InlineInterfacePair)
  (param second InlineInterfacePair)
  (param choose Bool)
  (call interface-total (call choose-interface first second choose))
)
```

```output
42
7
42
```

```brix
(import /core)

(main
  (print (call interface-caller (call InlineInterfacePair 20 22)))
  (print (call interface-join-caller (call InlineInterfacePair 20 22) (call InlineInterfacePair 3 4) true))
  (print (call interface-join-caller (call InlineInterfacePair 20 22) (call InlineInterfacePair 3 4) false))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-do + This _
    (param a _)
    (param b _)
  )
  (fn-expr + total Int @inline
    (+ a b)
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (call (. value total))
)

(fn-expr + interface-caller Int
  (param value InlineInterfacePair)
  (call interface-total value)
)

(fn-do + choose-interface InlineMeasure @inline
  (param first InlineMeasure)
  (param second InlineMeasure)
  (param choose Bool)
  (let result first)
  (if choose
    (= result second)
  )
  (return result)
)

(fn-expr + interface-join-caller Int
  (param first InlineInterfacePair)
  (param second InlineInterfacePair)
  (param choose Bool)
  (call interface-total (call choose-interface first second choose))
)
```

```output
42
7
42
```

```brix
(import /core)

(main
  (print (call interface-caller (call (of InlineInterfacePair Int) 20 22)))
  (print (call interface-join-caller (call (of InlineInterfacePair Int) 20 22) (call (of InlineInterfacePair Int) 3 4) true))
  (print (call interface-join-caller (call (of InlineInterfacePair Int) 20 22) (call (of InlineInterfacePair Int) 3 4) false))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + (of InlineInterfacePair T) (is Tuple)
  (implements InlineMeasure)
  (prop + a T)
  (prop + b T)
  (fn-expr + total Int @inline
    (+ a b)
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (call (. value total))
)

(fn-expr + interface-caller Int
  (param value (of InlineInterfacePair Int))
  (call interface-total value)
)

(fn-do + choose-interface InlineMeasure @inline
  (param first InlineMeasure)
  (param second InlineMeasure)
  (param choose Bool)
  (let result first)
  (if choose
    (= result second)
  )
  (return result)
)

(fn-expr + interface-join-caller Int
  (param first (of InlineInterfacePair Int))
  (param second (of InlineInterfacePair Int))
  (param choose Bool)
  (call interface-total (call choose-interface first second choose))
)
```

```output
42
7
42
```

```brix
(import /core)

(main
  (print (call interface-caller (call InlineInterfacePair 20 22)))
  (print (call interface-join-caller (call InlineInterfacePair 20 22) (call InlineInterfacePair 3 4) true))
  (print (call interface-join-caller (call InlineInterfacePair 20 22) (call InlineInterfacePair 3 4) false))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (prop + text String (= "rooted"))
  (fn-expr + total Int @inline
    (- (+ a b (. text size)) 6)
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (call (. value total))
)

(fn-expr + interface-caller Int
  (param value InlineInterfacePair)
  (call interface-total value)
)

(fn-do + choose-interface InlineMeasure @inline
  (param first InlineMeasure)
  (param second InlineMeasure)
  (param choose Bool)
  (let result first)
  (if choose
    (= result second)
  )
  (return result)
)

(fn-expr + interface-join-caller Int
  (param first InlineInterfacePair)
  (param second InlineInterfacePair)
  (param choose Bool)
  (call interface-total (call choose-interface first second choose))
)
```

```output
42
7
42
```

```brix
(import /core)

(main
  (print (call interface-caller (call InlineInterfacePair 20 22)))
  (print (call interface-join-caller (call InlineInterfacePair 20 22) (call InlineInterfacePair 3 4) true))
  (print (call interface-join-caller (call InlineInterfacePair 20 22) (call InlineInterfacePair 3 4) false))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (prop + nested InlineNested (= (call InlineNested 1)))
  (fn-expr + total Int @inline
    (- (+ a b (. nested last) (. nested field79)) 80)
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (call (. value total))
)

(fn-expr + interface-caller Int
  (param value InlineInterfacePair)
  (call interface-total value)
)

(fn-do + choose-interface InlineMeasure @inline
  (param first InlineMeasure)
  (param second InlineMeasure)
  (param choose Bool)
  (let result first)
  (if choose
    (= result second)
  )
  (return result)
)

(fn-expr + interface-join-caller Int
  (param first InlineInterfacePair)
  (param second InlineInterfacePair)
  (param choose Bool)
  (call interface-total (call choose-interface first second choose))
)

(type + InlineNested (is Tuple)
  (prop + last Int)
  (prop + field0 Int (= 0))
  (prop + field1 Int (= 1))
  (prop + field2 Int (= 2))
  (prop + field3 Int (= 3))
  (prop + field4 Int (= 4))
  (prop + field5 Int (= 5))
  (prop + field6 Int (= 6))
  (prop + field7 Int (= 7))
  (prop + field8 Int (= 8))
  (prop + field9 Int (= 9))
  (prop + field10 Int (= 10))
  (prop + field11 Int (= 11))
  (prop + field12 Int (= 12))
  (prop + field13 Int (= 13))
  (prop + field14 Int (= 14))
  (prop + field15 Int (= 15))
  (prop + field16 Int (= 16))
  (prop + field17 Int (= 17))
  (prop + field18 Int (= 18))
  (prop + field19 Int (= 19))
  (prop + field20 Int (= 20))
  (prop + field21 Int (= 21))
  (prop + field22 Int (= 22))
  (prop + field23 Int (= 23))
  (prop + field24 Int (= 24))
  (prop + field25 Int (= 25))
  (prop + field26 Int (= 26))
  (prop + field27 Int (= 27))
  (prop + field28 Int (= 28))
  (prop + field29 Int (= 29))
  (prop + field30 Int (= 30))
  (prop + field31 Int (= 31))
  (prop + field32 Int (= 32))
  (prop + field33 Int (= 33))
  (prop + field34 Int (= 34))
  (prop + field35 Int (= 35))
  (prop + field36 Int (= 36))
  (prop + field37 Int (= 37))
  (prop + field38 Int (= 38))
  (prop + field39 Int (= 39))
  (prop + field40 Int (= 40))
  (prop + field41 Int (= 41))
  (prop + field42 Int (= 42))
  (prop + field43 Int (= 43))
  (prop + field44 Int (= 44))
  (prop + field45 Int (= 45))
  (prop + field46 Int (= 46))
  (prop + field47 Int (= 47))
  (prop + field48 Int (= 48))
  (prop + field49 Int (= 49))
  (prop + field50 Int (= 50))
  (prop + field51 Int (= 51))
  (prop + field52 Int (= 52))
  (prop + field53 Int (= 53))
  (prop + field54 Int (= 54))
  (prop + field55 Int (= 55))
  (prop + field56 Int (= 56))
  (prop + field57 Int (= 57))
  (prop + field58 Int (= 58))
  (prop + field59 Int (= 59))
  (prop + field60 Int (= 60))
  (prop + field61 Int (= 61))
  (prop + field62 Int (= 62))
  (prop + field63 Int (= 63))
  (prop + field64 Int (= 64))
  (prop + field65 Int (= 65))
  (prop + field66 Int (= 66))
  (prop + field67 Int (= 67))
  (prop + field68 Int (= 68))
  (prop + field69 Int (= 69))
  (prop + field70 Int (= 70))
  (prop + field71 Int (= 71))
  (prop + field72 Int (= 72))
  (prop + field73 Int (= 73))
  (prop + field74 Int (= 74))
  (prop + field75 Int (= 75))
  (prop + field76 Int (= 76))
  (prop + field77 Int (= 77))
  (prop + field78 Int (= 78))
  (prop + field79 Int (= 79))
)
```

```output
42
7
42
```

```brix
(import /core)

(main
  (print (call inline-is (call InlineInterfacePair 20 22)))
  (print (call inline-is (call InlineInterfaceSingle 9)))
  (print (call interface-caller (call InlineInterfacePair 20 22)))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (+ a b)
  )
)

(type + InlineInterfaceSingle (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (fn-expr + total Int @inline
    a
  )
)

(fn-expr + inline-is Bool @inline
  (param value InlineMeasure)
  (is value InlineInterfacePair)
)

(fn-expr + inline-downcast Int @inline
  (param value InlineMeasure)
  (+ (. point a) (. point b))
  (where
    (lazy point (as value InlineInterfacePair))
  )
)

(fn-expr + interface-caller Int
  (param value InlineInterfacePair)
  (call inline-downcast value)
)
```

```output
true
false
42
```

```brix
(import /core)

(main
  (print (call interface-caller (call InlineInterfacePair (call InlineInner 20 22))))
  (print (call interface-join-caller (call InlineInterfacePair (call InlineInner 20 22)) (call InlineInterfacePair (call InlineInner 3 4)) true))
  (print (call interface-join-caller (call InlineInterfacePair (call InlineInner 20 22)) (call InlineInterfacePair (call InlineInner 3 4)) false))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Tuple)
  (delegate InlineMeasure)
  (prop + inner InlineInner)
)

(type + InlineInner (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (+ a b)
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (call (. value total))
)

(fn-expr + interface-caller Int
  (param value InlineInterfacePair)
  (call interface-total value)
)

(fn-do + choose-interface InlineMeasure @inline
  (param first InlineMeasure)
  (param second InlineMeasure)
  (param choose Bool)
  (let result first)
  (if choose
    (= result second)
  )
  (return result)
)

(fn-expr + interface-join-caller Int
  (param first InlineInterfacePair)
  (param second InlineInterfacePair)
  (param choose Bool)
  (call interface-total (call choose-interface first second choose))
)
```

```output
42
7
42
```

```brix
(import /core)

(main
  (print (call interface-caller (call InlineInterfacePair 20 22)))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineChild (is Interface)
  (implements InlineMeasure)
)

(type + InlineInterfacePair (is Tuple)
  (implements InlineChild)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (+ a b)
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (call (. value total))
)

(fn-expr + child-total Int @inline
  (param value InlineChild)
  (call interface-total (as value InlineMeasure))
)

(fn-expr + interface-caller Int
  (param value InlineInterfacePair)
  (call child-total value)
)
```

```output
42
```

```brix
(import /core)

(main
  (print (call interface-join-caller (call InlineInterfacePair 20 22) (call InlineInterfacePair 3 4) true))
  (print (call interface-join-caller (call InlineInterfacePair 20 22) (call InlineInterfacePair 3 4) false))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (+ a b)
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (call (. value total))
)

(fn-do + choose-interface InlineMeasure @inline
  (param first InlineMeasure)
  (param second InlineMeasure)
  (param choose Bool)
  (let result first)
  (let count 0)
  (while (< count 2)
    (if choose
      (= result second)
    )
    (+= count 1)
  )
  (return result)
)

(fn-expr + interface-join-caller Int
  (param first InlineInterfacePair)
  (param second InlineInterfacePair)
  (param choose Bool)
  (call interface-total (call choose-interface first second choose))
)
```

```output
7
42
```

```brix
(import /core)
(import /BxGC/BxGC.h)
(blank)

(main
  (print (call managed-caller (call ManagedPayload (call make-text 42))))
)

(type + Measured (is Interface)
  (fn-expr + size Int)
)

(type + ManagedPayload (is Tuple)
  (implements Measured)
  (prop + text String)
  (fn-do + size Int @inline
    (call C:brixGCCollect)
    (return (. text size))
  )
)

(fn-expr + make-text String
  (param value Int)
  (str "number " value)
)

(fn-expr + measured-size Int @inline
  (param value Measured)
  (call (. value size))
)

(fn-expr + managed-caller Int
  (param value ManagedPayload)
  (call measured-size value)
)
```

```output
9
```

```brix
(import /core)

(main
  (print (call identity-caller (call InlineInterfacePair 20 22)))
  (print (call interface-caller (call InlineInterfacePair 20 22)))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (+ a b)
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (call (. value total))
)

(type + InterfaceHolder (is Tuple)
  (prop + value InlineMeasure)
)

(fn-expr + same-interface Bool @inline
  (param first InlineMeasure)
  (param second InlineMeasure)
  (== (as first Pointer) (as second Pointer))
)

(fn-expr + identity-caller Bool
  (param value InlineInterfacePair)
  (and (call same-interface saved saved) (not (call same-interface value value)))
  (where
    (lazy saved (as value InlineMeasure))
  )
)

(fn-expr + interface-caller Int
  (param value InlineInterfacePair)
  (call interface-total (. (call InterfaceHolder value) value))
)
```

```output
true
42
```

```brix
(import /core)

(main
  (print (call identity-caller (call InlineInterfacePair 20 22)))
  (print (call interface-caller (call InlineInterfacePair 20 22)))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (+ a b)
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (call (. value total))
)

(type + InterfaceHolder (is Struct)
  (prop + value InlineMeasure)
)

(fn-expr + same-interface Bool @inline
  (param first InlineMeasure)
  (param second InlineMeasure)
  (== (as first Pointer) (as second Pointer))
)

(fn-expr + identity-caller Bool
  (param value InlineInterfacePair)
  (and (call same-interface saved saved) (not (call same-interface value value)))
  (where
    (lazy saved (as value InlineMeasure))
  )
)

(fn-expr + interface-caller Int
  (param value InlineInterfacePair)
  (call interface-total (. (call InterfaceHolder value) value))
)
```

```output
true
42
```

```brix
(import /core)

(main
  (print (call interface-caller (call InlineInterfacePair 20 22)))
  (print (call interface-join-caller (call InlineInterfacePair 20 22) (call InlineInterfacePair 3 4) true))
  (print (call interface-join-caller (call InlineInterfacePair 20 22) (call InlineInterfacePair 3 4) false))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int :get)
)

(type + InlineInterfacePair (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int :get @inline
    (+ a b)
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (. value total)
)

(fn-expr + interface-caller Int
  (param value InlineInterfacePair)
  (call interface-total value)
)

(fn-do + choose-interface InlineMeasure @inline
  (param first InlineMeasure)
  (param second InlineMeasure)
  (param choose Bool)
  (let result first)
  (if choose
    (= result second)
  )
  (return result)
)

(fn-expr + interface-join-caller Int
  (param first InlineInterfacePair)
  (param second InlineInterfacePair)
  (param choose Bool)
  (call interface-total (call choose-interface first second choose))
)
```

```output
42
7
42
```

```brix
(import /core)

(main
  (print (call interface-caller (call InlineInterfacePair 20 22)))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Struct)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (before
      (do
        (+= a 1)
      )
    )
    (+ a b)
  )
)

(fn-expr + interface-total Int @inline
  (param value InlineMeasure)
  (+ (call (. value total)) (call opaque-total value) (call (. value total)))
)

(fn-expr + interface-caller Int
  (param value InlineInterfacePair)
  (call interface-total value)
)

(fn-do + opaque-total Int
  (param value InlineMeasure)
  (if (is value InlineInterfacePair)
    (let point (as value InlineInterfacePair))
    (+= (. point a) 10)
  )
  (return (call (. value total)))
)
```

```output
129
```

```brix
(import /core)

(main
  (print (catch (call checked-total (call InlineInterfacePair 20 22) true)))
  (print (catch (call checked-total (call InlineInterfacePair 20 22) false)))
)

(type + InlineMeasure (is Interface)
  (fn-expr + total Int)
)

(type + InlineInterfacePair (is Tuple)
  (implements InlineMeasure)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + total Int @inline
    (+ a b)
  )
)

(type + InlineFailure (is Error))

(fn-do + checked-total Int (?! InlineFailure) @inline
  (param value InlineMeasure)
  (param succeeds Bool)
  (finally
    (print (str "cleanup " (call (. value total))))
  )
  (if succeeds
    (return (call (. value total)))
  )
  (throw (call InlineFailure "failed"))
)
```

```output
cleanup 42
42
cleanup 42
failed
```

```brix
(import /core)

(main
  (print (call forwarded (..< 0 0)))
  (print (call ordinary (..< 0 0)))
  (print (call forwarded (.. 2 2)))
  (print (call ordinary (.. 2 2)))
  (print (call forwarded (..< 2 7)))
  (print (call ordinary (..< 2 7)))
  (print (call forwarded (..< 7 2)))
  (print (call ordinary (..< 7 2)))
  (print (call forwarded (.. 2 7 (by: 2))))
  (print (call ordinary (.. 2 7 (by: 2))))
  (print (call forwarded (.. 7 2 (by: 2))))
  (print (call ordinary (.. 7 2 (by: 2))))
  (print (call forwarded (..< 2 7 (by: 2))))
  (print (call ordinary (..< 2 7 (by: 2))))
  (print (call forwarded (..< 7 2 (by: 2))))
  (print (call ordinary (..< 7 2 (by: 2))))
  (print (call forwarded (.. -4 3 (by: 2))))
  (print (call ordinary (.. -4 3 (by: 2))))
  (print (call forwarded (.. 2u64 7u64 (by: 2u64))))
  (print (call ordinary (.. 2u64 7u64 (by: 2u64))))
  (print (call forwarded (..< 7u64 2u64 (by: 2u64))))
  (print (call ordinary (..< 7u64 2u64 (by: 2u64))))
  (print (call forwarded (.. 2i64 7i64 (by: 2i64))))
  (print (call ordinary (.. 2i64 7i64 (by: 2i64))))
  (print (call forwarded (..< 0.0 1.0 (by: 0.25))))
  (print (call ordinary (..< 0.0 1.0 (by: 0.25))))
  (print (call forwarded (..< 0.0 1.0 (by: 0.1))))
  (print (call ordinary (..< 0.0 1.0 (by: 0.1))))
  (print (call forwarded (..< 0f32 1f32 (by: 0.1f32))))
  (print (call ordinary (..< 0f32 1f32 (by: 0.1f32))))
  (print (call forwarded (.. 2147483646 2147483647)))
  (print (call ordinary (.. 2147483646 2147483647)))
  (print (call forwarded (.. -2147483647 -2147483648)))
  (print (call ordinary (.. -2147483647 -2147483648)))
  (print (call stop-early (.. 1 10)))
)

(fn-do + (of ordinary T) String
  (param source (of Indexable T))
  (let text "[")
  (for (in item source)
    (+= text (str item ","))
  )
  (return (+ text "]"))
)

(fn-do + (of specialized T) String @inline
  (param source (of Indexable T))
  (let text "[")
  (for (in item source)
    (+= text (str item ","))
  )
  (return (+ text "]"))
)

(fn-expr + (of forwarded T) String @inline
  (param source (of Indexable T))
  (call specialized source)
)

(fn-do + stop-early Int @inline
  (param source (of Indexable Int))
  (for (in item source)
    (if (== item 3)
      (return item)
    )
  )
  (return 0)
)
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

```brix crash
(import /core)

(main
  (print (call hidden (.. 0 3)))
)

(fn-expr + hidden _ @inline
  (param source (of Indexable Int))
  (. source start)
)
```

```output
Exception: Indexable'I64 has no property "start"
<no file:10>
      (. source start)
      ^^^^^^^^^^^^^^^^
```
