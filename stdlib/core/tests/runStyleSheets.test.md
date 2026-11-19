# Style sheets

### StyleSheet slots resolve statically in markup style declarations and read late-bound values: without a provider the style-sheet's default record supplies initial values, installing a provider record changes the subtree's computed values, and slot writes propagate on the next settle, including through inherited flow-through aliasing.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink bright)
      (<> Plain)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let label (!! (. demo %first-child)))
  (let plain (!! (. label %first-child)))
  (blank)
  (print (call style-authored-snapshot label))
  (print (call style-computed-snapshot label))
  (print (call style-sheet-snapshot label "AppTheme"))
  (blank)
  (let theme (call StyleSheetRecord (!! (call style-sheet-info-named "AppTheme"))))
  (call (. theme set-slot-named) "bright" 42 false)
  (call style-provide-style-sheet demo theme)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-computed-snapshot label))
  (print (call style-sheet-snapshot label "AppTheme"))
  (blank)
  (call (. theme set-slot-named) "bright" 50 false)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-computed-snapshot label))
  (print (call style-computed-snapshot plain))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style AppTheme
  (prop + bright I64 (= 11))
)

(type + Label (is Box)
  (using TextStyle)
)

(type + Plain (is Box))
```

```output
node TextStyle.ink layer=5 order=0 kind=style-sheet bits=0 style-sheet=AppTheme@0
node TextStyle seg=owned gen=1 mask=1 ink=11
style-sheet AppTheme record=1 version=0 bright=11
node TextStyle seg=owned gen=2 mask=1 ink=42
style-sheet AppTheme record=2 version=1 bright=42
node TextStyle seg=owned gen=3 mask=1 ink=50
node TextStyle seg=parent gen=3 mask=1 ink=50
```

```brix crash
(style AppTheme
  (prop + accent I64 (= 1))
)

(style-extend AppTheme
  (prop + highlight I64 (= 2))
)
```

```output
Exception: StyleSheet extensions may not declare slots
<no file:7>
      (prop + highlight I64 (= 2))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(style AppTheme
  (rule primary
    (= value 1)
  )
)

(style-extend AppTheme
  (rule primary
    (= value 2)
  )
)
```

```output
Exception: Redefinition of style rule
<no file:9>
      (rule primary
      ^^^^^^^^^^^^
```

```brix crash
(type + Plain)

(style-extend Plain
  (prop + accent I64 (= 2))
)
```

```output
Exception: StyleSheet extension target "Plain" is not a StyleSheet
<no file:4>
    (style-extend Plain
    ^^^^^^^^^^^^^^^^^^
```

### A rule written in a rule of a style sheet is typed with the rule around it, though nothing applies it: its value names a case the property's union does not have.

```brix compile-error
(import /core/style)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Thing)
)

(type + Shape (is Union)
  (variant round)
  (variant square)
)

(type + ShapeStyle (is Style)
  (prop + shape Shape (= (. Shape round)))
  (prop + size I64 (= 0))
)

(style AppTheme
  (using ShapeStyle)
  (rule Thing
    (rule part
      (= shape (. Shape missing))
    )
  )
)

(type + Thing (is Box)
  (using AppTheme)
)
```

```output
Exception: Shape has no case "missing"
<no file:24>
          (= shape (. Shape missing))
                   ^^^^^^^^^^^^^^^^^
```

### A rule written in a rule of a style sheet is typed with the rule around it, though nothing applies it: its value is of a type the property does not convert from.

```brix compile-error
(import /core/style)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Thing)
)

(type + Shape (is Union)
  (variant round)
  (variant square)
)

(type + ShapeStyle (is Style)
  (prop + shape Shape (= (. Shape round)))
  (prop + size I64 (= 0))
)

(style AppTheme
  (using ShapeStyle)
  (rule Thing
    (rule part
      (= size (. Shape round))
    )
  )
)

(type + Thing (is Box)
  (using AppTheme)
)
```

```output
Exception: Expected I64 here, but this is Shape, and there is no conversion between them
<no file:24>
          (= size (. Shape round))
          ^^^^^^^^^^^^^^^^^^^^^^^^
```

### A rule written in a rule of a style sheet reads, in an expression, a slot of a style sheet the style sheet uses.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Thing (id: thing))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (. thing part $size))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + ShapeStyle (is Style)
  (prop + size I64 (= 0))
)

(style Metrics
  (prop + height I64 (= 24))
)

(style AppTheme
  (using ShapeStyle)
  (using Metrics)
  (rule Thing
    (rule part
      (= size (- height 2))
    )
  )
)

(type + Part (is Box)
  (using ShapeStyle)
)

(type + Thing (is Box)
  (using AppTheme)
  (content
    (<> This
      (<> Part (id: part))
    )
  )
)
```

```output
22
```

### Unmounting a style-sheet provider removes its scalar owner identity from the record's provider list. Fresh owners may reuse the released logical IDs without inheriting its generation.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Holder (id: provider)
      (<> Label
        (= ink bright)
      )
    )
  )
  (blank)
  (let theme (call StyleSheetRecord (!! (call style-sheet-info-named "AppTheme"))))
  (call style-provide-style-sheet provider theme)
  (print (str "providers=" (. theme providers length)))
  (blank)
  (call markup-remove demo provider)
  (print (str "after-unmount=" (. theme providers length)))
  (blank)
  (let first (call Holder))
  (let second (call Holder))
  (call markup-append demo first)
  (call markup-append demo second)
  (print (call style-attachment-snapshot first))
  (print (call style-attachment-snapshot second))
  (blank)
  (call (. theme set-slot-named) "bright" 22 false)
  (print (str "after-change=" (. theme providers length)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(style AppTheme
  (prop + bright I64 (= 11))
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(type + Label (is Box)
  (using TextStyle)
)

(type + Holder (is Box))
```

```output
providers=1
after-unmount=0
live id=3 root=1 parent=1 node=- gen=0 dirty=SI
live id=2 root=1 parent=1 node=- gen=0 dirty=SI
after-change=0
```

### Moving under a different style-sheet provider (unmount + remount) re-derives style-sheet-backed declarations against the new effective style-sheet, and slot writes are equality-gated: an unchanged value raises no pending domain bits.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Holder (id: pa))
    (<> Holder (id: pb))
    (<> Label
      (= ink bright)
    )
  )
  (blank)
  (let child (!! (. pb %next-sibling)))
  (blank)
  (let theme-a (call StyleSheetRecord (!! (call style-sheet-info-named "AppTheme"))))
  (call (. theme-a set-slot-named) "bright" 100 false)
  (call style-provide-style-sheet pa theme-a)
  (blank)
  (let theme-b (call StyleSheetRecord (!! (call style-sheet-info-named "AppTheme"))))
  (call (. theme-b set-slot-named) "bright" 200 false)
  (call style-provide-style-sheet pb theme-b)
  (blank)
  (call markup-remove demo child)
  (call markup-append pa child)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot child))
  (blank)
  (call markup-remove pa child)
  (call markup-append pb child)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot child))
  (blank)
  (= (. child $is-paint-invalid) false)
  (blank)
  ; Clear the bits the containers raised imperatively while the child was moved between parents.
  (= (. demo $is-paint-invalid) false)
  (= (. demo $is-layout-invalid) false)
  (= (. pa $is-paint-invalid) false)
  (= (. pa $is-layout-invalid) false)
  (= (. pb $is-paint-invalid) false)
  (= (. pb $is-layout-invalid) false)
  (blank)
  (call (. theme-b set-slot-named) "bright" 200 false)
  (call (. demo markup-batch-queue checkpoint))
  (print (+ "equal " (call style-pending-snapshot demo)))
  (blank)
  (call (. theme-b set-slot-named) "bright" 250 false)
  (call (. demo markup-batch-queue checkpoint))
  (print (+ "changed " (call style-pending-snapshot demo)))
  (print (call style-computed-snapshot child))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style AppTheme
  (prop + bright I64 (= 11))
)

(type + Holder (is Box))

(type + Label (is Box)
  (using TextStyle)
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=100
node TextStyle seg=owned gen=1 mask=1 ink=200
equal paint=0 layout=0
changed paint=1 layout=0
node TextStyle seg=owned gen=2 mask=1 ink=250
```

### A bare name in a style declaration that names both a StyleSheet slot and a lexical value is a diagnosed ambiguity, never a silent choice.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let bright 5)
  (blank)
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink bright)
    )
  )
  (blank)
  (call print bright)
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
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
  (prop + bright I64 (= 11))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
Exception: "bright" is ambiguous: it names both a StyleSheet slot of AppTheme and a lexical value
<no file:12>
          (= ink bright)
                 ^^^^^^
```

### A StyleSheet slot must be convertible to the style property's declared value type.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink bright)
    )
  )
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
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
  (prop + bright RGBA (= (call RGBA 1 0 0)))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
Exception: StyleSheet slot "bright" has type RGBA, which cannot be converted to style property "ink" type I64
<no file:10>
          (= ink bright)
                 ^^^^^^
```

### A normally convertible StyleSheet slot is still rejected if it would require decoded typed computation during settle instead of a zero-allocation bit projection.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink tiny)
    )
  )
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
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
  (prop + tiny U8 (= 3))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
Exception: StyleSheet slot "tiny" can convert to I64, but not as a zero-allocation style-sheet projection; declare the slot as I64 or assign a concrete value.
<no file:10>
          (= ink tiny)
                 ^^^^
```

### StyleSheet slots use normal compile-time coercion when assigned to TUI background-color: inferred AnsiColor and optional AnsiColor slots project into BackgroundColor? while staying late-bound to the effective provider record. Painter slots project separately into background.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import /core/tests/support/markup-colors)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Box (id: panel)
      (= background-color accent)
    )
    (<> Box (id: nullable)
      (= background-color maybe)
    )
    (<> Box (id: rgb)
      (= background-color glow)
    )
    (<> Box (id: hsv)
      (= background-color wash)
    )
    (<> Box (id: painted)
      (= background custom)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-authored-snapshot panel))
  (print (call background-color-label (. panel $background-color)))
  (print (call background-color-label (. nullable $background-color)))
  (print (call background-color-label (. rgb $background-color)))
  (print (call background-color-label (. hsv $background-color)))
  (print (call painter-label (. painted $background)))
  (blank)
  (let theme (call StyleSheetRecord (!! (call style-sheet-info-named "AppTheme"))))
  (call (. theme set-slot-named) "accent" 12 false)
  (call (. theme set-slot-named) "maybe" 9 false)
  (call style-provide-style-sheet demo theme)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call background-color-label (. panel $background-color)))
  (print (call background-color-label (. nullable $background-color)))
  (blank)
  (call (. theme set-slot-named) "maybe" 0 true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call background-color-label (. nullable $background-color)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(style AppTheme
  (prop + accent _ (= grey))
  (prop + maybe (? AnsiColor) (= null))
  (prop + glow RGBA (= (call RGBA 0.25 0.5 0.75 1)))
  (prop + wash HSVA (= (call HSVA 0.5 0.25 0.75 1)))
  (prop + custom Painter (= (call TestBackgroundPainter aqua)))
)

(type + TestBackgroundPainter (is Painter)
  (prop + color AnsiColor)
  (fn-do + This _
    (param color _)
  )
  (fn-expr + paint _
    (param node Box)
    (param rect Rect)
    (param canvas Canvas)
  )
)

(fn-do _ background-color-label String
  (param background (? BackgroundColor))
  (return-if "null" (if (not background)))
  (blank)
  (let live background)
  (return
    (match live
      (case (call .ansi color)
        (str "ansi(" color ")")
      )
      (case (call .rgba color)
        (str "rgba(" (as (* (. color red) 100) I64) " " (as (* (. color green) 100) I64) " " (as (* (. color blue) 100) I64) " " (as (* (. color alpha) 100) I64) ")")
      )
      (case (call .hsva color)
        (str "hsva(" (as (* (. color hue) 100) I64) " " (as (* (. color saturation) 100) I64) " " (as (* (. color value) 100) I64) " " (as (* (. color alpha) 100) I64) ")")
      )
      (default
        "other"
      )
    )
  )
)

(fn-expr _ painter-label String
  (param painter (? Painter))
  (case? painter
    "painter"
  )
  (default
    "null"
  )
)
```

```output
node BoxPaint.background-color layer=5 order=0 kind=style-sheet-projected bits=0 style-sheet=AppTheme@0
ansi(8)
null
rgba(25 50 75 100)
hsva(50 25 75 100)
painter
ansi(12)
ansi(9)
null
```

### Two distinct visible style sheets declaring the same slot name is a diagnosed ambiguity.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink bright)
    )
  )
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using AppTheme)
  (using OtherTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(style AppTheme
  (prop + bright I64 (= 11))
)

(style OtherTheme
  (prop + bright I64 (= 12))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
Exception: StyleSheet slot "bright" is ambiguous between visible style sheets AppTheme and OtherTheme
<no file:10>
          (= ink bright)
          ^^^^^^^^^^^^^^
```

### Ordinary expressions and method bodies do not gain style-sheet fallback lookup: slot names are only meaningful inside managed style authoring contexts.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo))
  (blank)
  (call print (call (. demo ordinary)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (fn-expr + ordinary I64
    bright
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(style AppTheme
  (prop + bright I64 (= 11))
)
```

```output
Exception: Name "bright" not found
<no file:22>
        bright
        ^^^^^^
```

### A StyleSheet can provide style rules: `using AppTheme` on a class makes the rule applicable to markup anywhere under that class, the rule's declarations resolve property names through the style-sheet's own style usings, and slot references stay late-bound through the effective style-sheet.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Plain
      (<> Plain (id: inner) (• primary))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot inner))
  (blank)
  (let theme (call StyleSheetRecord (!! (call style-sheet-info-named "AppTheme"))))
  (call (. theme set-slot-named) "accent" 42 false)
  (call style-provide-style-sheet demo theme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot inner))
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
  (prop + accent I64 (= 11))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(type + Plain (is Box))
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=11
node TextStyle seg=owned gen=2 mask=1 ink=42
```

### StyleSheet-provided style rules can also project a differently typed slot into a style property. The rule declaration is static, but the slot value remains late-bound through the effective provider record.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import /core/tests/support/markup-colors)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Box (id: panel) (• primary))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call background-label (. panel $background-color)))
  (blank)
  (let theme (call StyleSheetRecord (!! (call style-sheet-info-named "AppTheme"))))
  (call (. theme set-slot-named) "accent" 12 false)
  (call style-provide-style-sheet demo theme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call background-label (. panel $background-color)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(style AppTheme
  (using BoxPaint)
  (prop + accent _ (= grey))
  (blank)
  (rule primary
    (= background-color accent)
  )
)

(fn-do _ background-label String
  (param background (? BackgroundColor))
  (return-if "null" (if (not background)))
  (blank)
  (let live background)
  (return
    (match live
      (case (call .ansi color)
        (str "ansi(" color ")")
      )
      (default
        "other"
      )
    )
  )
)
```

```output
ansi(8)
ansi(12)
```

### StyleSheet-provided rules stay visible through nested markup bodies even when an intermediate component class does not name the style sheet.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (• primary)
      (<> Label (id: inner) (• primary))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot inner))
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
  (prop + accent I64 (= 11))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(type + Panel (is Box))

(type + Label (is Box))
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=11
```

### StyleSheet-provided rules stay visible through component-owned markup content when the component is instantiated under a themed parent.

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
  (let label (!! (. panel %first-child)))
  (print (call style-computed-snapshot label))
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
  (prop + accent I64 (= 11))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(type + Panel (is Box)
  (content
    (<> Label (• primary))
  )
)

(type + Label (is Box))
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=11
```

### StyleSheet-provided rules override style attributes authored inside component content.

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
  (let label (!! (. panel %first-child)))
  (print (call style-authored-snapshot label))
  (print (call style-computed-snapshot label))
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
  (prop + accent I64 (= 11))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(type + Panel (is Box)
  (content
    (<> Label (• primary)
      (= ink 3)
    )
  )
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
node TextStyle.ink layer=2 order=0 kind=style-sheet bits=0 style-sheet=AppTheme@0
node TextStyle.ink layer=1 order=1 kind=static bits=3
node TextStyle seg=owned gen=1 mask=1 ink=11
```

### Call-site style attributes on component instances remain inline and override StyleSheet rules.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (id: label) (• primary)
      (= ink 3)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-authored-snapshot label))
  (print (call style-computed-snapshot label))
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
  (prop + accent I64 (= 11))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
node TextStyle.ink layer=2 order=0 kind=style-sheet bits=0 style-sheet=AppTheme@0
node TextStyle.ink layer=5 order=1 kind=static bits=3
node TextStyle seg=owned gen=1 mask=1 ink=3
```

### Root <This> style attributes are component defaults that StyleSheet rules can override.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• primary))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-authored-snapshot panel))
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
  (prop + accent I64 (= 11))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(type + Panel (is Box)
  (using TextStyle)
  (content
    (<> This
      (= ink 3)
    )
  )
)
```

```output
node TextStyle.ink layer=0 order=65536 kind=static bits=3
node TextStyle.ink layer=2 order=0 kind=style-sheet bits=0 style-sheet=AppTheme@0
node TextStyle seg=owned gen=1 mask=1 ink=11
```

### Nested StyleSheet rules apply to a class a component writes on its root <This> when the component is made inside the rule's scope, over the sheet's own rule for that class.

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
  (print (call style-computed-snapshot (!! (. panel %first-child))))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using BaseTheme)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(style BaseTheme
  (using TextStyle)
  (rule inner
    (= ink 5)
  )
)

(style AppTheme
  (using TextStyle)
  (rule outer
    (rule inner
      (= ink 21)
    )
  )
)

(type + Panel (is Box)
  (content
    (<> This (• outer)
      (<> Comp)
    )
  )
)

(type + Comp (is Box)
  (using TextStyle)
  (content
    (<> This (• inner))
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=21
```

### A rule a class gets by name from a style sheet its own chain uses is one of its defaults: the classes a subclass writes on its root <This> override it, ungated and under a "when".

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Holder (id: holder))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (!! (. holder %first-child))))
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
  (prop + mark I64 (= 7))
)

(style BaseTheme
  (using TextStyle)
  (rule Bar
    (= ink 5)
    (when-style #flag
      (= mark 5)
    )
  )
)

(style AppTheme
  (using BaseTheme)
  (rule sub
    (= ink 21)
    (when-style #flag
      (= mark 21)
    )
  )
)

(type + Bar (is Box)
  (using BaseTheme)
  (using TextStyle)
  (prop + flag _ (= false))
)

(type + Sub (is Bar)
  (content
    (<> This (• sub))
  )
)

(type + Holder (is Box)
  (content
    (<> This
      (<> Sub
        (= flag true)
      )
    )
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=3 ink=21 mark=21
```

### Slot builders preserve whether their content was authored at a call site or inside component content.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> SlotPanel (id: caller)
      (<> Label (slot: header)
        (= ink 3)
      )
    )
    (<> DerivedPanel (id: authored))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let caller-label (!! (. caller %first-child)))
  (let authored-label (!! (. authored %first-child)))
  (print (call style-authored-snapshot caller-label))
  (print (call style-computed-snapshot caller-label))
  (print (call style-authored-snapshot authored-label))
  (print (call style-computed-snapshot authored-label))
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
  (prop + accent I64 (= 11))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(type + SlotPanel (is Box)
  (content
    (+<> Label (id: header) (• primary))
  )
)

(type + DerivedPanel (is SlotPanel)
  (content
    (<> Label (slot: header)
      (= ink 3)
    )
  )
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
node TextStyle.ink layer=5 order=0 kind=static bits=3
node TextStyle.ink layer=2 order=0 kind=style-sheet bits=0 style-sheet=AppTheme@0
node TextStyle seg=owned gen=1 mask=1 ink=3
node TextStyle.ink layer=1 order=0 kind=static bits=3
node TextStyle.ink layer=2 order=0 kind=style-sheet bits=0 style-sheet=AppTheme@0
node TextStyle seg=owned gen=1 mask=1 ink=11
```

### Nested StyleSheet rules under implicit base type selectors can refine explicit component rules.

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
  (let button (!! (. panel %first-child)))
  (print (call style-authored-snapshot button))
  (print (call style-computed-snapshot button))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using BaseTheme)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 0))
)

(style BaseTheme
  (using TextStyle)
  (rule accent
    (= ink 5)
  )
)

(style AppTheme
  (using TextStyle)
  (rule Button
    (rule accent
      (= ink 21)
    )
  )
)

(type + Panel (is Box)
  (content
    (<> SpecialButton (• accent))
  )
)

(type + Button (is Box)
  (using TextStyle)
)

(type + SpecialButton (is Button))
```

```output
node TextStyle.ink layer=2 order=0 kind=static bits=5
node TextStyle.ink layer=2 order=1 kind=static bits=21
node TextStyle seg=owned gen=1 mask=1 ink=21
```

### StyleSheet owner type selectors apply to root <This> component content.

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
  (let segment (!! (. panel %first-child)))
  (print (call style-authored-snapshot segment))
  (print (call style-computed-snapshot segment))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using BaseTheme)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 0))
)

(style BaseTheme
  (using TextStyle)
  (rule part
    (= ink 5)
  )
)

(style AppTheme
  (using TextStyle)
  (rule Panel
    (rule part
      (= ink 21)
    )
  )
)

(type + Panel (is Box)
  (content
    (<> This
      (<> Segment (• part))
    )
  )
)

(type + Segment (is Box)
  (using TextStyle)
)
```

```output
node TextStyle.ink layer=2 order=0 kind=static bits=5
node TextStyle.ink layer=2 order=1 kind=static bits=21
node TextStyle seg=owned gen=1 mask=1 ink=21
```

### Constructor-built component content sees the class's own StyleSheet usings when resolving slot outlet style rules.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo))
  (blank)
  (let panel (call DerivedPanel))
  (call markup-append demo panel)
  (call (. demo markup-batch-queue checkpoint))
  (let segment (!! (. panel %first-child)))
  (print (call style-authored-snapshot segment))
  (print (call style-computed-snapshot segment))
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
)

(style AppTheme
  (using TextStyle)
  (rule accent
    (= ink 21)
  )
)

(type + BasePanel (is Box)
  (using AppTheme)
  (content
    (<> This
      (+<> Segment (id: body) (• accent)
        (<> Segment)
      )
    )
  )
)

(type + DerivedPanel (is BasePanel)
  (content
    (<> Segment (slot: body))
  )
)

(type + Segment (is Box)
  (using TextStyle)
)
```

```output
node TextStyle.ink layer=2 order=0 kind=static bits=21
node TextStyle seg=owned gen=1 mask=1 ink=21
```

### Nested StyleSheet owner type selectors scope inherited slot outlet rules.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call DerivedPanel))
  (<> BatchDemo (id: demo)
    (<> Plain)
  )
  (blank)
  (call markup-append demo panel)
  (call (. demo markup-batch-queue checkpoint))
  (let segment (!! (. panel %first-child)))
  (print (call style-authored-snapshot segment))
  (print (call style-computed-snapshot segment))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Plain (is Box))

(type + TextStyle (is Style)
  (prop + ink I64 (= 0))
)

(style AppTheme
  (using TextStyle)
  (rule BasePanel
    (rule part
      (= ink 21)
    )
  )
)

(type + BasePanel (is Box)
  (using AppTheme)
  (content
    (<> This
      (+<> Segment (id: body) (• part)
        (<> Segment)
      )
    )
  )
)

(type + DerivedPanel (is BasePanel)
  (content
    (<> Segment (slot: body))
  )
)

(type + Segment (is Box)
  (using TextStyle)
)
```

```output
node TextStyle.ink layer=2 order=0 kind=static bits=21
node TextStyle seg=owned gen=1 mask=1 ink=21
```

### Nested StyleSheet rules scope component-owned markup content.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• themed))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let segment (!! (. panel %first-child)))
  (print (call style-authored-snapshot segment))
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
  (rule themed
    (rule part
      (= ink 21)
    )
  )
)

(type + Panel (is Box)
  (rule This
    (<> Segment (• part))
  )
)

(type + Segment (is Box)
  (using TextStyle)
)
```

```output
node TextStyle.ink layer=2 order=0 kind=static bits=21
```

### Nested StyleSheet rules reject markup content that is not generated style content.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• themed))
  )
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(style AppTheme
  (rule themed
    (rule part
      (<> Segment)
    )
  )
)

(type + Panel (is Box)
  (rule This
    (<> Segment (• part))
  )
)

(type + Segment (is Box))
```

```output
Exception: StyleSheet style rules may only contain style declarations and generated content
<no file:25>
          (<> Segment)
          ^^^^^^^^^^^^
```

### StyleSheet rules attach generated style content without a state condition.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• themed))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let generated (!! (. panel %first-child)))
  (blank)
  (print (call markup-is-style-shadow generated))
  (print (== (as (!! (call markup-style-parent-host generated)) Pointer) (as panel Pointer)))
  (print (call style-authored-snapshot generated))
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
  (rule themed
    (<> Segment (slot: first-child) (• part))
  )
  (blank)
  (rule part
    (= ink 21)
  )
)

(type + Panel (is Box))

(type + Segment (is Box)
  (using TextStyle)
)
```

```output
true
true
node TextStyle.ink layer=2 order=0 kind=static bits=21
```

### Nested StyleSheet rules matched by type attach generated style content inside a component.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• themed))
    (<> Panel (id: plain))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let generated (!! (. panel segment %next-sibling)))
  (blank)
  (print (call markup-is-style-shadow generated))
  (print (== (as (!! (call markup-style-parent-host generated)) Pointer) (as (. panel segment) Pointer)))
  (print (call style-authored-snapshot generated))
  (print (== (. plain segment %next-sibling) null))
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
  (rule themed
    (rule Segment
      (<> Mark (slot: after) (• part))
    )
  )
  (blank)
  (rule part
    (= ink 21)
  )
)

(type + Panel (is Box)
  (content
    (<> This
      (<> Segment (id: segment))
    )
  )
)

(type + Segment (is Box))

(type + Mark (is Box)
  (using TextStyle)
)
```

```output
true
true
node TextStyle.ink layer=2 order=0 kind=static bits=21
true
```

### Generated style content does not receive generated content from the rule that produced it.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Segment (id: segment))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let generated (!! (. segment %first-child)))
  (blank)
  (print (call markup-is-style-shadow generated))
  (print (call style-authored-snapshot generated))
  (print (== (. generated %first-child) null))
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
  (rule Segment
    (= ink 21)
    (blank)
    (<> Segment (slot: first-child))
  )
)

(type + Segment (is Box)
  (using TextStyle)
)
```

```output
true
node TextStyle.ink layer=2 order=0 kind=static bits=21
true
```

### Nested rules inside "when" state branches of a type-matched StyleSheet rule follow the matched element's state, including the negated branch.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel)
      (<> Segment (• part))
    )
    (<> Panel (id: shut)
      (= is-open false)
      (<> Segment (• part))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (!! (. panel %first-child))))
  (print (call style-computed-snapshot (!! (. shut %first-child))))
  (blank)
  (= (. panel is-open) false)
  (= (. shut is-open) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (!! (. panel %first-child))))
  (print (call style-computed-snapshot (!! (. shut %first-child))))
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
  (rule Panel
    (when-style #is-open
      (rule part
        (= ink 21)
      )
    )
    (blank)
    (when-style (not #is-open)
      (rule part
        (= ink 9)
      )
    )
  )
)

(type + Panel (is Box)
  (prop + is-open _ (= true))
)

(type + Segment (is Box)
  (using TextStyle)
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=21
node TextStyle seg=owned gen=1 mask=1 ink=9
node TextStyle seg=owned gen=2 mask=1 ink=9
node TextStyle seg=owned gen=2 mask=1 ink=21
```

### Nested rules inside a "when" state branch of a type-matched StyleSheet rule apply once to a component's own content.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let generated (!! (. panel inner %first-child)))
  (print (call markup-is-style-shadow generated))
  (print (== (. generated %next-sibling) null))
  (blank)
  (= (. panel is-open) false)
  (call (. demo markup-batch-queue checkpoint))
  (print (== (. panel inner %first-child) null))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(style AppTheme
  (rule Panel
    (when-style #is-open
      (rule part
        (<> Segment (slot: first-child))
      )
    )
  )
)

(type + Panel (is Box)
  (prop + is-open _ (= true))
  (content
    (<> This
      (<> Segment (id: inner) (• part))
    )
  )
)

(type + Segment (is Box))
```

```output
true
true
true
```

### A component with markup content is specialized once per distinct visible style-sheet context.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> ThemeABox
      (<> Panel (id: panel-a))
    )
    (<> ThemeBBox
      (<> Panel (id: panel-b))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let label-a (!! (. panel-a %first-child)))
  (let label-b (!! (. panel-b %first-child)))
  (print (call style-computed-snapshot label-a))
  (print (call style-computed-snapshot label-b))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + ThemeABox (is Box)
  (using AppTheme)
)

(type + ThemeBBox (is Box)
  (using OtherTheme)
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(style AppTheme
  (using TextStyle)
  (prop + accent I64 (= 11))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(style OtherTheme
  (using TextStyle)
  (prop + accent I64 (= 22))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(type + Panel (is Box)
  (content
    (<> Label (• primary))
  )
)

(type + Label (is Box))
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=11
node TextStyle seg=owned gen=1 mask=1 ink=22
```

### A themed component-content specialization is not reused for an unthemed instantiation.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> ThemeBox
      (<> Panel (id: themed))
    )
    (<> Panel (id: unthemed))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let themed-label (!! (. themed %first-child)))
  (let unthemed-label (!! (. unthemed %first-child)))
  (print (call style-computed-snapshot themed-label))
  (print (call style-computed-snapshot unthemed-label))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + ThemeBox (is Box)
  (using AppTheme)
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(style AppTheme
  (using TextStyle)
  (prop + accent I64 (= 11))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(type + Panel (is Box)
  (content
    (<> Label (• primary))
  )
)

(type + Label (is Box))
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=11
initial
```

### Ambiguous visible style sheets are still diagnosed inside recursively instantiated component content.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> AmbiguousBox
      (<> Panel)
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

(type + AmbiguousBox (is Box)
  (using AppTheme)
  (using OtherTheme)
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(style AppTheme
  (using TextStyle)
  (prop + accent I64 (= 11))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(style OtherTheme
  (using TextStyle)
  (prop + accent I64 (= 22))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(type + Panel (is Box)
  (content
    (<> Label (• primary))
  )
)

(type + Label (is Box))
```

```output
Exception: Style rule "primary" is ambiguous between visible style sheets AppTheme and OtherTheme
<no file:52>
        (<> Label (• primary))
                  ^^^^^^^
```

### A class-local style rule shadows a style-sheet-provided rule with the same matching signature, while overloads with different signatures still resolve across providers.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (id: a) (• primary))
    (<> Label (id: b) (• primary 9))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot a))
  (print (call style-computed-snapshot b))
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
  (prop + accent I64 (= 11))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(type + Label (is Box)
  (using TextStyle)
  (rule primary
    (= ink 5)
  )
  (blank)
  (rule primary
    (param v I64)
    (= ink v)
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=5
node TextStyle seg=owned gen=1 mask=1 ink=9
```

### Two visible style sheets providing a style rule with the same matching signature is a diagnosed ambiguity.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (• primary))
  )
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using AppTheme)
  (using OtherTheme)
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
  (prop + accent I64 (= 11))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(style OtherTheme
  (using TextStyle)
  (prop + accent I64 (= 12))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
Exception: Style rule "primary" is ambiguous between visible style sheets AppTheme and OtherTheme
<no file:9>
        (<> Label (• primary))
                  ^^^^^^^
```

### Two style sheets a class uses together that both write a rule by its name, one of them in a variant, make its defaults ambiguous. The error is at the class the rules name, even where a subclass is made.

```brix compile-error
(import /core/style)
(blank)

(main
  (let sub (call Sub))
  (print "made")
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(style AppTheme
  (using TextStyle)
  (rule Bar
    (= ink 5)
  )
)

(style MotionTheme
  (using TextStyle)
)

(style CalmTheme (is MotionTheme) (for calm)
  (rule Bar
    (= ink 3)
  )
)

(type + Bar
  (using AppTheme)
  (using MotionTheme)
)

(type + Sub (is Bar))
```

```output
Exception: Style rule "Bar" is ambiguous between visible style sheets AppTheme and MotionTheme
<no file:31>
    (type + Bar
    ^^^
```

### StyleSheets may not declare a "This" defaults rule: defaults belong to classes.

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
  (prop + accent I64 (= 11))
  (blank)
  (rule This
    (= ink accent)
  )
)
```

```output
Exception: StyleSheets may not declare a "This" defaults rule
<no file:28>
      (rule This
      ^^^^^^^^^
```

### StyleSheet declarations auto-import /core/style, so modules can declare styles and style sheets without spelling the core runtime import directly.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (• primary))
  )
  (blank)
  (print "auto-style-sheet")
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
  (prop + accent I64 (= 11))
  (blank)
  (rule primary
    (= ink accent)
  )
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
auto-style-sheet
```

### StyleSheet rule values are evaluated in the rule module's lexical/import scope, not just the markup application scope, so imported enum-like union cases are visible inside theme rules.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import ./theme)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "scoped=" (call style-authored-snapshot panel)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using DemoTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (using DemoStyle)
)
```

```brix file=edge.bx
(type + Edge (is Union)
  (variant none)
  (variant single)
)
```

```brix file=theme.bx
(import /core/style)
(import ./edge)

(type + DemoStyle (is Style)
  (prop + edge Edge (= (. Edge none)))
)

(style DemoTheme
  (using DemoStyle)
  (rule Panel
    (= edge (. Edge single))
  )
)
```

```output
scoped=node DemoStyle.edge layer=2 order=0 kind=static bits=257
```

### Scoped StyleSheet nested rules layer over same-node base StyleSheet rules.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Container (id: container))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let panel (!! (. container %first-child)))
  (print (call style-authored-snapshot panel))
)

(type + AccentStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + pad I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
  (style-domains pad (domain layout))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using DemoTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Container (is Box)
  (content
    (<> This
      (<> Panel)
    )
  )
)

(type + Panel (is Box)
  (using AccentStyle)
)

(style DemoTheme
  (using AccentStyle)
  (rule Panel
    (= ink 1)
  )
  (blank)
  (rule Container
    (rule Panel
      (= pad 2)
    )
  )
)
```

```output
node AccentStyle.ink layer=2 order=0 kind=static bits=1
node AccentStyle.pad layer=2 order=1 kind=static bits=2
```

### Imported modules can extend a StyleSheet with usings and rules. Rule bodies resolve lexical names from the extension module, not from the original style sheet module.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import ./theme)
(import ./component)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• panel-rule))
  )
  (blank)
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
```

```brix file=theme.bx
(import /core/tests/support/markup-fixtures)

(style AppTheme
  (prop + base-accent AnsiColor (= (call AnsiColor 11)))
)
```

```brix file=palette.bx
(import /core/tests/support/markup-fixtures)

(type + PaletteColor (is AnsiColor))
```

```brix file=component.bx
(import /core/tests/support/markup-fixtures)
(import ./theme)
(import ./palette)

(style-extend AppTheme
  (using TextPaint)
  (rule panel-rule
    (= color (call PaletteColor 9))
  )
)

(type + Panel (is Box))
```

```output
node TextPaint seg=owned gen=1 mask=1 color=9
```

### A StyleSheet extension in an unimported component module is not merged into the theme.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import ./theme)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Box (id: panel) (• panel-rule))
  )
  (blank)
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
```

```brix file=theme.bx
(import /core/tests/support/markup-fixtures)

(style AppTheme
  (using TextPaint)
  (prop + base-accent AnsiColor (= (call AnsiColor 11)))
  (blank)
  (rule panel-rule
    (= color base-accent)
  )
)
```

```brix file=component.bx
(import /core/tests/support/markup-fixtures)
(import ./theme)

(style-extend AppTheme
  (using TextPaint)
  (rule panel-rule
    (= color (call AnsiColor 9))
  )
)
```

```output
node TextPaint seg=owned gen=1 mask=1 color=11
```

### StyleSheet-provided rules stay visible inside markup loops of a component that does not name the style sheet.

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
  (let region (!! (. panel %first-child)))
  (print (call style-computed-snapshot (!! (. region %first-child))))
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
  (prop + items (list String) (= (list "a" "b")))
  (prop + shown _ (= true))
  (prop + state _ (= "a"))
  (content
    (<> This
      (for (in item #items (keyed-by item))
        (<> Label (• primary))
      )
    )
  )
)

(type + Label (is Box))
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=11
```

### StyleSheet-provided rules stay visible inside dynamic markup conditionals of a component that does not name the style sheet.

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
  (let region (!! (. panel %first-child)))
  (print (call style-computed-snapshot (!! (. region %first-child))))
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
  (prop + items (list String) (= (list "a" "b")))
  (prop + shown _ (= true))
  (prop + state _ (= "a"))
  (content
    (<> This
      (if #shown
        (<> Label (• primary))
      )
    )
  )
)

(type + Label (is Box))
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=11
```

### StyleSheet-provided rules stay visible inside dynamic markup match arms of a component that does not name the style sheet.

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
  (let region (!! (. panel %first-child)))
  (print (call style-computed-snapshot (!! (. region %first-child))))
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
  (prop + items (list String) (= (list "a" "b")))
  (prop + shown _ (= true))
  (prop + state _ (= "a"))
  (content
    (<> This
      (match #state
        (case= "a"
          (<> Label (• primary))
        )
        (case _
          (<> Label)
        )
      )
    )
  )
)

(type + Label (is Box))
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=11
```

### A component constructed inside another component's markup loop sees the ancestor style sheet in its own content.

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
  (let region (!! (. panel %first-child)))
  (let row (!! (. region %first-child)))
  (print (call style-computed-snapshot (!! (. row %first-child))))
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
  (prop + items (list String) (= (list "a" "b")))
  (prop + shown _ (= true))
  (prop + state _ (= "a"))
  (content
    (<> This
      (for (in item #items (keyed-by item))
        (<> Row)
      )
    )
  )
)

(type + Label (is Box))

(type + Row (is Box)
  (content
    (<> Label (• primary))
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=11
```

### A component's own defaults rule resolves StyleSheet-provided rules through the style sheet visible where the component is placed.

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
  (prop + items (list String) (= (list "a" "b")))
  (prop + shown _ (= true))
  (prop + state _ (= "a"))
  (blank)
  (rule This (• primary))
)

(type + Label (is Box))
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=11
```

### A component whose defaults rule uses a StyleSheet-provided rule is specialized per placement: an unthemed placement does not reuse the themed constructor.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> ThemeBox
      (<> Panel (id: themed))
    )
    (<> Panel (id: unthemed))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot themed))
  (print (call style-computed-snapshot unthemed))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + ThemeBox (is Box)
  (using AppTheme)
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
  (rule This (• primary))
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=11
initial
```

### A StyleSheet rule matched on an element inside a component's template scopes its nested rules over the slot content a caller supplies beneath that element, and a "when" branch follows the template element's state.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Box (• side)
      (<> List (id: list)
        (<> Row (id: leaf) (slot: row))
      )
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "idle " (call style-computed-snapshot (. leaf icon))))
  (= (. list item is-selected) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "selected " (call style-computed-snapshot (. leaf icon))))
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
  (rule side
    (rule Item
      (when-style #is-selected
        (rule icon
          (= ink 21)
        )
      )
    )
  )
  (blank)
  (rule icon
    (= ink 3)
  )
)

(type + List (is Box)
  (content
    (<> This
      (<> Item (id: item)
        (+<> Box (id: row))
      )
    )
  )
)

(type + Item (is Box)
  (prop + is-selected _ (= false))
)

(type + Row (is Box)
  (content
    (<> This
      (<> Segment (id: icon) (• icon))
    )
  )
)

(type + Segment (is Box)
  (using TextStyle)
)
```

```output
idle node TextStyle seg=owned gen=1 mask=1 ink=3
selected node TextStyle seg=owned gen=2 mask=1 ink=21
```

### Slot content sees the rules matched along the whole template path to its outlet: an element a component places in another component's row slot, the inner component itself, and an element inside the inner component's own template. Each row follows the state of its own elements.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let names (list "a" "b"))
  (blank)
  (<> BatchDemo (id: demo)
    (<> Tree (id: tree)
      (= names names)
      (<> Leaf (slot: row) (param: name)
        (= name name)
      )
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call leaf-inks tree))
  (= (. (call item-at tree 1) is-selected) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call leaf-inks tree))
  (= (. tree region is-keyed) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call leaf-inks tree))
  (= (. tree region is-keyed) false)
  (= (. (as (!! (call (. (!! (. tree region %first-child)) child-at) 0)) Cell) is-marked) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call leaf-inks tree))
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
  (rule Item
    (when-style #is-selected
      (rule icon
        (= ink 21)
      )
    )
  )
  (blank)
  (rule Region
    (when-style #is-keyed
      (rule mark
        (= ink 5)
      )
    )
  )
  (blank)
  (rule Cell
    (when-style #is-marked
      (rule mark
        (= ink 9)
      )
    )
  )
  (blank)
  (rule icon
    (= ink 3)
  )
  (blank)
  (rule mark
    (= ink 1)
  )
)

(type + Tree (is Box)
  (prop + names (list String))
  (content
    (<> This
      (<> Region (id: region)
        (= names #names)
        (<> Item (slot: row) (param: name)
          (+<> Box (id: row) (param: name String))
        )
      )
    )
  )
)

(type + Region (is Box)
  (prop + names (list String))
  (prop + is-keyed _ (= false))
  (content
    (<> This
      (for (in name #names)
        (<> Cell
          (+<> Box (id: row) (param: name String))
        )
      )
    )
  )
)

(type + Cell (is Box)
  (prop + is-marked _ (= false))
)

(type + Item (is Box)
  (prop + is-selected _ (= false))
)

(type + Leaf (is Box)
  (prop + name String)
  (content
    (<> This
      (<> Segment (id: icon) (• icon))
      (<> Segment (id: mark) (• mark))
    )
  )
)

(type + Segment (is Box)
  (using TextStyle)
)

(fn-do + item-at Item
  (param tree Tree)
  (param index Index)
  (return (as (!! (. (!! (call (. (!! (. tree region %first-child)) child-at) index)) %first-child)) Item))
)

(fn-do + leaf-inks String
  (param tree Tree)
  (let parts (list String) (call (of List String)))
  (for (in cell (. (!! (. tree region %first-child)) children))
    (let leaf (as (!! (. (!! (. cell %first-child)) %first-child)) Leaf))
    (+= parts (str (. leaf name) " " (call style-computed-snapshot (. leaf icon)) " / " (call style-computed-snapshot (. leaf mark))))
  )
  (return (call (. "\n" join) parts))
)
```

```output
a node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=1
b node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=1
a node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=1
b node TextStyle seg=owned gen=2 mask=1 ink=21 / node TextStyle seg=owned gen=1 mask=1 ink=1
a node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=2 mask=1 ink=5
b node TextStyle seg=owned gen=2 mask=1 ink=21 / node TextStyle seg=owned gen=2 mask=1 ink=5
a node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=3 mask=1 ink=9
b node TextStyle seg=owned gen=2 mask=1 ink=21 / node TextStyle seg=owned gen=3 mask=1 ink=1
```

### Rules matched in a base class template reach slot content whether the outlet lives in the base or in content a derived class supplies to the base's slot.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Card (id: card)
      (<> Segment (id: icon) (slot: body) (• icon))
    )
    (<> Plain (id: plain)
      (<> Segment (• icon))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot icon))
  (print (call style-computed-snapshot (!! (. plain frame %first-child))))
  (= (. card item is-selected) true)
  (= (. plain frame is-lit) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot icon))
  (print (call style-computed-snapshot (!! (. plain frame %first-child))))
  (= (. card item is-selected) false)
  (= (. card frame is-lit) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot icon))
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

(type + Segment (is Box)
  (using TextStyle)
)

(style AppTheme
  (using TextStyle)
  (rule Item
    (when-style #is-selected
      (rule icon
        (= ink 21)
      )
    )
  )
  (blank)
  (rule Frame
    (when-style #is-lit
      (rule icon
        (= ink 30)
      )
    )
  )
  (blank)
  (rule icon
    (= ink 3)
  )
)

(type + Shell (is Box)
  (content
    (<> This
      (<> Frame (id: frame)
        (+<> Box)
      )
    )
  )
)

(type + Card (is Shell)
  (content
    (<> This
      (<> Item (id: item)
        (+<> Box (id: body))
      )
    )
  )
)

(type + Plain (is Shell))

(type + Frame (is Box)
  (prop + is-lit _ (= false))
)

(type + Item (is Box)
  (prop + is-selected _ (= false))
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=3
node TextStyle seg=owned gen=1 mask=1 ink=3
node TextStyle seg=owned gen=2 mask=1 ink=21
node TextStyle seg=owned gen=2 mask=1 ink=30
node TextStyle seg=owned gen=3 mask=1 ink=30
```

### A component nested in its own slot content keeps each level's state apart: content follows every enclosing instance's template element, innermost included.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> List (id: outer)
      (<> Segment (id: a) (• icon))
      (<> List (id: inner)
        (<> Segment (• icon))
      )
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot a))
  (print (call style-computed-snapshot (!! (. inner item %first-child))))
  (= (. inner item is-selected) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot a))
  (print (call style-computed-snapshot (!! (. inner item %first-child))))
  (= (. inner item is-selected) false)
  (= (. outer item is-selected) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot a))
  (print (call style-computed-snapshot (!! (. inner item %first-child))))
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

(type + Segment (is Box)
  (using TextStyle)
)

(style AppTheme
  (using TextStyle)
  (rule Item
    (when-style #is-selected
      (rule icon
        (= ink 21)
      )
    )
  )
  (blank)
  (rule icon
    (= ink 3)
  )
)

(type + List (is Box)
  (content
    (<> This
      (<> Item (id: item)
        (+<> Box)
      )
    )
  )
)

(type + Item (is Box)
  (prop + is-selected _ (= false))
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=3
node TextStyle seg=owned gen=1 mask=1 ink=3
node TextStyle seg=owned gen=1 mask=1 ink=3
node TextStyle seg=owned gen=2 mask=1 ink=21
node TextStyle seg=owned gen=2 mask=1 ink=21
node TextStyle seg=owned gen=2 mask=1 ink=21
```

### Nested rules of a rule that takes arguments are not carried from a template element to slot content; the same content placed in the template still resolves them.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> List (id: list)
      (<> Segment (id: icon) (• icon))
      (<> Segment (id: mark) (• mark))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot icon))
  (print (call style-computed-snapshot mark))
  (print (call style-computed-snapshot (. list inside)))
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

(type + Segment (is Box)
  (using TextStyle)
)

(style AppTheme
  (using TextStyle)
  (rule tint
    (param amount I64)
    (rule icon
      (= ink amount)
    )
    (blank)
    (rule mark
      (= ink 40)
    )
  )
  (blank)
  (rule icon
    (= ink 3)
  )
  (blank)
  (rule mark
    (= ink 1)
  )
)

(type + List (is Box)
  (content
    (<> This
      (<> Item (id: item) (• tint 5)
        (<> Segment (id: inside) (• icon))
        (+<> Box)
      )
    )
  )
)

(type + Item (is Box))
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=3
node TextStyle seg=owned gen=1 mask=1 ink=1
node TextStyle seg=owned gen=1 mask=1 ink=5
```

### A class rule applied by type binds style sheet slots through the style sheets visible at each element, and an unrelated visible style sheet changes nothing.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> AppBox
      (<> Panel (id: app))
    )
    (<> OtherBox
      (<> Panel (id: other))
    )
    (<> AppSpareBox
      (<> Panel (id: spared))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot app))
  (print (call style-computed-snapshot other))
  (print (call style-computed-snapshot spared))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + tone I64 (= 0))
)

(style AppTheme
  (prop + accent I64 (= 11))
)

(style OtherTheme
  (prop + accent I64 (= 22))
)

(style SpareTheme
  (prop + spare I64 (= 33))
)

(type + AppBox (is Box)
  (using AppTheme)
)

(type + OtherBox (is Box)
  (using OtherTheme)
)

(type + AppSpareBox (is Box)
  (using SpareTheme)
  (using AppTheme)
)

(type + Panel (is Box)
  (using TintStyle)
  (rule Panel
    (= ink accent)
    (= tone 2)
  )
)
```

```output
node TintStyle seg=owned gen=1 mask=3 ink=11 tone=2
node TintStyle seg=owned gen=1 mask=3 ink=22 tone=2
node TintStyle seg=owned gen=1 mask=3 ink=11 tone=2
```

### A class rule applied by type applies style-sheet-provided rules through the style sheets visible at each element, and an unrelated visible style sheet changes nothing.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> AppBox
      (<> Panel (id: app))
    )
    (<> OtherBox
      (<> Panel (id: other))
    )
    (<> AppSpareBox
      (<> Panel (id: spared))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-authored-snapshot app))
  (print (call style-authored-snapshot other))
  (print (call style-authored-snapshot spared))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + tone I64 (= 0))
)

(style AppTheme
  (using TintStyle)
  (rule chrome
    (= ink 5)
  )
)

(style OtherTheme
  (using TintStyle)
  (rule chrome
    (= ink 6)
  )
)

(style SpareTheme
  (using TintStyle)
  (rule trim
    (= ink 7)
  )
)

(type + AppBox (is Box)
  (using AppTheme)
)

(type + OtherBox (is Box)
  (using OtherTheme)
)

(type + AppSpareBox (is Box)
  (using SpareTheme)
  (using AppTheme)
)

(type + Panel (is Box)
  (using TintStyle)
  (rule Panel (• chrome)
    (= tone 2)
  )
)
```

```output
node TintStyle.ink layer=2 order=0 kind=static bits=5
node TintStyle.tone layer=2 order=1 kind=static bits=2
node TintStyle.ink layer=2 order=0 kind=static bits=6
node TintStyle.tone layer=2 order=1 kind=static bits=2
node TintStyle.ink layer=2 order=0 kind=static bits=5
node TintStyle.tone layer=2 order=1 kind=static bits=2
```

### A forwarded row slot carries rules from both components to the caller's content: an ancestor of the forwarding element and that element itself in the forwarding template, and an element inside the template that holds the outlet. Each row follows its own elements.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let names (list "a" "b"))
  (blank)
  (<> BatchDemo (id: demo)
    (<> Grid (id: grid)
      (= names names)
      (<> Leaf (slot: row) (param: name)
        (= name name)
      )
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call leaf-inks grid))
  (= (. grid band is-lit) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call leaf-inks grid))
  (= (. grid rows is-keyed) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call leaf-inks grid))
  (= (. (call cell-at grid 1) is-marked) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call leaf-inks grid))
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

(type + Segment (is Box)
  (using TextStyle)
)

(style AppTheme
  (using TextStyle)
  (rule Band
    (when-style #is-lit
      (rule icon
        (= ink 30)
      )
    )
  )
  (blank)
  (rule RowPanel
    (when-style #is-keyed
      (rule mark
        (= ink 5)
      )
    )
  )
  (blank)
  (rule Cell
    (when-style #is-marked
      (rule dot
        (= ink 9)
      )
    )
  )
  (blank)
  (rule icon
    (= ink 3)
  )
  (blank)
  (rule mark
    (= ink 2)
  )
  (blank)
  (rule dot
    (= ink 1)
  )
)

(type + Grid (is Box)
  (prop + names (list String))
  (content
    (<> This
      (<> Band (id: band)
        (<> RowPanel (id: rows)
          (= names #names)
          (<+ row)
        )
      )
    )
  )
)

(type + Band (is Box)
  (prop + is-lit _ (= false))
)

(type + RowPanel (is Box)
  (prop + names (list String))
  (prop + is-keyed _ (= false))
  (content
    (<> This
      (for (in name #names)
        (<> Cell
          (+<> Box (id: row) (param: name String))
        )
      )
    )
  )
)

(type + Cell (is Box)
  (prop + is-marked _ (= false))
)

(type + Leaf (is Box)
  (prop + name String)
  (content
    (<> This
      (<> Segment (id: icon) (• icon))
      (<> Segment (id: mark) (• mark))
      (<> Segment (id: dot) (• dot))
    )
  )
)

(fn-do + cell-at Cell
  (param grid Grid)
  (param index Index)
  (return (as (!! (call (. (!! (. grid rows %first-child)) child-at) index)) Cell))
)

(fn-do + leaf-inks String
  (param grid Grid)
  (let parts (list String) (call (of List String)))
  (for (in cell (. (!! (. grid rows %first-child)) children))
    (let leaf (as (!! (. cell %first-child)) Leaf))
    (+= parts (str (. leaf name) " " (call style-computed-snapshot (. leaf icon)) " / " (call style-computed-snapshot (. leaf mark)) " / " (call style-computed-snapshot (. leaf dot))))
  )
  (return (call (. "\n" join) parts))
)
```

```output
a node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=2 / node TextStyle seg=owned gen=1 mask=1 ink=1
b node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=2 / node TextStyle seg=owned gen=1 mask=1 ink=1
a node TextStyle seg=owned gen=2 mask=1 ink=30 / node TextStyle seg=owned gen=1 mask=1 ink=2 / node TextStyle seg=owned gen=1 mask=1 ink=1
b node TextStyle seg=owned gen=2 mask=1 ink=30 / node TextStyle seg=owned gen=1 mask=1 ink=2 / node TextStyle seg=owned gen=1 mask=1 ink=1
a node TextStyle seg=owned gen=2 mask=1 ink=30 / node TextStyle seg=owned gen=2 mask=1 ink=5 / node TextStyle seg=owned gen=1 mask=1 ink=1
b node TextStyle seg=owned gen=2 mask=1 ink=30 / node TextStyle seg=owned gen=2 mask=1 ink=5 / node TextStyle seg=owned gen=1 mask=1 ink=1
a node TextStyle seg=owned gen=2 mask=1 ink=30 / node TextStyle seg=owned gen=2 mask=1 ink=5 / node TextStyle seg=owned gen=1 mask=1 ink=1
b node TextStyle seg=owned gen=2 mask=1 ink=30 / node TextStyle seg=owned gen=2 mask=1 ink=5 / node TextStyle seg=owned gen=2 mask=1 ink=9
```

### A forwarded slot carries the forwarding component's root rules along with rules matched on the forwarding element and inside the component that holds the outlet.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel)
      (<> Segment (id: icon) (slot: header) (• icon))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot icon))
  (= (. panel is-lit) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot icon))
  (= (. panel is-lit) false)
  (= (. panel inner is-keyed) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot icon))
  (= (. panel inner is-keyed) false)
  (= (. panel inner frame is-marked) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot icon))
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

(type + Segment (is Box)
  (using TextStyle)
)

(style AppTheme
  (using TextStyle)
  (rule lit-frame
    (when-style #is-lit
      (rule icon
        (= ink 30)
      )
    )
  )
  (blank)
  (rule Inner
    (when-style #is-keyed
      (rule icon
        (= ink 5)
      )
    )
  )
  (blank)
  (rule Frame
    (when-style #is-marked
      (rule icon
        (= ink 9)
      )
    )
  )
  (blank)
  (rule icon
    (= ink 3)
  )
)

(type + Panel (is Box)
  (prop + is-lit _ (= false))
  (content
    (<> This (• lit-frame)
      (<> Inner (id: inner)
        (<+ header)
      )
    )
  )
)

(type + Inner (is Box)
  (prop + is-keyed _ (= false))
  (content
    (<> This
      (<> Frame (id: frame)
        (+<> Box (id: header))
      )
    )
  )
)

(type + Frame (is Box)
  (prop + is-marked _ (= false))
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=3
node TextStyle seg=owned gen=2 mask=1 ink=30
node TextStyle seg=owned gen=3 mask=1 ink=5
node TextStyle seg=owned gen=4 mask=1 ink=9
```

### A type-matched StyleSheet rule's nested rules reach id-bound content a caller supplies to the component's slot, and the markup of components in that content, whether or not a rule names them.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Frame
      (<> Leaf (id: direct))
      (<> Named (id: named))
      (<> Other (id: other))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot direct))
  (print (call style-computed-snapshot (!! (. named %first-child))))
  (print (call style-computed-snapshot (!! (. other %first-child))))
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
  (rule Frame
    (rule Leaf
      (= ink 21)
    )
    (blank)
    (rule Named)
  )
)

(type + Frame (is Box)
  (content
    (<> This
      (+<> Box)
    )
  )
)

(type + Named (is Box)
  (content
    (<> This
      (<> Leaf)
    )
  )
)

(type + Other (is Box)
  (content
    (<> This
      (<> Leaf)
    )
  )
)

(type + Leaf (is Box)
  (using TextStyle)
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=21
node TextStyle seg=owned gen=1 mask=1 ink=21
node TextStyle seg=owned gen=1 mask=1 ink=21
```

### A rule written in a `when` of a style sheet's rule that a markup element applies is seen once in the markup of the element's class, though the class's root `<This` matches the sheet's rule again: its declaration is made once.

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
  (print (call style-authored-snapshot (. panel inner)))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (prop + is-open _ (= true))
  (content
    (<> This
      (<> Segment (id: inner) (• part))
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
)

(style AppTheme
  (using TintStyle)
  (rule Panel
    (when-style #is-open
      (rule part
        (= ink 7)
      )
    )
  )
)
```

```output
node TintStyle.ink layer=4 order=0 kind=static bits=7 block=0
```

### A rule nested under the `when`s of two or three rules applies only while every one of those states holds, through a component's content and a forwarded row slot, each row on its own.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let names (list "a" "b"))
  (blank)
  (<> BatchDemo (id: demo)
    (<> Grid (id: grid)
      (= names names)
      (<> Leaf (slot: row) (param: name)
        (= name name)
      )
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call leaf-inks grid))
  (= (. (call cell-at grid 1) is-marked) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call leaf-inks grid))
  (= (. grid band is-lit) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call leaf-inks grid))
  (= (. grid rows is-keyed) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call leaf-inks grid))
  (= (. grid band is-lit) false)
  (call (. demo markup-batch-queue checkpoint))
  (print (call leaf-inks grid))
  (= (. (call cell-at grid 0) is-marked) true)
  (= (. grid band is-lit) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call leaf-inks grid))
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

(type + Segment (is Box)
  (using TextStyle)
)

(style AppTheme
  (using TextStyle)
  (rule Band
    (when-style #is-lit
      (rule Cell
        (when-style #is-marked
          (rule icon
            (= ink 30)
          )
        )
      )
      (blank)
      (rule RowPanel
        (when-style #is-keyed
          (rule Cell
            (when-style #is-marked
              (rule dot
                (= ink 9)
              )
            )
          )
        )
      )
    )
  )
  (blank)
  (rule RowPanel
    (when-style #is-keyed
      (rule Cell
        (when-style #is-marked
          (rule mark
            (= ink 5)
          )
        )
      )
    )
  )
  (blank)
  (rule icon
    (= ink 3)
  )
  (blank)
  (rule mark
    (= ink 2)
  )
  (blank)
  (rule dot
    (= ink 1)
  )
)

(type + Grid (is Box)
  (prop + names (list String))
  (content
    (<> This
      (<> Band (id: band)
        (<> RowPanel (id: rows)
          (= names #names)
          (<+ row)
        )
      )
    )
  )
)

(type + Band (is Box)
  (prop + is-lit _ (= false))
)

(type + RowPanel (is Box)
  (prop + names (list String))
  (prop + is-keyed _ (= false))
  (content
    (<> This
      (for (in name #names)
        (<> Cell
          (+<> Box (id: row) (param: name String))
        )
      )
    )
  )
)

(type + Cell (is Box)
  (prop + is-marked _ (= false))
)

(type + Leaf (is Box)
  (prop + name String)
  (content
    (<> This
      (<> Segment (id: icon) (• icon))
      (<> Segment (id: mark) (• mark))
      (<> Segment (id: dot) (• dot))
    )
  )
)

(fn-do + cell-at Cell
  (param grid Grid)
  (param index Index)
  (return (as (!! (call (. (!! (. grid rows %first-child)) child-at) index)) Cell))
)

(fn-do + leaf-inks String
  (param grid Grid)
  (let parts (list String) (call (of List String)))
  (for (in cell (. (!! (. grid rows %first-child)) children))
    (let leaf (as (!! (. cell %first-child)) Leaf))
    (+= parts (str (. leaf name) " " (call style-computed-snapshot (. leaf icon)) " / " (call style-computed-snapshot (. leaf mark)) " / " (call style-computed-snapshot (. leaf dot))))
  )
  (return (call (. "\n" join) parts))
)
```

```output
a node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=2 / node TextStyle seg=owned gen=1 mask=1 ink=1
b node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=2 / node TextStyle seg=owned gen=1 mask=1 ink=1
a node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=2 / node TextStyle seg=owned gen=1 mask=1 ink=1
b node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=2 / node TextStyle seg=owned gen=1 mask=1 ink=1
a node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=2 / node TextStyle seg=owned gen=1 mask=1 ink=1
b node TextStyle seg=owned gen=2 mask=1 ink=30 / node TextStyle seg=owned gen=1 mask=1 ink=2 / node TextStyle seg=owned gen=1 mask=1 ink=1
a node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=2 / node TextStyle seg=owned gen=1 mask=1 ink=1
b node TextStyle seg=owned gen=2 mask=1 ink=30 / node TextStyle seg=owned gen=2 mask=1 ink=5 / node TextStyle seg=owned gen=2 mask=1 ink=9
a node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=2 / node TextStyle seg=owned gen=1 mask=1 ink=1
b node TextStyle seg=owned gen=3 mask=1 ink=3 / node TextStyle seg=owned gen=2 mask=1 ink=5 / node TextStyle seg=owned gen=3 mask=1 ink=1
a node TextStyle seg=owned gen=2 mask=1 ink=30 / node TextStyle seg=owned gen=2 mask=1 ink=5 / node TextStyle seg=owned gen=2 mask=1 ink=9
b node TextStyle seg=owned gen=4 mask=1 ink=30 / node TextStyle seg=owned gen=2 mask=1 ink=5 / node TextStyle seg=owned gen=4 mask=1 ink=9
```

### A rule nested under two `when`s reaches markup a slot builder makes and rows a slot gives, each gated by the outer element's state and its own item's.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Tree (id: tree) (• tree-frame)
      (= names (list "a" "b"))
      (<> Leaf (slot: entry) (param: row)
        (= name row)
      )
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call inks tree))
  (= (. tree is-focused) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call inks tree))
  (= (. tree item is-selected) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call inks tree))
  (= (. (as (!! (. (!! (call (. (!! (. tree rows %first-child)) child-at) 1)) %first-child)) Item) is-selected) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call inks tree))
  (= (. tree is-focused) false)
  (call (. demo markup-batch-queue checkpoint))
  (print (call inks tree))
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

(type + Segment (is Box)
  (using TextStyle)
)

(style AppTheme
  (using TextStyle)
  (rule tree-frame
    (when-style #is-focused
      (rule Item
        (when-style #is-selected
          (rule icon
            (= ink 21)
          )
        )
      )
    )
  )
  (blank)
  (rule icon
    (= ink 3)
  )
)

(type + Tree (is Box)
  (prop + is-focused _ (= false))
  (prop + names (list String))
  (content
    (<> This
      (<> Holder (id: holder)
        (<> Item (id: item)
          (<> Segment (id: icon) (• icon))
        )
      )
      (<> Rows (id: rows)
        (= names #names)
        (<> Item (slot: row) (param: name)
          (<> Segment (• icon))
          (+<> Box (id: entry) (param: name String))
        )
      )
    )
  )
)

(type + Holder (is Box)
  (content
    (<> This
      (+<> Box)
    )
  )
)

(type + Rows (is Box)
  (prop + names (list String))
  (content
    (<> This
      (for (in name #names)
        (<> Box
          (+<> Box (id: row) (param: name String))
        )
      )
    )
  )
)

(type + Item (is Box)
  (prop + is-selected _ (= false))
)

(type + Leaf (is Box)
  (prop + name String)
  (content
    (<> This
      (<> Segment (id: icon) (• icon))
    )
  )
)

(fn-do + inks String
  (param tree Tree)
  (let parts (list String) (call (of List String)))
  (+= parts (str "holder " (call style-computed-snapshot (. tree icon))))
  (for (in box (. (!! (. tree rows %first-child)) children))
    (let item (as (!! (. box %first-child)) Item))
    (let own (as (!! (. item %first-child)) Segment))
    (let leaf (as (!! (call (. item child-at) 1)) Leaf))
    (+= parts (str (. leaf name) " " (call style-computed-snapshot own) " / " (call style-computed-snapshot (. leaf icon))))
  )
  (return (call (. "\n" join) parts))
)
```

```output
holder node TextStyle seg=owned gen=1 mask=1 ink=3
a node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=3
b node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=3
holder node TextStyle seg=owned gen=1 mask=1 ink=3
a node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=3
b node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=3
holder node TextStyle seg=owned gen=2 mask=1 ink=21
a node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=3
b node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=3
holder node TextStyle seg=owned gen=2 mask=1 ink=21
a node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=3
b node TextStyle seg=owned gen=2 mask=1 ink=21 / node TextStyle seg=owned gen=2 mask=1 ink=21
holder node TextStyle seg=owned gen=3 mask=1 ink=3
a node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=3
b node TextStyle seg=owned gen=3 mask=1 ink=3 / node TextStyle seg=owned gen=3 mask=1 ink=3
```

### A style sheet derived from another (`• Name is Base for tags`) overrides slots of its base by restating them with a value, and registers under its own name as a record of the base's slots: providing it changes the subtree's values, and a sheet derived from a derived one keeps the overrides above it. A class that uses a derived sheet resolves slots against the sheet its family derives from, so using both names one sheet.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink bright)
    )
    (<> Label
      (= ink dim)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let first (!! (. demo %first-child)))
  (let second (!! (. first %next-sibling)))
  (blank)
  (print (call style-computed-snapshot first))
  (print (call style-computed-snapshot second))
  (blank)
  (call style-use-theme demo DarkTheme)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-computed-snapshot first))
  (print (call style-computed-snapshot second))
  (blank)
  (call style-use-theme demo ContrastTheme)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-computed-snapshot first))
  (print (call style-computed-snapshot second))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using AppTheme)
  (using DarkTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style AppTheme
  (prop + bright I64 (= 10))
  (prop + dim I64 (= 2))
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(style ContrastTheme (is DarkTheme) (for dark high-contrast)
  (prop + dim _ (= 0))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=10
node TextStyle seg=owned gen=1 mask=1 ink=2
node TextStyle seg=owned gen=2 mask=1 ink=20
node TextStyle seg=owned gen=1 mask=1 ink=2
node TextStyle seg=owned gen=2 mask=1 ink=20
node TextStyle seg=owned gen=2 mask=1 ink=0
```

### A derived style sheet only overrides slots of the sheet it derives from: it declares none of its own.

```brix crash
(style AppTheme
  (prop + bright I64 (= 10))
)

(style DarkTheme (is AppTheme)
  (prop + extra I64 (= 3))
)
```

```output
Exception: Derived StyleSheet "DarkTheme" can only override slots of AppTheme, which has no slot "extra"
<no file:7>
      (prop + extra I64 (= 3))
      ^^^^^^^^^^^^^^^^^^^^^^^^
```

### A style sheet derives only from another style sheet.

```brix crash
(type + Plain)

(style OddTheme (is Plain))
```

```output
Exception: StyleSheet base "Plain" is not a StyleSheet
<no file:4>
    (style OddTheme (is Plain))
                        ^^^^^
```

### A style sheet's base must be found.

```brix crash
(style OddTheme (is Missing))
```

```output
Exception: StyleSheet base "Missing" not found
<no file:2>
    (style OddTheme (is Missing))
                        ^^^^^^^
```

### A derived style sheet names each of its tags once.

```brix crash
(style AppTheme
  (prop + bright I64 (= 10))
)

(style DarkTheme (is AppTheme) (for dark dark))
```

```output
Exception: StyleSheet tag "dark" is repeated
<no file:6>
    (style DarkTheme (is AppTheme) (for dark dark))
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A style value that reads StyleSheet slots inside an expression, a slot given alone to a rule that declares what it takes, and a line of a sheet's rule that computes with the sheet's slots all take their values from the subtree's effective record of the sheet, each worked out once per record and kept until a slot of the record is written. A derived sheet holds every slot whose value reads another, so `doubled` follows `bright` in DarkTheme.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink (+ bright 1))
    )
    (<> Label (• ink-of dim))
    (<> Label (• glow))
    (<> Label
      (= ink doubled)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let labels (list (!! (. demo %first-child))))
  (+= labels (!! (. ([] labels 0) %next-sibling)))
  (+= labels (!! (. ([] labels 1) %next-sibling)))
  (+= labels (!! (. ([] labels 2) %next-sibling)))
  (blank)
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (call style-use-theme demo DarkTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (call style-use-theme demo ContrastTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (let custom (call StyleSheetRecord (!! (call style-sheet-info-named "AppTheme"))))
  (call style-provide-style-sheet demo custom)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (call (. custom set-slot-named) "bright" 5 false)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using AppTheme)
  (using InkRules)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style InkRules
  (using TextStyle)
  (rule ink-of
    (param value I64)
    (= ink value)
  )
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
  (prop + dim I64 (= 2))
  (prop + doubled I64 (= (* bright 2)))
  (blank)
  (rule glow
    (= ink (* dim 10))
  )
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(style ContrastTheme (is DarkTheme) (for dark high-contrast)
  (prop + dim _ (= 0))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=11 / node TextStyle seg=owned gen=1 mask=1 ink=2 / node TextStyle seg=owned gen=1 mask=1 ink=20 / node TextStyle seg=owned gen=1 mask=1 ink=20
node TextStyle seg=owned gen=2 mask=1 ink=21 / node TextStyle seg=owned gen=1 mask=1 ink=2 / node TextStyle seg=owned gen=1 mask=1 ink=20 / node TextStyle seg=owned gen=2 mask=1 ink=40
node TextStyle seg=owned gen=2 mask=1 ink=21 / node TextStyle seg=owned gen=2 mask=1 ink=0 / node TextStyle seg=owned gen=2 mask=1 ink=0 / node TextStyle seg=owned gen=2 mask=1 ink=40
node TextStyle seg=owned gen=3 mask=1 ink=11 / node TextStyle seg=owned gen=3 mask=1 ink=2 / node TextStyle seg=owned gen=3 mask=1 ink=20 / node TextStyle seg=owned gen=3 mask=1 ink=20
node TextStyle seg=owned gen=4 mask=1 ink=6 / node TextStyle seg=owned gen=3 mask=1 ink=2 / node TextStyle seg=owned gen=3 mask=1 ink=20 / node TextStyle seg=owned gen=3 mask=1 ink=20
```

### A style value that reads StyleSheet slots reads nothing else that differs from one making to the next.

```brix crash
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let extra 3)
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink (+ bright extra))
    )
  )
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using AppTheme)
  (using InkRules)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style InkRules
  (using TextStyle)
  (rule ink-of
    (param value I64)
    (= ink value)
  )
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
  (prop + dim I64 (= 2))
  (prop + doubled I64 (= (* bright 2)))
  (blank)
  (rule glow
    (= ink (* dim 10))
  )
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(style ContrastTheme (is DarkTheme) (for dark high-contrast)
  (prop + dim _ (= 0))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
Exception: A style value that reads StyleSheet slots may read no local, member, style or tracked value; read a slot as "$name" to compute with one
<no file:12>
          (= ink (+ bright extra))
                 ^^^^^^^^^^^^^^^^
```

### A style rule takes a StyleSheet slot only alone.

```brix crash
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (• ink-of (+ dim 1)))
  )
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using AppTheme)
  (using InkRules)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style InkRules
  (using TextStyle)
  (rule ink-of
    (param value I64)
    (= ink value)
  )
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
  (prop + dim I64 (= 2))
  (prop + doubled I64 (= (* bright 2)))
  (blank)
  (rule glow
    (= ink (* dim 10))
  )
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(style ContrastTheme (is DarkTheme) (for dark high-contrast)
  (prop + dim _ (= 0))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
Exception: A style rule takes a StyleSheet slot only alone, as in "• bg:panel-color", not in an expression
<no file:10>
        (<> Label (• ink-of (+ dim 1)))
                               ^^^
```

### A style rule given a StyleSheet slot computes with it where it is applied: the runtime evaluates the line from the subtree's theme, for each slot the rule is given, and again on a theme switch.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (id: dimmed) (• twice dim))
    (<> Label (id: lit) (• twice bright))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot dimmed))
  (print (call style-computed-snapshot lit))
  (call style-use-theme demo ContrastTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot dimmed))
  (print (call style-computed-snapshot lit))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using AppTheme)
  (using InkRules)
  (using MoreRules)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style InkRules
  (using TextStyle)
  (rule ink-of
    (param value I64)
    (= ink value)
  )
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
  (prop + dim I64 (= 2))
  (prop + doubled I64 (= (* bright 2)))
  (blank)
  (rule glow
    (= ink (* dim 10))
  )
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(style ContrastTheme (is DarkTheme) (for dark high-contrast)
  (prop + dim _ (= 0))
)

(type + Label (is Box)
  (using TextStyle)
)

(style MoreRules
  (using TextStyle)
  (rule twice
    (param value I64)
    (= ink (* value 2))
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=4
node TextStyle seg=owned gen=1 mask=1 ink=20
node TextStyle seg=owned gen=2 mask=1 ink=0
node TextStyle seg=owned gen=2 mask=1 ink=40
```

### A style value reads the slots of one StyleSheet.

```brix crash
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink (+ bright other))
    )
  )
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using AppTheme)
  (using InkRules)
  (using OtherTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style InkRules
  (using TextStyle)
  (rule ink-of
    (param value I64)
    (= ink value)
  )
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
  (prop + dim I64 (= 2))
  (prop + doubled I64 (= (* bright 2)))
  (blank)
  (rule glow
    (= ink (* dim 10))
  )
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(style ContrastTheme (is DarkTheme) (for dark high-contrast)
  (prop + dim _ (= 0))
)

(type + Label (is Box)
  (using TextStyle)
)

(style OtherTheme
  (prop + other I64 (= 1))
)
```

```output
Exception: A style value reads slots of one StyleSheet: "other" is a slot of OtherTheme, and others it reads are slots of AppTheme
<no file:11>
          (= ink (+ bright other))
                           ^^^^^
```

### A family's members are layers of its rules: the rules that apply are those of the active member, each name taken from the nearest sheet up its chain that writes it. DarkTheme restates "glow" and adds "shine", ContrastTheme takes both from DarkTheme and restates "Tag", and a rule a rule applies after its name follows the active member too.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (• glow))
    (<> Label (• shine))
    (<> Tag)
    (<> Label (• boxed))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let labels (list (!! (. demo %first-child))))
  (+= labels (!! (. ([] labels 0) %next-sibling)))
  (+= labels (!! (. ([] labels 1) %next-sibling)))
  (+= labels (!! (. ([] labels 2) %next-sibling)))
  (blank)
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (call style-use-theme demo DarkTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (call style-use-theme demo ContrastTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (call style-provide-style-sheet demo (call StyleSheetRecord (!! (call style-sheet-info-named "AppTheme"))))
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (prop + size I64 @inherited (= 1))
  (blank)
  (style-domains ink (domain paint))
  (style-domains size (domain paint))
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
  (blank)
  (rule glow
    (= ink 1)
  )
  (blank)
  (rule Tag
    (= ink 50)
  )
  (blank)
  (rule boxed (• glow)
    (= size 3)
  )
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
  (blank)
  (rule glow
    (= ink 2)
  )
  (blank)
  (rule shine
    (= ink (+ bright 10))
  )
)

(style ContrastTheme (is DarkTheme) (for dark high-contrast)
  (rule Tag
    (= ink 60)
  )
)

(type + Label (is Box)
  (using TextStyle)
)

(type + Tag (is Box)
  (using TextStyle)
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=1 size=1 / initial / node TextStyle seg=owned gen=1 mask=1 ink=50 size=1 / node TextStyle seg=owned gen=1 mask=3 ink=1 size=3
node TextStyle seg=owned gen=2 mask=1 ink=2 size=1 / node TextStyle seg=owned gen=1 mask=1 ink=30 size=1 / node TextStyle seg=owned gen=1 mask=1 ink=50 size=1 / node TextStyle seg=owned gen=2 mask=1 ink=2 size=3
node TextStyle seg=owned gen=2 mask=1 ink=2 size=1 / node TextStyle seg=owned gen=1 mask=1 ink=30 size=1 / node TextStyle seg=owned gen=2 mask=1 ink=60 size=1 / node TextStyle seg=owned gen=2 mask=1 ink=2 size=3
node TextStyle seg=owned gen=3 mask=1 ink=1 size=1 / initial / node TextStyle seg=owned gen=3 mask=1 ink=50 size=1 / node TextStyle seg=owned gen=3 mask=1 ink=1 size=3
```

### A derived sheet's rule that a class gets by name replaces the root's rule among the class's defaults while that member is active, its "when" included.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Chip)
    (<> Chip
      (= ink 5)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let labels (list (!! (. demo %first-child))))
  (+= labels (!! (. ([] labels 0) %next-sibling)))
  (blank)
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (call style-use-theme demo DarkTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (= (. (as ([] labels 0) Chip) is-hovered) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (call style-provide-style-sheet demo (call StyleSheetRecord (!! (call style-sheet-info-named "AppTheme"))))
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
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
  (prop + ink I64 @inherited (= 7))
  (prop + size I64 @inherited (= 1))
  (blank)
  (style-domains ink (domain paint))
  (style-domains size (domain paint))
)

(style AppTheme
  (using TextStyle)
  (rule Chip
    (= ink 1)
    (= size 2)
  )
)

(style DarkTheme (is AppTheme) (for dark)
  (rule Chip
    (= ink 3)
    (when-style #is-hovered
      (= size 9)
    )
  )
)

(type + Chip (is Box)
  (using TextStyle)
  (using AppTheme)
  (prop + is-hovered _ (= false))
)
```

```output
node TextStyle seg=owned gen=1 mask=3 ink=1 size=2 / node TextStyle seg=owned gen=1 mask=3 ink=5 size=2
node TextStyle seg=owned gen=2 mask=3 ink=3 size=1 / node TextStyle seg=owned gen=2 mask=2 ink=5 size=1
node TextStyle seg=owned gen=3 mask=2 ink=3 size=9 / node TextStyle seg=owned gen=2 mask=2 ink=5 size=1
node TextStyle seg=owned gen=4 mask=3 ink=1 size=2 / node TextStyle seg=owned gen=3 mask=2 ink=5 size=2
```

### A rule a derived sheet restates with the same values takes what the markup gives it, including a slot given alone and values a sheet's rule gives a rule it applies.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (• ink-of 3))
    (<> Label (• ink-of bright))
    (<> Label (• strong))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let labels (list (!! (. demo %first-child))))
  (+= labels (!! (. ([] labels 0) %next-sibling)))
  (+= labels (!! (. ([] labels 1) %next-sibling)))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (call style-use-theme demo DarkTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (prop + size I64 @inherited (= 1))
  (prop + weight I64 @inherited (= 0))
  (blank)
  (style-domains ink (domain paint))
  (style-domains size (domain paint))
  (style-domains weight (domain paint))
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
  (blank)
  (rule ink-of
    (param value I64)
    (= ink value)
  )
  (blank)
  (rule strong (• ink-of 5)
    (= weight 2)
  )
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
  (blank)
  (rule ink-of
    (param value I64)
    (= ink value)
    (= size 4)
  )
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=3 size=1 weight=0 / node TextStyle seg=owned gen=1 mask=1 ink=10 size=1 weight=0 / node TextStyle seg=owned gen=1 mask=5 ink=5 size=1 weight=2
node TextStyle seg=owned gen=2 mask=2 ink=3 size=4 weight=0 / node TextStyle seg=owned gen=2 mask=3 ink=20 size=4 weight=0 / node TextStyle seg=owned gen=2 mask=2 ink=5 size=4 weight=2
```

### A subtree's theme tags are those turned on nearest it, and in each family its active member is the most specific variant of the theme it uses whose tags all hold, or that theme itself. A variant of a variant needs the tags of both. Turning a tag off, or using another theme, holds for the subtree; a tag set to null is inherited again. A theme a subtree uses by name stays active where no variant of it matches.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink bright)
    )
    (<> Panel (id: panel)
      (<> Label (• glow))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let labels (list (!! (. demo %first-child))))
  (+= labels (!! (. panel %first-child)))
  (blank)
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (call style-set-theme-tag demo "dark" true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (call style-set-theme-tag demo "high-contrast" true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (call style-set-theme-tag panel "dark" false)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (call style-use-theme panel FlatTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (call style-set-theme-tag panel "dark" null)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
  (blank)
  (call style-use-theme demo DarkTheme)
  (call style-set-theme-tag demo "dark" null)
  (call style-set-theme-tag demo "high-contrast" null)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) labels)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
  (blank)
  (rule glow
    (= ink 1)
  )
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
  (blank)
  (rule glow
    (= ink 2)
  )
)

(style ContrastTheme (is DarkTheme) (for high-contrast)
  (prop + bright _ (= 30))
)

(style FlatTheme (is AppTheme)
  (prop + bright _ (= 40))
  (blank)
  (rule glow
    (= ink 4)
  )
)

(style FlatDark (is FlatTheme) (for dark)
  (prop + bright _ (= 50))
)

(type + Label (is Box)
  (using TextStyle)
)

(type + Panel (is Box)
  (using TextStyle)
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=10 / node TextStyle seg=owned gen=1 mask=1 ink=1
node TextStyle seg=owned gen=2 mask=1 ink=20 / node TextStyle seg=owned gen=2 mask=1 ink=2
node TextStyle seg=owned gen=3 mask=1 ink=30 / node TextStyle seg=owned gen=2 mask=1 ink=2
node TextStyle seg=owned gen=3 mask=1 ink=30 / node TextStyle seg=owned gen=3 mask=1 ink=1
node TextStyle seg=owned gen=3 mask=1 ink=30 / node TextStyle seg=owned gen=4 mask=1 ink=4
node TextStyle seg=owned gen=3 mask=1 ink=30 / node TextStyle seg=owned gen=4 mask=1 ink=4
node TextStyle seg=owned gen=4 mask=1 ink=20 / node TextStyle seg=owned gen=4 mask=1 ink=4
```

### An element binds a theme tag with `for`, without calling anything: the tag follows the value, which may read tracked state, from the time the element mounts. False turns the tag off beneath the element, null inherits it, and a binding written without a value turns it on.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Scene (id: scene))
  (blank)
  (call show scene)
  (= (. scene is-night) true)
  (call show scene)
  (= (. scene override) false)
  (call show scene)
  (= (. scene override) null)
  (= (. scene is-night) false)
  (call show scene)
)

(fn-do + show _
  (param scene Scene)
  (call (. scene markup-batch-queue checkpoint))
  (print (call (. " / " join) (in (call style-computed-snapshot each) (list (. scene lit) (. scene held) (. scene always)))))
)

(type + Scene (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (prop + is-night _ (= false))
  (prop + override (? Bool) (= null))
  (content
    (<> This
      (<> Box (for: dark #is-night)
        (<> Ink (id: lit)
          (= ink bright)
        )
        (<> Box (for: dark #override)
          (<> Ink (id: held)
            (= ink bright)
          )
        )
      )
      (<> Box (for: dark)
        (<> Ink (id: always)
          (= ink bright)
        )
      )
    )
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(type + Ink (is Box)
  (using TextStyle)
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=10 / node TextStyle seg=owned gen=1 mask=1 ink=10 / node TextStyle seg=owned gen=1 mask=1 ink=20
node TextStyle seg=owned gen=2 mask=1 ink=20 / node TextStyle seg=owned gen=2 mask=1 ink=20 / node TextStyle seg=owned gen=1 mask=1 ink=20
node TextStyle seg=owned gen=2 mask=1 ink=20 / node TextStyle seg=owned gen=3 mask=1 ink=10 / node TextStyle seg=owned gen=1 mask=1 ink=20
node TextStyle seg=owned gen=3 mask=1 ink=10 / node TextStyle seg=owned gen=3 mask=1 ink=10 / node TextStyle seg=owned gen=1 mask=1 ink=20
```

### A class's rule "• This" that applies no other rule takes the StyleSheet slot a line names from the sheets the object sees.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label)
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (!! (. demo %first-child))))
  (blank)
  (call style-use-theme demo DarkTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (!! (. demo %first-child))))
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
  (prop + ink I64 @inherited (= 7))
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(type + Label (is Box)
  (using TextStyle)
  (using AppTheme)
  (rule This
    (= ink bright)
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=10
node TextStyle seg=owned gen=2 mask=1 ink=20
```

### Code reads a StyleSheet slot as "$name" where no style property answers the name: the value of the slot in the node's active theme. A theme switch or a slot write raises the domains the sheet names for each slot on the nodes that read one, and only on those.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (id: label))
    (<> Label (id: other))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (call (. label shade)) " " (. label $tint)))
  (= (. label $is-paint-invalid) false)
  (= (. other $is-paint-invalid) false)
  (blank)
  (call style-use-theme demo DarkTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (. label $is-paint-invalid) " " (. other $is-paint-invalid)))
  (print (str (call (. label shade)) " " (. label $tint)))
  (= (. label $is-paint-invalid) false)
  (blank)
  (let custom (call StyleSheetRecord (!! (call style-sheet-info-named "AppTheme"))))
  (call style-provide-style-sheet demo custom)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (call (. label shade)) " " (. label $is-paint-invalid)))
  (= (. label $is-paint-invalid) false)
  (blank)
  (call (. custom set-slot-named) "bright" 5 false)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (. label $is-paint-invalid) " " (call (. label shade))))
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
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
  (prop + tint (? I64) (= null))
  (blank)
  (style-domains bright (domain paint))
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
  (prop + tint _ (= 3))
)

(type + Label (is Box)
  (using TextStyle)
  (using AppTheme)
  (fn-do + shade I64
    (return $bright)
  )
)
```

```output
10 unknown
true false
20 3
10 true
true 5
```

### "$name" names a slot of only one of the StyleSheets a class uses.

```brix crash
(import /core/style)
(blank)

(main
  (let label (call Label))
  (print (call (. label shade)))
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
)

(style OtherTheme
  (using TextStyle)
  (prop + bright I64 (= 30))
)

(type + Label
  (using TextStyle)
  (using AppTheme)
  (using OtherTheme)
  (fn-do + shade I64
    (return $bright)
  )
)
```

```output
Exception: "bright" is a slot of both AppTheme and OtherTheme
<no file:29>
        (return $bright)
                ^^^^^^^
```

### A domain line of a StyleSheet names one of its slots.

```brix crash
(import /core/style)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
  (blank)
  (style-domains dim (domain paint))
)
```

```output
Exception: StyleSheet slot "dim" does not exist
<no file:12>
      (style-domains dim (domain paint))
                     ^^^
```

### A StyleSheet slot raises its domains on the node that reads it, so they name no target.

```brix crash
(import /core/style)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
  (blank)
  (style-domains bright (domain paint parent))
)
```

```output
Exception: A StyleSheet slot raises its domains on the node that reads it, so they name no target
<no file:12>
      (style-domains bright (domain paint parent))
                                          ^^^^^^
```

### Only the root of a family names the domains of its slots.

```brix crash
(import /core/style)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
  (blank)
  (style-domains bright (domain paint))
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
  (blank)
  (style-domains bright (domain paint))
)
```

```output
Exception: Derived StyleSheet "DarkTheme" names no domains; its slots raise those AppTheme names
<no file:18>
      (style-domains bright (domain paint))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### "$name" in a tracked value of markup reads the slot from the theme of the object the markup belongs to, and is read again when that theme changes as when anything else it tracks changes. An object made after a switch reads the theme it mounts into.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel))
    (<> Holder (id: holder))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (!! (. panel %first-child))))
  (blank)
  (call style-use-theme demo DarkTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (!! (. panel %first-child))))
  (blank)
  (= (. panel flag) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (!! (. panel %first-child))))
  (blank)
  (= (. panel flag) false)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (!! (. panel %first-child))))
  (blank)
  (= (. holder show) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (!! (. (!! (. (!! (. holder %first-child)) %first-child)) %first-child))))
  (blank)
  (call style-use-theme demo AppTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (!! (. panel %first-child))))
  (print (call style-computed-snapshot (!! (. (!! (. (!! (. holder %first-child)) %first-child)) %first-child))))
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
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(type + Label (is Box)
  (using TextStyle)
)

(type + Panel (is Box)
  (using TextStyle)
  (using AppTheme)
  (prop + flag _ (= false))
  (content
    (<> This
      (<> Label
        (= ink (if-expr #flag 1 $bright))
      )
    )
  )
)

(type + Holder (is Box)
  (prop + show _ (= false))
  (content
    (<> This
      (if #show
        (<> Panel)
      )
    )
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=10
node TextStyle seg=owned gen=2 mask=1 ink=20
node TextStyle seg=owned gen=3 mask=1 ink=1
node TextStyle seg=owned gen=4 mask=1 ink=20
node TextStyle seg=owned gen=1 mask=1 ink=20
node TextStyle seg=owned gen=5 mask=1 ink=10
node TextStyle seg=owned gen=2 mask=1 ink=10
```

### "$name" in a tracked value of markup made while another action runs is noted by the value's own action alone: the running action does not depend on what it made, so it runs once.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Host (id: host))
  )
  (blank)
  (let maker (. host maker))
  (call (. demo markup-batch-queue checkpoint))
  (print (str "mounted " (. maker builds)))
  (= (. host view) "b")
  (call (. demo markup-batch-queue checkpoint))
  (print (str "rebuilt " (. maker builds)))
  (= (. (!! (. maker made)) flag) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "flagged " (. maker builds)))
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
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
)

(type + Swatch (is Box)
  (using TextStyle)
)

(type + Panel (is Box)
  (using TextStyle)
  (using AppTheme)
  (prop + flag _ (= false))
  (content
    (<> This
      (<> Swatch
        (= ink (if-expr #flag 1 $bright))
      )
    )
  )
)

(type + Maker
  (prop + builds _ (= 0))
  (prop + made (? Panel))
  (fn-do + make Panel
    (param view String)
    (+= builds 1)
    (let panel (call Panel))
    (= made panel)
    (return panel)
  )
)

(type + Host (is Box)
  (prop + maker _ (= (call Maker)))
  (prop + view _ (= "a"))
  (content
    (<> This
      (call (. maker make) #view)
    )
  )
)
```

```output
mounted 1
rebuilt 2
flagged 2
```

### A "when" of a class's rule "• This" gives a rule a StyleSheet slot, as in "• ink-of:bright".

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (id: label))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot label))
  (= (. label is-on) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot label))
  (call style-use-theme demo DarkTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot label))
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
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style InkRules
  (using TextStyle)
  (rule ink-of
    (param value I64)
    (= ink value)
  )
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(type + Label (is Box)
  (using TextStyle)
  (using InkRules)
  (using AppTheme)
  (prop + is-on _ (= false))
  (blank)
  (rule This
    (when-style #is-on
      (• ink-of bright)
    )
  )
)
```

```output
initial
node TextStyle seg=owned gen=1 mask=1 ink=10
node TextStyle seg=owned gen=2 mask=1 ink=20
```

### A tracked value of a class's rule "• This" may read "$name" and give "this" to a function; both follow the theme.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (id: label))
    (<> Badge (id: badge))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (call style-computed-snapshot label) " / " (call style-computed-snapshot badge)))
  (= (. label is-on) true)
  (= (. badge is-on) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (call style-computed-snapshot label) " / " (call style-computed-snapshot badge)))
  (call style-use-theme demo DarkTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (call style-computed-snapshot label) " / " (call style-computed-snapshot badge)))
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
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(type + Label (is Box)
  (using TextStyle)
  (using AppTheme)
  (prop + is-on _ (= false))
  (blank)
  (rule This
    (= ink (if-expr #is-on $bright 3))
  )
)

(type + Badge (is Box)
  (using TextStyle)
  (using AppTheme)
  (prop + is-on _ (= false))
  (blank)
  (rule This
    (= ink (call shade #is-on this))
  )
)

(fn-expr _ shade I64
  (param on Bool)
  (param node Badge)
  (if-expr on (+ (. node $bright) 1) 3)
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=3 / node TextStyle seg=owned gen=1 mask=1 ink=3
node TextStyle seg=owned gen=2 mask=1 ink=10 / node TextStyle seg=owned gen=2 mask=1 ink=11
node TextStyle seg=owned gen=3 mask=1 ink=20 / node TextStyle seg=owned gen=3 mask=1 ink=21
```

### A StyleSheet whose slot only the value a root "<This" gives a property reads is registered, and the value follows the theme.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (. panel tint))
  (call style-use-theme demo DarkTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (. panel tint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(style AppTheme
  (prop + bright I64 (= 10))
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(type + Panel (is Box)
  (using AppTheme)
  (prop + tint (? I64))
  (content
    (<> This
      (= tint $bright)
    )
  )
)
```

```output
10
20
```

### "$name" alone follows the theme in an attribute of markup, the value of a property, and a line of a class's rule "• This" that sets a property.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (. panel leaf tint) " " (. panel glow) " " (. panel shade)))
  (call style-use-theme demo DarkTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (. panel leaf tint) " " (. panel glow) " " (. panel shade)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(style AppTheme
  (prop + bright I64 (= 10))
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(type + Leaf (is Box)
  (prop + tint (? I64))
)

(type + Panel (is Box)
  (using AppTheme)
  (prop + glow _ (= (+ $bright 1)))
  (prop + shade (? I64))
  (blank)
  (rule This
    (= shade (+ $bright 2))
  )
  (content
    (<> This
      (<> Leaf (id: leaf)
        (= tint $bright)
      )
    )
  )
)
```

```output
10 11 12
20 21 22
```

### Writing a slot of a theme's own record changes it for every subtree whose active member the theme is, once "style-restyle-themes" marks the subtree; other members keep their values.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (id: label))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot label))
  (call style-set-theme-slot AppTheme "bright" 30 false)
  (call style-restyle-themes demo)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot label))
  (call style-use-theme demo DarkTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot label))
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
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(type + Label (is Box)
  (using TextStyle)
  (using AppTheme)
  (rule This
    (= ink bright)
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=10
node TextStyle seg=owned gen=2 mask=1 ink=30
node TextStyle seg=owned gen=3 mask=1 ink=20
```

### A rule a class gets by name from a style sheet its chain uses is one of its defaults wherever the object is made: one its constructor makes outside markup has it too, and follows the theme.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (id: written))
  )
  (blank)
  (let called (call Label))
  (call markup-append demo called)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (call style-computed-snapshot written) " / " (call style-computed-snapshot called)))
  (call style-use-theme demo DarkTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (call style-computed-snapshot written) " / " (call style-computed-snapshot called)))
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
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
  (blank)
  (rule Label
    (= ink bright)
  )
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(type + Label (is Box)
  (using TextStyle)
  (using AppTheme)
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=10 / node TextStyle seg=owned gen=1 mask=1 ink=10
node TextStyle seg=owned gen=2 mask=1 ink=20 / node TextStyle seg=owned gen=2 mask=1 ink=20
```

### A value a theme computes keeps every bit, whatever its two highest are: a float of 2 or more and a large negative integer read back as written.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (id: label))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot label))
  (call style-use-theme demo BigTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot label))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using SizeStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + SizeStyle (is Style)
  (prop + size F64 (= 0))
  (prop + depth I64 (= 0))
)

(style AppTheme
  (using SizeStyle)
  (prop + base F64 (= 3))
  (prop + low I64 (= -5))
  (blank)
  (rule Label
    (= size (* base 2))
    (= depth (* low 4))
  )
)

(style BigTheme (is AppTheme)
  (prop + base _ (= 1000))
  (prop + low _ (= -2000000000000000000))
)

(type + Label (is Box)
  (using SizeStyle)
  (using AppTheme)
)
```

```output
node SizeStyle seg=owned gen=1 mask=3 size=4618441417868443648 depth=-20
node SizeStyle seg=owned gen=2 mask=3 size=4656510908468559872 depth=-8000000000000000000
```

### A rule written in a class's rule "• This" that matches an element its base class's markup makes takes a StyleSheet slot the class uses, though the element's class does not use the sheet.

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
  (print (call style-computed-snapshot (. panel label)))
  (call style-use-theme demo DarkTheme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (. panel label)))
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
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(style InkRules
  (using TextStyle)
  (rule ink-of
    (param value I64)
    (= ink value)
  )
)

(style AppTheme
  (using TextStyle)
  (prop + bright I64 (= 10))
)

(style DarkTheme (is AppTheme) (for dark)
  (prop + bright _ (= 20))
)

(type + Label (is Box)
  (using TextStyle)
  (using InkRules)
)

(type + Frame (is Box)
  (content
    (<> This
      (<> Label (id: label) (• caption))
    )
  )
)

(type + Panel (is Frame)
  (using AppTheme)
  (rule This
    (rule caption (• ink-of bright))
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=10
node TextStyle seg=owned gen=2 mask=1 ink=20
```

### A style rule that gives what it takes straight to a property of a union the runtime holds a handle to cannot take a StyleSheet slot of a case's type: the slot's bits hold no case number.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (• span wide))
  )
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Extent (is Union)
  (variant auto)
  (variant exact F64)
)

(type + TextStyle (is Style)
  (prop + extent Extent (= (. Extent auto)))
)

(style AppTheme
  (prop + wide F64 (= 24))
)

(style SpanRules
  (using TextStyle)
  (rule span
    (param size F64)
    (= extent size)
  )
)

(type + Label (is Box)
  (using TextStyle)
  (using SpanRules)
)
```

```output
Exception: StyleSheet slot "wide" can convert to Extent, but not as a zero-allocation style-sheet projection; declare the slot as Extent or assign a concrete value.
<no file:40>
        (= extent size)
                  ^^^^
```

### A style rule's head takes a slot of the style sheet that declares the rule.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (id: label))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot label))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using Shorthands)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(style Shorthands
  (using TextPaint)
  (rule ink
    (param value AnsiColor)
    (= color value)
  )
)

(style AppTheme
  (using TextPaint)
  (prop + accent AnsiColor (= (call AnsiColor 11)))
  (blank)
  (rule Label (• ink accent))
)

(type + Label (is Box))
```

```output
node TextPaint seg=owned gen=1 mask=1 color=11
```

### A rule of a style sheet reads the slots of a style sheet it uses, in its head and in its body.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Caption (id: caption))
    (<> Footer (id: footer))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot caption))
  (print (call style-computed-snapshot footer))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using Shorthands)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(style Shorthands
  (using TextPaint)
  (rule ink
    (param value AnsiColor)
    (= color value)
  )
)

(style AppTheme
  (using TextPaint)
  (using Metrics)
  (rule Caption (• ink dim))
  (blank)
  (rule Footer
    (= color dim)
  )
)

(style Metrics
  (prop + dim AnsiColor (= (call AnsiColor 3)))
)

(type + Caption (is Box))

(type + Footer (is Box))
```

```output
node TextPaint seg=owned gen=1 mask=1 color=3
node TextPaint seg=owned gen=1 mask=1 color=3
```

### A rule of a style sheet ignores a rule it applies with a value that no class or sheet answers.

```brix
(main
  (<> Thing (id: thing))
  (print "ok")
)

(type + Thing
  (using AppTheme)
)

(style AppTheme
  (rule Thing (• missing 1))
)
```

```output
ok
```

### A keyframe animation a StyleSheet declares is a slot that the builder its head names makes from its tracks. Each track names a property of a style the sheet uses, in a variant too, and a parameter makes a shape that makes its value once for each argument.

```brix
(import /core/style)
(import /time/duration)
(import /units)
(blank)

(main
  (let theme (call AppTheme))
  (for (in line (. theme fade lines))
    (print line)
  )
  (let push (. theme push))
  (let pushed (shape push 4))
  (for (in line (. pushed lines))
    (print line)
  )
  (print (== (as pushed Pointer) (as (shape push 4) Pointer)))
  (let calm (call CalmTheme))
  (for (in line (. calm fade lines))
    (print line)
  )
)

(type + TintStyle (is Style)
  (prop + tone F64 (= 0))
  (prop + spot _ (= (call Spot 0 0)))
  (blank)
  (style-domains tone (domain paint))
  (style-domains spot (domain paint))
)

(style AppTheme
  (using TintStyle)
  (keyframes fade animation 200ms
    (= repeat 3)
    (range tone 0 1 (:after 10ms))
    (track spot.y
      (at 0% 0)
      (at 50% 2.5)
      (at 100% 0)
    )
  )
  (blank)
  (keyframes push (param by F64) animation 100ms
    (range tone _ by)
  )
)

(style CalmTheme (is AppTheme) (for calm)
  (keyframes fade animation 50ms
    (track spot.x
      (at 0% 1)
      (at 100% 0)
    )
  )
)

(type + Spot (is (tuple (: x F64) (: y F64))))

(fn-expr + animation Built
  (param duration Duration)
  (param repeat Int (= 1))
  (call Built (str duration " x" repeat))
)

(type + Built
  (prop + lines (list String))
  (fn-do + This _
    (param head String)
    (= lines (list head))
  )
  (fn-do + (of range Value) This
    (param property (of StyleProperty Value))
    (param from (? Value))
    (param to (? Value))
    (param after Duration (= 0ns))
    (+= lines (str "range " (?. (. property info) name) " " (. property offset) " " from " .. " to " after " after))
    (return this)
  )
  (fn-do + (of track Value) This
    (param property (of StyleProperty Value))
    (+= lines (str "track " (?. (. property info) name) " " (. property offset) " " (. property component)))
    (return this)
  )
  (fn-do + (of at Value) This
    (param property (of StyleProperty Value))
    (param at Ratio)
    (param value Value)
    (+= lines (str "at " at " = " value))
    (return this)
  )
)
```

```output
200000000 x3
range TintStyle 0 0 .. 1 after 10000000
track TintStyle 1 1
at 0 = 0
at 0.5 = 2.5
at 1 = 0
100000000 x1
range TintStyle 0 unknown .. 4 after 0
true
50000000 x1
track TintStyle 1 0
at 0 = 1
at 1 = 0
```

### A property default of a class the program never constructs makes an element whose class rule, written in a style sheet, computes with the sheet's slot and declares more than one style. Nothing the program runs evaluates the default, and the program runs.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let holders (list Holder))
  (print (. holders length))
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
  (prop + weight I64 (= 1))
)

(style AppTheme
  (prop + bright I64 (= 10))
  (blank)
  (rule Leaf
    (= ink (+ bright 1))
    (= weight 2)
  )
)

(type + Leaf (is Box)
  (using TextStyle)
  (using AppTheme)
)

(type + Holder
  (prop + leaf _ (= (call Leaf)))
)
```

```output
0
```
