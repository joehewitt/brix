# Style activation

### A style activates on a node when any of its properties is authored there by an enabled declaration; nodes with no authored declarations activate nothing. Dynamically authoring a property of a previously inactive style flips it active and raises the changed property's declared domain.

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
    )
    (<> Label
      (= gap 4)
    )
    (<> Label)
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  ; Clear the container bits Box raised imperatively while children were appended, so the
  ; snapshots below show only style-driven pending state.
  (= (. demo $is-paint-invalid) false)
  (= (. demo $is-layout-invalid) false)
  (blank)
  (let inked (!! (. demo %first-child)))
  (let gapped (!! (. inked %next-sibling)))
  (let plain (!! (. gapped %next-sibling)))
  (blank)
  (print (call style-activation-snapshot inked))
  (print (call style-activation-snapshot gapped))
  (print (call style-activation-snapshot plain))
  (print (call style-pending-snapshot demo))
  (blank)
  (= (. inked $is-paint-invalid) false)
  (= (. gapped $is-layout-invalid) false)
  (print (call style-pending-snapshot demo))
  (blank)
  (call style-author-dynamic inked (!! (call style-info-named "Spacing")) 0 (. StyleLayer inline) 99 -1 4 false)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-activation-snapshot inked))
  (print (call style-pending-snapshot demo))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (using Spacing)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(type + Spacing (is Style)
  (prop + gap I64 (= 0))
  (blank)
  (style-domains gap (domain layout))
)

(type + Label (is Box)
  (using TextStyle)
  (using Spacing)
)

(type + Plain (is Box))
```

```output
node=TextStyle
node=Spacing
unrecorded
paint=1 layout=1
paint=0 layout=0
node=TextStyle,Spacing
paint=0 layout=1
```

### An activation flip alone invalidates the flipped style's declared domains: authoring the property's initial value leaves computed state unchanged, but the activation change still raises the style's property domains on the node.

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
    )
    (<> Label)
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  ; Clear the container bits Box raised imperatively while children were appended.
  (= (. demo $is-paint-invalid) false)
  (= (. demo $is-layout-invalid) false)
  (blank)
  (let colored (!! (. demo %first-child)))
  (let plain (!! (. colored %next-sibling)))
  (blank)
  (print (call style-activation-snapshot plain))
  (blank)
  (= (. colored $is-paint-invalid) false)
  (print (call style-pending-snapshot demo))
  (blank)
  (call style-author-dynamic plain (!! (call style-info-named "TextStyle")) 0 (. StyleLayer inline) 0 -1 7 false)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-activation-snapshot plain))
  (print (call style-pending-snapshot demo))
  (print (str "plain-paint=" (. plain $is-paint-invalid) " colored-paint=" (. colored $is-paint-invalid)))
  (blank)
  (= (. plain $is-paint-invalid) false)
  (call style-author-dynamic plain (!! (call style-info-named "TextStyle")) 0 (. StyleLayer inline) 0 -1 7 false)
  (print (str "identical " (call style-drain-dirty-queue demo)))
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-pending-snapshot demo))
  (print (call style-computed-snapshot plain))
  (blank)
  (call style-author-dynamic plain (!! (call style-info-named "TextStyle")) 0 (. StyleLayer inline) 0 -1 9 false)
  (call (. demo markup-batch-queue checkpoint))
  (print (+ "changed " (call style-pending-snapshot demo)))
  (print (call style-computed-snapshot plain))
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
  (blank)
  (style-domains ink (domain paint))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
unrecorded
paint=0 layout=0
node=TextStyle
paint=1 layout=0
plain-paint=true colored-paint=false
identical applied=0 stale=0
paint=0 layout=0
initial
changed paint=1 layout=0
node TextStyle seg=owned gen=1 mask=1 ink=9
```

### A node whose settle changes several styles reports its domains to the host once, with every raised self domain in one mask and every raised parent domain in another.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Probe
      (= ink 9)
      (= gap 4)
      (= flex 2)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let probe (as (!! (. demo %first-child)) Probe))
  (print (. probe report))
  (blank)
  (call (. probe reset))
  (call style-author-dynamic probe (!! (call style-info-named "TextStyle")) 0 (. StyleLayer inline) 99 -1 5 false)
  (call style-author-dynamic probe (!! (call style-info-named "Spacing")) 0 (. StyleLayer inline) 99 -1 6 false)
  (call (. demo markup-batch-queue checkpoint))
  (print (. probe report))
  (blank)
  (call (. probe reset))
  (call (. demo markup-batch-queue checkpoint))
  (print (. probe report))
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
  (blank)
  (style-domains ink (domain paint))
)

(type + Spacing (is Style)
  (prop + gap I64 (= 0))
  (blank)
  (style-domains gap (domain layout))
)

(type + Probe (is Box)
  (using TextStyle)
  (using Spacing)
  (prop + self-calls Size)
  (prop + self-mask U32)
  (prop + parent-calls Size)
  (prop + parent-mask U32)
  (fn-do + reset _
    (= self-calls 0)
    (= self-mask 0)
    (= parent-calls 0)
    (= parent-mask 0)
  )
  (fn-do + report _ :get
    (let paint U32 (/lshift/ 1u32 (as (call style-domain-named "paint") Size)))
    (let layout U32 (/lshift/ 1u32 (as (call style-domain-named "layout") Size)))
    (let self-paint (!= (/and/ self-mask paint) 0))
    (let self-layout (!= (/and/ self-mask layout) 0))
    (let parent-layout (!= (/and/ parent-mask layout) 0))
    (return (str "self calls=" self-calls " paint=" self-paint " layout=" self-layout " parent calls=" parent-calls " layout=" parent-layout))
  )
  (fn-do + markup-style-domains-changed _
    (param mask U32)
    (+= self-calls 1)
    (= self-mask (/or/ self-mask mask))
  )
  (fn-do + markup-parent-style-domains-changed _
    (param mask U32)
    (+= parent-calls 1)
    (= parent-mask (/or/ parent-mask mask))
  )
)
```

```output
self calls=1 paint=true layout=true parent calls=1 layout=true
self calls=1 paint=true layout=true parent calls=0 layout=false
self calls=0 paint=false layout=false parent calls=0 layout=false
```

### `inherit` is an authored-layer keyword reserved for @inherited properties: assigning it to a non-inherited style property is a compile error at the authoring site.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink inherit)
    )
  )
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
Exception: Style property "ink" is not @inherited, so it cannot be assigned "inherit"
<no file:10>
          (= ink inherit)
          ^^^^^^^^^^^^^^^
```

### Unknown inline style property annotations are a compile error.

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
  (using TextStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @glowing (= 7))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
Exception: Unknown style property annotation "@glowing"; only "@inherited" is supported
<no file:25>
      (prop + ink I64 @glowing (= 7))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Property annotations are reserved for style properties; ordinary class properties reject them.

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
  (prop + glow I64 @inherited (= 0))
)
```

```output
Exception: Unknown property annotation "@inherited"; only "@hot" is supported on eager tuple fields
<no file:30>
      (prop + glow I64 @inherited (= 0))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Inherited computed values activate the style on descendants even when the descendant authored no declaration itself.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import /core/tests/support/markup-colors)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Box (id: host)
      (= color yellow)
    )
  )
  (blank)
  (let child (call Label "x"))
  (call markup-append host child)
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-computed-snapshot child))
  (print (call style-activation-snapshot child))
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
node TextPaint seg=parent gen=1 mask=1 color=11
node=TextPaint
```

### Style rule gates must be tracked Bool properties; tracked integers do not silently truth-test.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import /core/tests/support/markup-colors)
(blank)

(main
  (lazy selected 1)
  (blank)
  (<> BatchDemo (id: demo)
    (<> Label (id: a) (text: "x") (• bg red (if: #selected)))
  )
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
Exception: Style rule conditions must be Bool, but this condition is I64
<no file:12>
        (<> Label (id: a) (text: "x") (• bg red (if: #selected)))
                                                     ^^^^^^^^^
```

### The runtime permits more style indexes than fit in one U64 record mask.

```brix
(import /core/style)
(blank)

(main
  (let index Size 0)
  (while (< index 64)
    (call style-register (call StyleInfo (str "s" index) (str "S" index) 0))
    (+= index 1)
  )
  (blank)
  (let overflow (call style-register (call StyleInfo "overflow" "Overflow" 0)))
  (let fetched (call style-info-at 64))
  (blank)
  (print (. overflow name))
  (print (. (!! fetched) name))
)
```

```output
Overflow
Overflow
```

### The runtime rejects style property offsets that do not fit in per-segment U64 masks.

```brix
(import /core/style)
(blank)

(main
  (let attempt
    (do-new
      (let info (call StyleInfo "too-many" "TooMany" 65))
      (call style-register-property info 64 "p64" 0 false 0 0 0)
      (on-failure failure
        (let panic (? Panic) failure)
        (if panic
          (print (str "recovered " panic))
        )
      )
    )
  )
  (call (. attempt wait))
)
```

```output
recovered Style property offset exceeds the mask cap
```

### Past the 32 domains of invalidation the runtime holds, the error names the 33rd in the order the program first writes them, by line and then by place in the line, whatever order the program uses them in.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Late)
  (<> Early)
)

(type + First (is Style)
  (prop + size I64 (= 0))
  (blank)
  (style-domains size (domain a01) (domain a02) (domain a03) (domain a04) (domain a05) (domain a06) (domain a07) (domain a08) (domain a09) (domain a10) (domain a11) (domain a12) (domain a13) (domain a14) (domain a15) (domain a16) (domain a17) (domain a18) (domain a19) (domain a20))
)

(type + Second (is Style)
  (prop + size I64 (= 0))
  (blank)
  (style-domains size (domain b02) (domain b01) (domain b03) (domain b04) (domain b05) (domain b06) (domain b07) (domain b08) (domain b09) (domain b11) (domain b10) (domain b12) (domain b13) (domain b14) (domain b15) (domain b16) (domain b17) (domain b18) (domain b19) (domain b20))
)

(type + Early (is Box)
  (using First)
)

(type + Late (is Box)
  (using Second)
)
```

```output
Exception: Too many style invalidation domains (32 maximum); cannot intern "b13"
<no file:21>
      (style-domains size (domain b02) (domain b01) (domain b03) (domain b04) (domain b05) (domain b06) (domain b07) (domain b08) (domain b09) (domain b11) (domain b10) (domain b12) (domain b13) (domain b14) (domain b15) (domain b16) (domain b17) (domain b18) (domain b19) (domain b20))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An activation flip raises the domains of the style's properties that are declared on the node or differ from their initial values, not those of every property the style holds: authoring only the paint property of a style that also holds a parent-layout property flips the style active and raises paint on the node and nothing on its parent.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Probe)
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (let probe (as (!! (. demo %first-child)) Probe))
  (call (. probe reset))
  (call style-author-dynamic probe (!! (call style-info-named "Frame")) 0 (. StyleLayer inline) 99 -1 7 false)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-activation-snapshot probe))
  (print (. probe report))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Frame (is Style)
  (prop + ink I64 (= 7))
  (prop + edge I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
  (style-domains edge (domain layout) (domain layout parent))
)

(type + Probe (is Box)
  (using Frame)
  (prop + self-calls Size)
  (prop + self-mask U32)
  (prop + parent-calls Size)
  (prop + parent-mask U32)
  (fn-do + reset _
    (= self-calls 0)
    (= self-mask 0)
    (= parent-calls 0)
    (= parent-mask 0)
  )
  (fn-do + report _ :get
    (let paint U32 (/lshift/ 1u32 (as (call style-domain-named "paint") Size)))
    (let layout U32 (/lshift/ 1u32 (as (call style-domain-named "layout") Size)))
    (let self-paint (!= (/and/ self-mask paint) 0))
    (let self-layout (!= (/and/ self-mask layout) 0))
    (let parent-layout (!= (/and/ parent-mask layout) 0))
    (return (str "self calls=" self-calls " paint=" self-paint " layout=" self-layout " parent calls=" parent-calls " layout=" parent-layout))
  )
  (fn-do + markup-style-domains-changed _
    (param mask U32)
    (+= self-calls 1)
    (= self-mask (/or/ self-mask mask))
  )
  (fn-do + markup-parent-style-domains-changed _
    (param mask U32)
    (+= parent-calls 1)
    (= parent-mask (/or/ parent-mask mask))
  )
)
```

```output
node=Frame
self calls=1 paint=true layout=false parent calls=0 layout=false
```
