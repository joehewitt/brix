# Extensions

### Extension functions, getters, and converters work in the declaring module

```brix
(main
  (let thing (call Thing))
  (print (call (. thing describe)))
  (print (. thing label))
  (print (as thing String))
)

(type + Thing)

(extend + Thing
  (fn-expr + describe String
    "described"
  )
  (fn-expr + label String :get
    "label"
  )
  (fn-to-expr + String
    "converted"
  )
)
```

```output
described
label
converted
```

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 2))
  (= ([] rows 0) (call (. Node left) 10))
  (= ([] rows 1) (call (. Node right) 20))
  (print (. ([] rows 0) label))
  (print (. ([] rows 1) label))
)

(type + Left (is Tuple)
  (prop + value I32)
)

(type + Right (is Tuple)
  (prop + value I32)
)

(type + Node (is Union)
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
right:20
```

### Imported and re-exported union, shared-base, and narrowed-case extensions operate on live Table row views.

```brix
(import /core/table)
(blank)
(import ./lib/model)
(import ./lib/reexport)
(blank)

(main
  (let rows (call (of Table Node) 2))
  (= ([] rows 0) (call (. Node literal) 10 2))
  (= ([] rows 1) (call (. Node call) 20 3))
  (blank)
  (print (call (. ([] rows 0) describe)))
  (print (call (. ([] rows 1) describe)))
  (print (. ([] rows 0) source-plus))
  (blank)
  (for (in row rows)
    (print (call (. row describe)))
  )
  (blank)
  (match ([] rows 0)
    (case (call .literal literal)
      (print (. literal case-label))
      (call (. literal bump))
      (print "bumped")
    )
    (case .call
      (print "unexpected")
    )
  )
  (blank)
  (print (as ([] rows 0) Node))
)
```

```brix file=lib/model.bx
(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Literal (is Syntax)
  (prop + value U32)
)

(type + Call (is Syntax)
  (prop + target U32)
)

(type + Node (is Union)
  (variant literal Literal)
  (variant call Call)
)
```

```brix file=lib/extensions.bx
(import ./model)

(extend + Node
  (fn-expr + describe String
    (match this
      (case (call .literal literal)
        (str "literal:" (. literal value))
      )
      (case (call .call call)
        (str "call:" (. call target))
      )
    )
  )
)

(extend + Syntax
  (fn-expr + source-plus U32 :get
    (+ (. this source) 1)
  )
)

(extend + Literal
  (fn-expr + case-label String :get
    (str "literal:" (. this value))
  )
  (fn-do + bump _
    (+= (. this value) 5)
  )
)
```

```brix file=lib/reexport.bx
(export ./extensions)
```

```output
literal:2
call:3
11
literal:2
call:3
literal:2
bumped
(10, 7)
```

### A concrete-case extension is unavailable on an unmatched union row view.

```brix crash
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 1))
  (= ([] rows 0) (call (. Node literal) 1))
  (print (call (. ([] rows 0) only-literal)))
)

(type + Literal (is Tuple)
  (prop + value U32)
)

(type + Call (is Tuple)
  (prop + target U32)
)

(type + Node (is Union)
  (variant literal Literal)
  (variant call Call)
)

(extend + Literal
  (fn-expr + only-literal String
    "literal"
  )
)
```

```output
Exception: Union case 'call' has no complete dispatch for method 'only-literal'
<no file:8>
      (print (call (. ([] rows 0) only-literal)))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Logical and explicitly physical row extensions retain the existing ambiguity rule.

```brix crash
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 1))
  (= ([] rows 0) (call (. Node literal) 1))
  (print (call (. ([] rows 0) conflict)))
)

(type + Literal (is Tuple)
  (prop + value U32)
)

(type + Call (is Tuple)
  (prop + target U32)
)

(type + Node (is Union)
  (variant literal Literal)
  (variant call Call)
)

(extend + Node
  (fn-expr + conflict String
    "logical"
  )
)

(extend + (of TableRow Node)
  (fn-expr + conflict String
    "physical"
  )
)
```

```output
Exception: More than one extension declares "conflict" for TableRow'Node, so the call is ambiguous; remove or rename all but one of them
<no file:8>
      (print (call (. ([] rows 0) conflict)))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Extension members are visible after import

```brix
(import ./lib/model)
(import ./lib/extension)
(blank)

(main
  (let thing (call Thing))
  (print (call (. thing describe)))
  (print (. thing label))
  (print (as thing String))
)
```

```brix file=lib/model.bx
(type + Thing)
```

```brix file=lib/extension.bx
(import ./model)

(extend + Thing
  (fn-expr + describe String
    "imported"
  )
  (fn-expr + label String :get
    "imported-label"
  )
  (fn-to-expr + String
    "imported-conversion"
  )
)
```

```output
imported
imported-label
imported-conversion
```

### Extension members are unavailable without import

```brix crash
(import ./lib/model)
(blank)

(main
  (print (call (. (call Thing) describe)))
)
```

```brix file=lib/model.bx
(type + Thing)
```

```brix file=lib/extension.bx
(import ./model)

(extend + Thing
  (fn-expr + describe String
    "hidden"
  )
)
```

```output
Exception: Thing has no method "describe"
<no file:6>
      (print (call (. (call Thing) describe)))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Ambiguous extension methods produce a diagnostic

```brix crash
(import ./lib/model)
(import ./lib/ext-a)
(import ./lib/ext-b)
(blank)

(main
  (print (call (. (call Thing) conflict)))
)
```

```brix file=lib/model.bx
(type + Thing)
```

```brix file=lib/ext-a.bx
(import ./model)

(extend + Thing
  (fn-expr + conflict String
    "a"
  )
)
```

```brix file=lib/ext-b.bx
(import ./model)

(extend + Thing
  (fn-expr + conflict String
    "b"
  )
)
```

```output
Exception: More than one extension declares "conflict" for Thing, so the call is ambiguous; remove or rename all but one of them
<no file:8>
      (print (call (. (call Thing) conflict)))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Re-exported extension members are visible to importers

```brix
(import ./lib/model)
(import ./lib/reexport)
(blank)

(main
  (print (call (. (call Thing) describe)))
)
```

```brix file=lib/model.bx
(type + Thing)
```

```brix file=lib/extension.bx
(import ./model)

(extend + Thing
  (fn-expr + describe String
    "reexported"
  )
)
```

```brix file=lib/reexport.bx
(export ./extension)
```

```output
reexported
```

### Extension methods can overload direct methods with a distinct signature

```brix
(main
  (let thing (call Thing))
  (print (call (. thing describe)))
  (call (. thing describe) 7)
)

(type + Thing
  (fn-expr + describe String
    "direct"
  )
)

(extend + Thing
  (fn-do + describe _
    (param value Int)
    (print (str "extension" value))
  )
)
```

```output
direct
extension7
```

### Extension methods can not override direct methods with the same signature

```brix crash
(main
  (print (call (. (call Thing) describe)))
)

(type + Thing
  (fn-expr + describe String
    "direct"
  )
)

(extend + Thing
  (fn-expr + describe String
    "extension"
  )
)
```

```output
Exception: Extension member conflicts with existing member
<no file:3>
      (print (call (. (call Thing) describe)))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Extension getters can not override stored properties

```brix crash
(main
  (print (. (call Thing) label))
)

(type + Thing
  (prop + label _ (= "direct"))
)

(extend + Thing
  (fn-expr + label String :get
    "extension"
  )
)
```

```output
Exception: Extension member conflicts with existing member
<no file:3>
      (print (. (call Thing) label))
             ^^^^^^^^^^^^^^^^^^^^^^
```

### Ambiguous extension getters produce a diagnostic

```brix crash
(main
  (print (. (call Thing) label))
)

(type + Thing)

(extend + Thing
  (fn-expr + label String :get
    "a"
  )
)

(extend + Thing
  (fn-expr + label String :get
    "b"
  )
)
```

```output
Exception: More than one extension declares "label" for Thing, so the call is ambiguous; remove or rename all but one of them
<no file:3>
      (print (. (call Thing) label))
             ^^^^^^^^^^^^^^^^^^^^^^
```

### Extension converters can not override direct converters

```brix crash
(main
  (print (as (call Thing) String))
)

(type + Thing
  (fn-to-expr + String
    "direct"
  )
)

(extend + Thing
  (fn-to-expr + String
    "extension"
  )
)
```

```output
Exception: Extension member conflicts with existing member
<no file:3>
      (print (as (call Thing) String))
             ^^^^^^^^^^^^^^^^^^^^^^^^
```

### Ambiguous extension converters produce a diagnostic

```brix crash
(main
  (print (as (call Thing) String))
)

(type + Thing)

(extend + Thing
  (fn-to-expr + String
    "a"
  )
)

(extend + Thing
  (fn-to-expr + String
    "b"
  )
)
```

```output
Exception: Ambiguous extension member
<no file:3>
      (print (as (call Thing) String))
             ^^^^^^^^^^^^^^^^^^^^^^^^
```

### Extension fromverters can not override direct fromverters

```brix crash
(main
  (let bar (call Bar))
  (print (as bar Foo))
)

(type + Foo
  (fn-from-expr + (: bar Bar)
    (call This)
  )
)

(type + Bar)

(extend + Foo
  (fn-from-expr + (: bar Bar)
    (call This)
  )
)
```

```output
Exception: Extension member conflicts with existing member
<no file:4>
      (print (as bar Foo))
             ^^^^^^^^^^^^
```

### Ambiguous extension fromverters produce a diagnostic

```brix crash
(main
  (let bar (call Bar))
  (print (as bar Foo))
)

(type + Foo)

(type + Bar)

(extend + Foo
  (fn-from-expr + (: bar Bar)
    (call This)
  )
)

(extend + Foo
  (fn-from-expr + (: bar Bar)
    (call This)
  )
)
```

```output
Exception: Ambiguous extension member
<no file:4>
      (print (as bar Foo))
             ^^^^^^^^^^^^
```

### Conditional extension member is available when the receiver type satisfies where

```brix
(main
  (print (call (. (call (of Box Good)) describe)))
)

(type + Printable (is Interface))

(type + Good (is Printable))

(type + (of Box T))

(extend + (of Box ''T)
  (where
    (constraint is T Printable)
  )
  (fn-expr + describe String
    "printable"
  )
)
```

```output
printable
```

### Conditional extension member is unavailable when the receiver type does not satisfy where

```brix crash
(main
  (print (call (. (call (of Box Other)) describe)))
)

(type + Printable (is Interface))

(type + Other)

(type + (of Box T))

(extend + (of Box ''T)
  (where
    (constraint is T Printable)
  )
  (fn-expr + describe String
    "printable"
  )
)
```

```output
Exception: Box'Other has no method "describe"
<no file:3>
      (print (call (. (call (of Box Other)) describe)))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Generic function uses declaration-site extension visibility

```brix
(import ./lib/model)
(import ./lib/use-extension)
(blank)

(main
  (print (call call-describe (call Box)))
)
```

```brix file=lib/model.bx
(type + Box)
```

```brix file=lib/extension.bx
(import ./model)

(extend + Box
  (fn-expr + describe String
    "declaration-site"
  )
)
```

```brix file=lib/use-extension.bx
(import ./model)
(import ./extension)

(fn-expr + call-describe _
  (param box Box)
  (call (. box describe))
)
```

```output
declaration-site
```

### Concrete extensions do not apply to unresolved generic construction.

```brix
(main
  (let box (call (of Box Concrete) 1))
  (let source (call Source box))
  (print (call (. source ping)))
)

(type + Concrete)

(type + (of Box T) (is Tuple)
  (prop + size Size)
  (fn-do + This _
    (param size Size)
  )
)

(extend + (of Box Concrete)
  (fn-expr + marker String
    "marker"
  )
)

(type + (of Readable Token) (is Interface)
  (fn-to-expr + (of Box Concrete)
    (call (. (call (of TokenStream Token) this) read-box))
  )
)

(type + (of TokenStream Token)
  (prop _ readable (of Readable Token))
  (prop _ scratch _ (= (call (of Box Token) 0)))
  (fn-do + This _
    (param readable (of Readable Token))
  )
  (fn-expr + read-box (of Box Concrete)
    (call (of Box Concrete) (. scratch size))
  )
)

(type + (of Source Value)
  (implements (of Readable Value))
  (prop + box (of Box Value))
  (fn-do + This _
    (param box (of Box Value))
  )
  (fn-expr + ping String
    "pong"
  )
)
```

```output
pong
```

### Extension as clauses parse, then produce phase 1 NYI during AST lowering

```brix crash
(type + SomeInterface (is Interface))

(type + Thing)

(extend + Thing (as SomeInterface)
  (fn-expr + describe String
    "nyi"
  )
)
```

```output
Exception: Not yet implemented
<no file:6>
    (extend + Thing (as SomeInterface)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Stored properties are rejected in phase 1 extensions

```brix crash
(type + Thing)

(extend + Thing
  (prop + value Int)
)
```

```output
Exception: Stored properties are not allowed in extensions
<no file:5>
      (prop + value Int)
      ^^^^^^^^^^^^^^^^^^
```

### Generic and backing-specific extensions use the constructor's final concrete owner type.

```brix
(main
  (let managed (call (of ConstructorExtensionBox U32)))
  (let native (call (of ConstructorExtensionBox U32) (call ExtensionArenaToken)))
  (print (call (. managed generic-label)))
  (print (call (. native generic-label)))
  (print (call (. native arena-label)))
)

(type + ExtensionArenaToken)

(type + (of ManagedExtensionBacking Value))

(type + (of ArenaExtensionBacking Value))

(type + (of ConstructorExtensionBox Value Backing)
  (constraint = Backing (of ManagedExtensionBacking Value))
  (fn-do + This _)
  (fn-do + This _
    (param token ExtensionArenaToken)
    (constraint = Backing (of ArenaExtensionBacking Value))
  )
)

(extend + (of ConstructorExtensionBox ''Value ''Backing)
  (fn-expr + generic-label String
    "generic"
  )
)

(extend + (of ConstructorExtensionBox U32 (of ArenaExtensionBacking U32))
  (fn-expr + arena-label String
    "arena"
  )
)
```

```output
generic
generic
arena
```

### A method and a getter that `extend` adds to a number or to text take the value as `this`.

```brix
(main
  (let x 7)
  (print (call (. x double)))
  (print (call (. (call (. x double)) double)))
  (print (. x half))
  (print (call (. "ab" wrap) "[" "]"))
  (print (call (. 5 double)))
)

(extend + Int
  (fn-expr + double Int
    (* this 2)
  )
  (fn-expr + half Int :get
    (/ this 2)
  )
)

(extend + String
  (fn-expr + wrap String
    (param left String)
    (param right String)
    (++ left this right)
  )
)
```

```output
14
28
3
[ab]
10
```

### The receiver of a method is evaluated before its arguments.

```brix
(main
  (print (call (. (call show 1) plus) (call show 2) (call show 3)))
)

(fn-do + show Int
  (param n Int)
  (print n)
  (return n)
)

(extend + Int
  (fn-expr + plus Int
    (param a Int)
    (param b Int)
    (+ this a b)
  )
)
```

```output
1
2
3
6
```

### A name after a number and a dot is the number's unit, not a member.

```brix crash
(main
  (print (call 5.double))
)

(extend + Int
  (fn-expr + double Int
    (* this 2)
  )
)
```

```output
Exception: Unit "double" not found
<no file:3>
      (print (call 5.double))
                     ^^^^^^
```

### An extension gives a member to a class that has a subclass.

```brix
(main
  (let b Base (call Kid))
  (print (call (. (call Base) who)))
  (print (call (. (call Kid) who)))
  (print (call (. b who)))
  (print (. b tag))
)

(type + Base
  (fn-expr + name String
    "base"
  )
)

(type + Kid (is Base)
  (fn-expr + name String
    "kid"
  )
)

(extend + Base
  (fn-expr + who String
    (str "ext " (call name))
  )
  (fn-expr + tag String :get
    (str "tag " (call name))
  )
)
```

```output
ext base
ext kid
ext kid
tag kid
```

### An extension that names its class's parameter matches every instance, and its methods take their own parameters written.

```brix
(main
  (let d (call (of Holder Int)))
  (print (call (of (. d get) Int)))
  (print (call (of (. d describe) String)))
)

(type + (of Holder Event))

(extend + (of Holder Event)
  (fn-expr + (of get T) String
    (constraint is! T Event)
    "event"
  )
  (fn-expr + (of describe T) String
    "described"
  )
)
```

```output
event
described
```

### A method that `extend Number` adds takes the number it is called on as `this`, of that number's own type.

```brix
(main
  (let i 5)
  (let f 2.5)
  (print (call (. i doubled)))
  (print (call (. f doubled)))
  (print (call (. i kind)))
  (print (call (. f kind)))
)

(extend + Number
  (fn-expr + doubled _
    (* this 2)
  )
  (fn-expr + kind String
    (str (type-of this))
  )
)
```

```output
10
5
[TypeInfo I64]
[TypeInfo F64]
```

### A setter that an extension adds takes the value assigned to its name, and an assigning operator reads the extension's getter and stores through its setter.

```brix
(main
  (let box (call Box))
  (= (. box label) "abcde")
  (print (. box size))
  (print (. box label))
  (+= (. box label) "xy")
  (print (. box size))
)

(type + Box
  (prop + size _ (= 4))
)

(extend + Box
  (fn-expr + label String :get
    (str "size " size)
  )
  (fn-do + label _ :set (param value String)
    (= size (. value length))
  )
)
```

```output
5
size 5
8
```
