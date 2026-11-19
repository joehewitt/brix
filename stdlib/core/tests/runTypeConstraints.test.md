# Type constraints

```brix
(main
  (print (call (of event Int)))
  (print (call (of event Float)))
)

(type + (of Foo T))

(type + Dispatcher (is Interface)
  (fn-expr + (of get-foo T) (of Foo T))
)

(type + (of EventDispatcher Event)
  (implements Dispatcher)
  (prop + foo _ (= (call (of Foo Event))))
  (fn-expr + (of get-foo Other) (of Foo Other)
    ; ?! Error("Unsupported event")
  )
  (fn-expr + (of get-foo T) _
    (constraint is! T Event)
    foo
  )
)

(fn-do + (of event T) _
  (let dispatcher (as (call (of EventDispatcher T)) Dispatcher))
  (return (call (of (. dispatcher get-foo) T)))
)
```

```output
[Object __source__:Foo'core/number:I64]
[Object __source__:Foo'core/number:F64]
```

```brix
(main
  (print (call (of foo Int)))
  (print (call (of foo Float)))
)

(fn-expr + (of foo T) _
  (constraint is! T I64)
  "int"
)

(fn-expr + (of foo T) _
  "other"
)
```

```output
int
other
```

```brix
(main
  (print (call (of foo Foo)))
  (print (call (of foo Float)))
)

(type + Bar (is Interface))

(type + Foo (is Bar))

(fn-expr + (of foo T) _
  (constraint is T Bar)
  "bar"
)

(fn-expr + (of foo T) _
  "other"
)
```

```output
bar
other
```

## Class type constraints

```brix
(main
  (print (call Foo))
)

(type + (of Foo T)
  (constraint = T Char)
)
```

```output
[Object __source__:Foo'core/number:Char]
```

### Constructor assignments resolve before constructor and class constraints are checked.

```brix
(main
  (let token (call ConstraintArenaToken))
  (let managed (call (of ConstrainedConstructorBox U32)))
  (let native (call (of ConstrainedConstructorBox U32) token))
  (print (type-of managed))
  (print (type-of native))
)

(type + ConstraintArenaToken)

(type + ConstructorBacking (is Interface))

(type + (of ManagedConstraintBacking Value) (is ConstructorBacking))

(type + (of ArenaConstraintBacking Value) (is ConstructorBacking))

(type + (of ConstrainedConstructorBox Value Backing)
  (constraint = Backing (of ManagedConstraintBacking Value))
  (constraint is Backing ConstructorBacking)
  (fn-do + This _)
  (fn-do + This _
    (param token ConstraintArenaToken)
    (constraint = Backing (of ArenaConstraintBacking Value))
    (constraint is! Backing (of ArenaConstraintBacking Value))
  )
)
```

```output
[TypeInfo ConstrainedConstructorBox'U32'ManagedConstraintBacking'U32]
[TypeInfo ConstrainedConstructorBox'U32'ArenaConstraintBacking'U32]
```

### A class constraint rejects the concrete owner type selected by a constructor.

```brix crash
(main
  (call (of InvalidConstrainedConstructorBox U32) (call ConstraintFailureToken))
)

(type + ConstraintFailureToken)

(type + RequiredConstructorBacking (is Interface))

(type + (of ValidConstraintBacking Value) (is RequiredConstructorBacking))

(type + (of InvalidConstraintBacking Value))

(type + (of InvalidConstrainedConstructorBox Value Backing)
  (constraint = Backing (of ValidConstraintBacking Value))
  (constraint is Backing RequiredConstructorBacking)
  (fn-do + This _
    (param token ConstraintFailureToken)
    (constraint = Backing (of InvalidConstraintBacking Value))
  )
)
```

```output
Exception: No constructor of InvalidConstrainedConstructorBox matches these arguments; its declared constructors are "InvalidConstrainedConstructorBox(token ConstraintFailureToken)"
<no file:3>
      (call (of InvalidConstrainedConstructorBox U32) (call ConstraintFailureToken))
                ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (print (call (of Foo Char)))
)

(type + (of Foo T)
  (constraint is! T Char)
)
```

```output
[Object __source__:Foo'core/number:Char]
```

```brix crash
(main
  (print (call (of Foo Byte)))
)

(type + (of Foo T)
  (constraint is! T Char)
)
```

```output
Exception: No constructor of Foo matches these arguments
<no file:3>
      (print (call (of Foo Byte)))
                       ^^^
```

```brix
(main
  (call doobs)
  (call doobs (call (of Foo Char)))
)

(fn-do + doobs _
  (param f (? Foo) (= null))
  (print (type-of f))
)

(type + (of Foo T)
  (constraint = T Char)
)
```

```output
[TypeInfo Optional'Foo'Char]
[TypeInfo Optional'Foo'Char]
```

```brix
(main
  (let foo Foo)
  (print foo)
)

(type + (of Foo T)
  (constraint = T Char)
)
```

```output
[Object __source__:Foo'core/number:Char]
```

### A method whose type parameter must be exactly a parameter of its class is chosen where the class's parameter is that type, as the last legal method of its name.

```brix
(main
  (let d (call (of EventDispatcher Int)))
  (print (call (of (. d get-foo) Int)))
  (print (call (of (. d get-foo) Float)))
)

(type + (of Foo T))

(type + (of EventDispatcher Event)
  (prop + foo _ (= (call (of Foo Event))))
  (fn-expr + (of get-foo Other) (of Foo Other)
    (before
      (print "other")
    )
    (call (of Foo Other))
  )
  (fn-expr + (of get-foo T) _
    (constraint is! T Event)
    foo
  )
)
```

```output
[Object __source__:Foo'core/number:I64]
other
[Object __source__:Foo'core/number:F64]
```

### A generic function of an interface is answered for each of its instances by the method a direct call would choose.

```brix
(main
  (print (call (of event Int)))
  (print (call (of event Float)))
  (let d (as (call (of EventDispatcher Int)) Dispatcher))
  (print (call (of (. d get-foo) Float)))
)

(type + (of Foo T))

(type + Dispatcher (is Interface)
  (fn-expr + (of get-foo T) (of Foo T))
)

(type + (of EventDispatcher Event)
  (implements Dispatcher)
  (prop + foo _ (= (call (of Foo Event))))
  (fn-expr + (of get-foo Other) (of Foo Other)
    ; ?! Error("Unsupported event")
  )
  (fn-expr + (of get-foo T) _
    (constraint is! T Event)
    foo
  )
)

(fn-do + (of event T) _
  (let dispatcher (as (call (of EventDispatcher T)) Dispatcher))
  (return (call (of (. dispatcher get-foo) T)))
)
```

```output
[Object __source__:Foo'core/number:I64]
[Object __source__:Foo'core/number:F64]
[Object __source__:Foo'core/number:F64]
```
