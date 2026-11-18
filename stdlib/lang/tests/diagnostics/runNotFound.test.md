# Not found

Not-found errors name what is missing, the module that declares it, or a close match

### type in an unimported core module

```yumi crash
. b = Bitmap()
```

```output
Exception: Type "Bitmap" not found; it is declared in /core/bitmap, which this module does not import. Add the import /core/bitmap
<no file:2>
    . b = Bitmap()
          ^^^^^^
```

### misspelled local

```yumi crash
. value = 3
-- valeu + 1
```

```output
Exception: Name "valeu" not found; did you mean "value"?
<no file:3>
    -- valeu + 1
       ^^^^^
```

### misspelled property

```yumi crash
. text = "abc"
-- text.lenght
```

```output
Exception: String has no property "lenght"; did you mean "length"?
<no file:3>
    -- text.lenght
       ^^^^^^^^^^^
```

### misspelled method

```yumi crash
-- Counter().incremnt()

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ count = 0

────────────────────
+ do increment

count += 1
```

```output
Exception: Counter has no method "incremnt"; did you mean "increment"?
<no file:2>
    -- Counter().incremnt()
       ^^^^^^^^^^^^^^^^^^^^
```

### misspelled function

```yumi crash
-- greer()

━━━━━━━━━━━━━━━━━━━━
+ greet

"hi"
```

```output
Exception: Function "greer" not found; did you mean "greet"?
<no file:2>
    -- greer()
       ^^^^^^^
```

### misspelled type

```yumi crash
. s = Strng("a")
```

```output
Exception: Type "Strng" not found; did you mean "String"?
<no file:2>
    . s = Strng("a")
          ^^^^^
```

### short names get no suggestion

```yumi crash
. a = 1
-- b
```

```output
Exception: Name "b" not found
<no file:3>
    -- b
       ^
```

### property of an optional

```yumi crash
. text String? = "abc"
-- text.length
```

```output
Exception: Property "length" is read from String?, which may be null; unwrap it first with ! or ?.
<no file:3>
    -- text.length
       ^^^^^^^^^^^
```

### method of an optional

```yumi crash
. counter Counter? = Counter()
counter.increment()

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ count = 0

────────────────────
+ do increment

count += 1
```

```output
Exception: Method "increment" is called on Counter?, which may be null; unwrap it first with ! or ?.
<no file:3>
    counter.increment()
    ^^^^^^^^^^^^^^^^^^^
```

### method of an optional called through ?. with arguments it does not accept

```yumi crash
. counter Counter? = Counter()
counter?.increment(2)

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ count = 0

────────────────────
+ do increment

count += 1
```

```output
Exception: No method "increment" of Counter accepts (I64); it is declared as "increment"
<no file:3>
    counter?.increment(2)
    ^^^^^^^^^^^^^^^^^^^^^
```

### property of an optional a test narrowed, read after a store to a property a read typed later tracks

```yumi crash
. holder = Holder()
holder.options = Options()
holder.run()
. view = View(holder)

━━━━━━━━━━━━━━━━━━━━
+ Options

+ name = "x"

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ options Options?
+ flag = false

────────────────────
+ do run

if options
    flag = true
    -- options.name

━━━━━━━━━━━━━━━━━━━━
+ View

+ holder Holder
+ shown = "${holder.#flag}"

────────────────────
+ This
    - holder
```

```output
Exception: Property "name" is read from Options?, which may be null; unwrap it first with ! or ?.
<no file:23>
        -- options.name
           ^^^^^^^^^^^^
```

### interpolated property

```yumi crash
. id = 4
-- "$id.png"
```

```output
Exception: Property "png" not found; in a string, "$id.png" reads the property "png" of id. Write "${id}.png" to put text after the value
<no file:3>
    -- "$id.png"
        ^^^^^^^
```

### interpolated dash

```yumi crash
. prefix = "a"
-- "$prefix-1"
```

```output
Exception: Name "prefix-1" not found; in a string, "$prefix-1" reads the whole name "prefix-1". Write "${prefix}-1" to put "-1" after the value
<no file:3>
    -- "$prefix-1"
         ^^^^^^^^
```

### interface constructor

```yumi crash
-- Shape()

━━━━━━━━━━━━━━━━━━━━
+ Shape is Interface

────────────────────
+ sides I32
```

```output
Exception: Shape is an interface, so it has no constructor; construct a class that is Shape
<no file:2>
    -- Shape()
       ^^^^^
```

### type in a loaded module this one does not import, named by its relative path

```yumi crash
./lib/shapes

. s = Square()
```

```yumi file=lib/shapes.yumi

./square

━━━━━━━━━━━━━━━━━━━━
+ shape-count I32

1

```

```yumi file=lib/square.yumi

━━━━━━━━━━━━━━━━━━━━
+ Square

+ side = 1

```

```output
Exception: Type "Square" not found; it is declared in ./lib/square, which this module does not import. Add the import ./lib/square
<no file:4>
    . s = Square()
          ^^^^^^
```

### private function of another module

```yumi crash
./lib/tools

-- secret()
```

```yumi file=lib/tools.yumi

━━━━━━━━━━━━━━━━━━━━
_ secret I32

1

```

```output
Exception: Function "secret" is private, declared with _, so only its own module can call it
<no file:4>
    -- secret()
       ^^^^^^^^
```

### slot outlet of a type this module cannot see

```yumi crash
<Panel

━━━━━━━━━━━━━━━━━━━━
+ MarkNode
    is MarkupParent'MarkNode

+ first-child MarkNode?

────────────────────
+ markup-content MarkNode
    - child MarkNode

child

────────────────────
+ do markup-attach-child
    - child MarkNode
    - anchor MarkNode?

first-child = child

━━━━━━━━━━━━━━━━━━━━
+ Panel is MarkNode

────────────────────
+ <Nowhere header
```

```output
Exception: Type "Nowhere" not found
<no file:27>
    + <Nowhere header
       ^^^^^^^
```

### element of a class from another module whose slot outlet type is not found

```yumi crash
./lib/panel

<Panel
```

```yumi file=lib/panel.yumi

━━━━━━━━━━━━━━━━━━━━
+ MarkNode
    is MarkupParent'MarkNode

+ first-child MarkNode?

────────────────────
+ markup-content MarkNode
    - child MarkNode

child

────────────────────
+ do markup-attach-child
    - child MarkNode
    - anchor MarkNode?

first-child = child

━━━━━━━━━━━━━━━━━━━━
+ Panel is MarkNode

────────────────────
+ <Nowhere header

```

```output
Exception: Type "Nowhere" not found
<lib/panel.yumi:25>
    + <Nowhere header
       ^^^^^^^
```

### "each" read where nothing gives an item is named, with what gives one.

```yumi crash
+ m ~>
    ~> idle
    ~> busy if each > 1
```

```output
Exception: Name "each" not found, as nothing here gives an item; "each" reads the item of a stage, a loop, a callback or a trigger
<no file:4>
        ~> busy if each > 1
                   ^^^^
```

### pow names the power operator

```yumi crash
-- pow(2.0, 3.0)
```

```output
Exception: Function "pow" not found; write "a %% b" to raise a to the power b
<no file:2>
    -- pow(2.0, 3.0)
       ^^^^^^^^^^^^^
```

### pow as a method of a number names the power operator

```yumi crash
. x = 2.0
-- x.pow(3.0)
```

```output
Exception: F64 has no method "pow"; write "a %% b" to raise a to the power b
<no file:3>
    -- x.pow(3.0)
       ^^^^^^^^^^
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
