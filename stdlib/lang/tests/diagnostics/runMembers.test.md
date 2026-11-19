# Members

Errors about members, cases and overrides name the type, the member, the declaration involved, and the fix

### a property declared twice

```brix crash
(main
  (print (. (call Thing) a))
)

(type + Thing
  (prop + a _ (= 1))
  (prop + a _ (= 2))
)
```

```output
Exception: Thing declares "a" twice (first on line 7); remove one of them, or rename it
<no file:8>
      (prop + a _ (= 2))
      ^^^^^^^^^^^^^^^^^^
```

### an override that states a type

```brix crash
(main
  (let base Source (call Narrow))
  (print (. base value))
)

(type + Source
  (fn-expr + value I32 :get
    1
  )
)

(type + Narrow (is Source)
  (fn-expr + value I32 :get
    7
  )
)
```

```output
Exception: "value" is inherited from Source, so it keeps the type it has there and cannot state its own; remove "I32"
<no file:14>
      (fn-expr + value I32 :get
      ^^^^^^^^^^^^^^^^^^^^^^^^
```

### an arrow override that states a type, reported on its own line

```brix crash
(main
  (let base Source (call Narrow))
  (print (. base value))
)

(type + Source
  (prop + value I32 (=> 1))
)

(type + Narrow (is Source)
  (prop + value I32 (=> 7))
  (prop + other _ (=> 2))
)
```

```output
Exception: "value" is inherited from Source, so it keeps the type it has there and cannot state its own; remove "I32"
<no file:12>
      (prop + value I32 (=> 7))
      ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### an override stored differently from the property it overrides

```brix crash
(main
  (let base (as (call Thing) Base))
  (print (. base a))
)

(type + Base
  (prop + a _ (= "Base"))
)

(type + Thing (is Base)
  (prop + a _ (:= "Thing"))
)
```

```output
Exception: "a" is inherited from Base, which declares it with "=", so declare it with "=" here too, not with ":="
<no file:12>
      (prop + a _ (:= "Thing"))
      ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### a misspelled case of an enum

```brix crash
(main
  (let kind (. Kind thrid))
)

(type + Kind (is U32)
  (variant first (= 0))
  (variant third (= 1))
)
```

```output
Exception: Kind has no case "thrid"; did you mean "third"?
<no file:3>
      (let kind (. Kind thrid))
                ^^^^^^^^^^^^^^
```

### a misspelled case named by its symbol alone

```brix crash
(main
  (let kind Kind .thrid)
)

(type + Kind (is U32)
  (variant first (= 0))
  (variant third (= 1))
)
```

```output
Exception: Case ".thrid" not found; did you mean ".third"?
<no file:3>
      (let kind Kind .thrid)
                     ^^^^^^
```

### a case named alone whose type is in a module this one does not import

```brix crash
(import ./lib/rows)
(blank)

(main
  (let kind .second)
  (print (as kind U32))
)
```

```brix file=lib/rows.bx
(import ./kinds)

(fn-expr + first-kind Kind
  (. Kind first)
)
```

```brix file=lib/kinds.bx
(type + Kind (is U32)
  (variant first (= 0))
  (variant second (= 1))
)
```

```output
Exception: Case ".second" belongs to Kind, which this module does not import, so the case cannot be named alone; write "Kind.second" and add the import ./lib/kinds
<no file:6>
      (let kind .second)
                ^^^^^^^
```

### optional chaining on a value that is never null

```brix crash
(main
  (let thing (call Thing))
  (call (?. thing show))
)

(type + Thing
  (fn-do + show _
    (print "shown")
  )
)
```

```output
Exception: "thing" is Thing, which is never null, so it needs no "?"; write "(call (. thing show))"
<no file:4>
      (call (?. thing show))
      ^^^^^^^^^^^^^^^^^^^^^^
```

### a member two extensions declare

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

(type + Node (is Union)
  (variant literal Literal)
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

### an interface getter without a body that a class does not implement

```brix crash
(main
  (let source Source (call Lines))
  (print (. source revision))
)

(type + Source (is Interface)
  (fn-expr + revision U64 :get)
)

(type + Lines
  (implements Source)
)
```

```output
Exception: Lines does not implement "revision", which Source declares without a body; declare "revision" in Lines
<no file:11>
    (type + Lines
    ^^^^^^^^^^^^
Interface:
<no file:8>
      (fn-expr + revision U64 :get)
```

### a constructor's "lazy" initializer that calls a method of the object

```brix crash
(main
  (print (. (call Steps) first))
)

(type + Steps
  (prop + first String)
  (fn-do + This _
    (lazy first (call add-step "Check"))
  )
  (fn-do + add-step String
    (param title String)
    (return (str "step " title))
  )
)
```

```output
Exception: A "lazy" initializer runs before the object exists, so it cannot call "add-step", a method of Steps; compute the value from the constructor's parameters, or assign it in the constructor's body
<no file:9>
        (lazy first (call add-step "Check"))
                    ^^^^^^^^^^^^^^^^^^^^^^^
```

### a property's value that calls a method of the object

```brix crash
(main
  (print (. (call Steps) first))
)

(type + Steps
  (prop + first _ (= (call add-step "Check")))
  (fn-do + add-step String
    (param title String)
    (return (str "step " title))
  )
)
```

```output
Exception: A property's value is computed before the object exists, so it cannot call "add-step", a method of Steps; declare the property without a value and assign it in a constructor's body
<no file:7>
      (prop + first _ (= (call add-step "Check")))
                         ^^^^^^^^^^^^^^^^^^^^^^^
```

### a live markup loop under a host that makes no region for it

```brix compile-error
(main
  (<> Shelf (id: shelf)
    (for (in title (. shelf #titles))
      (<> Book (value: title))
    )
  )
)

(type + Book
  (implements (of MarkupParent Book))
  (prop + title String)
  (fn-do + This _
    (param title _)
  )
)

(type + Shelf
  (implements (of MarkupParent Book))
  (prop + titles (list String))
)
```

```output
Exception: Markup loop host Shelf must declare markup-make-region, or extend MarkupNode
<no file:4>
        (for (in title (. shelf #titles))
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### a live markup branch under a host that makes no region for it

```brix compile-error
(main
  (<> Shelf (id: shelf)
    (if #open
      (<> Book (value: "Dune"))
    )
  )
)

(type + Book
  (implements (of MarkupParent Book))
  (prop + title String)
  (fn-do + This _
    (param title _)
  )
)

(type + Shelf
  (implements (of MarkupParent Book))
  (prop + open _ (= true))
)
```

```output
Exception: Markup branch host Shelf must declare markup-make-region, or extend MarkupNode
<no file:4>
        (if #open
        ^^^^^^^^
```

### in Brix, a member read from a value that may be null names the unwrap in Brix

```brix crash
(type + Box
  (prop + n _ (= 2))
)

(main
  (let box (? Box) (call Box))
  (print (str (. box n)))
)
```

```output
Exception: Property "n" is read from Box?, which may be null; unwrap it first with "(!! …)" or "(?. …)"
<no file:8>
      (print (str (. box n)))
                  ^^^^^^^^^
```

### a number with no suffix that the cases of a union take as different types

```brix crash
(main
  (let item Item (call (. Item left) (call Left 10)))
  (print (call (. item scaled) 2))
)

(type + Left (is Tuple)
  (prop + value I32)
  (fn-expr + scaled Float
    (param by Float)
    (* (as value Float) by)
  )
)

(type + Right (is Tuple)
  (prop + value Float)
  (fn-expr + scaled Float
    (param by Int)
    (* value by)
  )
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
Exception: Union cases take this literal as different types: F64 for case 'left' and I64 for case 'right'; write "2i32"
<no file:4>
      (print (call (. item scaled) 2))
                                   ^
```
