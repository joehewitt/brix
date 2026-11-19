# Optionals

```brix
(main
  (print (call optional-middle 0 42))
  (print (call optional-middle 1 42))
  (print (call optional-middle 2 42))
  (print (call optional-middle 0 null))
  (print (call optional-middle 1 null))
  (print (call optional-middle 2 null))
  (print (call optional-middle-with-fallthrough 0 42))
  (print (call optional-middle-with-fallthrough 1 42))
  (print (call optional-middle-with-fallthrough 2 42))
  (print (call optional-middle-with-fallthrough 3 42))
)

(fn-do + optional-middle _
  (param branch Int)
  (param value (? Int))
  (if (== branch 0)
    (return value)
  )
  (if (== branch 1)
    (return 7)
  )
  (return value)
)

(fn-do + optional-middle-with-fallthrough _
  (param branch Int)
  (param value (? Int))
  (if (== branch 0)
    (return value)
  )
  (if (== branch 1)
    (return 7)
  )
  (if (== branch 2)
    (return value)
  )
)
```

```output
42
7
42
unknown
7
unknown
42
7
42
unknown
```

```brix
(main
  (let a (? I32))
  (print a)
)
```

```output
unknown
```

```brix
(main
  (let a (? I32))
  (print a)
  (= a 1)
  (print a)
  (= a (call (of Optional I32)))
  (print a)
)
```

```output
unknown
1
unknown
```

```brix
(main
  (do-new
    (let a (? I32))
    (print (as a I32))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Optional'core/number:I32 is null
```

```brix
(main
  (let x (? Int))
  (let y 1)
  (let attempt
    (do-new
      (= y (!! x))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
  (print y)
)
```

```output
Optional'core/number:I64 is null
1
```

```brix
(main
  (let a (? I32) 1)
  (print (as a I32))
)
```

```output
1
```

### Pass optional to function

```brix
(main
  (let a (? I32) 1)
  (call foo a)
  (let b (? I32))
  (call foo b)
)

(fn-do + foo _
  (param a (? I32))
  (print (if-expr a 5 3))
)
```

```output
5
3
```

### A parameter that may be null can be left out when it is written with `= null`.

```brix
(main
  (let a (? I32) 5)
  (call foo a)
  (call foo)
)

(fn-do + foo _
  (param a (? I32) (= null))
  (print (if-expr a 5 3))
)
```

```output
5
3
```

### A parameter that may be null and has no written default must be given, even as null.

```brix crash
(main
  (let a (? I32) 5)
  (call foo a)
)

(fn-do + foo _
  (param a (? I32))
  (param b (? I32))
  (print (if-expr a 5 3))
)
```

```output
Exception: No function "foo" accepts (I32?); it is declared as "foo(a I32?, b I32?)"; "b" may be null but has no default, so a call must give it: pass null, or declare "(param b (? I32) (= null))"
<no file:4>
      (call foo a)
      ^^^^^^^^^^^^
```

### Parameters with written defaults, `= null` among them, can be left out.

```brix
(main
  (let a (? I32) 5)
  (call foo a)
  (call foo)
)

(fn-do + foo _
  (param a (? I32) (= null))
  (param b _ (= 1))
  (param c (? I32) (= null))
  (print (if-expr a 5 3))
  (print b)
  (print (if-expr c 4 2))
)
```

```output
5
1
2
3
1
2
```

### A required parameter can follow one that may be null and has no written default.

```brix
(main
  (let a (? I32) 5)
  (call foo a 2)
  (call foo null 3)
)

(fn-do + foo _
  (param a (? I32))
  (param b I32)
  (print (?? a 0))
  (print b)
)
```

```output
5
2
0
3
```

### Optional arguments not required to be passed to call (if at the end)

```brix
(main
  (let a (? I32))
  (call foo a)
  (blank)
  (let b (? I32) 5)
  (call foo b)
)

(fn-do + foo _
  (param a I32 (= 1))
  (print a)
)

(fn-expr + bar _
  1
)
```

```output
1
5
```

```brix
(main
  (let attempt
    (do-new
      (print (as (call foo 1) I32))
      (on-failure failure
        (print failure)
        (print (as (call foo 11) I32))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + foo _
  (param a _)
  (if (> a 10)
    (return 5)
  )
)
```

```output
Optional'core/number:I64 is null
5
```

```brix
(main
  (print (call foo))
)

(fn-do + foo (? String)
  (if false
    (return "foo")
  )
)
```

```output
unknown
```

```brix
(main
  (print (call foo))
  (print (type-of (call foo)))
)

(fn-expr + foo (? String)
  (case? true
    null
  )
  (default
    "x"
  )
)
```

```output
unknown
[TypeInfo Optional'String]
```

```brix
(main
  (print (call foo))
)

(fn-do + foo (? Int)
  (if false
    (return 42)
  )
)
```

```output
unknown
```

```brix
(main
  (print (call foo))
  (print (type-of (call foo)))
)

(fn-do + foo (? Int)
  (if false
    (return (call (of Optional Int) 42))
    (else
      (return 3)
    )
  )
)
```

```output
3
[TypeInfo Optional'I64]
```

```brix
(main
  (print (call foo))
  (print (type-of (call foo)))
)

(fn-do + foo (? Int)
  (if false
    (return (call (of Optional Int) 42))
  )
)
```

```output
unknown
[TypeInfo Optional'I64]
```

```brix
(main
  (print (call foo))
  (print (type-of (call foo)))
)

(fn-do + foo (? Int)
  (if true
    (return (call (of Optional Int) 42))
  )
)
```

```output
42
[TypeInfo Optional'I64]
```

```brix
(main
  (print (call foo 24))
)

(fn-expr + (of foo T) _
  (param x (? T))
  x
)
```

```output
24
```

```brix
(main
  (print (call Optional 24))
)
```

```output
24
```

```brix
(main
  (let a (? Int) 42)
  (print (!! a))
)
```

```output
42
```

### Force unwrap checks a null numeric optional.

```brix crash
(main
  (let value (? Int))
  (print (!! value))
)
```

```stderr
Unhandled panic: Optional'core/number:I64 is null
<no file:4>

Task 1

Stack trace:
  0  __source__:main no file:4:10
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### Force unwrap checks a null class optional before dereferencing it.

```brix crash
(main
  (let value (? Foo))
  (print (. (!! value) zing))
)

(type + Foo
  (prop + zing _ (= 100))
)
```

```stderr
Unhandled panic: Optional'[Object __source__:Foo] is null
<no file:4>

Task 1

Stack trace:
  0  __source__:main no file:4:13
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### Forcing a value that is never null is illegal.

```brix crash
(main
  (let count Int 1)
  (print (!! count))
)
```

```output
Exception: "count" is I64, which is never null here, so it needs no "!!"
<no file:4>
      (print (!! count))
             ^^^^^^^^^^
```

### Forcing an optional the flow knows holds a value is illegal.

```brix crash
(main
  (let count (? Int) 1)
  (if count
    (print (!! count))
  )
)
```

```output
Exception: "count" is I64, which is never null here, so it needs no "!!"
<no file:5>
        (print (!! count))
               ^^^^^^^^^^
```

### A force in generic code is legal when its operand is optional in some instance.

```brix
(main
  (print (call first 3))
  (print (call first (as 4 (? Int))))
)

(fn-expr + (of first T) _
  (param value T)
  (!! value)
)
```

```output
3
4
```

### A force is legal where a local the flow knows holds a value is tested, as it tests the value.

```brix
(main
  (let count (? Int) 0)
  (if (!= count null)
    (print (if-expr (and count true) "present" "absent"))
    (print (if-expr (and (!! count) true) "nonzero" "zero"))
  )
)
```

```output
present
zero
```

### A typed declaration does not unwrap an optional: write ! or ?? to take what it holds.

```brix crash
(main
  (let value (? Int))
  (let unwrapped Int value)
  (print unwrapped)
)
```

```output
Exception: Expected I64 here, but this is I64?, which may hold nothing; write "(!! value)" to unwrap it, or "(?? value fallback)" to give a value for nothing
<no file:4>
      (let unwrapped Int value)
                     ^^^
```

### An optional number on the right of an assigning operator is not unwrapped.

```brix crash
(main
  (let total 1)
  (+= total (call given 2))
)

(fn-expr + given (? Int)
  (param value Int)
  value
)
```

```output
Exception: Expected I64 here, but this is I64?, which may hold nothing; write "(!! (call given 2))" to unwrap it, or "(?? (call given 2) fallback)" to give a value for nothing
<no file:4>
      (+= total (call given 2))
                ^^^^^^^^^^^^^^
```

## Optional Chaining

### Optional chaining on a non-optional receiver is illegal.

```brix crash
(main
  (let foo (call Foo))
  (print (?. foo zing))
)

(type + Foo
  (prop + zing _ (= 100))
)
```

```output
Exception: "foo" is Foo, which is never null, so it needs no "?"; write "(. foo zing)"
<no file:4>
      (print (?. foo zing))
             ^^^^^^^^^^^^^
```

### Optional chaining on a narrowed optional receiver is illegal.

```brix crash
(main
  (let foo (? Foo) (call Foo))
  (if foo
    (print (?. foo zing))
  )
)

(type + Foo
  (prop + zing _ (= 100))
)
```

```output
Exception: "foo" is Foo, which is never null, so it needs no "?"; write "(. foo zing)"
<no file:5>
        (print (?. foo zing))
               ^^^^^^^^^^^^^
```

### A parameter written with no type needs `?` where one call gives it nothing, though another gives it a value that is never null.

```brix
(main
  (print (call size (call Inner)))
  (print (call size null))
)

(type + Inner
  (prop + width Int (= 3))
)

(fn-expr + size Int
  (param inner _)
  (?? (?. inner width) 0)
)
```

```output
3
0
```

```brix
(main
  (let foo (? Foo))
  (print (?. foo zing))
  (print (type-of (?. foo zing)))
  (= foo (as (call Foo) (? Foo)))
  (print (?. foo zing))
)

(type + Foo
  (prop + zing _ (= 100))
)
```

```output
unknown
[TypeInfo Optional'I64]
100
```

```brix
(main
  (let foo (? Foo))
  (print (?. (?. foo bar) bing))
  (print (type-of (?. (?. foo bar) bing)))
  (= foo (as (call Foo) (? Foo)))
  (print (?. (?. foo bar) bing))
  (let f (!! foo))
  (= (. f bar) (call Bar))
  (print (?. (?. foo bar) bing))
)

(type + Foo
  (prop + bar (? Bar))
)

(type + Bar
  (prop + bing _ (= 100))
)
```

```output
unknown
[TypeInfo Optional'I64]
unknown
100
```

```brix
(main
  (let foo (? Foo))
  (print (. (?. foo stuff) length))
  (print (type-of (. (?. foo stuff) length)))
  (= foo (as (call Foo) (? Foo)))
  (print (. (?. foo stuff) length))
)

(type + Foo
  (prop + stuff _ (= (list 1 2 3)))
)
```

```output
unknown
[TypeInfo Optional'Size]
3
```

```brix
(main
  (let foo (? Foo))
  (print (call (?. foo bing)))
  (print (. (call (?. foo bing)) boom))
  (print (type-of (. (call (?. foo bing)) boom)))
  (= foo (as (call Foo) (? Foo)))
  (print (call (?. foo bing)))
  (print (. (call (?. foo bing)) boom))
)

(type + Foo
  (fn-expr + bing _
    (call Bing)
  )
)

(type + Bing
  (prop + boom _ (= 99))
)
```

```output
unknown
unknown
[TypeInfo Optional'I64]
[Object __source__:Bing]
99
```

### Optional void-returning method called as a statement

```brix
(main
  (let win (? Window))
  (call (?. win focus))
  (= win (as (call Window) (? Window)))
  (call (?. win focus))
)

(type + Window
  (fn-do + focus _
    (print "focus")
  )
)
```

```output
focus
```

### Optional void-returning interface method called as a statement

```brix
(main
  (let thing (? Thing))
  (call (?. thing focus))
  (= thing (as (call Window) (? Thing)))
  (call (?. thing focus))
)

(type + Thing (is Interface)
  (fn-expr + focus Void)
)

(type + Window
  (implements Thing)
  (fn-do + focus _
    (print "focus")
  )
)
```

```output
focus
```

```brix
(main
  (print (?. (call foo) bar))
)

(fn-expr + foo (? Foo)
  (call Foo)
)

(type + Foo
  (prop + bar _ (= 42))
)
```

```output
42
```

```brix
(main
  (let foo (? (list Int)))
  (print (?[] foo 0))
  (= foo (as (list 1 2 3) (? (list Int))))
  (print (?[] foo 0))
)
```

```output
unknown
1
```

## Optional assignment elimination

```brix
(main
  (let foo (? Foo))
  (= (?. foo bar) 42)
  (print (?. foo bar))
  (= foo (as (call Foo) (? Foo)))
  (= (?. foo bar) 42)
  (print (?. foo bar))
)

(type + Foo
  (prop + bar Int)
)
```

```output
unknown
42
```

```brix
(main
  (let foo (? Foo))
  (= (?. (?. foo bar) bing) 42)
  (print (?. (?. foo bar) bing))
  (= foo (as (call Foo) (? Foo)))
  (= (?. foo bar) (call Bar))
  (print (?. (?. foo bar) bing))
  (= (?. (?. foo bar) bing) 42)
  (print (?. (?. foo bar) bing))
)

(type + Foo
  (prop + bar (? Bar))
)

(type + Bar
  (prop + bing _ (= 100))
)
```

```output
unknown
100
42
```

### Compound assignment through an optional chain updates a present target and skips an absent one.

```brix
(main
  (let foo (? Foo))
  (+= (?. foo bar) 1)
  (print (?. foo bar))
  (= foo (as (call Foo) (? Foo)))
  (+= (?. foo bar) 2)
  (+= (?. foo bar) 3)
  (print (?. foo bar))
)

(type + Foo
  (prop + bar Int (= 10))
)
```

```output
unknown
15
```

```brix
(main
  (let foo Foo)
  (= (?[] (. foo bar) 0) 1)
  (print (?[] (. foo bar) 0))
  (= (. foo bar) (call Bar))
  (print (?[] (. foo bar) 0))
  (= (?[] (. foo bar) 0) 1)
  (print (?[] (. foo bar) 0))
)

(type + Foo
  (prop + bar (? Bar))
)

(type + Bar
  (prop + v _ (= 42))
  (fn-expr + (index-get this (param index Index)) _
    v
  )
  (fn-do + (index-set this (param index Index) (param item Int)) _
    (= v item)
  )
)
```

```output
unknown
42
1
```

```brix
(main
  (let foo (? (list Int)))
  (= (?[] foo 0) 10)
  (print (?[] foo 0))
  (= foo (as (list 1 2 3) (? (list Int))))
  (print (?[] foo 0))
  (= (?[] foo 0) 10)
  (print (?[] foo 0))
)
```

```output
unknown
1
10
```

```brix
(main
  (let foo Foo)
  (= (?[] (. foo bar) 0) 1)
  (print (?[] (. foo bar) 0))
  (= (. foo bar) (list 1 2 3))
  (print (?[] (. foo bar) 0))
  (= (?[] (. foo bar) 0) 10)
  (print (?[] (. foo bar) 0))
)

(type + Foo
  (prop + bar (? (list Int)))
)
```

```output
unknown
1
10
```

## Optional statement elimination

```brix
(main
  (print (call foo))
  (print (call foo 42))
)

(fn-do + foo _
  (param x (? Int) (= null))
  (return? x)
  (return 99)
)
```

```output
99
42
```

```brix
(main
  (for (in v (call foo))
    (print v)
  )
  (for (in v (call foo 42))
    (print v)
  )
)

(fn-do + foo _
  (param x (? Int) (= null))
  (yield 1)
  (yield? x)
  (yield 2)
)
```

```output
1
2
1
42
2
```

### Early-null return optionalizes a value continuation

```brix
(main
  (print (call maybe false))
  (print (call maybe true))
  (print (type-of (call maybe true)))
)

(fn-do + maybe _
  (param keep Bool)
  (return-if (if (not keep)))
  (return 7)
)
```

```output
unknown
7
[TypeInfo Optional'I64]
```

### Early-null return does not nest optional continuation values

```brix
(main
  (let present (? Int) 7)
  (print (call maybe false present))
  (print (call maybe true present))
  (print (type-of (call maybe true present)))
)

(fn-do + maybe _
  (param keep Bool)
  (param value (? Int))
  (return-if (if (not keep)))
  (return value)
)
```

```output
unknown
7
[TypeInfo Optional'I64]
```

### Early-null return narrows optional continuations

```brix
(main
  (print (call add-one 41))
  (print (call add-one null))
)

(fn-do + add-one _
  (param value (? Int))
  (return-if (if (not value)))
  (return (+ value 1))
)
```

```output
42
unknown
```

### Condition-only early return infers false in Bool continuations

```brix
(main
  (print (call maybe true))
  (print (call maybe false))
)

(fn-do + maybe _
  (param stop Bool)
  (return-if (if stop))
  (return true)
)
```

```output
false
true
```

### Two-line optional early return can return no value with null

```brix
(main
  (call voidy true)
  (call voidy false)
)

(fn-do + voidy _
  (param stop Bool)
  (return-if (if stop))
  (print "ran")
)
```

```output
ran
```

### Shorthand optional early return can return no value

```brix
(main
  (print (call maybe 0))
  (print (call maybe 7))
)

(fn-do + maybe _
  (param value Int)
  (return-if (if (== value 0)))
  (return value)
)
```

```output
unknown
7
```

### Shorthand optional early return can return void

```brix
(main
  (call voidy true)
  (call voidy false)
)

(fn-do + voidy _
  (param stop Bool)
  (return-if (if stop))
  (print "ran")
)
```

```output
ran
```

### Guard-only early-null functions remain Void

```brix crash
(main
  (print (?? (call guard-only false) "optional"))
)

(fn-do + guard-only _
  (param keep Bool)
  (return-if (if (not keep)))
)
```

```output
Exception: Operator "??" is not defined for Void and String
<no file:3>
      (print (?? (call guard-only false) "optional"))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (let x (? Int))
  (print? x)
  (= x 1)
  (print? x)
)
```

```output
1
```

```brix
(main
  (let x (? I32))
  (let y Float 1)
  (print (=? y x))
  (print y)
  (= x 10)
  (=? y x)
  (print y)
)
```

```output
1
1
10
```

```brix
(main
  (let x (? Int))
  (let y Float 1)
  (print (+=? y x))
  (print y)
  (= x 10)
  (+=? y x)
  (print y)
)
```

```output
1
1
11
```

```brix
(main
  (let thing (? Thing))
  (use? thing)
  (do-use? thing
    (print "using")
  )
  (= thing (as (call Thing) (? Thing)))
  (use? thing)
  (do-use? thing
    (print "using")
  )
)

(type + Thing
  (fn-do + begin-using _
    (print "begin")
    (return this)
  )
  (fn-do + end-using _
    (print "end")
  )
)
```

```output
begin
begin
using
end
end
```

## Comparison

```brix
(main
  (let foo (? Int))
  (print (== foo null))
  (print (== null foo))
  (= foo (as 1 (? Int)))
  (print (== foo null))
  (print (== null foo))
)
```

```output
true
true
false
false
```

```brix
(main
  (let foo (? Int))
  (let bar (? Int))
  (print (== foo bar))
  (print (!= foo bar))
  (= foo (as 1 (? Int)))
  (print (== foo bar))
  (print (!= foo bar))
  (= bar 1)
  (print (== foo bar))
  (print (!= foo bar))
)
```

```output
true
false
false
true
true
false
```

```brix
(main
  (let foo (? Int))
  (print (== foo 2))
  (print (!= foo 2))
  (print (== 2 foo))
  (print (!= 2 foo))
  (= foo 2)
  (print (== foo 2))
  (print (== 2 foo))
  (print (== foo 3))
  (print (== 3 foo))
)
```

```output
false
true
false
true
true
true
false
false
```

```brix
(main
  (print (?? "" "foo"))
  (print (?? "bar" "foo"))
)
```

```output
foo
bar
```

```brix
(main
  (print (if-expr false 0.0 1.0))
  (print (if-expr false 0 1.0))
)
```

```output
1
1
```

```brix
(main
  (let a (? Int))
  (let aa Int 3)
  (let b (if-expr a 1 aa))
  (print b)
  (print (type-of b))
  (blank)
  (let c (if-expr aa 1 a))
  (print c)
  (print (type-of c))
)
```

```output
3
[TypeInfo I64]
1
[TypeInfo Optional'I64]
```

```brix
(main
  (let a (? Int))
  (let aa (? Int) 3)
  (let b (if-expr a 1 aa))
  (print b)
  (print (type-of b))
)
```

```output
3
[TypeInfo Optional'I64]
```

```brix
(main
  (let a (? Int))
  (let aa (? Int))
  (let b (if-expr a 1 aa))
  (print b)
  (print (type-of b))
)
```

```output
unknown
[TypeInfo Optional'I64]
```

### Conditional structure matches flatten optional transform results

```brix
(main
  (print (type-of (call maybe true false)))
  (if (call maybe true false)
    (print "inner false passed")
    (else
      (print "inner false failed")
    )
  )
  (if (call maybe false false)
    (print "outer false passed")
    (else
      (print "outer false failed")
    )
  )
  (if (call maybe true true)
    (print (!! (call maybe true true)))
    (else
      (print "true failed")
    )
  )
)

(fn-expr + maybe-inner _
  (param flag Bool)
  (only-if "ok" (if flag))
)

(fn-expr + maybe _
  (param outer Bool)
  (param inner Bool)
  (case? (only-if 1 (if outer))
    (call maybe-inner inner)
  )
)
```

```output
[TypeInfo Optional'String]
inner false failed
outer false failed
ok
```

### An unwrapped optional converts to an alias of its type for dispatch

```brix
(main
  (let left (? I16) 2i16)
  (let right (? I16) 7i16)
  (print (call clip-for-dispatch (!! left) (!! right) 0i16 10i16))
)

(type + Coord (is I16))

(fn-expr + clip-for-dispatch _
  (param left Coord)
  (param right Coord)
  (param min Coord)
  (param max Coord)
  (tuple left right)
)
```

```output
(2, 7)
```

### A null match arm preserves absence when the other arm returns a tuple.

```brix
(main
  (let content TupleOptionalContent .none)
  (let pair (call tuple-content content))
  (print (type-of pair))
  (print (if-expr pair "present" "absent"))
  (let present (call tuple-content (call .item "name" (call TupleOptionalItem 7))))
  (print (. (!! present) name))
  (print (. (!! present) item value))
  (let switch-pair (call switch-tuple-content .none))
  (print (if-expr switch-pair "present" "absent"))
  (let switch-present (call switch-tuple-content (call .other "other" (call TupleOptionalItem 9))))
  (print (. (!! switch-present) name))
  (print (. (!! switch-present) item value))
)

(fn-expr + tuple-content (? (tuple (: name String) (: item TupleOptionalItem)))
  (param content TupleOptionalContent)
  (match content
    (case (call .item name item)
      (tuple name item)
    )
    (default
      null
    )
  )
)

(fn-expr + switch-tuple-content (? (tuple (: name String) (: item TupleOptionalItem)))
  (param content TupleOptionalContent)
  (match content
    (case (call .item name item)
      (tuple name item)
    )
    (case (call .other name item)
      (tuple name item)
    )
    (default
      null
    )
  )
)

(type + TupleOptionalContent (is Union)
  (variant none)
  (variant item (tuple (: name String) (: item TupleOptionalItem)))
  (variant other (tuple (: name String) (: item TupleOptionalItem)))
)

(type + TupleOptionalItem
  (prop + value I32)
  (fn-do + This _
    (param value _)
  )
)
```

```output
[TypeInfo Optional'Tuple]
absent
name
7
absent
other
9
```

### A type pattern matches the present payload of an optional union without a prior null check.

```brix
(main
  (let items (list DocValue))
  (+= items (as 1 DocValue))
  (print (call describe (as items DocValue)))
  (print (call describe (as "text" DocValue)))
  (print (call describe null))
)

(fn-expr + describe String
  (param value (? DocValue))
  (match value
    (case (: values (list DocValue))
      (str "array " (. values length))
    )
    (case (: text String)
      (str "string " text)
    )
    (default
      "other"
    )
  )
)

(type + DocValue (is Union)
  (variant Null)
  (variant Bool)
  (variant Int)
  (variant Float)
  (variant String)
  (variant (list DocValue))
  (variant (map String DocValue))
)
```

```output
array 1
string text
other
```

### An absent optional becomes a union's Null case with as.

```brix
(main
  (let values (map String DocValue))
  (= ([] values "count") 2)
  (let count (as ([] values "count") DocValue))
  (let missing (as ([] values "missing") DocValue))
  (print (call describe count))
  (print (call describe missing))
  (print (call describe (as ([] values "missing") DocValue)))
)

(fn-expr + describe String
  (param value DocValue)
  (if-expr (is value Null) "null" "present")
)

(type + DocValue (is Union)
  (variant Null)
  (variant Int)
  (variant String)
)
```

```output
present
null
null
```

### An optional becomes a union with a Null case and a case its value becomes, as an argument, a declared local and a variadic item.

```brix
(main
  (let missing (? String) null)
  (let present (? String) "x")
  (let count (? I32) 4)
  (let held Cell present)
  (print (call describe missing))
  (print (call describe count))
  (print (call describe held))
  (print (call all missing present 3 count))
)

(type + Cell (is Union)
  (variant Null)
  (variant I64)
  (variant String)
)

(fn-expr + describe String
  (param cell Cell)
  (match cell
    (case (exact Null)
      "null"
    )
    (case (similar I64)
      "int"
    )
    (case (similar String)
      "text"
    )
  )
)

(fn-expr + all String
  (param ... cells (list Cell))
  (call (. " " join) (in (call describe each) cells))
)
```

```output
null
int
text
null text int int
```

### An optional does not become a union with no Null case unasked.

```brix compile-error
(main
  (let present (? String) "x")
  (print (call describe present))
)

(type + Cell (is Union)
  (variant I64)
  (variant String)
)

(fn-expr + describe String
  (param cell Cell)
  (match cell
    (case (similar I64)
      "int"
    )
    (case (similar String)
      "text"
    )
  )
)
```

```output
Exception: No function "describe" accepts (String?); it is declared as "describe(cell Cell)"
<no file:4>
      (print (call describe present))
             ^^^^^^^^^^^^^^^^^^^^^^^
```

### Force unwrap can appear in the middle of member, call, and index chains.

```brix
(main
  (let holder (call ChainHolder (call ChainNode (call ChainNode null 3) 5)))
  (print (. (!! (. holder node)) value))
  (print (. (!! (. (!! (. holder node)) next)) value))
  (print (. (!! (. (!! (call (. holder find) 5)) next)) value))
  (print (. (!! (call chain-node 7)) value))
  (let nodes (list (? ChainNode)))
  (+= nodes (. holder node))
  (print (. (!! (. (!! ([] nodes 0)) next)) value))
  (print (!= (. (!! (. holder node)) value) 4))
  (let forced (!! (. holder node)))
  (print (. forced value))
  (print (!= (. holder node) null))
  (print (!= (call (. holder find) 1) null))
)

(fn-expr + chain-node (? ChainNode)
  (param value I32)
  (call ChainNode null value)
)

(type + ChainNode
  (prop + next (? ChainNode))
  (prop + value I32)
  (fn-do + This _
    (param next _)
    (param value _)
  )
)

(type + ChainHolder
  (prop + node (? ChainNode))
  (fn-do + This _
    (param node _)
  )
  (fn-do + find (? ChainNode)
    (param value I32)
    (let current node)
    (return-if current (if (and current (== (. current value) value))))
    (return null)
  )
)
```

```output
5
3
3
7
3
true
5
true
false
```

### Comparing an absent optional with custom equality does not unwrap it.

```brix
(main
  (let absent (? Point2) null)
  (let present (call Point2 1 2))
  (print (!= absent present))
  (print (== absent present))
  (print (!= present absent))
  (let other (? Point2) (call Point2 1 2))
  (print (!= other present))
  (print (== other present))
  (let none (? Point2) null)
  (print (== absent none))
  (let name (? String) null)
  (print (== name "a"))
  (print (!= name "a"))
)

(type + Point2 (is Tuple)
  (prop + x Int)
  (prop + y Int)
  (fn-expr + (infix this == (param other Point2)) _
    (and (== x (. other x)) (== y (. other y)))
  )
)
```

```output
true
false
true
false
true
true
false
true
```

### A presence operand stays a presence test when the other side of a logical operator narrowed it.

```brix
(main
  (let a (? Pair2) (call Pair2 1 2))
  (let b (? Pair2) (call Pair2 1 2))
  (let c (? Pair2) (call Pair2 3 4))
  (let none (? Pair2) null)
  (print (call same a b))
  (print (call same a c))
  (print (call same none none))
  (print (call same a none))
  (print (call both a b))
  (print (call both a none))
  (print (call both none b))
  (let zero (? Int) 0)
  (let one (? Int) 1)
  (print (call present-after zero one))
  (print (call present-after zero null))
)

(fn-expr + same Bool
  (param first (? Pair2))
  (param second (? Pair2))
  (or (and (not first) (not second)) (and first second (== first second)))
)

(fn-expr + both Bool
  (param first (? Pair2))
  (param second (? Pair2))
  (or (not first) (and first second))
)

(fn-expr + present-after Bool
  (param first (? Int))
  (param second (? Int))
  (or (not first) (and first second))
)

(type + Pair2 (is Tuple)
  (prop + x Int)
  (prop + y Int)
  (fn-expr + (infix this == (param other Pair2)) _
    (and (== x (. other x)) (== y (. other y)))
  )
)
```

```output
true
false
true
false
true
false
true
true
false
```

### Unwrapping an optional text that holds nothing panics.

```brix crash
(main
  (let none (? String))
  (let lost String (!! none))
  (print lost)
)
```

```stderr
Unhandled panic: Optional'core/string:String is null
<no file:4>

Task 1

Stack trace:
  0  __source__:main no file:4:20
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### An optional argument given to a parameter with no default must be unwrapped.

```brix crash
(main
  (let a (? I32))
  (call strict a)
)

(fn-do + strict _
  (param a I32)
  (print a)
)
```

```output
Exception: No function "strict" accepts (I32?); it is declared as "strict(a I32)"
<no file:4>
      (call strict a)
      ^^^^^^^^^^^^^^^
```

### An optional argument that holds nothing takes the parameter's default, and one that holds a value is unwrapped.

```brix
(main
  (let a (? I32))
  (let b (? I32) 5)
  (call foo a)
  (call foo b)
  (call bar "x" a)
  (call bar "y" b)
  (call zap a)
  (call zap b)
  (call (. (call Thing) go) a)
  (call (. (call Thing) go) b)
  (call foo (?? a 9))
)

(fn-do + foo _
  (param a I32 (= 1))
  (print a)
)

(fn-do + bar _
  (param s String)
  (param a Float (= 2.5))
  (print (tuple a s))
)

(fn-do + zap _
  (param a (? I32) (= 7))
  (print a)
)

(type + Thing
  (fn-do + go _
    (param n I32 (= 3))
    (print n)
  )
)
```

```output
1
5
(2.5, x)
(5, y)
7
5
3
5
9
```

### An optional text that holds nothing prints and interpolates as unknown, and ! takes what one holds.

```brix
(main
  (let x (? String))
  (print x)
  (let y (? String) "hi")
  (print y)
  (print (str "[" x "]"))
  (let s String (!! y))
  (print s)
)
```

```output
unknown
hi
[unknown]
hi
```

### An optional text that holds nothing is unknown as a field of a tuple.

```brix
(main
  (let x (? String))
  (print (tuple x 1))
)

(fn-do + show _
  (param s String)
  (print s)
)
```

```output
(unknown, 1)
```

### A function that returns a value on some paths and nothing or null on others returns an optional.

```brix
(main
  (print (call maybe true))
  (print (call maybe false))
  (print (call pick 1))
  (print (call pick 2))
  (print (call pick 3))
  (print (?? (call name true) "none"))
  (print (?? (call name false) "none"))
)

(fn-do + maybe _
  (param flag Bool)
  (if flag
    (return 7)
  )
)

(fn-do + pick _
  (param n Int)
  (if (== n 1)
    (return 10)
  )
  (if (== n 2)
    (return null)
  )
  (return 30)
)

(fn-do + name _
  (param flag Bool)
  (if flag
    (return "set")
  )
)
```

```output
7
unknown
10
unknown
30
set
none
```

### A return and a print written with a question mark act only where their optional holds a value, and a conditional return gives the optional itself.

```brix
(main
  (print (call first 3 null))
  (print (call first 3 8))
  (print (?? (call late 1) "none"))
  (print (?? (call late 5) "none"))
  (call show null)
  (call show "here")
  (let n (? Int) 4)
  (print? n)
  (let m (? Int))
  (print? m)
  (print "end")
)

(fn-do + first Int
  (param b Int)
  (param a (? Int))
  (return? a)
  (return b)
)

(fn-do + late (? String)
  (param n Int)
  (let label (? String) "big")
  (return-if label (if (> n 3)))
  (let none (? String))
  (return-if none (if (> n 0)))
  (return "small")
)

(fn-do + show _
  (param s (? String))
  (print? s)
)
```

```output
3
8
none
big
here
4
end
```

### A conditional return of an optional returns it whether or not it holds a value.

```brix
(main
  (print (?? (call pick 1) 0))
  (print (?? (call pick 5) 0))
  (print (?? (call pick 9) 0))
  (print (call strict 5))
)

(fn-do + pick _
  (param n Int)
  (let held (? Int) n)
  (let none (? Int))
  (return-if none (if (== n 1)))
  (return-if held (if (== n 5)))
)

(fn-do + strict Int
  (param n Int)
  (let held (? Int) n)
  (return-if (!! held) (if (> n 3)))
  (return 0)
)
```

```output
0
5
0
5
```

### An arm that is a test alone gives what the tested optional holds.

```brix
(main
  (print (call test))
  (print (call test 2))
  (print (call test 0))
  (print (call word))
  (print (call word "x"))
)

(fn-expr + test _
  (param t (? Int) (= null))
  (case? (> 0 1)
    1
  )
  (case? t)
  (default
    (inline 3)
  )
)

(fn-expr + word _
  (param w (? String) (= null))
  (case? w)
  (default
    (inline "none")
  )
)
```

```output
3
2
0
none
x
```

### An optional compares with null on either side, for equal and for not equal.

```brix
(main
  (let x (? Int) 5)
  (let y (? Int))
  (print (== x null))
  (print (== null y))
  (print (!= x null))
  (print (!= null y))
  (if (== y null)
    (print "y is null")
  )
  (let s (? String))
  (print (== s null))
  (= s "set")
  (print (!= s null))
)
```

```output
false
true
true
false
y is null
true
true
```

### A local that holds an optional known to hold a value is not equal to null.

```brix
(main
  (let n (? Int) 0)
  (= n 0)
  (print (== n null))
)
```

```output
false
```

### An optional local known to hold a value is unknown again after a branch that may assign it null.

```brix
(main
  (let flag true)
  (let x (? Int))
  (= x 5)
  (if flag
    (= x null)
  )
  (print x)
  (print (== x null))
)
```

```output
unknown
true
```

### A local assigned null under a branch is null where that is tested after the branch.

```brix
(main
  (print (call f 5))
  (print (call f 2))
)

(fn-do + f String
  (param x (? Int))
  (if (== x null)
    (return "null at start")
  )
  (if (> x 3)
    (= x null)
  )
  (if (== x null)
    (return "null now")
  )
  (return (str "still " x))
)
```

```output
null now
still 2
```

### An assignment of null under an else, a block, a loop or a nested branch ends what was known of the local after it.

```brix
(main
  (let x (? Int) 5)
  (if x
    (if (> x 30)
      (print "big")
      (else
        (= x null)
      )
    )
    (print x)
  )
  (let y (? Int) 5)
  (if y
    (do
      (= y null)
    )
    (print y)
  )
  (let z (? Int) 5)
  (while z
    (if (> z 3)
      (= z null)
    )
    (print z)
    (= z null)
  )
  (let a (? Int) 5)
  (let b (? Int) 6)
  (if (and a b)
    (if (> a 3)
      (= b null)
    )
    (print a)
    (print b)
  )
)
```

```output
unknown
unknown
unknown
5
unknown
```

### A branch that assigns null and leaves, and one that assigns a value, both leave the local known to hold one.

```brix
(main
  (print (call keep true))
  (print (call keep false))
)

(fn-do + keep Int
  (param flag Bool)
  (let x (? Int))
  (= x 5)
  (if flag
    (= x null)
    (return 0)
  )
  (let y (? Int))
  (= y 5)
  (if flag
    (= y 7)
  )
  (return (+ x y))
)
```

```output
0
10
```

### An optional struct assigned null under a nested branch is unknown after it.

```brix
(main
  (let p (? Point) (call Point 1 2))
  (if p
    (if (== (. p x) 1)
      (= p null)
    )
    (print p)
    (print (== p null))
  )
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
unknown
true
```

### An optional property assigned null under a branch or a block is unknown after it.

```brix
(main
  (let box (call Box))
  (call (. box run) true)
)

(type + Box
  (prop + held (? Int))
  (fn-do + run _
    (param flag Bool)
    (= held 5)
    (if flag
      (= held null)
    )
    (print held)
    (print (== held null))
    (= (. this held) 4)
    (if flag
      (do
        (= (. this held) null)
      )
    )
    (print (. this held))
  )
)
```

```output
unknown
true
unknown
```

### An operand read through `!` keeps what it held when an operand after it assigns the optional.

```brix
(prop + g (? Point) (= (call Point 1 2)))
(blank)

(main
  (let o (? Point) (call Point 1 2))
  (print (call take (!! o) (= o (call Point 8 9))))
  (let s (? String) "a")
  (print (call join (!! s) (= s "b")))
  (let n (? Int) 1)
  (print (call add (!! n) (= n 5)))
  (let t (? (tuple Int Int)) (tuple 1 2))
  (print (call pair (!! t) (= t (tuple 3 4))))
  (print (call take (!! g) (call change)))
  (print g)
  (let box (call Box))
  (print (call take (!! (. box p)) (call (. box change))))
  (print (call take (. box q) (call (. box change))))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(type + Box
  (prop + p (? Point) (= (call Point 1 2)))
  (prop + q _ (= (call Point 3 4)))
  (fn-do + change Point
    (= p (call Point 8 9))
    (= q (call Point 6 7))
    (return (call Point 5 5))
  )
)

(fn-do + change Point
  (= g (call Point 8 9))
  (return (call Point 5 5))
)

(fn-expr + take _
  (param p Point)
  (param q Point)
  (+ (* (. p x) 1000) (* (. p y) 100) (* (. q x) 10) (. q y))
)

(fn-expr + join _
  (param a String)
  (param b String)
  (++ a b)
)

(fn-expr + add _
  (param a Int)
  (param b Int)
  (+ (* a 10) b)
)

(fn-expr + pair _
  (param a (tuple Int Int))
  (param b (tuple Int Int))
  (+ (* ([] a 0) 1000) (* ([] a 1) 100) (* ([] b 0) 10) ([] b 1))
)
```

```output
1289
ab
15
1234
1255
(8, 9)
1255
6755
```

### The value of an optional that holds nothing is what its type has with nothing written: no bits set, and the empty text for a text.

```brix
(main
  (let s (? String))
  (print (str "[" (. s value) "] " (. s value length)))
  (let set (? String) "a")
  (= set null)
  (print (str "[" (. set value) "] " (. set value length)))
  (let given (call none))
  (print (str "[" (. given value) "] " (. given value length)))
  (let box (call Box))
  (print (str "[" (. box s value) "] " (. box s value length)))
  (print (. box t))
  (print (== ([] (. box u) 1) null))
  (print (. box q value))
  (let t (? (tuple Int String)))
  (print (. t value))
  (let named (? (tuple (: name String) (: age Int))))
  (print (. named value name length))
  (let n (? Int))
  (print (. n value))
)

(fn-do + none (? String)
  (return null)
)

(type + Box
  (prop + s (? String))
  (prop + t (tuple Int String))
  (prop + u (tuple Int (? Thing)))
  (prop + q (? (tuple Int String)))
)

(type + Thing
  (prop + n _ (= 1))
)
```

```output
[] 0
[] 0
[] 0
[] 0
(0, )
true
(0, )
(0, )
0
0
```

### An optional returned as a Bool is whether it holds a value, also through a lazy local read once.

```brix
(main
  (print (call has 3))
  (print (call has 0))
)

(fn-expr + has Bool
  (param n Int)
  found
  (where
    (lazy found (call find n))
  )
)

(fn-do + find (? Index)
  (param n Int)
  (if (> n 1)
    (return 5)
  )
  (return null)
)
```

```output
true
false
```

### `!` of an optional is true when it holds no value, whatever value it holds.

```brix
(main
  (let none (? Int))
  (let some (? Int) 0)
  (let flag (? Bool) false)
  (print (tuple (not none) (not some) (not flag)))
  (if (not none)
    (print "none is empty")
  )
  (if (not some)
    (print "some is empty")
    (else
      (print "some holds")
    )
  )
  (print (tuple (call count null) (call count 4)))
)

(fn-do + count Int
  (param given (? Int))
  (let first (? Index))
  (let other (? Index))
  (for (in round (..< 0 3))
    (if (not first)
      (if given
        (= first (+ round 10))
      )
      (else-if (not other)
        (= other round)
      )
    )
  )
  (if (not other)
    (return 99)
  )
  (return 1)
)
```

```output
(true, false, false)
none is empty
some holds
(99, 1)
```

### The value assigned through an optional chain is evaluated whatever the chain holds.

```brix
(main
  (let foo (? Foo))
  (= (?. foo bar) (call side 1))
  (print (?. foo bar))
  (= foo (as (call Foo) (? Foo)))
  (= (?. foo bar) (call side 2))
  (print (?. foo bar))
)

(fn-do + side Int
  (param n Int)
  (print (str "side " n))
  (return n)
)

(type + Foo
  (prop + bar Int)
)
```

```output
side 1
unknown
side 2
2
```

### A link written with no `?` after another link reads and stores through the one test.

```brix
(main
  (let foo (? Foo))
  (print (. (?. foo bar) bing))
  (= foo (as (call Foo) (? Foo)))
  (print (. (?. foo bar) bing))
  (= (. (?. foo bar) bing) 7)
  (print (. (?. foo bar) bing))
)

(type + Foo
  (prop + bar _ (= (call Bar)))
)

(type + Bar
  (prop + bing _ (= 100))
)
```

```output
unknown
100
7
```

### A link of an optional chain that gives nothing gives an optional that holds nothing.

```brix
(main
  (let win (? Win))
  (let a (call (?. win focus)))
  (print (== a null))
  (= win (as (call Win) (? Win)))
  (let b (call (?. win focus)))
  (print (== b null))
  (if (call (?. win focus))
    (print "made")
    (else
      (print "not made")
    )
  )
)

(type + Win
  (fn-do + focus _
    (print "focus")
  )
)
```

```output
true
focus
true
focus
not made
```

### An optional struct and an optional tuple are read through a link by name and by place.

```brix
(main
  (let p (? Point))
  (print (?. p x))
  (print (?[] p 1))
  (= p (as (call Point 3 4) (? Point)))
  (print (?. p x))
  (print (?[] p 1))
  (let t (? (tuple Int String)))
  (print (?[] t 0))
  (print (?[] t 1))
  (= t (as (tuple 5 "five") (? (tuple Int String))))
  (print (?[] t 0))
  (print (?[] t 1))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
unknown
unknown
3
4
unknown
unknown
5
five
```

### The receiver of a chain is evaluated once, and the arguments of a link only where it holds a value.

```brix
(main
  (print (?. (call make false) bar))
  (print (?. (call make true) bar))
  (print (call (?. (call make true) twice) (call side 2)))
  (print (call (?. (call make false) twice) (call side 3)))
  (print (?. (call make true) maybe))
  (print (?. (?. (call make true) other) bar))
  (print (. (?. (call make true) name) length))
  (print (. (?. (call make false) name) length))
  (call (?. (call make true) twice) (call side 4))
  (call (?. (call make false) twice) (call side 5))
)

(fn-do + side Int
  (param n Int)
  (print (str "side " n))
  (return n)
)

(fn-do + make (? Foo)
  (param really Bool)
  (print (str "make " really))
  (if really
    (return (call Foo))
  )
  (return null)
)

(type + Foo
  (prop + bar _ (= 42))
  (prop + maybe (? Int))
  (prop + other (? Foo))
  (prop + name _ (= "abc"))
  (fn-do + twice Int
    (param n Int)
    (return (* n 2))
  )
)
```

```output
make false
unknown
make true
42
make true
side 2
4
make false
unknown
make true
unknown
make true
unknown
make true
3
make false
unknown
make true
side 4
make false
```

### A value assigned through a chain is stored only where the chain holds a value.

```brix
(main
  (let foo (? Foo))
  (let none (? Int))
  (= (?. foo bar) (?? none 5))
  (print (?. foo bar))
  (= foo (as (call Foo) (? Foo)))
  (= (?. foo bar) (?? none 5))
  (print (?. foo bar))
)

(type + Foo
  (prop + bar _ (= 1))
)
```

```output
unknown
5
```

### A call written with no `?` after a link is made of what the link gives.

```brix
(main
  (let foo (? Foo))
  (print (call (. (?. foo bar) twice) 2))
  (= foo (as (call Foo) (? Foo)))
  (print (call (. (?. foo bar) twice) 2))
  (call (. (?. foo bar) poke))
)

(type + Foo
  (prop + bar _ (= (call Bar)))
)

(type + Bar
  (prop + bing _ (= 100))
  (fn-do + twice Int
    (param n Int)
    (return (* n bing))
  )
  (fn-do + poke _
    (print "poke")
  )
)
```

```output
unknown
200
poke
```

### An index written with no `?` after a link reads and stores what the link gives.

```brix
(main
  (let foo (? Foo))
  (print ([] (?. foo items) 0))
  (print (. (?. foo items) length))
  (= ([] (?. foo items) 0) 9)
  (= foo (as (call Foo) (? Foo)))
  (print ([] (?. foo items) 0))
  (print (. (?. foo items) length))
  (= ([] (?. foo items) 0) 9)
  (print ([] (?. foo items) 0))
)

(type + Foo
  (prop + items _ (= (list 1 2 3)))
)
```

```output
unknown
unknown
1
3
9
```

### A setter, a number converted, an optional and a field of a struct an object holds are assigned through a chain.

```brix
(main
  (let foo (? Foo))
  (= (?. foo count) 5)
  (print (?. foo count))
  (= foo (as (call Foo) (? Foo)))
  (= (?. foo count) 5)
  (print (?. foo count))
  (= (?. foo ratio) 2)
  (print (?. foo ratio))
  (= (?. foo maybe) 3)
  (print (?. foo maybe))
  (= (. (?. foo point) x) 8)
  (print (. (?. foo point) x))
  (print (. (?. foo point) y))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(type + Foo
  (prop + held _ (= 0))
  (prop + ratio Float (= 1))
  (prop + maybe (? Int))
  (prop + point _ (= (call Point 1 2)))
  (fn-expr + count Int :get
    held
  )
  (fn-do + count _ :set (param value Int)
    (print (str "set " value))
    (= held value)
  )
)
```

```output
unknown
set 5
5
2
3
8
2
```

### An assignment through a chain evaluates its value, then the chain, then the key.

```brix
(main
  (= (?. (call make true) bar) (call side 1))
  (= (?. (call make false) bar) (call side 2))
  (= (?. (?. (call make true) other) bar) (call side 3))
  (= (?[] (?. (call make true) items) (call side 4)) (call side 5))
  (let foo (call make true))
  (= (?. foo items) (list 1 2 3))
  (= (?[] (?. foo items) (call side 1)) (call side 6))
  (print (?[] (?. foo items) 1))
  (print (?[] (?. foo items) (?? (?. foo bar) 0)))
)

(fn-do + side Int
  (param n Int)
  (print (str "side " n))
  (return n)
)

(fn-do + make (? Foo)
  (param really Bool)
  (print (str "make " really))
  (if really
    (return (call Foo))
  )
  (return null)
)

(type + Foo
  (prop + bar _ (= 2))
  (prop + other (? Foo))
  (prop + items (? (list Int)))
)
```

```output
side 1
make true
side 2
make false
side 3
make true
side 5
make true
make true
side 6
side 1
6
3
```

### A field of an optional struct a local holds is assigned through a chain where it lies.

```brix
(main
  (let p (? Point))
  (= (?. p x) 5)
  (print (?. p x))
  (= p (as (call Point 1 2) (? Point)))
  (= (?. p x) 5)
  (print (?. p x))
  (print (?. p y))
  (let t (? (tuple Int Int)))
  (= t (as (tuple 1 2) (? (tuple Int Int))))
  (print (?[] t 0))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
unknown
5
2
1
```

### A property named alone is the receiver of a chain read and of one assigned.

```brix
(main
  (let box (call Box))
  (print (call (. box peek)))
  (call (. box fill))
  (print (call (. box peek)))
  (call (. box poke) 7)
  (print (call (. box peek)))
  (call (?. (. box inner) hello))
)

(type + Inner
  (prop + value _ (= 1))
  (fn-do + hello _
    (print (str "hello " value))
  )
)

(type + Box
  (prop + inner (? Inner))
  (fn-do + fill _
    (= inner (call Inner))
  )
  (fn-do + peek (? Int)
    (return (?. inner value))
  )
  (fn-do + poke _
    (param n Int)
    (= (?. inner value) n)
    (= (?. (. this inner) value) (+ n 1))
  )
)
```

```output
unknown
1
8
hello 8
```

### An assignment of an optional after `?` gives what the place then holds, as the type of the place.

```brix
(main
  (let x (? I32))
  (let y Float 1.5)
  (let a (=? y x))
  (print a)
  (print (type-of a))
  (= x 10)
  (let b (=? y x))
  (print b)
  (print (type-of b))
  (print y)
  (let n (? Int))
  (let c (+=? y n))
  (print c)
  (print (type-of c))
  (= n 2)
  (let d (+=? y n))
  (print d)
  (print (type-of d))
  (print y)
)
```

```output
1.5
[TypeInfo F64]
10
[TypeInfo I32]
10
10
[TypeInfo F64]
12
[TypeInfo F64]
12
```

### An assignment after `?` of what a call gives stores only where it holds a value.

```brix
(main
  (let x (? Int))
  (let y Float 1.5)
  (let a (=? y (call pick false)))
  (print a)
  (print (type-of a))
  (let b (=? y (call pick true)))
  (print b)
  (print (type-of b))
  (print y)
  (let c (+=? y (call pick false)))
  (print c)
  (print (type-of c))
  (let d (+=? y (call pick true)))
  (print d)
  (print (type-of d))
  (print y)
)

(fn-do + pick (? I32)
  (param really Bool)
  (print (str "pick " really))
  (if really
    (return 9)
  )
  (return null)
)
```

```output
pick false
1.5
[TypeInfo F64]
pick true
9
[TypeInfo F64]
9
pick false
9
[TypeInfo F64]
pick true
18
[TypeInfo F64]
18
```

### A property, a field, a global and a text are assigned after `?` only where the value holds one.

```brix
(main
  (let foo (call Foo))
  (let x (? Int))
  (=? (. foo bar) x)
  (print (. foo bar))
  (+=? (. foo bar) x)
  (print (. foo bar))
  (= x 4)
  (=? (. foo bar) x)
  (print (. foo bar))
  (+=? (. foo bar) (call pick true))
  (print (. foo bar))
  (+=? (. foo bar) (call pick false))
  (print (. foo bar))
  (=? (. foo ratio) (call pick true))
  (print (. foo ratio))
  (=? (. foo maybe) (call pick false))
  (print (. foo maybe))
  (=? (. foo maybe) (call pick true))
  (print (. foo maybe))
  (let p (call Point 1 2))
  (=? (. p x) (call pick false))
  (=? (. p y) (call pick true))
  (print (. p x))
  (print (. p y))
  (=? total (call pick false))
  (print total)
  (=? total (call pick true))
  (print total)
  (+=? total (call pick true))
  (print total)
  (let t "a")
  (=? t (call word false))
  (print t)
  (=? t (call word true))
  (print t)
  (+=? t (call word true))
  (print t)
  (+=? t (call word false))
  (print t)
)

(prop _ total _ (= 100))

(fn-do + pick (? I32)
  (param really Bool)
  (print (str "pick " really))
  (if really
    (return 9)
  )
  (return null)
)

(fn-do + word (? String)
  (param really Bool)
  (if really
    (return "w")
  )
  (return null)
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(type + Foo
  (prop + bar _ (= 1))
  (prop + ratio Float (= 0.5))
  (prop + maybe (? Int))
)
```

```output
1
1
4
pick true
13
pick false
13
pick true
9
pick false
unknown
pick true
9
pick false
pick true
1
9
pick false
100
pick true
9
pick true
18
a
w
ww
ww
```

### A list takes an item after `?` only where the item holds one, as from `catch?`.

```brix
(main
  (let found (call (of List Int)))
  (for (in n (list 1 -2 3))
    (+=? found (catch? (call doubled n)))
  )
  (print found)
)

(fn-do _ doubled Int (?! Error)
  (param n Int)
  (throw (call Error "negative") (if (< n 0)))
  (return (* n 2))
)
```

```output
2, 6
```

### An assignment after `?` in a loop and as a value.

```brix
(main
  (print (call run false))
  (print (call run true))
)

(fn-do + run Int
  (param really Bool)
  (let total 1)
  (let n 0)
  (while (< n 3)
    (+=? total (call pick (and really (!= n 1))))
    (+= n 1)
  )
  (let last (=? total (call pick really)))
  (return (+ last total))
)

(fn-do + pick (? Int)
  (param really Bool)
  (if really
    (return 9)
  )
  (return null)
)
```

```output
2
18
```

### A link written with no `?` after a link that gives an optional reads no property of it.

```brix crash
(main
  (let foo (? Foo))
  (print (. (?. foo bar) bing))
  (= foo (as (call Foo) (? Foo)))
  (print (. (?. foo bar) bing))
  (= (. (!! foo) bar) (call Bar))
  (print (. (?. foo bar) bing))
)

(type + Foo
  (prop + bar (? Bar))
)

(type + Bar
  (prop + bing _ (= 100))
)
```

```output
Exception: Property "bing" is read from Bar?, which may be null; unwrap it first with "(!! …)" or "(?. …)"
<no file:4>
      (print (. (?. foo bar) bing))
             ^^^^^^^^^^^^^^^^^^^^^
```

### An assigning operator updates a place after a link of an optional chain where the chain holds a value, and skips it where the chain holds none.

```brix
(main
  (let foo (? Foo))
  (+= (. (?. foo bar) bing) 1)
  (= foo (as (call Foo) (? Foo)))
  (+= (. (?. foo bar) bing) 1)
  (print (. (?. foo bar) bing))
)

(type + Foo
  (prop + bar _ (= (call Bar)))
)

(type + Bar
  (prop + bing _ (= 100))
)
```

```output
101
```

### An assigning operator updates an index after a link of an optional chain where the chain holds a value, and skips it where the chain holds none.

```brix
(main
  (let foo (? Foo))
  (+= ([] (?. foo items) 0) 1)
  (= foo (as (call Foo) (? Foo)))
  (+= ([] (?. foo items) 0) 1)
  (print ([] (?. foo items) 0))
)

(type + Foo
  (prop + items _ (= (list 1 2 3)))
)
```

```output
2
```

### An index written after `?` needs an optional to read from.

```brix crash
(main
  (let items (list 1 2 3))
  (print (?[] items 0))
)
```

```output
Exception: "items" is [I64], which is never null, so it needs no "?"; write "([] items 0)"
<no file:4>
      (print (?[] items 0))
             ^^^^^^^^^^^^^
```

### A property assigned after `?` needs an optional to read from.

```brix crash
(main
  (let foo Foo)
  (= (?. foo bar) 2)
  (print (. foo bar))
)

(type + Foo
  (prop + bar _ (= 1))
)
```

```output
Exception: "foo" is Foo, which is never null, so it needs no "?"; write "(. foo bar)"
<no file:4>
      (= (?. foo bar) 2)
         ^^^^^^^^^^^^
```

### An index assigned after `?` needs an optional to read from.

```brix crash
(main
  (let items (list 1 2 3))
  (= (?[] items 0) 5)
  (print ([] items 0))
)
```

```output
Exception: "items" is [I64], which is never null, so it needs no "?"; write "([] items 0)"
<no file:4>
      (= (?[] items 0) 5)
         ^^^^^^^^^^^^^
```

### Optional compared with a value by the == its class declares

```brix
(main
  (let a (? Foo) null)
  (let b (call Foo 1))
  (let c (? Foo) (call Foo 1))
  (let d (? Foo) (call Foo 2))
  (print (== c b))
  (print (== b c))
  (print (!= c b))
  (print (== d b))
  (print (!= b d))
  (print (== c d))
  (print (== c c))
  (print (!= c d))
)

(type + Foo
  (prop + x Int)
  (fn-do + This _
    (param x _)
  )
  (fn-expr + (infix this == (param other Foo)) _
    (== x (. other x))
  )
)
```

```output
true
true
false
false
true
false
true
true
```

### An optional that holds nothing is not equal to a value, whatever == its class declares

```brix
(main
  (let a (? Foo) null)
  (let b (call Foo 1))
  (print "start")
  (print (== a b))
)

(type + Foo
  (prop + x Int)
  (fn-do + This _
    (param x _)
  )
  (fn-expr + (infix this == (param other Foo)) _
    (== x (. other x))
  )
)
```

```output
start
false
```

### Two optionals that hold nothing are equal without the == their class declares

```brix
(main
  (let a (? Foo) null)
  (let n (? Foo) null)
  (print "start")
  (print (== a n))
)

(type + Foo
  (prop + x Int)
  (fn-do + This _
    (param x _)
  )
  (fn-expr + (infix this == (param other Foo)) _
    (== x (. other x))
  )
)
```

```output
start
true
```

### Optionals of a class compared as the objects they hold

```brix
(main
  (let box (call Box))
  (let other (call Box))
  (let a (? Box) null)
  (let n (? Box) null)
  (let p (? Box) box)
  (let q (? Box) other)
  (print (== a n))
  (print (== a box))
  (print (== box a))
  (print (== p box))
  (print (== box p))
  (print (== p q))
  (print (!= p q))
  (print (== p p))
  (print (!= a p))
  (print (!= p a))
)

(type + Box
  (prop + value Int (= 1))
)
```

```output
true
false
false
true
true
false
true
true
true
true
```

### Optionals of different numbers are compared as numbers and never both empty

```brix
(main
  (let a (? Int) null)
  (let b (? I8) null)
  (let c (? Int) 5)
  (let d (? I8) 5)
  (let f Float 5.0)
  (let u (? U64) 5)
  (let m (? Int) -1)
  (let w (? U64) 18446744073709551615)
  (print (== a b))
  (print (== c d))
  (print (== d c))
  (print (== a d))
  (print (== c f))
  (print (== f c))
  (print (== a f))
  (print (== c u))
  (print (== m w))
  (print (== w m))
  (print (!= c d))
  (print (!= a b))
)
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
false
false
false
true
```

### Optionals of text and of atoms compared

```brix
(main
  (let a (? String) null)
  (let n (? String) null)
  (let s (? String) "hi")
  (let t (? String) (++ "h" "i"))
  (let plain "hi")
  (print (== a n))
  (print (== a plain))
  (print (== plain a))
  (print (== s plain))
  (print (== plain s))
  (print (== s t))
  (print (!= s t))
  (print (!= a s))
  (print (== s "hi"))
  (print (== "ho" s))
  (let x (? Atom) (as "name" Atom))
  (let y (? Atom) (as "name" Atom))
  (let z (? Atom) null)
  (let k (as "name" Atom))
  (print (== x y))
  (print (!= x y))
  (print (== x z))
  (print (== z z))
  (print (== x k))
  (print (== k x))
  (print (== z k))
)
```

```output
true
false
false
true
true
true
false
true
true
false
true
false
false
true
true
true
false
```

### Optionals of tuples compared field by field

```brix
(main
  (let a (? (tuple Int Int)) null)
  (let n (? (tuple Int Int)) null)
  (let p (? (tuple Int Int)) (tuple 1 2))
  (let q (tuple 1 2))
  (print (== a n))
  (print (== a p))
  (print (== p q))
  (print (== q p))
  (print (!= p q))
  (print (== a q))
  (print (!= q a))
)
```

```output
true
false
true
true
false
false
true
```

### Optional tuple that holds nothing compared with a tuple that declares ==

```brix
(main
  (let absent (? Point2) null)
  (let present (call Point2 1 2))
  (print "start")
  (print (!= absent present))
  (print (== absent present))
  (print (!= present absent))
  (let other (? Point2) (call Point2 1 2))
  (print (!= other present))
  (print (== other present))
  (print (== present other))
)

(type + Point2 (is Tuple)
  (prop + x Int)
  (prop + y Int)
  (fn-expr + (infix this == (param other Point2)) _
    (and (== x (. other x)) (== y (. other y)))
  )
)
```

```output
start
true
false
true
false
true
true
```

### A comparison with an optional is false where it holds nothing, and otherwise calls == on what it holds: on the right one's where both are optional

```brix
(main
  (let a (? Foo) (call Foo 1))
  (let b (? Foo) (call Foo 2))
  (let c (call Foo 3))
  (let n (? Foo) null)
  (print (== a b))
  (print (== b a))
  (print (!= a b))
  (print (== a c))
  (print (== c a))
  (print (!= c a))
  (print (== n a))
  (print (== a n))
  (print (!= n n))
)

(type + Foo
  (prop + x Int)
  (fn-do + This _
    (param x _)
  )
  (fn-do + (infix this == (param other Foo)) _
    (print (str "eq " x " with " (. other x)))
    (return (== x (. other x)))
  )
)
```

```output
eq 2 with 1
false
eq 1 with 2
false
eq 2 with 1
true
eq 1 with 3
false
eq 1 with 3
false
eq 1 with 3
true
false
false
false
```

### An optional number is ordered against a number only once it is unwrapped

```brix compile-error
(main
  (let limit (? Int) 3)
  (let count 1)
  (print (< count limit))
)
```

```output
Exception: Operator "<" is not defined for I64 and I64?
<no file:5>
      (print (< count limit))
             ^^^^^^^^^^^^^^^
```

### Floats of two widths compare in the wider of the two, whether or not either is optional

```brix
(main
  (let x F64 0.1)
  (let y (? F32) 0.1)
  (let z (? F64) 0.1)
  (let w F32 0.1)
  (print (== x y))
  (print (== y x))
  (print (== z w))
  (print (== w z))
  (print (== y z))
  (print (== z y))
  (print (!= x y))
  (print (!= z w))
  (let i (? Int) 16777217)
  (let f F32 16777216.0)
  (print (== i f))
  (print (== f i))
  (let g (? F32) 16777216.0)
  (print (== i g))
  (print (== g i))
)
```

```output
false
false
false
false
false
false
true
true
true
true
true
true
```

### Optional comparison evaluates each operand once in order

```brix
(main
  (let p (call Pair 1 2))
  (print (== (call first) (call second)))
  (print (!= (call first) (call second)))
  (print (== (call second) (call first)))
  (print (== (call hold p) (call bump p)))
  (print (== (call hold p) (call hold p)))
  (let count 0)
  (let o (? Int) 1)
  (print (== o (call step)))
  (print (== (call step) o))
  (print (!= o (call step)))
)

(type + Pair (is Struct)
  (prop + a Int)
  (prop + b Int)
)

(fn-do + first (? Int)
  (print "first")
  (return 1)
)

(fn-do + second (? Int)
  (print "second")
  (return null)
)

(fn-do + hold (? Pair)
  (param pair Pair)
  (print "hold")
  (return pair)
)

(fn-do + bump (? Pair)
  (param pair Pair)
  (print "bump")
  (return (call Pair (+ (. pair a) 1) (. pair b)))
)

(fn-do + step Int
  (print "step")
  (return 1)
)
```

```output
first
second
false
first
second
true
second
first
false
hold
bump
false
hold
hold
true
step
true
step
true
step
false
```

### Optional enums and unions compared

```brix
(main
  (let a (? Level) null)
  (let h (? Level) (. Level high))
  (let l (. Level low))
  (print (== a h))
  (print (== h (. Level high)))
  (print (== (. Level low) h))
  (print (== a l))
  (print (!= a l))
  (print (!= h l))
  (print (== h h))
  (print (== a a))
  (let t (? Shape) (. Shape round))
  (let u (? Shape) null)
  (let v (. Shape square))
  (print (== t u))
  (print (== t v))
  (print (== t .round))
  (print (== .round t))
  (print (== u .round))
  (print (!= u .round))
  (print (!= t v))
  (print (== u u))
  (print (== t t))
)

(type + Level (is Int)
  (variant low (= 1))
  (variant high (= 2))
)

(type + Shape (is Union)
  (variant butt)
  (variant round)
  (variant square)
)
```

```output
false
true
false
false
true
true
true
true
false
false
true
true
false
true
true
true
true
```

### Optional comparison through a declared != and a declared ==

```brix
(main
  (let a (? Foo) (call Foo 1))
  (let b (? Foo) (call Foo 1))
  (let n (? Foo) null)
  (print (== a b))
  (print (!= a b))
  (print (== n a))
  (print (!= n a))
  (print (== n n))
  (print (!= n n))
  (let c (? Bar) (call Bar 1))
  (let d (? Bar) (call Bar 1))
  (let e (? Bar) (call Bar 2))
  (let m (? Bar) null)
  (print (== c d))
  (print (!= c d))
  (print (== c e))
  (print (!= c e))
  (print (!= m c))
  (print (== m m))
)

(type + Foo
  (prop + x Int)
  (fn-do + This _
    (param x _)
  )
  (fn-do + (infix this != (param other Foo)) _
    (print "ne")
    (return (!= x (. other x)))
  )
)

(type + Bar
  (prop + x Int)
  (fn-do + This _
    (param x _)
  )
  (fn-do + (infix this == (param other Bar)) _
    (print "bar eq")
    (return (== x (. other x)))
  )
  (fn-do + (infix this != (param other Bar)) _
    (print "bar ne")
    (return true)
  )
)
```

```output
ne
true
ne
false
false
true
true
false
bar eq
true
bar eq
false
bar eq
false
bar eq
true
true
true
```

### Optional comparison as a condition and of properties

```brix
(main
  (let a (? Int) 3)
  (let b (? Int) null)
  (let c (? Int) 3)
  (if (== a c)
    (print "a is c")
  )
  (if (== a b)
    (print "a is b")
    (else
      (print "a is not b")
    )
  )
  (if (and (!= a b) (== a c) (!= b c))
    (print "chain")
  )
  (let same (== a c))
  (let differ (!= a c))
  (print same)
  (print differ)
  (print (call check a c))
  (print (call check a b))
  (print (call check b b))
  (print (if-expr (== a c) 1 2))
  (while (!= a c)
    (print "never")
  )
  (let box (call Box))
  (print (== (. box value) a))
  (print (!= (. box value) (. box other)))
  (print (== (. box other) (. box other)))
  (= (. box other) 3)
  (print (== (. box other) a))
  (print (== (call (. box missing)) (. box value)))
  (print (== (call (. box missing)) (call (. box missing))))
)

(fn-expr + check String
  (param left (? Int))
  (param right (? Int))
  (if-expr (== left right) "same" "differ")
)

(type + Box
  (prop + value (? Int) (= 3))
  (prop + other (? Int) (= null))
  (fn-expr + missing (? Int)
    null
  )
)
```

```output
a is c
a is not b
chain
true
false
same
differ
same
1
true
true
true
true
false
true
```

### Optional structs and mixed optionals compared

```brix
(main
  (let a (? (tuple Int Int)) (tuple 1 2))
  (let b (? (tuple Int String)) (tuple 1 "2"))
  (let s (? Rec) (call Rec 1 2))
  (let t (? Rec) (call Rec 1 2))
  (let u (? Rec) null)
  (let r (call Rec 1 3))
  (print (== s t))
  (print (!= s t))
  (print (== s u))
  (print (== u u))
  (print (== s r))
  (print (== r s))
  (print (!= r u))
  (let x (? String) "a")
  (let k (? Atom) (as "a" Atom))
  (let i (? Int) 1)
  (let j (? I8) 1)
  (let e (? U8) null)
  (let o (? I8) null)
  (print (== i j))
  (print (== e o))
  (print (== j i))
  (print (!= o e))
)

(type + Rec (is Struct)
  (prop + a Int)
  (prop + b Int)
)
```

```output
true
false
false
true
false
false
true
true
false
true
true
```

### Optional compared by an == that takes an optional

```brix
(main
  (let a (? Foo) null)
  (let b (call Foo 1))
  (let c (? Foo) (call Foo 2))
  (print (== b a))
  (print (== a b))
  (print (!= a b))
  (print (== c b))
  (print (== b c))
  (print (== a c))
  (print (== c a))
  (print (== c c))
  (print (== a a))
)

(type + Foo
  (prop + x Int)
  (fn-do + This _
    (param x _)
  )
  (fn-do + (infix this == (param other (? Foo))) _
    (if other
      (print (str "eq " x " with " (. other x)))
      (return (== x (. other x)))
    )
    (print (str "eq " x " with nothing"))
    (return false)
  )
)
```

```output
eq 1 with nothing
false
eq 1 with nothing
false
eq 1 with nothing
true
eq 1 with 2
false
eq 1 with 2
false
false
false
eq 2 with 2
true
true
```

### Optional tuples compared by the == one declares for another

```brix
(main
  (let a (? Point) null)
  (let b (call Point 1 2))
  (let c (? Point) (call Point 1 2))
  (let d (? Other) (call Other 1 2))
  (let e (? Other) null)
  (print (== a b))
  (print (== b a))
  (print (== c b))
  (print (== c c))
  (print (== a a))
  (print (== c d))
  (print (== d c))
  (print (== a e))
  (print (== e a))
  (print (== a d))
  (print (!= c e))
  (print (== b d))
  (print (== d b))
  (print (== b e))
)

(type + Point (is Tuple)
  (prop + x Int)
  (prop + y Int)
  (fn-do + (infix this == (param other Point)) _
    (print "point eq")
    (return (and (== x (. other x)) (== y (. other y))))
  )
  (fn-do + (infix this == (param other Other)) _
    (print "point eq other")
    (return (and (== x (. other x)) (== y (. other y))))
  )
)

(type + Other (is Tuple)
  (prop + x Int)
  (prop + y Int)
)
```

```output
false
false
point eq
true
point eq
true
true
point eq other
true
point eq other
true
true
true
false
true
point eq other
true
point eq other
true
false
```

### An optional number compared with text has no answer.

```brix crash
(main
  (let a (? Int) null)
  (let c (? Int) 5)
  (print (== c "5"))
)
```

```output
Exception: Operator "==" is not defined for I64 and String
<no file:5>
      (print (== c "5"))
             ^^^^^^^^^^
```

### Equality pattern over an optional subject

```brix
(main
  (let a (? Int) 3)
  (let n (? Int) null)
  (print (call name a))
  (print (call name n))
  (print (call name 4))
  (print (call text "a"))
  (print (call text null))
  (print (call text "b"))
)

(fn-expr + name String
  (param value (? Int))
  (match value
    (case= 3
      "three"
    )
    (case= 4
      "four"
    )
    (default
      "other"
    )
  )
)

(fn-expr + text String
  (param value (? String))
  (match value
    (case= "a"
      "is a"
    )
    (default
      "other"
    )
  )
)
```

```output
three
other
four
is a
other
other
```

### Equality pattern of an optional over an optional subject

```brix
(main
  (let a (? Int) 3)
  (let n (? Int) null)
  (print (call pick a a))
  (print (call pick a n))
  (print (call pick n n))
  (print (call pick n a))
)

(fn-expr + pick String
  (param value (? Int))
  (param wanted (? Int))
  (match value
    (case= wanted
      "wanted"
    )
    (default
      "other"
    )
  )
)
```

```output
wanted
other
wanted
other
```

### A table of conditions with no else holds nothing where no arm is taken

```brix
(main
  (print (?? (call pick 1) "none"))
  (print (?? (call pick 2) "none"))
  (print (?? (call pick 3) "none"))
  (print (?? (call count true) -1))
  (print (?? (call count false) -1))
  (let kept (call pick 2))
  (print (!! kept))
  (print (type-of (call pick 1)))
)

(fn-expr + pick _
  (param n Int)
  (case? (== n 1)
    "one"
  )
  (case? (== n 2)
    "two"
  )
)

(fn-expr + count _
  (param on Bool)
  (case? on
    5
  )
)
```

```output
one
two
none
5
-1
two
[TypeInfo Optional'String]
```

### A table of conditions with no else gives an optional struct and an optional text

```brix
(main
  (print (. (!! (call where true)) y))
  (print (if-expr (not (call where false)) "none" "some"))
  (print (!! (call named true)))
  (print (?? (call named false) "anonymous"))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(fn-expr + where _
  (param on Bool)
  (case? on
    (call Point 1 2)
  )
)

(fn-expr + named (? String)
  (param on Bool)
  (case? on
    "name"
  )
)
```

```output
2
none
name
anonymous
```

### A local known to hold a value is tested by && for whether it holds one, not for what it holds

```brix
(main
  (let zero (? Int) 0)
  (let one (? Int) 1)
  (print (call present-after zero one))
  (print (call present-after zero null))
  (print (call present-after null one))
  (print (call chain zero))
  (print (call chain one))
  (print (call chain null))
  (print (call text-after "" "x"))
  (print (call text-after null "x"))
)

(fn-expr + present-after Bool
  (param first (? Int))
  (param second (? Int))
  (or (not first) (and first second))
)

(fn-expr + chain String
  (param first (? Int))
  (if-expr (and first first) "present" "absent")
)

(fn-expr + text-after Bool
  (param first (? String))
  (param second (? String))
  (or (not first) (and first second))
)
```

```output
true
false
true
present
present
absent
true
true
```

### Where a local known to hold zero is tested for holding a value and where for the value

```brix
(main
  (let zero (? Int) 0)
  (print (str "n1 " (call n1 zero)))
  (print (str "n2 " (call n2 zero)))
  (print (str "n3 " (call n3 zero)))
  (print (str "n4 " (call n4 zero)))
  (print (str "n5 " (call n5 zero)))
  (print (str "n6 " (call n6 zero)))
  (print (str "n7 " (call n7 zero)))
  (print (str "n8 " (call n8 zero)))
  (print (str "n9 " (call n9 zero)))
)

(fn-do + n1 String
  (param first (? Int))
  (if first
    (if first
      (return "held")
    )
    (return "lost")
  )
  (return "none")
)

(fn-do + n2 String
  (param first (? Int))
  (if (not first)
    (return "absent")
  )
  (if first
    (return "held")
  )
  (return "lost")
)

(fn-do + n3 String
  (param first (? Int))
  (if first
    (if (and first true)
      (return "held")
    )
    (return "lost")
  )
  (return "none")
)

(fn-do + n4 String
  (param first (? Int))
  (if first
    (if (not first)
      (return "denied")
    )
    (return "kept")
  )
  (return "none")
)

(fn-do + n5 String
  (param first (? Int))
  (if first
    (if (not (not first))
      (return "held")
    )
    (return "lost")
  )
  (return "none")
)

(fn-do + n6 String
  (param first (? Int))
  (if first
    (let turns 0)
    (while first
      (+= turns 1)
      (if (== turns 2)
        (break)
      )
    )
    (return (str "turns " turns))
  )
  (return "none")
)

(fn-do + n7 String
  (param first (? Int))
  (if (not first)
    (return "absent")
  )
  (let turns 0)
  (while first
    (+= turns 1)
    (if (== turns 2)
      (break)
    )
  )
  (return (str "turns " turns))
)

(fn-do + n8 String
  (param first (? Int))
  (if first
    (return-if "held" (if first))
    (return "lost")
  )
  (return "none")
)

(fn-do + n9 String
  (param first (? Int))
  (if first
    (let kept (or first false))
    (return (str "kept " kept))
  )
  (return "none")
)
```

```output
n1 lost
n2 lost
n3 held
n4 kept
n5 held
n6 turns 2
n7 turns 2
n8 held
n9 kept true
```

### Conditions that test what a local known to hold a value holds

```brix
(main
  (let zero (? Int) 0)
  (let five (? Int) 5)
  (print (str "n1 " (call n1 zero) " " (call n1 five)))
  (print (str "n2 " (call n2 zero) " " (call n2 five)))
  (print (str "w1 " (call w1 zero) " " (call w1 five)))
  (print (str "a1 " (call a1 zero)))
  (let raising
    (do-new
      (print (str "a1 " (call a1 five)))
      (on-failure failure
        (print (str "a1 " failure))
      )
    )
  )
  (call (. raising wait))
  (print (str "t1 " (call t1 zero) " " (call t1 five)))
  (print (str "t2 " (call t2 zero) " " (call t2 five)))
  (print (str "e1 " (call e1 zero) " " (call e1 five)))
  (print (str "q1 " (?? (call q1 zero) "none") " " (?? (call q1 five) "none")))
)

(fn-do + n1 String
  (param first (? Int))
  (if first
    (if first
      (return "held")
    )
    (return "lost")
  )
  (return "none")
)

(fn-do + n2 String
  (param first (? Int))
  (if (not first)
    (return "absent")
  )
  (if first
    (return "held")
  )
  (return "lost")
)

(fn-do + w1 String
  (param first (? Int))
  (if first
    (let wrapped (only-if "w" (if first)))
    (return (?? wrapped "unwrapped"))
  )
  (return "none")
)

(fn-do + a1 String
  (param first (? Int))
  (if first
    (throw (call Panic "raised") (if first))
    (return "passed")
  )
  (return "none")
)

(fn-do + t1 String
  (param first (? Int))
  (if (not first)
    (return "absent")
  )
  (case? first
    (return "held")
  )
  (return "lost")
)

(fn-expr + t2 String
  (param first (? Int))
  (case? (not first)
    "absent"
  )
  (case? first
    "held"
  )
  (default
    "lost"
  )
)

(fn-do + e1 String
  (param first (? Int))
  (if (not first)
    (return "absent")
    (else-if first
      (return "held")
    )
  )
  (return "lost")
)

(fn-do + q1 (? String)
  (param first (? Int))
  (if first
    (return (if-expr first null "value"))
  )
  (return "none")
)
```

```output
n1 lost held
n2 lost held
w1 unwrapped w
a1 passed
a1 raised
t1 lost held
t2 lost held
e1 lost held
q1 value none
```

### Operators that test whether a local known to hold a value holds one

```brix
(main
  (let zero (? Int) 0)
  (let five (? Int) 5)
  (print (str "f1 " (call f1 zero) " " (call f1 five)))
  (print (str "f2 " (call f2 zero) " " (call f2 five)))
  (print (str "f4 " (call f4 zero) " " (call f4 five)))
  (print (str "f5 " (call f5 zero) " " (call f5 five)))
  (print (str "f6 " (?? (call f6 zero) "none") " " (?? (call f6 five) "none")))
  (print (str "f7 " (?? (call f7 zero) "none") " " (?? (call f7 five) "none")))
  (print (str "m2 " (call m2 zero) " " (call m2 five)))
)

(fn-do + f1 String
  (param first (? Int))
  (if (not first)
    (return "absent")
  )
  (let denied (not first))
  (return (str "denied " denied))
)

(fn-do + f2 String
  (param first (? Int))
  (if (not first)
    (return "absent")
  )
  (let either (or first false))
  (return (str "either " either))
)

(fn-do + f4 String
  (param first (? Int))
  (if (not first)
    (return "absent")
  )
  (return (if-expr first "yes" "no"))
)

(fn-do + f5 String
  (param first (? Int))
  (if (not first)
    (return "absent")
  )
  (let pair (and first true))
  (return (str "pair " pair))
)

(fn-do + f6 (? String)
  (param first (? Int))
  (if (not first)
    (return "absent")
  )
  (return (only-if "w" (if first)))
)

(fn-do + f7 (? String)
  (param first (? Int))
  (if (not first)
    (return "absent")
  )
  (return (if-expr first null "value"))
)

(fn-do + m2 String
  (param first (? Int))
  (let seen "")
  (if (!= first null)
    (if first
      (= seen "truthy")
      (else
        (= seen "falsy")
      )
    )
  )
  (return seen)
)
```

```output
f1 denied false denied false
f2 either true either true
f4 no yes
f5 pair true pair true
f6 none w
f7 value none
m2 falsy truthy
```

### Arms that give null and a value under an optional the function declares, and a tuple held under other names

```brix
(main
  (print (?? (call find 0) -100))
  (print (?? (call find 20) -100))
  (print (?? (call find 5) -100))
  (print (?? (call look 0) -100))
  (print (?? (call look 20) -100))
  (print (?? (call look 5) -100))
  (print (. (!! (call named (call (. Kind pair) 3 "three"))) label))
  (print (. (!! (call named (call (. Kind pair) 3 "three"))) count))
  (print (if-expr (not (call named .empty)) "none" "some"))
  (print (. (!! (call loose (call (. Kind pair) 4 "four"))) label))
  (print (if-expr (not (call loose .empty)) "none" "some"))
)

(type + Kind (is Union)
  (variant empty)
  (variant pair (tuple (: count Int) (: label String)))
)

(fn-expr + find (? Int)
  (param n Int)
  (match n
    (case 0
      null
    )
    (case ({} (> it 10))
      it
    )
    (default
      -1
    )
  )
)

(fn-expr + look _
  (param n Int)
  (match n
    (case 0
      null
    )
    (case ({} (> it 10))
      it
    )
    (default
      -1
    )
  )
)

(fn-expr + named (? (tuple (: count Int) (: label String)))
  (param kind Kind)
  (match kind
    (case .empty
      null
    )
    (case (call .pair count label)
      (tuple count label)
    )
  )
)

(fn-expr + loose (? (tuple (: count Int) (: label String)))
  (param kind Kind)
  (match? kind
    (case (call .pair count label)
      (tuple count label)
    )
  )
)
```

```output
-100
20
-1
-100
20
-1
three
3
none
four
none
```

### An optional made with no value holds nothing

```brix
(main
  (let none (call (of Optional Int)))
  (print (?? none -1))
  (let some (call (of Optional Int) 4))
  (print (?? some -1))
  (let text (call (of Optional String)))
  (print (?? text "(no text)"))
  (let thing (call (of Optional Thing)))
  (print (if-expr thing (. thing n) 0))
  (let flags (list (call (of Optional Bool) true) (call (of Optional Bool)) (call (of Optional Bool) false)))
  (for (in flag flags)
    (print (if-expr (!= flag null) "set" "unset"))
  )
  (for (in value (list (call (of Optional Int) 1) (call (of Optional Int)) (call (of Optional Int) 3)) (when value))
    (print (+ value 1))
  )
  (print (?? (call absent) 9))
)

(type + Thing
  (prop + n _ (= 7))
)

(fn-expr + absent (? Int)
  (call (of Optional Int))
)
```

```output
-1
4
(no text)
0
set
unset
set
2
4
9
```

### use? of an optional the flow knows holds a value uses it.

```brix
(main
  (let t (? Thing) null)
  (let a (use? t))
  (print (== a null))
  (= t (call Thing 4))
  (let b (use? t))
  (print (. (!! b) n))
)

(type + Thing
  (prop + n _ (= 0))
  (fn-do + This _
    (param n _)
    (print (str "hello " n))
  )
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print (str "bye " n))
  )
)
```

```output
true
hello 4
4
bye 4
```

### A fallback on an optional already known to be present yields the optional's own value.

```brix
(main
  (print (call inline "-" "-"))
  (print (call inline "-" "+"))
  (print (call inline "" ""))
  (print (call bound "-" "-"))
  (print (call lazy "-" "-"))
  (print (call lazy "" ""))
  (print (call chosen "-"))
  (print (call chosen ""))
  (print (call passed "-"))
  (print (?? (call widened "-") "none"))
  (print (call counted 0))
  (print (call counted 4))
)

(fn-expr _ pick (? String)
  (param value String)
  value
)

(fn-expr _ same String
  (param value String)
  value
)

(fn-expr _ number (? Int)
  (param value Int)
  value
)

(fn-expr _ inline Bool
  (param value String)
  (param mark String)
  (and (!= raw null) (== (?? raw "fallback") mark))
  (where
    (lazy raw (call pick value))
  )
)

(fn-do _ bound Bool
  (param value String)
  (param mark String)
  (let raw (call pick value))
  (return (and (!= raw null) (== (?? raw "fallback") mark)))
)

(fn-expr _ lazy Bool
  (param value String)
  (param mark String)
  (and (!= raw null) (== text mark))
  (where
    (lazy raw (call pick value))
    (lazy text (?? raw "fallback"))
  )
)

(fn-do _ chosen String
  (param value String)
  (let raw (call pick value))
  (if (!= raw null)
    (return (?? raw "fallback"))
  )
  (return "null")
)

(fn-expr _ passed String
  (param value String)
  (if-expr (!= raw null) text "null")
  (where
    (lazy raw (call pick value))
    (lazy text (call same (?? raw "fallback")))
  )
)

(fn-do _ widened (? String)
  (param value String)
  (let raw (call pick value))
  (let other (? String) null)
  (if (!= raw null)
    (return (?? raw other))
  )
  (return null)
)

(fn-do _ counted Int
  (param value Int)
  (let raw (call number value))
  (if (!= raw null)
    (return (?? raw 9))
  )
  (return -1)
)
```

```output
true
false
true
true
true
true
-

-
-
0
4
```

### A local declared as an optional atom with no value holds none.

```brix
(import /core/atom-pool)
(blank)

(main
  (let pool (call AtomPool))
  (print (call pick pool (list "a" "task" "b")))
  (print (call pick pool (list "a")))
)

(fn-do + pick String
  (param pool AtomPool)
  (param names (list String))
  (let found (? Atom))
  (for (in name names)
    (if (== name "task")
      (= found (call (. pool store) name))
    )
  )
  (return (?? (?. found text) "none"))
)
```

```output
task
none
```

### An optional object is cast to a pointer to the object it holds.

```brix
(main
  (let u (call Thing))
  (let t (? Thing) u)
  (print (== (as t Pointer) (as u Pointer)))
)

(type + Thing)
```

```output
true
```

### A `|?` line with no body gives what its condition tested, which is evaluated again for it.

```brix
(main
  (let counter (call Counter))
  (print (?? (call kind counter "b") 9))
  (print (?? (call kind counter "z") 9))
  (print (. counter count))
)

(type + Counter
  (prop + count Index (= 0))
  (fn-do + same Bool
    (param left String)
    (param right String)
    (+= count 1)
    (return (== left right))
  )
)

(fn-expr + kind (? U8)
  (param counter Counter)
  (param name String)
  (case? (only-if 1 (if (== name "a"))))
  (case? (only-if 2 (if (call (. counter same) name "b"))))
  (default
    (inline null)
  )
)
```

```output
2
9
3
```

### An optional atom field of a tuple, once a test finds it holds a value, is read as the atom it holds, and still is after it is compared with an optional atom.

```brix
(import /core/atom-pool)
(blank)

(main
  (let pool (call AtomPool))
  (let box (? Atom) (call (. pool store) "box"))
  (let lid (? Atom) (call (. pool store) "lid"))
  (let a (call check (call Square (call (. pool store) "box")) box lid))
  (let b (call check (call Square null) box lid))
  (let c (call check (call Square (call (. pool store) "lid")) box lid))
  (let d (call check (call Square (call (. pool store) "cap")) box lid))
  (print (str a " " b " " c " " d))
)

(fn-expr + check Bool
  (param row Square)
  (param first (? Atom))
  (param second (? Atom))
  (case? (not (. row name))
    false
  )
  (default
    (or (== (. row name) first) (== (. row name) second))
  )
)

(type + Square (is Tuple)
  (prop + name (? Atom))
)
```

```output
true false true false
```

### `??` after a value that is always nothing gives the value after it.

```brix
(main
  (print (call pick null))
)

(fn-expr + pick Int
  (param value _)
  (?? value 5)
)
```

```output
5
```

### A choice of two values gives the value its condition chooses even when that value is nothing; the other value is not tried in its place.

```brix
(main
  (call show 7 (call get 3))
  (call show 7)
  (call show 9 (call get -1))
  (call show 9 (call get 4))
  (call both 5 (call get 4))
  (call both null (call get 4))
  (call both null (call get 3))
  (call both 6)
)

(fn-do + show _
  (param owner Int)
  (param variable (? Int) (= null))
  (let bound (if-expr variable (call lookup variable) owner))
  (print (str "bound " (?? bound -100)))
)

(fn-expr + get (? Int)
  (param n Int)
  (only-if n (if (> n 0)))
)

(fn-expr + lookup (? Int)
  (param n Int)
  (case? (and found (!= n 4))
    (* n 10)
  )
  (default
    null
  )
  (where
    (lazy found (> n 1))
  )
)

(fn-do + both _
  (param owner (? Int))
  (param variable (? Int) (= null))
  (let bound (if-expr variable (call lookup variable) owner))
  (print (str "both " (?? bound -100)))
)
```

```output
bound 30
bound 7
bound 9
bound -100
both -100
both -100
both 30
both 6
```

### `??` after a value written with `if` is refused, since it reads as a choice of two values without being one.

```brix crash
(main
  (let ready true)
  (print (?? (only-if 1 (if ready)) 2))
)
```

```output
Exception: `(?? (only-if value guard) other)` is not a choice of two values. To give one value or another, write `(if-expr condition value other)`.
<no file:4>
      (print (?? (only-if 1 (if ready)) 2))
             ^
```

### `??` after a value written with `if` is refused when parentheses hold the value.

```brix crash
(main
  (let ready true)
  (print (?? (only-if 1 (if ready)) 2))
)
```

```output
Exception: `(?? (only-if value guard) other)` is not a choice of two values. To give one value or another, write `(if-expr condition value other)`.
<no file:4>
      (print (?? (only-if 1 (if ready)) 2))
             ^
```

### `??` after a chain whose last value is written with `if` is refused.

```brix crash
(main
  (let ready true)
  (let held (? Int) null)
  (print (?? held (only-if 1 (if ready)) 2))
)
```

```output
Exception: `(?? (only-if value guard) other)` is not a choice of two values. To give one value or another, write `(if-expr condition value other)`.
<no file:5>
      (print (?? held (only-if 1 (if ready)) 2))
             ^
```

### Conditions alone with no else give an optional, which a function of a type that is no optional does not return.

```brix crash
(main
  (print (call name 2))
)

(fn-expr _ name String
  (param n Int)
  (case? (only-if "two" (if (== n 2))))
  (case? (only-if "three" (if (== n 3))))
)
```

```output
Exception: Expected String here, but this is String?, which may hold nothing; write "(!! (case? (only-if "two" (if (== n 2)))))" to unwrap it, or "(?? (case? (only-if "two" (if (== n 2)))) fallback)" to give a value for nothing
<no file:8>
      (case? (only-if "two" (if (== n 2))))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A getter a condition tests for a value is called again where it is read after the test, and `!` takes what that call answers.

```brix
(main
  (let rock (call Rock))
  (= (. rock held-by) (call Pit))
  (print (call (. rock describe)))
  (= (. rock held-by) (call Rock))
  (print (call (. rock describe)))
  (= (. rock held-by) null)
  (print (call (. rock describe)))
)

(type + Hole (is Interface)
  (fn-do + depth I32)
)

(type + Thing
  (prop + held-by (? Thing))
  (fn-do + holder (? Thing) :get
    (return held-by)
  )
)

(type + Rock (is Thing)
  (fn-do + describe String
    (if (and (!= holder null) (is (!! holder) Hole))
      (return (str "in a hole " (call (. (as (!! holder) Hole) depth))))
    )
    (return (if-expr (!= holder null) "held" "free"))
  )
)

(type + Pit (is Thing)
  (implements Hole)
  (fn-do + depth I32
    (return 3)
  )
)
```

```output
in a hole 3
held
free
```

```brix
(main
  (let holder (call Holder))
  (call (. holder add) 1)
  (= (. holder run) (call Run))
  (call (. holder add) 2)
  (call (. holder add) 3)
  (print (. (!! (. holder run)) reported length))
)

(type + Run
  (prop + reported (list Int))
)

(type + Holder
  (prop + run (? Run))
  (fn-do + add _
    (param value Int)
    (+= (?. run reported) value)
  )
)
```

```output
2
```

```brix
(main
  (let found (catch? (call load true)))
  (print (?? (?. found name) "none"))
  (let missing (catch? (call load false)))
  (print (?? (?. missing name) "none"))
  (print (== (catch? (call load false)) null))
  (let failed (catch? (call fail)))
  (print (?? (?. failed name) "failed"))
)

(type + Config
  (prop + name _ (= "config"))
)

(fn-do + load (? Config) (?! Error)
  (param exists Bool)
  (return-if null (if (not exists)))
  (return (call Config))
)

(fn-do + fail (? Config) (?! Error)
  (throw (call Error "missing"))
)
```

```output
config
none
true
failed
```

```brix
(main
  (let found
    (match? "[n]"
      (~ (: value Val)
        "matched"
      )
    )
  )
  (blank)
  (print (?? found "none"))
)

(type + Val (is Union)
  (variant Null)
  (variant (list Val))
  (fn-expr + This _ (over: Char)
    (~ \value)
    (where
      (lazy blank
        (pattern
          (~ "n"
            null
          )
        )
      )
      (lazy array
        (pattern
          (~ (seq "[" (: items (call \delimited (~ ",") (~ \value))) "]")
            items
          )
        )
      )
      (lazy value Val
        (pattern
          (~ (| \blank \array))
        )
      )
    )
  )
)
```

```output
matched
```

### A literal in an optional choice with null takes the declared optional type.

```brix
(main
  (let flag true)
  (let chosen (? I32) (if-expr flag 5 null))
  (print (?? chosen 0))
)
```

```output
5
```
