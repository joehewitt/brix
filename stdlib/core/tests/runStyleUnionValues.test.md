# Style union values

### Union-typed style properties author every case shape — inline scalar, wide value, descriptor object — and read back the active case and payload through $property; wide-value properties box and decode whole values while computed slots stay one I64.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Swatch (id: levels)
      (= fill (call (. TestPaint level) 3))
    )
    (<> Swatch (id: colors)
      (= fill (call (. TestPaint color) 0.25 0.5 0.75 1))
      (= shade (call RGBA 0.5 0.25 0))
    )
    (<> Swatch (id: tints)
      (= fill (call (. TestPaint tint) 0.25 0.5 0.75 1))
    )
    (<> Swatch (id: painted)
      (= fill (call (. TestPaint painter) (call StyleTestPainter 42)))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call (. demo paint-of) levels))
  (print (call (. demo paint-of) colors))
  (print (call (. demo paint-of) tints))
  (print (call (. demo paint-of) painted))
  (print (call (. demo shade-of) colors))
  (print (call (. demo shade-of) levels))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using PaintStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (fn-expr + paint-of String
    (param swatch Box)
    (call paint-label (. swatch $fill))
  )
  (fn-expr + shade-of String
    (param swatch Box)
    (call rgba-label (. swatch $shade))
  )
)

(type + StyleTestPainter
  (prop + tag I64)
  (fn-do + This _
    (param tag _)
  )
)

(type + TestPaint (is Union)
  (variant level (tuple (: n U8)))
  (variant color (tuple (: c RGBA)))
  (variant tint (tuple (: t HSVA)))
  (variant painter (tuple (: p StyleTestPainter)))
)

(type + PaintStyle (is Style)
  (prop + fill TestPaint (= (call (. TestPaint level) 1)))
  (prop + shade RGBA (= (call RGBA 0 0 0)))
)

(type + Swatch (is Box)
  (using PaintStyle)
)

(fn-expr + paint-label String
  (param paint TestPaint)
  (match paint
    (case (call .level n)
      (str "level(" n ")")
    )
    (case (call .color c)
      (call rgba-label c)
    )
    (case (call .tint t)
      (str "tint(" (as (* (. t hue) 100) I64) " " (as (* (. t saturation) 100) I64) " " (as (* (. t value) 100) I64) " " (as (* (. t alpha) 100) I64) ")")
    )
    (case (call .painter p)
      (str "painter(" (. p tag) ")")
    )
  )
)

(fn-expr + rgba-label String
  (param c RGBA)
  (str "color(" (as (* (. c red) 100) I64) " " (as (* (. c green) 100) I64) " " (as (* (. c blue) 100) I64) " " (as (* (. c alpha) 100) I64) ")")
)
```

```output
level(3)
color(25 50 75 100)
tint(25 50 75 100)
painter(42)
color(50 25 0 100)
color(0 0 0 100)
```

### Static rule declarations with wide union cases evaluate once per value site: every application shares the cached interned handle, so the registry grows by one entry per rule value, not per instance. A forced collection cannot reclaim the objects behind those permanent handles.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import /BxGC/BxGC.h)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Swatch (id: a) (• vivid))
    (<> Swatch (id: b) (• vivid))
    (<> Swatch (id: c) (• stamped))
    (<> Swatch (id: d) (• stamped))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (call C:brixCollect)
  (blank)
  (print (str "grew=" (call style-object-live-count)))
  (print (call style-authored-snapshot a))
  (print (call style-authored-snapshot b))
  (print (call style-authored-snapshot c))
  (print (call (. demo paint-of) a))
  (print (call (. demo paint-of) d))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using PaintStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (fn-expr + paint-of String
    (param swatch Box)
    (call paint-label (. swatch $fill))
  )
)

(type + StyleTestPainter
  (prop + tag I64)
  (fn-do + This _
    (param tag _)
  )
)

(type + TestPaint (is Union)
  (variant level (tuple (: n U8)))
  (variant color (tuple (: c RGBA)))
  (variant tint (tuple (: t HSVA)))
  (variant painter (tuple (: p StyleTestPainter)))
)

(type + PaintStyle (is Style)
  (prop + fill TestPaint (= (call (. TestPaint level) 1)))
)

(type + Swatch (is Box)
  (using PaintStyle)
  (rule vivid
    (= fill (call (. TestPaint color) 1 0 0 1))
  )
  (blank)
  (rule stamped
    (= fill (call (. TestPaint painter) (call StyleTestPainter 42)))
  )
)

(fn-expr + paint-label String
  (param paint TestPaint)
  (match paint
    (case (call .level n)
      (str "level(" n ")")
    )
    (case (call .color c)
      (str "color(" (as (* (. c red) 100) I64) " " (as (* (. c green) 100) I64) " " (as (* (. c blue) 100) I64) " " (as (* (. c alpha) 100) I64) ")")
    )
    (case (call .tint t)
      (str "tint(" (as (* (. t hue) 100) I64) " " (as (* (. t saturation) 100) I64) " " (as (* (. t value) 100) I64) " " (as (* (. t alpha) 100) I64) ")")
    )
    (case (call .painter p)
      (str "painter(" (. p tag) ")")
    )
  )
)
```

```output
grew=2
node PaintStyle.fill layer=2 order=0 kind=static bits=257
node PaintStyle.fill layer=2 order=0 kind=static bits=257
node PaintStyle.fill layer=2 order=0 kind=static bits=515
color(100 0 0 100)
painter(42)
```

### Dynamic declarations switch among inline, wide, and descriptor union cases. Forced collection reclaims replaced weak handles without disturbing mounted reads; ordinary unmount preserves the owner template for remount, and owner reclaim clears the last holder.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(import /BxGC/BxGC.h)
(blank)

(main
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo))
  (blank)
  (let base (call churn-paints demo done))
  (call C:brixCollect)
  (do
    (inline (call C:brixGCDrainCleanups))
  )
  (print (str "reclaimed=" (- (call style-object-live-count) base)))
)

(fn-do + churn-paints Size
  (param demo BatchDemo)
  (param done Semaphore)
  (lazy mode 0)
  (blank)
  (<> Swatch (id: swatch)
    (= fill (call paint-for #mode))
  )
  (call markup-append demo swatch)
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let base (call style-object-live-count))
  (print (call (. demo paint-of) swatch))
  (print (str "live=" (- (call style-object-live-count) base)))
  (blank)
  (= mode 1)
  (do-new
    (call (. demo markup-batch-queue checkpoint))
    (print (call (. demo paint-of) swatch))
    (call C:brixCollect)
    (do
      (inline (call C:brixGCDrainCleanups))
    )
    (print (str "live=" (- (call style-object-live-count) base)))
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (= mode 2)
  (do-new
    (call (. demo markup-batch-queue checkpoint))
    (print (call (. demo paint-of) swatch))
    (call C:brixCollect)
    (do
      (inline (call C:brixGCDrainCleanups))
    )
    (print (str "live=" (- (call style-object-live-count) base)))
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (= mode 3)
  (do-new
    (call (. demo markup-batch-queue checkpoint))
    (print (call (. demo paint-of) swatch))
    (call C:brixCollect)
    (do
      (inline (call C:brixGCDrainCleanups))
    )
    (print (str "live=" (- (call style-object-live-count) base)))
    (call markup-remove demo swatch)
    (call (. demo markup-batch-queue checkpoint))
    (print (str "unmounted=" (- (call style-object-live-count) base)))
    (call markup-append demo swatch)
    (call (. demo markup-batch-queue checkpoint))
    (call C:brixCollect)
    (do
      (inline (call C:brixGCDrainCleanups))
    )
    (print (str "remounted=" (call (. demo paint-of) swatch) " live=" (- (call style-object-live-count) base)))
    (call markup-remove demo swatch)
    (call (. demo markup-batch-queue checkpoint))
    (call (. done post))
  )
  (call (. done wait))
  (return base)
)

(fn-expr + paint-for TestPaint
  (param mode I64)
  (match mode
    (case= 1
      (call (. TestPaint color) 0.25 0.5 0.75 1)
    )
    (case= 2
      (call (. TestPaint tint) 0.5 0.25 0.75 1)
    )
    (case= 3
      (call (. TestPaint painter) (call StyleTestPainter 9))
    )
    (default
      (call (. TestPaint level) 5)
    )
  )
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using PaintStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (fn-expr + paint-of String
    (param swatch Box)
    (call paint-label (. swatch $fill))
  )
)

(type + StyleTestPainter
  (prop + tag I64)
  (fn-do + This _
    (param tag _)
  )
)

(type + TestPaint (is Union)
  (variant level (tuple (: n U8)))
  (variant color (tuple (: c RGBA)))
  (variant tint (tuple (: t HSVA)))
  (variant painter (tuple (: p StyleTestPainter)))
)

(type + PaintStyle (is Style)
  (prop + fill TestPaint (= (call (. TestPaint level) 1)))
)

(type + Swatch (is Box)
  (using PaintStyle)
)

(fn-expr + paint-label String
  (param paint TestPaint)
  (match paint
    (case (call .level n)
      (str "level(" n ")")
    )
    (case (call .color c)
      (str "color(" (as (* (. c red) 100) I64) " " (as (* (. c green) 100) I64) " " (as (* (. c blue) 100) I64) " " (as (* (. c alpha) 100) I64) ")")
    )
    (case (call .tint t)
      (str "tint(" (as (* (. t hue) 100) I64) " " (as (* (. t saturation) 100) I64) " " (as (* (. t value) 100) I64) " " (as (* (. t alpha) 100) I64) ")")
    )
    (case (call .painter p)
      (str "painter(" (. p tag) ")")
    )
  )
)
```

```output
level(5)
live=0
color(25 50 75 100)
live=1
tint(50 25 75 100)
live=1
painter(9)
live=1
unmounted=1
remounted=painter(9) live=1
reclaimed=0
```

### Optional union and wide properties preserve null versus present: absent values store zero bits with the none flag and decode to null without touching the registry, and dynamic declarations toggle between null and present cases.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (lazy has-accent false)
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo)
    (<> Swatch (id: plain))
    (<> Swatch (id: fancy)
      (= accent (call (. TestPaint tint) 0.5 0.25 0.75 1))
    )
    (<> Swatch (id: toggly)
      (= accent (call accent-for #has-accent))
    )
    (<> Swatch (id: veiled)
      (= veil (call RGBA 0.25 0.25 0.5))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call (. demo accent-of) plain))
  (print (call (. demo accent-of) fancy))
  (print (call (. demo accent-of) toggly))
  (print (call (. demo veil-of) veiled))
  (print (call (. demo veil-of) plain))
  (blank)
  (= has-accent true)
  (do-new
    (call (. demo markup-batch-queue checkpoint))
    (print (call (. demo accent-of) toggly))
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (= has-accent false)
  (do-new
    (call (. demo markup-batch-queue checkpoint))
    (print (call (. demo accent-of) toggly))
    (call (. done post))
  )
  (call (. done wait))
)

(fn-do + accent-for (? TestPaint)
  (param has Bool)
  (return-if (call (. TestPaint level) 9) (if has))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using PaintStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (fn-do + accent-of String
    (param swatch Box)
    (let accent (. swatch $accent))
    (return-if (call paint-label accent) (if accent))
    (return "null")
  )
  (fn-do + veil-of String
    (param swatch Box)
    (let veil (. swatch $veil))
    (return-if (call rgba-label veil) (if veil))
    (return "null")
  )
)

(type + StyleTestPainter
  (prop + tag I64)
  (fn-do + This _
    (param tag _)
  )
)

(type + TestPaint (is Union)
  (variant level (tuple (: n U8)))
  (variant color (tuple (: c RGBA)))
  (variant tint (tuple (: t HSVA)))
  (variant painter (tuple (: p StyleTestPainter)))
)

(type + PaintStyle (is Style)
  (prop + accent (? TestPaint) (= null))
  (prop + veil (? RGBA) (= null))
)

(type + Swatch (is Box)
  (using PaintStyle)
)

(fn-expr + paint-label String
  (param paint TestPaint)
  (match paint
    (case (call .level n)
      (str "level(" n ")")
    )
    (case (call .color c)
      (call rgba-label c)
    )
    (case (call .tint t)
      (str "tint(" (as (* (. t hue) 100) I64) " " (as (* (. t saturation) 100) I64) " " (as (* (. t value) 100) I64) " " (as (* (. t alpha) 100) I64) ")")
    )
    (case (call .painter p)
      (str "painter(" (. p tag) ")")
    )
  )
)

(fn-expr + rgba-label String
  (param c RGBA)
  (str "color(" (as (* (. c red) 100) I64) " " (as (* (. c green) 100) I64) " " (as (* (. c blue) 100) I64) " " (as (* (. c alpha) 100) I64) ")")
)
```

```output
null
tint(50 25 75 100)
null
color(25 25 50 100)
null
level(9)
null
```

### StyleSheet slots hold union values: registration encodes typed defaults, provider records carry scalar and interned wide cases, and slot changes settle and invalidate dependents. The debug snapshot marks encoded slots so handles never read as plain numbers.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Swatch (id: swatch)
      (= fill glow)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. demo paint-of) swatch))
  (blank)
  (let theme (call StyleSheetRecord (!! (call style-sheet-info-named "PaintTheme"))))
  (call (. theme set-slot-named) "glow" (/lshift/ 9 8) false)
  (call style-provide-style-sheet demo theme)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. demo paint-of) swatch))
  (blank)
  (let held (call style-intern-holder (call (of StyleWideHolder RGBA) (call RGBA 0.25 0.5 0.75))))
  (call (. theme set-slot-named) "glow" (/or/ (/lshift/ held 8) 1) false)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. demo paint-of) swatch))
  (print (str "glow=enc(" (call (. theme value-bits) 0) ")"))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using PaintStyle)
  (using PaintTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (fn-expr + paint-of String
    (param swatch Box)
    (call paint-label (. swatch $fill))
  )
)

(type + StyleTestPainter
  (prop + tag I64)
  (fn-do + This _
    (param tag _)
  )
)

(type + TestPaint (is Union)
  (variant level (tuple (: n U8)))
  (variant color (tuple (: c RGBA)))
  (variant tint (tuple (: t HSVA)))
  (variant painter (tuple (: p StyleTestPainter)))
)

(style PaintTheme
  (prop + glow TestPaint (= (call (. TestPaint level) 7)))
)

(type + PaintStyle (is Style)
  (prop + fill TestPaint (= (call (. TestPaint level) 1)))
)

(type + Swatch (is Box)
  (using PaintStyle)
)

(fn-expr + paint-label String
  (param paint TestPaint)
  (match paint
    (case (call .level n)
      (str "level(" n ")")
    )
    (case (call .color c)
      (str "color(" (as (* (. c red) 100) I64) " " (as (* (. c green) 100) I64) " " (as (* (. c blue) 100) I64) " " (as (* (. c alpha) 100) I64) ")")
    )
    (case (call .tint t)
      (str "tint(" (as (* (. t hue) 100) I64) " " (as (* (. t saturation) 100) I64) " " (as (* (. t value) 100) I64) " " (as (* (. t alpha) 100) I64) ")")
    )
    (case (call .painter p)
      (str "painter(" (. p tag) ")")
    )
  )
)
```

```output
level(7)
level(9)
color(25 50 75 100)
glow=enc(257)
```

### StyleSheet slot projections wrap reusable payload bits into union style properties without decoding: RGBA and HSVA reuse wide-value handles, and object payloads reuse descriptor handles.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Swatch (id: color)
      (= fill accent-color)
    )
    (<> Swatch (id: tint)
      (= fill accent-tint)
    )
    (<> Swatch (id: painted)
      (= fill accent-painter)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call (. demo paint-of) color))
  (print (call (. demo paint-of) tint))
  (print (call (. demo paint-of) painted))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using PaintStyle)
  (using PaintTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (fn-expr + paint-of String
    (param swatch Box)
    (call paint-label (. swatch $fill))
  )
)

(type + StyleTestPainter
  (prop + tag I64)
  (fn-do + This _
    (param tag _)
  )
)

(type + TestPaint (is Union)
  (variant level (tuple (: n U8)))
  (variant color (tuple (: c RGBA)))
  (variant tint (tuple (: t HSVA)))
  (variant painter (tuple (: p StyleTestPainter)))
)

(style PaintTheme
  (prop + accent-color RGBA (= (call RGBA 0.25 0.5 0.75 1)))
  (prop + accent-tint HSVA (= (call HSVA 0.5 0.25 0.75 1)))
  (prop + accent-painter StyleTestPainter (= (call StyleTestPainter 77)))
)

(type + PaintStyle (is Style)
  (prop + fill TestPaint (= (call (. TestPaint level) 1)))
)

(type + Swatch (is Box)
  (using PaintStyle)
)

(fn-expr + paint-label String
  (param paint TestPaint)
  (match paint
    (case (call .level n)
      (str "level(" n ")")
    )
    (case (call .color c)
      (str "color(" (as (* (. c red) 100) I64) " " (as (* (. c green) 100) I64) " " (as (* (. c blue) 100) I64) " " (as (* (. c alpha) 100) I64) ")")
    )
    (case (call .tint t)
      (str "tint(" (as (* (. t hue) 100) I64) " " (as (* (. t saturation) 100) I64) " " (as (* (. t value) 100) I64) " " (as (* (. t alpha) 100) I64) ")")
    )
    (case (call .painter p)
      (str "painter(" (. p tag) ")")
    )
  )
)
```

```output
color(25 50 75 100)
tint(50 25 75 100)
painter(77)
```

### Union-to-union StyleSheet projections remap tags while preserving payload bits, and repeated construction of the same converted markup site does not re-register remap metadata per instance.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Swatch (id: swatch)
      (= fill accent)
    )
    (for (in item (. demo #items))
      (<> Swatch
        (= fill accent)
      )
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (str "remaps=" (call style-projection-remap-site-count)))
  (print (str "attempts=" (call style-projection-remap-registration-attempt-count)))
  (print (call (. demo paint-of) swatch))
  (blank)
  (let theme (call StyleSheetRecord (!! (call style-sheet-info-named "PaintTheme"))))
  (let held (call style-intern-holder (call (of StyleWideHolder RGBA) (call RGBA 0.25 0.5 0.75 1))))
  (call (. theme set-slot-named) "accent" (/or/ (/lshift/ held 8) 0) false)
  (call style-provide-style-sheet demo theme)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call (. demo paint-of) swatch))
  (print (str "remaps=" (call style-projection-remap-site-count)))
  (print (str "attempts=" (call style-projection-remap-registration-attempt-count)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using PaintStyle)
  (using PaintTheme)
  (prop + items (list String) (= (call (of List String) (list "a" "b" "c" "d"))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (fn-expr + paint-of String
    (param swatch Box)
    (call paint-label (. swatch $fill))
  )
)

(type + SourcePaint (is Union)
  (variant color (tuple (: c RGBA)))
  (variant level (tuple (: n U8)))
)

(type + TestPaint (is Union)
  (variant level (tuple (: n U8)))
  (variant color (tuple (: c RGBA)))
)

(style PaintTheme
  (prop + accent SourcePaint (= (call (. SourcePaint level) 7)))
)

(type + PaintStyle (is Style)
  (prop + fill TestPaint (= (call (. TestPaint level) 1)))
)

(type + Swatch (is Box)
  (using PaintStyle)
)

(fn-expr + paint-label String
  (param paint TestPaint)
  (match paint
    (case (call .level n)
      (str "level(" n ")")
    )
    (case (call .color c)
      (str "color(" (as (* (. c red) 100) I64) " " (as (* (. c green) 100) I64) " " (as (* (. c blue) 100) I64) " " (as (* (. c alpha) 100) I64) ")")
    )
  )
)
```

```output
remaps=2
attempts=2
level(7)
color(25 50 75 100)
remaps=2
attempts=2
```

### Rule arguments flow into union case constructors of every case shape — inline scalar, wide value, descriptor object — and each declaration records exactly the constructed case.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Swatch (id: levels) (• level 3))
    (<> Swatch (id: colors) (• color (call RGBA 0.25 0.5 0.75 1)))
    (<> Swatch (id: tints) (• tint (call HSVA 0.25 0.5 0.75 1)))
    (<> Swatch (id: painted) (• painter (call StyleTestPainter 42)))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call (. demo paint-of) levels))
  (print (call (. demo paint-of) colors))
  (print (call (. demo paint-of) tints))
  (print (call (. demo paint-of) painted))
  (print (call style-authored-snapshot levels))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using PaintStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (fn-expr + paint-of String
    (param swatch Box)
    (call paint-label (. swatch $fill))
  )
)

(type + StyleTestPainter
  (prop + tag I64)
  (fn-do + This _
    (param tag _)
  )
)

(type + TestPaint (is Union)
  (variant level (tuple (: n U8)))
  (variant color (tuple (: c RGBA)))
  (variant tint (tuple (: t HSVA)))
  (variant painter (tuple (: p StyleTestPainter)))
)

(type + PaintStyle (is Style)
  (prop + fill TestPaint (= (call (. TestPaint level) 1)))
)

(type + Swatch (is Box)
  (using PaintStyle)
  (rule level
    (param n U8)
    (= fill (call (. TestPaint level) n))
  )
  (blank)
  (rule color
    (param c RGBA)
    (= fill (call (. TestPaint color) c))
  )
  (blank)
  (rule tint
    (param t HSVA)
    (= fill (call (. TestPaint tint) t))
  )
  (blank)
  (rule painter
    (param p StyleTestPainter)
    (= fill (call (. TestPaint painter) p))
  )
)

(fn-expr + paint-label String
  (param paint TestPaint)
  (match paint
    (case (call .level n)
      (str "level(" n ")")
    )
    (case (call .color c)
      (str "color(" (as (* (. c red) 100) I64) " " (as (* (. c green) 100) I64) " " (as (* (. c blue) 100) I64) " " (as (* (. c alpha) 100) I64) ")")
    )
    (case (call .tint t)
      (str "tint(" (as (* (. t hue) 100) I64) " " (as (* (. t saturation) 100) I64) " " (as (* (. t value) 100) I64) " " (as (* (. t alpha) 100) I64) ")")
    )
    (case (call .painter p)
      (str "painter(" (. p tag) ")")
    )
  )
)
```

```output
level(3)
color(25 50 75 100)
tint(25 50 75 100)
painter(42)
node PaintStyle.fill layer=2 order=0 kind=static bits=768
```

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Swatch (id: swatch))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. swatch label)))
  (blank)
  (= (. swatch stamp) (call Stamp 2))
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. swatch label)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Stamp
  (prop + tag I64)
  (fn-do + This _
    (param tag _)
  )
)

(type + Paint (is Union)
  (variant level (tuple (: n U8)))
  (variant stamped (tuple (: stamp Stamp)))
)

(type + PaintStyle (is Style)
  (prop + fill Paint (= (call (. Paint level) 1)))
)

(type + Swatch (is Box)
  (using PaintStyle)
  (prop + stamp _ (= (call Stamp 1)))
  (blank)
  (rule This
    (= fill (call (. Paint stamped) #stamp))
  )
  (fn-expr + label String
    (match $fill
      (case (call .level n)
        (str "level(" n ")")
      )
      (case (call .stamped stamp)
        (str "stamped(" (. stamp tag) ")")
      )
    )
  )
)
```

```output
stamped(1)
stamped(2)
```
