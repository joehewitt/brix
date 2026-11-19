# Not found

Not-found errors name what is missing, the module that declares it, or a close match

### type in an unimported core module

```brix crash
(main
  (let b (call Bitmap))
)
```

```output
Exception: Type "Bitmap" not found; it is declared in /core/bitmap, which this module does not import. Add the import /core/bitmap
<no file:3>
      (let b (call Bitmap))
                   ^^^^^^
```

### misspelled local

```brix crash
(main
  (let value 3)
  (print (+ valeu 1))
)
```

```output
Exception: Name "valeu" not found; did you mean "value"?
<no file:4>
      (print (+ valeu 1))
                ^^^^^
```

### misspelled property

```brix crash
(main
  (let text "abc")
  (print (. text lenght))
)
```

```output
Exception: String has no property "lenght"; did you mean "length"?
<no file:4>
      (print (. text lenght))
             ^^^^^^^^^^^^^^^
```

### misspelled method

```brix crash
(main
  (print (call (. (call Counter) incremnt)))
)

(type + Counter
  (prop + count _ (= 0))
  (fn-do + increment _
    (+= count 1)
  )
)
```

```output
Exception: Counter has no method "incremnt"; did you mean "increment"?
<no file:3>
      (print (call (. (call Counter) incremnt)))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### misspelled function

```brix crash
(main
  (print (call greer))
)

(fn-expr + greet _
  "hi"
)
```

```output
Exception: Function "greer" not found; did you mean "greet"?
<no file:3>
      (print (call greer))
             ^^^^^^^^^^^^
```

### misspelled type

```brix crash
(main
  (let s (call Strng "a"))
)
```

```output
Exception: Type "Strng" not found; did you mean "String"?
<no file:3>
      (let s (call Strng "a"))
                   ^^^^^
```

### short names get no suggestion

```brix crash
(main
  (let a 1)
  (print b)
)
```

```output
Exception: Name "b" not found
<no file:4>
      (print b)
             ^
```

### property of an optional

```brix crash
(main
  (let text (? String) "abc")
  (print (. text length))
)
```

```output
Exception: Property "length" is read from String?, which may be null; unwrap it first with "(!! …)" or "(?. …)"
<no file:4>
      (print (. text length))
             ^^^^^^^^^^^^^^^
```

### method of an optional

```brix crash
(main
  (let counter (? Counter) (call Counter))
  (call (. counter increment))
)

(type + Counter
  (prop + count _ (= 0))
  (fn-do + increment _
    (+= count 1)
  )
)
```

```output
Exception: Method "increment" is called on Counter?, which may be null; unwrap it first with "(!! …)" or "(?. …)"
<no file:4>
      (call (. counter increment))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### method of an optional called through ?. with arguments it does not accept

```brix crash
(main
  (let counter (? Counter) (call Counter))
  (call (?. counter increment) 2)
)

(type + Counter
  (prop + count _ (= 0))
  (fn-do + increment _
    (+= count 1)
  )
)
```

```output
Exception: No method "increment" of Counter accepts (I64); it is declared as "increment"
<no file:4>
      (call (?. counter increment) 2)
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### property of an optional a test narrowed, read after a store to a property a read typed later tracks

```brix crash
(main
  (let holder (call Holder))
  (= (. holder options) (call Options))
  (call (. holder run))
  (let view (call View holder))
)

(type + Options
  (prop + name _ (= "x"))
)

(type + Holder
  (prop + options (? Options))
  (prop + flag _ (= false))
  (fn-do + run _
    (if options
      (= flag true)
      (print (. options name))
    )
  )
)

(type + View
  (prop + holder Holder)
  (prop + shown _ (= (str (. holder #flag))))
  (fn-do + This _
    (param holder _)
  )
)
```

```output
Exception: Property "name" is read from Options?, which may be null; unwrap it first with "(!! …)" or "(?. …)"
<no file:19>
          (print (. options name))
                 ^^^^^^^^^^^^^^^^
```

### interface constructor

```brix crash
(main
  (print (call Shape))
)

(type + Shape (is Interface)
  (fn-expr + sides I32)
)
```

```output
Exception: Shape is an interface, so it has no constructor; construct a class that is Shape
<no file:3>
      (print (call Shape))
                   ^^^^^
```

### type in a loaded module this one does not import, named by its relative path

```brix crash
(import ./lib/shapes)
(blank)

(main
  (let s (call Square))
)
```

```brix file=lib/shapes.bx
(import ./square)

(fn-expr + shape-count I32
  1
)
```

```brix file=lib/square.bx
(type + Square
  (prop + side _ (= 1))
)
```

```output
Exception: Type "Square" not found; it is declared in ./lib/square, which this module does not import. Add the import ./lib/square
<no file:6>
      (let s (call Square))
                   ^^^^^^
```

### private function of another module

```brix crash
(import ./lib/tools)
(blank)

(main
  (print (call secret))
)
```

```brix file=lib/tools.bx
(fn-expr _ secret I32
  1
)
```

```output
Exception: Function "secret" is private, declared with _, so only its own module can call it
<no file:6>
      (print (call secret))
             ^^^^^^^^^^^^^
```

### slot outlet of a type this module cannot see

```brix crash
(main
  (<> Panel)
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (prop + first-child (? MarkNode))
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
    (= first-child child)
  )
)

(type + Panel (is MarkNode)
  (content
    (+<> Nowhere (id: header))
  )
)
```

```output
Exception: Type "Nowhere" not found
<no file:22>
        (+<> Nowhere (id: header))
             ^^^^^^^
```

### element of a class from another module whose slot outlet type is not found

```brix crash
(import ./lib/panel)
(blank)

(main
  (<> Panel)
)
```

```brix file=lib/panel.bx
(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (prop + first-child (? MarkNode))
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
    (= first-child child)
  )
)

(type + Panel (is MarkNode)
  (content
    (+<> Nowhere (id: header))
  )
)
```

```output
Exception: Type "Nowhere" not found
<lib/panel.bx:17>
        (+<> Nowhere (id: header))
             ^^^^^^^
```

### "each" read where nothing gives an item is named, with what gives one.

```brix crash
(prop + m _
  (machine
    (state idle)
    (state busy (trigger if (> each 1)))
  )
)
```

```output
Exception: Name "each" not found, as nothing here gives an item; "each" reads the item of a stage, a loop, a callback or a trigger
<no file:5>
        (state busy (trigger if (> each 1)))
                                   ^^^^
```

### pow names the power operator

```brix crash
(main
  (print (call pow 2.0 3.0))
)
```

```output
Exception: Function "pow" not found; write "(%% a b)" to raise a to the power b
<no file:3>
      (print (call pow 2.0 3.0))
             ^^^^^^^^^^^^^^^^^^
```

### pow as a method of a number names the power operator

```brix crash
(main
  (let x 2.0)
  (print (call (. x pow) 3.0))
)
```

```output
Exception: F64 has no method "pow"; write "(%% a b)" to raise a to the power b
<no file:4>
      (print (call (. x pow) 3.0))
             ^^^^^^^^^^^^^^^^^^^^
```

### in Brix, pow names the power operator in Brix

```brix crash
(main (print (str (call pow 2.0 3.0))))
```

```output
Exception: Function "pow" not found; write "(%% a b)" to raise a to the power b
<no file:2>
    (main (print (str (call pow 2.0 3.0))))
                      ^^^^^^^^^^^^^^^^^^
```
