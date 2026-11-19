# Inlining

```brix
(main
  (let nodes (list Node) (list (call Row) (call Node) (call Row)))
  (for (in row (call selected nodes))
    (print (. row enabled))
  )
)

(type + Node)

(type + Row (is Node)
  (prop + enabled _ (= true))
)

(fn-expr + selected _ @inline
  (param nodes (list Node))
  (spread nodes)
  (pipe ?*> (call select-row each))
)

(fn-do + select-row _ @inline
  (param node Node)
  (if (is node Row)
    (let row (as node Row))
    (if (. row enabled)
      (return row)
    )
  )
)
```

```output
true
true
```

```brix warn
(import /core)

(main
  (print (call caller 4))
)

(fn-do + leaf Int @inline
  (param value Int)
  (let total 0)
  (for (in index (..< 0 value))
    (if (== index 2)
      (continue)
    )
    (+= total index)
  )
  (if (> total 1)
    (return total)
  )
  (return value)
)

(fn-expr + caller Int
  (param value Int)
  (+ (call leaf value) (call leaf (+ value 1)))
)
```

```stderr
```

```output
12
```

```brix
(main
  (print (call selected-property (call Value 4) (call Value 9) true))
  (print (call selected-property (call Value 4) (call Value 9) false))
)

(type + Value
  (prop + number Int)
  (fn-do + This _
    (param number Int)
  )
)

(fn-expr + invoke Int @inline
  (param callback (of Mapper Int Int))
  (param value Int)
  (call callback value)
)

(fn-do + choose-value Value @inline
  (param first Value)
  (param second Value)
  (param choose Bool)
  (if choose
    (return second)
  )
  (return first)
)

(fn-do + selected-property Int
  (param first Value)
  (param second Value)
  (param choose Bool)
  (let selected (call choose-value first second choose))
  (let offset 2)
  (let callback (of Mapper Int Int) (=> (value) (+ value offset)))
  (return (call invoke callback (. selected number)))
)
```

```output
11
6
```

```brix
(main
  (print (call selected-property (call Value 4) (call Value 9) true))
  (print (call selected-property (call Value 4) (call Value 9) false))
)

(type + Value
  (prop + number Int)
  (fn-do + This _
    (param number Int)
  )
)

(fn-expr + invoke Int
  (param callback (of Mapper Int Int))
  (param value Int)
  (call callback value)
)

(fn-do + choose-value Value
  (param first Value)
  (param second Value)
  (param choose Bool)
  (if choose
    (return second)
  )
  (return first)
)

(fn-do + selected-property Int
  (param first Value)
  (param second Value)
  (param choose Bool)
  (let selected (call choose-value first second choose))
  (let offset 2)
  (let callback (of Mapper Int Int) (=> (value) (+ value offset)))
  (return (call invoke callback (. selected number)))
)
```

```output
11
6
```

```brix warn
(import /core)

(main
  (print (catch (call outer true)))
  (print (catch (call outer false)))
  (call panic-leaf)
  (print "after")
)

(type + Failure (is Error))

(fn-do + leaf Int (?! Failure) @inline
  (param succeeds Bool)
  (finally
    (print "leaf cleanup")
  )
  (if succeeds
    (return 7)
  )
  (throw (call Failure "failed"))
)

(fn-do + outer Int (?! Failure) @inline
  (param succeeds Bool)
  (finally
    (print "outer cleanup")
  )
  (return (+ (call leaf succeeds) 1))
)

(fn-do + panic-leaf _ @inline
  (let attempt
    (do-new
      (throw (call Panic "panic"))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```stderr
```

```output
leaf cleanup
outer cleanup
8
leaf cleanup
outer cleanup
failed
panic
after
```

```brix warn
(import /core)
(blank)

(main
  (print (call combine (call tick 1) (call tick 2)))
  (print (call combine (call tick 3)))
  (print state)
)

(blank)
(prop _ state _ (= 0))

(fn-do + tick Int
  (param value Int)
  (= state (+ (* state 10) value))
  (return value)
)

(fn-expr + combine Int @inline
  (param value I64)
  (param unused Int (= (call tick 9)))
  (as (+ value value) Int)
)
```

```stderr
```

```output
2
6
1239
```

```brix warn
(import /core)
(blank)

(main
  (print (call inline-identity 42))
  (print (call inline-identity "text"))
)

(fn-expr + (of inline-identity T) T @inline
  (param value T)
  value
)
```

```stderr
```

```output
42
text
```

```brix warn
(import /core)
(blank)

(main
  (let counter (call Counter))
  (print (call (. counter bump) 4))
  (print (call (. counter bump) 3))
  (print (. counter value))
)

(type + Counter
  (prop + value Int (= 0))
  (fn-do + bump Int @inline
    (param amount Int)
    (+= value amount)
    (return value)
  )
)
```

```stderr
```

```output
4
7
7
```

```brix warn
(import /core)
(blank)

(main
  (print (call lazy-value 5))
)

(fn-expr + lazy-value Int @inline
  (param value Int)
  (scope (+ twice twice)
    (lazy twice (* value 2))
  )
)
```

```stderr
```

```output
20
```

```brix warn
(import /core)
(blank)

(main
  (let sum 0)
  (for (in index (..< 0 3))
    (+= sum (+ (call fresh index) (call fresh (+ index 1))))
  )
  (print sum)
)

(fn-do + fresh Int @inline
  (param value Int)
  (let local value)
  (for (in index (..< 0 4))
    (if (== index 1)
      (continue)
    )
    (if (== index 3)
      (break)
    )
    (+= local index)
  )
  (if (< local 3)
    (return (+ local 10))
  )
  (return local)
)
```

```stderr
```

```output
31
```

```brix warn
(import ./inline-support)
(import /core)
(blank)

(main
  (print (call cross-module 8))
  (print (call cross-module-ordinary 8))
)
```

```brix file=inline-support.bx
(fn-expr + cross-module Int @inline
  (param value Int)
  (+ value 3)
)

(fn-expr + cross-module-ordinary Int
  (param value Int)
  (+ value 3)
)
```

```stderr
```

```output
11
11
```

```brix warn
(import /BxEvents/BxGCHandshake.h)
(import /BxGC/BxGC.h)
(import /core)
(blank)

(main
  (print (call managed "held"))
)

(fn-do + managed String @inline
  (param seed String)
  (let live (str seed " value"))
  (call C:brixGCSafepoint)
  (call C:brixCollect)
  (call allocate)
  (return live)
)

(fn-do + allocate _
  (print (str "allocation " (. (call Counter) value)))
  (call C:brixCollect)
)

(type + Counter
  (prop + value Int (= 0))
  (fn-do + bump Int @inline
    (param amount Int)
    (+= value amount)
    (return value)
  )
)
```

```stderr
```

```output
allocation 0
held value
```

```brix
(import /core)

(main
  (print (call recurse 3))
  (print (call aggregate (tuple 4 5)))
  (for (in value (call inline-numbers))
    (print value)
  )
)

(fn-expr + recurse Int @inline
  (param value Int)
  (match value
    (case 0
      0
    )
    (default
      (+ (call recurse (- value 1)) 1)
    )
  )
)

(fn-expr + aggregate Int @inline
  (param pair (tuple Int Int))
  ([] pair 0)
)

(fn-do + inline-numbers _ @inline
  (yield 7)
)

(fn-expr + unused Int @inline
  42
)
```

```output
3
4
7
```

```brix
(import /core)

(main
  (print (call dispatch (call Child)))
)

(type + Base
  (fn-expr + value Int @inline
    1
  )
)

(type + Child (is Base)
  (fn-expr + value Int
    2
  )
)

(fn-expr + dispatch Int
  (param object Base)
  (call (. object value))
)
```

```output
2
```

```brix
(import /core)

(main
  (let fn bound-leaf)
  (print (call fn 4))
  (print (call bound-leaf 7))
)

(fn-expr + bound-leaf Int @inline
  (param value Int)
  (+ value 1)
)
```

```output
5
8
```

```brix
(import /core)
(blank)

(main
  (print (call even 4))
  (print (call even 3))
)

(fn-do + even Bool @inline
  (param value Int)
  (if (== value 0)
    (return true)
  )
  (return (call odd (- value 1)))
)

(fn-do + odd Bool @inline
  (param value Int)
  (if (== value 0)
    (return false)
  )
  (return (call even (- value 1)))
)
```

```output
true
false
```

```brix warn
(import ./inline-panic)
(import /core)
(blank)

(main
  (call attempt 0)
  (call attempt 1)
  (call attempt 2)
)

(fn-do + attempt _
  (param which Int)
  (let task
    (do-new
      (call choose which)
      (on-failure failure
        (print (?. (. failure source) line))
      )
    )
  )
  (call (. task wait))
)
```

```brix file=inline-panic.bx
(fn-do + choose _ @inline
  (param which Int)
  (if (== which 0)
    (let missing (? Int))
    (print (!! missing))
    (else-if (== which 1)
      (let missing (? Int))
      (print (!! missing))
    )
    (else
      (throw (call Panic "existing"))
    )
  )
)
```

```stderr
```

```output
5
8
11
```

```brix warn
(import /core)

(main
  (print (call batch-caller 3))
)

(fn-expr + batch-leaf Int @inline
  (param value Int)
  (+ value 1)
)

(fn-expr + batch-caller Int
  (param value Int)
  (+ (call batch-leaf value) (call batch-leaf (+ value 1)))
)
```

```stderr
```

```output
9
```

```brix
(import /core)

(main
  (print (call increment 4))
)

(fn-expr + increment Int @inline (without gc)
  (param value Int)
  (+ value 1)
)
```

```output
5
```

```brix warn
(import /core)
(blank)

(main
  (print (call link-0 1))
)

(fn-expr + link-0 Int @inline
  (param value Int)
  (+ (call link-1 value) 1)
)

(fn-expr + link-1 Int @inline
  (param value Int)
  (+ (call link-2 value) 1)
)

(fn-expr + link-2 Int @inline
  (param value Int)
  (+ (call link-3 value) 1)
)

(fn-expr + link-3 Int @inline
  (param value Int)
  (+ (call link-4 value) 1)
)

(fn-expr + link-4 Int @inline
  (param value Int)
  (+ (call link-5 value) 1)
)

(fn-expr + link-5 Int @inline
  (param value Int)
  (+ (call link-6 value) 1)
)

(fn-expr + link-6 Int @inline
  (param value Int)
  (+ (call link-7 value) 1)
)

(fn-expr + link-7 Int @inline
  (param value Int)
  (+ (call link-8 value) 1)
)

(fn-expr + link-8 Int @inline
  (param value Int)
  (+ (call link-9 value) 1)
)

(fn-expr + link-9 Int @inline
  (param value Int)
  (+ (call link-10 value) 1)
)

(fn-expr + link-10 Int @inline
  (param value Int)
  (+ (call link-11 value) 1)
)

(fn-expr + link-11 Int @inline
  (param value Int)
  (+ value 1)
)
```

```stderr
```

```output
13
```

```brix warn
(import /core)
(blank)

(main
  (print (call large 1))
)

(fn-do + large Int @inline
  (param value Int)
  (let total value)
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (+= total (+ value value value value value value value value))
  (return total)
)
```

```stderr
```

```output
321
```

```brix warn
(import /core)
(blank)

(main
  (print (call tuple-sum (tuple 3 4)))
  (let point (call InlinePoint 3.0 4.0))
  (print (call (. point dot) (call InlinePoint 2.0 5.0)))
  (print (call (. (call (. point shift) 1.0) dot) point))
)

(fn-expr + tuple-sum Int @inline
  (param pair (tuple Int Int))
  (+ ([] pair 0) ([] pair 1))
)

(type + InlinePoint (is Tuple)
  (prop + x F64)
  (prop + y F64)
  (fn-expr + dot F64 @inline
    (param other InlinePoint)
    (+ (* x (. other x)) (* y (. other y)))
  )
  (fn-expr + shift InlinePoint @inline
    (param amount F64)
    (call InlinePoint (+ x amount) (+ y amount))
  )
)
```

```stderr
```

```output
7
26
32
```

```brix warn
(import /core)
(blank)

(main
  (let value (call InlineWide 1 2 3 4 5 6))
  (print (call wide-sum value))
  (print (call wide-forward value))
  (print (call wide-sum (call wide-choice value true)))
  (print (call wide-sum (call wide-choice value false)))
  (print (call wide-sum (call inline-tuple-identity value)))
  (print (call nested-sum (call InlineNested value (tuple 7 8))))
)

(type + InlineWide (is (tuple (: a I64) (: b I64) (: c I64) (: d I64) (: e I64) (: f I64))))

(type + InlineNested (is (tuple (: wide InlineWide) (: pair (tuple I64 I64)))))

(fn-expr + wide-sum I64 @inline
  (param value InlineWide)
  (+ (. value a) (. value b) (. value c) (. value d) (. value e) (. value f))
)

(fn-expr + wide-forward I64 @inline
  (param value InlineWide)
  (+ (call wide-sum value) (call wide-sum value))
)

(fn-expr + wide-choice InlineWide @inline
  (param value InlineWide)
  (param original Bool)
  (case? original
    value
  )
  (default
    (call InlineWide 6 5 4 3 2 10)
  )
)

(fn-expr + (of inline-tuple-identity T) T @inline
  (param value T)
  value
)

(fn-expr + nested-sum I64 @inline
  (param value InlineNested)
  (+ (call wide-sum (. value wide)) ([] (. value pair) 0) ([] (. value pair) 1))
)
```

```stderr
```

```output
21
42
21
30
21
36
```

```brix warn
(import /BxGC/BxGC.h)
(import /core)
(blank)

(main
  (let payload (call InlinePayload "first" "second" (call InlineBox 7)))
  (print (call keep-payload payload))
  (print (. payload box value))
)

(type + InlineBox
  (prop + value Int)
  (fn-do + This _
    (param value Int)
  )
)

(type + InlinePayload (is (tuple (: first String) (: second String) (: box InlineBox))))

(fn-do + keep-payload String @inline
  (param value InlinePayload)
  (let retained value)
  (call C:brixCollect)
  (+= (. retained box value) 1)
  (call C:brixCollect)
  (return (str (. retained first) " " (. retained second) " " (. retained box value)))
)
```

```stderr
```

```output
first second 8
8
```

```brix warn
(import /core)
(blank)

(main
  (print (call use-tuple-once (call make-tuple)))
  (print tuple-evaluations)
)

(blank)
(prop _ tuple-evaluations _ (= 0))

(fn-do + make-tuple (tuple Int Int)
  (+= tuple-evaluations 1)
  (return (tuple 3 4))
)

(fn-expr + use-tuple-once Int @inline
  (param value (tuple Int Int))
  (+ ([] value 0) ([] value 0) ([] value 1) ([] value 1))
)
```

```stderr
```

```output
14
1
```

```brix warn
(import /core)
(blank)

(main
  (let small (call InlineMutablePair 3 4))
  (let wide (call InlineMutableWide 1 2 3 4))
  (print (call update-pair small))
  (print (. small a))
  (print (call update-wide wide))
  (print (. wide a))
)

(type + InlineMutablePair (is Struct)
  (prop + a Int)
  (prop + b Int)
)

(type + InlineMutableWide (is Struct)
  (prop + a I64)
  (prop + b I64)
  (prop + c I64)
  (prop + d I64)
)

(fn-expr + update-pair Int @inline
  (param value InlineMutablePair)
  (before
    (do
      (+= (. value a) 10)
    )
  )
  (+ (. value a) (. value b))
)

(fn-expr + update-wide I64 @inline
  (param value InlineMutableWide)
  (before
    (do
      (+= (. value a) 10)
    )
  )
  (+ (. value a) (. value b) (. value c) (. value d))
)
```

```stderr
```

```output
17
3
20
1
```

```brix warn
(import /core)
(blank)

(main
  (let missing (? InlineMaybePoint))
  (print (call optional-point-sum (call InlineMaybePoint 3 4)))
  (print (call optional-point-sum missing))
  (print (catch (call checked-point-sum (call InlineMaybePoint 3 4) true)))
  (print (catch (call checked-point-sum (call InlineMaybePoint 3 4) false)))
)

(type + InlineMaybePoint (is (tuple (: x I64) (: y I64))))

(type + InlinePointError (is Error))

(fn-expr + optional-point-sum I64 @inline
  (param value (? InlineMaybePoint))
  (case? value
    (+ (. value x) (. value y))
  )
  (default
    -1
  )
)

(fn-do + checked-point-sum I64 (?! InlinePointError) @inline
  (param value InlineMaybePoint)
  (param succeeds Bool)
  (finally
    (print (str "cleanup " (. value x)))
  )
  (if succeeds
    (return (+ (. value x) (. value y)))
  )
  (throw (call InlinePointError "failed"))
)
```

```stderr
```

```output
7
-1
cleanup 3
7
cleanup 3
failed
```

```brix warn
(import ./inline-tuple-module)
(import /core)
(blank)

(main
  (let point (call ModulePoint 3.0 4.0))
  (print (call module-point-dot point point))
)
```

```brix file=inline-tuple-module.bx
(type + ModulePoint (is (tuple (: x F64) (: y F64))))

(fn-expr + module-point-dot F64 @inline
  (param left ModulePoint)
  (param right ModulePoint)
  (+ (* (. left x) (. right x)) (* (. left y) (. right y)))
)
```

```stderr
```

```output
25
```

```brix warn
(import /core)
(blank)

(main
  (let original (call ReboundTuple 1 2 3 4))
  (print (call rebind-tuple original))
  (print (. original a))
)

(type + ReboundTuple (is (tuple (: a I64) (: b I64) (: c I64) (: d I64))))

(fn-do + rebind-tuple I64 @inline
  (param value ReboundTuple)
  (= value (call ReboundTuple 9 8 7 6))
  (return (+ (. value a) (. value b)))
)
```

```stderr
```

```output
17
1
```

```brix warn
(import /core)
(blank)
(prop _ source _ (= (call SnapshotTuple 1 2 3 4)))

(main
  (print (call snapshot-tuple source))
  (print (. source a))
)

(type + SnapshotTuple (is (tuple (: a I64) (: b I64) (: c I64) (: d I64))))

(fn-do + snapshot-tuple I64 @inline
  (param value SnapshotTuple)
  (call replace-source)
  (return (. value a))
)

(fn-do + replace-source _
  (= source (call SnapshotTuple 9 8 7 6))
)
```

```stderr
```

```output
1
9
```

```brix warn
(import /core)
(blank)

(main
  (let value (call HugeInlineTuple 1 2 3 4 5 6 7 8 9 10))
  (print (call huge-inline-sum value))
  (print (call branch-inline-tuple value true))
  (print (call branch-inline-tuple value false))
  (print (call loop-inline-tuple value))
  (print (. value a))
)

(type + HugeInlineTuple (is (tuple (: a I64) (: b I64) (: c I64) (: d I64) (: e I64) (: f I64) (: g I64) (: h I64) (: i I64) (: j I64))))

(fn-expr + huge-inline-sum I64 @inline
  (param value HugeInlineTuple)
  (+ (. value a) (. value b) (. value c) (. value d) (. value e) (. value f) (. value g) (. value h) (. value i) (. value j))
)

(fn-do + branch-inline-tuple I64 @inline
  (param value HugeInlineTuple)
  (param replace Bool)
  (if replace
    (= value (call HugeInlineTuple 10 9 8 7 6 5 4 3 2 1))
  )
  (return (. value a))
)

(fn-do + loop-inline-tuple I64 @inline
  (param value HugeInlineTuple)
  (for (in index (..< 0 3))
    (= value (call HugeInlineTuple (+ (. value a) 1) (. value b) (. value c) (. value d) (. value e) (. value f) (. value g) (. value h) (. value i) (. value j)))
  )
  (return (. value a))
)
```

```stderr
```

```output
55
10
1
4
1
```

```brix
(import /core)
(blank)
(prop _ source _ (= (call SnapshotTuple 1 2 3 4)))

(main
  (print (call snapshot-tuple source))
  (print (. source a))
)

(type + SnapshotTuple (is (tuple (: a I64) (: b I64) (: c I64) (: d I64))))

(fn-do + snapshot-tuple I64
  (param value SnapshotTuple)
  (call replace-source)
  (return (. value a))
)

(fn-do + replace-source _
  (= source (call SnapshotTuple 9 8 7 6))
)
```

```output
1
9
```

```brix
(import /core)

(main
  (print (call closure-inline 4))
)

(fn-expr + twice Int @inline
  (param callback (of Mapper Int Int))
  (param value Int)
  (+ (call callback value) (call callback (+ value 1)))
)

(fn-do + closure-inline Int
  (param value Int)
  (let offset 3)
  (let callback (of Mapper Int Int) (=> (item) (+ item offset)))
  (let first (call twice callback value))
  (= offset 10)
  (return (+ first (call callback value)))
)
```

```output
29
```

```brix
(import /core)

(main
  (print (call closure-escape 3))
)

(fn-expr + invoke Int @inline
  (param callback (of Mapper Int Int))
  (param value Int)
  (call callback value)
)

(fn-expr + ordinary-invoke Int
  (param callback (of Mapper Int Int))
  (param value Int)
  (call callback value)
)

(fn-do + closure-escape Int
  (param value Int)
  (let count 1)
  (let callback (of Mapper Int Int)
    (=> (amount) :do
      (block
        (+= count amount)
        (return count)
      )
    )
  )
  (let alias callback)
  (let first (call invoke callback value))
  (let second (call ordinary-invoke alias 5))
  (let third (call invoke callback 2))
  (print (== callback alias))
  (return (+ (* first 100) (* second 10) third))
)
```

```output
true
501
```

```brix
(import /core)

(main
  (print (call closure-join true 3))
  (print (call closure-join false 3))
)

(fn-expr + invoke Int @inline
  (param callback (of Mapper Int Int))
  (param value Int)
  (call callback value)
)

(fn-do + closure-join Int
  (param first Bool)
  (param value Int)
  (let offset 4)
  (let callback (of Mapper Int Int) (=> (item) (+ item offset)))
  (if first
    (= callback (=> (item) (* item offset)))
  )
  (let result (call invoke callback value))
  (= callback (=> (item) (- item offset)))
  (return (+ result (call invoke callback 10)))
)
```

```output
18
13
```

```brix
(import /core)

(main
  (print (call closure-nested 3))
)

(fn-expr + invoke Int @inline
  (param callback (of Mapper Int Int))
  (param value Int)
  (call callback value)
)

(fn-expr + forward (of Mapper Int Int) @inline
  (param callback (of Mapper Int Int))
  callback
)

(fn-expr + make-adder (of Mapper Int Int) @inline
  (param offset Int)
  (=> (value) (+ value offset))
)

(fn-do + closure-nested Int
  (param value Int)
  (let first (call make-adder 2))
  (let second (call make-adder 5))
  (let nested (of Mapper Int Int) (=> (item) (+ (call invoke (call forward first) item) (call invoke second item))))
  (return (call invoke nested value))
)
```

```output
13
```

```brix
(import /core)

(main
  (print (call closure-loop 4))
)

(fn-do + consume Int @inline
  (param callback (of Mapper Int Int))
  (param count Int)
  (let total 0)
  (for (in index (..< 0 count))
    (+= total (call callback index))
  )
  (return total)
)

(fn-do + closure-loop Int
  (param count Int)
  (let calls 0)
  (let callback (of Mapper Int Int)
    (=> (item) :do
      (block
        (+= calls 1)
        (return (+ item calls))
      )
    )
  )
  (let zero (call consume callback 0))
  (let result (call consume callback count))
  (return (+ zero result calls))
)
```

```output
20
```

```brix
(import /core)
(import /BxGC/BxGC.h)

(main
  (print (call closure-managed "capture"))
)

(type + Payload (is (tuple (: name String) (: a Int) (: b Int) (: c Int) (: d Int))))

(fn-expr + apply-payload Payload @inline
  (param callback (of Mapper Payload Payload))
  (param value Payload)
  (call callback value)
)

(fn-do + closure-managed String
  (param text String)
  (let capture (call Payload text 1 2 3 4))
  (let callback (of Mapper Payload Payload)
    (=> (value) :do
      (block
        (call C:brixGCCollect)
        (return (call Payload (+ (. capture name) (. value name)) (+ (. capture a) (. value a)) (. value b) (. value c) (. value d)))
      )
    )
  )
  (let result (call apply-payload callback (call Payload "-alive" 5 6 7 8)))
  (return (+ (. result name) (str "-" (. result a))))
)
```

```output
capture-alive-6
```

```brix
(import /core)

(main
  (print (call closure-each 3))
  (let iterator (call closure-generator 2))
  (print (call iterator))
  (print (call iterator))
)

(fn-expr + apply-each Int @inline
  (param value Int)
  (param callback _)
  (call callback value)
)

(fn-expr + closure-each Int
  (param value Int)
  (call apply-each value (:callback (* each 2)))
)

(fn-do + closure-generator _
  (param value Int)
  (let offset 4)
  (yield (call apply-each value (:callback (+ each offset))))
  (= offset 5)
  (yield (call apply-each value (:callback (+ each offset))))
)
```

```output
6
6
7
```

```brix
(import /core)

(main
  (print (catch (call closure-errors true)))
  (print (catch (call closure-errors false)))
)

(type + CallbackFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(fn-expr + checked-apply Int (?! CallbackFailure) @inline
  (param callback CheckedCallback)
  (param value Int)
  (call callback value)
)

(fn-do + closure-errors Int (?! CallbackFailure)
  (param succeeds Bool)
  (let callback CheckedCallback
    (=> (value) :do
      (block
        (finally
          (print "callback cleanup")
        )
        (if succeeds
          (return (+ value 1))
        )
        (throw (call CallbackFailure "callback failed"))
      )
    )
  )
  (return (call checked-apply callback 4))
)
```

```output
callback cleanup
5
callback cleanup
callback failed
```

```brix
(import /core)

(main
  (print (catch (call closure-errors-ordinary true)))
  (print (catch (call closure-errors-ordinary false)))
)

(type + CallbackFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(fn-expr + checked-apply Int (?! CallbackFailure)
  (param callback CheckedCallback)
  (param value Int)
  (call callback value)
)

(fn-do + closure-errors-ordinary Int (?! CallbackFailure)
  (param succeeds Bool)
  (let callback CheckedCallback
    (=> (value) :do
      (block
        (finally
          (print "callback cleanup")
        )
        (if succeeds
          (return (+ value 1))
        )
        (throw (call CallbackFailure "callback failed"))
      )
    )
  )
  (return (call checked-apply callback 4))
)
```

```output
callback cleanup
5
callback cleanup
callback failed
```

```brix
(import /core)

(main
  (print (call closure-creation-join true 3))
  (print (call closure-creation-join false 3))
)

(fn-expr + make-adder (of Mapper Int Int) @inline
  (param offset Int)
  (=> (value) (+ value offset))
)

(fn-expr + invoke Int @inline
  (param callback (of Mapper Int Int))
  (param value Int)
  (call callback value)
)

(fn-do + closure-creation-join Int
  (param choose Bool)
  (param value Int)
  (let first (call make-adder 2))
  (let second (call make-adder 5))
  (let callback first)
  (if choose
    (= callback second)
  )
  (return (call invoke callback value))
)
```

```output
8
5
```

```brix
(import /core)

(main
  (print (call closure-identity 3))
  (print (call closure-unknown true (=> (value) (* value 3))))
  (print (call closure-unknown false (=> (value) (* value 3))))
)

(fn-expr + invoke Int @inline
  (param callback (of Mapper Int Int))
  (param value Int)
  (call callback value)
)

(fn-do + closure-identity Int
  (param value Int)
  (let offset 4)
  (let callback (of Mapper Int Int) (=> (item) (+ item offset)))
  (let alias callback)
  (print (== callback alias))
  (return (call invoke callback value))
)

(fn-do + closure-unknown Int
  (param choose Bool)
  (param unknown (of Mapper Int Int))
  (let offset 4)
  (let callback (of Mapper Int Int) (=> (item) (+ item offset)))
  (if choose
    (= callback unknown)
  )
  (return (call invoke callback 3))
)
```

```output
true
7
9
7
```

```brix
(import /core)

(main
  (print (call closure-lazy-counter (list 3 2 5 1 4)))
  (print (call closure-lazy-counter (list 9)))
  (print (call closure-lazy-counter (list 4.0 8.0)))
)

(type + (of Accumulate A I) (is (fn-type A (param accum A) (param item I))))

(fn-do + (of fold I A) _ @inline
  (param seed A)
  (param stream (stream I))
  (param callback (of Accumulate A I))
  (let result seed)
  (for (in item stream)
    (= result (call callback result item))
  )
  (return result)
)

(fn-expr + (of closure-lazy-counter Item) Item
  (param stream (stream Item))
  (/ result count)
  (where
    (lazy count (call Item))
    (lazy result
      (call (of fold Item) (call Item) stream
        (:callback
          (=> (accum item) :do
            (block
              (+= count 1)
              (return (+ accum item))
            )
          )
        )
      )
    )
  )
)
```

```output
3
9
6
```

```brix
(import /core)

(main
  (print (call closure-unrelated 4))
  (print (call closure-selected 4))
  (print (call closure-shared-sibling 3))
)

(fn-expr + scalar-step Int @inline
  (param value Int)
  (+ value 1)
)

(fn-expr + select-callback Int @inline
  (param callback (of Mapper Int Int))
  (param value Int)
  (call callback value)
)

(fn-expr + ordinary-callback Int
  (param callback (of Mapper Int Int))
  (param value Int)
  (call callback value)
)

(fn-do + closure-unrelated Int
  (param value Int)
  (let offset 3)
  (let callback (of Mapper Int Int) (=> (item) (+ item offset)))
  (let result (+ (call scalar-step value) (call ordinary-callback callback value)))
  (= offset 10)
  (return (+ result (call ordinary-callback callback value)))
)

(fn-do + closure-selected Int
  (param value Int)
  (let offset 2)
  (let callback (of Mapper Int Int) (=> (item) (+ item offset)))
  (return (call select-callback callback value))
)

(fn-do + closure-shared-sibling Int
  (param value Int)
  (let count 1)
  (let selected (of Mapper Int Int)
    (=> (amount) :do
      (block
        (+= count amount)
        (return count)
      )
    )
  )
  (let sibling (of Mapper Int Int)
    (=> (amount) :do
      (block
        (+= count amount)
        (return count)
      )
    )
  )
  (let first (call select-callback selected value))
  (let second (call ordinary-callback sibling 5))
  (let third (call select-callback selected 2))
  (return (+ (* first 100) (* second 10) third))
)
```

```output
26
6
501
```

```brix
(import /core)

(main
  (print (call closure-recursive 3))
  (print (call closure-recursive 0))
)

(fn-expr + recursive-invoke Int @inline
  (param callback (of Mapper Int Int))
  (param value Int)
  (call callback value)
)

(fn-do + closure-recursive Int
  (param depth Int)
  (let callback (of Mapper Int Int) (=> (value) value))
  (= callback
    (=> (value)
      (block
        (case? (> value 0)
          (+ value (call recursive-invoke callback (- value 1)))
        )
        (default
          0
        )
      )
    )
  )
  (return (call recursive-invoke callback depth))
)
```

```output
6
0
```

```brix
(main
  (print (call fusion-sum))
)

(fn-do + fusion-sum Int
  (let source (call fusion-numbers 3))
  (print "created")
  (let alias source)
  (let total 0)
  (for (in value alias)
    (print (str "consumer " value))
    (if (== value 2)
      (continue)
    )
    (+= total value)
    (if (== value 3)
      (break)
    )
  )
  (return total)
)

(fn-do + fusion-numbers _ @inline
  (param count Int)
  (print "started")
  (finally
    (print "closed")
  )
  (let index 1)
  (while (<= index count)
    (yield index)
    (print (str "resumed " index))
    (+= index 1)
  )
  (yield 99)
)
```

```output
created
started
consumer 1
resumed 1
consumer 2
resumed 2
consumer 3
closed
4
```

```brix
(main
  (print (call fusion-multiple))
)

(fn-do + fusion-multiple Int
  (let total 0)
  (for (in value (call fusion-pair 10))
    (+= total value)
  )
  (return total)
)

(fn-do + fusion-pair _ @inline
  (param value Int)
  (yield value)
  (yield (+ value 1))
)
```

```output
21
```

```brix
(main
  (print (call fusion-return))
)

(fn-do + fusion-return Int
  (for (in value (call fusion-finally))
    (return value)
  )
  (return 0)
)

(fn-do + fusion-finally _ @inline
  (finally
    (print "closed")
  )
  (yield 17)
  (print "unexpected")
  (yield 18)
)
```

```output
closed
17
```

```brix
(main
  (print (call fusion-callback))
)

(fn-do + fusion-callback Int
  (let total 0)
  (for (in value (call fusion-offsets 3))
    (+= total value)
  )
  (return total)
)

(fn-expr + fusion-apply Int @inline
  (param value Int)
  (param callback _)
  (call callback value)
)

(fn-do + fusion-offsets _ @inline
  (param value Int)
  (let offset 4)
  (yield (call fusion-apply value (:callback (+ each offset))))
  (= offset 5)
  (yield (call fusion-apply value (:callback (+ each offset))))
)
```

```output
15
```

```brix
(main
  (print (call fusion-nested))
)

(fn-do + fusion-nested Int
  (let total 0)
  (for (in value (call fusion-doubled))
    (+= total value)
  )
  (return total)
)

(fn-do + fusion-doubled _ @inline
  (for (in value (call fusion-inner))
    (yield (* value 2))
  )
)

(fn-do + fusion-inner _ @inline
  (yield 3)
  (yield 4)
)
```

```output
14
```

```brix
(main
  (let result (catch (call fusion-errors)))
  (print (is result FusionProblem))
)

(type + FusionProblem (is Error))

(fn-do + fusion-errors Int (?! FusionProblem)
  (for (in value (call fusion-failure))
    (print value)
  )
  (return 0)
)

(fn-do + fusion-failure (stream Int) (?! FusionProblem) @inline
  (finally
    (print "closed")
  )
  (yield 1)
  (throw (call FusionProblem "failed"))
)
```

```output
1
closed
true
```

```brix
(main
  (print (call fusion-choice true))
  (print (call fusion-choice false))
)

(fn-do + fusion-choice Int
  (param first Bool)
  (let source
    (match first
      (case= true
        (call fusion-left 3)
      )
      (default
        (call fusion-right 7)
      )
    )
  )
  (let total 0)
  (for (in value source)
    (+= total value)
  )
  (return total)
)

(fn-do + fusion-left _ @inline
  (param value Int)
  (yield value)
  (yield (+ value 1))
)

(fn-do + fusion-right _ @inline
  (param value Int)
  (yield (* value 2))
)
```

```output
7
14
```

```brix
(main
  (call fusion-cleanup-panic)
)

(fn-do + fusion-cleanup-panic _
  (let attempt
    (do-new
      (for (in value (call fusion-cleanup-failure))
        (print value)
        (break)
      )
      (print "unexpected")
      (on-failure failure
        (let error (? Panic) failure)
        (if error
          (print "recovered")
        )
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + fusion-cleanup-failure _ @inline
  (finally
    (throw (call Panic "cleanup"))
  )
  (yield 1)
)
```

```output
1
recovered
```

```brix
(main
  (print (call fusion-interface))
)

(fn-do + fusion-interface Int
  (let source (of Stream Int) (call fusion-forward))
  (let total 0)
  (for (in value source)
    (+= total value)
  )
  (return total)
)

(fn-expr + fusion-forward (of Stream Int) @inline
  (call fusion-interface-items)
)

(fn-do + fusion-interface-items (of Stream Int) @inline
  (yield 3)
  (yield 4)
)
```

```output
7
```

```brix
(main
  (print (call fusion-mixed))
)

(fn-do + fusion-mixed Int
  (let total 0)
  (let saved (call fusion-mixed-items))
  (for (in value (call fusion-mixed-items))
    (+= total value)
  )
  (for (in value saved)
    (+= total value)
  )
  (for (in value saved)
    (+= total 100)
  )
  (return total)
)

(fn-do + fusion-mixed-items _ @inline
  (yield 2)
  (yield 3)
)
```

```output
10
```

```brix
(main
  (print (call fusion-repeated))
)

(fn-do + fusion-repeated Int
  (let source (call fusion-repeated-items))
  (let total 0)
  (for (in pass (..< 0 2))
    (for (in value source)
      (+= total value)
    )
  )
  (return total)
)

(fn-do + fusion-repeated-items _ @inline
  (yield 2)
  (yield 3)
)
```

```output
5
```

```brix
(import /BxGC/BxGC.h)

(main
  (print (call fusion-escaped))
)

(fn-do + fusion-escaped Int
  (let callbacks (list (of Getter Int)) (call (of List (of Getter Int))))
  (for (in callback (call fusion-captures))
    (+= callbacks callback)
  )
  (call C:brixGCCollect)
  (return (+ (call ([] callbacks 0)) (call ([] callbacks 1))))
)

(fn-do + fusion-captures (stream (of Getter Int)) @inline
  (let value 3)
  (yield (=> () value))
  (= value 4)
  (yield (=> () value))
)
```

```output
8
```

```brix
(import /BxGC/BxGC.h)

(main
  (print (call fusion-snapshot))
)

(type + FusionRecord (is Struct)
  (prop + name String)
  (prop + count Int)
)

(fn-do + fusion-snapshot Int
  (let input (call FusionRecord "before" 7))
  (let source (call fusion-records input))
  (= (. input name) "after")
  (= (. input count) 9)
  (let total 0)
  (for (in item source)
    (call C:brixGCCollect)
    (print (. item name))
    (+= total (. item count))
  )
  (return total)
)

(fn-do + fusion-records _ @inline
  (param value FusionRecord)
  (yield value)
  (+= (. value count) 1)
  (yield value)
)
```

```output
before
before
15
```

```brix
(main
  (print (call fusion-input-capture))
)

(fn-do + fusion-input-capture Int
  (let offset 1)
  (let source (call fusion-map 2 (:callback (+ each offset))))
  (= offset 5)
  (let total 0)
  (for (in value source)
    (+= total value)
    (+= offset 1)
  )
  (return total)
)

(fn-do + fusion-map _ @inline
  (param value Int)
  (param callback _)
  (yield (call callback value))
  (yield (call callback value))
)
```

```output
15
```

```brix
(main
  (print (call fusion-generic))
)

(type + FusionProducer (is Tuple)
  (prop + count Int)
  (fn-do + (of items T) _ @inline
    (param value T)
    (for (in index (..< 0 count))
      (yield value)
    )
  )
)

(fn-do + fusion-generic Int
  (let producer (call FusionProducer 3))
  (let total 0)
  (for (in value (call (. producer items) (tuple "item" 4)))
    (+= total ([] value 1))
  )
  (return total)
)
```

```output
12
```

```brix
(main
  (print (call fusion-surrounding))
)

(fn-do + fusion-surrounding Int
  (let total 0)
  (for (in value (call fusion-surrounding-outer))
    (+= total value)
  )
  (return total)
)

(fn-do + fusion-surrounding-outer _
  (let count 10)
  (for (in value (call fusion-surrounding-inner))
    (yield (+ count value))
    (+= count 1)
  )
)

(fn-do + fusion-surrounding-inner _ @inline
  (yield 3)
  (yield 4)
)
```

```output
28
```

```brix
(main
  (print (call fusion-unbounded))
)

(fn-do + fusion-unbounded Int
  (let total 0)
  (for (in value (call fusion-forever))
    (if (== value 4)
      (break)
    )
    (+= total value)
  )
  (return total)
)

(fn-do + fusion-forever _ @inline
  (finally
    (print "closed")
  )
  (let index 0)
  (while true
    (yield index)
    (+= index 1)
  )
)
```

```output
closed
6
```

```brix
(main
  (print (call fusion-opaque-choice true))
  (print (call fusion-opaque-choice false))
)

(fn-do + fusion-opaque-choice Int
  (param known Bool)
  (let source
    (match known
      (case= true
        (call fusion-known)
      )
      (default
        (call fusion-unknown)
      )
    )
  )
  (let total 0)
  (for (in value source)
    (+= total value)
  )
  (return total)
)

(fn-do + fusion-known _ @inline
  (yield 3)
  (yield 4)
)

(fn-do + fusion-unknown _
  (yield 10)
)
```

```output
7
10
```

```brix
(main
  (do-new
    (print (call fusion-task))
  )
)

(fn-do + fusion-task Int
  (let total 0)
  (for (in value (call fusion-task-values))
    (+= total value)
  )
  (return total)
)

(fn-do + fusion-task-values _ @inline
  (yield 3)
  (yield 4)
)
```

```output
7
```

```brix
(main
  (let result (catch (call fusion-consumer-error)))
  (print (is result FusionProblem))
)

(type + FusionProblem (is Error))

(fn-do + fusion-consumer-error Int (?! FusionProblem)
  (finally
    (print "consumer closed")
  )
  (for (in value (call fusion-consumer-error-values))
    (print value)
    (throw (call FusionProblem "failed"))
  )
  (return 0)
)

(fn-do + fusion-consumer-error-values _ @inline
  (finally
    (print "producer closed")
  )
  (yield 3)
  (yield 4)
)
```

```output
3
producer closed
consumer closed
true
```

```brix
(main
  (print (call fusion-empty))
)

(fn-do + fusion-empty Int
  (let total 0)
  (for (in value (call fusion-empty-values false))
    (+= total value)
  )
  (return total)
)

(fn-do + fusion-empty-values _ @inline
  (param present Bool)
  (finally
    (print "closed")
  )
  (if present
    (yield 3)
  )
)
```

```output
closed
0
```

```brix
(main
  (print (call fusion-never-started))
)

(fn-do + fusion-never-started Int
  (let unused (call fusion-unused (call fusion-unused-argument)))
  (return 17)
)

(fn-do + fusion-unused-argument Int
  (print "created")
  (return 3)
)

(fn-do + fusion-unused _ @inline
  (param value Int)
  (print "entered")
  (finally
    (print "closed")
  )
  (yield value)
)
```

```output
created
17
```

```brix
(main
  (print (call fusion-yield-copy))
)

(type + FusionYieldRecord (is Struct)
  (prop + count Int)
)

(fn-do + fusion-yield-copy Int
  (let total 0)
  (for (in value (call fusion-yield-copy-values))
    (+= (. value count) 10)
    (+= total (. value count))
  )
  (return total)
)

(fn-do + fusion-yield-copy-values _ @inline
  (let value (call FusionYieldRecord 3))
  (yield value)
  (+= (. value count) 1)
  (yield value)
)
```

```output
27
```

```brix
(main
  (print (call fusion-recursive))
)

(fn-do + fusion-recursive Int
  (let total 0)
  (let seen 0)
  (for (in value (call fusion-recursive-values 3))
    (+= seen 1)
    (if (> seen 4)
      (return -1)
    )
    (+= total value)
  )
  (return total)
)

(fn-do + fusion-recursive-values (of Generator I32) @inline
  (param count I32)
  (yield count)
  (if (> count 0)
    (for (in value (call fusion-recursive-values (- count 1)))
      (yield value)
    )
  )
)
```

```output
6
```

```brix
(main
  (print (call fusion-pipeline))
)

(fn-do + fusion-pipeline Int
  (let total 0)
  (for (in value (call pipeline-filter (call pipeline-map (..< 0 8) (:with (+ each 1))) (:where (== (% each 2) 0))))
    (+= total value)
  )
  (for (in value (call pipeline-map (call pipeline-filter (..< 0 8) (:where (== (% each 2) 0))) (:with (+ each 1))))
    (+= total value)
  )
  (let first (call pipeline-map (..< 0 8) (:with (+ each 1))))
  (let second (call pipeline-filter first (:where (== (% each 2) 0))))
  (let third (call pipeline-map second (:with (* each 3))))
  (for (in value (call pipeline-filter third (:where (> each 10))))
    (+= total value)
  )
  (return total)
)

(fn-do + (of pipeline-map From To) _ @inline
  (param stream (stream From))
  (param with (of Mapper From To))
  (for (in item stream)
    (yield (call with item))
  )
)

(fn-do + (of pipeline-filter Item) _ @inline
  (param stream (stream Item))
  (param where (of Predicate Item))
  (for (in item stream (when (call where item)))
    (yield item)
  )
)
```

```output
90
```

```brix
(main
  (let result (catch (call fusion-pipeline-errors)))
  (print (is result FusionProblem))
)

(type + FusionProblem (is Error))

(fn-do + fusion-pipeline-errors Int (?! FusionProblem)
  (for (in value (call pipeline-forward (call pipeline-forward (call fusion-failure))))
    (print value)
  )
  (return 0)
)

(fn-do + fusion-failure (stream Int) (?! FusionProblem) @inline
  (finally
    (print "closed")
  )
  (yield 1)
  (throw (call FusionProblem "failed"))
)

(fn-do + pipeline-forward _ @inline
  (param stream (stream Int))
  (for (in item stream)
    (yield item)
  )
)
```

```output
1
closed
true
```

```brix
(main
  (print (call fusion-pipeline-repeated))
)

(fn-do + fusion-pipeline-repeated Int
  (let source (call fusion-repeated-items))
  (let total 0)
  (for (in pass (..< 0 2))
    (for (in value (call pipeline-forward source))
      (+= total value)
    )
  )
  (return total)
)

(fn-do + fusion-repeated-items _ @inline
  (yield 2)
  (yield 3)
)

(fn-do + pipeline-forward _ @inline
  (param stream (stream Int))
  (for (in item stream)
    (yield item)
  )
)
```

```output
5
```

```brix
(main
  (print (call fusion-pipeline-cleanup))
)

(fn-do + fusion-pipeline-cleanup Int
  (let inner (call pipeline-stage (call pipeline-items) "inner"))
  (for (in value (call pipeline-stage inner "outer"))
    (print (str "consumer " value))
    (return value)
  )
  (return 0)
)

(fn-do + pipeline-stage _ @inline
  (param stream (stream Int))
  (param name String)
  (finally
    (print (str name " closed"))
  )
  (for (in item stream)
    (print (str name " " item))
    (yield (* item 2))
  )
)

(fn-do + pipeline-items _ @inline
  (finally
    (print "source closed")
  )
  (yield 1)
  (yield 2)
)
```

```output
inner 1
outer 2
consumer 4
source closed
inner closed
outer closed
4
```

```brix
(main
  (let result (catch (call fusion-pipeline-consumer-error)))
  (print (is result FusionProblem))
)

(type + FusionProblem (is Error))

(fn-do + fusion-pipeline-consumer-error Int (?! FusionProblem)
  (finally
    (print "consumer closed")
  )
  (for (in value (call pipeline-forward (call pipeline-forward (call fusion-consumer-error-values))))
    (print value)
    (throw (call FusionProblem "failed"))
  )
  (return 0)
)

(fn-do + fusion-consumer-error-values _ @inline
  (finally
    (print "producer closed")
  )
  (yield 3)
  (yield 4)
)

(fn-do + pipeline-forward _ @inline
  (param stream (stream Int))
  (for (in item stream)
    (yield item)
  )
)
```

```output
3
producer closed
consumer closed
true
```

```brix
(main
  (print (call fusion-stdlib-pipeline))
)

(fn-do + fusion-stdlib-pipeline Int
  (let total 0)
  (for (in value (call filter (call map (..< 0 8) (:with (+ each 1))) (:where (== (% each 2) 0))))
    (+= total value)
  )
  (for (in value (call map (call filter (..< 0 8) (:where (== (% each 2) 0))) (:with (+ each 1))))
    (+= total value)
  )
  (let first (call map (..< 0 8) (:with (+ each 1))))
  (let second (call filter first (:where (== (% each 2) 0))))
  (let third (call map second (:with (* each 3))))
  (for (in value (call filter third (:where (> each 10))))
    (+= total value)
  )
  (return total)
)
```

```output
90
```

```brix
(main
  (print (call fusion-pipeline-choice true))
  (print (call fusion-pipeline-choice false))
)

(fn-do + fusion-pipeline-choice Int
  (param first Bool)
  (let source
    (match first
      (case= true
        (call fusion-left 3)
      )
      (default
        (call fusion-right 7)
      )
    )
  )
  (let total 0)
  (for (in value (call pipeline-forward source))
    (+= total value)
  )
  (return total)
)

(fn-do + fusion-left _ @inline
  (param value Int)
  (yield value)
  (yield (+ value 1))
)

(fn-do + fusion-right _ @inline
  (param value Int)
  (yield (* value 2))
)

(fn-do + pipeline-forward _ @inline
  (param stream (stream Int))
  (for (in item stream)
    (yield item)
  )
)
```

```output
7
14
```

```brix
(main
  (print (call feed-chain))
)

(fn-do + feed-chain Int
  (let offset 2)
  (let source
    (block
      (item 0)
      (spread (.. 1 5))
      (item? (only-if 8 (if false)))
      (spread? (only-if (list 6 7) (if true)))
      (pipe *> (+ each offset))
      (pipe ?> (== (% each 2) 0))
      (pipe ?*> (only-if (* each 3) (if (> each 3))))
    )
  )
  (let alias source)
  (let total 0)
  (for (in value alias)
    (+= total value)
  )
  (return total)
)
```

```output
54
```

```brix
(main
  (print (call feed-list))
)

(fn-expr + feed-list (of List Int)
  (item 1)
  (spread (.. 2 4))
  (pipe *> (* each 2))
)
```

```output
2, 4, 6, 8
```

```brix
(main
  (print (call List (call feed-escaping)))
)

(fn-expr + feed-escaping _
  (item 1)
  (spread (.. 2 4))
  (pipe *> (* each 2))
  (pipe ?> (> each 3))
)
```

```output
4, 6, 8
```

```brix
(main
  (let source
    (block
      (item 0)
      (spread? (only-if (list 6 7) (if true)))
    )
  )
  (for (in item source)
    (print item)
  )
)
```

```output
0
6
7
```

```brix
(main
  (print (call feed-partial))
)

(fn-do + feed-partial Int
  (let factor 2)
  (let values
    (block
      (item (call feed-effect 1))
      (spread (call feed-cleanup))
      (item (call feed-effect 9))
      (pipe *> (* each factor))
    )
  )
  (= factor 3)
  (print "created")
  (for (in value values)
    (print value)
    (if (>= value 6)
      (return 17)
    )
  )
  (return 0)
)

(fn-do + feed-effect Int
  (param value Int)
  (print (str "produce " value))
  (return value)
)

(fn-do + feed-cleanup _ @inline
  (finally
    (print "closed")
  )
  (yield 2)
  (yield 3)
)
```

```output
created
produce 1
3
6
closed
17
```

```brix
(main
  (let state (call FeedState 2))
  (let values (call feed-lazy state))
  (print "created")
  (= (. state factor) 3)
  (for (in value values)
    (print value)
    (if (>= value 6)
      (break)
    )
  )
)

(type + FeedState
  (prop + factor Int)
  (fn-do + This _
    (param factor _)
  )
)

(fn-expr + feed-lazy _
  (param state FeedState)
  (item (call feed-lazy-value 1))
  (spread (call feed-lazy-inner))
  (item (call feed-lazy-value 9))
  (pipe *> (* each (. state factor)))
  (pipe ?> (> each 0))
)

(fn-do + feed-lazy-value Int
  (param value Int)
  (print (str "produce " value))
  (return value)
)

(fn-do + feed-lazy-inner _ @inline
  (finally
    (print "closed")
  )
  (yield 2)
  (yield 3)
)
```

```output
created
produce 1
3
6
closed
```

```brix
(main
  (print (call feed-nested))
)

(fn-do + feed-nested Int
  (let rows (in (* each 10) (.. 1 3)))
  (let values
    (block
      (spread rows)
      (spread (in (+ each 1) (.. 1 2) (when (> each 1))))
      (pipe *> (+ each 1))
    )
  )
  (let total 0)
  (for (in value values)
    (+= total value)
  )
  (return total)
)
```

```output
67
```

```brix
(main
  (let result (catch (call feed-errors)))
  (print (is result FeedFailure))
)

(type + FeedFailure (is Error))

(fn-do + feed-errors Int (?! FeedFailure)
  (let values
    (block
      (item 1)
      (spread (call feed-failure))
      (item 9)
      (pipe *> (+ each 1))
    )
  )
  (for (in value values)
    (print value)
  )
  (return 0)
)

(fn-do + feed-failure (stream Int) (?! FeedFailure) @inline
  (finally
    (print "closed")
  )
  (yield 2)
  (throw (call FeedFailure "failed"))
)
```

```output
2
3
closed
true
```

```brix
(import /BxGC/BxGC.h)

(main
  (let values (call feed-managed (call FeedBox "one")))
  (call C:brixGCCollect)
  (for (in value values)
    (call C:brixGCCollect)
    (print (. value text))
  )
)

(type + FeedBox
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(fn-expr + feed-managed _
  (param first FeedBox)
  (spread (call feed-boxes first))
  (item (call FeedBox "three"))
  (pipe *> each)
  (pipe ?> (!= (. each text) ""))
)

(fn-do + feed-boxes _ @inline
  (param first FeedBox)
  (yield first)
  (call C:brixGCCollect)
  (yield (call FeedBox "two"))
)
```

```output
one
two
three
```

```brix
(import /core/channel)
(import /core/task)

(main
  (let input (call (of SyncBufferedChannel Int)))
  (let output (call (of SyncBufferedChannel Int)))
  (<- input 7)
  (call (. input close))
  (for (in value (call feed-task-values input output))
    (print value)
  )
)

(fn-do + feed-task-values _
  (param input (of SyncBufferedChannel Int))
  (param output (of SyncBufferedChannel Int))
  (do-new
    (for (in value input)
      (let callback (of Getter Int) (=> () (+ value 1)))
      (<- output (call callback))
    )
    (call (. output close))
  )
  (for (in value output)
    (yield value)
  )
)
```

```output
8
```

```brix
(main
  (call specialization-repeated)
)

(fn-do + specialization-repeated _
  (let source (call specialization-producer))
  (for (in index (..< 0 2))
    (let mapped (call map source (:with (+ each 10))))
    (call specialization-drain mapped)
  )
)

(fn-do + specialization-producer _ @inline
  (finally
    (print "closed")
  )
  (print "producer")
  (yield 1)
  (yield 2)
)

(fn-do + specialization-drain _
  (param source (stream Int))
  (for (in value source)
    (print value)
  )
)
```

```output
producer
11
12
closed
```

```brix
(main
  (print (call fusion-callback-join true))
  (print (call fusion-callback-join false))
)

(fn-do + fusion-callback-join Int
  (param choose Bool)
  (let source
    (match choose
      (case= true
        (call fusion-callback-left)
      )
      (default
        (call fusion-callback-right)
      )
    )
  )
  (let last (of Getter Int) (=> () 0))
  (for (in value source)
    (= last (=> () value))
  )
  (return (call last))
)

(fn-do + fusion-callback-left _ @inline
  (yield 1)
  (yield 2)
)

(fn-do + fusion-callback-right _ @inline
  (yield 3)
  (yield 4)
)
```

```output
2
4
```

```brix
(main
  (print (call fusion-callback-single))
)

(fn-do + fusion-callback-single Int
  (let source (call fusion-callback-left))
  (let last (of Getter Int) (=> () 0))
  (for (in value source)
    (= last (=> () value))
  )
  (return (call last))
)

(fn-do + fusion-callback-left _ @inline
  (yield 1)
  (yield 2)
)
```

```output
2
```

```brix
(main
  (let holder (call KeptHolder))
  (+= (. holder items) (call KeptStored 7 "seven"))
  (= (. holder before) (call KeptStored 3 "three"))
  (let kept (call (. holder settle) 1))
  (print (str (. kept id) " " (. kept name)))
  (let next (call (. holder settle) 1))
  (print (str (. next id) " " (. next name)))
  (= (. holder before) (call KeptStored 4 "four"))
  (let swapped (call (. holder swap) (call KeptStored 5 "five")))
  (print (str (. swapped id) " " (. swapped name)))
  (let picked (call kept-pick holder (!! (. holder before))))
  (print (str (. picked id) " " (. picked name)))
  (print (== (. holder before) null))
)

(type + KeptThing (is Interface)
  (fn-expr + id U64 :get)
  (fn-expr + name String :get)
)

(type _ KeptStored (is Tuple)
  (implements KeptThing)
  (prop + id U64)
  (prop + name String)
)

(fn-expr _ kept-doubled Size @inline
  (param count Size)
  (* count 2)
)

(fn-do _ kept-pick KeptThing @inline
  (param holder KeptHolder)
  (param thing KeptThing)
  (= (. holder before) null)
  (return thing)
)

(type _ KeptHolder
  (prop + before (? KeptThing))
  (prop + items (list KeptThing))
  (prop + seen Size)
  (fn-do + settle KeptThing
    (param scale Size)
    (let original (?? before ([] items 0)))
    (= before null)
    (+= seen (call kept-doubled scale))
    (return original)
  )
  (fn-do + swap KeptThing
    (param next KeptThing)
    (let previous (!! before))
    (= before next)
    (+= seen (call kept-doubled 1))
    (return previous)
  )
)
```

```output
3 three
7 seven
4 four
5 five
true
```

```brix
(import ./callback-support)
(import /core)
(blank)

(main
  (print (call closure-interface 4))
)

(type + Measured (is Interface)
  (fn-expr + total Int)
)

(type + Amount (is Tuple)
  (implements Measured)
  (prop + value Int)
  (fn-expr + total Int @inline
    value
  )
)

(fn-expr + read Int @inline
  (param value Measured)
  (call (. value total))
)

(fn-do + closure-interface Int
  (param value Int)
  (let capture Measured (call Amount value))
  (let callback (of Mapper Measured Measured) (=> (input) (call Amount (+ (call (. input total)) (call (. capture total))))))
  (return (call read (call (of apply Measured Measured) callback (call Amount 3))))
)
```

```brix file=callback-support.bx
(import /core)

(fn-expr + (of apply T U) U @inline
  (param callback (of Mapper T U))
  (param value T)
  (call callback value)
)
```

```output
7
```

```brix
(import ./generator-producer)
(import /core)
(blank)

(main
  (print (call fused-total))
)

(fn-do + fused-total Int
  (let total 0)
  (for (in value (call values 12))
    (+= total value)
  )
  (return total)
)
```

```brix file=generator-producer.bx
(import /core)

(fn-do + values _ @inline
  (param count Int)
  (let index 0)
  (while (< index count)
    (yield index)
    (+= index 1)
  )
)
```

```output
66
```

```brix
(import ./feed-producer)
(import /core)
(blank)

(main
  (print (call feed-module))
)

(fn-do + feed-module Int
  (let values (call feed-values))
  (let alias values)
  (let total 0)
  (for (in value alias)
    (+= total value)
  )
  (return total)
)
```

```brix file=feed-producer.bx
(import /core)

(fn-expr + feed-values _ @inline
  (item 1)
  (spread (.. 2 12))
  (pipe *> (* each 2))
  (pipe ?> (== (% each 4) 0))
)
```

```output
84
```

```brix
(main
  (call specialization-repeated)
)

(fn-do + specialization-repeated _
  (for (in index (..< 0 2))
    (let source (call specialization-producer))
    (let mapped (call map source (:with (+ each 10))))
    (call specialization-drain mapped)
  )
)

(fn-do + specialization-producer _ @inline
  (finally
    (print "closed")
  )
  (print "producer")
  (yield 1)
  (yield 2)
)

(fn-do + specialization-drain _
  (param source (stream Int))
  (for (in value source)
    (print value)
  )
)
```

```output
producer
11
12
closed
producer
11
12
closed
```

```brix
(main
  (print (call fusion-callback-join true))
  (print (call fusion-callback-join false))
  (print (call fusion-callback-single))
)

(fn-do + fusion-callback-join Int
  (param choose Bool)
  (let source
    (match choose
      (case= true
        (call fusion-callback-left)
      )
      (default
        (call fusion-callback-right)
      )
    )
  )
  (let last (of Getter Int) (=> () 0))
  (for (in value source)
    (= last (=> () value))
    (break)
  )
  (return (call last))
)

(fn-do + fusion-callback-single Int
  (let source (call fusion-callback-left))
  (let last (of Getter Int) (=> () 0))
  (for (in value source)
    (= last (=> () value))
    (break)
  )
  (return (call last))
)

(fn-do + fusion-callback-left _ @inline
  (yield 1)
  (yield 2)
)

(fn-do + fusion-callback-right _ @inline
  (yield 3)
  (yield 4)
)
```

```output
1
3
1
```

```brix
(main
  (print (call fusion-callback-join true))
  (print (call fusion-callback-join false))
  (print (call fusion-callback-single))
)

(fn-do + fusion-callback-join Int
  (param choose Bool)
  (let source
    (match choose
      (case= true
        (call fusion-callback-left)
      )
      (default
        (call fusion-callback-right)
      )
    )
  )
  (let last (of Getter Int) (=> () 0))
  (for (in value source)
    (print (call last))
    (= last (=> () value))
  )
  (return (call last))
)

(fn-do + fusion-callback-single Int
  (let source (call fusion-callback-left))
  (let last (of Getter Int) (=> () 0))
  (for (in value source)
    (print (call last))
    (= last (=> () value))
  )
  (return (call last))
)

(fn-do + fusion-callback-left _ @inline
  (yield 1)
  (yield 2)
)

(fn-do + fusion-callback-right _ @inline
  (yield 3)
  (yield 4)
)
```

```output
0
1
2
0
3
4
0
1
2
```

```brix
(import ./projection-nodes)
(import /core/arena)
(import /core/array-builder)
(import /core/parsing)
(import /core/table)
(blank)
(prop _ xml-tree Tree @stack)
(blank)

(main
  (let arena (use (call Arena)))
  (let tree (call Tree (call (of Table Node) arena 16) (call (of ArrayBuilder NodeId) arena)))
  (let a (as (call (. tree nodes append) (call Leaf (call MatchLocation) 7)) NodeId))
  (call (. tree children append) a)
  (let b (as (call (. tree nodes append) (call Call (call MatchLocation) a (call (of RowRange NodeId) 0 1) 0 false)) NodeId))
  (call (. tree children append) b)
  (let c (as (call (. tree nodes append) (call Call (call MatchLocation) b (call (of RowRange NodeId) 1 1) 0 true)) NodeId))
  (do-with xml-tree tree
    (print (as (. ([] (. tree nodes) c) xml) String))
  )
)

(type + Tree (is Tuple)
  (prop + nodes (of Table Node))
  (prop + children (of ArrayBuilder NodeId ArenaBacking))
  (fn-expr + child-ids (of IndexSpan NodeId)
    (param range (of RowRange NodeId))
    (call (of IndexSpan NodeId) children (..< (. range start) (. range end)))
  )
)

(fn-expr _ child-xml XML
  (param id NodeId)
  (. ([] (. xml-tree nodes) id) xml)
)

(fn-do _ child-xmls (? (of ImmutableArray XML))
  (param range (of RowRange NodeId))
  (print (str "range " (. range start) "+" (. range count)))
  (return-if (if (== (. range count) 0)))
  (let builder (call (of ArrayBuilder XML)))
  (for (in node ([] (. xml-tree nodes) (call (. xml-tree child-ids) range)))
    (call (. builder append) (. node xml))
  )
  (return (call (. builder finish-immutable)))
)

(extend + Leaf
  (fn-expr + xml XML :get
    (<> XML (text: "Leaf"))
  )
)

(extend + Call
  (fn-expr + xml XML :get
    (<> XML (text: "Call") (:curried? is-curried)
      (call child-xml callable)
      (if (. params count)
        (<> XML (text: "params")
          (call child-xmls params)
        )
      )
      (call child-xmls args)
    )
  )
)

(type + XML
  (prop + node-name String)
  (prop + attrs (list (of KeyValuePair String String)))
  (prop + children (list This))
  (fn-do + This _
    (param node-name String)
  )
  (fn-to-do + String
    (let text (str "<" node-name))
    (for (in attr attrs)
      (+= text (str " " (. attr key) "=\"" (. attr value) "\""))
    )
    (+= text ">")
    (for (in child children)
      (+= text (as child String))
    )
    (return (+ text (str "</" node-name ">")))
  )
  (fn-do + (infix this += (param child (? This))) _
    (if child
      (+= children child)
    )
  )
  (fn-do + (infix this ++= (param kids (? (stream This)))) _
    (if kids
      (for (in kid kids)
        (+= children kid)
      )
    )
  )
  (fn-do + (infix this += (param kv (? (of KeyValuePair String String)))) _
    (if kv
      (+= attrs kv)
    )
  )
  (fn-do + (infix this += (param kv (? (of KeyValuePair String ''Value)))) _
    (if kv
      (+= attrs (tuple (. kv key) (str (. kv value))))
    )
  )
)
```

```brix file=projection-nodes.bx
(import /core/parsing)
(blank)
(type-line + NodeId (is U32))

(type + (of RowRange ''Row) (is Tuple)
  (prop + start U32)
  (prop + count U32)
  (fn-expr + end U32 :get @inline
    (+ start count)
  )
)

(type + Node (is Union)
  (variant leaf Leaf)
  (variant call Call)
)

(type + Base (is Tuple)
  (prop + loc MatchLocation)
)

(type + Leaf (is Base)
  (prop + value U32)
)

(type + Call (is Base)
  (prop + callable NodeId)
  (prop + operands (of RowRange NodeId))
  (prop + param-count U8)
  (prop + is-curried Bool)
  (fn-expr + params (of RowRange NodeId) :get @inline
    (call (of RowRange NodeId) (. operands start) param-count)
  )
  (fn-expr + args (of RowRange NodeId) :get @inline
    (call (of RowRange NodeId) (+ (. operands start) param-count) (- (. operands count) param-count))
  )
)
```

```output
range 0+1
range 1+1
range 0+1
<Call curried="true"><Call><Leaf></Leaf><Leaf></Leaf></Call><Call><Leaf></Leaf><Leaf></Leaf></Call></Call>
```
