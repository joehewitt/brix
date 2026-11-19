# Static methods

### Static type method calls lower directly without a value receiver

```brix
(main
  (print (call (. NumberFactory make) 7u32))
)

(type + NumberFactory
  (fn-expr + make U32 :static
    (param value U32)
    (+ value 1u32)
  )
)
```

```output
8
```

### A constrained generic static requirement specializes to the conformer's implementation

```brix
(main
  (let direct (call (. BoxFactory make) 5u32))
  (let box (call (of build BoxFactory U32) 7u32))
  (print (type-of direct))
  (print (. direct value))
  (print (. box value))
)

(type + Factory (is Interface)
  (prop + _ (of Product Value))
  (fn-expr + (of make Value) (. This (of Product Value)) :static
    (param value Value)
  )
)

(type + (of Box Value) (is Tuple)
  (prop + value Value)
)

(type + BoxFactory
  (implements Factory)
  (prop + _ (of Product Value) (= (of Box Value)))
  (fn-expr + (of make Value) (of Box Value) :static
    (param value Value)
    (call (of Box Value) value)
  )
)

(fn-expr + (of build Builder Value) (. Builder (of Product Value))
  (param value Value)
  (constraint is Builder Factory)
  (call (of (. Builder make) Value) value)
)
```

```output
[TypeInfo Box'U32]
5
7
```

### Bodyless static do requirements and implementations need no runtime receiver

```brix
(main
  (call (of release ReleaseThing))
  (print "released")
)

(type + Releasable (is Interface)
  (fn-do + release _ :static)
)

(type + ReleaseThing
  (implements Releasable)
  (fn-do + release _ :static)
)

(fn-do + (of release Owner) _
  (constraint is Owner Releasable)
  (call (. Owner release))
)
```

```output
released
```

### Static interface conformance ignores do because it is not part of a method signature

```brix
(main
  (print (call (of build-static-value PureStaticValue)))
  (print (call (of build-static-value ImperativeStaticValue)))
)

(type + StaticValue (is Interface)
  (fn-do + make U32 :static)
)

(type + PureStaticValue
  (implements StaticValue)
  (fn-expr + make U32 :static
    41u32
  )
)

(type + PureStaticValueRequirement (is Interface)
  (fn-expr + make U32 :static)
)

(type + ImperativeStaticValue
  (implements PureStaticValueRequirement)
  (fn-do + make U32 :static
    (return 42u32)
  )
)

(fn-do + (of build-static-value Factory) U32
  (constraint is Factory StaticValue)
  (return (call (. Factory make)))
)

(fn-do + (of build-static-value Factory) U32
  (constraint is Factory PureStaticValueRequirement)
  (return (call (. Factory make)))
)
```

```output
41
42
```

### A missing static requirement fails when normal dispatch reaches it

```brix crash
(main
  (call (of build-required MissingFactory))
)

(type + RequiredFactory (is Interface)
  (fn-expr + create U32 :static)
)

(type + MissingFactory
  (implements RequiredFactory)
)

(fn-do + (of build-required Factory) U32
  (constraint is Factory RequiredFactory)
  (return (call (. Factory create)))
)
```

```output
Exception: Static function not found
<no file:16>
      (return (call (. Factory create)))
              ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An unconstrained generic receiver cannot discover a specialization's static members

```brix crash
(main
  (call (of invoke Concrete) (call Concrete))
)

(type + Concrete
  (fn-do + run _ :static)
)

(fn-do + (of invoke Owner) _
  (param value Owner)
  (call (. Owner run))
)
```

```output
Exception: Static method run is not declared by constraints on Owner
<no file:12>
      (call (. Owner run))
      ^^^^^^^^^^^^^^^^^^^^
```

### Static overloads with equal argument rank are ambiguous

```brix crash
(main
  (call (. Overloaded pick) 1u32)
)

(type + Overloaded
  (fn-do + pick _ :static
    (param value U32)
  )
  (fn-do + pick _ :static
    (param other U32)
  )
)
```

```output
Exception: Ambiguous static method
<no file:3>
      (call (. Overloaded pick) 1u32)
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A static method has no implicit instance receiver

```brix crash
(main
  (print (call (. StaticHolder read)))
)

(type + StaticHolder
  (prop + value _ (= 42))
  (fn-expr + read I32 :static
    value
  )
)
```

```output
Exception: Required self not defined
<no file:9>
        value
        ^^^^^
```

### Static requirements cannot be invoked on the bare interface type

```brix crash
(main
  (call (. RequiredFactory create))
)

(type + RequiredFactory (is Interface)
  (fn-expr + create U32 :static)
)
```

```output
Exception: Static methods cannot be called on an interface type
<no file:3>
      (call (. RequiredFactory create))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Generic owner specializations have distinct static function identities

```brix
(main
  (print (type-of (call (. (of TypedFactory U32) make) 1u32)))
  (print (type-of (call (. (of TypedFactory String) make) "hello")))
)

(type + (of TypedFactory Value)
  (fn-expr + make Value :static
    (param value Value)
    value
  )
)
```

```output
[TypeInfo U32]
[TypeInfo String]
```

### Static methods preserve declared error results

```brix
(main
  (print (?? (catch? (call (. Risky read) true)) 99u32))
  (print (?? (catch? (call (. Risky read) false)) 99u32))
)

(type + Risky
  (fn-do + read U32 :static (?! Error)
    (param fail Bool)
    (if fail
      (throw (call Error "failed"))
    )
    (return 7u32)
  )
)
```

```output
99
7
```

### Static and instance methods occupy separate namespaces

```brix
(main
  (let object (call Both))
  (call (. object label))
  (call (. Both label))
)

(type + Both
  (fn-do + label _
    (print "instance")
  )
  (fn-do + label _ :static
    (print "static")
  )
)
```

```output
instance
static
```

### Static dispatch accepts an implementation with broader generic constraints

```brix
(main
  (print (. (call (of constrained-build GeneralConstrainedFactory Marked) (call Marked 7u32)) value))
)

(type + Marker (is Interface))

(type + Marked (is Tuple)
  (implements Marker)
  (prop + value U32)
)

(type + ConstrainedFactory (is Interface)
  (fn-expr + (of make Value) Value :static
    (param value Value)
    (constraint is Value Marker)
  )
)

(type + GeneralConstrainedFactory
  (implements ConstrainedFactory)
  (fn-expr + (of make Value) Value :static
    (param value Value)
    value
  )
)

(fn-do + (of constrained-build Factory Value) Value
  (param value Value)
  (constraint is Factory ConstrainedFactory)
  (constraint is Value Marker)
  (return (call (of (. Factory make) Value) value))
)
```

```output
7
```

### This names the receiving type inside a static method

```brix
(main
  (print (type-of (call (. SelfFactory make))))
)

(type + SelfFactory
  (fn-expr + make This :static
    (call This)
  )
)
```

```output
[TypeInfo SelfFactory]
```

### Static requirements are inherited through interface constraints

```brix
(main
  (print (call (of inherited-build InheritedFactory) 9u32))
)

(type + ParentFactory (is Interface)
  (fn-expr + make U32 :static
    (param value U32)
  )
)

(type + ChildFactory (is Interface)
  (implements ParentFactory)
)

(type + InheritedFactory
  (implements ChildFactory)
  (fn-expr + make U32 :static
    (param value U32)
    (+ value 1u32)
  )
)

(fn-expr + (of inherited-build Builder) U32
  (param value U32)
  (constraint is Builder ChildFactory)
  (call (. Builder make) value)
)
```

```output
10
```

### A call of a static is followed by a member.

```brix
(main
  (print (. (call (. Thing zero)) total))
  (print (. (call (. (call (. Thing make) 5) twice)) total))
  (print (. (call (. Thing make) 5) label))
  (print (call (. (call (. Thing make) 6) twice)))
  (print (+ (. (call (. Thing zero)) total) 1))
  (print (. (call (. Pair of) 4) a))
)

(type + Thing
  (prop + total _ (= 0))
  (fn-expr + zero Thing :static
    (call Thing)
  )
  (fn-do + make Thing :static
    (param n Int)
    (let t (call Thing))
    (= (. t total) n)
    (return t)
  )
  (fn-do + twice Thing
    (let t (call Thing))
    (= (. t total) (* total 2))
    (return t)
  )
  (fn-expr + label _ :get
    (str "label " total)
  )
  (fn-to-expr + String
    (str "Thing(" total ")")
  )
)

(type + Pair (is Tuple)
  (prop + a Int)
  (prop + b Int)
  (fn-expr + of Pair :static
    (param n Int)
    (call Pair n (+ n 1))
  )
)
```

```output
0
10
label 5
Thing(12)
1
4
```

### A converter calls a static method that a constraint on its type's parameter declares.

```brix
(main
  (let box (call (of Box Doubling) 5))
  (print (as box String))
)

(type + Scaling (is Interface)
  (fn-expr + scale Int :static
    (param value Int)
  )
)

(type + Doubling
  (implements Scaling)
  (fn-expr + scale Int :static
    (param value Int)
    (* value 2)
  )
)

(type + (of Box Scaler)
  (constraint is Scaler Scaling)
  (prop + value Int)
  (fn-do + This _
    (param value _)
  )
  (fn-to-expr + String
    (str "scaled " (call (. Scaler scale) value))
  )
)
```

```output
scaled 10
```
