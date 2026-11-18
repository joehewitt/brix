# Type constraints

```yumi
-- event'Int()
-- event'Float()

━━━━━━━━━━━━━━━━━━━━
+ Foo'T

━━━━━━━━━━━━━━━━━━━━
+ Dispatcher is Interface

────────────────────
+ get-foo'T Foo'T

━━━━━━━━━━━━━━━━━━━━
+ EventDispatcher'Event
    is Dispatcher

+ foo = Foo'Event()

────────────────────
+ get-foo'Other Foo'Other

` ?! Error("Unsupported event")

────────────────────
+ get-foo'T
    - T is! Event

foo

━━━━━━━━━━━━━━━━━━━━
+ do event'T

. dispatcher = EventDispatcher'T() as Dispatcher
= dispatcher.get-foo'T()
```

```output
[Object __source__:Foo'core/number:I64]
[Object __source__:Foo'core/number:F64]
```

```yumi
-- foo'Int()
-- foo'Float()

━━━━━━━━━━━━━━━━━━━━
+ foo'T
    - T is! I64

"int"

━━━━━━━━━━━━━━━━━━━━
+ foo'T

"other"
```

```output
int
other
```

```yumi
-- foo'Foo()
-- foo'Float()

━━━━━━━━━━━━━━━━━━━━
+ Bar is Interface

━━━━━━━━━━━━━━━━━━━━
+ Foo is Bar

━━━━━━━━━━━━━━━━━━━━
+ foo'T
    - T is Bar

"bar"

━━━━━━━━━━━━━━━━━━━━
+ foo'T

"other"
```

```output
bar
other
```

## Class type constraints

```yumi
-- Foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo'T
    - T = Char
```

```output
[Object __source__:Foo'core/number:Char]
```

### Constructor assignments resolve before constructor and class constraints are checked.

```yumi
. token = ConstraintArenaToken()
. managed = ConstrainedConstructorBox'U32()
. native = ConstrainedConstructorBox'U32(token)
-- type-of managed
-- type-of native

━━━━━━━━━━━━━━━━━━━━
+ ConstraintArenaToken

━━━━━━━━━━━━━━━━━━━━
+ ConstructorBacking is Interface

━━━━━━━━━━━━━━━━━━━━
+ ManagedConstraintBacking'Value is ConstructorBacking

━━━━━━━━━━━━━━━━━━━━
+ ArenaConstraintBacking'Value is ConstructorBacking

━━━━━━━━━━━━━━━━━━━━
+ ConstrainedConstructorBox'Value'Backing
    - Backing = ManagedConstraintBacking'Value
    - Backing is ConstructorBacking

────────────────────
+ This

────────────────────
+ This
    - token ConstraintArenaToken
    - Backing = ArenaConstraintBacking'Value
    - Backing is! ArenaConstraintBacking'Value
```

```output
[TypeInfo ConstrainedConstructorBox'U32'ManagedConstraintBacking'U32]
[TypeInfo ConstrainedConstructorBox'U32'ArenaConstraintBacking'U32]
```

### A class constraint rejects the concrete owner type selected by a constructor.

```yumi crash
InvalidConstrainedConstructorBox'U32(ConstraintFailureToken())

━━━━━━━━━━━━━━━━━━━━
+ ConstraintFailureToken

━━━━━━━━━━━━━━━━━━━━
+ RequiredConstructorBacking is Interface

━━━━━━━━━━━━━━━━━━━━
+ ValidConstraintBacking'Value is RequiredConstructorBacking

━━━━━━━━━━━━━━━━━━━━
+ InvalidConstraintBacking'Value

━━━━━━━━━━━━━━━━━━━━
+ InvalidConstrainedConstructorBox'Value'Backing
    - Backing = ValidConstraintBacking'Value
    - Backing is RequiredConstructorBacking

────────────────────
+ This
    - token ConstraintFailureToken
    - Backing = InvalidConstraintBacking'Value
```

```output
Exception: No constructor of InvalidConstrainedConstructorBox matches these arguments; its declared constructors are "InvalidConstrainedConstructorBox(token ConstraintFailureToken)"
<no file:2>
    InvalidConstrainedConstructorBox'U32(ConstraintFailureToken())
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```yumi
-- Foo'Char()

━━━━━━━━━━━━━━━━━━━━
+ Foo'T
    - T is! Char
```

```output
[Object __source__:Foo'core/number:Char]
```

```yumi crash
-- Foo'Byte()

━━━━━━━━━━━━━━━━━━━━
+ Foo'T
    - T is! Char
```

```output
Exception: No constructor of Foo matches these arguments
<no file:2>
    -- Foo'Byte()
       ^^^
```

```yumi
doobs()
doobs(Foo'Char())

━━━━━━━━━━━━━━━━━━━━
+ do doobs
    - f Foo? = null

-- type-of f

━━━━━━━━━━━━━━━━━━━━
+ Foo'T
    - T = Char
```

```output
[TypeInfo Optional'Foo'Char]
[TypeInfo Optional'Foo'Char]
```

```yumi
. foo Foo
-- foo

━━━━━━━━━━━━━━━━━━━━
+ Foo'T
    - T = Char
```

```output
[Object __source__:Foo'core/number:Char]
```

### A method whose type parameter must be exactly a parameter of its class is chosen where the class's parameter is that type, as the last legal method of its name.

```yumi
. d = EventDispatcher'Int()
-- d.get-foo'Int()
-- d.get-foo'Float()

━━━━━━━━━━━━━━━━━━━━
+ Foo'T

━━━━━━━━━━━━━━━━━━━━
+ EventDispatcher'Event

+ foo = Foo'Event()

────────────────────
+ get-foo'Other Foo'Other

-- "other"
Foo'Other()

────────────────────
+ get-foo'T
    - T is! Event

foo
```

```output
[Object __source__:Foo'core/number:I64]
other
[Object __source__:Foo'core/number:F64]
```

### A generic function of an interface is answered for each of its instances by the method a direct call would choose.

```yumi
-- event'Int()
-- event'Float()
. d = EventDispatcher'Int() as Dispatcher
-- d.get-foo'Float()

━━━━━━━━━━━━━━━━━━━━
+ Foo'T

━━━━━━━━━━━━━━━━━━━━
+ Dispatcher is Interface

────────────────────
+ get-foo'T Foo'T

━━━━━━━━━━━━━━━━━━━━
+ EventDispatcher'Event
    is Dispatcher

+ foo = Foo'Event()

────────────────────
+ get-foo'Other Foo'Other

` ?! Error("Unsupported event")

────────────────────
+ get-foo'T
    - T is! Event

foo

━━━━━━━━━━━━━━━━━━━━
+ do event'T

. dispatcher = EventDispatcher'T() as Dispatcher
= dispatcher.get-foo'T()
```

```output
[Object __source__:Foo'core/number:I64]
[Object __source__:Foo'core/number:F64]
[Object __source__:Foo'core/number:F64]
```
