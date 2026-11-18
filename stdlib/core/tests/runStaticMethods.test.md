# Static methods

### Static type method calls lower directly without a value receiver

```yumi
-- NumberFactory.make(7u32)

━━━━━━━━━━━━━━━━━━━━
+ NumberFactory

────────────────────
+ static make U32
    - value U32

value + 1u32
```

```output
8
```

### A constrained generic static requirement specializes to the conformer's implementation

```yumi
. direct = BoxFactory.make(5u32)
. box = build'BoxFactory'U32(7u32)
-- type-of direct
-- direct.value
-- box.value

━━━━━━━━━━━━━━━━━━━━
+ Factory is Interface

+ Product'Value

────────────────────
+ static make'Value This.Product'Value
    - value Value

━━━━━━━━━━━━━━━━━━━━
+ Box'Value is Tuple

+ value Value

━━━━━━━━━━━━━━━━━━━━
+ BoxFactory
    is Factory

+ Product'Value = Box'Value

────────────────────
+ static make'Value Box'Value
    - value Value

Box'Value(value)

━━━━━━━━━━━━━━━━━━━━
+ build'Builder'Value Builder.Product'Value
    - value Value
    - Builder is Factory

Builder.make'Value(value)
```

```output
[TypeInfo Box'U32]
5
7
```

### Bodyless static do requirements and implementations need no runtime receiver

```yumi
release'ReleaseThing()
-- "released"

━━━━━━━━━━━━━━━━━━━━
+ Releasable is Interface

────────────────────
+ static do release

━━━━━━━━━━━━━━━━━━━━
+ ReleaseThing
    is Releasable

────────────────────
+ static do release

━━━━━━━━━━━━━━━━━━━━
+ do release'Owner
    - Owner is Releasable

Owner.release()
```

```output
released
```

### Static interface conformance ignores do because it is not part of a method signature

```yumi
-- build-static-value'PureStaticValue()
-- build-static-value'ImperativeStaticValue()

━━━━━━━━━━━━━━━━━━━━
+ StaticValue is Interface

────────────────────
+ static do make U32

━━━━━━━━━━━━━━━━━━━━
+ PureStaticValue
    is StaticValue

────────────────────
+ static make U32

41u32

━━━━━━━━━━━━━━━━━━━━
+ PureStaticValueRequirement is Interface

────────────────────
+ static make U32

━━━━━━━━━━━━━━━━━━━━
+ ImperativeStaticValue
    is PureStaticValueRequirement

────────────────────
+ static do make U32

= 42u32

━━━━━━━━━━━━━━━━━━━━
+ do build-static-value'Factory U32
    - Factory is StaticValue

= Factory.make()

━━━━━━━━━━━━━━━━━━━━
+ do build-static-value'Factory U32
    - Factory is PureStaticValueRequirement

= Factory.make()
```

```output
41
42
```

### A missing static requirement fails when normal dispatch reaches it

```yumi crash
build-required'MissingFactory()

━━━━━━━━━━━━━━━━━━━━
+ RequiredFactory is Interface

────────────────────
+ static create U32

━━━━━━━━━━━━━━━━━━━━
+ MissingFactory
    is RequiredFactory

━━━━━━━━━━━━━━━━━━━━
+ do build-required'Factory U32
    - Factory is RequiredFactory

= Factory.create()
```

```output
Exception: Static function not found
<no file:18>
    = Factory.create()
      ^^^^^^^^^^^^^^^^
```

### An unconstrained generic receiver cannot discover a specialization's static members

```yumi crash
invoke'Concrete(Concrete())

━━━━━━━━━━━━━━━━━━━━
+ Concrete

────────────────────
+ static do run

━━━━━━━━━━━━━━━━━━━━
+ do invoke'Owner
    - value Owner

Owner.run()
```

```output
Exception: Static method run is not declared by constraints on Owner
<no file:14>
    Owner.run()
    ^^^^^^^^^^^
```

### Static overloads with equal argument rank are ambiguous

```yumi crash
Overloaded.pick(1u32)

━━━━━━━━━━━━━━━━━━━━
+ Overloaded

────────────────────
+ static do pick
    - value U32

────────────────────
+ static do pick
    - other U32
```

```output
Exception: Ambiguous static method
<no file:2>
    Overloaded.pick(1u32)
    ^^^^^^^^^^^^^^^^^^^^^
```

### A static method has no implicit instance receiver

```yumi crash
-- StaticHolder.read()

━━━━━━━━━━━━━━━━━━━━
+ StaticHolder

+ value = 42

────────────────────
+ static read I32

value
```

```output
Exception: Required self not defined
<no file:12>
    value
    ^^^^^
```

### Static requirements cannot be invoked on the bare interface type

```yumi crash
RequiredFactory.create()

━━━━━━━━━━━━━━━━━━━━
+ RequiredFactory is Interface

────────────────────
+ static create U32
```

```output
Exception: Static methods cannot be called on an interface type
<no file:2>
    RequiredFactory.create()
    ^^^^^^^^^^^^^^^^^^^^^^^^
```

### Generic owner specializations have distinct static function identities

```yumi
-- type-of TypedFactory'U32.make(1u32)
-- type-of TypedFactory'String.make("hello")

━━━━━━━━━━━━━━━━━━━━
+ TypedFactory'Value

────────────────────
+ static make Value
    - value Value

value
```

```output
[TypeInfo U32]
[TypeInfo String]
```

### Static methods preserve declared error results

```yumi
-- catch? Risky.read(true) ?? 99u32
-- catch? Risky.read(false) ?? 99u32

━━━━━━━━━━━━━━━━━━━━
+ Risky

────────────────────
+ static do read U32 ?! Error
    - fail Bool

if fail
    ?! Error("failed")
= 7u32
```

```output
99
7
```

### Static and instance methods occupy separate namespaces

```yumi
. object = Both()
object.label()
Both.label()

━━━━━━━━━━━━━━━━━━━━
+ Both

────────────────────
+ do label

-- "instance"

────────────────────
+ static do label

-- "static"
```

```output
instance
static
```

### Static dispatch accepts an implementation with broader generic constraints

```yumi
-- constrained-build'GeneralConstrainedFactory'Marked(Marked(7u32)).value

━━━━━━━━━━━━━━━━━━━━
+ Marker is Interface

━━━━━━━━━━━━━━━━━━━━
+ Marked is Tuple
    is Marker

+ value U32

━━━━━━━━━━━━━━━━━━━━
+ ConstrainedFactory is Interface

────────────────────
+ static make'Value Value
    - value Value
    - Value is Marker

━━━━━━━━━━━━━━━━━━━━
+ GeneralConstrainedFactory
    is ConstrainedFactory

────────────────────
+ static make'Value Value
    - value Value

value

━━━━━━━━━━━━━━━━━━━━
+ do constrained-build'Factory'Value Value
    - value Value
    - Factory is ConstrainedFactory
    - Value is Marker

= Factory.make'Value(value)
```

```output
7
```

### This names the receiving type inside a static method

```yumi
-- type-of SelfFactory.make()

━━━━━━━━━━━━━━━━━━━━
+ SelfFactory

────────────────────
+ static make This

This()
```

```output
[TypeInfo SelfFactory]
```

### Static requirements are inherited through interface constraints

```yumi
-- inherited-build'InheritedFactory(9u32)

━━━━━━━━━━━━━━━━━━━━
+ ParentFactory is Interface

────────────────────
+ static make U32
    - value U32

━━━━━━━━━━━━━━━━━━━━
+ ChildFactory is Interface
    is ParentFactory

━━━━━━━━━━━━━━━━━━━━
+ InheritedFactory
    is ChildFactory

────────────────────
+ static make U32
    - value U32

value + 1u32

━━━━━━━━━━━━━━━━━━━━
+ inherited-build'Builder U32
    - value U32
    - Builder is ChildFactory

Builder.make(value)
```

```output
10
```

### A call of a static is followed by a member.

```yumi
-- Thing.zero().total
-- Thing.make(5).twice().total
-- Thing.make(5).label
-- Thing.make(6).twice()
-- Thing.zero().total + 1
-- Pair.of(4).a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ total = 0

────────────────────
+ static zero Thing

Thing()

────────────────────
+ static do make Thing
    - n Int

. t = Thing()
t.total = n
= t

────────────────────
+ do twice Thing

. t = Thing()
t.total = total * 2
= t

────────────────────
+ get label

"label $total"

────────────────────
+ to String

"Thing($total)"

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ a Int
+ b Int

────────────────────
+ static of Pair
    - n Int

Pair(n, n + 1)
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

```yumi
. box = Box'Doubling(5)
-- box as String

━━━━━━━━━━━━━━━━━━━━
+ Scaling is Interface

────────────────────
+ static scale Int
    - value Int

━━━━━━━━━━━━━━━━━━━━
+ Doubling
    is Scaling

────────────────────
+ static scale Int
    - value Int

value * 2

━━━━━━━━━━━━━━━━━━━━
+ Box'Scaler
    - Scaler is Scaling

+ value Int

────────────────────
+ This
    - value

────────────────────
+ to String

"scaled ${Scaler.scale(value)}"
```

```output
scaled 10
```
