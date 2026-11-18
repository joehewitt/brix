# Style sidecar

### Mounting under a batch root creates one attachment per live object. Nodes with no authored style stay recordless; draining clears conservative dirty bits.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <TextBox = "a"
    <TextBox = "b"

. first = demo.%first-child!
-- style-attachment-snapshot(demo)
-- style-attachment-snapshot(first)
-- style-store-snapshot(demo)
-- style-drain-dirty-queue(demo)
-- style-attachment-snapshot(first)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)
+ flush-count Size = 0

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

flush-count += 1
```

```output
live id=1 root=1 parent=- node=- gen=0 dirty=SI
live id=2 root=1 parent=1 node=- gen=0 dirty=SI
store id=1 live=3 attachments=3 free-attachments=0 nodes=0 free-nodes=0 segments=0 free-segments=0 queue=3 inherited-roots=0
applied=3 stale=0
live id=2 root=1 parent=1 node=- gen=0 dirty=--
```

### Objects that never enter a styled tree pay no visible style storage, including markup built without a participating MarkupRootElement root.

```yumi
/core/style
/core/style-debug
/core/tests/support/markup-fixtures

. plain = Box()
. text = TextBox("x")
markup-append(plain, text)

<Box box
    <TextBox = "y"

-- style-attachment-snapshot(plain)
-- style-attachment-snapshot(text)
-- style-attachment-snapshot(box)
-- style-attachment-snapshot(box.%first-child!)
```

```output
none
none
none
none
```

### Unmounting releases records and owned segments back to the root store pools and turns pending dirty-queue entries stale instead of touching released records.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <TextBox = "a"

. child = demo.%first-child!
-- style-drain-dirty-queue(demo)
style-debug-touch-node-segment(child)
-- style-node-snapshot(child)
markup-remove(demo, child)
-- style-attachment-snapshot(child)
-- style-store-snapshot(demo)
-- style-drain-dirty-queue(demo)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)
+ flush-count Size = 0

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

flush-count += 1
```

```output
applied=2 stale=0
node id=0 seg=owned gen=1 mask=1
none
store id=1 live=1 attachments=2 free-attachments=1 nodes=1 free-nodes=1 segments=1 free-segments=1 queue=1 inherited-roots=0
applied=0 stale=1
```

### Mounting the same object in two places at once fails deterministically.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Box
    <Box

. a = demo.%first-child!
. b = demo.%last-child!
. text = TextBox("t")
markup-append(a, text)

. attempt = do new
    markup-append(b, text)
    -- "unreachable"
catch failure
    . panic Panic? = failure
    if panic
        -- "recovered $panic"
        -- "a-children=${count-children(a)} b-children=${count-children(b)} parent-a=${text.%parent == a} parent-b=${text.%parent == b}"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)
+ flush-count Size = 0

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

flush-count += 1

━━━━━━━━━━━━━━━━━━━━
+ do count-children Size
    - box Box

. count Size = 0
for child in box.children
    count += 1
= count
```

```output
recovered Markup child is already linked
a-children=1 b-children=0 parent-a=true parent-b=false
```

### Any move — within the same root or between roots — is a style unmount followed by remount. The merged owner attachment persists while its logical style identity is released and reused.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Box
    <Box

. a = demo.%first-child!
. b = demo.%last-child!
. text = TextBox("t")
markup-append(a, text)
-- style-drain-dirty-queue(demo)
-- style-attachment-snapshot(text)

markup-remove(a, text)
-- style-attachment-snapshot(text)
markup-append(b, text)
-- style-attachment-snapshot(text)
-- style-store-snapshot(demo)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)
+ flush-count Size = 0

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

flush-count += 1
```

Unmounting hides detached style state; remounting under the new parent reuses the freed logical
identity, keeps the unstyled node recordless, and retains the release generation bump.

```output
applied=4 stale=0
live id=4 root=1 parent=2 node=- gen=0 dirty=--
none
live id=4 root=1 parent=3 node=- gen=1 dirty=SI
store id=1 live=4 attachments=4 free-attachments=0 nodes=0 free-nodes=0 segments=0 free-segments=0 queue=1 inherited-roots=0
```

### Moving an object between roots rehomes it: unstyled nodes keep only attachments in the new store. Moving under an unrooted parent unmounts.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo left
    <TextBox = "a"

<BatchDemo right
    <TextBox = "b"

. child = left.%first-child!
-- style-attachment-snapshot(child)

markup-remove(left, child)
markup-append(right, child)
-- style-attachment-snapshot(child)
-- style-store-snapshot(left)
-- style-store-snapshot(right)

. plain = Box()
markup-remove(right, child)
markup-append(plain, child)
-- style-attachment-snapshot(child)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)
+ flush-count Size = 0

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

flush-count += 1
```

Rehoming allocates fresh logical state in the new store; the merged attachment retains its
release generation while the unstyled node stays recordless.

```output
live id=2 root=1 parent=1 node=- gen=0 dirty=SI
live id=3 root=2 parent=1 node=- gen=1 dirty=SI
store id=1 live=1 attachments=2 free-attachments=1 nodes=0 free-nodes=0 segments=0 free-segments=0 queue=2 inherited-roots=0
store id=2 live=3 attachments=3 free-attachments=0 nodes=0 free-nodes=0 segments=0 free-segments=0 queue=3 inherited-roots=0
none
```

### Unmounting discards style state: a remounted object adopts the segments of the tree it joins instead of carrying values across roots.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo left
    <TextBox = "a"

<BatchDemo right
    <TextBox = "b"

. child = left.%first-child!
style-debug-touch-node-segment(child)
-- style-node-snapshot(child)

markup-remove(left, child)
markup-append(right, child)
-- style-node-snapshot(child)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)
+ flush-count Size = 0

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

flush-count += 1
```

The remounted unstyled object stays recordless until authored state appears.

```output
node id=0 seg=owned gen=1 mask=1
none
```

### Persistent pending-domain bits live on the sidecar attachment: accessor writes raise and clear them, the root's per-domain counts track live nodes exactly, idempotent writes do not double-count, and unmounting a node releases its pending state.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Marked a
        ink = 9
    <Marked b

demo.markup-batch-queue.checkpoint()
-- style-pending-snapshot(demo)
-- "a=${a.$is-paint-invalid} b=${b.$is-paint-invalid}"

b.$is-paint-invalid = true
b.$is-paint-invalid = true
-- style-pending-snapshot(demo)

a.$is-paint-invalid = false
-- style-pending-snapshot(demo)
-- "a=${a.$is-paint-invalid} b=${b.$is-paint-invalid}"

markup-remove(demo, b)
-- style-pending-snapshot(demo)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)
+ flush-count Size = 0

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

flush-count += 1

━━━━━━━━━━━━━━━━━━━━
+ MarkStyle is Style

+ ink I64 = 7

. ink -> paint

━━━━━━━━━━━━━━━━━━━━
+ Marked is Box
    using MarkStyle
```

The container raised its own layout+paint bits imperatively when children were appended.

```output
paint=2 layout=1
a=true b=false
paint=3 layout=1
paint=2 layout=1
a=false b=true
paint=1 layout=1
```

### A domain used only through `$is-<domain>-invalid` accessors is still visible to name-based root/debug APIs.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Box a
    <Box b

demo.markup-batch-queue.checkpoint()

b.$is-audit-invalid = true
. audit-on = style-domain-pending-count(demo, "audit")
-- "audit=$audit-on b=${b.$is-audit-invalid}"

b.$is-audit-invalid = false
. audit-off = style-domain-pending-count(demo, "audit")
-- "audit=$audit-off b=${b.$is-audit-invalid}"

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)
+ flush-count Size = 0

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

flush-count += 1
```

```output
audit=1 b=true
audit=0 b=false
```

### Region boxes created by markup loops participate. Style parentage is derived from the structural tree, so linking the parent late requires no second dirty or inherited-root entry.

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    for item in demo.#items
        <TextBox = item

. region = demo.%first-child!
. text = region.%first-child!
-- style-attachment-snapshot(region)
-- style-attachment-snapshot(text)
-- style-store-snapshot(demo)

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ items [String] = List'String(["a", "b"])
+ markup-batch-queue := MarkupBatchQueue(this)
+ flush-count Size = 0

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

flush-count += 1
```

The region receives its batch root before linking to a parent. Derived traversal observes the
structural link directly, so the original mount entries are sufficient.

```output
live id=2 root=1 parent=1 node=- gen=0 dirty=SI
live id=3 root=1 parent=2 node=- gen=0 dirty=SI
store id=1 live=4 attachments=4 free-attachments=0 nodes=0 free-nodes=0 segments=0 free-segments=0 queue=4 inherited-roots=0
```

### Static style declarations from one construction site are registered once even when the site creates many instances.

```yumi
/core/style
/core/markup
/core/tests/support/markup-fixtures
/BxGC/BxGC.h

. template-lane = C:brixGCStyleTemplatesSidePayloadLane()
. templates-before = C:brixGCObjectSidePayloadCount(template-lane)

<BatchDemo demo
    for item in demo.#items
        <Label
            ink = 12
            tone = 4

. declarations = style-site-template-declaration-count()
. templates = C:brixGCObjectSidePayloadCount(template-lane) - templates-before
-- "declarations=$declarations templates=$templates"

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement

+ items [String] = List'String(["a", "b", "c", "d", "e", "f", "g", "h"])
+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ ink I64 = 7
+ tone I64 = 3

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
    using TextStyle
```

```output
declarations=2 templates=8
```

### The style side-payload lane now belongs only to the batch root. Non-root style state lives in the merged markup attachment: ordinary unmount releases only its mounted identity so templates and object values survive remount, while dropping a mounted parentless owner runs merged cleanup. Template and object lanes return exactly; the object lane is bounded by one because the file runner shares its process with sibling cases.

```yumi
/core/style
/core/markup
/core/tests/support/markup-fixtures
/BxGC/BxGC.h

. attachment-lane = C:brixGCStyleAttachmentsSidePayloadLane()
. template-lane = C:brixGCStyleTemplatesSidePayloadLane()
. object-lane = C:brixGCStyleObjectsSidePayloadLane()

<BatchDemo demo

. attachments-before = C:brixGCObjectSidePayloadCount(attachment-lane)
. templates-before = C:brixGCObjectSidePayloadCount(template-lane)
. objects-before = C:brixGCObjectSidePayloadCount(object-lane)

do exercise-reclaim(demo, attachment-lane, template-lane, object-lane, attachments-before, templates-before, objects-before)

C:brixCollect()
do C:brixGCDrainCleanups()
. objects-final = C:brixGCObjectSidePayloadCount(object-lane)
-- "reclaimed attachments=${C:brixGCObjectSidePayloadCount(attachment-lane) - attachments-before} templates=${C:brixGCObjectSidePayloadCount(template-lane) - templates-before} objects-bounded=${objects-final <= objects-before + 1}"

━━━━━━━━━━━━━━━━━━━━
+ do exercise-reclaim
    - demo BatchDemo
    - attachment-lane U64
    - template-lane U64
    - object-lane U64
    - attachments-before U64
    - templates-before U64
    - objects-before U64

.. panel Panel? = make-panel(77)

markup-adopt(panel!, null, demo)
demo.markup-batch-queue.checkpoint()
-- "mounted=${panel-tag(panel!)} attachments=${C:brixGCObjectSidePayloadCount(attachment-lane) - attachments-before} templates=${C:brixGCObjectSidePayloadCount(template-lane) - templates-before} objects=${C:brixGCObjectSidePayloadCount(object-lane) - objects-before}"

markup-release(panel!)
demo.markup-batch-queue.checkpoint()
-- "unmounted attachments=${C:brixGCObjectSidePayloadCount(attachment-lane) - attachments-before} templates=${C:brixGCObjectSidePayloadCount(template-lane) - templates-before} objects=${C:brixGCObjectSidePayloadCount(object-lane) - objects-before}"

markup-adopt(panel!, null, demo)
demo.markup-batch-queue.checkpoint()
-- "remounted=${panel-tag(panel!)} attachments=${C:brixGCObjectSidePayloadCount(attachment-lane) - attachments-before} templates=${C:brixGCObjectSidePayloadCount(template-lane) - templates-before} objects=${C:brixGCObjectSidePayloadCount(object-lane) - objects-before}"
. remounted-attachment = markup-attachment(panel! as Pointer)
?! Panic("cleanup-mode merged attachment retained its owning store")
    if remounted-attachment!.retained-store
panel = null
C:brixCollect()
do C:brixGCDrainCleanups()

━━━━━━━━━━━━━━━━━━━━
+ do make-panel Panel
    - tag I64

<Panel panel
    decor = Decor(tag)
= panel

━━━━━━━━━━━━━━━━━━━━
+ do panel-tag I64
    - panel Panel

. decor = panel.$decor
= decor!.tag

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
+ Panel
    is MarkupElement
    using PanelStyle
```

```output
mounted=77 attachments=0 templates=1 objects=1
unmounted attachments=0 templates=1 objects=1
remounted=77 attachments=0 templates=1 objects=1
reclaimed attachments=0 templates=0 objects-bounded=true
```

```yumi
/core/style
/core/markup
/core/tests/support/markup-fixtures
/BxGC/BxGC.h

. root = AllocationBatchRoot()
markup-mount-root(root, root)
root.markup-batch-queue.checkpoint()
. panel = AllocationPanel()
. registrations-before = C:brixGCCleanupRegistrationCount()

. cold-before = managed-allocation-count()
do allocation-mount-cycle(root, panel)
. cold-after = managed-allocation-count()

. warm-before = managed-allocation-count()
for cycle in 0 .. < 32
    do allocation-mount-cycle(root, panel)
. warm-after = managed-allocation-count()

. cold-delta = cold-after - cold-before
. warm-delta = warm-after - warm-before
?! Panic("cold style mount allocated $cold-delta objects instead of its required 8")
    if cold-delta != 8
?! Panic("warm style cycles allocated ${warm-delta} objects instead of their required 192")
    if warm-delta != 32 * 6
?! Panic("style mount registered scalar cleanup handles")
    if C:brixGCCleanupRegistrationCount() != registrations-before

-- "cold=${cold-delta} warm=${warm-delta} registrations=${C:brixGCCleanupRegistrationCount() - registrations-before}"

━━━━━━━━━━━━━━━━━━━━
+ do managed-allocation-count U64

= C:brixGCAllocatedObjectCount() + C:brixGCThreadPendingAllocatedObjectCount()

━━━━━━━━━━━━━━━━━━━━
+ do allocation-mount-cycle
    - root AllocationBatchRoot
    - panel AllocationPanel

markup-adopt(panel, null, root)
root.markup-batch-queue.checkpoint()
markup-release(panel)
root.markup-batch-queue.checkpoint()

━━━━━━━━━━━━━━━━━━━━
+ AllocationBatchRoot is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ AllocationPanel is Box
```

```output
cold=8 warm=192 registrations=0
```

```yumi
/core/style
/core/markup
/core/tests/support/markup-fixtures

. root = MutationRoot()
markup-mount-root(root, root)
. panel = MutationPanel()
panel.amount = 7
panel.decor = MutationDecor(9)
-- "unmounted amount=${panel.$amount} decor=${panel.$decor.tag}"
markup-adopt(panel, null, root)
root.markup-batch-queue.checkpoint()
-- "mounted amount=${panel.$amount} decor=${panel.$decor.tag}"

. inherited = MutationSubPanel()
inherited.amount = 23
inherited.decor = MutationDecor(29)
markup-adopt(inherited, null, root)
root.markup-batch-queue.checkpoint()
-- "inherited amount=${inherited.$amount} decor=${inherited.$decor.tag}"

. mounted = MutationSubPanel()
markup-adopt(mounted, null, root)
root.markup-batch-queue.checkpoint()
mounted.amount = 31
mounted.decor = MutationDecor(37)
root.markup-batch-queue.checkpoint()
-- "mounted-first-write amount=${mounted.$amount} decor=${mounted.$decor.tag}"

panel.amount = 11
panel.decor = MutationDecor(13)
root.markup-batch-queue.checkpoint()
-- "updated amount=${panel.$amount} decor=${panel.$decor.tag}"

markup-release(panel)
panel.amount = 17
panel.decor = MutationDecor(19)
-- "released amount=${panel.$amount} decor=${panel.$decor.tag}"
markup-adopt(panel, null, root)
root.markup-batch-queue.checkpoint()
-- "remounted amount=${panel.$amount} decor=${panel.$decor.tag}"

━━━━━━━━━━━━━━━━━━━━
+ MutationRoot is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ MutationDecor

+ tag I64

────────────────────
+ This
    - tag

━━━━━━━━━━━━━━━━━━━━
+ MutationStyle is Style

+ amount I64 = 1
+ decor MutationDecor = MutationDecor(2)

━━━━━━━━━━━━━━━━━━━━
+ MutationPanel is Box
    using MutationStyle

━━━━━━━━━━━━━━━━━━━━
+ MutationSubPanel is MutationPanel
```

```output
unmounted amount=7 decor=9
mounted amount=7 decor=9
inherited amount=23 decor=29
mounted-first-write amount=31 decor=37
updated amount=11 decor=13
released amount=17 decor=19
remounted amount=17 decor=19
```

```yumi
/core/style
/core/style-debug
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Label parent
        ink = 9
        <Shadow region
            <Plain child

demo.markup-batch-queue.checkpoint()

-- style-attachment-snapshot(region)
-- style-attachment-snapshot(child) != "none"
-- style-computed-snapshot(child)

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

━━━━━━━━━━━━━━━━━━━━
+ Plain is Box

━━━━━━━━━━━━━━━━━━━━
+ Shadow is Box

────────────────────
+ get markup-is-transparent-region Bool

true
```

```output
none
true
node TextStyle seg=parent gen=1 mask=1 ink=9
```
