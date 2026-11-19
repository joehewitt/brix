# Stored function values

```brix
(main
  (let holder (call NarrowedHookHolder))
  (print (call (. holder invoke) 7))
  (= (. holder hook) accepts-seven)
  (print (call (. holder invoke) 7))
  (print (call (. holder invoke) 9))
  (= (. holder hook) null)
  (print (call (. holder invoke) 7))
)

(type + NarrowedHook (is (fn-type Bool (param value I64))))

(fn-expr + accepts-seven _
  (param value I64)
  (== value 7)
)

(type + NarrowedHookHolder
  (prop + hook (? NarrowedHook))
  (fn-do + invoke Bool
    (param value I64)
    (return-if false (if (not hook)))
    (return (call hook value))
  )
)
```

```output
false
true
false
false
```

### A Bool-returning function stored in another module's hook slot returns its real value when called through the stored function value.

```brix
(import ./lib/hooklib)
(blank)

(main
  (call set-stored-hook my-hook)
  (print (str "stored-call-7=" (call call-stored-hook 7)))
  (print (str "stored-call-9=" (call call-stored-hook 9)))
)

(fn-expr + my-hook Bool
  (param value I64)
  (== value 7)
)
```

```brix file=lib/hooklib.bx
(prop _ stored-hook (? StoredHook) (= null))

(type + StoredHook (is (fn-type Bool (param value I64))))

(fn-do + set-stored-hook _
  (param hook StoredHook)
  (= stored-hook hook)
)

(fn-do + call-stored-hook Bool
  (param value I64)
  (if (not stored-hook)
    (return false)
  )
  (let hook (!! stored-hook))
  (return (call hook value))
)
```

```output
stored-call-7=true
stored-call-9=false
```

### Calls through a function-typed parameter resolve reordered named arguments like direct calls.

```brix
(main
  (print (str "direct=" (call combine 1000 (:c 3) (:b 7))))
  (print (str "indirect=" (call apply combine)))
)

(type + Op (is (fn-type I32 (param a I32) (param b I32) (param c I32))))

(fn-expr + combine I32
  (param a I32)
  (param b I32)
  (param c I32)
  (- a (* b 100) c)
)

(fn-expr + apply I32
  (param f Op)
  (call f 1000 (:c 3) (:b 7))
)
```

```output
direct=297
indirect=297
```

### An unknown argument name is rejected through a function-typed parameter.

```brix crash
(main
  (print (call apply combine))
)

(type + Op (is (fn-type I32 (param a I32) (param b I32))))

(fn-expr + combine I32
  (param a I32)
  (param b I32)
  (- a b)
)

(fn-expr + apply I32
  (param f Op)
  (call f 10 (:by 3))
)
```

```output
Exception: "f" has no "by" argument
<no file:16>
      (call f 10 (:by 3))
                 ^^^^^^^
```

### An unknown argument name is rejected through a function-typed field.

```brix crash
(main
  (let holder (call Holder combine))
  (let op (. holder op))
  (print (call op 10 (:by 3)))
)

(type + Op (is (fn-type I32 (param a I32) (param b I32))))

(fn-expr + combine I32
  (param a I32)
  (param b I32)
  (- a b)
)

(type + Holder
  (prop + op Op)
  (fn-do + This _
    (param op Op)
    (= (. this op) op)
  )
)
```

```output
Exception: "op" has no "by" argument
<no file:5>
      (print (call op 10 (:by 3)))
                         ^^^^^^^
```

### An unknown argument name is rejected while partially applying a function value.

```brix crash
(main
  (print (call partial combine))
)

(type + Op (is (fn-type I32 (param a I32) (param b I32) (param c I32))))

(fn-expr + combine I32
  (param a I32)
  (param b I32)
  (param c I32)
  (- a (* b 100) c)
)

(fn-expr + partial _
  (param f Op)
  (call f 1000 ... (:over 3))
)
```

```output
Exception: "f" has no "over" argument
<no file:17>
      (call f 1000 ... (:over 3))
                       ^^^^^^^^^
```

### Partial application through a function value preserves each named argument's declared slot.

```brix
(main
  (print (call apply-partial combine))
)

(type + Op (is (fn-type I32 (param a I32) (param b I32) (param c I32))))

(fn-expr + combine I32
  (param a I32)
  (param b I32)
  (param c I32)
  (- a (* b 100) c)
)

(fn-do + apply-partial I32
  (param f Op)
  (let partial (call f 1000 ... (:c 3)))
  (return (call partial (:b 7)))
)
```

```output
297
```

### A generic optional String-returning function value still returns through a multi-arm match when a fallback arm is a single-interpolation string literal.

```brix
(main
  (let key-builder (? (of KeyBuilder Int)) build-int-key)
  (print (call key-for-item 7 0 key-builder "p"))
  (print (call key-for-item 8 1 null "p"))
  (print (call key-for-item 9 2 null null))
)

(type + (of KeyBuilder Item) (is (fn-type String (param item Item))))

(fn-expr + build-int-key String
  (param item Int)
  (str "built-" item)
)

(fn-expr + (of key-for-item Item) String
  (param item Item)
  (param index Int)
  (param key-builder (? (of KeyBuilder Item)))
  (param key-prefix (? String))
  (match true
    (case? has-builder
      built-key
    )
    (case? key-prefix
      (str key-prefix "/" index)
    )
    (default
      (str index)
    )
  )
  (where
    (lazy has-builder (!= key-builder null))
    (lazy built-key (if-expr has-builder (call (!! key-builder) item) ""))
  )
)
```

```output
built-7
p/1
2
```

### Same with the markup mount-hook parameter shape: Pointer, optional Pointer, optional interface.

```brix
(import ./lib/ptrhooklib)
(blank)

(main
  (let node (call Probe))
  (call set-ptr-hook my-ptr-hook)
  (print (str "with-root=" (call call-ptr-hook (as node Pointer) null (as node Rootish))))
  (print (str "no-root=" (call call-ptr-hook (as node Pointer) null null)))
)

(fn-do + my-ptr-hook Bool
  (param node-pointer Pointer)
  (param parent-pointer (? Pointer))
  (param root (? Rootish))
  (if root
    (return true)
  )
  (return false)
)

(type + Probe
  (implements Rootish)
)
```

```brix file=lib/ptrhooklib.bx
(prop _ ptr-hook (? PtrHook) (= null))

(type + Rootish (is Interface))

(type + PtrHook (is (fn-type Bool (param node-pointer Pointer) (param parent-pointer (? Pointer)) (param root (? Rootish)))))

(fn-do + set-ptr-hook _
  (param hook PtrHook)
  (= ptr-hook hook)
)

(fn-do + call-ptr-hook Bool
  (param node-pointer Pointer)
  (param parent-pointer (? Pointer))
  (param root (? Rootish))
  (if (not ptr-hook)
    (return false)
  )
  (let hook (!! ptr-hook))
  (return (call hook node-pointer parent-pointer root))
)
```

```output
with-root=true
no-root=false
```

### Same, but the hook is a private function registered at an imported module's init, like /core/style registering style-apply-mount with /core/markup.

```brix
(import ./lib/ptrhooklib)
(import ./lib/registrar)
(blank)

(main
  (let node (call Probe))
  (print (str "with-root=" (call call-ptr-hook (as node Pointer) null (as node Rootish))))
  (print (str "no-root=" (call call-ptr-hook (as node Pointer) null null)))
)

(type + Probe
  (implements Rootish)
)
```

```brix file=lib/registrar.bx
(import ./ptrhooklib)
(blank)

(main
  (call set-ptr-hook registrar-hook)
)

(fn-do _ registrar-hook Bool
  (param node-pointer Pointer)
  (param parent-pointer (? Pointer))
  (param root (? Rootish))
  (if root
    (return true)
  )
  (return false)
)
```

```brix file=lib/ptrhooklib.bx
(prop _ ptr-hook (? PtrHook) (= null))

(type + Rootish (is Interface))

(type + PtrHook (is (fn-type Bool (param node-pointer Pointer) (param parent-pointer (? Pointer)) (param root (? Rootish)))))

(fn-do + set-ptr-hook _
  (param hook PtrHook)
  (= ptr-hook hook)
)

(fn-do + call-ptr-hook Bool
  (param node-pointer Pointer)
  (param parent-pointer (? Pointer))
  (param root (? Rootish))
  (if (not ptr-hook)
    (return false)
  )
  (let hook (!! ptr-hook))
  (return (call hook node-pointer parent-pointer root))
)
```

```output
with-root=true
no-root=false
```

### Same again, but the hook callee can panic and is recursive, like style-apply-mount. The recursion forces the callee's ABI to be fixed before its body finishes compiling, which is the case that once silently dropped Bool returns through stored function values.

```brix
(import ./lib/ptrhooklib)
(blank)

(main
  (let node (call Probe))
  (call set-ptr-hook my-panicky-hook)
  (print (str "with-root=" (call call-ptr-hook (as node Pointer) null (as node Rootish))))
  (print (str "no-root=" (call call-ptr-hook (as node Pointer) null null)))
)

(type + HookPanic (is Panic))

(fn-do + my-panicky-hook Bool
  (param node-pointer Pointer)
  (param parent-pointer (? Pointer))
  (param root (? Rootish))
  (if parent-pointer
    (throw (call HookPanic "unexpected parent"))
  )
  (if root
    (if parent-pointer
      (return (call my-panicky-hook node-pointer null root))
    )
    (return true)
  )
  (return false)
)

(type + Probe
  (implements Rootish)
)
```

```brix file=lib/ptrhooklib.bx
(prop _ ptr-hook (? PtrHook) (= null))

(type + Rootish (is Interface))

(type + PtrHook (is (fn-type Bool (param node-pointer Pointer) (param parent-pointer (? Pointer)) (param root (? Rootish)))))

(fn-do + set-ptr-hook _
  (param hook PtrHook)
  (= ptr-hook hook)
)

(fn-do + call-ptr-hook Bool
  (param node-pointer Pointer)
  (param parent-pointer (? Pointer))
  (param root (? Rootish))
  (if (not ptr-hook)
    (return false)
  )
  (let hook (!! ptr-hook))
  (return (call hook node-pointer parent-pointer root))
)
```

```output
with-root=true
no-root=false
```

### A function that raises is not stored in a hook slot whose function type carries no errors.

```brix crash
(import ./lib/hooklib)
(blank)

(main
  (call set-stored-hook my-hook)
  (print (str "stored-call-7=" (call call-stored-hook 7)))
  (print (str "stored-call-9=" (call call-stored-hook 9)))
)

(fn-expr + my-hook Bool (?! IOError)
  (param value I64)
  (before
    (throw (call IOError "zero") (if (== value 0)))
  )
  (== value 7)
)
```

```brix file=lib/hooklib.bx
(prop _ stored-hook (? StoredHook) (= null))

(type + StoredHook (is (fn-type Bool (param value I64))))

(fn-do + set-stored-hook _
  (param hook StoredHook)
  (= stored-hook hook)
)

(fn-do + call-stored-hook Bool
  (param value I64)
  (if (not stored-hook)
    (return false)
  )
  (let hook (!! stored-hook))
  (return (call hook value))
)
```

```output
Exception: IOError can leave this function, and StoredHook does not carry it; add ?! IOError to StoredHook, or catch it here
<no file:6>
      (call set-stored-hook my-hook)
                            ^^^^^^^
```

### A fallible closure stored in a generic tuple's field returns its value and raises its error through the adapter that casts it to the field's function type.

```brix
(main
  (let holder (call (of Holder Scale) "scaled" (=> (value scale) (call scaled value scale))))
  (let run (. holder run))
  (print (call run 2 (call Scale)))
  (let failed (catch (call run 20 (call Scale))))
  (let message
    (match failed
      (case (: error Error)
        (. error message)
      )
      (default
        "no error"
      )
    )
  )
  (print message)
)

(type + (of Scaler Context) (is (fn-type F64 (?! Error) (param value F64) (param context Context))))

(type + (of Holder Context) (is Tuple)
  (prop + name String)
  (prop + run (of Scaler Context))
)

(type + Scale
  (prop + factor F64 (= 3))
)

(fn-do + scaled F64 (?! Error)
  (param value F64)
  (param scale Scale)
  (throw (call Error "too big") (if (> value 10)))
  (return (* value (. scale factor)))
)
```

```output
6
too big
```

### A closure stored in an optional property of a function type reads the local it captures.

```brix
(main
  (let holder (call Holder))
  (let base 10)
  (= (. holder action) (=> (amount) (+ amount base)))
  (call churn)
  (print (call (. holder run) 5))
  (= base 100)
  (call churn)
  (print (call (. holder run) 5))
)

(type + Step (is (fn-type Int (param amount Int))))

(type + Holder
  (prop + action (? Step))
  (fn-do + run Int
    (param amount Int)
    (let step action)
    (if step
      (return (call step amount))
    )
    (return -1)
  )
)

(fn-do + churn _
  (let kept "")
  (let turn 0)
  (while (< turn 20000)
    (= kept (str "text " turn))
    (+= turn 1)
  )
)
```

```output
15
105
```

### A method read from an object and kept as a function value calls on that object.

```brix
(main
  (let counter (call Counter))
  (let bump Bump (. counter bump))
  (call bump 2)
  (call bump 3)
  (print (. counter total))
  (= (. counter total) 100)
  (call bump 1)
  (print (. counter total))
  (print (call apply bump 5))
  (print (. counter total))
)

(type + Bump (is (fn-type Int (param amount Int))))

(type + Counter
  (prop + total _ (= 0))
  (fn-do + bump Int
    (param amount Int)
    (+= total amount)
    (return total)
  )
)

(fn-do + apply Int
  (param action Bump)
  (param amount Int)
  (return (+ (call action amount) (call action amount)))
)
```

```output
5
101
217
111
```

### A function returns a closure as a function value, each with its own environment.

```brix
(main
  (let first (call make 3))
  (let second (call make 10))
  (print (call first 1))
  (print (call second 1))
  (print (call first 2))
  (print (call twice first 5))
)

(type + Step (is (fn-type Int (param amount Int))))

(fn-do + make Step
  (param base Int)
  (let scale (* base 2))
  (return (=> (amount) (+ amount base scale)))
)

(fn-do + twice Int
  (param step Step)
  (param amount Int)
  (return (call step (call step amount)))
)
```

```output
10
31
11
23
```

### Function values take and return structs, small and large.

```brix
(main
  (let origin (call Point 1 2))
  (let shift Shift (=> (point) (call Point (+ (. point x) (. origin x)) (+ (. point y) (. origin y)))))
  (let moved (call shift (call Point 10 20)))
  (print (. moved x))
  (print (. moved y))
  (let wide Widen (=> (box) (call Box (+ (. box a) 1) (+ (. box b) 2) (+ (. box c) 3) (+ (. box d) 4))))
  (let grown (call wide (call Box 1 2 3 4)))
  (print (+ (. grown a) (. grown b) (. grown c) (. grown d)))
)

(type + Shift (is (fn-type Point (param point Point))))

(type + Widen (is (fn-type Box (param box Box))))

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(type + Box (is Struct)
  (prop + a I64)
  (prop + b I64)
  (prop + c I64)
  (prop + d I64)
)
```

```output
11
22
20
```

### A function type that returns nothing takes a closure that assigns, and one that answers.

```brix
(main
  (let log "")
  (let note Note
    (=> (text) :do
      (block
        (= log (str log "[" text "]"))
      )
    )
  )
  (call note "a")
  (call note "b")
  (call each note)
  (print log)
  (let quiet Note (=> (text) (. text length)))
  (call quiet "x")
  (print "done")
)

(type + Note (is (fn-type Void (param text String))))

(fn-do + each _
  (param note Note)
  (call note "c")
  (call note "d")
)
```

```output
[a][b][c][d]
done
```

### A function and a method of a new object passed where a function type is declared.

```brix
(main
  (print (call run show))
  (print (call run (. (call Maker) make)))
)

(type + Text (is (fn-type String (param amount Int))))

(fn-expr + run String
  (param text Text)
  (call text 4)
)

(fn-expr + show String
  (param amount Int)
  (str "show " amount)
)

(fn-expr + double String
  (param amount Int)
  (param by Int)
  (str "double " (* amount by))
)

(type + Maker
  (prop + prefix _ (= "made"))
  (fn-expr + make String
    (param amount Int)
    (str prefix " " amount)
  )
)
```

```output
show 4
made 4
```

### Two function values are equal when they run one code with one data.

```brix
(main
  (let n 4)
  (let a Step (=> (x) (+ x n)))
  (let b Step (=> (x) (- x n)))
  (let c a)
  (let plain Step (=> (x) x))
  (let flat Step (=> (x) (* x 2)))
  (let bound Step (call add 1 ...))
  (let other Step (call add 2 ...))
  (let free Step same)
  (let again Step same)
  (print (== a c))
  (print (== a b))
  (print (!= a b))
  (print (== plain flat))
  (print (== a plain))
  (print (== bound other))
  (print (== bound bound))
  (print (== free again))
  (print (== free plain))
)

(type + Step (is (fn-type Int (param amount Int))))

(fn-expr + add Int
  (param left Int)
  (param right Int)
  (+ left right)
)

(fn-expr + same Int
  (param amount Int)
  amount
)
```

```output
true
false
true
false
false
false
true
true
false
```

### Function values that capture nothing are equal where they run one code, and those of two environments are not.

```brix
(main
  (let plain Step (=> (x) x))
  (let flat Step (=> (x) (* x 2)))
  (let free Step same)
  (let first (call make))
  (let second (call make))
  (let third (call capture 1))
  (let fourth (call capture 1))
  (print (== plain flat))
  (print (== plain free))
  (print (== first second))
  (print (== first plain))
  (print (== third fourth))
  (print (== third third))
  (print (call inside))
)

(type + Step (is (fn-type Int (param amount Int))))

(fn-expr + same Int
  (param amount Int)
  amount
)

(fn-do + make Step
  (let step Step (=> (x) (+ x 1)))
  (return step)
)

(fn-do + capture Step
  (param base Int)
  (let step Step (=> (x) (+ x base)))
  (return step)
)

(fn-do + inside Bool
  (let one Step (=> (x) (+ x 1)))
  (let two Step (=> (x) (+ x 2)))
  (return (== one two))
)
```

```output
false
false
true
false
false
true
false
```

### Function values made by two runs of a closure that declares nothing are not equal.

```brix
(main
  (let n 2)
  (let mk
    (=> ()
      (block
        (=> () n)
      )
    )
  )
  (let g1 Get (call mk))
  (let g2 Get (call mk))
  (print (== g1 g2))
)

(type + Get (is (fn-type Int)))
```

```output
false
```

### A function value made in a run of a closure is not equal to one made around it.

```brix
(main
  (let n 2)
  (let g1 Get (=> () n))
  (let g2 Get (=> () (+ n 1)))
  (print (== g1 g2))
  (let keep
    (=> () :do
      (block
        (let h Get (=> () n))
        (return (== h g1))
      )
    )
  )
  (print (call keep))
)

(type + Get (is (fn-type Int)))
```

```output
false
false
```

### Function values made in one run of a closure are not equal where they run other code.

```brix
(main
  (let n 1)
  (let a Get (=> () n))
  (let inner
    (=> () :do
      (block
        (let b Get (=> () n))
        (return (== a b))
      )
    )
  )
  (print (call inner))
  (let deep
    (=> () :do
      (block
        (let k 2)
        (let c Get (=> () n))
        (let d Get (=> () k))
        (return (== c d))
      )
    )
  )
  (print (call deep))
)

(type + Get (is (fn-type Int)))
```

```output
false
false
```

### An optional property of a function type given a function, a closure and nothing, and called where it holds one.

```brix
(main
  (let holder (call Holder))
  (print (call (. holder run) 3))
  (= (. holder hook) double)
  (print (call (. holder run) 3))
  (= (. holder hook) (=> (n) (+ n 100)))
  (print (call (. holder run) 3))
  (= (. holder hook) null)
  (print (call (. holder run) 3))
)

(type + Hook (is (fn-type Int (param n Int))))

(fn-expr + double _
  (param n Int)
  (* n 2)
)

(type + Holder
  (prop + hook (? Hook))
  (fn-do + run Int
    (param n Int)
    (if hook
      (return (call hook n))
    )
    (return -1)
  )
)
```

```output
-1
6
103
-1
```

### A closure given as an argument that is an optional of a function type.

```brix
(main
  (print (call show (=> (item) (str "closure-" item))))
  (print (call show null))
)

(type + Key (is (fn-type String (param item Int))))

(fn-do + show String
  (param key (? Key))
  (if key
    (return (call key 7))
  )
  (return "none")
)
```

```output
closure-7
none
```

### A function given as an argument that is an optional of a function type.

```brix
(main
  (print (call show build))
  (print (call show null))
)

(type + Key (is (fn-type String (param item Int))))

(fn-expr + build String
  (param item Int)
  (str "built-" item)
)

(fn-do + show String
  (param key (? Key))
  (if key
    (return (call key 7))
  )
  (return "none")
)
```

```output
built-7
none
```

### A closure declared as an optional of a function type and passed on.

```brix
(main
  (let key (? Key) (=> (item) (str "built-" item)))
  (print (call show key))
  (print (call show null))
)

(type + Key (is (fn-type String (param item Int))))

(fn-do + show String
  (param key (? Key))
  (if key
    (return (call key 7))
  )
  (return "none")
)
```

```output
built-7
none
```

### A value of a function type called with arguments by name: evaluated as written, given in their places.

```brix
(main
  (print (call apply combine))
)

(type + Op (is (fn-type I32 (param a I32) (param b I32) (param c I32))))

(fn-expr + combine I32
  (param a I32)
  (param b I32)
  (param c I32)
  (- a (* b 100) c)
)

(fn-do + apply I32
  (param f Op)
  (let first (call f (call tick "a" 1000) (:c (call tick "c" 3)) (:b (call tick "b" 7))))
  (let second (call f (call tick "a" 2000) (:b (call tick "b" 1)) (:c (call tick "c" 9))))
  (let partial (call f ... (:b (call tick "held b" 2))))
  (let third (call partial (call tick "a" 500) (:c (call tick "c" 5))))
  (return (+ first second third))
)

(fn-do + tick I32
  (param label String)
  (param value I32)
  (print (str "tick " label))
  (return value)
)
```

```output
tick a
tick c
tick b
tick a
tick b
tick c
tick held b
tick a
tick c
2483
```

### A value of a function type bound in steps, the arguments held converted where it is called.

```brix
(main
  (print (call apply scale))
)

(type + Scale (is (fn-type Float (param amount Float) (param factor Float) (param offset I64))))

(fn-expr + scale Float
  (param amount Float)
  (param factor Float)
  (param offset I64)
  (+ (* amount factor) offset)
)

(fn-do + apply Float
  (param f Scale)
  (let half (call f 3 ...))
  (let more (call half 2 ...))
  (return (+ (call more 1) (call half 4 10)))
)
```

```output
29
```

### A value of a function type bound in a loop and called.

```brix
(main
  (print (call total add))
)

(type + Add (is (fn-type Int (param a Int) (param b Int))))

(fn-expr + add _
  (param a Int)
  (param b Int)
  (+ a b)
)

(fn-do + total Int
  (param f Add)
  (let sum 0)
  (let i 0)
  (while (< i 3)
    (let plus (call f i ...))
    (+= sum (call plus 10))
    (+= i 1)
  )
  (return sum)
)
```

```output
33
```

### Function type that raises holds a closure and a function that raise nothing

```brix
(main
  (let none CheckedCallback (=> (v) (+ v 1)))
  (print (str "none=" (catch (call none 1))))
  (let named CheckedCallback plain)
  (print (str "named=" (catch (call named 1))))
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```output
none=2
named=2
```

### Function type that raises a base error holds a function that raises a derived one

```brix
(main
  (let sub BaseCallback risky)
  (let r1 (catch (call sub 1)))
  (print (str "sub1=" r1))
  (let r2 (catch (call sub 2)))
  (print (str "sub2 is CallbackFailure: " (is r2 CallbackFailure) " is Error: " (is r2 Error)))
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```output
sub1=11
sub2 is CallbackFailure: true is Error: true
```

### Function type that raises a base error holds a closure that raises a derived one

```brix
(main
  (let subc BaseCallback
    (=> (v) :do
      (block
        (throw (call CallbackFailure "closure failed") (if (== v 2)))
        (return (+ v 100))
      )
    )
  )
  (let r3 (catch (call subc 2)))
  (print (str "subc2 is CallbackFailure: " (is r3 CallbackFailure)))
  (print (catch (call subc 1)))
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```output
subc2 is CallbackFailure: true
101
```

### Optional catch of a call of a function value that raises

```brix
(main
  (let sub BaseCallback risky)
  (let opt (catch? (call sub 2)))
  (print (str "opt=" opt))
  (let opt1 (catch? (call sub 1)))
  (print (str "opt1=" opt1))
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```output
opt=unknown
opt1=11
```

### Errors of a function value leave the function that calls it

```brix
(main
  (let sub BaseCallback risky)
  (let first (catch (call inferred sub 2)))
  (print (str "inferred=" (is first CallbackFailure)))
  (print (str "inferred-ok=" (catch (call inferred sub 1))))
)

(fn-do + inferred Int
  (param callback BaseCallback)
  (param value Int)
  (return (+ (call callback value) 1000))
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```output
inferred=true
inferred-ok=1011
```

### Function type that raises two errors answers with either

```brix
(main
  (let two TwoCallback other)
  (let a (catch (call two 3)))
  (print (is a OtherFailure))
  (print (is a CallbackFailure))
  (let b (catch (call two 1)))
  (print b)
  (let c TwoCallback risky)
  (let d (catch (call c 2)))
  (print (is d CallbackFailure))
  (print (is d OtherFailure))
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```output
true
false
21
true
false
```

### Function type that raises and returns nothing

```brix
(main
  (let v VoidCallback quiet)
  (let a (catch (call v 2)))
  (print (is a CallbackFailure))
  (let b (catch (call v 1)))
  (print (is b CallbackFailure))
  (print "done")
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```output
true
quiet 1
false
done
```

### Error of a function value that returns nothing leaves its caller unhandled

```brix crash
(main
  (let v VoidCallback quiet)
  (call run v 1)
  (print "between")
  (call run v 2)
  (print "never")
)

(fn-do + run _ (?! CallbackFailure)
  (param callback VoidCallback)
  (param value Int)
  (call callback value)
  (print (str "ran " value))
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```output
quiet 1
ran 1
between
```

### Error of a function value runs the cleanup of its caller

```brix crash
(main
  (let c CheckedCallback risky)
  (print (call run c 1))
  (print (call run c 2))
  (print "never")
)

(fn-do + run Int
  (param callback CheckedCallback)
  (param value Int)
  (finally
    (print (str "run cleanup " value))
  )
  (let got (call callback value))
  (print (str "got " got))
  (return (* got 2))
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```output
got 11
run cleanup 1
22
run cleanup 2
```

### Error of a closure held as a function value runs the cleanup of the closure

```brix crash
(main
  (let c CheckedCallback
    (=> (value) :do
      (block
        (finally
          (print "closure cleanup")
        )
        (throw (call CallbackFailure "closure failed") (if (== value 2)))
        (return (+ value 5))
      )
    )
  )
  (print (call run c 1))
  (print (call run c 2))
  (print "never")
)

(fn-do + run Int
  (param callback CheckedCallback)
  (param value Int)
  (return (+ (call callback value) (call callback (+ value 1))))
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```output
closure cleanup
closure cleanup
```

### Function value that raises held by a property

```brix
(main
  (let holder (call Holder risky))
  (let held (. holder callback))
  (print (catch (call held 1)))
  (let failed (catch (call held 2)))
  (print (is failed CallbackFailure))
  (print (call (. holder run) 1))
  (let second (catch (call (. holder run) 2)))
  (print (is second CallbackFailure))
)

(type + Holder
  (prop + callback CheckedCallback)
  (fn-do + This _
    (param given CheckedCallback)
    (lazy callback given)
  )
  (fn-do + run Int (?! CallbackFailure)
    (param value Int)
    (return (+ (call callback value) 1))
  )
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```output
11
true
12
true
```

### Function values that raise held by a list

```brix
(main
  (let a CheckedCallback risky)
  (let b CheckedCallback plain)
  (let c CheckedCallback (=> (v) (* v 3)))
  (let list (list a b c))
  (for (in item list)
    (print (catch (call item 2)))
  )
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```output
risky failed
3
6
```

### Method bound as a function value that raises

```brix
(main
  (let object (call Counter 5))
  (let c CheckedCallback (. object add))
  (print (catch (call c 1)))
  (let failed (catch (call c 2)))
  (print (is failed CallbackFailure))
  (let p CheckedCallback (. object add-plain))
  (print (catch (call p 2)))
)

(type + Counter
  (prop + base Int)
  (fn-do + This _
    (param start Int)
    (lazy base start)
  )
  (fn-do + add Int (?! CallbackFailure)
    (param value Int)
    (throw (call CallbackFailure "add failed") (if (== value 2)))
    (return (+ base value))
  )
  (fn-do + add-plain Int
    (param value Int)
    (return (+ base value))
  )
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```output
6
true
7
```

### Parameter of a function type bound to what a function that raises answers

```brix
(main
  (print (catch (call get-it five)))
  (let failed (catch (call get-it fails)))
  (print (is failed CallbackFailure))
  (print (catch (call get-it text)))
)

(type + (of Getter Result) (is (fn-type Result)))

(fn-do + (of get-it Result) Result
  (param getter (of Getter Result))
  (return (call getter))
)

(fn-do + five Int
  (return 5)
)

(fn-do + text String
  (return "text")
)

(fn-do + fails Int (?! CallbackFailure)
  (throw (call CallbackFailure "fails"))
  (return 1)
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```output
5
true
text
```

### Closure held as a function value that raises may also panic

```brix crash
(main
  (let c CheckedCallback
    (=> (value) :do
      (block
        (finally
          (print "closure cleanup")
        )
        (throw (call Panic "closure panicked") (if (== value 3)))
        (throw (call CallbackFailure "closure failed") (if (== value 2)))
        (return (+ value 5))
      )
    )
  )
  (print (catch (call c 1)))
  (let failed (catch (call c 2)))
  (print (is failed CallbackFailure))
  (print (call c 3))
  (print "never")
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + DeepFailure (is CallbackFailure))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + deep Int (?! DeepFailure)
  (param value Int)
  (throw (call DeepFailure "deep failed") (if (== value 2)))
  (return (+ value 30))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)
```

```output
closure cleanup
6
closure cleanup
true
closure cleanup
```

### Match on what a function value that raises answers

```brix
(main
  (let c CheckedCallback risky)
  (let first (catch (call c 2)))
  (let message
    (match first
      (case (: error CallbackFailure)
        (str "callback: " (. error message))
      )
      (case (: value Int)
        (str "value " value)
      )
    )
  )
  (print message)
  (let second (catch (call c 1)))
  (let other
    (match second
      (case (: error CallbackFailure)
        (str "callback: " (. error message))
      )
      (case (: value Int)
        (str "value " value)
      )
    )
  )
  (print other)
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + DeepFailure (is CallbackFailure))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + deep Int (?! DeepFailure)
  (param value Int)
  (throw (call DeepFailure "deep failed") (if (== value 2)))
  (return (+ value 30))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)
```

```output
callback: risky failed
value 11
```

### Function value that raises an error derived twice

```brix
(main
  (let c BaseCallback deep)
  (let failed (catch (call c 2)))
  (print (is failed DeepFailure))
  (print (is failed CallbackFailure))
  (print (is failed Error))
  (let d CheckedCallback deep)
  (let again (catch (call d 2)))
  (print (is again DeepFailure))
  (print (is again CallbackFailure))
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + DeepFailure (is CallbackFailure))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + deep Int (?! DeepFailure)
  (param value Int)
  (throw (call DeepFailure "deep failed") (if (== value 2)))
  (return (+ value 30))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)
```

```output
true
true
true
true
true
```

### Optional catch of a function value in a loop

```brix
(main
  (let total 0)
  (let c CheckedCallback risky)
  (for (in value (list 1 2 3))
    (let got (catch? (call c value)))
    (if got
      (+= total got)
      (else
        (print (str "skipped " value))
      )
    )
  )
  (print total)
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + DeepFailure (is CallbackFailure))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + deep Int (?! DeepFailure)
  (param value Int)
  (throw (call DeepFailure "deep failed") (if (== value 2)))
  (return (+ value 30))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)
```

```output
skipped 2
24
```

### Function value that raises called in a loop

```brix
(main
  (print (call sum-all risky))
  (print (call sum-all plain))
)

(fn-do + sum-all Int
  (param callback CheckedCallback)
  (let total 0)
  (for (in value (list 1 3 5))
    (+= total (call callback value))
  )
  (return total)
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + DeepFailure (is CallbackFailure))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + deep Int (?! DeepFailure)
  (param value Int)
  (throw (call DeepFailure "deep failed") (if (== value 2)))
  (return (+ value 30))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)
```

```output
39
12
```

### Function types that raise and return a text or an optional

```brix
(main
  (let c TextCallback describe)
  (print (catch (call c 1)))
  (let failed (catch (call c 2)))
  (print (is failed CallbackFailure))
  (let o OptionalCallback maybe)
  (print (catch (call o 1)))
  (print (catch (call o 3)))
  (let bad (catch (call o 2)))
  (print (is bad CallbackFailure))
)

(type + TextCallback (is (fn-type String (?! CallbackFailure) (param value Int))))

(type + OptionalCallback (is (fn-type (? Int) (?! CallbackFailure) (param value Int))))

(fn-do + describe String (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "describe failed") (if (== value 2)))
  (return (str "described " value))
)

(fn-do + maybe (? Int) (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "maybe failed") (if (== value 2)))
  (if (== value 3)
    (return null)
  )
  (return value)
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + DeepFailure (is CallbackFailure))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + deep Int (?! DeepFailure)
  (param value Int)
  (throw (call DeepFailure "deep failed") (if (== value 2)))
  (return (+ value 30))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)
```

```output
described 1
true
1
unknown
true
```

### Function type that raises converts what its function returns

```brix
(main
  (let c FloatCallback risky)
  (print (catch (call c 1)))
  (let failed (catch (call c 2)))
  (print (is failed CallbackFailure))
  (let d FloatCallback (=> (value) (as (+ value 1) I32)))
  (print (catch (call d 1)))
)

(type + FloatCallback (is (fn-type Float (?! CallbackFailure) (param value Int))))

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + DeepFailure (is CallbackFailure))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(fn-do + risky I32 (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (as (+ value 10) I32))
)

(fn-do + deep Int (?! DeepFailure)
  (param value Int)
  (throw (call DeepFailure "deep failed") (if (== value 2)))
  (return (+ value 30))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)
```

```output
11
true
2
```

### Function type that raises and returns a tuple

```brix crash
(main
  (let c PairCallback make-pair)
  (let got (catch (call c 1)))
  (print (is got CallbackFailure))
  (let failed (catch (call c 2)))
  (print (is failed CallbackFailure))
  (print (call show c 1))
  (print (call show c 2))
)

(type + Pair (is Tuple)
  (prop + left Int)
  (prop + right Int)
  (prop + label String)
)

(type + PairCallback (is (fn-type Pair (?! CallbackFailure) (param value Int))))

(fn-do + make-pair Pair (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "pair failed") (if (== value 2)))
  (return (call Pair value (+ value 1) "pair"))
)

(fn-do + show String (?! CallbackFailure)
  (param callback PairCallback)
  (param value Int)
  (let pair (call callback value))
  (return (str (. pair left) " " (. pair right) " " (. pair label)))
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + DeepFailure (is CallbackFailure))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + deep Int (?! DeepFailure)
  (param value Int)
  (throw (call DeepFailure "deep failed") (if (== value 2)))
  (return (+ value 30))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)
```

```output
false
true
1 2 pair
```

### Function type that raises and returns nothing holds functions that return a value

```brix
(main
  (let c VoidCallback risky)
  (print "start")
  (let first (catch (call c 1)))
  (print (is first CallbackFailure))
  (let second (catch (call c 2)))
  (print (is second CallbackFailure))
  (let d VoidCallback plain)
  (call d 1)
  (print "done")
)

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + DeepFailure (is CallbackFailure))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + deep Int (?! DeepFailure)
  (param value Int)
  (throw (call DeepFailure "deep failed") (if (== value 2)))
  (return (+ value 30))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)
```

```output
start
false
true
done
```

### Parameter of a function type bound to the answer of each function given

```brix
(main
  (print (catch (call invoke deep 1)))
  (let failed (catch (call invoke risky 2)))
  (print (is failed CallbackFailure))
  (print (catch (call invoke plain 2)))
  (print (catch (call invoke describe 2)))
)

(type + (of Mapper2 From To) (is (fn-type To (param value From))))

(fn-do + (of invoke Output) Output
  (param callback (of Mapper2 Int Output))
  (param value Int)
  (return (call callback value))
)

(fn-do + describe String
  (param value Int)
  (return (str "described " value))
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + DeepFailure (is CallbackFailure))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + deep Int (?! DeepFailure)
  (param value Int)
  (throw (call DeepFailure "deep failed") (if (== value 2)))
  (return (+ value 30))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)
```

```output
31
true
3
described 2
```

### Parameter of a function type bound through two generic functions

```brix
(main
  (print (catch (call twice deep 1)))
  (let failed (catch (call twice risky 2)))
  (print (is failed CallbackFailure))
  (print (catch (call twice plain 2)))
)

(type + (of Mapper2 From To) (is (fn-type To (param value From))))

(fn-do + (of twice Output) Output
  (param callback (of Mapper2 Int Output))
  (param value Int)
  (return (call invoke callback value))
)

(fn-do + (of invoke Output) Output
  (param callback (of Mapper2 Int Output))
  (param value Int)
  (return (call callback value))
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + DeepFailure (is CallbackFailure))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + deep Int (?! DeepFailure)
  (param value Int)
  (throw (call DeepFailure "deep failed") (if (== value 2)))
  (return (+ value 30))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)
```

```output
31
true
3
```

### Function values that raise held by a map

```brix
(main
  (let map (call (of Map String CheckedCallback)))
  (= ([] map "risky") risky)
  (= ([] map "plain") plain)
  (let first ([] map "risky"))
  (if first
    (print (catch (call first 1)))
    (let failed (catch (call first 2)))
    (print (is failed CallbackFailure))
  )
  (let second ([] map "plain"))
  (if second
    (print (catch (call second 2)))
  )
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + DeepFailure (is CallbackFailure))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + deep Int (?! DeepFailure)
  (param value Int)
  (throw (call DeepFailure "deep failed") (if (== value 2)))
  (return (+ value 30))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)
```

```output
11
true
3
```

### Error of a function value leaves a method through its cleanup

```brix crash
(main
  (let object (call Worker))
  (print (call (. object run) deep 1))
  (print (call (. object run) risky 2))
  (print "never")
)

(type + Worker
  (prop + runs Int (= 0))
  (fn-do + run Int (?! CallbackFailure)
    (param callback CheckedCallback)
    (param value Int)
    (finally
      (print "run cleanup")
    )
    (+= runs 1)
    (return (+ (call callback value) runs))
  )
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + DeepFailure (is CallbackFailure))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + deep Int (?! DeepFailure)
  (param value Int)
  (throw (call DeepFailure "deep failed") (if (== value 2)))
  (return (+ value 30))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)
```

```output
run cleanup
32
run cleanup
```

### Stack trace of a panic under a function value that raises

```brix crash
(main
  (let c CheckedCallback thrower)
  (print (catch (call c 1)))
  (let failed (catch (call c 2)))
  (print (is failed CallbackFailure))
  (print (call c 3))
  (print "never")
)

(fn-do + thrower Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "thrower failed") (if (== value 2)))
  (throw (call Panic "thrower panicked") (if (== value 3)))
  (return value)
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```stderr
Unhandled panic: thrower panicked
<no file:14>

Task 1

Stack trace:
  0  __source__:thrower no file:14:3
  1  __source__:throwerCasting no file:3:10
  2  __source__:main no file:7:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### Stack trace of an error raised under a bound function value

```brix crash
(main
  (let c CheckedCallback risky)
  (print (call c 1))
  (print (call c 2))
  (print "never")
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```stderr
Unhandled error: risky failed
<no file:23>

Task 1

Stack trace:
  0  __source__:risky no file:23:3
  1  __source__:riskyCasting no file:3:10
  2  __source__:main no file:5:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### Stack trace of an error raised under two function values

```brix crash
(main
  (let inner CheckedCallback risky)
  (let outer CheckedCallback (=> (value) (+ (call inner value) 100)))
  (print (catch (call outer 1)))
  (let failed (catch (call outer 2)))
  (print (is failed CallbackFailure))
  (print (call outer 2))
  (print "never")
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + DeepFailure (is CallbackFailure))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + deep Int (?! DeepFailure)
  (param value Int)
  (throw (call DeepFailure "deep failed") (if (== value 2)))
  (return (+ value 30))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)
```

```stderr
Unhandled error: risky failed
<no file:24>

Task 1

Stack trace:
  0  __source__:risky no file:24:3
  1  __source__:riskyCasting no file:3:14
  2  __source__:outer no file:4:45
  3  __source__:outerCasting no file:4:14
  4  __source__:main no file:8:10
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### Stack trace of an error through a function value given to two functions

```brix crash
(main
  (print "start")
  (print (call run-it false))
  (print (call run-it true))
  (print "not reached")
)

(type + CallbackFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(fn-do + checked-apply Int (?! CallbackFailure)
  (param callback CheckedCallback)
  (param value Int)
  (return (call callback value))
)

(fn-do + run-it Int (?! CallbackFailure)
  (param fails Bool)
  (let failing-callback CheckedCallback
    (=> (value) :do
      (block
        (finally
          (print "callback cleanup")
        )
        (throw (call CallbackFailure "callback failed") (if fails))
        (return (+ value 1))
      )
    )
  )
  (return (call checked-apply failing-callback 4))
)
```

```stderr
Unhandled error: callback failed
<no file:27>

Task 1

Stack trace:
  0  __source__:failing-callback no file:27:9
  1  __source__:failing-callbackCasting no file:21:25
  2  __source__:checked-apply no file:16:11
  3  __source__:run-it no file:32:11
  4  __source__:main no file:5:10
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### Stack trace of an error through a function value made a value of a wider type

```brix crash
(main
  (print "start")
  (let stored CheckedCallback risky)
  (print (call stored 1))
  (let wide WideCallback stored)
  (print (call wide 1))
  (print (call wide 2))
  (print "not reached")
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + WideCallback (is (fn-type Int (?! CallbackFailure) (?! OtherFailure) (param value Int))))

(fn-do + checked-apply Int (?! CallbackFailure)
  (param callback CheckedCallback)
  (param value Int)
  (return (call callback value))
)

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)
```

```stderr
Unhandled error: risky failed
<no file:28>

Task 1

Stack trace:
  0  __source__:risky no file:28:3
  1  __source__:riskyCasting no file:4:15
  2  __source__:Casting no file:6:13
  3  __source__:main no file:8:10
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### Stack trace names the function made for a generic tuple's field given a function

```brix crash
(main
  (let holder (call (of Holder Scale) "scaled" scaled))
  (let run (. holder run))
  (print (call run 2 (call Scale)))
  (print (call run 20 (call Scale)))
)

(type + (of Scaler Context) (is (fn-type F64 (param value F64) (param context Context))))

(type + (of Holder Context) (is Tuple)
  (prop + name String)
  (prop + run (of Scaler Context))
)

(type + Scale
  (prop + factor F64 (= 3))
)

(fn-do + scaled F64
  (param value F64)
  (param scale Scale)
  (throw (call Panic "too big") (if (> value 10)))
  (return (* value (. scale factor)))
)
```

```stderr
Unhandled panic: too big
<no file:23>

Task 1

Stack trace:
  0  __source__:scaled no file:23:3
  1  __source__:scaledCasting no file:3:48
  2  __source__:Casting no file:13:3
  3  __source__:main no file:6:10
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### Stack trace places the function made for two fields of a generic tuple at the first

```brix crash
(main
  (let pair (call (of Pair Scale) (=> (value scale) (call scaled value scale)) (=> (value scale) (call scaled (+ value 1) scale))))
  (let first (. pair first))
  (let second (. pair second))
  (print (call first 2 (call Scale)))
  (print (call second 2 (call Scale)))
  (print (call second 20 (call Scale)))
)

(type + (of Scaler Context) (is (fn-type F64 (param value F64) (param context Context))))

(type + (of Pair Context) (is Tuple)
  (prop + first (of Scaler Context))
  (prop + second (of Scaler Context))
)

(type + Scale
  (prop + factor F64 (= 3))
)

(fn-do + scaled F64
  (param value F64)
  (param scale Scale)
  (throw (call Panic "too big") (if (> value 10)))
  (return (* value (. scale factor)))
)
```

```stderr
Unhandled panic: too big
<no file:25>

Task 1

Stack trace:
  0  __source__:scaled no file:25:3
  1  __source__:pair2 no file:3:98
  2  __source__:pair2Casting no file:3:80
  3  __source__:Casting no file
  4  __source__:main no file:8:10
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### Stack trace names no place for the function made for a function made a value of a function type at two places.

```brix crash
(main
  (let first CheckedCallback thrower)
  (let second CheckedCallback thrower)
  (print (call first 1))
  (print (call second 3))
)

(fn-do + thrower Int (?! CallbackFailure)
  (param value Int)
  (throw (call Panic "thrower panicked") (if (== value 3)))
  (return value)
)

(type + CallbackFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))
```

```stderr
Unhandled panic: thrower panicked
<no file:11>

Task 1

Stack trace:
  0  __source__:thrower no file:11:3
  1  __source__:throwerCasting no file
  2  __source__:main no file:6:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### Stack trace names both functions made for a function value given to a generic tuple

```brix crash
(main
  (let f (of Scaler Scale) (=> (value scale) (call scaled value scale)))
  (let holder (call (of Holder Scale) "x" f))
  (let run (. holder run))
  (print (call run 2 (call Scale)))
  (print (call run 20 (call Scale)))
)

(type + (of Holder Context) (is Tuple)
  (prop + name String)
  (prop + run (of Scaler Context))
)

(type + Scale
  (prop + factor F64 (= 3))
)

(type + (of Scaler Context) (is (fn-type F64 (param value F64) (param context Context))))

(type + Plain (is (fn-type F64 (param value F64) (param context Scale))))

(fn-do + scaled F64
  (param value F64)
  (param scale Scale)
  (throw (call Panic "too big") (if (> value 10)))
  (return (* value (. scale factor)))
)
```

```stderr
Unhandled panic: too big
<no file:26>

Task 1

Stack trace:
  0  __source__:scaled no file:26:3
  1  __source__:f no file:3:46
  2  __source__:fCasting no file:3:10
  3  __source__:Casting no file:4:43
  4  __source__:Casting no file:12:3
  5  __source__:main no file:7:10
  6  __source__:init.__source__ no file
  7  __source__:brix.main no file
```

### Stack trace of an error raised under a closure held as a function value

```brix crash
(main
  (let checked-closure CheckedCallback
    (=> (value) :do
      (block
        (throw (call CallbackFailure "callback failed") (if (== value 2)))
        (return value)
      )
    )
  )
  (print (call run-it checked-closure 1))
  (print (call run-it checked-closure 2))
)

(fn-do + run-it Int (?! CallbackFailure)
  (param callback CheckedCallback)
  (param value Int)
  (return (call checked-apply callback value))
)

(fn-do + checked-apply Int (?! CallbackFailure)
  (param callback CheckedCallback)
  (param value Int)
  (return (call callback value))
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(type + TwoCallback (is (fn-type Int (?! OtherFailure) (?! CallbackFailure) (param value Int))))

(type + VoidCallback (is (fn-type Void (?! CallbackFailure) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)

(fn-do + other Int (?! OtherFailure)
  (param value Int)
  (throw (call OtherFailure "other failed") (if (== value 3)))
  (return (+ value 20))
)

(fn-do + quiet _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "quiet failed") (if (== value 2)))
  (print (str "quiet " value))
)
```

```stderr
Unhandled error: callback failed
<no file:6>

Task 1

Stack trace:
  0  __source__:checked-closure no file:6:9
  1  __source__:checked-closureCasting no file:3:24
  2  __source__:checked-apply no file:24:11
  3  __source__:run-it no file:18:11
  4  __source__:main no file:12:10
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### Stack trace of a panic in a closure held as a function type that raises

```brix crash
(main
  (let panicking-closure CheckedCallback
    (=> (value) :do
      (block
        (throw (call Panic "panicked in closure") (if (== value 2)))
        (return value)
      )
    )
  )
  (print (call panicking-closure 1))
  (print (call panicking-closure 2))
)

(type + CallbackFailure (is Error))

(type + OtherFailure (is Error))

(type + DeepFailure (is CallbackFailure))

(type + CheckedCallback (is (fn-type Int (?! CallbackFailure) (param value Int))))

(type + BaseCallback (is (fn-type Int (?! Error) (param value Int))))

(fn-do + risky Int (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "risky failed") (if (== value 2)))
  (return (+ value 10))
)

(fn-do + deep Int (?! DeepFailure)
  (param value Int)
  (throw (call DeepFailure "deep failed") (if (== value 2)))
  (return (+ value 30))
)

(fn-do + plain Int
  (param value Int)
  (return (+ value 1))
)
```

```stderr
Unhandled panic: panicked in closure
<no file:6>

Task 1

Stack trace:
  0  __source__:panicking-closure no file:6:9
  1  __source__:panicking-closureCasting no file:3:26
  2  __source__:main no file:12:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### Stack trace of an error under a function value held by a generic tuple

```brix crash
(main
  (let scaled-holder (call (of Holder Scale) "scaled" (=> (value scale) (call scaled value scale))))
  (let run (. scaled-holder run))
  (print (call run 2 (call Scale)))
  (print (call run 20 (call Scale)))
  (print "not reached")
)

(type + (of Scaler Context) (is (fn-type F64 (?! Error) (param value F64) (param context Context))))

(type + (of Holder Context) (is Tuple)
  (prop + name String)
  (prop + run (of Scaler Context))
)

(type + Scale
  (prop + factor F64 (= 3))
)

(fn-do + scaled F64 (?! Error)
  (param value F64)
  (param scale Scale)
  (throw (call Error "too big") (if (> value 10)))
  (return (* value (. scale factor)))
)
```

```stderr
Unhandled error: too big
<no file:24>

Task 1

Stack trace:
  0  __source__:scaled no file:24:3
  1  __source__:scaled-holder no file:3:73
  2  __source__:scaled-holderCasting no file:3:55
  3  __source__:Casting no file:14:3
  4  __source__:main no file:6:10
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### Stack trace names the function made for a generic tuple's function field

```brix crash
(main
  (let tuple-holder (call (of Holder Scale) "scaled" (=> (value scale) (call scaled value scale))))
  (let run (. tuple-holder run))
  (print (call run 2 (call Scale)))
  (print (call run 20 (call Scale)))
)

(type + (of Scaler Context) (is (fn-type F64 (param value F64) (param context Context))))

(type + (of Holder Context) (is Tuple)
  (prop + name String)
  (prop + run (of Scaler Context))
)

(type + Scale
  (prop + factor F64 (= 3))
)

(fn-do + scaled F64
  (param value F64)
  (param scale Scale)
  (throw (call Panic "too big") (if (> value 10)))
  (return (* value (. scale factor)))
)
```

```stderr
Unhandled panic: too big
<no file:23>

Task 1

Stack trace:
  0  __source__:scaled no file:23:3
  1  __source__:tuple-holder no file:3:72
  2  __source__:tuple-holderCasting no file:3:54
  3  __source__:Casting no file:13:3
  4  __source__:main no file:6:10
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### Stack trace names the function made for a generic struct's function field

```brix crash
(main
  (let struct-holder (call (of Holder Scale) "x" (=> (value scale) (call scaled value scale))))
  (let run (. struct-holder run))
  (print (call run 2 (call Scale)))
  (print (call run 20 (call Scale)))
)

(type + (of Holder Context) (is Struct)
  (prop + name String)
  (prop + run (of Scaler Context))
)

(type + Scale
  (prop + factor F64 (= 3))
)

(type + (of Scaler Context) (is (fn-type F64 (param value F64) (param context Context))))

(type + Plain (is (fn-type F64 (param value F64) (param context Scale))))

(fn-do + scaled F64
  (param value F64)
  (param scale Scale)
  (throw (call Panic "too big") (if (> value 10)))
  (return (* value (. scale factor)))
)
```

```stderr
Unhandled panic: too big
<no file:25>

Task 1

Stack trace:
  0  __source__:scaled no file:25:3
  1  __source__:struct-holder no file:3:68
  2  __source__:struct-holderCasting no file:3:50
  3  __source__:Casting no file:11:3
  4  __source__:main no file:6:10
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### Stack trace names the function made for a generic tuple's optional function field

```brix crash
(main
  (let optional-holder (call (of Holder Scale) "x" (=> (value scale) (call scaled value scale))))
  (let run (. optional-holder run))
  (if run
    (print (call run 2 (call Scale)))
    (print (call run 20 (call Scale)))
  )
)

(type + (of Holder Context) (is Tuple)
  (prop + name String)
  (prop + run (? (of Scaler Context)))
)

(type + Scale
  (prop + factor F64 (= 3))
)

(type + (of Scaler Context) (is (fn-type F64 (param value F64) (param context Context))))

(type + Plain (is (fn-type F64 (param value F64) (param context Scale))))

(fn-do + scaled F64
  (param value F64)
  (param scale Scale)
  (throw (call Panic "too big") (if (> value 10)))
  (return (* value (. scale factor)))
)
```

```stderr
Unhandled panic: too big
<no file:27>

Task 1

Stack trace:
  0  __source__:scaled no file:27:3
  1  __source__:optional-holder no file:3:70
  2  __source__:optional-holderCasting no file:3:52
  3  __source__:Casting no file:13:3
  4  __source__:main no file:7:12
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### Stack trace names each function made as a function value passes between generic tuples

```brix crash
(main
  (let first-holder (call (of Holder Scale) "x" (=> (value scale) (call scaled value scale))))
  (let second-holder (call (of Holder Scale) "y" (. first-holder run)))
  (let run (. second-holder run))
  (print (call run 2 (call Scale)))
  (print (call run 20 (call Scale)))
)

(type + (of Holder Context) (is Tuple)
  (prop + name String)
  (prop + run (of Scaler Context))
)

(type + Scale
  (prop + factor F64 (= 3))
)

(type + (of Scaler Context) (is (fn-type F64 (param value F64) (param context Context))))

(type + Plain (is (fn-type F64 (param value F64) (param context Scale))))

(fn-do + scaled F64
  (param value F64)
  (param scale Scale)
  (throw (call Panic "too big") (if (> value 10)))
  (return (* value (. scale factor)))
)
```

```stderr
Unhandled panic: too big
<no file:26>

Task 1

Stack trace:
  0  __source__:scaled no file:26:3
  1  __source__:first-holder no file:3:67
  2  __source__:first-holderCasting no file:3:49
  3  __source__:Casting no file:12:3
  4  __source__:Casting no file:4:50
  5  __source__:Casting no file:12:3
  6  __source__:main no file:7:10
  7  __source__:init.__source__ no file
  8  __source__:brix.main no file
```

### A closure that raises is not made a value of a function type that carries no errors.

```brix crash
(main
  (let hook PlainHook
    (=> (value) :do
      (block
        (if (== value 0)
          (throw (call IOError "zero"))
        )
        (return (* value 2))
      )
    )
  )
  (print (call hook 3))
)

(type + PlainHook (is (fn-type I64 (param value I64))))
```

```output
Exception: IOError can leave this function, and PlainHook does not carry it; add ?! IOError to PlainHook, or catch it here
<no file:3>
      (let hook PlainHook
                ^^^^^^^^^
```

### A function that raises an error its function type does not carry is not given as that type.

```brix crash
(main
  (print (call apply parse "4"))
)

(type + Parser (is (fn-type I64 (?! TypeError) (param text String))))

(fn-expr + apply I64 (?! TypeError)
  (param parser Parser)
  (param text String)
  (call parser text)
)

(fn-do + parse I64 (?! IOError)
  (param text String)
  (if (== text "")
    (throw (call IOError "empty"))
  )
  (return 4)
)
```

```output
Exception: IOError can leave this function, and Parser does not carry it; add ?! IOError to Parser, or catch it here
<no file:3>
      (print (call apply parse "4"))
                         ^^^^^
```

### A catch attached to a call of a function value takes the error the value raises: a property, an argument, an item of a list, an optional forced, and a call that answers nothing.

```brix
(main
  (let box (call Box))
  (call (. box run))
  (call run-given fail-given)
  (call run-listed)
  (call run-maybe fail-maybe)
  (call run-maybe null)
  (call notify-each)
)

(type + Risky (is (fn-type I64 (?! IOError) (param value I64))))

(type + Notice (is (fn-type Void (?! IOError))))

(type + Box
  (prop + action Risky (:= fail-property))
  (fn-do + run _
    (let value
      (call action 1
        (block
          (on-error IOError
            -1
          )
        )
      )
    )
    (print (str "property " value))
  )
)

(fn-do + run-given _
  (param action Risky)
  (let value
    (call action 2
      (block
        (on-error IOError
          -2
        )
      )
    )
  )
  (print (str "argument " value))
)

(fn-do + run-listed _
  (let actions (list (as fail-listed Risky)))
  (let value
    (call ([] actions 0) 3
      (block
        (on-error IOError
          -3
        )
      )
    )
  )
  (print (str "listed " value))
)

(fn-do + run-maybe _
  (param action (? Risky))
  (if (not action)
    (print "nothing")
    (return)
  )
  (let value
    (call action 4
      (block
        (on-error IOError
          -4
        )
      )
    )
  )
  (print (str "optional " value))
)

(fn-do + notify-each _
  (let notices (list (as notice Notice)))
  (call ([] notices 0)
    (block
      (on-error (: error IOError)
        (call heard error)
      )
    )
  )
)

(fn-do + heard _
  (param error IOError)
  (print (str "heard " (. error message)))
)

(fn-do + fail-property I64 (?! IOError)
  (param value I64)
  (throw (call IOError "property"))
)

(fn-do + fail-given I64 (?! IOError)
  (param value I64)
  (throw (call IOError "argument"))
)

(fn-do + fail-listed I64 (?! IOError)
  (param value I64)
  (throw (call IOError "listed"))
)

(fn-do + fail-maybe I64 (?! IOError)
  (param value I64)
  (throw (call IOError "optional"))
)

(fn-do + notice _ (?! IOError)
  (throw (call IOError "notice"))
)
```

```output
property -1
argument -2
listed -3
optional -4
nothing
heard notice
```

### A function-typed field is called directly through its owner, here holding a bound method.

```brix
(main
  (let notes (call Notes))
  (let route (call Route (. notes label)))
  (print (call (. route fetch) "a"))
  (= (. notes prefix) "changed")
  (print (call (. route fetch) "b"))
)

(type + Fetcher (is (fn-type String (param path String))))

(type + Notes
  (prop + prefix _ (= "notes"))
  (fn-expr + label _
    (param path String)
    (str prefix ":" path)
  )
)

(type + Route
  (prop + fetch Fetcher)
  (fn-do + This _
    (param fetch Fetcher)
  )
)
```

```output
notes:a
changed:b
```

### A lambda chosen by a condition where an optional function is expected is that function.

```brix
(main
  (print (call run true 4))
  (print (call run false 4))
)

(type + (of Predicate Item) (is (fn-type Bool (param item Item))))

(fn-do + run String
  (param on Bool)
  (param limit U64)
  (let check (? (of Predicate U64)) (only-if (=> (node) (< node limit)) (if on)))
  (let plain (? (of Predicate U64)) (only-if (=> (node) (< node limit)) (if on)))
  (return-if "none" (if (or (not check) (not plain))))
  (return (str (call check 3) " " (call check 9) " " (call plain 9)))
)
```

```output
true false false
none
```

### Two function values are equal when they run one code with one captured data, so removing one subscriber leaves another that captures nothing.

```brix
(main
  (let a Hook (=> () 1))
  (let b Hook (=> () 2))
  (let h Hook three)
  (let i Hook four)
  (let j Hook three)
  (let e (call make 1))
  (let f (call make 1))
  (print (str (== a b) " " (== h i) " " (== h j) " " (!= h i) " " (== e e) " " (== e f) " " (+ (call a) (call b) (call e) (call f))))
  (let source (call Source))
  (let endpoint (of Broadcastable Int) (. source changed))
  (call (. endpoint subscribe) log-first)
  (call (. endpoint subscribe) log-second)
  (catch (call (. endpoint unsubscribe) log-second))
  (call (. source emit) 1)
)

(type + Hook (is (fn-type Int)))

(fn-expr + make _
  (param x Int)
  (=> () x)
)

(fn-expr + three Int
  3
)

(fn-expr + four Int
  4
)

(fn-do + log-first _
  (param item Int)
  (param broadcaster (of Broadcastable Int))
  (print (str "first " item))
)

(fn-do + log-second _
  (param item Int)
  (param broadcaster (of Broadcastable Int))
  (print (str "second " item))
)

(type + Source
  (event + changed Int)
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)
```

```output
false false true true true false 5
first 1
```

### A raising function stored in a field, called through the field with an attached handler.

```brix
(main
  (let holder (call Holder fail))
  (call (. holder finish)
    (block
      (on-error (: error Error)
        (print (str "caught " (. error message)))
      )
    )
  )
  (print "done")
)

(type + Holder
  (prop + finish (fn-type Void (?! Error)))
  (fn-do + This _
    (param finish _)
  )
)

(fn-do + fail _ (?! Error)
  (throw (call Error "bad"))
)
```

```output
caught bad
done
```
