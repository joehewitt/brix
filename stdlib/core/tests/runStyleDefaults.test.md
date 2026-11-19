# Style defaults

### A parameterless "This" rule auto-applies to every instance of its class at the defaults layer, below rule and inline declarations.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Fancy (• accent)
      (= ink 9)
    )
  )
  (blank)
  (print (call style-authored-snapshot (!! (. demo %first-child))))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
  (prop + bold Bool (= false))
)

(type + Fancy (is Box)
  (using TextStyle)
  (rule This
    (= ink 13)
    (= bold true)
  )
  (blank)
  (rule accent
    (= ink 5)
  )
)
```

```output
node TextStyle.ink layer=0 order=65536 kind=static bits=13
node TextStyle.bold layer=0 order=65537 kind=static bits=1
node TextStyle.ink layer=2 order=0 kind=static bits=5
node TextStyle.ink layer=5 order=1 kind=static bits=9
```

### Markup property assignments still feed construction when the node also has style rule applications, so content created by the constructed object sees the assigned value.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let plain (call Card))
  (let child (call ChildCard))
  (blank)
  (print (call title-child-of plain))
  (print (call title-child-of child))
)

(fn-expr + title-child-of String
  (param card Card)
  (. (as (!! (. (as (!! (. card %first-child)) TitleBox) %first-child)) TitleText) title)
)

(type + Card (is Box)
  (prop + title _ (= "Default"))
  (content
    (<> This
      (<> TitleBox (• marker)
        (= title title)
      )
    )
  )
)

(type + ChildCard (is Card)
  (prop + title _ (= "Child"))
)

(type + TitleBox (is Box)
  (prop + title _ (= "Box Default"))
  (blank)
  (rule marker
    (= flex 1)
  )
  (content
    (<> TitleText
      (= title title)
    )
  )
)

(type + TitleText (is Box)
  (prop + title _ (= "Text Default"))
)
```

```output
Default
Child
```

### Computed values follow layer precedence: a class's "This" defaults yield to an applied rule, and reappear on instances with no rule applied.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Fancy (id: plain))
    (<> Fancy (id: styled) (• accent))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot plain))
  (print (call style-computed-snapshot styled))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(type + Fancy (is Box)
  (using TextStyle)
  (rule This
    (= ink 13)
  )
  (blank)
  (rule accent
    (= ink 5)
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=13
node TextStyle seg=owned gen=1 mask=1 ink=5
```

### "This" mirrors constructors: it is the class's one parameterless defaults rule, so declaring parameters on it is a compile error.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(type + Fancy (is Box)
  (using TextStyle)
  (rule This
    (param on Bool)
    (= ink 13)
  )
)
```

```output
Exception: Style rule "This" must be parameterless
<no file:25>
      (rule This
      ^^^^^^^^^
```

### A "when" state branch inside a "This" rule contributes its declarations only while the tracked condition is true, reverting to the rule's base declarations when it clears.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo)
    (<> Toggle (id: t))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot t))
  (blank)
  (= (. t is-on) true)
  (do-new
    (print (call style-computed-snapshot t))
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (= (. t is-on) false)
  (do-new
    (print (call style-computed-snapshot t))
    (call (. done post))
  )
  (call (. done wait))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(type + Toggle (is Box)
  (using TextStyle)
  (prop + is-on _ (= false))
  (blank)
  (rule This
    (= ink 9)
    (when-style #is-on
      (= ink 11)
    )
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=9
node TextStyle seg=owned gen=2 mask=1 ink=11
node TextStyle seg=owned gen=3 mask=1 ink=9
```

### A negated "when" state branch contributes its declarations only while the tracked condition is false.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo)
    (<> Toggle (id: t))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot t))
  (blank)
  (= (. t is-on) true)
  (do-new
    (print (call style-computed-snapshot t))
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (= (. t is-on) false)
  (do-new
    (print (call style-computed-snapshot t))
    (call (. done post))
  )
  (call (. done wait))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(type + Toggle (is Box)
  (using TextStyle)
  (prop + is-on _ (= false))
  (blank)
  (rule This
    (= ink 9)
    (when-style (not #is-on)
      (= ink 11)
    )
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=11
node TextStyle seg=owned gen=2 mask=1 ink=9
node TextStyle seg=owned gen=3 mask=1 ink=11
```

### Gated blocks from the class "This" rule and from a markup-applied rule on the same object use distinct block bits: toggling one gate never disturbs the other.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo)
    (<> Toggle (id: t) (• glow))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot t))
  (blank)
  (= (. t is-on) true)
  (do-new
    (print (call style-computed-snapshot t))
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (= (. t is-lit) true)
  (do-new
    (print (call style-computed-snapshot t))
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (= (. t is-on) false)
  (do-new
    (print (call style-computed-snapshot t))
    (call (. done post))
  )
  (call (. done wait))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
  (prop + tone I64 (= 3))
)

(type + Toggle (is Box)
  (using TextStyle)
  (prop + is-on _ (= false))
  (prop + is-lit _ (= false))
  (blank)
  (rule This
    (= ink 9)
    (when-style #is-on
      (= ink 11)
    )
  )
  (blank)
  (rule glow
    (= tone 4)
    (when-style #is-lit
      (= tone 5)
    )
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=3 ink=9 tone=4
node TextStyle seg=owned gen=2 mask=1 ink=11 tone=4
node TextStyle seg=owned gen=3 mask=2 ink=11 tone=5
node TextStyle seg=owned gen=4 mask=1 ink=9 tone=5
```

### Root <This> style declarations apply at the defaults layer. They override a legacy "This" defaults rule for the same class, while rule and inline declarations still win.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Fancy (id: plain))
    (<> Fancy (id: styled) (• accent)
      (= ink 2)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot plain))
  (print (call style-computed-snapshot styled))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
  (prop + bold Bool (= false))
)

(type + Fancy (is Box)
  (using TextStyle)
  (rule This
    (= ink 9)
  )
  (blank)
  (rule accent
    (= ink 5)
  )
  (content
    (<> This
      (= ink 13)
      (= bold true)
    )
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=3 ink=13 bold=1
node TextStyle seg=owned gen=1 mask=3 ink=2 bold=1
```

### Attributes authored on a node inside component content override that node's own class defaults; they sit on the content layer between class defaults and style-sheet rules.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel)
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let badge (!! (. (!! (. demo %first-child)) %first-child)))
  (print (call style-authored-snapshot badge))
  (print (call style-computed-snapshot badge))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + bold _ (= false))
)

(type + Badge (is Box)
  (using TextStyle)
  (rule This
    (= bold true)
    (= ink 2)
  )
)

(type + Panel (is Box)
  (content
    (<> This
      (<> Badge
        (= ink 5)
      )
    )
  )
)
```

```output
node TextStyle.bold layer=0 order=65536 kind=static bits=1
node TextStyle.ink layer=0 order=65537 kind=static bits=2
node TextStyle.ink layer=1 order=0 kind=static bits=5
node TextStyle seg=owned gen=1 mask=3 ink=5 bold=1
```

### A class's rule `• This` that applies a rule of a style sheet and declares a style under a `when` follows the state the `when` names.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot panel))
  (= (. panel shown) false)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot panel))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(style AppTheme
  (using TextStyle)
  (rule primary
    (= ink 11)
  )
)

(type + Panel (is Box)
  (using TextStyle)
  (prop + shown _ (= true))
  (blank)
  (rule This (• primary)
    (when-style #shown
      (= ink 5)
    )
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=5
node TextStyle seg=owned gen=2 mask=1 ink=11
```

### Rules written under a `when` of a class's rule `• This`, which applies a rule itself, style the class's markup while the `when`'s state holds.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot panel))
  (print (call style-computed-snapshot (. panel lab)))
  (= (. panel shown) false)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot panel))
  (print (call style-computed-snapshot (. panel lab)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(style AppTheme
  (using TextStyle)
  (rule primary
    (= ink 11)
  )
  (blank)
  (rule label-text
    (= ink 2)
  )
)

(type + Panel (is Box)
  (using TextStyle)
  (prop + shown _ (= true))
  (blank)
  (rule This (• primary)
    (when-style #shown
      (rule label-text (• primary))
    )
  )
  (content
    (<> This
      (<> Box (id: lab) (• label-text))
    )
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=11
node TextStyle seg=owned gen=1 mask=1 ink=11
node TextStyle seg=owned gen=1 mask=1 ink=11
node TextStyle seg=owned gen=2 mask=1 ink=2
```

### A "This" default whose value fails to convert lets the error out of the constructor, and one that reads something tracked is declared again as it changes

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)
(prop _ failing _ (= false))
(blank)

(main
  (print (call made))
  (= failing true)
  (print (call made))
  (= failing false)
  (blank)
  (<> BatchDemo (id: demo)
    (<> Shaded (id: shaded))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (. shaded $shade level))
  (= (. shaded dark) false)
  (call (. demo markup-batch-queue checkpoint))
  (print (. shaded $shade level))
)

(fn-do + made String
  (let made (catch (call Shaded)))
  (return
    (match made
      (case Shaded
        "made"
      )
      (case (: error SyntaxError)
        (. error message)
      )
    )
  )
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Shade (is Tuple)
  (prop + level I64)
  (fn-from-expr + (: value String)
    (match value
      (~ (seq "dark" (! .))
        (call This 2)
      )
      (~ (seq "light" (! .))
        (call This 1)
      )
      (throw (call SyntaxError "Invalid shade"))
    )
  )
)

(type + ShadeStyle (is Style)
  (prop + shade _ (= (call Shade 0)))
)

(type + Shaded (is Box)
  (using ShadeStyle)
  (prop + dark _ (= true))
  (blank)
  (rule This
    (= shade (if-expr failing "bogus" (if-expr #dark "dark" "light")))
  )
)
```

```output
made
Invalid shade
2
1
```

### Errors in the defaults a span's class writes in another module are reported where they are written, though the module that makes the span is larger

```brix crash
(import ./lib/card)
(import /gui)

(main
  (let numbers (list 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30
    31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60))
  (print (. numbers length))
  (<> Text
    (<> Card)
  )
)
```

```brix file=lib/card.bx
(import /gui)

(type + Card (is Span)
  (prop + is-bold _ (= true))
  (rule This
    (when-style #is-bold
      (= font-weight (. FontWeight bold))
    )
  )
  (content
    (<> This
      (= color (call shade this))
    )
  )
)

(fn-expr _ shade RGBA
  (param span Card)
  (. span $gui-syntax-plain-color)
)
```

```output
Exception: Type "FontWeight" not found; it is declared in /graphics/text, which this module does not import. Add the import /graphics/text
<lib/card.bx:7>
          (= font-weight (. FontWeight bold))
                            ^^^^^^^^^^

Exception: Type "RGBA" not found; it is declared in /graphics/color, which this module does not import. Add the import /graphics/color
<lib/card.bx:17>
    (fn-expr _ shade RGBA
                     ^^^^
```
