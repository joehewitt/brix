# Style authoring

### Inline style attributes lower to static authored template declarations on the node subject, in declaration order, on the inline layer. Vocabulary comes from the object's own usings.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Label
        ink = 9
        bold = true

-- style-authored-snapshot(demo.%first-child!)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ ink I64 = 7
+ bold Bool = false

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
    using TextStyle
```

```output
node TextStyle.ink layer=5 order=0 kind=static bits=9
node TextStyle.bold layer=5 order=1 kind=static bits=1
```

### A style vocabulary installed by an enclosing markup class is visible to nested markup whose own class declares no usings.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Plain
        ink = 11

-- style-authored-snapshot(demo.%first-child!)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement
    using TextStyle

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ ink I64 = 7

━━━━━━━━━━━━━━━━━━━━
+ Plain is Box
```

```output
node TextStyle.ink layer=5 order=0 kind=static bits=11
```

### Style rule declarations lower to the rule layer, which inline attributes outrank.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Label • accent
        ink = 9

-- style-authored-snapshot(demo.%first-child!)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ ink I64 = 7
+ bold Bool = false

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
    using TextStyle

• accent
    ink = 13
    bold = true
```

```output
node TextStyle.ink layer=2 order=0 kind=static bits=13
node TextStyle.bold layer=2 order=1 kind=static bits=1
node TextStyle.ink layer=5 order=2 kind=static bits=9
```

### Conditional declarations outrank unconditional style-rule declarations, so component state styles can still apply when a named rule sets the same property.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Widget widget • themed

demo.markup-batch-queue.checkpoint()
-- widget.$ink

widget.is-hot = true
demo.markup-batch-queue.checkpoint()
-- widget.$ink
-- style-authored-snapshot(widget)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TintStyle is Style

+ ink I64 = 0

. ink -> paint

━━━━━━━━━━━━━━━━━━━━
+ Widget is Box
    using TintStyle

+ is-hot = false

• This
    ink = 1

    when #is-hot
        ink = 2

• themed
    ink = 9
```

```output
9
2
node TintStyle.ink layer=0 order=65536 kind=static bits=1
node TintStyle.ink layer=3 order=65537 kind=static bits=2 block=0
node TintStyle.ink layer=2 order=0 kind=static bits=9
```

### Tracked declaration expressions lower to dynamic batch actions that re-author the sidecar when their dependencies change, settling at batch flush.

```yumi
/core/style
/core/style-debug
/core/markup
/core/task
/core/tests/support/markup-fixtures

.. base = 12
. done = Semaphore()

<BatchDemo demo
    <Label
        ink = #base + 1

. label = demo.%first-child!
-- style-authored-snapshot(label)

base = 30

do new
    -- style-authored-snapshot(label)
    done.post()

done.wait()

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ ink I64 = 7

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
    using TextStyle
```

```output
node TextStyle.ink layer=5 order=0 kind=dynamic bits=13
node TextStyle.ink layer=5 order=0 kind=dynamic bits=31
```

### A dynamic declaration on a node inside a branch arm stops evaluating once the arm is removed. When one change both closes the arm and makes the declaration fail, the flush removes the node first and drops the declaration's queued update instead of evaluating it.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

.. value I64? = 3

<BatchDemo demo
    if #value != null
        <Label
            ink = #value!

demo.markup-batch-queue.checkpoint()
-- style-authored-snapshot(demo.%first-child!.%first-child!)
value = null
demo.markup-batch-queue.checkpoint()
-- "${demo.%first-child!.%first-child == null}"
value = 5
demo.markup-batch-queue.checkpoint()
-- style-authored-snapshot(demo.%first-child!.%first-child!)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ ink I64 = 7

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
    using TextStyle
```

```output
node TextStyle.ink layer=5 order=0 kind=dynamic bits=3
true
node TextStyle.ink layer=5 order=0 kind=dynamic bits=5
```

### An unmounted node stops observing the sources of its dynamic declarations. Remounting re-evaluates them against current values and observes the sources again.

```yumi
/core/style
/core/markup
/core/tracker
/core/tests/support/markup-fixtures

. source = Source()

<BatchDemo demo

markup-mount-root(demo, demo)
. label = make-label(source)
markup-append(demo, label)
demo.markup-batch-queue.checkpoint()
-- "mounted ink=${label.$ink} observed=${property-observed(source as Pointer, "value")}"

markup-remove(demo, label)
demo.markup-batch-queue.checkpoint()
source.value = 2
-- "unmounted observed=${property-observed(source as Pointer, "value")}"

markup-append(demo, label)
demo.markup-batch-queue.checkpoint()
-- "remounted ink=${label.$ink} observed=${property-observed(source as Pointer, "value")}"

source.value = 3
demo.markup-batch-queue.checkpoint()
-- "changed ink=${label.$ink}"

━━━━━━━━━━━━━━━━━━━━
+ do make-label Label
    - source Source

<Label label
    ink = source.#value
= label

━━━━━━━━━━━━━━━━━━━━
+ Source

+ value I64 = 1

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ ink I64 = 7

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
    using TextStyle
```

```output
mounted ink=1 observed=true
unmounted observed=false
remounted ink=2 observed=true
changed ink=3
```

### A live function in a dynamic declaration keeps its state across an unmount and remount. The state is detached while the node is unmounted, and the remount's run of the declaration finds it and attaches it again.

```yumi
/core/style
/core/markup
/core/tracker
/core/tests/support/markup-fixtures

_ made = 0
_ last-count Count?

. source = Source()

<BatchDemo demo

markup-mount-root(demo, demo)
. label = make-label(source)
markup-append(demo, label)
demo.markup-batch-queue.checkpoint()
-- "mounted ink=${label.$ink} ${state()}"

markup-remove(demo, label)
demo.markup-batch-queue.checkpoint()
-- "unmounted ${state()}"

markup-append(demo, label)
demo.markup-batch-queue.checkpoint()
-- "remounted ink=${label.$ink} ${state()}"

source.value = 2
demo.markup-batch-queue.checkpoint()
-- "changed ink=${label.$ink} ${state()}"

━━━━━━━━━━━━━━━━━━━━
+ do state String

. count = last-count!
= "made=$made runs=${count.runs} attached=${count.action != null}"

━━━━━━━━━━━━━━━━━━━━
+ do make-label Label
    - source Source

<Label label
    ink = tallied(source.#value)
= label

━━━━━━━━━━━━━━━━━━━━
+ do tallied I64 @live
    - value I64

. found = live-state'Count()
. count = found ?? Count()
if !found
    keep-live-state(count)
    made += 1
    last-count = count
count.runs += 1
= value

━━━━━━━━━━━━━━━━━━━━
+ Count
    is Tracker

+ runs = 0
+ action LiveAction?

────────────────────
+ do attach
    - action LiveAction

this.action = action

────────────────────
+ do detach
    - action LiveAction

this.action = null

━━━━━━━━━━━━━━━━━━━━
+ Source

+ value I64 = 1

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ ink I64 = 7

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
    using TextStyle
```

```output
mounted ink=1 made=1 runs=2 attached=true
unmounted made=1 runs=2 attached=false
remounted ink=1 made=1 runs=3 attached=true
changed ink=2 made=1 runs=4 attached=true
```

### All authored declarations land on the node record in authoring order, regardless of which style declares the property.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Label
        ink = 9
        margin = 3

-- style-authored-snapshot(demo.%first-child!)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ PlaceStyle is Style

+ ink I64 = 7
+ margin I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
    using PlaceStyle
```

```output
node PlaceStyle.ink layer=5 order=0 kind=static bits=9
node PlaceStyle.margin layer=5 order=1 kind=static bits=3
```

### Properties marked inherited accept the explicit inherit keyword, recorded as an inherit declaration rather than a value.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Label
        ink = inherit

-- style-authored-snapshot(demo.%first-child!)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ @inherited ink I64 = 7

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
    using TextStyle
```

```output
node TextStyle.ink layer=5 order=0 kind=inherit bits=0
```

### A property name that no visible style declares is a compile error.

```yumi crash
/core/style
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Label
        glow = 1

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ ink I64 = 7

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
    using TextStyle
```

```output
Exception: Property "glow" not found; an assignment in an element's body sets the element's property. To assign a local, put it in a do row: "do glow = …"
<no file:8>
            glow = 1
            ^^^^
```

### Two visible styles declaring the same property name with different value types is a compile error at the authoring site.

```yumi crash
/core/style
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Label
        ink = 9

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement
    using OtherStyle

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ ink I64 = 7

━━━━━━━━━━━━━━━━━━━━
+ OtherStyle is Style

+ ink Bool = false

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
    using TextStyle
```

```output
Exception: Style property "ink" has conflicting types in visible styles TextStyle and OtherStyle
<no file:8>
            ink = 9
            ^^^
```

### Per-instance object-valued declarations keep stable append-only handles while their owner lives. Mounted reads survive unmount/remount, and once the owners leave scope a forced collection clears their weak registry entries instead of retaining them through a refcount/free-list protocol.

```yumi
/core/style
/core/markup
/core/task
/core/tests/support/markup-fixtures
/BxGC/BxGC.h

. done = Semaphore()

<BatchDemo demo

markup-mount-root(demo, demo)
demo.markup-batch-queue.checkpoint()
. base = style-object-live-count()
do churn-panels(demo, done, base)

C:brixCollect()
do C:brixGCDrainCleanups()
. reclaimed = style-object-live-count() - base
-- "reclaimed=$reclaimed"

━━━━━━━━━━━━━━━━━━━━
+ do churn-panels
    - demo BatchDemo
    - done Semaphore
    - base Size

. a = make-panel(7)
. b = make-panel(8)
. c = make-panel(9)

markup-append(demo, a)
do new
    demo.markup-batch-queue.checkpoint()
    . mounted = style-object-live-count() - base
    -- "mounted=$mounted"
    -- "first=${panel-tag(a)}"
    markup-remove(demo, a)
    demo.markup-batch-queue.checkpoint()
    markup-append(demo, a)
    demo.markup-batch-queue.checkpoint()
    -- "remounted=${panel-tag(a)}"
    markup-remove(demo, a)
    demo.markup-batch-queue.checkpoint()
    markup-append(demo, b)
    done.post()
done.wait()

do new
    demo.markup-batch-queue.checkpoint()
    markup-remove(demo, b)
    demo.markup-batch-queue.checkpoint()
    markup-append(demo, c)
    done.post()
done.wait()

do new
    demo.markup-batch-queue.checkpoint()
    markup-remove(demo, c)
    demo.markup-batch-queue.checkpoint()
    done.post()
done.wait()

━━━━━━━━━━━━━━━━━━━━
+ do panel-tag I64
    - panel Panel

. decor = panel.$decor
= decor!.tag

━━━━━━━━━━━━━━━━━━━━
+ do make-panel Panel
    - tag I64

<Panel panel
    decor = Decor(tag)
= panel

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ Decor

+ tag I64

────────────────────
+ This
    - tag

━━━━━━━━━━━━━━━━━━━━
+ PanelStyle is Style

+ decor Decor? = null

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box
    using PanelStyle
```

```output
mounted=1
first=7
remounted=7
reclaimed=0
```

### Regression: program-wide style indexes past 31 still author and settle correctly. The filler styles push TextStyle's interned index beyond the low 32 bits of the per-record masks, so a 32-bit wrap in any owned/active/candidate mask shift would leave the label stuck on its initial segment instead of the authored value.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Filler
    <Label
        ink = 11

demo.markup-batch-queue.checkpoint()

-- style-computed-snapshot(demo.%last-child!)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ Filler is Box
    using Pad1Style
    using Pad2Style
    using Pad3Style
    using Pad4Style
    using Pad5Style
    using Pad6Style
    using Pad7Style
    using Pad8Style
    using Pad9Style
    using Pad10Style
    using Pad11Style
    using Pad12Style
    using Pad13Style
    using Pad14Style
    using Pad15Style
    using Pad16Style
    using Pad17Style
    using Pad18Style
    using Pad19Style
    using Pad20Style
    using Pad21Style
    using Pad22Style
    using Pad23Style
    using Pad24Style
    using Pad25Style
    using Pad26Style
    using Pad27Style
    using Pad28Style
    using Pad29Style
    using Pad30Style
    using Pad31Style
    using Pad32Style

━━━━━━━━━━━━━━━━━━━━
+ Pad1Style is Style

+ pad1 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad2Style is Style

+ pad2 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad3Style is Style

+ pad3 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad4Style is Style

+ pad4 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad5Style is Style

+ pad5 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad6Style is Style

+ pad6 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad7Style is Style

+ pad7 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad8Style is Style

+ pad8 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad9Style is Style

+ pad9 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad10Style is Style

+ pad10 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad11Style is Style

+ pad11 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad12Style is Style

+ pad12 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad13Style is Style

+ pad13 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad14Style is Style

+ pad14 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad15Style is Style

+ pad15 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad16Style is Style

+ pad16 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad17Style is Style

+ pad17 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad18Style is Style

+ pad18 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad19Style is Style

+ pad19 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad20Style is Style

+ pad20 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad21Style is Style

+ pad21 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad22Style is Style

+ pad22 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad23Style is Style

+ pad23 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad24Style is Style

+ pad24 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad25Style is Style

+ pad25 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad26Style is Style

+ pad26 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad27Style is Style

+ pad27 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad28Style is Style

+ pad28 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad29Style is Style

+ pad29 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad30Style is Style

+ pad30 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad31Style is Style

+ pad31 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Pad32Style is Style

+ pad32 I64 = 0

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ @inherited ink I64 = 7

. ink -> paint

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
    using TextStyle
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=11
```

### A subclass cannot use a style that redeclares a style property name its base class already sees, because inherited members resolve bare style names against their declaring class.

```yumi crash
/core/style
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <FancyLabel
        ink = 9

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ ink I64 = 7

━━━━━━━━━━━━━━━━━━━━
+ OtherStyle is Style

+ ink I64 = 3

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
    using TextStyle

━━━━━━━━━━━━━━━━━━━━
+ FancyLabel is Label
    using OtherStyle
```

```output
Exception: Style property "ink" is declared by both TextStyle and OtherStyle
<no file:36>
        using OtherStyle
        ^^^^^^^^^^^^^^^^
```

```yumi
/core/style
/core/markup
/core/task
/core/tests/support/markup-fixtures

. done = Semaphore()

<BatchDemo demo
    <Panel panel

demo.markup-batch-queue.checkpoint()
-- "${panel.mover.$ink} ${panel.mover.$tone}"

panel.positions <- 4
do new
    demo.markup-batch-queue.checkpoint()
    -- "${panel.mover.$ink} ${panel.mover.$tone}"
    done.post()
done.wait()

panel.positions <- 8
do new
    demo.markup-batch-queue.checkpoint()
    -- "${panel.mover.$ink} ${panel.mover.$tone}"
    done.post()
done.wait()

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TintStyle is Style

+ ink I64 = 1
+ tone I64 = 2

━━━━━━━━━━━━━━━━━━━━
+ Mover is Box
    using TintStyle

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ positions = AsyncChannel'I64()

────────────────────

<This
    <Mover mover
        ink = ##positions
```

```output
0 2
4 2
8 2
```

```yumi
/core/style
/core/markup
/core/task
/core/tests/support/markup-fixtures

. done = Semaphore()

<BatchDemo demo
    <Panel panel

demo.markup-batch-queue.checkpoint()
-- "${panel.mover.$ink} ${panel.mover.$tone}"

panel.positions <- 4
do new
    demo.markup-batch-queue.checkpoint()
    -- "${panel.mover.$ink} ${panel.mover.$tone}"
    done.post()
done.wait()

panel.positions <- 8
do new
    demo.markup-batch-queue.checkpoint()
    -- "${panel.mover.$ink} ${panel.mover.$tone}"
    done.post()
done.wait()

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TintStyle is Style

+ ink I64 = 1
+ tone I64 = 2

━━━━━━━━━━━━━━━━━━━━
+ Mover is Box
    using TintStyle

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ positions = AsyncChannel'I64()

────────────────────

<This
    <Mover mover
        ink = ##positions
        tone = (##positions) * 10 + 1
```

```output
0 1
4 41
8 81
```

### A component's own markup sees the styles of the markup that makes it, so an element there declares a style neither its class nor the component uses.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Holder
        <Panel panel

demo.markup-batch-queue.checkpoint()
-- style-computed-snapshot(panel.%first-child!)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ ink I64 = 7

━━━━━━━━━━━━━━━━━━━━
+ Holder is Box
    using TextStyle

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

────────────────────

<Label
    ink = 3

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=3
```

### A style a section of a class declares for the object on its own event is set as the event is delivered, in a module that also declares the style.

```yumi
/core/style
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Label

. label = demo.%first-child! as Label
demo.markup-batch-queue.checkpoint()
-- demo.read-ink(label)
label.emit(5)
demo.markup-batch-queue.checkpoint()
-- demo.read-ink(label)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement
    using TextStyle

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

────────────────────
+ do read-ink I64
    - label Box

= label.$ink

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
    using TextStyle

+ event changed I64

────────────────────
for value on changed

ink = value * 2

────────────────────
+ do emit
    - value I64

changed <- value

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ ink I64 = 7

. ink -> paint
```

```output
7
10
```
