# Style authoring

### Inline style attributes lower to static authored template declarations on the node subject, in declaration order, on the inline layer. Vocabulary comes from the object's own usings.

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
      (= bold true)
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

(type + Label (is Box)
  (using TextStyle)
)
```

```output
node TextStyle.ink layer=5 order=0 kind=static bits=9
node TextStyle.bold layer=5 order=1 kind=static bits=1
```

### A style vocabulary installed by an enclosing markup class is visible to nested markup whose own class declares no usings.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Plain
      (= ink 11)
    )
  )
  (blank)
  (print (call style-authored-snapshot (!! (. demo %first-child))))
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

(type + Plain (is Box))
```

```output
node TextStyle.ink layer=5 order=0 kind=static bits=11
```

### Style rule declarations lower to the rule layer, which inline attributes outrank.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (• accent)
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

(type + Label (is Box)
  (using TextStyle)
  (rule accent
    (= ink 13)
    (= bold true)
  )
)
```

```output
node TextStyle.ink layer=2 order=0 kind=static bits=13
node TextStyle.bold layer=2 order=1 kind=static bits=1
node TextStyle.ink layer=5 order=2 kind=static bits=9
```

### Conditional declarations outrank unconditional style-rule declarations, so component state styles can still apply when a named rule sets the same property.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Widget (id: widget) (• themed))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (. widget $ink))
  (blank)
  (= (. widget is-hot) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (. widget $ink))
  (print (call style-authored-snapshot widget))
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
  (blank)
  (style-domains ink (domain paint))
)

(type + Widget (is Box)
  (using TintStyle)
  (prop + is-hot _ (= false))
  (blank)
  (rule This
    (= ink 1)
    (blank)
    (when-style #is-hot
      (= ink 2)
    )
  )
  (blank)
  (rule themed
    (= ink 9)
  )
)
```

```output
9
2
node TintStyle.ink layer=0 order=65536 kind=static bits=1
node TintStyle.ink layer=3 order=65537 kind=static bits=2 block=0
node TintStyle.ink layer=2 order=0 kind=static bits=9
```

### Tracked declaration expressions lower to dynamic batch actions that re-author the sidecar when their dependencies change, settling at batch flush.

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
      (= ink (+ #base 1))
    )
  )
  (blank)
  (let label (!! (. demo %first-child)))
  (print (call style-authored-snapshot label))
  (blank)
  (= base 30)
  (blank)
  (do-new
    (print (call style-authored-snapshot label))
    (call (. done post))
  )
  (blank)
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

(type + Label (is Box)
  (using TextStyle)
)
```

```output
node TextStyle.ink layer=5 order=0 kind=dynamic bits=13
node TextStyle.ink layer=5 order=0 kind=dynamic bits=31
```

### A dynamic declaration on a node inside a branch arm stops evaluating once the arm is removed. When one change both closes the arm and makes the declaration fail, the flush removes the node first and drops the declaration's queued update instead of evaluating it.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (lazy value (? I64) 3)
  (blank)
  (<> BatchDemo (id: demo)
    (if (!= #value null)
      (<> Label
        (= ink (!! #value))
      )
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-authored-snapshot (!! (. (!! (. demo %first-child)) %first-child))))
  (= value null)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (== (. (!! (. demo %first-child)) %first-child) null)))
  (= value 5)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-authored-snapshot (!! (. (!! (. demo %first-child)) %first-child))))
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

(type + Label (is Box)
  (using TextStyle)
)
```

```output
node TextStyle.ink layer=5 order=0 kind=dynamic bits=3
true
node TextStyle.ink layer=5 order=0 kind=dynamic bits=5
```

### An unmounted node stops observing the sources of its dynamic declarations. Remounting re-evaluates them against current values and observes the sources again.

```brix
(import /core/style)
(import /core/markup)
(import /core/tracker)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let source (call Source))
  (blank)
  (<> BatchDemo (id: demo))
  (blank)
  (call markup-mount-root demo demo)
  (let label (call make-label source))
  (call markup-append demo label)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "mounted ink=" (. label $ink) " observed=" (call property-observed (as source Pointer) "value")))
  (blank)
  (call markup-remove demo label)
  (call (. demo markup-batch-queue checkpoint))
  (= (. source value) 2)
  (print (str "unmounted observed=" (call property-observed (as source Pointer) "value")))
  (blank)
  (call markup-append demo label)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "remounted ink=" (. label $ink) " observed=" (call property-observed (as source Pointer) "value")))
  (blank)
  (= (. source value) 3)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "changed ink=" (. label $ink)))
)

(fn-do + make-label Label
  (param source Source)
  (<> Label (id: label)
    (= ink (. source #value))
  )
  (return label)
)

(type + Source
  (prop + value I64 (= 1))
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

(type + Label (is Box)
  (using TextStyle)
)
```

```output
mounted ink=1 observed=true
unmounted observed=false
remounted ink=2 observed=true
changed ink=3
```

### A live function in a dynamic declaration keeps its state across an unmount and remount. The state is detached while the node is unmounted, and the remount's run of the declaration finds it and attaches it again.

```brix
(import /core/style)
(import /core/markup)
(import /core/tracker)
(import /core/tests/support/markup-fixtures)
(blank)
(prop _ made _ (= 0))
(prop _ last-count (? Count))
(blank)

(main
  (let source (call Source))
  (blank)
  (<> BatchDemo (id: demo))
  (blank)
  (call markup-mount-root demo demo)
  (let label (call make-label source))
  (call markup-append demo label)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "mounted ink=" (. label $ink) " " (call state)))
  (blank)
  (call markup-remove demo label)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "unmounted " (call state)))
  (blank)
  (call markup-append demo label)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "remounted ink=" (. label $ink) " " (call state)))
  (blank)
  (= (. source value) 2)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "changed ink=" (. label $ink) " " (call state)))
)

(fn-do + state String
  (let count (!! last-count))
  (return (str "made=" made " runs=" (. count runs) " attached=" (!= (. count action) null)))
)

(fn-do + make-label Label
  (param source Source)
  (<> Label (id: label)
    (= ink (call tallied (. source #value)))
  )
  (return label)
)

(fn-do + tallied I64 @live
  (param value I64)
  (let found (call (of live-state Count)))
  (let count (?? found (call Count)))
  (if (not found)
    (call keep-live-state count)
    (+= made 1)
    (= last-count count)
  )
  (+= (. count runs) 1)
  (return value)
)

(type + Count
  (implements Tracker)
  (prop + runs _ (= 0))
  (prop + action (? LiveAction))
  (fn-do + attach _
    (param action LiveAction)
    (= (. this action) action)
  )
  (fn-do + detach _
    (param action LiveAction)
    (= (. this action) null)
  )
)

(type + Source
  (prop + value I64 (= 1))
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

(type + Label (is Box)
  (using TextStyle)
)
```

```output
mounted ink=1 made=1 runs=2 attached=true
unmounted made=1 runs=2 attached=false
remounted ink=1 made=1 runs=3 attached=true
changed ink=2 made=1 runs=4 attached=true
```

### All authored declarations land on the node record in authoring order, regardless of which style declares the property.

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
      (= margin 3)
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

(type + PlaceStyle (is Style)
  (prop + ink I64 (= 7))
  (prop + margin I64 (= 0))
)

(type + Label (is Box)
  (using PlaceStyle)
)
```

```output
node PlaceStyle.ink layer=5 order=0 kind=static bits=9
node PlaceStyle.margin layer=5 order=1 kind=static bits=3
```

### Properties marked inherited accept the explicit inherit keyword, recorded as an inherit declaration rather than a value.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink inherit)
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
  (prop + ink I64 @inherited (= 7))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
node TextStyle.ink layer=5 order=0 kind=inherit bits=0
```

### A property name that no visible style declares is a compile error.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= glow 1)
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

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
Exception: Property "glow" not found; an assignment in an element's body sets the element's property. To assign a local, put it in a do row: "do glow = …"
<no file:10>
          (= glow 1)
             ^^^^
```

### Two visible styles declaring the same property name with different value types is a compile error at the authoring site.

```brix crash
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
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using OtherStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(type + OtherStyle (is Style)
  (prop + ink Bool (= false))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
Exception: Style property "ink" has conflicting types in visible styles TextStyle and OtherStyle
<no file:10>
          (= ink 9)
             ^^^
```

### Per-instance object-valued declarations keep stable append-only handles while their owner lives. Mounted reads survive unmount/remount, and once the owners leave scope a forced collection clears their weak registry entries instead of retaining them through a refcount/free-list protocol.

```brix
(import /core/style)
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
  (call markup-mount-root demo demo)
  (call (. demo markup-batch-queue checkpoint))
  (let base (call style-object-live-count))
  (do
    (inline (call churn-panels demo done base))
  )
  (blank)
  (call C:brixCollect)
  (do
    (inline (call C:brixGCDrainCleanups))
  )
  (let reclaimed (- (call style-object-live-count) base))
  (print (str "reclaimed=" reclaimed))
)

(fn-do + churn-panels _
  (param demo BatchDemo)
  (param done Semaphore)
  (param base Size)
  (let a (call make-panel 7))
  (let b (call make-panel 8))
  (let c (call make-panel 9))
  (blank)
  (call markup-append demo a)
  (do-new
    (call (. demo markup-batch-queue checkpoint))
    (let mounted (- (call style-object-live-count) base))
    (print (str "mounted=" mounted))
    (print (str "first=" (call panel-tag a)))
    (call markup-remove demo a)
    (call (. demo markup-batch-queue checkpoint))
    (call markup-append demo a)
    (call (. demo markup-batch-queue checkpoint))
    (print (str "remounted=" (call panel-tag a)))
    (call markup-remove demo a)
    (call (. demo markup-batch-queue checkpoint))
    (call markup-append demo b)
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (do-new
    (call (. demo markup-batch-queue checkpoint))
    (call markup-remove demo b)
    (call (. demo markup-batch-queue checkpoint))
    (call markup-append demo c)
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (do-new
    (call (. demo markup-batch-queue checkpoint))
    (call markup-remove demo c)
    (call (. demo markup-batch-queue checkpoint))
    (call (. done post))
  )
  (call (. done wait))
)

(fn-do + panel-tag I64
  (param panel Panel)
  (let decor (. panel $decor))
  (return (. (!! decor) tag))
)

(fn-do + make-panel Panel
  (param tag I64)
  (<> Panel (id: panel)
    (= decor (call Decor tag))
  )
  (return panel)
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Decor
  (prop + tag I64)
  (fn-do + This _
    (param tag _)
  )
)

(type + PanelStyle (is Style)
  (prop + decor (? Decor) (= null))
)

(type + Panel (is Box)
  (using PanelStyle)
)
```

```output
mounted=1
first=7
remounted=7
reclaimed=0
```

### Regression: program-wide style indexes past 31 still author and settle correctly. The filler styles push TextStyle's interned index beyond the low 32 bits of the per-record masks, so a 32-bit wrap in any owned/active/candidate mask shift would leave the label stuck on its initial segment instead of the authored value.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Filler)
    (<> Label
      (= ink 11)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-computed-snapshot (!! (. demo %last-child))))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Filler (is Box)
  (using Pad1Style)
  (using Pad2Style)
  (using Pad3Style)
  (using Pad4Style)
  (using Pad5Style)
  (using Pad6Style)
  (using Pad7Style)
  (using Pad8Style)
  (using Pad9Style)
  (using Pad10Style)
  (using Pad11Style)
  (using Pad12Style)
  (using Pad13Style)
  (using Pad14Style)
  (using Pad15Style)
  (using Pad16Style)
  (using Pad17Style)
  (using Pad18Style)
  (using Pad19Style)
  (using Pad20Style)
  (using Pad21Style)
  (using Pad22Style)
  (using Pad23Style)
  (using Pad24Style)
  (using Pad25Style)
  (using Pad26Style)
  (using Pad27Style)
  (using Pad28Style)
  (using Pad29Style)
  (using Pad30Style)
  (using Pad31Style)
  (using Pad32Style)
)

(type + Pad1Style (is Style)
  (prop + pad1 I64 (= 0))
)

(type + Pad2Style (is Style)
  (prop + pad2 I64 (= 0))
)

(type + Pad3Style (is Style)
  (prop + pad3 I64 (= 0))
)

(type + Pad4Style (is Style)
  (prop + pad4 I64 (= 0))
)

(type + Pad5Style (is Style)
  (prop + pad5 I64 (= 0))
)

(type + Pad6Style (is Style)
  (prop + pad6 I64 (= 0))
)

(type + Pad7Style (is Style)
  (prop + pad7 I64 (= 0))
)

(type + Pad8Style (is Style)
  (prop + pad8 I64 (= 0))
)

(type + Pad9Style (is Style)
  (prop + pad9 I64 (= 0))
)

(type + Pad10Style (is Style)
  (prop + pad10 I64 (= 0))
)

(type + Pad11Style (is Style)
  (prop + pad11 I64 (= 0))
)

(type + Pad12Style (is Style)
  (prop + pad12 I64 (= 0))
)

(type + Pad13Style (is Style)
  (prop + pad13 I64 (= 0))
)

(type + Pad14Style (is Style)
  (prop + pad14 I64 (= 0))
)

(type + Pad15Style (is Style)
  (prop + pad15 I64 (= 0))
)

(type + Pad16Style (is Style)
  (prop + pad16 I64 (= 0))
)

(type + Pad17Style (is Style)
  (prop + pad17 I64 (= 0))
)

(type + Pad18Style (is Style)
  (prop + pad18 I64 (= 0))
)

(type + Pad19Style (is Style)
  (prop + pad19 I64 (= 0))
)

(type + Pad20Style (is Style)
  (prop + pad20 I64 (= 0))
)

(type + Pad21Style (is Style)
  (prop + pad21 I64 (= 0))
)

(type + Pad22Style (is Style)
  (prop + pad22 I64 (= 0))
)

(type + Pad23Style (is Style)
  (prop + pad23 I64 (= 0))
)

(type + Pad24Style (is Style)
  (prop + pad24 I64 (= 0))
)

(type + Pad25Style (is Style)
  (prop + pad25 I64 (= 0))
)

(type + Pad26Style (is Style)
  (prop + pad26 I64 (= 0))
)

(type + Pad27Style (is Style)
  (prop + pad27 I64 (= 0))
)

(type + Pad28Style (is Style)
  (prop + pad28 I64 (= 0))
)

(type + Pad29Style (is Style)
  (prop + pad29 I64 (= 0))
)

(type + Pad30Style (is Style)
  (prop + pad30 I64 (= 0))
)

(type + Pad31Style (is Style)
  (prop + pad31 I64 (= 0))
)

(type + Pad32Style (is Style)
  (prop + pad32 I64 (= 0))
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
node TextStyle seg=owned gen=1 mask=1 ink=11
```

### A subclass cannot use a style that redeclares a style property name its base class already sees, because inherited members resolve bare style names against their declaring class.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> FancyLabel
      (= ink 9)
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

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(type + OtherStyle (is Style)
  (prop + ink I64 (= 3))
)

(type + Label (is Box)
  (using TextStyle)
)

(type + FancyLabel (is Label)
  (using OtherStyle)
)
```

```output
Exception: Style property "ink" is declared by both TextStyle and OtherStyle
<no file:36>
      (using OtherStyle)
      ^^^^^^^^^^^^^^^^^^
```

```brix
(import /core/style)
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (. panel mover $ink) " " (. panel mover $tone)))
  (blank)
  (<- (. panel positions) 4)
  (do-new
    (call (. demo markup-batch-queue checkpoint))
    (print (str (. panel mover $ink) " " (. panel mover $tone)))
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (<- (. panel positions) 8)
  (do-new
    (call (. demo markup-batch-queue checkpoint))
    (print (str (. panel mover $ink) " " (. panel mover $tone)))
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

(type + TintStyle (is Style)
  (prop + ink I64 (= 1))
  (prop + tone I64 (= 2))
)

(type + Mover (is Box)
  (using TintStyle)
)

(type + Panel (is Box)
  (prop + positions _ (= (call (of AsyncChannel I64))))
  (content
    (<> This
      (<> Mover (id: mover)
        (= ink (## positions))
      )
    )
  )
)
```

```output
0 2
4 2
8 2
```

```brix
(import /core/style)
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (. panel mover $ink) " " (. panel mover $tone)))
  (blank)
  (<- (. panel positions) 4)
  (do-new
    (call (. demo markup-batch-queue checkpoint))
    (print (str (. panel mover $ink) " " (. panel mover $tone)))
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (<- (. panel positions) 8)
  (do-new
    (call (. demo markup-batch-queue checkpoint))
    (print (str (. panel mover $ink) " " (. panel mover $tone)))
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

(type + TintStyle (is Style)
  (prop + ink I64 (= 1))
  (prop + tone I64 (= 2))
)

(type + Mover (is Box)
  (using TintStyle)
)

(type + Panel (is Box)
  (prop + positions _ (= (call (of AsyncChannel I64))))
  (content
    (<> This
      (<> Mover (id: mover)
        (= ink (## positions))
        (= tone (+ (* (## positions) 10) 1))
      )
    )
  )
)
```

```output
0 1
4 41
8 81
```

### A component's own markup sees the styles of the markup that makes it, so an element there declares a style neither its class nor the component uses.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Holder
      (<> Panel (id: panel))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (!! (. panel %first-child))))
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

(type + Holder (is Box)
  (using TextStyle)
)

(type + Panel (is Box)
  (content
    (<> Label
      (= ink 3)
    )
  )
)

(type + Label (is Box))
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=3
```

### A style a section of a class declares for the object on its own event is set as the event is delivered, in a module that also declares the style.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label)
  )
  (blank)
  (let label (as (!! (. demo %first-child)) Label))
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. demo read-ink) label))
  (call (. label emit) 5)
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. demo read-ink) label))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (fn-do + read-ink I64
    (param label Box)
    (return (. label $ink))
  )
)

(type + Label (is Box)
  (using TextStyle)
  (event + changed I64)
  (subscribe-for (on value changed)
    (= ink (* value 2))
  )
  (fn-do + emit _
    (param value I64)
    (<- changed value)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
  (blank)
  (style-domains ink (domain paint))
)
```

```output
7
10
```
