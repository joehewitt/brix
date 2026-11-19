# Void values

### Void is an ordinary logical value with a zero-sized physical representation.

```brix
(main
  (let empty Void (call Void))
  (print (. (type-of empty) name))
  (print (. Void size))
  (print (== (call Void) (call Void)))
)
```

```output
Void
0
true
```

### A union keeps its tag while omitting an all-Void payload.

```brix
(main
  (let tagged ZeroUnion (call EmptyPayload (call Void)))
  (print (. tagged case))
  (print (. (type-of (. (as tagged EmptyPayload) empty)) name))
)

(type + EmptyPayload (is Tuple)
  (prop + empty Void)
)

(type + ZeroUnion (is Union)
  (variant I32)
  (variant EmptyPayload)
)
```

```output
1
Void
```

### Void fields retain their logical positions without contributing to aggregate layout.

```brix
(main
  (let mixed (call Mixed 11 (call Void) 22))
  (let only (call OnlyVoid (call Void) (call Void)))
  (let tuple (call Tuple 33 (call Void) 44))
  (print (. Mixed size))
  (print (. OnlyVoid size))
  (print (. (type-of (. mixed empty)) name))
  (print (. mixed before))
  (print (. mixed after))
  (print (. (type-of (. only left)) name))
  (print (. tuple length))
  (print (. (type-of ([] tuple 1)) name))
  (print ([] tuple 0))
  (print ([] tuple 2))
)

(type + Mixed (is Struct)
  (prop + before I32)
  (prop + empty Void)
  (prop + after I32)
)

(type + OnlyVoid (is Struct)
  (prop + left Void)
  (prop + right Void)
)
```

```output
8
0
Void
11
22
Void
3
Void
33
44
```

### Void parameters are omitted physically while every argument expression keeps source order.

```brix
(main
  (call consume (call first) 41 (call last))
)

(fn-do + first Void
  (print "first")
  (return (call Void))
)

(fn-do + last Void
  (print "last")
  (return (call Void))
)

(fn-do + consume _
  (param leading Void)
  (param number I32)
  (param trailing Void)
  (print (str "consume " number " " (. (type-of leading) name) " " (. (type-of trailing) name)))
)
```

```output
first
last
consume 41 Void Void
```

### An all-Void aggregate is also passed and returned without an ABI payload.

```brix
(main
  (let empty (call make-empty))
  (call consume-empty empty 57)
)

(type + EmptyPair (is Struct)
  (prop + left Void)
  (prop + right Void)
)

(fn-do + make-empty EmptyPair
  (print "make-empty")
  (return (call EmptyPair (call Void) (call Void)))
)

(fn-do + consume-empty _
  (param empty EmptyPair)
  (param number I32)
  (print (str "empty " number " " (. EmptyPair size) " " (. (type-of (. empty left)) name)))
)
```

```output
make-empty
empty 57 0 Void
```

### Void returns and assignments preserve effects while storing no payload.

```brix
(main
  (let value Void (call make "initial"))
  (= value (call make "assigned"))
  (let box (call EffectBox (call Void) 19))
  (= (. box empty) (call make "field"))
  (call accept value)
  (print (. box value))
)

(type + EffectBox (is Struct)
  (prop + empty Void)
  (prop + value I32)
)

(fn-do + make Void
  (param label String)
  (print label)
  (return (call Void))
)

(fn-do + accept _
  (param value Void)
  (print (str "accepted " (. (type-of value) name)))
)
```

```output
initial
assigned
field
accepted Void
19
```

### Closure calls retain Void in their logical signature and omit its physical argument.

```brix
(main
  (let closure
    (=> ((: empty Void) (: number I32)) :do
      (block
        (print (str "closure " number " " (. (type-of empty) name)))
      )
    )
  )
  (let callback (as closure VoidConsumer))
  (blank)
  (call callback (call Void) 73)
)

(type + VoidConsumer (is (fn-type Void (param empty Void) (param number I32))))
```

```output
closure 73 Void
```

### A constrained associated type may specialize to Void and remain callable as a value.

```brix
(main
  (let context (call associated-void (call VoidAssociatedBacking)))
  (print (. (type-of context) name))
)

(type + AssociatedVoidProvider (is Interface)
  (prop + _ Context)
)

(type + VoidAssociatedBacking
  (implements AssociatedVoidProvider)
  (prop + _ Context (= Void))
)

(fn-do + (of associated-void Backing) (. Backing Context)
  (param backing Backing)
  (constraint is Backing AssociatedVoidProvider)
  (return (call (. Backing Context)))
)
```

```output
Void
```

### Nullable Void retains its discriminant.

```brix
(main
  (let some (call (of Optional Void) (call Void)))
  (let none (call (of Optional Void)))
  (print (. (of Optional Void) size))
  (print (as some Bool))
  (print (as none Bool))
  (print (. (type-of (as some Void)) name))
)
```

```output
1
true
false
Void
```

### Generic specialization may erase a Void field independently of neighboring values.

```brix
(main
  (let box (call (of ContextBox Void) (call Void) 91))
  (print (. (of ContextBox Void) size))
  (print (. (type-of (. box context)) name))
  (print (. box value))
)

(type + (of ContextBox Context) (is Struct)
  (prop + context Context)
  (prop + value I32)
)
```

```output
4
Void
91
```

### A pointer cannot target a zero-sized Void value.

```brix crash
(main
  (let pointer (call (of CPointer Void)))
  (print ([] pointer 0))
)
```

```output
Exception: Void has no physical address
<no file:4>
      (print ([] pointer 0))
             ^^^^^^^^^^^^^^
```

### A local Void value has no address to expose.

```brix crash
(main
  (let empty Void)
  (print &empty)
)
```

```output
Exception: Void has no physical address
<no file:4>
      (print &empty)
             ^^^^^^
```

### An argument that is nothing is not passed, wherever it stands among the others.

```brix
(main
  (call consume (call first) 41 (call last))
  (print (call pass (call first) 1 2 3 4 5 6 7 8 9))
  (let v (call first))
  (call consume v 5 v)
)

(fn-do + first Void
  (print "first")
  (return (call Void))
)

(fn-do + last Void
  (print "last")
  (return (call Void))
)

(fn-do + consume _
  (param leading Void)
  (param number I32)
  (param trailing Void)
  (print (str "consume " number))
)

(fn-expr + pass Int
  (param nothing Void)
  (param a Int)
  (param b Int)
  (param c Int)
  (param d Int)
  (param e Int)
  (param f Int)
  (param g Int)
  (param h Int)
  (param i Int)
  (+ a (* b 2) c d e f g (* h 3) (* i 5))
)
```

```output
first
last
consume 41
first
99
first
consume 5
```

### A value that is nothing prints as the name of its type.

```brix
(main
  (print (call foo))
  (let a (call foo))
  (print a)
  (print (str "[" (call foo) "]"))
)

(fn-do + foo _
  (print "hi")
)
```

```output
hi
core:Void
hi
core:Void
hi
[core:Void]
```

### A function that returns text does not return nothing as empty text.

```brix crash
(main
  (print (str "[" (call via) "]"))
)

(fn-do + foo _
  (print "hi")
)

(fn-do + via String
  (return (call foo))
)
```

```output
Exception: Expected String here, but this is Void, which becomes text only inside a text or a print, or when asked; write "(as (call foo) String)", or put it in a "(str …)"
<no file:11>
      (return (call foo))
              ^^^^^^^^^^
```

### A property that holds nothing is read as nothing, and is passed to a function and kept in a local.

```brix
(main
  (let h (call Holder 3))
  (print (call (. h take)))
  (call (. h give))
  (print (call (. h take)))
  (let g (call (of Generic Int)))
  (print (. g size))
)

(type + Holder
  (prop + count Int)
  (prop _ nothing Void)
  (fn-do + This _
    (param count Int)
    (lazy nothing (call Void))
  )
  (fn-expr + take Int
    (call sum count nothing)
  )
  (fn-do + give _
    (let kept nothing)
    (= count (call sum count kept))
  )
)

(fn-expr + sum Int
  (param count Int)
  (param nothing Void)
  (+ count 1)
)

(type + (of Generic Item Backing)
  (constraint = Backing GCBacking)
  (constraint is Backing StorageBacking)
  (prop + size Size (= 0))
  (prop _ items (of Array Item Backing))
  (prop _ context (. Backing Context))
  (fn-do + This _
    (constraint = Backing GCBacking)
    (lazy context (call Void))
    (lazy items (call (of Array Item Backing) context 0))
  )
)
```

```output
4
5
0
```
