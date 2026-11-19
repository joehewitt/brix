# Unions

```brix
(main
  (let probe (call Probe))
  (let item Item (call (. Item first) 10))
  (print (call (. item render) (call (. probe next))))
  (= item (call (. Item second) 20))
  (print (call (. item render) (call (. probe next))))
  (= item (call (. Item right) "R"))
  (print (call (. item render) (call (. probe next))))
  (print (. probe next-calls))
)

(type + Probe
  (prop + next-calls I32 (= 0))
  (fn-do + next I32
    (+= next-calls 1)
    (return next-calls)
  )
)

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    (param amount I32)
    (str "L" (+ value amount))
  )
)

(type + Right (is Tuple)
  (prop + value String)
  (fn-expr + render String
    (param amount I32)
    (str value amount)
  )
)

(type + Item (is Union)
  (variant first Left)
  (variant second Left)
  (variant right Right)
)
```

```output
L11
L22
R3
3
```

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Node) arena 1))
  (call (. rows append) (call (. Node wide) 1 10 20 30 40))
  (blank)
  (match ([] rows 0)
    (case (call .small small)
      (call show-union-case-call (. small value))
    )
    (case (call .wide wide)
      (let escaped (call escape-union-case wide))
      (= ([] rows 0) (call (. Node wide) 2 100 200 300 400))
      (call show-union-case-call (. escaped external-a))
    )
  )
  (blank)
  (match ([] rows 0)
    (case (call .small small)
      (call show-union-case-call (. small value))
    )
    (case (call .wide wide)
      (call show-union-case-call (. wide external-a))
    )
  )
)

(fn-do + show-union-case-call _
  (param value U64)
  (print value)
)

(fn-expr + escape-union-case Wide
  (param wide Wide)
  wide
)

(type + Syntax (is Tuple)
  (prop + source U64)
)

(type + Small (is Syntax)
  (prop + value U64)
)

(type + Wide (is Syntax)
  (prop + inline-a U64 @hot)
  (prop + inline-b U64 @hot)
  (prop + external-a U64)
  (prop + external-b U64)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
```

```output
30
300
```

```brix
(main
  (let item Item (call (. Item left) 10))
  (print (call (. item render)))
)

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    "case"
  )
)

(type + Item (is Union)
  (variant left Left)
  (fn-expr + render String
    "union"
  )
)
```

```output
union
```

```brix
(main
  (let item Item (call (. Item left) 10))
  (print (. item label))
  (= item (call (. Item right) "R"))
  (print (. item label))
)

(type + Left (is Tuple)
  (prop + value I32)
)

(type + Right (is Tuple)
  (prop + value String)
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)

(extend + Left
  (fn-expr + label String :get
    (str "left:" value)
  )
)

(extend + Right
  (fn-expr + label String :get
    (str "right:" value)
  )
)
```

```output
left:10
right:R
```

```brix
(main
  (let item Item (call (. Item left) 10))
  (print (. item label))
)

(type + Left (is Tuple)
  (prop + value I32)
)

(type + Item (is Union)
  (variant left Left)
)

(extend + Left
  (fn-expr + label String :get
    "case"
  )
)

(extend + Item
  (fn-expr + label String :get
    "union"
  )
)
```

```output
union
```

```brix
(main
  (let item Item (call (. Item left) 10))
  (= (. item label) "one")
  (= item (call (. Item right) "R"))
  (= (. item label) "two")
)

(type + Left (is Tuple)
  (prop + value I32)
  (fn-do + label _ :set (param value String)
    (print (str "left:" value))
  )
)

(type + Right (is Tuple)
  (prop + value String)
  (fn-do + label _ :set (param value String)
    (print (str "right:" value))
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
left:one
right:two
```

```brix crash
(main
  (let item Item (call (. Item left) 10))
  (print (. item label))
)

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + label String :get
    "left"
  )
)

(type + Right (is Tuple)
  (prop + value I32)
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
Exception: Union case 'right' has no complete dispatch for getter 'label'
<no file:4>
      (print (. item label))
             ^^^^^^^^^^^^^^
```

```brix crash
(main
  (let item Item (call (. Item value) 10))
  (print (. item label))
)

(type + Value (is Tuple)
  (prop + value I32)
  (fn-expr + label String :get
    "value"
  )
)

(type + Item (is Union)
  (variant value Value)
  (variant empty)
)
```

```output
Exception: Union case 'empty' has no payload for getter 'label'
<no file:4>
      (print (. item label))
             ^^^^^^^^^^^^^^
```

```brix crash
(main
  (let item Item (call (. Item left) 10))
  (print (call (. item render)))
)

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    "left"
  )
)

(type + Right (is Tuple)
  (prop + value I32)
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
Exception: Union case 'right' has no complete dispatch for method 'render'
<no file:4>
      (print (call (. item render)))
             ^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let item Item (call (. Item value) 10))
  (print (call (. item render)))
)

(type + Value (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    "value"
  )
)

(type + Item (is Union)
  (variant value Value)
  (variant empty)
)
```

```output
Exception: Union case 'empty' has no payload for method 'render'
<no file:4>
      (print (call (. item render)))
             ^^^^^^^^^^^^^^^^^^^^^^
```

### Inspect union contents without exceptions

```brix
(main
  (print (. (call Foo 42) case))
  (print (. (call Foo 42) type))
  (print (as (call Foo 42) Int))
  (print "")
  (print (. (call Foo 10.5) case))
  (print (. (call Foo 10.5) type))
  (print (as (call Foo 10.5) Float))
)

(type + Foo (is Union)
  (variant Int)
  (variant Float)
)
```

```output
0
Class:core/number:I64
42

1
Class:core/number:F64
10.5
```

### @hot metadata does not change ordinary tuple construction, union matching, conversion, or values.

```brix
(main
  (let first (call (. Value left) 10 20))
  (let second Value (call (. Value right) 30 40))
  (print first)
  (print second)
  (print (as first Left))
  (print
    (match second
      (case (call .left left)
        (+ (. left hot) (. left cold))
      )
      (case (call .right right)
        (+ (. right hot) (. right cold))
      )
    )
  )
)

(type + Left (is Tuple)
  (prop + hot I32 @hot)
  (prop + cold I32)
)

(type + Right (is Tuple)
  (prop + hot I32 @hot)
  (prop + cold I32)
)

(type + Value (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
(10, 20)
(30, 40)
(10, 20)
70
```

```brix
(main
  (print (. Foo error))
  (print (. Foo status))
)

(type + Foo (is Union)
  (variant error)
  (variant status)
)
```

```output
error
status
```

```brix
(main
  (print (== (. Foo status) (. Foo status)))
  (print (== (. Foo status) (. Foo error)))
)

(type + Foo (is Union)
  (variant error)
  (variant status)
)
```

```output
true
false
```

### Explicitly declared Union type name in assignment

```brix
(main
  (let foo Foo (. Foo error))
  (print foo)
)

(type + Foo (is Union)
  (variant error (= 1))
  (variant status (= "good"))
)
```

```output
1
```

```brix
(main
  (do-new
    (let foo Foo 42)
    (print foo)
    (blank)
    (= foo 10.5)
    (print foo)
    (blank)
    (= foo "hello")
    (print foo)
    (on-failure failure
      (print failure)
    )
  )
)

(type + Foo (is Union)
  (variant Int)
  (variant Float)
  (variant String)
)
```

```output
42
10.5
hello
```

### A list written where a list of a union is declared holds each item in the case of its type.

```brix
(main
  (let values (list Value) (list 3 "three" 4.5))
  (for (in value values)
    (print (str value " " (. value case)))
  )
  (let shapes (? (list Shape)) (list (call Circle 1) (call Square 2)))
  (for (in shape (!! shapes))
    (print (is shape Square))
  )
)

(type + Value (is Union)
  (variant Int)
  (variant String)
  (variant Float)
)

(type + Shape (is Union)
  (variant Circle)
  (variant Square)
)

(type + Circle (is Tuple)
  (prop + radius Int)
)

(type + Square (is Tuple)
  (prop + side Int)
)
```

```output
3 0
three 1
4.5 2
false
true
```

```brix
(main
  (let foo Foo "hello")
  (print (as foo String))
  (blank)
  (print (catch (as foo Int)))
  (print (catch? (as foo Int)))
  (print (type-of (catch (as foo Int))))
)

(type + Foo (is Union)
  (variant Int)
  (variant String)
)
```

```output
hello
Invalid union cast
unknown
[TypeInfo Union]
```

```brix
(main
  (let foo Foo 42)
  (print (as foo Int))
  (blank)
  (= foo "hi")
  (print (as foo String))
)

(type + Foo (is Union)
  (variant Int)
  (variant String)
)
```

```output
42
hi
```

```brix
(main
  (let foo Foo 42)
  (print (. foo type))
  (blank)
  (= foo "hello")
  (print (. foo type))
)

(type + Foo (is Union)
  (variant Int)
  (variant String)
)
```

```output
Class:core/number:I64
Class:core/string:String
```

```brix
(main
  (let foo Foo 42)
  (print (is foo Int))
  (blank)
  (= foo "hello")
  (print (is! foo Int))
  (print (is foo String))
  (blank)
  (= foo (list (call Foo 42)))
  (print (is foo (of List Foo)))
)

(type + Foo (is Union)
  (variant Int)
  (variant String)
  (variant (list Foo))
)
```

```output
true
false
true
true
```

```brix
(main
  (let foo Foo (list (call Foo 42)))
  (print (is foo List))
)

(type + Foo (is Union)
  (variant Int)
  (variant String)
  (variant (list Foo))
)
```

```output
true
```

```brix
(main
  (let a Foo 42)
  (let b Foo 10.5)
  (let c Foo "hello")
  (let d Foo (list (call Foo 99) (call Foo 99.99f)))
  (let foos Foo (list a b c d))
  (print (call (. "\n" join) (as foos (list Foo))))
)

(type + Foo (is Union)
  (variant Int)
  (variant Float)
  (variant String)
  (variant (list Foo))
)
```

```output
42
10.5
hello
99, 99.99
```

```brix
(main
  (let a Foo 1)
  (print
    (match a
      (case Int
        "num"
      )
      (case String
        "str"
      )
      (default
        (inline "")
      )
    )
  )
)

(type + Foo (is Union)
  (variant Int)
  (variant String)
)
```

```output
num
```

```brix
(main
  (let a Foo (. Foo status))
  (print
    (match a
      (case .loading
        "loading"
      )
      (case .status
        "status"
      )
      (case .error
        "error"
      )
      (case .done
        "done"
      )
      (default
        "unknown"
      )
    )
  )
)

(type + Foo (is Union)
  (variant loading)
  (variant status)
  (variant error)
  (variant done)
)
```

```output
status
```

```brix
(main
  (let a Foo (. Foo status))
  (print
    (match a
      (case= .loading
        "loading"
      )
      (case= .status
        "status"
      )
      (case= .error
        "error"
      )
      (case= .done
        "done"
      )
      (default
        "unknown"
      )
    )
  )
  (print
    (match (. Foo done)
      (case= (. Foo loading)
        "loading"
      )
      (case= (. Foo status)
        "status"
      )
      (case= (. Foo error)
        "error"
      )
      (case= (. Foo done)
        "done"
      )
      (default
        "unknown"
      )
    )
  )
)

(type + Foo (is Union)
  (variant loading)
  (variant status)
  (variant error)
  (variant done)
)
```

```output
status
done
```

```brix
(main
  (let a Foo (list (call Foo 42)))
  (print
    (match a
      (case Int
        "num"
      )
      (case (exact String)
        "str"
      )
      (case (: x List)
        (str "list " l)
        (lazy l (. x length))
      )
      (case (: m Map)
        (str "map " l)
        (lazy l (. m length))
      )
      (default
        ""
      )
    )
  )
)

(type + Foo (is Union)
  (variant Int)
  (variant Float)
  (variant String)
  (variant (list Foo))
  (variant (map String Foo))
)
```

```output
list 1
```

## Enums in unions

```brix
(main
  (print (. Foo default))
  (print (. Foo error))
  (print (. Foo status))
  (print "")
  (print (is (. Foo error) Int))
  (print (is! (. Foo error) String))
  (print (type-of (. Foo error)))
  (print "")
  (print (is (. Foo status) String))
  (print (is! (. Foo status) Int))
  (print (type-of (. Foo status)))
)

(type + Foo (is Union)
  (variant default)
  (variant error (= -1))
  (variant status (= "good"))
)
```

```output
default
-1
good

true
false
[TypeInfo Foo]

true
false
[TypeInfo Foo]
```

## Union comparison

```brix
(main
  (let foo (. Foo error))
  (blank)
  (print (== foo (. Foo error)))
  (print (== foo (. Foo life)))
  (blank)
  (print (== foo (. Foo status)))
  (blank)
  (print (== foo 1))
  (print (== foo 41))
  (print (!= foo (. Foo error)))
  (print (!= foo (. Foo life)))
  (print (!= foo (. Foo status)))
  (blank)
  (print (!= foo 1))
  (print (!= foo 41))
)

(type + Foo (is Union)
  (variant error (= 1))
  (variant life (= 42))
  (variant status (= "good"))
  (variant Float)
  (variant Int)
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
```

```brix
(main
  (let a (call (. Authorization valid) "a" "b"))
  (print (case (= (. Authorization valid) a)))
  (print (case (= (. Authorization invalid-password) a)))
)

(type + Authorization (is Union)
  (variant invalid-password)
  (variant invalid-email)
  (variant valid (tuple (: email String) (: password String)))
)
```

```output
(a, b)
unknown
```

```brix
(main
  (let y (? Foo) (call Foo 1))
  (print (. (as y Foo) case))
)

(type + Foo (is Union)
  (variant Int)
  (variant String)
)
```

```output
0
```

```brix crash
(main
  (print (call Foo (call List)))
)

(type + Foo (is Union)
  (variant String)
  (variant Int)
)
```

```output
Exception: Could not resolve parameter Item
<no file:3>
      (print (call Foo (call List)))
                       ^^^^^^^^^^^
```

```brix
(main
  (let u (call Foo 8.5))
  (print u)
  (print (. u case))
)

(type + Foo (is Union)
  (variant String)
  (variant Int)
)
```

```output
8
1
```

```brix
(main
  (let foo Foo "hi")
  (print (. foo case))
  (let bar Bar foo)
  (print (. bar case))
  (print bar)
)

(type + Foo (is Union)
  (variant Int)
  (variant String)
)

(type + Bar (is Union)
  (variant String)
  (variant Int)
)
```

```output
1
0
hi
```

### Casting doesn't work for untyped cases

```brix crash
(main
  (print (as 1 Foo))
)

(type + Foo (is Union)
  (variant foo)
  (variant bar)
)
```

```output
Exception: I64 cannot be cast to Foo; there is no conversion between them
<no file:3>
      (print (as 1 Foo))
             ^^^^^^^^^^
```

### Casting works for typed cases

```brix
(main
  (print (as 1 Foo))
)

(type + Foo (is Union)
  (variant foo Int)
  (variant bar String)
)
```

```output
1
```

### Constructors don't work for untyped cases

```brix crash
(main
  (print (call Foo 1))
)

(type + Foo (is Union)
  (variant foo)
  (variant bar)
)
```

```output
Exception: No constructor of Foo accepts (I64)
<no file:3>
      (print (call Foo 1))
                   ^^^
```

### Constructors work for typed cases

```brix
(main
  (print (. (call Foo 1) case))
)

(type + Foo (is Union)
  (variant foo String)
  (variant bar Int)
)
```

```output
1
```

### Named typed case accessor returns payload

```brix
(main
  (let foo Foo (call Foo 42))
  (print (. foo foo))
)

(type + Foo (is Union)
  (variant foo Int)
  (variant bar String)
)
```

```output
42
```

### Named union case match distinguishes duplicate payload types

```brix
(main
  (let v Foo (call (. Foo b) 2))
  (print
    (match v
      (case (call .a x)
        (str "a " x)
      )
      (case (call .b x)
        (str "b " x)
      )
      (default
        "none"
      )
    )
  )
)

(type + Foo (is Union)
  (variant a Int)
  (variant b Int)
)
```

```output
b 2
```

### Non-capturing payload case match can dispatch before checking payload shape

```brix
(main
  (let v Foo (call (. Foo b) 2))
  (print
    (match v
      (case (call .a Int)
        (str "a " it)
      )
      (case (call .b Int)
        (str "b " it)
      )
      (default
        "none"
      )
    )
  )
)

(type + Foo (is Union)
  (variant a Int)
  (variant b Int)
)
```

```output
b 2
```

### Named typed case accessor defaults on wrong case

```brix
(main
  (let foo Foo (call Foo 42))
  (print (. foo bar))
)

(type + Foo (is Union)
  (variant foo Int)
  (variant bar String)
)
```

### Named tuple case accessor returns tuple payload

```brix
(main
  (let auth (call (. Authorization valid) "a" "b"))
  (let valid (. auth valid))
  (print (. valid email))
  (print (. valid password))
)

(type + Authorization (is Union)
  (variant invalid-password)
  (variant invalid-email)
  (variant valid (tuple (: email String) (: password String)))
)
```

```output
a
b
```

### Named untyped case accessor returns default payload

```brix
(main
  (let foo Foo (. Foo foo))
  (print (. foo foo))
)

(type + Foo (is Union)
  (variant foo)
  (variant bar)
)
```

```output
0
```

## Anonymous unions

```brix
(main
  (print
    (scope (tuple v (type-of v) (. v case))
      (lazy v (call foo "hi"))
    )
  )
  (blank)
  (print
    (scope (tuple v (type-of v) (. v case))
      (lazy v (call foo "foo"))
    )
  )
  (blank)
  (print
    (scope (tuple v (type-of v) (. v case))
      (lazy v (call foo "bar"))
    )
  )
)

(fn-expr + foo _
  (param value _)
  (match value
    (case-union= "foo"
      42
    )
    (case-union= "bar"
      (list 1 2 3)
    )
    (default
      "abc"
    )
  )
)
```

```output
(abc, [TypeInfo Union], 2)
(42, [TypeInfo Union], 0)
(1, 2, 3, [TypeInfo Union], 1)
```

```brix
(main
  (print .foo)
  (print (== .foo (. Foo foo)))
)

(type + Foo (is Union)
  (variant foo)
  (variant bar)
)
```

```output
foo
true
```

```brix
(import ./lib/stuff)
(blank)

(main
  (print .bar)
  (print (== .foo (. Foo foo)))
  (print (== .baz (. Boo baz)))
)

(type + Foo (is Union)
  (variant foo)
  (variant bar)
)
```

```brix file=lib/stuff.bx
(type + Boo (is Union)
  (variant foo)
  (variant baz)
)
```

```output
bar
true
true
```

### A bare case that several visible types share needs an expected type or a qualified name.

```brix crash
(main
  (print .foo)
)

(type + Foo (is Union)
  (variant foo)
  (variant bar)
)

(type + Boo (is Union)
  (variant foo)
  (variant baz)
)
```

```output
Exception: Case '.foo' is ambiguous between 'Boo', 'Foo'
<no file:3>
      (print .foo)
             ^^^^
```

### A bare case constructor that several visible unions share must be qualified.

```brix crash
(main
  (print (call .word "x"))
)

(type + Token (is Union)
  (variant word (tuple (: text String)))
  (variant number (tuple (: value Int)))
)

(type + Part (is Union)
  (variant word (tuple (: text String)))
  (variant space)
)
```

```output
Exception: Case '.word' is ambiguous between 'Part', 'Token'
<no file:3>
      (print (call .word "x"))
                   ^^^^^
```

```brix
(main
  (let v (if-expr true .x .y))
  (print v)
  (print (type-of v))
)

(type + Foo (is Union)
  (variant x)
  (variant y)
)
```

```output
x
[TypeInfo Foo]
```

### Explicit constructor return type participates in constructor inference

```brix
(main
  (let event (call Mouse 1))
  (print (type-of event))
  (print (as event Down))
)

(type + Event (is Union)
  (variant Down)
  (variant Unknown)
)

(type + Mouse
  (fn-do + This Event
    (param button Int)
    (if (== button 1)
      (return (call Down))
      (else
        (return (call Unknown))
      )
    )
  )
)

(type + Down
  (fn-to-expr + String
    "down"
  )
)

(type + Unknown
  (fn-to-expr + String
    "unknown"
  )
)
```

```output
[TypeInfo Event]
down
```

```brix
(main
  (let event (call Mouse 1))
  (print (type-of event))
)

(type + Event (is Union)
  (variant Down)
  (variant Unknown)
)

(type + Mouse
  (fn-do + This Event
    (param button Int)
    (return
      (match button
        (case= 1
          (call Down)
        )
        (default
          (call Unknown)
        )
      )
    )
  )
)

(type + Down)

(type + Unknown)
```

```output
[TypeInfo Event]
```

```brix
(import ./lib/model)
(import ./lib/use-one)
(import ./lib/use-two)
(blank)

(main
  (let item Item (call (. Item left) 10))
  (print (call call-one item))
  (print (call call-two item))
)
```

```brix file=lib/model.bx
(type + Left (is Tuple)
  (prop + value I32)
)

(type + Right (is Tuple)
  (prop + value I32)
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```brix file=lib/extension-one.bx
(import ./model)

(extend + Left
  (fn-expr + render String
    "one-left"
  )
)

(extend + Right
  (fn-expr + render String
    "one-right"
  )
)
```

```brix file=lib/extension-two.bx
(import ./model)

(extend + Left
  (fn-expr + render String
    "two-left"
  )
)

(extend + Right
  (fn-expr + render String
    "two-right"
  )
)
```

```brix file=lib/use-one.bx
(import ./model)
(import ./extension-one)

(fn-expr + call-one String
  (param item Item)
  (call (. item render))
)
```

```brix file=lib/use-two.bx
(import ./model)
(import ./extension-two)

(fn-expr + call-two String
  (param item Item)
  (call (. item render))
)
```

```output
one-left
two-left
```

```brix
(main
  (let item Item (call (. Item left) 10))
  (print (call (. item render)))
)

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    "case"
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant empty)
)

(extend + Item
  (fn-expr + render String
    "union-extension"
  )
)
```

```output
union-extension
```

```brix
(main
  (let leaf Node (call (. Node leaf) "x"))
  (let branch Node (call (. Node branch) (list leaf)))
  (print (call (. branch render)))
)

(type + Leaf (is Tuple)
  (prop + text String)
  (fn-expr + render String
    text
  )
)

(type + Branch (is Tuple)
  (prop + children (list Node))
  (fn-expr + render String
    (call (. ([] children 0) render))
  )
)

(type + Node (is Union)
  (variant leaf Leaf)
  (variant branch Branch)
)
```

```output
x
```

```brix
(main
  (let item Item (call (. Item left) 10))
  (let described (call (. item describe)))
  (print (type-of described))
  (print (. (as described Alpha) value))
  (= item (call (. Item right) "R"))
  (= described (call (. item describe)))
  (print (. (as described Beta) value))
)

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + describe Alpha
    (call Alpha value)
  )
)

(type + Right (is Tuple)
  (prop + value String)
  (fn-expr + describe Beta
    (call Beta value)
  )
)

(type + Alpha
  (prop + value I32)
  (fn-do + This _
    (param value _)
  )
)

(type + Beta
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
[TypeInfo Union]
10
R
```

```brix
(main
  (let item Item (call (. Item left) 1))
  (print (?? (catch? (call (. item render))) "caught-left"))
  (= item (call (. Item right) 2))
  (print (?? (catch? (call (. item render))) "caught-right"))
)

(type + LeftError (is Error))

(type + RightError (is Error))

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + render String (?! LeftError)
    (throw (call LeftError "left"))
  )
)

(type + Right (is Tuple)
  (prop + value I32)
  (fn-expr + render String (?! RightError)
    (throw (call RightError "right"))
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
caught-left
caught-right
```

```brix
(main
  (let xs (list 1 2))
  (let generated (in each xs))
  (let payload (call Payload generated))
  (blank)
  (print (. payload items length))
  (print ([] (. payload items) 0))
)

(type + Payload (is Union)
  (variant items (list Int))
  (variant empty)
)
```

```output
2
1
```

### Union wrap case order breaks same-kind side-cast ties

```brix
(main
  (let offset PositionOffset 4)
  (let offset-with-unit PositionOffset 4tc)
  (blank)
  (print (call offset-status offset))
  (print (call offset-status offset-with-unit))
)

(type + TestCoord (alias tc) (is I16))

(type + TestPercent (alias %) (is F64))

(type + PositionOffset (is Union)
  (variant coord TestCoord)
  (variant percent TestPercent)
  (variant anchor)
)

(fn-expr + offset-status String
  (param offset PositionOffset)
  (match offset
    (case (call (. PositionOffset coord) n)
      (str "coord " n)
    )
    (case (call (. PositionOffset percent) percent)
      (str "percent " (as percent TestPercent))
    )
    (default
      "other"
    )
  )
)
```

```output
coord 4
coord 4
```

### Arena handles and allocation pointers use ordinary union and optional copies.

```brix
(import /core/arena)
(blank)

(main
  (let first (call Arena))
  (let second (call Arena))
  (let first-pointer (call (of (. first allocate) U32) 1))
  (= ([] first-pointer 0) 1u32)
  (let second-pointer (call (of (. second allocate) U32) 1))
  (= ([] second-pointer 0) 2u32)
  (let choice ArenaChoice (call (. ArenaChoice pointer) first-pointer))
  (print ([] (. choice pointer) 0))
  (= choice (call (. ArenaChoice pointer) second-pointer))
  (print ([] (. choice pointer) 0))
  (blank)
  (let owner-choice ArenaChoice first)
  (let owner-pointer (call (of (. owner-choice arena allocate) U32) 1))
  (= ([] owner-pointer 0) 3u32)
  (print ([] owner-pointer 0))
  (let optional (? Arena) second)
  (let optional-pointer (call (of (. (!! optional) allocate) U32) 1))
  (= ([] optional-pointer 0) 4u32)
  (print ([] optional-pointer 0))
  (blank)
  (call (. first release))
  (call (. second release))
)

(type + ArenaChoice (is Union)
  (variant arena Arena)
  (variant pointer (of CPointer U32))
)
```

```output
1
2
3
4
```

### A map joins the union through its map case, not as a stream of truth-tested pairs.

```brix
(main
  (let entry
    (call (of Map String Value)
      (block
        (entry "count" 1)
      )
    )
  )
  (print (call pick entry))
  (print (call pick (list entry entry)))
)

(type + Value (is Union)
  (variant Bool)
  (variant Int)
  (variant (map String Value))
)

(fn-expr + pick String
  (param value Value)
  "whole"
)

(fn-expr + pick String
  (param values (stream Value))
  "items"
)
```

```output
whole
items
```

### A value never joins a union through the truth test of its Bool case.

```brix crash
(main
  (print (call describe true))
  (print (call describe (call Box 3)))
)

(type + Box (is Tuple)
  (prop + count Int)
)

(type + Value (is Union)
  (variant Bool)
  (variant Int)
)

(fn-expr + describe String
  (param value Value)
  "value"
)
```

```output
Exception: No function "describe" accepts (Box); it is declared as "describe(value Value)"
<no file:4>
      (print (call describe (call Box 3)))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A payloadless union case can call getters declared on its union.

```brix
(main
  (let mode (. Mode pen))
  (print (. mode title))
  (print (. mode draws))
  (print (. Mode zoom title))
  (print (. Mode zoom draws))
)

(type + Mode (is Union)
  (variant pen)
  (variant zoom)
  (fn-expr + title _ :get
    (match this
      (case (. Mode pen)
        "Pen"
      )
      (default
        "Zoom"
      )
    )
  )
  (fn-expr + draws _ :get
    (== this (. Mode pen))
  )
)
```

```output
Pen
true
Zoom
false
```

### A union getter cannot shadow a reserved union property.

```brix crash
(main
  (print (. Mode pen name))
)

(type + Mode (is Union)
  (variant pen)
  (fn-expr + name _ :get
    "Pen"
  )
)
```

```output
Exception: Union property 'name' conflicts with reserved union property
<no file:8>
      (fn-expr + name _ :get
      ^^^^^^^^^^^^^^^^^^^^^
```

### A case whose single field is a tuple accepts a tuple value as well as the tuple's fields.

```brix
(type-line + CaseSpot (is (tuple (: x I32) (: y I32))))
(blank)

(main
  (let spot (call CaseSpot 1 2))
  (print (call describe (call (. CasePiece single) spot)))
  (print (call describe (call (. CasePiece single) 3 4)))
  (print (call describe (call wrap (call CaseSpot 5 6))))
  (print (call describe (call (. CasePiece pair) 9 spot)))
)

(fn-expr + wrap CasePiece
  (param spot CaseSpot)
  (call .single spot)
)

(fn-expr + describe String
  (param piece CasePiece)
  (match piece
    (case (call .single place)
      (str "single " (. place x) " " (. place y))
    )
    (case (call .pair x place)
      (str "pair " x " " (. place y))
    )
  )
)

(type + CasePiece (is Union)
  (variant single (tuple (: place CaseSpot)))
  (variant pair (tuple (: x I32) (: place CaseSpot)))
)
```

```output
single 1 2
single 3 4
single 5 6
pair 9 2
```

### A case shorthand shared by several unions resolves against the type it is compared with, assigned to, passed as, or listed beside, instead of whichever union was declared last.

```brix
(main
  (let cap (. TipShape round))
  (let join (. BendShape miter))
  (print (== cap .round))
  (print (== join .round))
  (print (== .round cap))
  (print (call describe-tip .square))
  (print (call describe-tip .round))
  (print (call default-tip))
  (let tips (? TipShape) .round)
  (print (== tips .round))
  (for (in tip (list .butt .round .square))
    (print (call describe-tip tip))
  )
)

(fn-expr + describe-tip String
  (param tip TipShape)
  (match tip
    (case (. TipShape butt)
      "butt"
    )
    (case (. TipShape round)
      "round"
    )
    (case (. TipShape square)
      "square"
    )
  )
)

(fn-expr + default-tip String
  (param tip TipShape (= .round))
  (call describe-tip tip)
)

(type + TipShape (is Union)
  (variant butt)
  (variant round)
  (variant square)
)

(type + BendShape (is Union)
  (variant miter)
  (variant round)
  (variant bevel)
)
```

```output
true
false
true
square
round
round
true
butt
round
square
```

### Coalescing an optional alternative with a union that contains it widens to the union instead of unwrapping the union, which would fail at runtime for the union's other cases.

```brix
(main
  (print (call describe (call pick null)))
  (print (call describe (call pick (call Tint 4))))
)

(type + Tint (is Tuple)
  (prop + amount I32)
)

(type + Shade (is Union)
  (variant none)
  (variant Tint)
)

(fn-expr + pick Shade
  (param level (? Tint))
  (if-expr level level (. Shade none))
)

(fn-expr + describe String
  (param shade Shade)
  (match shade
    (case= (. Shade none)
      "none"
    )
    (case (: found Tint)
      (str "level " (. found amount))
    )
  )
)
```

```output
none
level 4
```

### A function without a declared return type that returns shared case shorthands infers the union its other returns agree on.

```brix
(main
  (print (call describe-cap (call pick-cap 0)))
  (print (call describe-cap (call pick-cap 1)))
  (print (call describe-cap (call pick-cap 2)))
)

(fn-do + pick-cap _
  (param kind I32)
  (if (== kind 0)
    (return .flat)
    (else-if (== kind 1)
      (return .pointed)
    )
  )
  (blank)
  (return .rounded)
)

(fn-expr + describe-cap String
  (param cap CapShape)
  (match cap
    (case (. CapShape flat)
      "flat"
    )
    (case (. CapShape pointed)
      "pointed"
    )
    (case (. CapShape rounded)
      "rounded"
    )
  )
)

(type + CapShape (is Union)
  (variant flat)
  (variant pointed)
  (variant rounded)
)

(type + CornerShape (is Union)
  (variant sharp)
  (variant rounded)
)
```

```output
flat
pointed
rounded
```

### A match capture narrows to a union alternative declared as a shorthand tuple.

```brix
(main
  (print (call describe (call pick null)))
  (print (call describe (call pick (call Tint 4))))
)

(type + Tint (is (tuple (: amount I32))))

(type + Shade (is Union)
  (variant none)
  (variant Tint)
)

(fn-expr + pick Shade
  (param level (? Tint))
  (if-expr level level (. Shade none))
)

(fn-expr + describe String
  (param shade Shade)
  (match shade
    (case= (. Shade none)
      "none"
    )
    (case (: found Tint)
      (str "level " (. found amount))
    )
  )
)
```

```output
none
level 4
```

### A generic class can hold a union parameterized by its own type parameter.

```brix
(main
  (let h (call (of Holder Int)))
  (call (. h set) 5)
  (print
    (match (!! (. h m))
      (case (call .some v)
        (str "some " v)
      )
      (default
        "none"
      )
    )
  )
)

(type + (of Maybe T) (is Union)
  (variant some (tuple T))
  (variant none)
)

(type + (of Holder Value)
  (prop + m (? (of Maybe Value)))
  (fn-do + set _
    (param v Value)
    (= m (call (. (of Maybe Value) some) v))
  )
)
```

```output
some 5
```

### A case shorthand shared by two unions resolves to the one that converts to the expected type.

```brix
(main
  (let holder (call Holder))
  (print (. holder chord label))
  (= (. holder chord) .second)
  (print (. holder chord label))
)

(type + Step
  (variant first)
  (variant second)
)

(type + Motion
  (variant second)
  (variant third)
)

(type + Chord (is Tuple)
  (prop + label String)
  (fn-from-expr + (: step Step)
    (call This (if-expr (== step (. Step first)) "step first" "step second"))
  )
)

(type + Holder
  (prop + chord Chord (= (. Step first)))
)
```

```output
step first
step second
```

### A case shorthand shared by two unions resolves by the parameter type in a constructor call.

```brix
(main
  (let span (call Span .invalid "x"))
  (print (== (. span kind) (. SpanKind invalid)))
  (print (. span text))
)

(type + SpanKind (is Union)
  (variant plain)
  (variant invalid)
)

(type + TokenKind (is Union)
  (variant word)
  (variant invalid)
)

(type + Span (is Tuple)
  (prop + kind SpanKind)
  (prop + text String)
)
```

```output
true
x
```

### A union case built inside arguments whose types are inferred, such as a union's constructor, resolves as a case constructor instead of a method on the union's values.

```brix
(main
  (let wrapped (call Wrapped (call label (call (. Choice previous) 9))))
  (match wrapped
    (case (call .text text)
      (print text)
    )
    (default
      (print "number")
    )
  )
  (print (call label (. Choice every)))
  (print (call render (call Entry (call render (call choice-fields (call (. Choice previous) 9))))))
  (let checked
    (call checked-text (call render (call choice-fields (call (. Choice previous) 9)))
      (block
        (on-error (: err Error)
          (throw (call Panic (as err String)))
        )
      )
    )
  )
  (print checked)
)

(type + Wrapped (is Union)
  (variant text String)
  (variant number F64)
)

(type + Choice (is Union)
  (variant previous U64)
  (variant every)
)

(type + Entry (is Union)
  (variant String)
  (variant (map String Entry))
  (fn-do + This _
    (param text String)
    (return
      (call (of Map String Entry)
        (block
          (entry "text" text)
        )
      )
    )
  )
)

(fn-expr + label String
  (param choice Choice)
  (match choice
    (case (call .previous source)
      (str "previous " source)
    )
    (default
      "every"
    )
  )
)

(fn-expr + render String
  (param value _)
  (match value
    (case String
      (str "'" value "'")
    )
    (case (: map Map)
      (str "{" joined "}")
      (lazy joined (call (. ", " join) pairs))
      (lazy pairs
        (case-each (in (tuple key val) map)
          (str key ": " (call render val))
        )
      )
    )
    (default
      "?"
    )
  )
)

(fn-do + checked-text String (?! Error)
  (param text String)
  (throw (call Error "Empty text") (if (not (. text length))))
  (return text)
)

(fn-do + choice-fields (map String Entry)
  (param choice Choice)
  (return
    (call (of Map String Entry)
      (block
        (entry "choice" (call label choice))
      )
    )
  )
)
```

```output
previous 9
every
{text: '{choice: 'previous 9'}'}
{choice: 'previous 9'}
```

### A list built from one value of a union with a list case holds that value

```brix
(main
  (let trees (call (of List Tree) (call (. Tree leaf) "a")))
  (print (. trees length))
  (print (. ([] trees 0) leaf))
  (let pair (call (of List Tree) (call (. Tree leaf) "a") (call (. Tree branch) (call (of List Tree)))))
  (print (. pair length))
  (let shapes (call (of List Shape) (call Shape 3)))
  (print (. shapes length))
  (let copied (call (of List Tree) trees))
  (print (. copied length))
)

(type + Tree (is Union)
  (variant leaf String)
  (variant branch (list Tree))
)

(type + Shape (is Union)
  (variant Int)
  (variant (list Shape))
)
```

```output
1
a
2
1
1
```

### Overloads that accept several owners of a bare case take the last visible owner

```brix
(import ./lib/one)
(import ./lib/two)
(blank)

(main
  (print (call choose .enter))
  (print (call choose .start))
  (print (call rank .equal))
  (print (call fallback .enter))
)

(type + Local (is Union)
  (variant enter)
)

(type + Later (is Union)
  (variant enter)
)

(type + Record (is Tuple)
  (prop + size Int (= 0))
)

(fn-expr + choose String
  (param two Two)
  "Two"
)

(fn-expr + choose String
  (param later Later)
  "Later"
)

(fn-expr + choose String
  (param one One)
  "One"
)

(fn-expr + choose String
  (param local Local)
  "Local"
)

(fn-expr + rank String
  (param comparison Comparison)
  "Comparison"
)

(fn-expr + rank String
  (param one One)
  "One"
)

(fn-expr + fallback String
  (param record Record)
  "Record"
)

(fn-expr + fallback String
  (param one One)
  "One"
)
```

```brix file=lib/one.bx
(type + One (is Union)
  (variant enter)
  (variant equal)
  (variant start)
)
```

```brix file=lib/two.bx
(type + Two (is Union)
  (variant enter)
  (variant start)
)
```

```output
Later
One
One
One
```

### A case named alone as a default is a case of its parameter's type, though another type has the name

```brix
(main
  (print (call tip-name))
  (print (call tip-name .square))
  (print (call bend-name))
  (print (call maybe-tip))
)

(fn-expr + tip-name String
  (param tip TipShape (= .round))
  (str "tip " tip)
)

(fn-expr + bend-name String
  (param bend BendShape (= .round))
  (str "bend " bend)
)

(fn-expr + maybe-tip String
  (param tip (? TipShape) (= .round))
  (str "maybe " (?? tip (. TipShape butt)))
)

(type + TipShape (is Union)
  (variant butt)
  (variant round)
  (variant square)
)

(type + BendShape (is Union)
  (variant miter)
  (variant round)
  (variant bevel)
)
```

```output
tip round
tip square
bend round
maybe round
```

### A case named alone in a list is a case of the type the other items give the list

```brix
(main
  (let tips (list .butt .round .square))
  (print (tuple (call tip-name ([] tips 0)) (call tip-name ([] tips 1)) (call tip-name ([] tips 2))))
  (let bends (list .round .miter))
  (print (tuple (call bend-name ([] bends 0)) (call bend-name ([] bends 1))))
)

(fn-expr + tip-name String
  (param tip TipShape)
  (str "tip " tip)
)

(fn-expr + bend-name String
  (param bend BendShape)
  (str "bend " bend)
)

(type + TipShape (is Union)
  (variant butt)
  (variant round)
  (variant square)
)

(type + BendShape (is Union)
  (variant miter)
  (variant round)
  (variant bevel)
)
```

```output
(tip butt, tip round, tip square)
(bend round, bend miter)
```

### A case named alone and assigned to a property is a case of the property's type

```brix
(main
  (let holder (call Holder))
  (= (. holder tip) .round)
  (print (. holder tip))
  (= (. holder bend) .round)
  (print (. holder bend))
)

(type + Holder
  (prop + tip TipShape (= (. TipShape butt)))
  (prop + bend BendShape (= (. BendShape miter)))
)

(type + TipShape (is Union)
  (variant butt)
  (variant round)
  (variant square)
)

(type + BendShape (is Union)
  (variant miter)
  (variant round)
  (variant bevel)
)
```

```output
round
round
```

### Two unions of one type are equal where they hold the same case and what it holds is equal

```brix
(main
  (let a Shape (call .count 3))
  (let b Shape (call .count 3))
  (let c Shape (call .count 4))
  (let d Shape (call .named "x"))
  (let e Shape (call .named "x"))
  (let f Shape .empty)
  (let g Shape (call .pair 1 "p"))
  (let h Shape (call .pair 1 "q"))
  (print (== a b))
  (print (== a c))
  (print (!= a c))
  (print (== a d))
  (print (== d e))
  (print (!= d e))
  (print (== f .empty))
  (print (== f a))
  (print (== g g))
  (print (== g h))
  (print (!= g h))
  (print (== .empty f))
)

(type + Shape (is Union)
  (variant count Int)
  (variant named String)
  (variant pair (tuple (: a Int) (: b String)))
  (variant empty)
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
false
true
false
true
true
```

### Unions compared inside a tuple and inside a union, and a case constructed as an equality pattern

```brix
(main
  (let a (call Wrap (call .count 3) 1))
  (let b (call Wrap (call .count 3) 1))
  (let c (call Wrap (call .named "n") 1))
  (print (== a b))
  (print (== a c))
  (print (!= a c))
  (let n Nest (call .inner (call .count 2)))
  (let m Nest (call .inner (call .count 2)))
  (let o Nest (call .inner (call .named "z")))
  (print (== n m))
  (print (== n o))
  (print (call describe (call .count 1)))
  (print (call describe (call .named "x")))
  (print (call describe .empty))
)

(fn-expr + describe String
  (param shape Shape)
  (match shape
    (case= .empty
      "empty"
    )
    (case= (call .count 1)
      "one"
    )
    (default
      "other"
    )
  )
)

(type + Wrap (is (tuple (: shape Shape) (: weight Int))))

(type + Nest (is Union)
  (variant inner Shape)
  (variant other)
)

(type + Shape (is Union)
  (variant count Int)
  (variant named String)
  (variant empty)
)
```

```output
true
false
true
true
false
one
other
empty
```

### A case constructed by its symbol alone as an operand, for an optional, and inside a case

```brix
(main
  (let a Shape (call .count 3))
  (print (== a (call .count 3)))
  (print (== a (call .count 4)))
  (print (== (call .count 3) a))
  (print (!= a (call .named "x")))
  (let o (? Shape) (call .count 3))
  (print o)
  (let n Nest (call .inner (call .count 2)))
  (print n)
  (let m Nest (call (. Nest inner) .empty))
  (print m)
  (print (== n m))
  (print (== n (call .inner (call .count 2))))
)

(type + Nest (is Union)
  (variant inner Shape)
  (variant other)
)

(type + Shape (is Union)
  (variant count Int)
  (variant named String)
  (variant empty)
)
```

```output
true
false
true
true
3
2
empty
false
true
```

### Cases declared with values

```brix
(main
  (print (. Code missing))
  (print (. Code denied))
  (let code Code (. Code denied))
  (print (== code (. Code denied)))
  (print (== code (. Code missing)))
  (print (call describe (. Code missing)))
  (print (call describe (. Code denied)))
  (print (call describe .other))
)

(fn-expr + describe String
  (param code Code)
  (match code
    (case= (. Code missing)
      "missing"
    )
    (case .denied
      "denied"
    )
    (case .other
      "other"
    )
    (default
      "none"
    )
  )
)

(type + Code (is Union)
  (variant missing (= 404))
  (variant denied (= "no"))
  (variant other)
)
```

```output
404
no
true
false
missing
denied
other
```

### A cast out of a union under catch gives an error where the union holds another case.

```brix
(main
  (let foo Foo "hello")
  (let num Foo 42)
  (let pair Foo (tuple 3 "three"))
  (print (catch (as foo Int)))
  (print (catch (as num Int)))
  (print (catch? (as num Int)))
  (print (catch? (as foo Int)))
  (print (?? (catch? (as pair (tuple Int String))) (tuple 0 "none")))
  (print (?? (catch? (as foo (tuple Int String))) (tuple 0 "none")))
  (print
    (match (catch (as foo String))
      (case String
        (str "text " it)
      )
      (case UnionError
        "not text"
      )
    )
  )
  (print
    (match (catch (as num String))
      (case String
        (str "text " it)
      )
      (case UnionError
        "not text"
      )
    )
  )
  (print (call describe foo))
  (print (call describe num))
)

(type + Foo (is Union)
  (variant Int)
  (variant String)
  (variant (tuple Int String))
)

(fn-do + describe String
  (param value Foo)
  (let number (catch (as value Int)))
  (return
    (match number
      (case Int
        (str "number " it)
      )
      (case UnionError
        (str "error " (. it message))
      )
    )
  )
)
```

```output
Invalid union cast
42
42
unknown
(3, three)
(0, none)
text hello
text 42
error 
number 42
```

### A pattern of a type matches a case that is an optional of it where the case holds a value

```brix
(main
  (print (call describe (call found 0)))
  (print (call describe (call found 1)))
  (print (call describe (call found 2)))
  (print (call count 0))
  (print (call count 1))
  (print (call count 2))
  (print (call label 0))
  (print (call label 1))
  (print (call label 2))
)

(type + Spot (is Tuple)
  (prop + number Int)
  (prop + count Int)
)

(type + Found (is Union)
  (variant spot (? Spot))
  (variant code Int)
)

(fn-do + found Found
  (param kind Int)
  (if (== kind 0)
    (let none (? Spot) null)
    (return (call (. Found spot) none))
  )
  (if (== kind 1)
    (return (call (. Found spot) (call Spot 3 4)))
  )
  (return (call (. Found code) 9))
)

(fn-do + describe String
  (param found Found)
  (match? found
    (case (: spot Spot)
      (return (str "spot " (. spot number)))
    )
    (case (: code Int)
      (return (str "code " code))
    )
  )
  (return "nothing")
)

(fn-do + look (? Spot) (?! IOError)
  (param kind Int)
  (if (== kind 0)
    (throw (call IOError "io"))
  )
  (if (== kind 1)
    (return (call Spot 5 6))
  )
  (return null)
)

(fn-do + count Int
  (param kind Int)
  (let held (catch (call look kind)))
  (match? held
    (case (: spot Spot)
      (return (. spot count))
    )
  )
  (return -1)
)

(fn-do + label String
  (param kind Int)
  (let held (catch (call look kind)))
  (return
    (match held
      (case (: spot Spot)
        (str "spot " (. spot number))
      )
      (case IOError
        "io"
      )
      (default
        (inline "none")
      )
    )
  )
)
```

```output
nothing
spot 3
code 9
-1
6
-1
io
spot 5
none
```

### A union answers `case`, `name`, `type` and the name of a case by itself, and the first value of a case it does not hold.

```brix
(main
  (let a (call pick true))
  (print (. a type))
  (print (. a case))
  (print (. a name))
  (let b (call pick false))
  (print (. b type))
  (print (. b case))
  (print (. b name))
  (let kind Kind (. Kind second))
  (print (. kind name))
  (print (. kind type))
  (print (. kind case))
  (let mixed Mixed (. Mixed none))
  (print (. mixed name))
  (print (. mixed type))
  (print (. mixed some))
  (print (. mixed none))
  (print (. mixed word))
  (print (. mixed pair))
  (= mixed (call (. Mixed some) 4))
  (print (. mixed name))
  (print (. mixed type))
  (print (. mixed some))
  (print (. mixed none))
  (= mixed (call (. Mixed pair) 1 2.5))
  (print (. mixed pair))
  (print (. mixed type))
  (print (call held mixed))
  (print (call held (call (. Mixed word) "w")))
)

(fn-do + held String
  (param mixed Mixed)
  (return (str (. mixed name) ":" (. mixed case) ":" (. mixed word) ":" (. mixed some)))
)

(fn-do + pick Either
  (param really Bool)
  (if really
    (return 1)
  )
  (return "one")
)

(type + Either (is Union)
  (variant Int)
  (variant String)
)

(type + Kind (is Union)
  (variant first)
  (variant second)
)

(type + Mixed (is Union)
  (variant none)
  (variant some (tuple Int))
  (variant word (tuple String))
  (variant pair (tuple Int Float))
)
```

```output
Class:core/number:I64
0

Class:core/string:String
1

second
Class:core/number:U8
1
none
Class:core/number:U8
0
0

(0, 0)
some
Class:core/number:I64
4
0
(1, 2.5)
Tuple:((0:Class:core/number:I64), (1:Class:core/number:F64))
pair:3::0
word:2:w:0
```

### A member of a union of objects is the member of the object it holds, with the arguments evaluated once.

```brix
(main
  (let shape Shape (call Circle 2))
  (print (call (. shape area) (call side 3)))
  (print (. shape sides))
  (print (. shape label))
  (call (. shape poke))
  (= shape (call Square 3))
  (print (call (. shape area) (call side 4)))
  (print (. shape sides))
  (print (. shape label))
  (call (. shape poke))
  (let box (call Box shape))
  (print (call (. box shape area) 1))
  (call (. box shape poke))
  (print (call total (call Square 2)))
  (print (call total shape))
)

(fn-do + side Int
  (param n Int)
  (print (str "side " n))
  (return n)
)

(fn-do + total Int
  (param shape Shape)
  (return (+ (call (. shape area) 2) (. shape sides)))
)

(type + Box
  (prop + shape Shape)
  (fn-do + This _
    (param shape _)
  )
)

(type + Circle
  (prop + radius Int)
  (prop + sides _ (= 0))
  (fn-do + This _
    (param radius _)
  )
  (fn-do + area Int
    (param scale Int)
    (return (* radius radius 3 scale))
  )
  (fn-expr + label String :get
    (str "circle " radius)
  )
  (fn-do + poke _
    (print "poke circle")
  )
)

(type + Square
  (prop + edge Int)
  (prop + sides _ (= 4))
  (fn-do + This _
    (param edge _)
  )
  (fn-do + area Int
    (param scale Int)
    (return (* edge edge scale))
  )
  (fn-expr + label String :get
    (str "square " edge)
  )
  (fn-do + poke _
    (print "poke square")
  )
)

(type + Shape (is Union)
  (variant Circle)
  (variant Square)
)
```

```output
side 3
36
0
circle 2
poke circle
side 4
36
4
square 3
poke square
9
poke square
12
22
```

### A method one case of a union raises from and another does not is forwarded, and its error leaves the caller.

```brix
(main
  (let a (call run (call (. Job quick) (call Quick))))
  (print a)
  (let b (catch (call run (call (. Job slow) (call Slow true)))))
  (print b)
  (print (catch (call run (call (. Job slow) (call Slow false)))))
)

(fn-do + run Int (?! Failed)
  (param job Job)
  (return (call (. job work)))
)

(type + Failed (is Error))

(type + Quick
  (prop + name _ (= "quick"))
  (fn-do + work Int
    (return 1)
  )
)

(type + Slow
  (prop + fails Bool)
  (prop + name _ (= "slow"))
  (fn-do + This _
    (param fails _)
  )
  (fn-do + work Int (?! Failed)
    (if fails
      (throw (call Failed))
    )
    (return 2)
  )
)

(type + Job (is Union)
  (variant quick Quick)
  (variant slow Slow)
)
```

```output
1

2
```

### A property each case of a union stores in its own type, and a setter each case has, are forwarded.

```brix
(main
  (let item Item (call (. Item left) 10))
  (print (. item value))
  (= (. item label) (call word "a"))
  (= item (call (. Item right) 2.5))
  (print (. item value))
  (= (. item label) (call word "b"))
)

(fn-do + side Int
  (param n Int)
  (print (str "side " n))
  (return n)
)

(fn-do + word String
  (param text String)
  (print (str "word " text))
  (return text)
)

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + scaled Float
    (param by Float)
    (* value by)
  )
  (fn-do + label _ :set (param text String)
    (print (str "left " text))
  )
)

(type + Right (is Tuple)
  (prop + value Float)
  (fn-expr + scaled Float
    (param by Int)
    (* value by)
  )
  (fn-do + label _ :set (param text String)
    (print (str "right " text))
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
10
word a
left a
2.5
word b
right b
```

### A method every case of a union raises from is forwarded under `catch`.

```brix
(main
  (let a (catch (call run (call (. Job quick) (call Quick false)))))
  (print a)
  (let b (catch (call run (call (. Job slow) (call Slow true)))))
  (print b)
  (print (catch (call run (call (. Job slow) (call Slow false)))))
  (print (catch (call run (call (. Job quick) (call Quick true)))))
)

(fn-do + run Int (?! Failed)
  (param job Job)
  (return (call (. job work)))
)

(type + Failed (is Error))

(type + Quick
  (prop + fails Bool)
  (prop + name _ (= "quick"))
  (fn-do + This _
    (param fails _)
  )
  (fn-do + work Int (?! Failed)
    (if fails
      (throw (call Failed))
    )
    (return 1)
  )
)

(type + Slow
  (prop + fails Bool)
  (prop + name _ (= "slow"))
  (fn-do + This _
    (param fails _)
  )
  (fn-do + work Int (?! Failed)
    (if fails
      (throw (call Failed))
    )
    (return 2)
  )
)

(type + Job (is Union)
  (variant quick Quick)
  (variant slow Slow)
)
```

```output
1

2
```

### A union that holds itself through an object forwards a member all the way down.

```brix
(main
  (let tree Tree (call (. Tree leaf) (call Leaf 3)))
  (print (. tree weight))
  (print (call (. tree describe) 1))
  (= tree (call (. Tree node) (call Node (call (. Tree leaf) (call Leaf 4)) 5)))
  (print (. tree weight))
  (print (call (. tree describe) 2))
  (let kind (. tree case))
  (print kind)
  (print (. tree name))
  (print (call sum (list (call (. Tree leaf) (call Leaf 1)) tree)))
)

(fn-do + sum Int
  (param trees (list Tree))
  (let total 0)
  (let index 0)
  (while (< index (. trees length))
    (+= total (. ([] trees index) weight))
    (+= index 1)
  )
  (return total)
)

(type + Leaf
  (prop + value Int)
  (fn-do + This _
    (param value _)
  )
  (fn-expr + weight Int :get
    value
  )
  (fn-expr + describe String
    (param depth Int)
    (str "leaf " value " at " depth)
  )
)

(type + Node
  (prop + child Tree)
  (prop + extra Int)
  (fn-do + This _
    (param child _)
    (param extra _)
  )
  (fn-expr + weight Int :get
    (+ (. child weight) extra)
  )
  (fn-expr + describe String
    (param depth Int)
    (str "node over (" (call (. child describe) (+ depth 1)) ")")
  )
)

(type + Tree (is Union)
  (variant leaf Leaf)
  (variant node Node)
)
```

```output
3
leaf 3 at 1
9
node over (leaf 4 at 3)
1
node
10
```

### Objects and atoms held in tuples, structs and unions compare by identity

```brix
(main
  (let box (call Box))
  (let other (call Box))
  (let a (tuple 1 box))
  (let b (tuple 1 box))
  (let c (tuple 1 other))
  (print (== a b))
  (print (== a c))
  (print (!= a c))
  (let k (tuple (as "x" Atom) 2))
  (let l (tuple (as "x" Atom) 2))
  (let m (tuple (as "y" Atom) 2))
  (print (== k l))
  (print (== k m))
  (let p Holder box)
  (let q Holder box)
  (let r Holder other)
  (let s Holder (. Holder none))
  (print (== p q))
  (print (== p r))
  (print (!= p r))
  (print (== p s))
  (print (== s (. Holder none)))
  (print (!= s p))
  (let w (call Wrap box 1))
  (let x (call Wrap box 1))
  (let y (call Wrap other 1))
  (print (== w x))
  (print (== w y))
  (let deep (tuple 1 (tuple box "t") p))
  (let same (tuple 1 (tuple box "t") q))
  (let apart (tuple 1 (tuple box "t") r))
  (print (== deep same))
  (print (== deep apart))
  (let opt (? (tuple Int Box)) (tuple 1 box))
  (let none (? (tuple Int Box)) null)
  (print (== opt a))
  (print (== a opt))
  (print (== opt c))
  (print (== none a))
  (print (== none none))
)

(type + Box
  (prop + value Int (= 1))
)

(type + Wrap (is Struct)
  (prop + box Box)
  (prop + count Int)
)

(type + Holder (is Union)
  (variant none)
  (variant Box)
  (variant Int)
)
```

```output
true
false
true
true
false
true
false
true
false
true
true
true
false
true
false
true
true
false
false
true
```

### Union compared with a value of the type a case holds

```brix
(main
  (let foo (. Foo error))
  (let bar (. Foo life))
  (let baz Foo 7)
  (let flo Foo 2.5)
  (let sta (. Foo status))
  (print (== foo 1))
  (print (== bar 42))
  (print (== bar 1))
  (print (== baz 7))
  (print (== baz 1))
  (print (== flo 2.5))
  (print (== flo 2))
  (print (== sta "good"))
  (print (== sta "bad"))
  (print (== foo "good"))
  (print (== sta 1))
  (print (!= foo 1))
  (print (!= baz 7))
)

(type + Foo (is Union)
  (variant error (= 1))
  (variant life (= 42))
  (variant status (= "good"))
  (variant Float)
  (variant Int)
)
```

```output
true
false
false
true
false
true
false
true
false
false
false
false
false
```

### Valued case keeps its own number where another case holds the same type

```brix
(main
  (let foo (. Foo error))
  (let bar (. Foo life))
  (let sta (. Foo status))
  (print (. foo case))
  (print (. bar case))
  (print (. sta case))
  (print (== bar (. Foo error)))
  (print (== foo (. Foo life)))
  (print (== sta (. Foo status)))
  (print (!= bar (. Foo status)))
  (print (. bar name))
  (print bar)
  (print (call held bar))
  (print (call held foo))
  (print (call held 7))
  (print (call same bar 42))
  (print (call same foo 1))
  (print (call same bar 1))
  (print (call same 7 7))
)

(fn-expr + held U8
  (param value Foo)
  (. value case)
)

(fn-expr + same Bool
  (param value Foo)
  (param number Int)
  (== value number)
)

(type + Foo (is Union)
  (variant error (= 1))
  (variant life (= 42))
  (variant status (= "good"))
  (variant Float)
  (variant Int)
)
```

```output
0
1
2
false
false
true
true
life
42
1
0
0
false
true
false
true
```

### Union compared with an object and a number

```brix
(main
  (let box (call Box))
  (let p Holder box)
  (let q Holder 3)
  (let r Holder (. Holder none))
  (print (== p box))
  (print (== p (call Box)))
  (print (== q 3))
  (print (== q 4))
  (print (== r 3))
  (print (== r box))
  (print (!= p box))
  (print (!= q 3))
)

(type + Box
  (prop + value Int (= 1))
)

(type + Holder (is Union)
  (variant none)
  (variant Box)
  (variant Int)
)
```

```output
true
false
true
false
false
false
false
false
```

### An optional union compared with a value of the type a case holds

```brix
(main
  (let no (? Flag) false)
  (let yes (? Flag) true)
  (let count (? Flag) 3)
  (let none (? Flag) null)
  (print (== no false))
  (print (== no true))
  (print (== yes true))
  (print (== count false))
  (print (== count 3))
  (print (== none false))
  (print (!= no false))
  (print (!= none false))
  (print (call same no false))
  (print (call same none false))
  (print (call same count null))
)

(fn-expr + same Bool
  (param value (? Flag))
  (param other (? Bool))
  (== value other)
)

(type + Flag (is Union)
  (variant Bool)
  (variant Int)
)
```

```output
true
false
true
false
true
false
false
true
true
false
false
```

### Union compared with a value in conditions and by a declared ==

```brix
(main
  (let name Named "ann")
  (let other Named "bob")
  (let count Named 3)
  (let box (call Box 2))
  (let held Named box)
  (print (== name "ann"))
  (print (== name "bob"))
  (print (!= other "ann"))
  (print (== count 3))
  (print (== count "3"))
  (print (== held box))
  (print (== held (call Box 2)))
  (print (!= held box))
  (print (call label name))
  (print (call label count))
  (print (call label held))
  (print (call label other))
  (if (and (== name "ann") (== count 3))
    (print "both")
  )
  (let maybe (? Named) name)
  (print (call twice (call make) "ann"))
)

(fn-do + make Named
  (print "make")
  (return "ann")
)

(fn-expr + twice Bool
  (param value Named)
  (param text String)
  (and (== value text) (!= value "zed"))
)

(fn-expr + label String
  (param value Named)
  (case? (== value "ann")
    "is ann"
  )
  (case? (== value 3)
    "is three"
  )
  (default
    "other"
  )
)

(type + Box
  (prop + x Int)
  (fn-do + This _
    (param x _)
  )
  (fn-do + (infix this == (param other Box)) _
    (print "box eq")
    (return (== x (. other x)))
  )
)

(type + Named (is Union)
  (variant nothing)
  (variant String)
  (variant Int)
  (variant Box)
)
```

```output
true
false
true
true
false
box eq
true
box eq
true
box eq
false
is ann
is three
other
other
both
make
true
```

### A union is tested for the first of its cases that is a list

```brix
(main
  (let foo Foo (list (call Foo 42)))
  (print (is foo List))
  (print (is-not foo List))
  (let bar Foo 42)
  (print (is bar List))
  (let baz Foo "text")
  (print (is baz List))
  (print (is baz String))
  (call show foo)
  (call show bar)
)

(type + Foo (is Union)
  (variant Int)
  (variant String)
  (variant (list Foo))
)

(fn-do + show _
  (param value Foo)
  (if (is value List)
    (print "list")
    (else
      (print "not a list")
    )
  )
)
```

```output
true
false
false
false
true
list
not a list
```

### A case read by name where the union holds another gives what its type makes with nothing written.

```brix
(main
  (let payload (call (. Payload box) (call Box 3)))
  (print (. payload box size))
  (let other (. Payload empty))
  (print (. other box size))
)

(type + Box
  (prop + size _ (= 0))
  (fn-do + This _
    (param n Int)
    (= size n)
  )
)

(type + Payload (is Union)
  (variant box Box)
  (variant empty)
)
```

```output
3
0
```

### A case of a tuple, a list or text read by name where the union holds another gives its empty value.

```brix
(main
  (let payload (call (. Payload point) (call Point 1 2)))
  (print (. payload point))
  (let other (. Payload empty))
  (print (. other point))
  (print (. other names length))
  (print (str "[" (. other label) "]"))
)

(type + Point (is (tuple (: x Int) (: y Int))))

(type + Payload (is Union)
  (variant point Point)
  (variant names (list String))
  (variant label String)
  (variant empty)
)
```

```output
(1, 2)
(0, 0)
0
[]
```

### A panic through a member a union forwards names its function

```brix crash
(main
  (let item Item (call (. Item left) (call Left 1)))
  (print (call (. item render)))
  (= item (call (. Item right) (call Right 2)))
  (print (call (. item render)))
)

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    (throw (call Panic "left"))
  )
)

(type + Right (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    "right"
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```stderr
Unhandled panic: left
<no file:12>

Task 1

Stack trace:
  0  __source__:Left:render no file:12:5
  1  __source__:union-dispatch-1 no file:4:10
  2  __source__:main no file:4:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### An unhandled error through a member a union forwards names its function

```brix crash
(main
  (let item Item (call (. Item left) (call Left 1)))
  (print (call (. item render)))
)

(type + LeftError (is Error))

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + render String (?! LeftError)
    (throw (call LeftError "left"))
  )
)

(type + Right (is Tuple)
  (prop + value I32)
  (fn-expr + render String (?! LeftError)
    "right"
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```stderr
Unhandled error: left
<no file:12>

Task 1

Stack trace:
  0  __source__:Left:render no file:12:5
  1  __source__:union-dispatch-1 no file:4:10
  2  __source__:main no file:4:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A member a union forwards that raises, caught in a function

```brix
(main
  (print (call show (call (. Item right) (call Right 2))))
  (print (call show (call (. Item left) (call Left 1))))
)

(type + LeftError (is Error))

(type + RightError (is Error))

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + render String (?! LeftError)
    (throw (call LeftError "left"))
  )
)

(type + Right (is Tuple)
  (prop + value I32)
  (fn-expr + render String (?! RightError)
    "right"
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)

(fn-do + show String
  (param item Item)
  (return (?? (catch? (call (. item render))) "caught"))
)
```

```output
right
caught
```

### A panic from the second call of a member a union forwards

```brix crash
(main
  (let b Item (call (. Item left) (call Left 3)))
  (print (call (. b scale) 2))
  (print (call (. b scale) 12))
)

(type + LeftError (is Error))

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    (before
      (throw (call Panic "left") (if (> value 5)))
    )
    (str "left " value)
  )
  (fn-expr + scale String
    (param by Int)
    (before
      (throw (call Panic "scale") (if (> by 9)))
    )
    (str "scaled " (* value by))
  )
  (fn-expr + check String (?! LeftError)
    (before
      (throw (call LeftError "bad left") (if (> value 5)))
    )
    "ok left"
  )
)

(type + Right (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    (str "right " value)
  )
  (fn-expr + scale String
    (param by Int)
    (str "rscaled " (+ value by))
  )
  (fn-expr + check String (?! LeftError)
    "ok right"
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```stderr
Unhandled panic: scale
<no file:21>

Task 1

Stack trace:
  0  __source__:Left:scale no file:21:7
  1  __source__:union-dispatch-1 no file:4:10
  2  __source__:main no file:5:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A member a union forwards that raises, caught twice

```brix
(main
  (let b Item (call (. Item left) (call Left 9)))
  (print (?? (catch? (call (. b check))) "caught"))
  (let c Item (call (. Item left) (call Left 1)))
  (print (?? (catch? (call (. c check))) "caught"))
)

(type + LeftError (is Error))

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    (before
      (throw (call Panic "left") (if (> value 5)))
    )
    (str "left " value)
  )
  (fn-expr + scale String
    (param by Int)
    (before
      (throw (call Panic "scale") (if (> by 9)))
    )
    (str "scaled " (* value by))
  )
  (fn-expr + check String (?! LeftError)
    (before
      (throw (call LeftError "bad left") (if (> value 5)))
    )
    "ok left"
  )
)

(type + Right (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    (str "right " value)
  )
  (fn-expr + scale String
    (param by Int)
    (str "rscaled " (+ value by))
  )
  (fn-expr + check String (?! LeftError)
    "ok right"
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
caught
ok left
```

### A member a union forwards that raises, caught by a block

```brix
(main
  (let b Item (call (. Item left) (call Left 7)))
  (let t
    (call (. b check)
      (block
        (on-error (: error Error)
          (str "caught " (. error message))
        )
      )
    )
  )
  (print t)
)

(type + LeftError (is Error))

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    (before
      (throw (call Panic "left") (if (> value 5)))
    )
    (str "left " value)
  )
  (fn-expr + scale String
    (param by Int)
    (before
      (throw (call Panic "scale") (if (> by 9)))
    )
    (str "scaled " (* value by))
  )
  (fn-expr + check String (?! LeftError)
    (before
      (throw (call LeftError "bad left") (if (> value 5)))
    )
    "ok left"
  )
)

(type + Right (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    (str "right " value)
  )
  (fn-expr + scale String
    (param by Int)
    (str "rscaled " (+ value by))
  )
  (fn-expr + check String (?! LeftError)
    "ok right"
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
caught bad left
```

### A member a union forwards compares the low 32 bits of a wide field

```brix
(main
  (let item Item (call (. Item wide) (call Wide -4294967287)))
  (print (call (. item above)))
)

(type + Wide (is Tuple)
  (prop + value I64)
  (fn-expr + above String
    (str "above " (> (as value I32) 5))
  )
)

(type + Narrow (is Tuple)
  (prop + value I32)
  (fn-expr + above String
    (str "above " (> value 5))
  )
)

(type + Item (is Union)
  (variant wide Wide)
  (variant narrow Narrow)
)
```

```output
above true
```

### A member a union forwards from two places of one function panics

```brix crash
(main
  (print (call twice (call (. Item left) (call Left 1))))
  (print (call twice (call (. Item left) (call Left 8))))
)

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    (before
      (throw (call Panic "left") (if (> value 5)))
    )
    (str "left " value)
  )
)

(type + Right (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    (str "right " value)
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)

(fn-do + twice String
  (param item Item)
  (let first (call (. item render)))
  (return (str first "/" (call (. item render))))
)
```

```stderr
Unhandled panic: left
<no file:11>

Task 1

Stack trace:
  0  __source__:Left:render no file:11:7
  1  __source__:union-dispatch-1 no file:31:14
  2  __source__:twice no file:31:14
  3  __source__:main no file:4:10
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A lazy property a union forwards panics in its value

```brix crash
(main
  (let a Item (call (. Item right) (call Right)))
  (print (. a label))
  (let b Item (call (. Item left) (call Left)))
  (print (. b label))
)

(type + Left
  (prop + value I32 (= 7))
  (prop + label String (:= (call pick value)))
)

(type + Right
  (prop + value I32 (= 1))
  (prop + label String (:= "right"))
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)

(fn-do + pick String
  (param n I32)
  (if (> n 5)
    (throw (call Panic "too big"))
  )
  (return "small")
)
```

```stderr
Unhandled panic: too big
<no file:27>

Task 1

Stack trace:
  0  __source__:pick no file:27:5
  1  __source__:Left:label no file:11:28
  2  __source__:main no file:6:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A member a union forwards in a loop panics on the third item

```brix crash
(main
  (let items (list (call (. Item left) (call Left 1)) (call (. Item right) (call Right 2)) (call (. Item left) (call Left 6))))
  (for (in each items)
    (print (call (. each render)))
  )
)

(type + LeftError (is Error))

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    (before
      (throw (call Panic "left") (if (> value 5)))
    )
    (str "left " value)
  )
  (fn-expr + scale String
    (param by Int)
    (before
      (throw (call Panic "scale") (if (> by 9)))
    )
    (str "scaled " (* value by))
  )
  (fn-expr + check String (?! LeftError)
    (before
      (throw (call LeftError "bad left") (if (> value 5)))
    )
    "ok left"
  )
)

(type + Right (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    (str "right " value)
  )
  (fn-expr + scale String
    (param by Int)
    (str "rscaled " (+ value by))
  )
  (fn-expr + check String (?! LeftError)
    "ok right"
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```stderr
Unhandled panic: left
<no file:15>

Task 1

Stack trace:
  0  __source__:Left:render no file:15:7
  1  __source__:union-dispatch-1 no file:5:12
  2  __source__:main no file:5:12
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A setter a union forwards before a forwarded method is named union-dispatch-1 in a trace.

```brix crash
(main
  (let item Item (call (. Item right) 1.5))
  (= (. item label) "x")
  (print (call (. item scale) 2.0))
  (= item (call (. Item left) 3))
  (= (. item label) "boom")
)

(type + Left (is Tuple)
  (prop + value I32)
  (fn-do + label _ :set (param text String)
    (print (str "left " text))
    (throw (call Panic "bad left") (if (== text "boom")))
  )
  (fn-do + scale Float
    (param by Float)
    (throw (call Panic "bad scale") (if (== by 0)))
    (return (* value by))
  )
)

(type + Right (is Tuple)
  (prop + value Float)
  (fn-do + label _ :set (param text String)
    (print (str "right " text))
  )
  (fn-do + scale Float
    (param by Float)
    (return (* value by))
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```stderr
Unhandled panic: bad left
<no file:14>

Task 1

Stack trace:
  0  __source__:Left:label no file:14:5
  1  __source__:union-dispatch-1 no file:4:6
  2  __source__:main no file:7:6
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A setter a union forwards is given the value written, a local or a text made from one.

```brix
(main
  (let item Item (call (. Item left) 1))
  (= (. item label) "a")
  (let name "b")
  (= (. item label) name)
  (= (. item label) (str name "c"))
)

(type + Left (is Tuple)
  (prop + value I32)
  (fn-do + label _ :set (param text String)
    (print (str "left " text))
    (throw (call Panic "bad left") (if (== text "boom")))
  )
  (fn-do + scale Float
    (param by Float)
    (throw (call Panic "bad scale") (if (== by 0)))
    (return (* value by))
  )
)

(type + Right (is Tuple)
  (prop + value Float)
  (fn-do + label _ :set (param text String)
    (print (str "right " text))
  )
  (fn-do + scale Float
    (param by Float)
    (return (* value by))
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
left a
left b
left bc
```

```brix
(main
  (let item Item (call (. Item left) 10))
  (print (call (. item render) 1))
  (print (call (. item render) 2))
  (print (call (. item render) "x"))
)

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + render String
    (param amount I32)
    (str "L" (+ value amount))
  )
  (fn-expr + render String
    (param suffix String)
    (str "L" suffix)
  )
)

(type + Right (is Tuple)
  (prop + value String)
  (fn-expr + render String
    (param amount I32)
    (str value amount)
  )
  (fn-expr + render String
    (param suffix String)
    (str value suffix)
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
L11
L12
Lx
```

```brix
(main
  (print (call describe (call entry null)))
  (print (call describe (call entry 1)))
)

(type + Value (is Union)
  (variant Null)
  (variant Int)
  (variant (list Value))
)

(fn-expr + entry _
  (param value (? Int))
  (call (of Map String Value)
    (block
      (:values (as (call optional-items value) Value))
    )
  )
)

(fn-expr _ optional-items _
  (param value (? Int))
  (match? value
    (case Int
      (call items it)
    )
  )
)

(fn-expr _ items _
  (param value Int)
  (item (as value Value))
  (item (as (+ value 1) Value))
)

(fn-expr + describe String
  (param map (map String Value))
  (match (!! ([] map "values"))
    (case Null
      (str (. map length) " null")
    )
    (case Int
      (str (. map length) " int")
    )
    (case (: list List)
      (str (. map length) " list " (call (. ", " join) list))
    )
  )
)
```

```output
1 null
1 list 1, 2
```

### A base class test on a union holding a subclass in its own case matches and narrows to the base.

```brix
(main
  (call show (call Base))
  (call show (call Derived))
  (call show 7)
)

(fn-do _ show _
  (param value Choice)
  (if (is value Base)
    (print (. value name))
    (else
      (print "not base")
    )
  )
)

(type + Base
  (prop + name _ (= "base"))
)

(type + Derived (is Base)
  (prop + name _ (= "derived"))
)

(type + Choice (is Union)
  (variant Int)
  (variant Base)
  (variant Derived)
)
```

```output
base
derived
not base
```

### A subclass test on a union holding a subclass object in the base class's case matches and narrows to the subclass, as a match arm for the subclass ahead of one for the base does.

```brix
(main
  (let held Base (call Derived))
  (call show held)
  (call show (call Derived))
  (call show (call Base))
  (call show 7)
)

(fn-do _ show _
  (param value Choice)
  (let arm
    (match value
      (case (: d Derived)
        (str "derived arm " (. d extra))
      )
      (case (: b Base)
        (str "base arm " (. b name))
      )
      (default
        "other arm"
      )
    )
  )
  (if (is value Derived)
    (print (str "derived " (. value extra) ", " arm))
    (else
      (print (str "not derived, " arm))
    )
  )
)

(type + Base
  (prop + name _ (= "base"))
)

(type + Derived (is Base)
  (prop + extra _ (= "extra"))
)

(type + Choice (is Union)
  (variant Int)
  (variant Base)
  (variant Derived)
)
```

```output
derived extra, derived arm extra
derived extra, derived arm extra
not derived, base arm base
not derived, other arm
```

### A union cast to a subclass, or narrowed past `is not` the subclass, gives a subclass object held in the base class's case.

```brix
(main
  (let held Base (call Derived))
  (let value Choice held)
  (print (. (as value Derived) extra))
  (print (call describe value))
  (print (call describe (call Base)))
)

(fn-do _ describe String
  (param value Choice)
  (if (is-not value Derived)
    (return "not derived")
  )
  (return (str "derived " (. value extra)))
)

(type + Base
  (prop + name _ (= "base"))
)

(type + Derived (is Base)
  (prop + extra _ (= "extra"))
)

(type + Choice (is Union)
  (variant Int)
  (variant Base)
  (variant Derived)
)
```

```output
extra
derived extra
not derived
```

### A subclass test on a union with a case for the base class and none for the subclass narrows the union to the subclass.

```brix
(main
  (call show (call Derived))
  (call show (call Base))
  (call show 7)
)

(fn-do _ show _
  (param value Choice)
  (if (is value Derived)
    (print (str "derived " (. value extra)))
    (else
      (print "not derived")
    )
  )
)

(type + Base
  (prop + name _ (= "base"))
)

(type + Derived (is Base)
  (prop + extra _ (= "extra"))
)

(type + Choice (is Union)
  (variant Int)
  (variant Base)
)
```

```output
derived extra
not derived
not derived
```

### An attached catch for a subclass error catches one of that class raised as its base class, ahead of a catch for the base.

```brix
(main
  (call show 1)
  (call show 2)
  (call show 3)
)

(fn-do _ show _
  (param n Int)
  (let text
    (call describe n
      (block
        (on-error IOError
          "io"
        )
        (on-error Error
          "error"
        )
      )
    )
  )
  (print text)
)

(fn-do _ describe String (?! Error) (?! IOError)
  (param n Int)
  (if (== n 1)
    (let held Error (call IOError "held"))
    (throw held)
  )
  (if (== n 2)
    (throw (call Error "plain"))
  )
  (return "ok")
)
```

```output
io
error
ok
```

### An error of a class above every error a function raises leaves it as the most derived of them that the error is of, raised there or by a call.

```brix
(main
  (print (call describe (catch (call raise 1))))
  (print (call describe (catch (call raise 2))))
  (print (call describe (catch (call relay 1))))
  (print (call describe (catch (call relay 2))))
)

(fn-expr _ describe String
  (param result _)
  (match result
    (case (: e Missing)
      (str "missing " (. e message))
    )
    (case (: e IOError)
      (str "io " (. e message))
    )
    (default
      "ok"
    )
  )
)

(type + Missing (is IOError))

(fn-do _ raise Int (?! IOError) (?! Missing)
  (param n Int)
  (let error Error (if-expr (== n 1) (call Missing "gone") (call IOError "disk")))
  (if (< n 3)
    (throw error)
  )
  (return n)
)

(fn-do _ relay Int (?! IOError) (?! Missing)
  (param n Int)
  (return (call inner n))
)

(fn-do _ inner Int (?! Error)
  (param n Int)
  (if (== n 1)
    (throw (call Missing "gone"))
  )
  (if (== n 2)
    (throw (call IOError "disk"))
  )
  (return n)
)
```

```output
missing gone
io disk
missing gone
io disk
```

### A caught Result that can hold Error and one of its subclasses is an Error in either case.

```brix
(main
  (call show "derived first" (catch (call derived-first 1)))
  (call show "base first" (catch (call base-first 1)))
  (call show "base thrown" (catch (call derived-first 9)))
)

(fn-do _ show _
  (param label String)
  (param result _)
  (if (is result Error)
    (print (str label " caught " (. (as result Error) message)))
    (else
      (print (str label " not caught"))
    )
  )
)

(fn-do _ derived-first Int
  (param n Int)
  (if (> n 5)
    (throw (call Error "plain"))
  )
  (if (> n 0)
    (throw (call IOError "boom"))
  )
  (return n)
)

(fn-do _ base-first Int
  (param n Int)
  (if (> n 0)
    (throw (call IOError "boom"))
  )
  (if (> n 5)
    (throw (call Error "plain"))
  )
  (return n)
)
```

```output
derived first caught boom
base first caught boom
base thrown caught plain
```

```brix
(main
  (let made (call .other "x" true))
  (print (is made .other))
)

(type + Partial (is Union)
  (variant get (tuple (: name String)))
  (variant other (tuple (: name String) (: is-optional Bool)))
)
```

```output
true
```

```brix crash
(main
  (let made (call .nothing-here "x" true))
  (print 1)
)
```

```output
Exception: Unable to resolve symbol .nothing-here
<no file:3>
      (let made (call .nothing-here "x" true))
                ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A class instance converts into a union through an interface it implements.

```brix
(main
  (print (call describe (call Reader)))
  (print (call describe "text"))
  (print (call describe null))
  (print (call (. (call Holder (call Reader)) describe-body)))
)

(type + Readable (is Interface)
  (fn-expr + label String :get)
)

(type + Reader
  (implements Readable)
  (fn-expr + label _ :get
    "reader"
  )
)

(type + Body (is Union)
  (variant Null)
  (variant String)
  (variant Readable)
)

(type + Holder
  (prop + body Body)
  (fn-do + This _
    (param body Body)
  )
  (fn-expr + describe-body _
    (call describe body)
  )
)

(fn-expr + describe _
  (param body Body)
  (match body
    (case Null
      "null"
    )
    (case (: text String)
      (str "string " text)
    )
    (case (: source Readable)
      (str "readable " (. source label))
    )
  )
)
```

```output
readable reader
string text
null
readable reader
```

### A union's own string converter wins over a named case that holds a string.

```brix
(main
  (print (. Verb get))
  (print (call (. Verb other) "PURGE"))
  (print (str "verb " (. Verb post)))
)

(type + Verb (is Union)
  (variant get)
  (variant post)
  (variant other (tuple (: name String)))
  (fn-to-expr + String
    (match this
      (case .get
        "GET"
      )
      (case .post
        "POST"
      )
      (case (call .other name)
        name
      )
    )
  )
)
```

```output
GET
PURGE
verb POST
```

### Testing a union's interface member for one implementing class checks the held object's class.

```brix
(main
  (print (. (call Holder (call First)) holds-first))
  (print (. (call Holder (call Second)) holds-first))
)

(type + Named (is Interface)
  (fn-expr + label String :get)
)

(type + First
  (implements Named)
  (fn-expr + label _ :get
    "first"
  )
)

(type + Second
  (implements Named)
  (fn-expr + label _ :get
    "second"
  )
)

(type + Slot (is Union)
  (variant Null)
  (variant Named)
)

(type + Holder
  (prop + slot Slot)
  (fn-do + This _
    (param slot Slot)
  )
  (fn-expr + holds-first _ :get
    (is slot First)
  )
)
```

```output
true
false
```

### Testing or matching as text a union that converts itself to text finds the case that holds text, though converting it gives the text it converts to.

```brix
(main
  (print (call size (. Verb get)))
  (print (call size (call (. Verb other) "PURGE")))
  (print (call pick (. Verb post)))
  (print (call pick (call (. Verb other) "x")))
  (print (str "verb " (call (. Verb other) "x")))
)

(fn-do + size Int
  (param verb Verb)
  (if (is-not verb String)
    (return -1)
  )
  (return (. verb length))
)

(fn-expr + pick String
  (param verb Verb)
  (match verb
    (case (: s String)
      (str "text " s)
    )
    (default
      (inline "none")
    )
  )
)

(type + Verb (is Union)
  (variant get)
  (variant post)
  (variant other (tuple (: name String)))
  (fn-to-expr + String
    (match this
      (case .get
        "GET"
      )
      (case .post
        "POST"
      )
      (case (call .other name)
        (str "<" name ">")
      )
    )
  )
)
```

```output
-1
5
none
text x
verb <x>
```

### An arm that names a map type gives `it` as that type.

```brix
(main
  (let map (call (of Map String Int)))
  (= ([] map "b") 3)
  (call show map)
  (call show 4)
)

(type + Value (is Union)
  (variant Int)
  (variant (map String Int))
)

(fn-do _ show _
  (param value Value)
  (match value
    (case (map String Int)
      (print (!! ([] it "b")))
    )
    (default
      (print "not a map")
    )
  )
)
```

```output
3
not a map
```

### A match written as a statement runs its arms for what they do, one ending in a call that gives nothing.

```brix
(main
  (print (call measure (list (call (. W flex) 2) (call (. W fixed) 3) (. W auto))))
)

(type + W (is Union)
  (variant auto)
  (variant fixed Int)
  (variant flex Int)
)

(fn-do + measure Int
  (param widths (list W))
  (let total 0)
  (let flexes (list Int) (call (of List Int)))
  (for (in w widths)
    (let width 1)
    (match w
      (case (call (. W fixed) value)
        (= width value)
      )
      (case (call (. W flex) flex)
        (+= total flex)
        (+= flexes flex)
      )
      (default
        (= width 7)
      )
    )
    (+= total width)
  )
  (return (+ total (. flexes length)))
)
```

```output
14
```

### A case named by its symbol alone that gives an arm of a returned match is of the type the function returns, though a union declared after that type has the case too.

```brix
(main
  (print (str (call cap 0)))
  (print (str (call cap 1)))
)

(fn-expr + cap Cap
  (param index _)
  (match kind
    (case= 0
      .round
    )
    (default
      .square
    )
  )
  (where
    (lazy kind (% index 2))
  )
)

(type + Cap (is Union)
  (variant butt)
  (variant round)
  (variant square)
  (fn-to-expr + String
    (case? (only-if "round" (if (== this (. Cap round)))))
    (case? (only-if "square" (if (== this (. Cap square)))))
    (default
      (inline "butt")
    )
  )
)

(type + Join (is Union)
  (variant miter)
  (variant round)
  (variant bevel)
)
```

```output
round
square
```

### A union, or an optional of one, cast to a subclass that has no case of its own is a checked down-cast through the case of a class it derives from, and a test or an arm for the subclass checks the object held there.

```brix
(main
  (let held Base (call Derived))
  (let derived Choice held)
  (let optional (? Choice) held)
  (let base Choice (call Base))
  (let number Choice 7)
  (print (. (as derived Derived) extra))
  (print (. (as optional Derived) extra))
  (print (catch (as base Derived)))
  (print (catch (as number Derived)))
  (print (call describe derived))
  (print (call describe base))
  (print (call describe number))
)

(fn-do _ describe String
  (param value Choice)
  (let arm
    (match value
      (case (: d Derived)
        (str "derived arm " (. d extra))
      )
      (case (: b Base)
        (str "base arm " (. b name))
      )
      (default
        "other arm"
      )
    )
  )
  (if (is-not value Derived)
    (return (str "not derived, " arm))
  )
  (return (str "derived " (. value extra) ", " arm))
)

(type + Base
  (prop + name _ (= "base"))
)

(type + Derived (is Base)
  (prop + extra _ (= "extra"))
)

(type + Choice (is Union)
  (variant Int)
  (variant Base)
)
```

```output
extra
extra
Invalid union cast
Invalid union cast
derived extra, derived arm extra
not derived, base arm base
not derived, other arm
```

### A union cast to a class no case of which derives from or is derived from it is illegal.

```brix crash
(main
  (let value Choice 7)
  (print (. (as value Other) name))
)

(type + Base
  (prop + name _ (= "base"))
)

(type + Other
  (prop + name _ (= "other"))
)

(type + Choice (is Union)
  (variant Int)
  (variant Base)
)
```

```output
Exception: Choice cannot be cast to Other; there is no conversion between them
<no file:4>
      (print (. (as value Other) name))
                ^^^^^^^^^^^^^^^^
```

### The false edge of a test for a class keeps a union's case for an interface the class implements or for a class it derives from, either of which may hold an object of another class.

```brix
(main
  (call show-slot (call First))
  (call show-slot (call Second))
  (call show-slot 7)
  (call show-choice (call Derived))
  (call show-choice (call Base))
  (call show-choice 7)
)

(fn-do _ show-slot _
  (param value Slot)
  (if (is value First)
    (print "first")
    (else
      (print (call describe-slot value))
    )
  )
)

(fn-expr _ describe-slot String
  (param value Slot)
  (match value
    (case Int
      "int"
    )
    (case (: n Named)
      (str "named " (. n label))
    )
  )
)

(fn-do _ show-choice _
  (param value Choice)
  (if (is value Derived)
    (print "derived")
    (else
      (print (call describe-choice value))
    )
  )
)

(fn-expr _ describe-choice String
  (param value Choice)
  (match value
    (case Int
      "int"
    )
    (case (: b Base)
      (str "base " (. b name))
    )
  )
)

(type + Named (is Interface)
  (fn-expr + label String :get)
)

(type + First
  (implements Named)
  (fn-expr + label _ :get
    "first"
  )
)

(type + Second
  (implements Named)
  (fn-expr + label _ :get
    "second"
  )
)

(type + Slot (is Union)
  (variant Int)
  (variant Named)
)

(type + Base
  (prop + name _ (= "base"))
)

(type + Derived (is Base)
  (prop + name _ (= "derived"))
)

(type + Choice (is Union)
  (variant Int)
  (variant Base)
)
```

```output
first
named second
int
derived
base base
int
```

### A case an `is` test names must be a case some type declares.

```brix crash
(main
  (let shape Shape (call Circle 2))
  (print (is shape .triangle))
)

(type + Circle (is Tuple)
  (prop + radius Int)
)

(type + Square (is Tuple)
  (prop + side Int)
)

(type + Shape (is Union)
  (variant circle Circle)
  (variant square Square)
)
```

```output
Exception: Case ".triangle" not found in Shape
<no file:4>
      (print (is shape .triangle))
                       ^^^^^^^^^
```

### A local of a union whose first case is Null, declared with no value, holds null.

```brix
(main
  (let empty Shape)
  (print (call describe empty))
  (= empty "text")
  (print (call describe empty))
)

(type + Shape (is Union)
  (variant Null)
  (variant I64)
  (variant String)
)

(fn-expr _ describe String
  (param value Shape)
  (case? (is value Null)
    "null"
  )
  (case? (is value String)
    "text"
  )
  (default
    (str "number " (as value I64))
  )
)
```

```output
null
text
```

```brix
(main
  (let a (call (. Part text) "x"))
  (let b (call (. Part item) (call Thing 1)))
  (print (!= a b))
  (print (== a (call (. Part text) "x")))
)

(type + Thing
  (implements Valued)
  (prop + id Int)
  (fn-do + This _
    (param id _)
  )
  (fn-expr + key String :get
    (str id)
  )
)

(type + Part (is Union)
  (variant text String)
  (variant item Valued)
)

(type + Valued (is Interface)
  (fn-expr + key String :get)
)
```

```output
true
true
```

### A number alias wraps into the union case of its own kind before converting to another one

```brix
(main
  (let amount Amount 1.5)
  (let span Span 2.5)
  (let count Count 3)
  (let small F32 4.5)
  (print (call describe amount))
  (print (call describe span))
  (print (call describe count))
  (print (call describe small))
  (let listed (call (of List Number) (in each (list amount (* amount 2)))))
  (print (call describe ([] listed 1)))
)

(type + Amount (is Float))

(type + Span (alias sp) (is Float))

(type + Count (is Int))

(type + Number (is Union)
  (variant Int)
  (variant Float)
)

(fn-expr + describe String
  (param value Number)
  (match value
    (case (: whole Int)
      (str "int " whole)
    )
    (case (: real Float)
      (str "float " real)
    )
  )
)
```

```output
float 1.5
float 2.5
int 3
float 4.5
float 3
```

### A union whose every case is a class casts to a pointer at the object it holds

```brix
(main
  (let circle (call Circle 1))
  (print (call same circle circle))
  (print (call same circle (call Square 1)))
  (print (call same circle (call Circle 1)))
)

(type + Shape (is Union)
  (variant Circle)
  (variant Square)
)

(type + Circle
  (prop + radius Int)
  (fn-do + This _
    (param radius _)
  )
)

(type + Square
  (prop + side Int)
  (fn-do + This _
    (param side _)
  )
)

(fn-expr + same Bool
  (param first Shape)
  (param second Shape)
  (== (as first Pointer) (as second Pointer))
)
```

```output
true
false
false
```

### A union casts to a number its case converts to by converting the value that case holds

```brix
(main
  (let whole Reading 3)
  (print (/ (as whole Float) 2))
  (print (as whole I32))
)

(type + Reading (is Union)
  (variant Int)
  (variant String)
)
```

```output
1.5
3
```

### A pointer does not cast to a union, since nothing says which case it would hold

```brix compile-error
(main
  (let circle (call Circle 1))
  (let shape (as (as circle Pointer) Shape))
)

(type + Shape (is Union)
  (variant Circle)
  (variant Square)
)

(type + Circle
  (prop + radius Int)
  (fn-do + This _
    (param radius _)
  )
)

(type + Square
  (prop + side Int)
  (fn-do + This _
    (param side _)
  )
)
```

```output
Exception: Pointer cannot be cast to Shape; there is no conversion between them
<no file:4>
      (let shape (as (as circle Pointer) Shape))
                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A value goes into a case of a union it converts to unasked before a nearer case it converts to only when asked.

```brix
(main
  (let small 7u32)
  (let v Mixed small)
  (print (is v F64))
)

(type + Mixed (is Union)
  (variant I32)
  (variant F64)
)
```

```output
true
```

### A U64 goes into a union of Int and Float only when asked, since neither holds every value of it.

```brix compile-error
(main
  (let big 9u64)
  (let v Number big)
  (print v)
)

(type + Number (is Union)
  (variant Int)
  (variant Float)
)
```

```output
Exception: Expected Number here, but this is U64, which becomes Number only when asked; write "(as big Number)"
<no file:4>
      (let v Number big)
             ^^^^^^
```

### A union, or an optional of one, read from an object and passed or returned in registers reads none of the bytes after it, so one that ends where readable memory ends is read without a fault.

```brix
(import /core/intrinsics)
(import /sys/mman.h)
(import /unistd.h)
(blank)

(main
  (print (call wide-argument (call edge 24)))
  (print (call wide-return (call edge 24)))
  (print (call narrow-argument (call edge 24)))
)

(type + Wide (is Union)
  (variant whole Int)
  (variant ratio Float)
)

(type + Narrow (is Union)
  (variant whole I32)
  (variant flag Bool)
)

(type + WideHolder
  (prop + value (? Wide))
)

(type + NarrowHolder
  (prop + first (? Narrow))
  (prop + second (? Narrow))
)

(fn-do + wide-argument Int (without gc)
  (param address Pointer)
  (let holder (call (of gc-object-from-pointer WideHolder) address))
  (= (. holder value) (call (. Wide whole) 42))
  (return (call wide-whole (. holder value) 1))
)

(fn-do + wide-return Int (without gc)
  (param address Pointer)
  (let holder (call (of gc-object-from-pointer WideHolder) address))
  (= (. holder value) (call (. Wide whole) 43))
  (return (call wide-whole (call wide-read holder 1) 0))
)

(fn-do + narrow-argument Int (without gc)
  (param address Pointer)
  (let holder (call (of gc-object-from-pointer NarrowHolder) address))
  (= (. holder first) (call (. Narrow flag) true))
  (= (. holder second) (call (. Narrow whole) 44))
  (return (call narrow-whole (. holder second) 1))
)

(fn-do + wide-whole Int (without gc)
  (param value (? Wide))
  (param depth Int)
  (return-if (call wide-whole value (- depth 1)) (if (> depth 0)))
  (match? (!! value)
    (case (call .whole whole)
      (return whole)
    )
  )
  (return 0)
)

(fn-do + wide-read (? Wide) (without gc)
  (param holder WideHolder)
  (param depth Int)
  (return-if (call wide-read holder (- depth 1)) (if (> depth 0)))
  (return (. holder value))
)

(fn-do + narrow-whole Int (without gc)
  (param value (? Narrow))
  (param depth Int)
  (return-if (call narrow-whole value (- depth 1)) (if (> depth 0)))
  (match? (!! value)
    (case (call .whole whole)
      (return whole)
    )
  )
  (return 0)
)

(fn-do + edge Pointer
  (param size U64)
  (let page (as (call C:getpagesize) U64))
  (let base (call C:mmap null (* page 2) (/or/ C:PROT_READ C:PROT_WRITE) (/or/ C:MAP_PRIVATE C:MAP_ANON) -1 0))
  (call C:mprotect (as (+ (. base address) page) (of CPointer Void)) page C:PROT_NONE)
  (return (as (- (+ (. base address) page) size) Pointer))
)
```

```output
42
43
44
```

```brix
(main
  (let state (call State))
  (print (== (. state previous) null))
)

(type + Shade (is Tuple)
  (prop + amount _ (= 0.0))
)

(type + Color (is Union)
  (variant plain (tuple Int))
  (variant shade (tuple Shade))
)

(type + State
  (prop + color (? Color))
  (prop + previous (? State))
)
```

```output
true
```
