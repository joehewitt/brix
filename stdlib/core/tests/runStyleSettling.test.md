# Style settling

### Settling resolves authored priority (inline beats rule, later rule beats earlier), @inherited properties default to the parent's computed value (the initializer is the root fallback), explicit inherit overrides a lower-priority concrete declaration, and changed declared properties raise persistent per-node domain bits summarized in the root's pending counts.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (• accent) (• loud)
      (= ink 9)
      (<> Label (• padded)
        (= pad inherit)
      )
      (<> Plain)
    )
    (<> Label
      (= margin 3)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  ; Clear the container bits Box raised imperatively while children were appended, leaving only
  ; style-driven pending state.
  (= (. demo $is-paint-invalid) false)
  (= (. demo $is-layout-invalid) false)
  (blank)
  (let label (!! (. demo %first-child)))
  (let padder (!! (. label %first-child)))
  (let plain (!! (. padder %next-sibling)))
  (let placed (!! (. label %next-sibling)))
  (blank)
  (print (call style-computed-snapshot label))
  (print (call style-computed-snapshot padder))
  (print (call style-computed-snapshot plain))
  (print (call style-computed-snapshot placed))
  (print (call style-pending-snapshot demo))
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
  (prop + weight I64 (= 1))
  (prop + pad I64 @inherited (= 0))
  (prop + margin I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
  (style-domains weight (domain layout))
  (style-domains pad (domain layout))
  (style-domains margin (domain layout))
)

(type + Label (is Box)
  (using TextStyle)
  (rule accent
    (= ink 13)
    (= weight 4)
    (= pad 5)
  )
  (blank)
  (rule loud
    (= weight 6)
  )
  (blank)
  (rule padded
    (= pad 9)
  )
)

(type + Plain (is Box))
```

```output
node TextStyle seg=owned gen=1 mask=7 ink=9 weight=6 pad=5 margin=0
node TextStyle seg=owned gen=1 mask=5 ink=9 weight=1 pad=5 margin=0
node TextStyle seg=owned gen=1 mask=5 ink=9 weight=1 pad=5 margin=0
node TextStyle seg=owned gen=1 mask=8 ink=7 weight=1 pad=0 margin=3
paint=3 layout=4
```

### A style whose properties are all inherited flows through descendants by pointer aliasing: only the authoring node owns a segment, and the whole subtree shares it.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink 9)
      (<> Plain
        (<> Plain)
      )
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let label (!! (. demo %first-child)))
  (let mid (!! (. label %first-child)))
  (let leaf (!! (. mid %first-child)))
  (blank)
  (print (call style-computed-snapshot label))
  (print (call style-computed-snapshot mid))
  (print (call style-computed-snapshot leaf))
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

(type + Label (is Box)
  (using TextStyle)
)

(type + Plain (is Box))
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=9
node TextStyle seg=parent gen=1 mask=1 ink=9
node TextStyle seg=parent gen=1 mask=1 ink=9
```

### A dynamic declaration whose dependency changes but whose value stays equal raises no pending domain bits; a real change raises its declared domain, and markup-batch-did-flush observes already-settled computed values.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (lazy base 12)
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink (* (/ #base 10) 100))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let label (!! (. demo %first-child)))
  (print (call style-computed-snapshot label))
  (blank)
  (= (. label $is-paint-invalid) false)
  (= (. demo $is-paint-invalid) false)
  (= (. demo $is-layout-invalid) false)
  (= base 13)
  (blank)
  (do-new
    (print (+ "equal " (call style-pending-snapshot demo)))
    (print (call style-computed-snapshot label))
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (= (. label $is-paint-invalid) false)
  (= base 40)
  (blank)
  (do-new
    (print (+ "changed " (call style-pending-snapshot demo)))
    (print (call style-computed-snapshot label))
    (call (. done post))
  )
  (call (. done wait))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let child %first-child)
    (if child
      (call print (+ "did-flush " (call style-computed-snapshot child)))
    )
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
did-flush node TextStyle seg=owned gen=1 mask=1 ink=100
node TextStyle seg=owned gen=1 mask=1 ink=100
did-flush node TextStyle seg=owned gen=1 mask=1 ink=100
equal paint=0 layout=0
node TextStyle seg=owned gen=1 mask=1 ink=100
did-flush node TextStyle seg=owned gen=2 mask=1 ink=400
changed paint=1 layout=0
node TextStyle seg=owned gen=2 mask=1 ink=400
```

### Boxed wide values compare by value. A dynamic declaration that re-evaluates to an equal RGBA or an equal wide union case raises no pending domain bits; a different value, a different union case holding the same fields, and a distinct descriptor object each raise the declared domain.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (lazy shade-step 0)
  (lazy fill-step 0)
  (lazy painter (call StyleTestPainter 7))
  (blank)
  (<> BatchDemo (id: demo)
    (<> Swatch (id: swatch)
      (= shade (call shade-for #shade-step))
      (= fill (call fill-for #fill-step #painter))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (= (. swatch $is-paint-invalid) false)
  (print (str "start shade=" (call rgba-label (. swatch $shade)) " fill=" (call paint-label (. swatch $fill))))
  (blank)
  (for (in step (..< 1 4))
    (= shade-step step)
    (print (str "shade " step " " (call settled-label demo swatch)))
  )
  (blank)
  (for (in step (..< 1 9))
    (= fill-step step)
    (print (str "fill " step " " (call settled-label demo swatch)))
  )
)

(fn-do + settled-label String
  (param demo BatchDemo)
  (param swatch Swatch)
  (call (. demo markup-batch-queue checkpoint))
  (let label (str "paint=" (. swatch $is-paint-invalid) " shade=" (call rgba-label (. swatch $shade)) " fill=" (call paint-label (. swatch $fill))))
  (= (. swatch $is-paint-invalid) false)
  (return label)
)

(fn-expr + shade-for RGBA
  (param step I64)
  (case? (< step 2)
    (call RGBA 0.5 0.25 0)
  )
  (default
    (call RGBA 0.75 0.25 0)
  )
)

(fn-expr + fill-for TestPaint
  (param step I64)
  (param painter StyleTestPainter)
  (case? (< step 2)
    (call (. TestPaint color) 0.25 0.5 0.75 1)
  )
  (case? (< step 4)
    (call (. TestPaint tint) 0.25 0.5 0.75 1)
  )
  (case? (< step 6)
    (call (. TestPaint level) 3)
  )
  (case? (< step 8)
    (call (. TestPaint painter) painter)
  )
  (default
    (call (. TestPaint painter) (call StyleTestPainter 7))
  )
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
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
  (blank)
  (style-domains fill (domain paint))
  (style-domains shade (domain paint))
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
      (str "tint(" (as (* (. t hue) 100) I64) " " (as (* (. t saturation) 100) I64) " " (as (* (. t value) 100) I64) ")")
    )
    (case (call .painter p)
      (str "painter(" (. p tag) ")")
    )
  )
)

(fn-expr + rgba-label String
  (param c RGBA)
  (str "color(" (as (* (. c red) 100) I64) " " (as (* (. c green) 100) I64) " " (as (* (. c blue) 100) I64) ")")
)
```

```output
start shade=color(50 25 0) fill=color(25 50 75)
shade 1 paint=false shade=color(50 25 0) fill=color(25 50 75)
shade 2 paint=true shade=color(75 25 0) fill=color(25 50 75)
shade 3 paint=false shade=color(75 25 0) fill=color(25 50 75)
fill 1 paint=false shade=color(75 25 0) fill=color(25 50 75)
fill 2 paint=true shade=color(75 25 0) fill=tint(25 50 75)
fill 3 paint=false shade=color(75 25 0) fill=tint(25 50 75)
fill 4 paint=true shade=color(75 25 0) fill=level(3)
fill 5 paint=false shade=color(75 25 0) fill=level(3)
fill 6 paint=true shade=color(75 25 0) fill=painter(7)
fill 7 paint=false shade=color(75 25 0) fill=painter(7)
fill 8 paint=true shade=color(75 25 0) fill=painter(7)
```

### Equal wide values authored by different declarations settle without raising domains: enabling a conditional block whose values equal the unconditional ones swaps the published handles only. Inline union payloads are never read as handles, so every level change raises its domain even when the payloads name registry slots that hold equal boxed values.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let tone (call RGBA 0.5 0.25 0))
  (lazy level U8 0)
  (blank)
  (<> BatchDemo (id: demo)
    (<> Widget (id: widget))
    (<> Chip
      (= shade tone)
    )
    (<> Chip
      (= shade tone)
    )
    (<> Chip
      (= shade tone)
    )
    (<> Chip
      (= shade tone)
    )
    (<> Gauge (id: gauge)
      (= fill (call (. TestPaint level) #level))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (= (. widget $is-paint-invalid) false)
  (= (. gauge $is-paint-invalid) false)
  (print (str "start shade=" (call rgba-label (. widget $shade)) " fill=" (call paint-label (. widget $fill))))
  (blank)
  (= (. widget is-hot) true)
  (print (str "hot " (call settled-label demo widget)))
  (= (. widget is-cold) true)
  (print (str "cold " (call settled-label demo widget)))
  (= (. widget is-cold) false)
  (print (str "hot again " (call settled-label demo widget)))
  (= (. widget is-hot) false)
  (print (str "plain " (call settled-label demo widget)))
  (blank)
  (let raised 0)
  (for (in next (..< 1 65))
    (= level (as next U8))
    (call (. demo markup-batch-queue checkpoint))
    (if (. gauge $is-paint-invalid)
      (+= raised 1)
    )
    (= (. gauge $is-paint-invalid) false)
  )
  (print (str "level changes raised=" raised))
)

(fn-do + settled-label String
  (param demo BatchDemo)
  (param widget Widget)
  (call (. demo markup-batch-queue checkpoint))
  (let label (str "paint=" (. widget $is-paint-invalid) " shade=" (call rgba-label (. widget $shade)) " fill=" (call paint-label (. widget $fill))))
  (= (. widget $is-paint-invalid) false)
  (return label)
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TestPaint (is Union)
  (variant level (tuple (: n U8)))
  (variant color (tuple (: c RGBA)))
  (variant tint (tuple (: t HSVA)))
)

(type + PaintStyle (is Style)
  (prop + fill TestPaint (= (call (. TestPaint level) 1)))
  (prop + shade RGBA (= (call RGBA 0 0 0)))
  (blank)
  (style-domains fill (domain paint))
  (style-domains shade (domain paint))
)

(type + Widget (is Box)
  (using PaintStyle)
  (prop + is-hot _ (= false))
  (prop + is-cold _ (= false))
  (blank)
  (rule This
    (= shade (call RGBA 0.5 0.25 0))
    (= fill (call (. TestPaint color) 0.25 0.5 0.75 1))
    (blank)
    (when-style #is-hot
      (= shade (call RGBA 0.5 0.25 0))
      (= fill (call (. TestPaint color) 0.25 0.5 0.75 1))
    )
    (blank)
    (when-style #is-cold
      (= shade (call RGBA 0 0.25 0.5))
      (= fill (call (. TestPaint tint) 0.25 0.5 0.75 1))
    )
  )
)

(type + Chip (is Box)
  (using PaintStyle)
)

(type + Gauge (is Box)
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
      (str "tint(" (as (* (. t hue) 100) I64) " " (as (* (. t saturation) 100) I64) " " (as (* (. t value) 100) I64) ")")
    )
  )
)

(fn-expr + rgba-label String
  (param c RGBA)
  (str "color(" (as (* (. c red) 100) I64) " " (as (* (. c green) 100) I64) " " (as (* (. c blue) 100) I64) ")")
)
```

```output
start shade=color(50 25 0) fill=color(25 50 75)
hot paint=false shade=color(50 25 0) fill=color(25 50 75)
cold paint=true shade=color(0 25 50) fill=tint(25 50 75)
hot again paint=true shade=color(50 25 0) fill=color(25 50 75)
plain paint=false shade=color(50 25 0) fill=color(25 50 75)
level changes raised=64
```

### An inherited wide value replaced by an equal one republishes its handle without raising domains: descendants sharing the segment and descendants holding their own copy both read the live handle after the replaced holder is collected. A different value raises the whole chain.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import /BxGC/BxGC.h)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Tinted (id: parent)
      (<> Tinted (id: shared)
        (<> Tinted (id: copied)
          (= gain 2)
        )
      )
    )
  )
  (blank)
  (= (. parent echo) (call RGBA 0.5 0.25 0))
  (call (. demo markup-batch-queue checkpoint))
  (call clear-paint parent shared copied)
  (print (str "start " (call echo-label parent shared copied)))
  (blank)
  (= (. parent echo) (call RGBA 0 0.25 0.5))
  (= (. parent echo) (call RGBA 0.5 0.25 0))
  (call (. demo markup-batch-queue checkpoint))
  (call C:brixCollect)
  (do
    (inline (call C:brixGCDrainCleanups))
  )
  (print (str "equal " (call echo-label parent shared copied)))
  (call clear-paint parent shared copied)
  (blank)
  (= (. parent echo) (call RGBA 0 0.25 0.5))
  (call (. demo markup-batch-queue checkpoint))
  (call C:brixCollect)
  (do
    (inline (call C:brixGCDrainCleanups))
  )
  (print (str "changed " (call echo-label parent shared copied)))
)

(fn-do + clear-paint _
  (param parent Tinted)
  (param shared Tinted)
  (param copied Tinted)
  (= (. parent $is-paint-invalid) false)
  (= (. shared $is-paint-invalid) false)
  (= (. copied $is-paint-invalid) false)
)

(fn-expr + echo-label String
  (param parent Tinted)
  (param shared Tinted)
  (param copied Tinted)
  (str "paint=" (. parent $is-paint-invalid) "/" (. shared $is-paint-invalid) "/" (. copied $is-paint-invalid) " echo=" (call rgba-label (. parent $echo)) "/" (call rgba-label (. shared $echo)) "/" (call rgba-label (. copied $echo)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + EchoStyle (is Style)
  (prop + echo RGBA @inherited (= (call RGBA 0 0 0)))
  (prop + gain I64 @inherited (= 0))
  (blank)
  (style-domains echo (domain paint))
  (style-domains gain (domain paint))
)

(type + Tinted (is Box)
  (using EchoStyle)
)

(fn-expr + rgba-label String
  (param c RGBA)
  (str "color(" (as (* (. c red) 100) I64) " " (as (* (. c green) 100) I64) " " (as (* (. c blue) 100) I64) ")")
)
```

```output
start paint=false/false/false echo=color(50 25 0)/color(50 25 0)/color(50 25 0)
equal paint=false/false/false echo=color(50 25 0)/color(50 25 0)/color(50 25 0)
changed paint=true/true/true echo=color(0 25 50)/color(0 25 50)/color(0 25 50)
```

### Moving a box between parents through the Box API (remove + append) unmounts and remounts the style attachment; the remounted subtree re-derives inherited values from the new parent.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink 2)
      (<> Plain)
    )
    (<> Label
      (= ink 4)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let pa (!! (. demo %first-child)))
  (let child (!! (. pa %first-child)))
  (let pb (!! (. pa %next-sibling)))
  (blank)
  (print (call style-computed-snapshot child))
  (blank)
  (call markup-remove pa child)
  (call markup-append pb child)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-computed-snapshot child))
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

(type + Label (is Box)
  (using TextStyle)
)

(type + Plain (is Box))
```

```output
node TextStyle seg=parent gen=1 mask=1 ink=2
node TextStyle seg=parent gen=1 mask=1 ink=4
```

### Raising a domain bit on a deep node marks the conservative descendant rollup on every ancestor, read through $is-<domain>-invalid-below. Clearing the node's own bit leaves rollups alone (phase walks clear them after visiting), and untouched sibling subtrees stay clean throughout.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (<> Plain
        (<> Plain)
      )
    )
    (<> Label)
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let branch (!! (. demo %first-child)))
  (let mid (!! (. branch %first-child)))
  (let leaf (!! (. mid %first-child)))
  (let sibling (!! (. branch %next-sibling)))
  (blank)
  ; Clear the bits and rollups Box raised imperatively while children were appended
  (= (. demo $is-layout-invalid) false)
  (= (. demo $is-layout-invalid-below) false)
  (= (. branch $is-layout-invalid) false)
  (= (. branch $is-layout-invalid-below) false)
  (= (. mid $is-layout-invalid) false)
  (= (. mid $is-layout-invalid-below) false)
  (= (. leaf $is-layout-invalid) false)
  (= (. sibling $is-layout-invalid) false)
  (blank)
  (= (. leaf $is-layout-invalid) true)
  (blank)
  (let raised-demo (. demo $is-layout-invalid-below))
  (let raised-mid (. mid $is-layout-invalid-below))
  (let raised-sibling (. sibling $is-layout-invalid-below))
  (print (str "raised demo=" raised-demo " mid=" raised-mid " sibling=" raised-sibling))
  (blank)
  (= (. leaf $is-layout-invalid) false)
  (blank)
  (let cleared-own (. leaf $is-layout-invalid))
  (let sticky-demo (. demo $is-layout-invalid-below))
  (print (str "cleared own=" cleared-own " demo-below=" sticky-demo))
  (blank)
  (= (. demo $is-layout-invalid-below) false)
  (= (. mid $is-layout-invalid-below) false)
  (blank)
  (let final-demo (. demo $is-layout-invalid-below))
  (let final-mid (. mid $is-layout-invalid-below))
  (print (str "walked demo=" final-demo " mid=" final-mid))
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

(type + Label (is Box)
  (using TextStyle)
)

(type + Plain (is Box))
```

```output
raised demo=true mid=true sibling=false
cleared own=false demo-below=true
walked demo=false mid=false
```

### The `$is-<domain>-invalid` accessors preserve the accounting split: clearing own clears pending-domain counts, while clearing below clears only the descendant rollup.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Box
      (<> Box (id: leaf))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let branch (!! (. demo %first-child)))
  (blank)
  (= (. demo $is-layout-invalid) false)
  (= (. demo $is-layout-invalid-below) false)
  (= (. branch $is-layout-invalid) false)
  (= (. branch $is-layout-invalid-below) false)
  (= (. leaf $is-layout-invalid) false)
  (blank)
  (= (. leaf $is-layout-invalid) true)
  (print (str "raised branch-own=" (. branch $is-layout-invalid) " branch-below=" (. branch $is-layout-invalid-below) " leaf-own=" (. leaf $is-layout-invalid) " leaf-below=" (. leaf $is-layout-invalid-below)))
  (blank)
  (= (. leaf $is-layout-invalid) false)
  (let pending-after-own (call style-domain-pending-count demo "layout"))
  (print (str "cleared-own leaf-own=" (. leaf $is-layout-invalid) " leaf-below=" (. leaf $is-layout-invalid-below) " branch-own=" (. branch $is-layout-invalid) " branch-below=" (. branch $is-layout-invalid-below) " pending=" pending-after-own))
  (blank)
  (= (. branch $is-layout-invalid-below) false)
  (print (str "cleared-below branch-own=" (. branch $is-layout-invalid) " branch-below=" (. branch $is-layout-invalid-below)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)
```

```output
raised branch-own=false branch-below=true leaf-own=true leaf-below=false
cleared-own leaf-own=false leaf-below=false branch-own=false branch-below=true pending=0
cleared-below branch-own=false branch-below=false
```

### An element a branch builds after mount is put under the batch before it is linked; once linked, the domain bits it and its children raised while it was built reach the rollup of every ancestor.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (if (. demo #shown)
      (<> Box
        (<> Box
          (<> Box)
        )
      )
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (= (. demo shown) true)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let region (!! (. demo %first-child)))
  (let built (!! (. region %first-child)))
  (let inner (!! (. built %first-child)))
  (print (str "region own=" (. region $is-layout-invalid) " below=" (. region $is-layout-invalid-below)))
  (print (str "built own=" (. built $is-layout-invalid) " below=" (. built $is-layout-invalid-below)))
  (print (str "inner own=" (. inner $is-layout-invalid) " below=" (. inner $is-layout-invalid-below)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + shown _ (= false))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)
```

```output
region own=true below=true
built own=true below=true
inner own=true below=false
```

### Markup batch flush summaries expose ordinary domain booleans backed by the same domain indexes as node pending bits. Dynamic style changes report the domains still pending after settling.

```brix
(import /core/style)
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (lazy ink-base 1)
  (lazy gap-base 1)
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink #ink-base)
      (= gap #gap-base)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let label (!! (. demo %first-child)))
  (= (. label $is-paint-invalid) false)
  (= (. label $is-layout-invalid) false)
  (= (. demo $is-paint-invalid) false)
  (= (. demo $is-layout-invalid) false)
  (blank)
  (= ink-base 2)
  (do-new
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (= (. label $is-paint-invalid) false)
  (= (. label $is-layout-invalid) false)
  (= (. demo $is-paint-invalid) false)
  (= (. demo $is-layout-invalid) false)
  (blank)
  (= gap-base 3)
  (do-new
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (= (. label $is-paint-invalid) false)
  (= (. label $is-layout-invalid) false)
  (= (. demo $is-paint-invalid) false)
  (= (. demo $is-layout-invalid) false)
  (blank)
  (= ink-base 4)
  (= gap-base 5)
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let paint (. flush is-paint-invalid))
    (let layout (. flush is-layout-invalid))
    (let audit (. flush is-audit-invalid))
    (call print (str "flush applied=" (. flush applied) " paint=" paint " layout=" layout " audit=" audit))
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + gap I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
  (style-domains gap (domain layout))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
flush applied=0 paint=true layout=true audit=false
flush applied=1 paint=true layout=false audit=false
flush applied=1 paint=false layout=true audit=false
flush applied=2 paint=true layout=true audit=false
```

### A zero-action batch still notifies the root when style settling alone ran; the summary preserves applied=0 while exposing the settled domain snapshot.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink 2)
      (= gap 3)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "flush-count=" (. demo flush-count)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
    (let paint (. flush is-paint-invalid))
    (let layout (. flush is-layout-invalid))
    (let audit (. flush is-audit-invalid))
    (call print (str "zero applied=" (. flush applied) " paint=" paint " layout=" layout " audit=" audit))
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + gap I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
  (style-domains gap (domain layout))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
zero applied=0 paint=true layout=true audit=false
flush-count=1
```

### Flush summary domain flags are read-only event-snapshot properties.

```brix compile-error
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Root (id: root))
  (call (. root markup-batch-did-flush) (call MarkupBatchFlush 0 0))
)

(type + Root (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (= (. flush is-layout-invalid) false)
  )
)
```

```output
Exception: Markup batch flush domain flags are immutable
<no file:17>
        (= (. flush is-layout-invalid) false)
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (id: parent)
      (= ink 9)
      (<> Plain (id: inherited))
      (<> Label (id: reset)
        (= ink 7)
        (<> Plain (id: reset-child))
      )
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call (. demo ink-of) demo))
  (print (call (. demo ink-of) parent))
  (print (call (. demo ink-of) inherited))
  (print (call (. demo ink-of) reset))
  (print (call (. demo ink-of) reset-child))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (fn-expr + ink-of I64
    (param node Box)
    (. node $ink)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @inherited (= 7))
)

(type + Label (is Box)
  (using TextStyle)
)

(type + Plain (is Box))
```

```output
7
9
9
7
7
```
