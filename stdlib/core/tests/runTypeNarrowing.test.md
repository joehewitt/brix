# Type narrowing

Type narrowing

These tests specify the desired flow-sensitive narrowing behavior. Phase-one optional
and union narrowing cases are active; future smart-cast behavior remains marked %nyi.

### Optional locals narrow to their payload type in a truthy branch.

```brix
(main
  (let value (? Int) 41)
  (if value
    (print (+ value 1))
    (print (type-of value))
  )
  (print (type-of value))
)
```

```output
42
[TypeInfo I64]
[TypeInfo Optional'I64]
```

### Optional parameters narrow to their payload type.

```brix
(main
  (print (call describe (call Color 10 20 30)))
  (print (call describe null))
  (print (call describe-guard (call Color 40 50 60)))
  (print (call describe-guard null))
)

(fn-do + describe _
  (param color (? Color))
  (if color
    (return (str (. color red) "," (. color green) "," (. color blue)))
    (else
      (return "none")
    )
  )
)

(fn-expr + describe-guard _
  (param color (? Color))
  (case? color
    (call describe-present color)
  )
  (default
    "none"
  )
)

(fn-expr + describe-present String
  (param color Color)
  (str (. color red) "," (. color green) "," (. color blue))
)

(type + Color (is (tuple (: red Int) (: green Int) (: blue Int))))
```

```output
10,20,30
none
40,50,60
none
```

### Guard returns narrow the remainder of the function.

```brix
(main
  (print (call add-one 41))
  (print (call add-one null))
)

(fn-do + add-one _
  (param value (? Int))
  (if (not value)
    (return 0)
  )
  (blank)
  (return (+ value 1))
)
```

```output
42
0
```

### Panic assertions narrow the fallthrough path.

```brix
(main
  (print (call guarded-name "Brix"))
)

(fn-do + guarded-name (? String)
  (param name (? String))
  (throw (call Panic "missing") (if (not name)))
  (blank)
  (return name)
)
```

```output
Brix
```

### Implicit self properties share flow facts with explicit property paths.

```brix
(main
  (let holder (call NarrowingHolder "present"))
  (call (. holder after-assert))
  (call (. holder after-early-return))
  (call (. holder after-returning-branch))
  (call (. holder after-explicit-guard))
  (call (. holder explicit-read-after-implicit-guard))
  (call (. holder after-other-assignment))
  (call (. holder after-same-assignment))
  (= (. holder value) "present")
  (call (. holder after-mutating-call))
  (call (. holder shadowed-local))
)

(type + NarrowingHolder
  (prop + value (? String))
  (prop + other Int)
  (fn-do + This _
    (param value _)
  )
  (fn-do + after-assert _
    (throw (call Panic "missing") (if (not value)))
    (print (type-of value))
  )
  (fn-do + after-early-return _
    (return-if (if (not value)))
    (print (type-of value))
  )
  (fn-do + after-returning-branch _
    (if (not value)
      (return)
    )
    (print (type-of value))
  )
  (fn-do + after-explicit-guard _
    (return-if (if (not (. this value))))
    (print (type-of value))
  )
  (fn-do + explicit-read-after-implicit-guard _
    (return-if (if (not value)))
    (print (type-of (. this value)))
  )
  (fn-do + after-other-assignment _
    (return-if (if (not value)))
    (= other 1)
    (print (type-of value))
  )
  (fn-do + after-same-assignment _
    (return-if (if (not value)))
    (= value null)
    (print (type-of value))
  )
  (fn-do + after-mutating-call _
    (return-if (if (not value)))
    (call clear)
    (print (type-of value))
  )
  (fn-do + clear _
    (= value null)
  )
  (fn-do + shadowed-local _
    (let value (? String) "local")
    (return-if (if (not value)))
    (print (type-of value))
  )
)
```

```output
[TypeInfo String]
[TypeInfo String]
[TypeInfo String]
[TypeInfo String]
[TypeInfo String]
[TypeInfo String]
[TypeInfo Optional'String]
[TypeInfo Optional'String]
[TypeInfo String]
```

### A property that takes the type of its value narrows as one written with its type.

```brix
(main
  (let holder (call InferredHolder))
  (call (. holder check))
)

(fn-do + found (? String)
  (param wanted Bool)
  (return (only-if "present" (if wanted)))
)

(type + InferredHolder
  (prop + value _ (= (call found true)))
  (fn-do + check _
    (return-if (if (not value)))
    (print (type-of value))
    (if (and value (> (. value length) 0))
      (print (type-of value))
    )
  )
)
```

```output
[TypeInfo String]
[TypeInfo String]
```

### Optional chaining sees the payload type of a narrowed implicit self property.

```brix crash
(main
  (let holder (call ImplicitChainHolder))
  (call (. holder check))
)

(type + ImplicitChainHolder
  (prop + value (? String) (= "present"))
  (fn-do + check _
    (return-if (if (not value)))
    (print (?. value length))
  )
)
```

```output
Exception: "value" is String, which is never null, so it needs no "?"; write "(. value length)"
<no file:11>
        (print (?. value length))
               ^^^^^^^^^^^^^^^^^
```

### A test of an implicit self property narrows no property of a class in another module.

```brix
(import ./lib/inherited)
(blank)

(main
  (let child (call InheritingChild))
  (call (. child total))
  (= (. child caught) (call Caught))
  (call (. child total))
)

(type + Caught)

(type + InheritingChild (is InheritedBase)
  (prop + caught (? Caught))
  (fn-do + total _
    (if (!= caught null)
      (print (+ p0 p1 p2 p3 p4 p5 p6 p7 p8 p9 p10 p11 p12 p13 p14 p15))
    )
  )
)
```

```brix file=lib/inherited.bx
(type + InheritedBase
  (prop + p0 _ (= 0))
  (prop + p1 _ (= 1))
  (prop + p2 _ (= 2))
  (prop + p3 _ (= 3))
  (prop + p4 _ (= 4))
  (prop + p5 _ (= 5))
  (prop + p6 _ (= 6))
  (prop + p7 _ (= 7))
  (prop + p8 _ (= 8))
  (prop + p9 _ (= 9))
  (prop + p10 _ (= 10))
  (prop + p11 _ (= 11))
  (prop + p12 _ (= 12))
  (prop + p13 _ (= 13))
  (prop + p14 _ (= 14))
  (prop + p15 _ (= 15))
)
```

```output
120
```

### The right side of && sees facts introduced by the left side.

```brix
(main
  (let color (? Color) (call Color 255 128 64))
  (if (and color (> (. color red) 200))
    (print (. color green))
  )
)

(type + Color (is (tuple (: red Int) (: green Int) (: blue Int))))
```

```output
128
```

### Stable optional path subjects narrow to their payload type.

```brix
(main
  (let box (? Box) (call Box))
  (if (and box (. box child))
    (print (. box child hello))
  )
  (blank)
  (if (and box (. box child))
    (let child (. box child))
    (print (. child hello))
  )
)

(type + Box
  (prop + child (? Child) (= (call Child)))
)

(type + Child
  (prop + hello _ (= "hi"))
)
```

```output
hi
hi
```

### Optional-chain receivers keep their optional wrapper after path narrowing.

```brix
(main
  (let anchor (? Node) (call Node))
  (let previous (call Node))
  (= (. (!! anchor) previous) previous)
  (let child (call Node))
  (blank)
  (if anchor
    (let target anchor)
    (= (. child previous) (. target previous))
    (= (. child next) target)
    (if (. target previous)
      (= (. target previous next) child)
    )
    (= (. target previous) child)
  )
  (blank)
  (if (. previous next)
    (print "linked")
  )
)

(type + Node
  (prop + previous (? Node))
  (prop + next (? Node))
)
```

```output
linked
```

### The right side of || sees the negated facts from the left side when it evaluates.

```brix
(main
  (let value (? Int) 5)
  (if (or (not value) (> value 3))
    (print "pass")
  )
)
```

```output
pass
```

### The right side of || can use a narrowed optional with payload-specific truthiness.

```brix
(main
  (print (call check null))
  (print (call check ""))
  (print (call check "body"))
)

(fn-do + check _
  (param head (? String))
  (return (if-expr (or (not head) (call empty-string head)) "empty" "body"))
)

(fn-do + empty-string _
  (param value String)
  (return (not value))
)
```

```output
empty
empty
body
```

### Negated optional checks narrow the else branch.

```brix
(main
  (let name (? String) "Brix")
  (if (not name)
    (print "missing")
    (else
      (print (. name length))
      (print (type-of name))
    )
  )
)
```

```output
4
[TypeInfo String]
```

### Conditional expressions should narrow the result expression.

```brix
(main
  (let value (? Int) 41)
  (print (if-expr value (+ value 1) 0))
)
```

```output
42
```

### Lazy Bool dependencies used as conditional feed guards should not be projected.

```brix
(main
  (print
    (call (. ", " join)
      (block
        (spread (call headers "body"))
      )
    )
  )
  (print
    (call (. ", " join)
      (block
        (spread (call headers null))
      )
    )
  )
)

(fn-expr + headers _
  (param body (? String))
  (item? (only-if "json" (if has-body)))
  (where
    (lazy has-body (if-expr body true false))
  )
)
```

```output
json
```

### Assignment to a narrowed optional name updates the original.

```brix
(main
  (let value (? Int) 41)
  (if value
    (= value 10)
    (print (+ value 1))
  )
  (blank)
  (print (?? value 0))
  (let total (? Int) 1)
  (let other (? Int) 2)
  (if (and other total)
    (= total (+ total other))
  )
  (print (?? total 0))
  (let missing (? Int))
  (if missing
    (= missing 0)
    (else-if total
      (= total (* total 10))
    )
    (else
      (= total null)
    )
  )
  (print (?? total 0))
  (let cleared (? Int) 5)
  (if cleared
    (= cleared null)
  )
  (print (?? cleared 0))
)
```

```output
11
10
3
30
0
```

### Branch assignments to narrowed locals carry across loop iterations.

```brix
(main
  (let values (list (? Int)))
  (+= values 5)
  (+= values null)
  (+= values 7)
  (let total (? Int))
  (for (in value values)
    (if (and value total)
      (= total (+ total value))
      (else-if value
        (= total value)
      )
    )
  )
  (print (?? total 0))
  (let items (list (? Pair)))
  (+= items (call Pair 1 2))
  (+= items (call Pair 3 4))
  (let merged (? Pair))
  (for (in item items)
    (if (and item merged)
      (= merged (call Pair (+ (. merged x) (. item x)) (+ (. merged y) (. item y))))
      (else
        (= merged item)
      )
    )
  )
  (print (. (!! merged) x))
  (print (. (!! merged) y))
)

(type + Pair (is (tuple (: x Int) (: y Int))))
```

```output
12
4
6
```

### The original optional can be reassigned and re-narrowed after a narrowed scope.

```brix
(main
  (let value (? Int) 1)
  (if value
    (print (+ value 1))
  )
  (blank)
  (= value null)
  (if value
    (print "unexpected")
    (else
      (print "missing")
    )
  )
  (blank)
  (= value 40)
  (if value
    (print (+ value 2))
  )
)
```

```output
2
missing
42
```

### Reassignment to a payload-compatible value can be narrowed by a later test.

```brix
(main
  (let value (? Int) null)
  (= value 40)
  (if value
    (print (+ value 2))
  )
)
```

```output
42
```

### While conditions narrow inside the loop body.

```brix
(main
  (let value (? Int) 1)
  (let once true)
  (while (and value once)
    (print (+ value 1))
    (= once false)
  )
)
```

```output
2
```

### Assignment to a while condition local updates the loop carrier, not the narrowed shadow.

```brix
(main
  (let value (? Link) (call Link))
  (while value
    (= value (. value next))
  )
  (print "done")
)

(type + Link
  (prop + next (? Link))
)
```

```output
done
```

### Clause conditions can narrow loop body values.

```brix
(main
  (for (in value (list (call (of Optional Int) 1) (call (of Optional Int)) (call (of Optional Int) 3)) (when value))
    (print (+ value 1))
  )
)
```

```output
2
4
```

### Optional payload narrowing composes with union case narrowing.

```brix
(main
  (let event (? Event) (call (. Event click) 10 20))
  (if event
    (if (is event .click)
      (print (. event x))
      (print (. event y))
    )
  )
)

(type + Event (is Union)
  (variant click (tuple (: x Int) (: y Int)))
  (variant key (tuple String))
)
```

```output
10
20
```

### Optional payload and union case narrowing compose inside one && condition.

```brix
(main
  (let event (? Event) (call (. Event click) 10 20))
  (if (and event (is event .click) (> (. event x) 5))
    (print (. event y))
  )
)

(type + Event (is Union)
  (variant click (tuple (: x Int) (: y Int)))
  (variant key (tuple String))
)
```

```output
20
```

### A union a closure captures and tests is read there as the case the test found.

```brix
(main
  (call probe (call (. Spot edge) 2))
  (call probe (call (. Spot cell) "a"))
)

(fn-do + probe _
  (param spot Spot)
  (call run
    (:edit
      (=> () :do
        (block
          (if (is spot .edge)
            (print (+ spot 1))
          )
          (let open (and (is spot .edge) (> spot 0)))
          (print open)
        )
      )
    )
  )
)

(fn-do + run _
  (param edit (fn-type Void))
  (call edit)
)

(type + Spot (is Union)
  (variant edge (tuple (: side Int)))
  (variant cell (tuple (: name String)))
)
```

```output
3
true
false
```

### Anonymous unions narrow by payload type.

```brix
(main
  (let value Choice 41)
  (if (is value Int)
    (print (+ value 1))
    (print (type-of value))
  )
)

(type + Choice (is Union)
  (variant Int)
  (variant String)
)
```

```output
42
[TypeInfo I64]
```

### Else-if chains carry the negative facts from earlier branches.

```brix
(main
  (let value Choice "hello")
  (if (is value Int)
    (print (+ value 1))
    (else-if (is value String)
      (print (. value length))
      (print (type-of value))
    )
  )
)

(type + Choice (is Union)
  (variant Int)
  (variant String)
)
```

```output
5
[TypeInfo String]
```

### is-not narrows the true branch by subtracting the rejected type.

```brix
(main
  (let value Choice "hello")
  (if (is-not value Int)
    (print (. value length))
    (print (type-of value))
  )
)

(type + Choice (is Union)
  (variant Int)
  (variant String)
)
```

```output
5
[TypeInfo String]
```

### Guard-style is-not checks narrow the remainder.

```brix
(main
  (print (call length-or-zero "hello"))
  (print (call length-or-zero 42))
)

(fn-do + length-or-zero _
  (param value Choice)
  (if (is-not value String)
    (return 0)
  )
  (blank)
  (return (. value length))
)

(type + Choice (is Union)
  (variant Int)
  (variant String)
)
```

```output
5
0
```

### The right side of && sees union narrowing facts.

```brix
(main
  (let value Choice "hello")
  (if (and (is value String) (== (. value length) 5))
    (print value)
  )
)

(type + Choice (is Union)
  (variant Int)
  (variant String)
)
```

```output
hello
```

### The right side of || sees the negated union fact from the left side.

```brix
(main
  (let value Choice 41)
  (if (or (is-not value Int) (> value 40))
    (print "pass")
  )
)

(type + Choice (is Union)
  (variant Int)
  (variant String)
)
```

```output
pass
```

### Narrowing does not leak out of an ordinary branch.

```brix
(main
  (let value Choice 41)
  (if (is value Int)
    (print (+ value 1))
  )
  (blank)
  (print (type-of value))
)

(type + Choice (is Union)
  (variant Int)
  (variant String)
)
```

```output
42
[TypeInfo Choice]
```

### Nested scopes inherit active narrowing facts.

```brix
(main
  (let value Choice "hello")
  (if (is value String)
    (do
      (print (. value length))
      (print (type-of value))
    )
  )
)

(type + Choice (is Union)
  (variant Int)
  (variant String)
)
```

```output
5
[TypeInfo String]
```

### Assignment to a narrowed union name updates the original and narrows to the assigned case.

```brix
(main
  (let value Choice "hello")
  (if (is value String)
    (print (. value length))
    (= value "cat")
    (print (. value length))
  )
  (if (is value String)
    (print value)
  )
  (let other Choice "hello")
  (if (is other String)
    (= other 7)
    (print (+ other 1))
  )
  (if (is other Int)
    (print (+ other 2))
  )
)

(type + Choice (is Union)
  (variant Int)
  (variant String)
)
```

```output
5
3
cat
8
9
```

### Assigning a case value to a union local narrows later reads, and union members stay reachable.

```brix
(main
  (let value Choice 5)
  (= value "cat")
  (print (. value length))
  (print (. value type))
  (print (. value case))
  (= value 9)
  (print (+ value 1))
  (print (. value type))
)

(type + Choice (is Union)
  (variant Int)
  (variant String)
)
```

```output
3
Class:core/string:String
1
10
Class:core/number:I64
```

### Narrowing carries into a loop when every back edge keeps it, and re-checks see the current value.

```brix
(main
  (let value (? Int) 1)
  (if value
    (for (in step (list 1 2))
      (print (+ value step))
      (= value (+ value 10))
    )
  )
  (let count (? Int) 3)
  (if count
    (while (and count (> count 0))
      (print count)
      (= count (only-if (- count 1) (if (> count 1))))
    )
  )
  (let choice Choice "hi")
  (if (is choice String)
    (for (in step (list 1 2))
      (if (is choice String)
        (print (. choice length))
        (else
          (print "int")
        )
      )
      (= choice 4)
    )
  )
  (print (call guarded 3))
  (print (call guarded null))
)

(type + Choice (is Union)
  (variant Int)
  (variant String)
)

(fn-do + guarded Int
  (param start (? Int))
  (let value start)
  (return-if 0 (if (not value)))
  (let count 0)
  (while (< count 2)
    (+= count 1)
    (return-if -1 (if (not value)))
    (= value null)
  )
  (return count)
)
```

```output
2
13
3
2
1
2
int
-1
0
```

### Using carried narrowing in a loop whose back edge breaks it is an error.

```brix crash
(main
  (let value (? Int) 1)
  (if value
    (for (in step (list 1 2))
      (print (+ value 1))
      (= value null)
    )
  )
)
```

```output
Exception: Narrowing of 'value' is used in the loop but does not hold when the loop repeats
<no file:5>
        (for (in step (list 1 2))
        ^^^^^^^^^^^^^^^^^^^^^^^^
```

### Named union cases narrow to their payload value.

```brix
(main
  (let result Result (call (. Result success) 41))
  (if (is result .success)
    (print (+ result 1))
    (print (type-of result))
  )
)

(type + Result (is Union)
  (variant success Int)
  (variant failure String)
)
```

```output
42
[TypeInfo I64]
```

### Named union case narrowing distinguishes duplicate payload types.

```brix
(main
  (let value Foo (call (. Foo b) 41))
  (if (is value .b)
    (print (+ value 1))
  )
)

(type + Foo (is Union)
  (variant a Int)
  (variant b Int)
)
```

```output
42
```

### Named tuple payload cases narrow to the tuple payload with named fields.

```brix
(main
  (let event Event (call (. Event click) 10 20))
  (if (is event .click)
    (print (. event x))
    (print (. event y))
    (print (type-of event))
  )
)

(type + Event (is Union)
  (variant click (tuple (: x Int) (: y Int)))
  (variant key (tuple String))
)
```

```output
10
20
[TypeInfo Tuple]
```

### Exact named-case checks can flow into else-if cases.

```brix
(main
  (let event Event (call (. Event key) "K"))
  (if (is event .click)
    (print (. event x))
    (else-if (is event .key)
      (print event)
    )
  )
)

(type + Event (is Union)
  (variant click (tuple (: x Int) (: y Int)))
  (variant key (tuple String))
)
```

```output
K
```

### Structure-pattern captures keep the projected type.

```brix
(main
  (let value (call Foo))
  (if (= (| (: projected (similar Bar))) value)
    (print (type-of projected))
    (print (. projected name))
  )
)

(type + Foo
  (fn-to-expr + Bar
    (call Bar)
  )
)

(type + Bar
  (prop + name _ (= "bar"))
)
```

```output
[TypeInfo Bar]
bar
```

### Match-expression pattern captures keep the projected type.

```brix
(main
  (let value (call Foo))
  (print
    (match value
      (case (: projected (similar Bar))
        (str (type-of projected) ": " (. projected name))
      )
      (default
        "miss"
      )
    )
  )
)

(type + Foo
  (fn-to-expr + Bar
    (call Bar)
  )
)

(type + Bar
  (prop + name _ (= "bar"))
)
```

```output
[TypeInfo Bar]: bar
```

### Optional pattern captures unwrap and project to the captured payload type.

```brix
(main
  (let value (? Int) 41)
  (if (= (| (: projected (similar Float))) value)
    (print (+ projected 1.5))
    (print (type-of projected))
  )
)
```

```output
42.5
[TypeInfo F64]
```

### Captures from named union patterns expose typed payload fields in the body.

```brix
(main
  (let submission Authorization (call (. Authorization valid) "joe@example.com" "secret"))
  (if (= (| (call .valid email password)) submission)
    (print email)
    (print (. password length))
  )
)

(type + Authorization (is Union)
  (variant invalid-password)
  (variant invalid-email)
  (variant valid (tuple (: email String) (: password String)))
)
```

```output
joe@example.com
6
```

### Guards narrow locals the block assigns without copying them, so updates persist across iterations.

```brix
(main
  (print (call guard-loop 1))
  (print (call guard-loop null))
  (print (call guard-union))
  (print (call case-chain))
  (print (call case-match))
)

(type + Choice (is Union)
  (variant Int)
  (variant String)
)

(type + Event (is Union)
  (variant click (tuple (: x Int) (: y Int)))
  (variant key (tuple String))
)

(fn-do + guard-loop Int
  (param start (? Int))
  (let total start)
  (for (in step (list 1 2 3))
    (return-if -1 (if (not total)))
    (= total (+ total step))
  )
  (return (?? total 0))
)

(fn-do + guard-union Int
  (let value Choice "hi")
  (let count 0)
  (for (in step (list 1 2 3))
    (return-if -1 (if (is value Int)))
    (+= count (. value length))
    (= value "hello")
  )
  (return count)
)

(fn-do + case-chain String
  (let event Event (call (. Event key) "K"))
  (let out "")
  (if (is event .click)
    (= out "click")
    (else-if (is event .key)
      (= out "key")
    )
  )
  (= event (call (. Event click) 1 2))
  (return out)
)

(fn-do + case-match String
  (let event Event (call (. Event key) "K"))
  (return-if "click" (if (is event .click)))
  (let out
    (match event
      (case (call .click click)
        "click"
      )
      (case (call .key key)
        key
      )
    )
  )
  (= event (call (. Event click) 1 2))
  (return out)
)
```

```output
7
-1
12
key
K
```

### A presence check reads only the checked local unnarrowed, not locals its lazy initializer reads.

```brix
(main
  (print (call first-x true (call Pair 3 4)))
  (print (call first-x false (call Pair 3 4)))
  (print (call first-x true null))
)

(type + Pair (is (tuple (: x Int) (: y Int))))

(fn-expr + first-x Int
  (param use Bool)
  (param pair (? Pair))
  (case? (not pair)
    0
  )
  (case? (not picked)
    -1
  )
  (default
    picked
  )
  (where
    (lazy picked (only-if (. pair x) (if use)))
  )
)
```

```output
3
-1
0
```

### Carried narrowing does not survive past a loop that can break after breaking it.

```brix crash
(main
  (let x (? Int) 5)
  (if x
    (for (in i (list 1 2))
      (if (== i 1)
        (= x null)
        (break)
      )
    )
    (print (+ x 1))
  )
)
```

```output
Exception: Operator "+" is not defined for I64? and I64
<no file:11>
        (print (+ x 1))
               ^^^^^^^
```

### Match on a union narrowed by an earlier guard still sees the union case.

```brix
(main
  (print (call case-of (call (. Event key) "K")))
  (print (call case-of (call (. Event click) 1 2)))
  (print (call optional-case-of (call (. Event key) "J")))
  (print (call optional-case-of null))
)

(type + Event (is Union)
  (variant click (tuple (: x Int) (: y Int)))
  (variant key (tuple String))
)

(fn-do + case-of String
  (param event Event)
  (return-if "click" (if (is event .click)))
  (return
    (match event
      (case (call .click click)
        "click"
      )
      (case (call .key key)
        key
      )
    )
  )
)

(fn-do + optional-case-of String
  (param event (? Event))
  (return-if "none" (if (not event)))
  (return-if "click" (if (is event .click)))
  (return
    (match event
      (case (call .click click)
        "click"
      )
      (case (call .key key)
        key
      )
    )
  )
)
```

```output
K
click
J
none
```

### Comparing a union with == or != against a payload-free case narrows like is.

```brix
(main
  (let color Paint (call Swatch "abc"))
  (let none Paint (. Paint none))
  (print (call chain-hex color))
  (print (call chain-hex (. Paint inherit)))
  (print (call chain-hex none))
  (print (call both-not color))
  (print (call both-not none))
  (print (call reversed color))
  (print (call reversed none))
  (print (call symbol-else color))
  (print (call symbol-else (. Paint inherit)))
  (print (call valued (. Shade light)))
  (print (call valued (call Swatch "def")))
)

(type + Swatch
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
  (fn-to-expr + String
    (str "swatch(" text ")")
  )
  (fn-to-expr + Hex
    (str "#" text)
  )
)

(type + Paint (is Union)
  (variant inherit)
  (variant none)
  (variant Swatch)
)

(type + Shade (is Union)
  (variant light (= 1))
  (variant dark (= 2))
  (variant Swatch)
)

(fn-expr + chain-hex String
  (param paint Paint)
  (case? (== paint (. Paint inherit))
    "inherit"
  )
  (case? (== paint (. Paint none))
    "none"
  )
  (default
    (as paint Hex)
  )
)

(fn-do + both-not String
  (param paint Paint)
  (if (and (!= paint (. Paint inherit)) (!= paint (. Paint none)))
    (return (. paint text))
  )
  (return "named")
)

(fn-do + reversed String
  (param paint Paint)
  (return-if "plain" (if (or (== (. Paint inherit) paint) (== (. Paint none) paint))))
  (return (. paint text))
)

(fn-do + symbol-else String
  (param paint Paint)
  (if (or (== paint .inherit) (== paint .none))
    (return "named")
    (else
      (return (. paint text))
    )
  )
)

(fn-do + valued String
  (param shade Shade)
  (if (== shade (. Shade light))
    (return "light")
  )
  (return (str (type-of shade)))
)
```

```output
#abc
inherit
none
abc
named
abc
plain
abc
named
light
[TypeInfo Shade]
```

### Narrowing carried across a loop survives nested loops and breaks that keep it.

```brix
(main
  (print (call nested-for))
  (print (call nested-while))
  (print (call kept-break))
)

(fn-do + nested-for Int
  (let x (? Int) 3)
  (let total 0)
  (if x
    (for (in i (..< 0 2))
      (+= total x)
      (for (in j (..< 0 2))
        (= x (+ x 1))
      )
    )
  )
  (return total)
)

(fn-do + nested-while Int
  (let x (? Int) 3)
  (let total 0)
  (let n 0)
  (if x
    (while (< n 2)
      (+= n 1)
      (+= total x)
      (let m 0)
      (while (< m 2)
        (+= m 1)
        (= x (+ x 1))
      )
    )
  )
  (return total)
)

(fn-do + kept-break Int
  (let x (? Int) 3)
  (let n 0)
  (if x
    (while (< n 3)
      (+= n 1)
      (if (== n 2)
        (= x 9)
        (break)
      )
    )
    (return (+ x 1))
  )
  (return 0)
)
```

```output
8
8
10
```

### A break that clears the value ends the narrowing after the loop.

```brix crash
(main
  (print (call run))
)

(fn-do + run Int
  (let x (? Int) 3)
  (let n 0)
  (if x
    (while (< n 3)
      (+= n 1)
      (if (== n 2)
        (= x null)
        (break)
      )
    )
    (return (+ x 1))
  )
  (return 0)
)
```

```output
Exception: Operator "+" is not defined for I64? and I64
<no file:17>
        (return (+ x 1))
                ^^^^^^^
```

### A type test that can never match skips its branch, as match skips impossible cases.

```brix
(main
  (let box (call NarrowingBox))
  (call (. box run) bump)
  (call (. box run) bump-or-fail)
  (call (. box run) bump-and-return)
  (print (. box count))
  (print (. box copied))
  (print (. box failures))
)

(type + NarrowingFailure (is Error))

(fn-do + bump _
  (param box NarrowingBox)
  (+= (. box count) 1)
)

(fn-do + bump-or-fail _ (?! NarrowingFailure)
  (param box NarrowingBox)
  (+= (. box count) 1)
  (if (== (. box count) 2)
    (throw (call NarrowingFailure "second"))
  )
)

(fn-do + bump-and-return NarrowingBox
  (param box NarrowingBox)
  (+= (. box count) 1)
  (return box)
)

(type + NarrowingBox
  (prop + count _ (= 0))
  (prop + copied _ (= 0))
  (prop + failures _ (= 0))
  (fn-do + run _
    (param body _)
    (let result (catch (call body this)))
    (if (is result Error)
      (+= failures 1)
      (else-if (is result NarrowingBox)
        (= copied (. result count))
      )
    )
  )
)
```

```output
3
3
1
```

### A type test that can never match short-circuits the rest of an && condition.

```brix
(main
  (print (call describe-count 12345))
  (print (call describe-count (call NarrowingCounter 5)))
  (print (call describe-count (call NarrowingCounter 2)))
)

(fn-do + describe-count String
  (param value _)
  (if (and (is value NarrowingCounter) (> (. value count) 3))
    (return "many")
  )
  (return "other")
)

(type + NarrowingCounter
  (prop + count _ (= 0))
  (fn-do + This _
    (param count _)
  )
)
```

```output
other
many
other
```

### A property of a struct that a test found to hold a value is read as what it holds, as an index too.

```brix
(main
  (print (call run 4))
  (print (call run 9))
)

(fn-do + run Int
  (param at Index)
  (let probe (call find at))
  (let slots (call (of Array Int) 8))
  (if (. probe found)
    (= ([] slots (. probe found)) 7)
    (return (+ ([] slots 4) 1))
  )
  (return 0)
)

(fn-expr + find Probe
  (param at Index)
  (call Probe (only-if at (if (< at 8))) 2)
)

(type + Probe (is Struct)
  (prop + found (? Index))
  (prop + slot Index)
)
```

```output
8
0
```

### An optional local known to hold a value still holds one after an operator that assigns nothing

```brix
(main
  (let list (call find))
  (if list
    (+= list 3)
    (print (. list size))
  )
)

(fn-do + find (? (list Int))
  (return (list 1 2))
)
```

```output
3
```

### is and is not between a union and the types and cases it may hold

```brix
(main
  (print (call kind (call .count 1)))
  (print (call kind (call .wide 2.5)))
  (print (call kind (call .named "n")))
  (print (call kind .empty))
  (print (call test (call .count 1)))
  (print (call test .empty))
  (let s (? Shape) (. Shape empty))
  (print (is s .empty))
  (print (is s .count))
  (let t (? Shape))
  (print (is t .empty))
  (print (is-not t .empty))
)

(fn-do + kind String
  (param shape Shape)
  (let text "")
  (if (is shape Int)
    (++= text "int ")
  )
  (if (is shape Float)
    (++= text "float ")
  )
  (if (is shape String)
    (++= text "string ")
  )
  (if (is-not shape Int)
    (++= text "not-int ")
  )
  (if (is shape .empty)
    (++= text "empty ")
  )
  (if (is-not shape .named)
    (++= text "not-named ")
  )
  (return text)
)

(fn-expr + test Bool
  (param shape Shape)
  (or (is shape .count) (is shape .wide))
)

(type + Shape (is Union)
  (variant count Int)
  (variant wide Float)
  (variant named String)
  (variant empty)
)
```

```output
int not-named 
float not-int not-named 
string not-int 
not-int empty not-named 
true
false
true
false
false
true
```

### A union read as the case an is test found

```brix
(main
  (print (call grow (call .count 1)))
  (print (call grow (call .named "n")))
  (print (call size (call .count 5)))
  (print (call size (call .named "abc")))
  (print (call size .empty))
)

(fn-do + grow String
  (param shape Shape)
  (if (is shape Int)
    (return (str "int " (+ shape 1)))
  )
  (if (and (is shape String) (== (. shape length) 1))
    (return (str "one " shape))
  )
  (return "other")
)

(fn-do + size Int
  (param shape Shape)
  (if (is-not shape String)
    (return -1)
  )
  (return (. shape length))
)

(type + Shape (is Union)
  (variant count Int)
  (variant named String)
  (variant empty)
)
```

```output
int 2
one n
-1
3
-1
```

### A union known to hold one case is tested and matched as the union

```brix
(main
  (print (call trace (call .click 1 2)))
  (print (call trace (call .key "K")))
  (print (call trace .idle))
  (print (call again (call .key "Q")))
  (print (call again (call .click 3 4)))
  (print (call pair (call .left 1)))
  (print (call pair (call .right "r")))
)

(fn-do + trace String
  (param event Event)
  (if (is event .click)
    (return (str "click " (. event x)))
    (else-if (is event .key)
      (return (str "key " event))
    )
  )
  (return "idle")
)

(fn-do + again String
  (param event Event)
  (if (is-not event .click)
    (if (is event .idle)
      (return "idle")
    )
    (return
      (match event
        (case (call .key text)
          (str "key " text)
        )
        (default
          "none"
        )
      )
    )
  )
  (return
    (match event
      (case (call .click x y)
        (str "click " x " " y)
      )
      (default
        "none"
      )
    )
  )
)

(fn-do + pair String
  (param side Side)
  (if (is side .left)
    (return (str "left " (+ side 1)))
  )
  (return
    (match side
      (case Int
        (str "int " it)
      )
      (case String
        (str "text " it)
      )
    )
  )
)

(type + Side (is Union)
  (variant left Int)
  (variant right String)
)

(type + Event (is Union)
  (variant click (tuple (: x Int) (: y Int)))
  (variant key (tuple String))
  (variant idle)
)
```

```output
click 1
key K
idle
key Q
click 3 4
left 2
text r
```

### An optional of a union known to hold a value and one case is read as that case, in arguments and locals.

```brix
(main
  (print (call show null))
  (print (call show (call (. Event click) 1 2)))
  (print (call show (call (. Event key) "k")))
  (print (call both (call (. Event click) 3 4) (call (. Event key) "z")))
  (print (call both null (call (. Event click) 5 6)))
  (let event (? Event) (call (. Event key) "q"))
  (if (and event (is event .key))
    (print event)
  )
  (= event (call again (call (. Event click) 7 8)))
  (if event
    (if (is event .click)
      (print (+ (. event x) (. event y)))
      (= event null)
      (print (== event null))
    )
  )
)

(fn-do + again (? Event)
  (param event (? Event))
  (return event)
)

(fn-do + show String
  (param event (? Event))
  (if (and event (is event .click))
    (return (str "click " (. event x) "," (. event y)))
  )
  (if (not event)
    (return "none")
  )
  (if (is event .key)
    (return (str "key " event))
  )
  (return "other")
)

(fn-do + both String
  (param first (? Event))
  (param second (? Event))
  (if (and first second (is first .click) (is second .key))
    (return (str (. first x) " " second))
  )
  (if (and second (is second .click))
    (return (str "second " (. second y) " " (== first null)))
  )
  (return "neither")
)

(type + Event (is Union)
  (variant click (tuple (: x Int) (: y Int)))
  (variant key (tuple String))
)
```

```output
none
click 1,2
key k
3 z
second 6 true
q
15
true
```

### A type test that can never hold leaves the operand it guards and the branch it guards out

```brix
(main
  (print (call describe 12345))
  (print (call describe (call Counter 5)))
  (print (call describe (call Counter 2)))
  (print (call either 7))
  (print (call either (call Counter 1)))
  (print (call either (call Counter 8)))
  (print (call nested 3))
  (print (call nested 1))
  (print (call nested (call Counter 9)))
  (print (call nested "text"))
)

(fn-do + describe String
  (param value _)
  (if (and (is value Counter) (> (. value count) 3))
    (return "many")
  )
  (return "other")
)

(fn-do + either String
  (param value _)
  (if (or (is-not value Counter) (> (. value count) 3))
    (return "not few")
  )
  (return "few")
)

(fn-do + nested String
  (param value _)
  (if (is value Counter)
    (if (> (. value count) 3)
      (return "many")
    )
    (return "few")
    (else-if (and (is value Int) (> value 2))
      (return "int")
    )
  )
  (return "other")
)

(type + Counter
  (prop + count _ (= 0))
  (fn-do + This _
    (param count _)
  )
)
```

```output
other
many
other
not few
few
not few
int
other
many
other
```

### A union local that an if assigns one case and whose condition leaves the same case otherwise is that case after it

```brix
(main
  (let value Choice "hello")
  (if (is value String)
    (= value 7)
  )
  (print (+ value 1))
  (let other Choice 5)
  (if (is other String)
    (= other 9)
  )
  (print (+ other 1))
  (let third Choice "x")
  (if (is third Int)
    (= third "was int")
  )
  (print (. third length))
  (print (call settle "four"))
  (print (call settle 10))
  (print (call text-of 3))
  (print (call text-of "kept"))
)

(type + Choice (is Union)
  (variant Int)
  (variant String)
)

(fn-do + settle Int
  (param given Choice)
  (let choice given)
  (if (is choice String)
    (= choice (. choice length))
  )
  (if (is choice Int)
    (return (+ choice 1))
  )
  (return -1)
)

(fn-do + text-of String
  (param given Choice)
  (let choice given)
  (if (is choice Int)
    (= choice "number")
  )
  (return (str choice "!"))
)
```

```output
8
6
1
5
11
number!
kept!
```

### What a test found of a property ends at a print, a loop and a call, and holds past an assertion with a condition, a branch that leaves and an assignment to a local.

```brix
(main
  (let holder (call Holder))
  (call (. holder after-print))
  (call (. holder after-loop))
  (call (. holder after-call))
  (call (. holder after-assertion) 1)
  (call (. holder after-leaving) 1)
  (call (. holder after-local) 2)
)

(type + Holder
  (prop + value (? Int) (= 0))
  (prop + count Int (= 0))
  (fn-do + after-print _
    (return-if (if (not value)))
    (print "print")
    (print (type-of value))
  )
  (fn-do + after-loop _
    (return-if (if (not value)))
    (for (in at-turn (..< 0 2))
      (let next (+ at-turn 1))
    )
    (print (type-of value))
  )
  (fn-do + after-call _
    (return-if (if (not value)))
    (call touch)
    (print (type-of value))
  )
  (fn-do + after-assertion _
    (param given Int)
    (return-if (if (not value)))
    (throw (call Panic "negative") (if (< given 0)))
    (print (type-of value))
  )
  (fn-do + after-leaving _
    (param given Int)
    (return-if (if (not value)))
    (if (< given 0)
      (call touch)
      (return)
    )
    (print (type-of value))
  )
  (fn-do + after-local _
    (param given Int)
    (return-if (if (not value)))
    (let sum (+ given 1))
    (= sum (* sum 2))
    (print (type-of value))
  )
  (fn-do + touch _
    (+= count 1)
  )
)
```

```output
print
[TypeInfo Optional'I64]
[TypeInfo Optional'I64]
[TypeInfo Optional'I64]
[TypeInfo I64]
[TypeInfo I64]
[TypeInfo I64]
```

### What a test found of a property is read as what it holds until something ends it.

```brix
(main
  (let holder (call Holder))
  (call (. holder after-print))
  (call (. holder after-loop))
  (call (. holder after-call))
  (call (. holder after-assertion) 1)
  (call (. holder after-leaving) 1)
  (call (. holder after-local) 2)
  (call (. holder after-store))
)

(type + Holder
  (prop + value (? Int) (= 0))
  (prop + count Int (= 0))
  (fn-do + after-print _
    (return-if (if (not value)))
    (print "print")
    (let read (?? value 9))
    (print read)
  )
  (fn-do + after-loop _
    (return-if (if (not value)))
    (for (in at-turn (..< 0 2))
      (let next (+ at-turn 1))
    )
    (let read (?? value 9))
    (print read)
  )
  (fn-do + after-call _
    (return-if (if (not value)))
    (call touch)
    (let read (?? value 9))
    (print read)
  )
  (fn-do + after-assertion _
    (param given Int)
    (return-if (if (not value)))
    (throw (call Panic "negative") (if (< given 0)))
    (let read (?? value 9))
    (print read)
  )
  (fn-do + after-leaving _
    (param given Int)
    (return-if (if (not value)))
    (if (< given 0)
      (call touch)
      (return)
    )
    (let read (?? value 9))
    (print read)
  )
  (fn-do + after-local _
    (param given Int)
    (return-if (if (not value)))
    (let sum (+ given 1))
    (= sum (* sum 2))
    (let read (?? value 9))
    (print read)
  )
  (fn-do + after-store _
    (return-if (if (not value)))
    (= count 1)
    (let read (?? value 9))
    (print read)
  )
  (fn-do + touch _
    (+= count 1)
  )
)
```

```output
print
0
0
0
9
9
9
9
```

### What a test found of a field of a tuple read through a local holds past a loop, a print and a call.

```brix
(main
  (let holder (call Holder (list 1 2)))
  (if (. holder items)
    (for (in item (. holder items))
      (print item)
    )
    (for (in item (. holder items))
      (print item)
    )
    (print (call (. "," join) (in (str each) (. holder items))))
    (print (call (. "," join) (in (str each) (. holder items))))
  )
)

(type + Holder (is Tuple)
  (prop + items (? (list Int)) (= null))
)
```

```output
1
2
1
2
1,2
1,2
```

### A field of a tuple read through a local that a test found to hold one case of a union is read as what the case holds, past a call too.

```brix
(main
  (let shaped (call Shaped (call (. Shape circle) 2.0) (call (. Event click) 3 4)))
  (if (is (. shaped shape) .circle)
    (call touch)
    (print (+ (. shaped shape) 1.0))
  )
  (if (is (. shaped event) .click)
    (call touch)
    (print (+ (. shaped event x) (. shaped event y)))
  )
)

(fn-do + touch _
  (print "touch")
)

(type + Shaped (is Tuple)
  (prop + shape Shape)
  (prop + event Event)
)

(type + Shape (is Union)
  (variant circle (tuple (: radius Float)))
  (variant square (tuple (: side Float)))
)

(type + Event (is Union)
  (variant click (tuple (: x Int) (: y Int)))
  (variant key (tuple String))
)
```

```output
touch
3
touch
7
```

### An assignment to a property leaves it as it is declared, and one to another property named alone leaves it as the test found it.

```brix
(main
  (let holder (call Holder))
  (call (. holder after-other-property))
  (call (. holder after-adding))
  (call (. holder after-own))
  (call (. holder after-receiver))
  (call (. holder after-null))
)

(type + Holder
  (prop + value (? Int) (= 0))
  (prop + count Int (= 0))
  (fn-do + after-other-property _
    (return-if (if (not value)))
    (= count 1)
    (print (type-of value))
  )
  (fn-do + after-adding _
    (return-if (if (not value)))
    (+= count 1)
    (print (type-of value))
  )
  (fn-do + after-own _
    (return-if (if (not value)))
    (= value 4)
    (print (type-of value))
  )
  (fn-do + after-null _
    (return-if (if (not value)))
    (= value null)
    (print (type-of value))
  )
  (fn-do + after-receiver _
    (return-if (if (not value)))
    (= (. this value) 4)
    (print (type-of value))
  )
)
```

```output
[TypeInfo I64]
[TypeInfo I64]
[TypeInfo Optional'I64]
[TypeInfo Optional'I64]
[TypeInfo Optional'I64]
```

### A local that a closure assigns keeps what is known of it until a call.

```brix
(main
  (let value (? Int))
  (= value 7)
  (let clear
    (=> () :do
      (block
        (= value null)
      )
    )
  )
  (print (type-of value))
  (let noop (=> () 1))
  (call noop)
  (print (?? value 0))
  (call clear)
  (print (?? value 0))
)
```

```output
[TypeInfo I64]
7
0
```

### A call of a closure that assigns a narrowed local ends what a test found.

```brix
(main
  (let value (? Int))
  (= value (call maybe))
  (let clear
    (=> () :do
      (block
        (= value null)
      )
    )
  )
  (if value
    (print (+ value 1))
    (call clear)
    (print (?? value -1))
  )
)

(fn-do + maybe (? Int)
  (return 7)
)
```

```output
8
-1
```

### A closure that assigns another local leaves a narrowed one as it was.

```brix
(main
  (let count 5)
  (let held (? Int) count)
  (let grow
    (=> () :do
      (block
        (+= count 1)
      )
    )
  )
  (if held
    (call grow)
    (print (+ held count))
  )
)
```

```output
11
```

### `is~` of a list and a list of another item answers whether the value converts.

```brix
(main
  (print (is~ (list 1 2 3) (of List Float)))
  (print (is~ (list 1 2 3) (of List Int)))
  (print (is~ (list 1 2 3) (of List String)))
  (print (is~ (list 1.5) (of List Int)))
  (let list (list 1 2 3))
  (print (is~ list (of List Float)))
  (print (is~ list (of List Int)))
  (print (is~ 1 Float))
  (print (is~ 1 Int))
)
```

```output
true
true
true
true
true
true
true
true
```

### `is`, `is!` and `is~` between classes derived from one another and between lists.

```brix
(main
  (let a A (call B))
  (print (is a B))
  (print (is a C))
  (print (is a A))
  (let b (call B))
  (print (is b A))
  (print (is b B))
  (print (is! b A))
  (print (is! b B))
  (let list (list 1 2))
  (print (is list (of List Int)))
  (print (is! list (of List Float)))
  (print (is~ list (of List Float)))
  (print (is list (of List A)))
)

(type + A)

(type + B (is A))

(type + C (is A))
```

```output
true
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
```

### `is!` and a pattern written `Animal!` test the class of the object, not the type a local is declared with: an object of a class derived from the declared one is not exactly of it.

```brix
(main
  (let pets (list Animal) (list (call Animal) (call Dog)))
  (for (in pet pets)
    (print (str (is! pet Animal) " " (is! pet Dog)))
    (match pet
      (case (exact Animal)
        (print "exactly an animal")
      )
      (default
        (print "derived")
      )
    )
  )
  (let maybe (? Animal) (call Dog))
  (print (is! maybe Animal))
)

(type + Animal)

(type + Dog (is Animal))
```

```output
true false
exactly an animal
false true
derived
false
```

### What a call ended of a property stays ended after a test of another property.

```brix
(main
  (let holder (call Holder))
  (call (. holder retest))
)

(type + Holder
  (prop + first (? Int) (= 0))
  (prop + second (? Int) (= 1))
  (prop + count Int (= 0))
  (fn-do + retest _
    (return-if (if (not first)))
    (call touch)
    (return-if (if (not second)))
    (let second-type (type-of second))
    (let first-type (type-of first))
    (print second-type)
    (print first-type)
  )
  (fn-do + touch _
    (+= count 1)
  )
)
```

```output
[TypeInfo I64]
[TypeInfo Optional'I64]
```

### A property tested before a method call on it stays narrowed for the receiver when an argument calls a function

```brix
(main
  (let holder (call Holder))
  (print (call (. holder depth)))
  (= (. holder node) (call Node))
  (print (call (. holder depth)))
)

(type + Node
  (prop + n Int (= 4))
  (fn-expr + plus Int
    (param k Int)
    (+ n k)
  )
)

(type + Holder
  (prop + node (? Node))
  (fn-expr + depth Int
    (case? node
      (call (. node plus) (call seven))
    )
    (default
      0
    )
  )
)

(fn-expr + seven Int
  7
)
```

```output
0
11
```

### A property stored in a local is no longer narrowed by the test before the store

```brix
(main
  (let holder (call Holder))
  (= (. holder node) (call Node))
  (call (. holder show))
)

(type + Node
  (prop + n Int (= 4))
)

(type + Holder
  (prop + node (? Node))
  (fn-do + show _
    (return-if (if (not node)))
    (print (type-of node))
    (let kept node)
    (print (type-of node))
    (print (type-of kept))
  )
)
```

```output
[TypeInfo Node]
[TypeInfo Optional'Node]
[TypeInfo Optional'Node]
```

### A property known to hold a value keeps it in an assignment that adds to a property of it.

```brix
(main
  (let r (call Recorder))
  (= (. r chunk) (call Chunk))
  (call (. r push))
  (print (. (!! (. r chunk)) items length))
)

(type + Chunk
  (prop + items (list I32))
)

(type + Recorder
  (prop + chunk (? Chunk))
  (fn-do + push _
    (if chunk
      (+= (. chunk items) (call next))
    )
  )
)

(fn-expr + next I32
  3
)
```

```output
1
```

### A local an `if` leaves present through a match whose other arms return is present after it.

```brix
(main
  (print (?? (call read null 4) 0))
  (print (?? (call read (call Box 7) 4) 0))
  (print (?? (call read null 0) 0))
)

(fn-do + read (? U32)
  (param given (? Box))
  (param size U32 (= 0))
  (let value given)
  (if (not value)
    (match size
      (case= 0
        (return null)
      )
      (default
        (= value (call Box size))
      )
    )
  )
  (return (. value size))
)

(type + Box
  (prop + size U32)
  (fn-do + This _
    (param size U32)
  )
)
```

```output
4
7
0
```

### Reading a field of a tuple by a constant index keeps what a test found of a property.

```brix
(main
  (print (call (. (call Snap 7) find) (tuple 1 7)))
  (print (call (. (call Snap 7) find) (tuple 1 3)))
  (print (call (. (call Snap null) find) (tuple 1 7)))
)

(type + Snap
  (prop _ primary-id (? U64))
  (fn-do + This _
    (param primary-id _)
  )
  (fn-expr + find String
    (param pair (tuple Index U64))
    (if-expr (and primary-id (== ([] pair 1) primary-id)) (str "found " ([] pair 0)) "none")
  )
)
```

```output
found 1
none
none
```

### Arithmetic between the test of a property and its read keeps what the test found: dividing calls nothing.

```brix
(main
  (let doc (call Doc))
  (print (call (. doc fill)))
)

(type + Doc
  (prop _ name (? String) (= "abcdef"))
  (prop _ buffer ([] U32) (= ([] U32 6)))
  (fn-do + fill Size
    (return-if 0 (if (not name)))
    (let capacity (/ (. buffer size) 3))
    (return (+ capacity (. name length)))
  )
)
```

```output
8
```

### `is` with a base of a case's class narrows a union to that case: inside the test, a caught error reads as its case, which casts up to the base.

```brix
(main
  (for (in text (list "good" "bad"))
    (let read (catch (call parse text)))
    (if (is read Error)
      (print (str "error: " (. (as read Error) message)))
      (else
        (print (str "value: " (. (as read Doc) name)))
      )
    )
  )
)

(type + Doc
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
)

(type + ParseError (is Error))

(fn-do + parse Doc (?! ParseError)
  (param text String)
  (throw (call ParseError (str "cannot read " text)) (if (== text "bad")))
  (return (call Doc text))
)
```

```output
value: good
error: cannot read bad
```

### A test for a class derived from the class of a union's case leaves the case on its false edge, since a value of the case may be of another class.

```brix
(main
  (print (call describe 0))
  (print (call describe 1))
  (print (call describe 2))
)

(type + Special (is Error)
  (fn-do + This _
    (param text String)
    (= message text)
  )
)

(fn-do _ pick I64 (?! Error)
  (param n I64)
  (throw (call Special "special") (if (== n 1)))
  (throw (call Error "plain") (if (== n 2)))
  (return (+ n 10))
)

(fn-do _ describe String
  (param n I64)
  (let result (catch (call pick n)))
  (if (is result Special)
    (return (str "special " (. (as result Special) message)))
  )
  (if (is result Error)
    (return (str "error " (. (as result Error) message)))
  )
  (return (str "value " (as result I64)))
)
```

```output
value 10
special special
error plain
```

### A property a conditional tests again after a call holds a value in the branch that finds one, though the call ended what an earlier test found of it.

```brix
(main
  (let holder (call Holder))
  (= (. holder name) "four")
  (print (call (. holder measure)))
  (= (. holder name) null)
  (print (call (. holder measure)))
)

(type + Holder
  (prop + name (? String))
  (fn-do + This _)
  (fn-do + measure Size
    (return-if 0 (if (not name)))
    (let count (call twice 1))
    (return (if-expr name (+ (. name length) count) 1))
  )
)

(fn-expr _ twice Size
  (param value Size)
  (* value 2)
)
```

```output
6
0
```

### A test of a value for a case of a union named alone calls nothing, so what a test found of a property holds past it.

```brix
(main
  (call (. (call Holder) run) (call (. Shape circle) 1.0))
)

(type + Holder
  (prop + value (? Int) (= 0))
  (fn-do + run _
    (param shape Shape)
    (return-if (if (not value)))
    (let round (is shape .circle))
    (let kind (type-of value))
    (print kind)
    (print round)
  )
)

(type + Shape (is Union)
  (variant circle (tuple (: radius Float)))
  (variant square (tuple (: side Float)))
)
```

```output
[TypeInfo I64]
true
```

### A float `min` or `max` of the math module is answered in place, with no call, so what a test found of a property holds past it.

```brix
(import /math)
(blank)

(main
  (let holder (call Holder))
  (= (. holder maximum) 2.0)
  (print (call (. holder bounded) 5.0))
  (print (call (. holder bounded) 1.0))
)

(type + Holder
  (prop + maximum (? F64))
  (fn-do + This _)
  (fn-do + bounded F64
    (param candidate F64)
    (return (if-expr maximum (call max (call min candidate 9.0) maximum) candidate))
  )
)
```

```output
5
2
```

### A row of a table read at an index is read in place, with no call, so what a test found of a property holds past it.

```brix
(import /core/table)
(blank)

(main
  (let holder (call Holder))
  (= (. holder name) "four")
  (print (call (. holder measure) 0))
  (print (call (. holder measure) 1))
  (= (. holder name) null)
  (print (call (. holder measure) 0))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(type + Holder
  (prop + name (? String))
  (prop + points (of Table Point) (= (call (of Table Point) 2 (:with (call Point (as each I32) 0)))))
  (fn-do + This _)
  (fn-do + measure Size
    (param at Size)
    (return-if 0 (if (not name)))
    (if (== (. ([] points at) x) 0)
      (return (. name length))
    )
    (return 1)
  )
)
```

```output
4
1
0
```

### A force of an optional the flow does not know to hold a value may panic, which ends what is known of properties; a property read after it is forced again.

```brix
(main
  (let holder (call Holder))
  (= (. holder name) "four")
  (= (. holder other) "ab")
  (print (call (. holder measure)))
  (= (. holder name) null)
  (print (call (. holder measure)))
)

(type + Holder
  (prop + name (? String))
  (prop + other (? String))
  (fn-do + This _)
  (fn-do + measure Size
    (return-if 0 (if (not name)))
    (let given (!! other))
    (return (+ (. (!! name) length) (. given length)))
  )
)
```

```output
6
0
```

### A local declared with the type its value already has is taken as it is, with no test that may panic, so what a test found of a property holds past it.

```brix
(main
  (let holder (call Holder))
  (= (. holder name) "four")
  (print (call (. holder measure)))
  (= (. holder name) null)
  (print (call (. holder measure)))
)

(type + Holder
  (prop + name (? String))
  (fn-do + This _)
  (fn-do + measure I64
    (return-if 0 (if (not name)))
    (let extra I64 2i64)
    (return (+ (as (. name length) I64) extra))
  )
)
```

```output
6
0
```

### A read of a lazy property calls for its value, which ends what a test found of a property.

```brix crash
(main
  (let holder (call Holder))
  (= (. holder name) "four")
  (print (call (. holder measure)))
)

(type + Helper
  (prop + count Size (= 2))
  (fn-do + This _)
)

(type + Holder
  (prop + name (? String))
  (prop _ helper _ (:= (call Helper)))
  (fn-do + This _)
  (fn-do + measure Size
    (return-if 0 (if (not name)))
    (let found (. helper count))
    (return (+ (. name length) found))
  )
)
```

```output
Exception: Property "length" is read from String?, which may be null; unwrap it first with "(!! …)" or "(?. …)"
<no file:20>
        (return (+ (. name length) found))
                   ^^^^^^^^^^^^^^^
```

### A condition reads an array's items by integer index, not as fields of a tuple.

```brix
(main
  (let values ([] U64 4))
  (= ([] values 2) 5)
  (if (or ([] values 0) ([] values 2))
    (print ([] values 2))
  )
)
```

```output
5
```

### A map-filter stage reads its item as the union case its condition narrowed it to.

```brix
(main
  (let parts (call (of List Part)))
  (+= parts (call Leaf "a"))
  (+= parts (call Twig 2))
  (+= parts (call Leaf "b"))
  (for (in leaf (call leaves parts))
    (print (. leaf name))
  )
)

(type + Leaf
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
)

(type + Twig
  (prop + size Int)
  (fn-do + This _
    (param size _)
  )
)

(type + Part (is Union)
  (variant Leaf)
  (variant Twig)
)

(fn-expr + leaves (list Leaf)
  (param parts (list Part))
  (spread parts)
  (pipe ?*> (only-if (as each Leaf) (if (is each Leaf))))
  (pipe |> List)
)
```

```output
a
b
```

### A property written without a type is narrowed by a test as one with its type is.

```brix
(main
  (let holder (call Holder))
  (print (call (. holder has-paths)))
  (print (call (. holder has-tracked-paths)))
)

(type + Change
  (prop + paths _ (= 2))
)

(type + Holder
  (prop + source (? Change) (= (call Change)))
  (prop + change _ (= source))
  (prop + tracked _ (= #source))
  (fn-do + has-paths Bool
    (return (and change (> (. change paths) 0)))
  )
  (fn-do + has-tracked-paths Bool
    (return (and tracked (> (. tracked paths) 0)))
  )
)
```

```output
true
true
```

### A narrowed optional compared with null, alone and as the condition of a value

```brix
(main
  (print (call described "a/b"))
  (print (call described "ab"))
  (print (call tail "a/b"))
  (print (call tail "ab"))
)

(fn-do + described String
  (param path String)
  (let slash (call (. path find) "/"))
  (if (!= slash null)
    (return (str (!= slash null) " " (== slash null) " " (!= null slash)))
  )
  (return (str (!= slash null) " " (== slash null)))
)

(fn-expr + tail String
  (param path String)
  (if-expr (and (!= slash null) (> (. name length) 0)) name "none")
  (where
    (lazy slash (call (. path find) "/"))
    (lazy name (if-expr (!= slash null) ([] path (... (+ slash 1))) path))
  )
)
```

```output
true false true
false true
b
none
```

### A property tested after `value if` tests another property, read again after a call

```brix
(main
  (let holder (call Holder))
  (print (call (. holder moved)))
  (= (. holder drag) (. Kind shape))
  (print (call (. holder moved)))
)

(type + Kind (is Union)
  (variant shape)
  (variant move)
)

(type + Holder
  (prop + count (? Int))
  (prop + drag _ (= (. Kind move)))
  (fn-do + placed Int
    (return 1)
  )
  (fn-do + moved String
    (let counted (only-if (+ count 1) (if count)))
    (if (== drag (. Kind shape))
      (return (str "shape " (call placed) " " (?? counted 0)))
    )
    (return (str "move " (== drag (. Kind move)) " " (?? counted 0)))
  )
)
```

```output
move true 0
shape 1 0
```

### A narrowed optional property that holds a callable object is called as the object it holds.

```brix
(main
  (let holder (call Holder))
  (= (. holder action) (call Action))
  (call (. holder fire))
)

(type + Action
  (fn-do + this _
    (print "fired")
  )
)

(type + Holder
  (prop + action (? Action))
  (fn-do + fire _
    (if action
      (call action)
    )
  )
)
```

```output
fired
```

### An optional local assigned a lambda holds a function until it is assigned again.

```brix
(main
  (let action (? (of Getter Int)))
  (= action (=> () 7))
  (print (call action))
)
```

```output
7
```

### A guard whose body calls a function marked `@no-return` leaves, so what it tested is narrowed after it.

```brix
(main
  (let name (call lookup "a"))
  (if (not name)
    (call stop "no name")
  )
  (print (. name length))
)

(fn-do + lookup (? String)
  (param key String)
  (return-if "Ada" (if (== key "a")))
  (return null)
)

(fn-do + stop _ @no-return
  (param reason String)
  (throw (call Error reason))
)
```

```output
3
```

### A getter always returns, so it cannot be marked `@no-return`.

```brix compile-error
(main
  (print (. (call Box) size))
)

(type + Box
  (fn-expr + size Int :get @no-return
    1
  )
)
```

```output
Exception: @no-return is not supported on getters
<no file:7>
      (fn-expr + size Int :get @no-return
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```
