# Style reads

### $property reads resolve statically against the visible style vocabulary and compile to the declared value type: bare $name reads the current node and object.$name reads an explicit subject. Reads never settle: values read before settling are the style's initial values.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink 9)
      (= bold true)
      (= scale 2.5)
      (= margin 3)
    )
  )
  (blank)
  (let label (!! (. demo %first-child)))
  (print (call (. demo read-styles) label))
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. demo read-styles) label))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (fn-do + read-styles String
    (param label Box)
    (let own $ink)
    (let child-ink (. label $ink))
    (let child-bold (. label $bold))
    (let child-scale (. label $scale))
    (let child-margin (. label $margin))
    (return (str "own=" own " ink=" child-ink " bold=" child-bold " scale=" child-scale " margin=" child-margin))
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (prop + bold Bool (= false))
  (prop + scale F64 (= 1.5))
  (prop + margin I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
  (style-domains bold (domain paint))
  (style-domains scale (domain layout))
  (style-domains margin (domain layout))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
own=7 ink=7 bold=false scale=1.5 margin=0
own=7 ink=9 bold=true scale=2.5 margin=3
```

### Bare style reads can be the root of an ordinary member chain.

```brix
(import /core/style)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call ChainDemo))
  (print (call (. demo read-chain)))
)

(type + ChainMetric (is Tuple)
  (prop + amount I64)
)

(type + ChainStyle (is Style)
  (prop + metric ChainMetric (= (call ChainMetric 41)))
)

(type + ChainDemo (is Box)
  (using ChainStyle)
  (fn-expr + read-chain I64
    (. $metric amount)
  )
)
```

```output
41
```

### Style reads are legal anywhere: free functions and ordinary methods read settled computed values directly through subject.$property without any annotation.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink 9)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let label (as (!! (. demo %first-child)) Label))
  (print (str "free=" (call read-ink-anywhere label)))
  (print (str "method=" (call (. demo plain-read) label)))
  (print (str "top=" (. label $ink)))
)

(fn-expr + read-ink-anywhere I64
  (param label Label)
  (. label $ink)
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (fn-expr + plain-read I64
    (param label Label)
    (. label $ink)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
free=9
method=9
top=9
```

### Optional chaining on style reads returns the computed value for a present subject and null for a missing subject, matching ordinary member and structural markup chains.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink 14)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let maybe-label (? Label) (as (!! (. demo %first-child)) Label))
  (let missing (? Label) null)
  (print (str "present=" (?? (?. maybe-label $ink) -1)))
  (print (str "missing=" (?? (?. missing $ink) -1)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
present=14
missing=-1
```

### Style reads are total: authored values are contextual to the mounted tree, so an object that leaves it reads the property's registered initial value instead of its settled value.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink 42)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let label (as (!! (. demo %first-child)) Label))
  (print (. label $ink))
  (blank)
  (call markup-remove demo label)
  (print (. label $ink))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
42
7
```

### Styles register at the head of their defining module's init: module-level code in an importing module can mount, settle, and read styled nodes during its own initialization, before main runs, and sees authored values and unauthored defaults.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import ./theme)
(blank)

(main
  (<> InitDemo (id: demo)
    (<> Label
      (= ink 12)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let label (as (!! (. demo %first-child)) Label))
  (let ink (. label $ink))
  (let tone (. label $tone))
  (print (str "init-read ink=" ink " tone=" tone))
)

(type + InitDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Label (is Box)
  (using TextStyle)
)
```

```brix file=theme.bx
(import /core/style)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (prop + tone I64 (= 3))
  (blank)
  (style-domains ink (domain paint))
)
```

```output
init-read ink=12 tone=3
```

### An interface can declare the style vocabularies its values expose. Style reads and imperative writes on interface values resolve through the interface's usings.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (list Tinted) (call (of List Tinted)))
  (+= items (call Swatch))
  (+= items (call Chip))
  (let level 5)
  (for (in item items)
    (= (. item tint) level)
    (+= level 1)
  )
  (for (in item items)
    (print (str "tint=" (. item $tint) " authored=" (call style-has-authored-property item "TintStyle" "tint")))
  )
  (print (str "fresh=" (call tint-of (call Chip))))
)

(fn-expr + tint-of I64
  (param item Tinted)
  (. item $tint)
)

(type + TintStyle (is Style)
  (prop + tint I64 (= 1))
  (blank)
  (style-domains tint (domain paint))
)

(type + Tinted (is Interface)
  (implements MarkupElement)
  (using TintStyle)
)

(type + Swatch (is Box)
  (implements Tinted)
  (using TintStyle)
)

(type + Chip (is Box)
  (implements Tinted)
  (using TintStyle)
)
```

```output
tint=5 authored=true
tint=6 authored=true
fresh=1
```

### Implementing an interface that declares a style vocabulary requires using that style.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let item Tinted (call Plain))
  (print (. item $tint))
)

(type + TintStyle (is Style)
  (prop + tint I64 (= 1))
)

(type + Tinted (is Interface)
  (implements MarkupElement)
  (using TintStyle)
)

(type + Plain (is Box)
  (implements Tinted)
)
```

```output
Exception: Plain must use TintStyle to implement Tinted
<no file:21>
    (type + Plain (is Box)
    ^^^^^^^^^^^^^^^^^^^^^
```

### A character style value set from a name is kept in its slot as a number and read back as the character.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let chosen 'y')
  (blank)
  (<> BatchDemo (id: demo)
    (<> Label
      (= mark chosen)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (. (as (!! (. demo %first-child)) Label) $mark))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using MarkStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + MarkStyle (is Style)
  (prop + mark Char (= 'a'))
)

(type + Label (is Box)
  (using MarkStyle)
)
```

```output
y
```

### A style that only an object the program never makes uses is not registered, so its initial values are never evaluated.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let label (call Label))
  (print "made")
)

(type + InkStyle (is Style)
  (prop + ink I64 (= 7))
)

(type + ToneStyle (is Style)
  (prop + tone I64 (= (call boom)))
)

(fn-do + boom I64
  (throw (call Panic "boom"))
  (return 3)
)

(type + Label (is Box)
  (using InkStyle)
)

(type + Badge (is Box)
  (using ToneStyle)
)

(type + Holder
  (prop + badge _ (:= (call Badge)))
)
```

```output
made
```

### A name a method assigns that nothing declares is the property of a style the object uses, which the assignment declares for the object.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (<> Label (id: label))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (call (. label paint))
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot label))
)

(type + Demo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(type + Label (is Box)
  (using TextStyle)
  (fn-do + paint _
    (= ink 12)
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=12
```

### A style read with no object reads this, so a script, which has none, sees no style property.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (print $ink)
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)
```

```output
Exception: Style property "ink" not found
<no file:8>
      (print $ink)
             ^^^^
```
