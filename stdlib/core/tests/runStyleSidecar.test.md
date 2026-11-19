# Style sidecar

### Mounting under a batch root creates one attachment per live object. Nodes with no authored style stay recordless; draining clears conservative dirty bits.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> TextBox (value: "a"))
    (<> TextBox (value: "b"))
  )
  (blank)
  (let first (!! (. demo %first-child)))
  (print (call style-attachment-snapshot demo))
  (print (call style-attachment-snapshot first))
  (print (call style-store-snapshot demo))
  (print (call style-drain-dirty-queue demo))
  (print (call style-attachment-snapshot first))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)
```

```output
live id=1 root=1 parent=- node=- gen=0 dirty=SI
live id=2 root=1 parent=1 node=- gen=0 dirty=SI
store id=1 live=3 attachments=3 free-attachments=0 nodes=0 free-nodes=0 segments=0 free-segments=0 queue=3 inherited-roots=0
applied=3 stale=0
live id=2 root=1 parent=1 node=- gen=0 dirty=--
```

### Objects that never enter a styled tree pay no visible style storage, including markup built without a participating MarkupRootElement root.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let plain (call Box))
  (let text (call TextBox "x"))
  (call markup-append plain text)
  (blank)
  (<> Box (id: box)
    (<> TextBox (value: "y"))
  )
  (blank)
  (print (call style-attachment-snapshot plain))
  (print (call style-attachment-snapshot text))
  (print (call style-attachment-snapshot box))
  (print (call style-attachment-snapshot (!! (. box %first-child))))
)
```

```output
none
none
none
none
```

### Unmounting releases records and owned segments back to the root store pools and turns pending dirty-queue entries stale instead of touching released records.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> TextBox (value: "a"))
  )
  (blank)
  (let child (!! (. demo %first-child)))
  (print (call style-drain-dirty-queue demo))
  (call style-debug-touch-node-segment child)
  (print (call style-node-snapshot child))
  (call markup-remove demo child)
  (print (call style-attachment-snapshot child))
  (print (call style-store-snapshot demo))
  (print (call style-drain-dirty-queue demo))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)
```

```output
applied=2 stale=0
node id=0 seg=owned gen=1 mask=1
none
store id=1 live=1 attachments=2 free-attachments=1 nodes=1 free-nodes=1 segments=1 free-segments=1 queue=1 inherited-roots=0
applied=0 stale=1
```

### Mounting the same object in two places at once fails deterministically.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Box)
    (<> Box)
  )
  (blank)
  (let a (!! (. demo %first-child)))
  (let b (!! (. demo %last-child)))
  (let text (call TextBox "t"))
  (call markup-append a text)
  (blank)
  (let attempt
    (do-new
      (call markup-append b text)
      (print "unreachable")
      (on-failure failure
        (let panic (? Panic) failure)
        (if panic
          (print (str "recovered " panic))
          (print (str "a-children=" (call count-children a) " b-children=" (call count-children b) " parent-a=" (== (. text %parent) a) " parent-b=" (== (. text %parent) b)))
        )
      )
    )
  )
  (call (. attempt wait))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)

(fn-do + count-children Size
  (param box Box)
  (let count Size 0)
  (for (in child (. box children))
    (+= count 1)
  )
  (return count)
)
```

```output
recovered Markup child is already linked
a-children=1 b-children=0 parent-a=true parent-b=false
```

### Any move — within the same root or between roots — is a style unmount followed by remount. The merged owner attachment persists while its logical style identity is released and reused.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Box)
    (<> Box)
  )
  (blank)
  (let a (!! (. demo %first-child)))
  (let b (!! (. demo %last-child)))
  (let text (call TextBox "t"))
  (call markup-append a text)
  (print (call style-drain-dirty-queue demo))
  (print (call style-attachment-snapshot text))
  (blank)
  (call markup-remove a text)
  (print (call style-attachment-snapshot text))
  (call markup-append b text)
  (print (call style-attachment-snapshot text))
  (print (call style-store-snapshot demo))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)
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

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: left)
    (<> TextBox (value: "a"))
  )
  (blank)
  (<> BatchDemo (id: right)
    (<> TextBox (value: "b"))
  )
  (blank)
  (let child (!! (. left %first-child)))
  (print (call style-attachment-snapshot child))
  (blank)
  (call markup-remove left child)
  (call markup-append right child)
  (print (call style-attachment-snapshot child))
  (print (call style-store-snapshot left))
  (print (call style-store-snapshot right))
  (blank)
  (let plain (call Box))
  (call markup-remove right child)
  (call markup-append plain child)
  (print (call style-attachment-snapshot child))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)
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

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: left)
    (<> TextBox (value: "a"))
  )
  (blank)
  (<> BatchDemo (id: right)
    (<> TextBox (value: "b"))
  )
  (blank)
  (let child (!! (. left %first-child)))
  (call style-debug-touch-node-segment child)
  (print (call style-node-snapshot child))
  (blank)
  (call markup-remove left child)
  (call markup-append right child)
  (print (call style-node-snapshot child))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)
```

The remounted unstyled object stays recordless until authored state appears.

```output
node id=0 seg=owned gen=1 mask=1
none
```

### Persistent pending-domain bits live on the sidecar attachment: accessor writes raise and clear them, the root's per-domain counts track live nodes exactly, idempotent writes do not double-count, and unmounting a node releases its pending state.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Marked (id: a)
      (= ink 9)
    )
    (<> Marked (id: b))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-pending-snapshot demo))
  (print (str "a=" (. a $is-paint-invalid) " b=" (. b $is-paint-invalid)))
  (blank)
  (= (. b $is-paint-invalid) true)
  (= (. b $is-paint-invalid) true)
  (print (call style-pending-snapshot demo))
  (blank)
  (= (. a $is-paint-invalid) false)
  (print (call style-pending-snapshot demo))
  (print (str "a=" (. a $is-paint-invalid) " b=" (. b $is-paint-invalid)))
  (blank)
  (call markup-remove demo b)
  (print (call style-pending-snapshot demo))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)

(type + MarkStyle (is Style)
  (prop + ink I64 (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(type + Marked (is Box)
  (using MarkStyle)
)
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

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Box (id: a))
    (<> Box (id: b))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (= (. b $is-audit-invalid) true)
  (let audit-on (call style-domain-pending-count demo "audit"))
  (print (str "audit=" audit-on " b=" (. b $is-audit-invalid)))
  (blank)
  (= (. b $is-audit-invalid) false)
  (let audit-off (call style-domain-pending-count demo "audit"))
  (print (str "audit=" audit-off " b=" (. b $is-audit-invalid)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)
```

```output
audit=1 b=true
audit=0 b=false
```

### Region boxes created by markup loops participate. Style parentage is derived from the structural tree, so linking the parent late requires no second dirty or inherited-root entry.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (blank)
  (let region (!! (. demo %first-child)))
  (let text (!! (. region %first-child)))
  (print (call style-attachment-snapshot region))
  (print (call style-attachment-snapshot text))
  (print (call style-store-snapshot demo))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + items (list String) (= (call (of List String) (list "a" "b"))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)
```

The region receives its batch root before linking to a parent. Derived traversal observes the
structural link directly, so the original mount entries are sufficient.

```output
live id=2 root=1 parent=1 node=- gen=0 dirty=SI
live id=3 root=1 parent=2 node=- gen=0 dirty=SI
store id=1 live=4 attachments=4 free-attachments=0 nodes=0 free-nodes=0 segments=0 free-segments=0 queue=4 inherited-roots=0
```

### Static style declarations from one construction site are registered once even when the site creates many instances.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import /BxGC/BxGC.h)
(blank)

(main
  (let template-lane (call C:brixGCStyleTemplatesSidePayloadLane))
  (let templates-before (call C:brixGCObjectSidePayloadCount template-lane))
  (blank)
  (<> BatchDemo (id: demo)
    (for (in item (. demo #items))
      (<> Label
        (= ink 12)
        (= tone 4)
      )
    )
  )
  (blank)
  (let declarations (call style-site-template-declaration-count))
  (let templates (- (call C:brixGCObjectSidePayloadCount template-lane) templates-before))
  (print (str "declarations=" declarations " templates=" templates))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + items (list String) (= (call (of List String) (list "a" "b" "c" "d" "e" "f" "g" "h"))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
  (prop + tone I64 (= 3))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
declarations=2 templates=8
```

### The style side-payload lane now belongs only to the batch root. Non-root style state lives in the merged markup attachment: ordinary unmount releases only its mounted identity so templates and object values survive remount, while dropping a mounted parentless owner runs merged cleanup. Template and object lanes return exactly; the object lane is bounded by one because the file runner shares its process with sibling cases.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import /BxGC/BxGC.h)
(blank)

(main
  (let attachment-lane (call C:brixGCStyleAttachmentsSidePayloadLane))
  (let template-lane (call C:brixGCStyleTemplatesSidePayloadLane))
  (let object-lane (call C:brixGCStyleObjectsSidePayloadLane))
  (blank)
  (<> BatchDemo (id: demo))
  (blank)
  (let attachments-before (call C:brixGCObjectSidePayloadCount attachment-lane))
  (let templates-before (call C:brixGCObjectSidePayloadCount template-lane))
  (let objects-before (call C:brixGCObjectSidePayloadCount object-lane))
  (blank)
  (do
    (inline (call exercise-reclaim demo attachment-lane template-lane object-lane attachments-before templates-before objects-before))
  )
  (blank)
  (call C:brixCollect)
  (do
    (inline (call C:brixGCDrainCleanups))
  )
  (let objects-final (call C:brixGCObjectSidePayloadCount object-lane))
  (print (str "reclaimed attachments=" (- (call C:brixGCObjectSidePayloadCount attachment-lane) attachments-before) " templates=" (- (call C:brixGCObjectSidePayloadCount template-lane) templates-before) " objects-bounded=" (<= objects-final (+ objects-before 1))))
)

(fn-do + exercise-reclaim _
  (param demo BatchDemo)
  (param attachment-lane U64)
  (param template-lane U64)
  (param object-lane U64)
  (param attachments-before U64)
  (param templates-before U64)
  (param objects-before U64)
  (lazy panel (? Panel) (call make-panel 77))
  (blank)
  (call markup-adopt (!! panel) null demo)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "mounted=" (call panel-tag (!! panel)) " attachments=" (- (call C:brixGCObjectSidePayloadCount attachment-lane) attachments-before) " templates=" (- (call C:brixGCObjectSidePayloadCount template-lane) templates-before) " objects=" (- (call C:brixGCObjectSidePayloadCount object-lane) objects-before)))
  (blank)
  (call markup-release (!! panel))
  (call (. demo markup-batch-queue checkpoint))
  (print (str "unmounted attachments=" (- (call C:brixGCObjectSidePayloadCount attachment-lane) attachments-before) " templates=" (- (call C:brixGCObjectSidePayloadCount template-lane) templates-before) " objects=" (- (call C:brixGCObjectSidePayloadCount object-lane) objects-before)))
  (blank)
  (call markup-adopt (!! panel) null demo)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "remounted=" (call panel-tag (!! panel)) " attachments=" (- (call C:brixGCObjectSidePayloadCount attachment-lane) attachments-before) " templates=" (- (call C:brixGCObjectSidePayloadCount template-lane) templates-before) " objects=" (- (call C:brixGCObjectSidePayloadCount object-lane) objects-before)))
  (let remounted-attachment (call markup-attachment (as (!! panel) Pointer)))
  (throw (call Panic "cleanup-mode merged attachment retained its owning store") (if (. (!! remounted-attachment) retained-store)))
  (= panel null)
  (call C:brixCollect)
  (do
    (inline (call C:brixGCDrainCleanups))
  )
)

(fn-do + make-panel Panel
  (param tag I64)
  (<> Panel (id: panel)
    (= decor (call Decor tag))
  )
  (return panel)
)

(fn-do + panel-tag I64
  (param panel Panel)
  (let decor (. panel $decor))
  (return (. (!! decor) tag))
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

(type + Panel
  (implements MarkupElement)
  (using PanelStyle)
)
```

```output
mounted=77 attachments=0 templates=1 objects=1
unmounted attachments=0 templates=1 objects=1
remounted=77 attachments=0 templates=1 objects=1
reclaimed attachments=0 templates=0 objects-bounded=true
```

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import /BxGC/BxGC.h)
(blank)

(main
  (let root (call AllocationBatchRoot))
  (call markup-mount-root root root)
  (call (. root markup-batch-queue checkpoint))
  (let panel (call AllocationPanel))
  (let registrations-before (call C:brixGCCleanupRegistrationCount))
  (blank)
  (let cold-before (call managed-allocation-count))
  (do
    (inline (call allocation-mount-cycle root panel))
  )
  (let cold-after (call managed-allocation-count))
  (blank)
  (let warm-before (call managed-allocation-count))
  (for (in cycle (..< 0 32))
    (do
      (inline (call allocation-mount-cycle root panel))
    )
  )
  (let warm-after (call managed-allocation-count))
  (blank)
  (let cold-delta (- cold-after cold-before))
  (let warm-delta (- warm-after warm-before))
  (throw (call Panic (str "cold style mount allocated " cold-delta " objects instead of its required 8")) (if (!= cold-delta 8)))
  (throw (call Panic (str "warm style cycles allocated " warm-delta " objects instead of their required 192")) (if (!= warm-delta (* 32 6))))
  (throw (call Panic "style mount registered scalar cleanup handles") (if (!= (call C:brixGCCleanupRegistrationCount) registrations-before)))
  (blank)
  (print (str "cold=" cold-delta " warm=" warm-delta " registrations=" (- (call C:brixGCCleanupRegistrationCount) registrations-before)))
)

(fn-do + managed-allocation-count U64
  (return (+ (call C:brixGCAllocatedObjectCount) (call C:brixGCThreadPendingAllocatedObjectCount)))
)

(fn-do + allocation-mount-cycle _
  (param root AllocationBatchRoot)
  (param panel AllocationPanel)
  (call markup-adopt panel null root)
  (call (. root markup-batch-queue checkpoint))
  (call markup-release panel)
  (call (. root markup-batch-queue checkpoint))
)

(type + AllocationBatchRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + AllocationPanel (is Box))
```

```output
cold=8 warm=192 registrations=0
```

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let root (call MutationRoot))
  (call markup-mount-root root root)
  (let panel (call MutationPanel))
  (= (. panel amount) 7)
  (= (. panel decor) (call MutationDecor 9))
  (print (str "unmounted amount=" (. panel $amount) " decor=" (. panel $decor tag)))
  (call markup-adopt panel null root)
  (call (. root markup-batch-queue checkpoint))
  (print (str "mounted amount=" (. panel $amount) " decor=" (. panel $decor tag)))
  (blank)
  (let inherited (call MutationSubPanel))
  (= (. inherited amount) 23)
  (= (. inherited decor) (call MutationDecor 29))
  (call markup-adopt inherited null root)
  (call (. root markup-batch-queue checkpoint))
  (print (str "inherited amount=" (. inherited $amount) " decor=" (. inherited $decor tag)))
  (blank)
  (let mounted (call MutationSubPanel))
  (call markup-adopt mounted null root)
  (call (. root markup-batch-queue checkpoint))
  (= (. mounted amount) 31)
  (= (. mounted decor) (call MutationDecor 37))
  (call (. root markup-batch-queue checkpoint))
  (print (str "mounted-first-write amount=" (. mounted $amount) " decor=" (. mounted $decor tag)))
  (blank)
  (= (. panel amount) 11)
  (= (. panel decor) (call MutationDecor 13))
  (call (. root markup-batch-queue checkpoint))
  (print (str "updated amount=" (. panel $amount) " decor=" (. panel $decor tag)))
  (blank)
  (call markup-release panel)
  (= (. panel amount) 17)
  (= (. panel decor) (call MutationDecor 19))
  (print (str "released amount=" (. panel $amount) " decor=" (. panel $decor tag)))
  (call markup-adopt panel null root)
  (call (. root markup-batch-queue checkpoint))
  (print (str "remounted amount=" (. panel $amount) " decor=" (. panel $decor tag)))
)

(type + MutationRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + MutationDecor
  (prop + tag I64)
  (fn-do + This _
    (param tag _)
  )
)

(type + MutationStyle (is Style)
  (prop + amount I64 (= 1))
  (prop + decor MutationDecor (= (call MutationDecor 2)))
)

(type + MutationPanel (is Box)
  (using MutationStyle)
)

(type + MutationSubPanel (is MutationPanel))
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

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label (id: parent)
      (= ink 9)
      (<> Shadow (id: region)
        (<> Plain (id: child))
      )
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-attachment-snapshot region))
  (print (!= (call style-attachment-snapshot child) "none"))
  (print (call style-computed-snapshot child))
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

(type + Plain (is Box))

(type + Shadow (is Box)
  (fn-expr + markup-is-transparent-region Bool :get
    true
  )
)
```

```output
none
true
node TextStyle seg=parent gen=1 mask=1 ink=9
```
