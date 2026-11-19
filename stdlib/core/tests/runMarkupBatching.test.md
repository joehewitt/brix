# Markup batching

```brix
(import /core/markup)
(import /core/task)
(blank)

(main
  (lazy name "bob")
  (let done (call Semaphore))
  (blank)
  (<> BatchRoot (id: root)
    (<> MarkText
      (= text #name)
    )
  )
  (blank)
  (print (call dump root))
  (= name "tom")
  (= name "fred")
  (print (call dump root))
  (print (. root flush-count))
  (blank)
  (do-new
    (print (call dump root))
    (print (. root flush-count))
    (call (. done post))
  )
  (blank)
  (call (. done wait))
)

(fn-expr + dump String
  (param root BatchRoot)
  (. (as (!! (. root first-child)) MarkText) text)
)

(type + MarkNode
  (prop + parent (? MarkNode))
  (prop + first-child (? MarkNode))
  (prop + last-child (? MarkNode))
  (prop + previous (? MarkNode))
  (prop + next (? MarkNode))
  (fn-do + (infix this += (param child MarkNode)) _
    (= (. child parent) this)
    (if (not first-child)
      (= first-child (= last-child child))
      (else
        (= (?. last-child next) child)
        (= (. child previous) last-child)
        (= last-child child)
      )
    )
    (return child)
  )
  (fn-do + (infix this -= (param child MarkNode)) _
    (if (== (. child parent) this)
      (= (. child parent) null)
      (if (== first-child child)
        (= first-child (. child next))
      )
      (if (== last-child child)
        (= last-child (. child previous))
      )
      (= (?. (. child previous) next) (. child next))
      (= (?. (. child next) previous) (. child previous))
      (= (. child next) null)
      (= (. child previous) null)
    )
  )
  (fn-do + clear-children _
    (while first-child
      (let child (!! first-child))
      (-= this child)
      (call (. child close))
    )
  )
  (fn-do + close _
    (call clear-children)
  )
)

(type + BatchRoot (is MarkNode)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)
```

```output
bob
bob
0
fred
1
```

### Mount/unmount callbacks retain their batching order across a live remount. Owner reclamation clears the attachment lane only after the final mounted owner is dropped and emits no synthetic unmounted callback.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import /BxGC/BxGC.h)
(blank)

(main
  (let lane (call C:brixGCMarkupAttachmentsSidePayloadLane))
  (let root (call CleanupBatchRoot))
  (call markup-mount-root root root)
  (call (. root markup-batch-queue checkpoint))
  (call (. root events clear))
  (let opening (call C:brixGCObjectSidePayloadCount lane))
  (blank)
  (do
    (inline (call exercise-cleanup-batch-order root lane opening))
  )
  (call C:brixCollect)
  (do
    (inline (call C:brixGCDrainCleanups))
  )
  (blank)
  (print (str "events=" (call (. "|" join) (. root events))))
  (print (str "reclaimed=" (- (call C:brixGCObjectSidePayloadCount lane) opening)))
)

(fn-do + exercise-cleanup-batch-order _
  (param root CleanupBatchRoot)
  (param lane U64)
  (param opening U64)
  (lazy child (? CleanupBatchChild) (call CleanupBatchChild root))
  (blank)
  (call markup-adopt (!! child) null root)
  (call (. root markup-batch-queue checkpoint))
  (+= (. root events) (str "lane:" (- (call C:brixGCObjectSidePayloadCount lane) opening)))
  (blank)
  (call markup-release (!! child))
  (call (. root markup-batch-queue checkpoint))
  (blank)
  (call markup-adopt (!! child) null root)
  (call (. root markup-batch-queue checkpoint))
  (= child null)
)

(type + CleanupBatchRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (prop + events (list String) (= (call (of List String))))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= events "flush")
  )
)

(type + CleanupBatchChild (is Box)
  (prop + sink CleanupBatchRoot)
  (fn-do + This _
    (param sink _)
  )
  (fn-do + mounted _
    (+= (. sink events) "mounted")
  )
  (fn-do + unmounted _
    (+= (. sink events) "unmounted")
  )
)
```

```output
events=mounted|flush|lane:1|unmounted|mounted|flush
reclaimed=0
```

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> EarlyMountedQueryRoot (id: root)
    (<> QueryMirror (id: mirror))
  )
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (print (call count-children root))
  (print (. root nodes length))
  (print (if-expr (. root nodes) (is ([] (. root nodes) 0) QueryMarker) false))
  (print (== (. mirror owner) root))
)

(type + QueryMarker (is Box))

(type + QueryMirror (is Box)
  (prop + owner (of Ancestor EarlyMountedQueryRoot))
)

(fn-do + count-children Size
  (param box Box)
  (let count Size 0)
  (for (in child (. box children))
    (+= count 1)
  )
  (return count)
)

(type + EarlyMountedQueryRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + nodes (of Descendants QueryMarker))
  (fn-do + This _
    (call markup-mount-root this this)
    (call (. markup-batch-queue checkpoint))
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (content
    (<> This
      (<> QueryMarker)
    )
  )
)
```

```output
2
1
true
true
```

### Keyed reconciliation preserves identity and duplicate-key order around the measured crossover.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (print (call keyed-crossover-summary 31))
  (print (call keyed-crossover-summary 32))
  (print (call keyed-crossover-summary 33))
  (print (call keyed-crossover-duplicate-summary))
)

(type + CrossoverItem (is Tuple)
  (prop + id String)
  (prop + text String)
)

(type + CrossoverRow (is Label)
  (prop + sink CrossoverRoot)
  (prop + local-state Size (= 0))
  (prop + focused _ (= false))
  (prop + selected _ (= false))
  (fn-do + mounted _
    (+= (. sink events) (str "M" text))
  )
  (fn-do + unmounted _
    (+= (. sink events) (str "U" text))
  )
)

(type + CrossoverRoot (is Box)
  (implements MarkupRootElement)
  (prop + items (list CrossoverItem) (= (call (of List CrossoverItem))))
  (prop + events (list String) (= (call (of List String))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + This _
    (param count Size)
    (= items (call make-crossover-items count))
  )
  (fn-do + replace-with _
    (param replacement (list CrossoverItem))
    (= items replacement)
    (call (. markup-batch-queue checkpoint))
  )
  (content
    (<> This
      (for (in item #items (keyed-by (call crossover-key item)))
        (<> CrossoverRow
          (= sink this)
          (= text (. item text))
        )
      )
    )
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(fn-do + keyed-crossover-summary String
  (param count Size)
  (let root (call CrossoverRoot count))
  (call markup-mount-root root root)
  (call (. root markup-batch-queue checkpoint))
  (let region (!! (. root %first-child)))
  (let opening (list CrossoverRow))
  (let index Index 0)
  (for (in child (. region children))
    (let row (as child CrossoverRow))
    (= (. row local-state) (+ index 100))
    (+= opening row)
    (+= index 1)
  )
  (blank)
  (let middle ([] opening (/ count 2)))
  (= (. middle focused) true)
  (= (. middle selected) true)
  (call (. root replace-with) (call reverse-crossover-items (. root items)))
  (blank)
  (let identity-ok true)
  (= index 0)
  (for (in child (. region children))
    (let row (as child CrossoverRow))
    (let old-index (- count index 1))
    (if (or (!= row ([] opening old-index)) (!= (. row local-state) (+ old-index 100)))
      (= identity-ok false)
    )
    (+= index 1)
  )
  (blank)
  (let current-middle (as (!! (call (. region child-at) (- count (/ count 2) 1))) CrossoverRow))
  (let interaction-ok (and (== current-middle middle) (. current-middle focused) (. current-middle selected)))
  (return (str count ":" identity-ok ":" interaction-ok))
)

(fn-do + keyed-crossover-duplicate-summary String
  (let root (call CrossoverRoot 33))
  (= ([] (. root items) 0) (call CrossoverItem "duplicate" "first"))
  (= ([] (. root items) 1) (call CrossoverItem "duplicate" "second"))
  (call markup-mount-root root root)
  (call (. root markup-batch-queue checkpoint))
  (let region (!! (. root %first-child)))
  (let first (!! (. region %first-child)))
  (let second (!! (call (. region child-at) 1)))
  (let replacement (call make-crossover-items 33))
  (= ([] replacement 0) (call CrossoverItem "duplicate" "new-first"))
  (= ([] replacement 1) (call CrossoverItem "duplicate" "new-second"))
  (call (. root replace-with) replacement)
  (let fifo (and (== first (!! (. region %first-child))) (== second (!! (call (. region child-at) 1)))))
  (return (str "duplicate-fifo=" fifo))
)

(fn-do + make-crossover-items (list CrossoverItem)
  (param count Size)
  (let items (list CrossoverItem))
  (for (in index (..< 0 count))
    (+= items (call CrossoverItem (str "id-" index) (str "row-" index)))
  )
  (return items)
)

(fn-do + reverse-crossover-items (list CrossoverItem)
  (param source (list CrossoverItem))
  (let items (list CrossoverItem))
  (let index (. source length))
  (while (> index 0)
    (-= index 1)
    (+= items ([] source index))
  )
  (return items)
)

(fn-do + crossover-key String
  (param item CrossoverItem)
  (if (== (. item id) "boom")
    (throw (call Panic "key failure"))
  )
  (return (. item id))
)
```

```output
31:true:true
32:true:true
33:true:true
duplicate-fifo=true
```

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> QueryOrderRoot (id: root))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (let first ([] (. root markers) 0))
  (let second ([] (. root markers) 1))
  (print (call marker-order root))
  (call (. root items move) 1 0)
  (call (. root markup-batch-queue checkpoint))
  (print (call marker-order root))
  (print (if-expr (== second ([] (. root markers) 0)) "second-reused" "second-remounted"))
  (print (if-expr (== first ([] (. root markers) 1)) "first-reused" "first-remounted"))
)

(type + RowData (is Tuple)
  (prop + id String)
  (prop + text String)
)

(type + MarkerBox (is Box)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(fn-expr + marker-order String
  (param root QueryOrderRoot)
  (call (. "|" join) (in (. each text) (. root markers)))
)

(type + QueryOrderRoot (is Box)
  (implements MarkupRootElement)
  (prop + items (list RowData) (= (call (of List RowData) (list (call RowData "a" "A") (call RowData "b" "B")))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + markers (of Descendants MarkerBox))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (content
    (<> This
      (for (in item #items (keyed-by (. item id)))
        (<> MarkerBox (value: (. item text)))
      )
    )
  )
)
```

```output
A|B
B|A
second-reused
first-reused
```

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> QueryLifecycleRoot (id: root))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (print (call event-marker-list (. root markers)))
  (print (call event-marker-list (. root left markers)))
  (print (call event-marker-list (. root right markers)))
  (blank)
  (let branch (call QueryEventBranch "alpha"))
  (call markup-append (. root left) branch)
  (call (. root markup-batch-queue checkpoint))
  (print (call event-marker-list (. root markers)))
  (print (call event-marker-list (. root left markers)))
  (print (call event-marker-list (. root right markers)))
  (blank)
  (call markup-remove (. root left) branch)
  (call (. root markup-batch-queue checkpoint))
  (print (call event-marker-list (. root markers)))
  (print (call event-marker-list (. root left markers)))
  (blank)
  (call markup-append (. root left) branch)
  (call (. root markup-batch-queue checkpoint))
  (print (call event-marker-list (. root markers)))
  (print (call event-marker-list (. root left markers)))
  (print (call event-marker-list (. root right markers)))
  (blank)
  (<> QuerySeedOwner (id: seeded)
    (<> QueryEventBox
      (<> QueryEventMarker (value: "seeded"))
    )
  )
  (blank)
  (call (. seeded markup-batch-queue checkpoint))
  (print (call event-marker-list (. seeded markers)))
)

(type + QueryEventMarker (is Box)
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + QueryEventBox (is Box))

(type + QueryEventBranch (is Box)
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
  (content
    (<> This
      (<> QueryEventBox
        (<> QueryEventMarker (value: text))
      )
    )
  )
)

(type + QueryEventPane (is Box)
  (prop + markers (of Descendants QueryEventMarker))
)

(type + QueryLifecycleRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + left _ (:= (call QueryEventPane)))
  (prop + right _ (:= (call QueryEventPane)))
  (prop + markers (of Descendants QueryEventMarker))
  (fn-do + This _
    (call markup-mount-root this this)
    (call markup-append this left)
    (call markup-append this right)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
)

(type + QuerySeedOwner (is QueryEventPane)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
)

(fn-do + event-marker-list String
  (param markers (list QueryEventMarker))
  (if (not markers)
    (return "none")
  )
  (blank)
  (return (call (. "|" join) (in (. each text) markers)))
)
```

```output
none
none
none
alpha
alpha
none
none
none
alpha
alpha
none
seeded
```

```brix compile-error
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let root (call QueryAssignmentRoot))
  (= (. root markers) (call (of List QueryAssignmentMarker)))
)

(type + QueryAssignmentMarker (is Box))

(type + QueryAssignmentRoot (is Box)
  (prop + markers (of Descendants QueryAssignmentMarker))
)
```

```output
Exception: Markup query properties are compiler-managed and cannot be assigned
<no file:8>
      (= (. root markers) (call (of List QueryAssignmentMarker)))
         ^^^^^^^^^^^^^^^^
```

```brix compile-error
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let child (call QueryAncestorAssignmentChild))
  (= (. child owner) null)
)

(type + QueryAncestorAssignmentRoot (is Box))

(type + QueryAncestorAssignmentChild (is Box)
  (prop + owner (of Ancestor QueryAncestorAssignmentRoot))
)
```

```output
Exception: Markup query properties are compiler-managed and cannot be assigned
<no file:8>
      (= (. child owner) null)
         ^^^^^^^^^^^^^^^
```

```brix
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (blank)
  (let plain (call Box))
  (let region (!! (. demo %first-child)))
  (call markup-remove demo region)
  (call markup-append plain region)
  (blank)
  (+= (. demo items) "c")
  (print (call dump region))
  (print (. demo flush-count))
  (blank)
  (do-new
    (print (call dump region))
    (print (. demo flush-count))
    (call (. done post))
  )
  (blank)
  (call (. done wait))
)

(fn-expr + dump String
  (param region Box)
  (str count ":" last-text)
  (where
    (lazy count (call count-children region))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
  )
)

(fn-do + count-children _
  (param box Box)
  (let count 0)
  (for (in child (. box children))
    (+= count 1)
  )
  (return count)
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + items (list String) (= (call (of List String) (list "a" "b"))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)
```

The initial mount settle notifies demo once on the first task tick; the detached region's
mutation adds nothing.

```output
3:c
0
3:c
1
```

```brix
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (blank)
  (print (call dump demo))
  (+= (. demo items) "c")
  (+= (. demo items) "d")
  (print (call dump demo))
  (print (. demo flush-count))
  (blank)
  (do-new
    (print (call dump demo))
    (print (. demo flush-count))
    (call (. done post))
  )
  (blank)
  (call (. done wait))
)

(fn-expr + dump String
  (param demo BatchDemo)
  (str count ":" first-text ":" last-text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy count (call count-children region))
    (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
  )
)

(fn-do + count-children _
  (param box Box)
  (let count 0)
  (for (in child (. box children))
    (+= count 1)
  )
  (return count)
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + items (list String) (= (call (of List String) (list "a" "b"))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)
```

```output
2:a:b
2:a:b
0
4:a:d
1
```

```brix
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (blank)
  (print (call dump demo))
  (+= (. demo items) "c")
  (-= (. demo items) "c")
  (print (call dump demo))
  (blank)
  (do-new
    (print (call dump demo))
    (print (. demo flush-count))
    (call (. done post))
  )
  (blank)
  (call (. done wait))
)

(fn-expr + dump String
  (param demo BatchDemo)
  (str count ":" first-text ":" last-text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy count (call count-children region))
    (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
  )
)

(fn-do + count-children _
  (param box Box)
  (let count 0)
  (for (in child (. box children))
    (+= count 1)
  )
  (return count)
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + items (list String) (= (call (of List String) (list "a" "b"))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)
```

```output
2:a:b
2:a:b
2:a:b
1
```

```brix
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (blank)
  (print (call dump demo))
  (call (. demo items remove) 1)
  (+= (. demo items) "c")
  (print (call dump demo))
  (blank)
  (do-new
    (print (call dump demo))
    (print (. demo flush-count))
    (call (. done post))
  )
  (blank)
  (call (. done wait))
)

(fn-expr + dump String
  (param demo BatchDemo)
  (str count ":" first-text ":" last-text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy count (call count-children region))
    (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
  )
)

(fn-do + count-children _
  (param box Box)
  (let count 0)
  (for (in child (. box children))
    (+= count 1)
  )
  (return count)
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + items (list String) (= (call (of List String) (list "a" "b"))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)
```

```output
2:a:b
2:a:b
2:a:c
1
```

```brix
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: (+ item (. demo #suffix))))
    )
  )
  (blank)
  (print (call dump demo))
  (= (. demo suffix) "?")
  (print (call dump demo))
  (blank)
  (do-new
    (print (call dump demo))
    (print (. demo flush-count))
    (call (. done post))
  )
  (blank)
  (call (. done wait))
)

(fn-expr + dump String
  (param demo BatchDemo)
  (str count ":" first-text ":" last-text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy count (call count-children region))
    (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
  )
)

(fn-do + count-children _
  (param box Box)
  (let count 0)
  (for (in child (. box children))
    (+= count 1)
  )
  (return count)
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + items (list String) (= (call (of List String) (list "a" "b"))))
  (prop + suffix _ (= "!"))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)
```

```output
2:a!:b!
2:a!:b!
2:a?:b?
1
```

```brix
(import /core/markup)
(import /core/task)
(blank)

(main
  (let flag false)
  (let done (call Semaphore))
  (blank)
  (<> BatchRoot (id: root)
    (if #flag
      (<> MarkText (value: "on"))
      (else
        (<> MarkText (value: "off"))
      )
    )
  )
  (blank)
  (print (call dump root))
  (= flag true)
  (= flag false)
  (print (call dump root))
  (blank)
  (do-new
    (print (call dump root))
    (print (. root flush-count))
    (call (. done post))
  )
  (blank)
  (call (. done wait))
)

(fn-expr + dump String
  (param root BatchRoot)
  (str count ":" text)
  (where
    (lazy region (!! (. root first-child)))
    (lazy count (call count-children region))
    (lazy text (. (as (!! (. region first-child)) MarkText) text))
  )
)

(fn-do + count-children _
  (param node MarkNode)
  (let count 0)
  (for (in child (. node children))
    (+= count 1)
  )
  (return count)
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (prop + parent (? MarkNode))
  (prop + first-child (? MarkNode))
  (prop + last-child (? MarkNode))
  (prop + previous (? MarkNode))
  (prop + next (? MarkNode))
  (fn-do + children _ :get
    (let maybe-child first-child)
    (if maybe-child
      (let child maybe-child)
      (while true
        (yield child)
        (if (not (. child next))
          (break)
        )
        (= child (. child next))
      )
    )
  )
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
    (= (. child parent) this)
    (if (not anchor)
      (if (not first-child)
        (= first-child (= last-child child))
        (else
          (= (?. last-child next) child)
          (= (. child previous) last-child)
          (= last-child child)
        )
      )
      (else
        (let target anchor)
        (= (. child previous) (. target previous))
        (= (. child next) target)
        (if (. target previous)
          (= (. target previous next) child)
          (else
            (= first-child child)
          )
        )
        (= (. target previous) child)
      )
    )
  )
  (fn-do + markup-detach-child _
    (param child MarkNode)
    (if (== (. child parent) this)
      (= (. child parent) null)
      (if (== first-child child)
        (= first-child (. child next))
      )
      (if (== last-child child)
        (= last-child (. child previous))
      )
      (= (?. (. child previous) next) (. child next))
      (= (?. (. child next) previous) (. child previous))
      (= (. child next) null)
      (= (. child previous) null)
    )
  )
  (fn-do + clear-children _
    (while first-child
      (let child first-child)
      (call markup-remove this child)
      (call (. child close))
    )
  )
  (fn-do + close _
    (call markup-close-region this)
    (call clear-children)
  )
  (fn-expr + markup-make-region (? MarkNode)
    (call MarkNode)
  )
)

(type + BatchRoot (is MarkNode)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)
```

```output
1:off
1:off
1:off
1
```

```brix
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: left)
    (for (in item (. left #items))
      (<> TextBox (value: item))
    )
  )
  (blank)
  (let right (call BatchDemo))
  (call markup-mount-root right (as right MarkupRootElement))
  (let region (!! (. left %first-child)))
  (blank)
  (+= (. left items) "c")
  (blank)
  (do-new
    (call markup-adopt region (!! (. region %parent)) (as right MarkupRootElement))
    (+= (. left items) "d")
    (blank)
    (print (call dump region))
    (print (. left flush-count))
    (print (. right flush-count))
    (blank)
    (do-new
      (print (call dump region))
      (print (. left flush-count))
      (print (. right flush-count))
      (call (. done post))
    )
  )
  (blank)
  (call (. done wait))
)

(fn-expr + dump String
  (param region Box)
  (str count ":" last-text)
  (where
    (lazy count (call count-children region))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
  )
)

(fn-do + count-children _
  (param box Box)
  (let count 0)
  (for (in child (. box children))
    (+= count 1)
  )
  (return count)
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + items (list String) (= (call (of List String) (list "a" "b"))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + flush-count Size (= 0))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (+= flush-count 1)
  )
)
```

Each root is notified once for its initial mount settle: right's count starts at 1 before the
rehomed region's action flushes there. The pending region action follows the moved owner to
the right root, so left does not get a second flush.

```output
3:c
1
1
4:d
1
2
```

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> ChildTriggerRoot (id: root))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (let a (call ChildTriggerItem "a"))
  (let b (call ChildTriggerItem "b"))
  (call markup-append root a)
  (call markup-append root b)
  (print (call event-list (. root events)))
  (call (. root markup-batch-queue checkpoint))
  (print (call event-list (. root events)))
  (blank)
  (let nested (call ChildTriggerItem "nested"))
  (call markup-append a nested)
  (call (. root markup-batch-queue checkpoint))
  (print (call event-list (. root events)))
  (blank)
  (call markup-remove root b)
  (call (. root markup-batch-queue checkpoint))
  (print (call event-list (. root events)))
)

(type + ChildTriggerItem (is Box)
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + ChildTriggerRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + events (list String) (= (call (of List String))))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (whenever-rule (trigger children ChildTriggerItem)
    (+= events "children")
  )
)

(fn-do + event-list String
  (param events (list String))
  (if (not events)
    (return "none")
  )
  (blank)
  (return (call (. "|" join) events))
)
```

```output
none
children
children
children|children
```

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> ChangeTriggerRoot (id: root))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (let a (call ChangeTriggerMarker "a"))
  (let b (call ChangeTriggerMarker "b"))
  (call markup-append root a)
  (call markup-append root b)
  (call (. root markup-batch-queue checkpoint))
  (print (call event-list (. root events)))
  (blank)
  (call markup-remove root a)
  (call (. root markup-batch-queue checkpoint))
  (print (call event-list (. root events)))
)

(type + ChangeTriggerMarker (is Box)
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + ChangeTriggerRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + events (list String) (= (call (of List String))))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (whenever-rule (trigger descendants ChangeTriggerMarker)
    (param changes _)
    (+= events (call change-summary changes))
  )
)

(fn-expr + change-summary String
  (param changes (of MarkupChangeSet ChangeTriggerMarker))
  (str "added:" added " removed:" removed)
  (where
    (lazy added (call (. "|" join) (in (. each text) (. changes added))))
    (lazy removed (call (. "|" join) (in (. each text) (. changes removed))))
  )
)

(fn-do + event-list String
  (param events (list String))
  (if (not events)
    (return "none")
  )
  (blank)
  (return (call (. ";" join) events))
)
```

```output
added:a|b removed:
added:a|b removed:;added: removed:a
```

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let root (call MountTriggerRoot))
  (let child (call MountTriggerChild))
  (print (call event-list (. child events)))
  (call markup-mount-root root root)
  (call markup-append root child)
  (call (. root markup-batch-queue checkpoint))
  (print (call event-list (. child events)))
  (call markup-remove root child)
  (call (. root markup-batch-queue checkpoint))
  (print (call event-list (. child events)))
)

(type + MountTriggerRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
)

(type + MountTriggerChild (is Box)
  (prop + events (list String) (= (call (of List String))))
  (fn-do + mounted _
    (+= events "mounted")
  )
  (fn-do + unmounted _
    (+= events "unmounted")
  )
)

(fn-do + event-list String
  (param events (list String))
  (if (not events)
    (return "none")
  )
  (blank)
  (return (call (. "|" join) events))
)
```

```output
none
mounted
mounted|unmounted
```

### whenever #node-name observes a class-content markup id when it mounts, not when it unmounts

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> InternalIdTriggerRoot (id: root))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (print (call event-list (. root events)))
  (call markup-remove root (!! (. root %first-child)))
  (call (. root markup-batch-queue checkpoint))
  (print (call event-list (. root events)))
)

(type + InternalIdTriggerNode (is Box))

(type + InternalIdTriggerRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + events (list String) (= (call (of List String))))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (whenever-rule #named-child
    (+= events "named-child")
  )
  (content
    (<> This
      (<> InternalIdTriggerNode (id: named-child))
    )
  )
)

(fn-do + event-list String
  (param events (list String))
  (if (not events)
    (return "none")
  )
  (blank)
  (return (call (. "|" join) events))
)
```

```output
named-child
named-child
```

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let root (call AncestorTriggerRoot "root"))
  (let child (call AncestorTriggerChild))
  (call markup-mount-root root root)
  (call markup-append root child)
  (call (. root markup-batch-queue checkpoint))
  (print (call event-list (. child events)))
  (call markup-remove root child)
  (call (. root markup-batch-queue checkpoint))
  (print (call event-list (. child events)))
)

(type + AncestorTriggerRoot (is Box)
  (implements MarkupRootElement)
  (prop + name String)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + This _
    (param name _)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
)

(type + AncestorTriggerChild (is Box)
  (prop + events (list String) (= (call (of List String))))
  (whenever-rule (trigger ancestor AncestorTriggerRoot)
    (param changes _)
    (+= events (call ancestor-change-summary changes))
  )
)

(fn-expr + ancestor-change-summary String
  (param changes (of MarkupChangeSet AncestorTriggerRoot))
  (str "added:" added " removed:" removed)
  (where
    (lazy added (call (. "|" join) (in (. each name) (. changes added))))
    (lazy removed (call (. "|" join) (in (. each name) (. changes removed))))
  )
)

(fn-do + event-list String
  (param events (list String))
  (if (not events)
    (return "none")
  )
  (blank)
  (return (call (. ";" join) events))
)
```

```output
added:root removed:
added:root removed:;added: removed:root
```

### Keyed set refresh preserves the mounted row and tracked content; changed keys and explicit replacement retain the remove/insert lifecycle.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> KeyedRefreshRoot (id: root))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (let region (!! (. root %first-child)))
  (let first-row (as (!! (. region %first-child)) KeyedRefreshRow))
  (= (. first-row local-state) 41)
  (= (. first-row focused) true)
  (= (. first-row selected) true)
  (print (call keyed-refresh-summary root first-row))
  (blank)
  (= ([] (. root items) 0) (call KeyedRefreshItem "a" "B"))
  (call (. root markup-batch-queue checkpoint))
  (let refreshed-row (as (!! (. region %first-child)) KeyedRefreshRow))
  (print (if-expr (== refreshed-row first-row) "same-identity" "remounted"))
  (print (call keyed-refresh-summary root refreshed-row))
  (blank)
  (= ([] (. root items) 0) (call KeyedRefreshItem "b" "C"))
  (call (. root markup-batch-queue checkpoint))
  (let changed-row (as (!! (. region %first-child)) KeyedRefreshRow))
  (print (if-expr (!= changed-row first-row) "changed-identity" "reused"))
  (print (call keyed-refresh-summary root changed-row))
  (blank)
  (call (. root items replace) 0 (call KeyedRefreshItem "b" "D"))
  (call (. root markup-batch-queue checkpoint))
  (let replaced-row (as (!! (. region %first-child)) KeyedRefreshRow))
  (print (if-expr (!= replaced-row changed-row) "forced-identity" "reused"))
  (print (call keyed-refresh-summary root replaced-row))
)

(type + KeyedRefreshItem
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id _)
    (param text _)
  )
  (fn-expr + (infix this == (param other _)) _
    (== id (. other id))
  )
)

(type + KeyedRefreshRow (is Box)
  (prop + sink KeyedRefreshRoot)
  (prop + text String)
  (prop + local-state Size (= 0))
  (prop + focused _ (= false))
  (prop + selected _ (= false))
  (fn-do + mounted _
    (+= (. sink events) "M")
  )
  (fn-do + unmounted _
    (+= (. sink events) "U")
  )
)

(type + KeyedRefreshRoot (is Box)
  (implements MarkupRootElement)
  (prop + items (list KeyedRefreshItem) (= (call (of List KeyedRefreshItem) (list (call KeyedRefreshItem "a" "A")))))
  (prop + events (list String) (= (call (of List String))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (content
    (<> This
      (for (in item #items (keyed-by (. item id)))
        (<> KeyedRefreshRow
          (= sink this)
          (= text (. item text))
        )
      )
    )
  )
)

(fn-do + keyed-refresh-summary String
  (param root KeyedRefreshRoot)
  (param row KeyedRefreshRow)
  (let identity (str "text=" (. row text) " state=" (. row local-state)))
  (let interaction (str "focus=" (. row focused) " selected=" (. row selected)))
  (let events (call (. "" join) (. root events)))
  (return (str identity " " interaction " events=" events))
)
```

```output
text=A state=41 focus=true selected=true events=
same-identity
text=B state=41 focus=true selected=true events=
changed-identity
text=C state=0 focus=false selected=false events=UM
forced-identity
text=D state=0 focus=false selected=false events=UMUM
```

### Keyed refresh evaluates a property binding in its original lexical scope when the loop variable and target property share a name.

```brix
(import /core/collection)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> SameNameRefreshRoot (id: root))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (let region (!! (. root %first-child)))
  (let first-row (as (!! (. region %first-child)) SameNameRefreshRow))
  (print (. first-row item text))
  (blank)
  (= ([] (. root items) 0) (call SameNameRefreshItem "a" "after"))
  (call (. root markup-batch-queue checkpoint))
  (let refreshed-row (as (!! (. region %first-child)) SameNameRefreshRow))
  (print (== refreshed-row first-row))
  (print (. refreshed-row item text))
)

(type + SameNameRefreshItem
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id _)
    (param text _)
  )
)

(type + SameNameRefreshRow (is Box)
  (prop + item SameNameRefreshItem)
)

(type + SameNameRefreshRoot (is Box)
  (implements MarkupRootElement)
  (prop + items (list SameNameRefreshItem) (= (call (of List SameNameRefreshItem) (list (call SameNameRefreshItem "a" "before")))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (content
    (<> This
      (for (in item #items (keyed-by (. item id)))
        (<> SameNameRefreshRow
          (= item item)
        )
      )
    )
  )
)
```

```output
before
true
after
```

### Unkeyed sets remount only their coalesced range while every unchanged row keeps identity, interaction state, and live subscriptions.

```brix
(import /core/collection)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> UnkeyedRangeRoot (id: root))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (let region (!! (. root %first-child)))
  (let initial-rows (list UnkeyedRangeRow))
  (for (in index (..< 0 (. root items length)))
    (let row (as (!! (call (. region child-at) index)) UnkeyedRangeRow))
    (= (. row local-state) (+ index 10))
    (+= initial-rows row)
  )
  (blank)
  (let tail ([] initial-rows 4))
  (= (. tail local-state) 41)
  (= (. tail focused) true)
  (= (. tail selected) true)
  (call (. root events clear))
  (blank)
  (call notify-collection-set (. root items) 2 0)
  (call (. root markup-batch-queue checkpoint))
  (print (str "empty-states=" (call unkeyed-range-states region)))
  (print (str "empty-events=" (call (. "|" join) (. root events))))
  (blank)
  (= ([] (. root items) 2) (call UnkeyedRangeItem "C"))
  (= ([] (. root items) 3) (call UnkeyedRangeItem "D"))
  (= ([] (. root items) 2) (call UnkeyedRangeItem "E"))
  (call (. root markup-batch-queue checkpoint))
  (print (call unkeyed-range-texts region))
  (print (str "middle-states=" (call unkeyed-range-states region)))
  (let current-tail (as (!! (call (. region child-at) 4)) UnkeyedRangeRow))
  (print (str "tail-state=" (. current-tail local-state) ":" (. current-tail focused) ":" (. current-tail selected) ":" (. current-tail shared)))
  (print (str "middle-events=" (call (. "|" join) (. root events))))
  (blank)
  (= (. root source-shared) "after")
  (call (. root markup-batch-queue checkpoint))
  (print (str "tail-subscription=" (. current-tail shared)))
  (blank)
  (call (. root events clear))
  (= ([] (. root items) 0) (call UnkeyedRangeItem "A"))
  (call (. root markup-batch-queue checkpoint))
  (print (str "first-states=" (call unkeyed-range-states region)))
  (print (str "first-events=" (call (. "|" join) (. root events))))
  (blank)
  (call (. root events clear))
  (= ([] (. root items) 5) (call UnkeyedRangeItem "F"))
  (call (. root markup-batch-queue checkpoint))
  (print (str "last-states=" (call unkeyed-range-states region)))
  (print (str "last-events=" (call (. "|" join) (. root events))))
  (blank)
  (call (. root events clear))
  (let current-index Index 0)
  (for (in row (call unkeyed-range-rows region))
    (= (. row local-state) (+ current-index 100))
    (+= current-index 1)
  )
  (for (in index (..< 0 (. root items length)))
    (= ([] (. root items) index) (call UnkeyedRangeItem (str "W" index)))
  )
  (call (. root markup-batch-queue checkpoint))
  (print (call unkeyed-range-texts region))
  (print (str "whole-states=" (call unkeyed-range-states region)))
  (print (str "whole-events=" (call (. "|" join) (. root events))))
)

(type + UnkeyedRangeItem
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + UnkeyedRangeRow (is Box)
  (prop + sink UnkeyedRangeRoot)
  (prop + text String)
  (prop + shared String)
  (prop + local-state Size (= 0))
  (prop + focused _ (= false))
  (prop + selected _ (= false))
  (fn-do + mounted _
    (+= (. sink events) (str "M" text))
  )
  (fn-do + unmounted _
    (+= (. sink events) (str "U" text))
  )
)

(type + UnkeyedRangeRoot (is Box)
  (implements MarkupRootElement)
  (prop + items (list UnkeyedRangeItem) (:= (call unkeyed-range-items)))
  (prop + source-shared String (= "before"))
  (prop + events (list String) (= (call (of List String))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (content
    (<> This
      (for (in item #items)
        (<> UnkeyedRangeRow
          (= sink this)
          (= text (. item text))
          (= shared #source-shared)
        )
      )
    )
  )
)

(fn-do + unkeyed-range-rows (list UnkeyedRangeRow)
  (param region Box)
  (let rows (list UnkeyedRangeRow))
  (for (in child (. region children))
    (+= rows (as child UnkeyedRangeRow))
  )
  (return rows)
)

(fn-do + unkeyed-range-texts String
  (param region Box)
  (let texts (list String))
  (for (in row (call unkeyed-range-rows region))
    (+= texts (. row text))
  )
  (return (call (. "|" join) texts))
)

(fn-do + unkeyed-range-states String
  (param region Box)
  (let states (list String))
  (for (in row (call unkeyed-range-rows region))
    (+= states (str (. row local-state)))
  )
  (return (call (. "|" join) states))
)

(fn-do + unkeyed-range-items (list UnkeyedRangeItem)
  (let items (list UnkeyedRangeItem))
  (for (in text (list "a" "b" "c" "d" "e" "f"))
    (+= items (call UnkeyedRangeItem text))
  )
  (return items)
)
```

```output
empty-states=10|11|12|13|41|15
empty-events=
a|b|E|D|e|f
middle-states=10|11|0|0|41|15
tail-state=41:true:true:before
middle-events=Uc|Ud|ME|MD
tail-subscription=after
first-states=0|11|0|0|41|15
first-events=Ua|MA
last-states=0|11|0|0|41|0
last-events=Uf|MF
W0|W1|W2|W3|W4|W5
whole-states=0|0|0|0|0|0
whole-events=UA|Ub|UE|UD|Ue|UF|MW0|MW1|MW2|MW3|MW4|MW5
```

### Keyed tuple reorder keeps exact item objects and row identity.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> KeyedTupleRoot (id: root))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (let region (!! (. root %first-child)))
  (let first (!! (. region %first-child)))
  (= (. root items) (call (of List KeyedTupleItem) (list ([] (. root items) 1) ([] (. root items) 0))))
  (call (. root markup-batch-queue checkpoint))
  (print (== (!! (call (. region child-at) 1)) first))
)

(type + KeyedTupleItem (is Tuple)
  (prop + id String)
)

(type + KeyedTupleRoot (is Box)
  (implements MarkupRootElement)
  (prop + items (list KeyedTupleItem) (= (call (of List KeyedTupleItem) (list (call KeyedTupleItem "a") (call KeyedTupleItem "b")))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (content
    (<> This
      (for (in item #items (keyed-by (. item id)))
        (<> TextBox
          (= text (. item id))
        )
      )
    )
  )
)
```

```output
true
```

### Markup-owned actions defer to their root checkpoint, deduplicate repeated writes, preserve first-dirtied order across reentrant passes, and cancel pending work on disposal.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> OwnerActionBatchRoot (id: root))
  (let child (call OwnerActionBatchChild))
  (call markup-adopt child null root)
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (call (. child events clear))
  (= (. child source) 1)
  (= (. child source) 2)
  (print (. child events length))
  (call (. root markup-batch-queue checkpoint))
  (print (call (. "|" join) (. child events)))
  (call (. child events clear))
  (= (. child source) 3)
  (call (. root markup-batch-queue checkpoint))
  (print (call (. "|" join) (. child events)))
  (call (. child events clear))
  (= (. child source) 4)
  (call (. root markup-batch-queue checkpoint))
  (print (call (. "|" join) (. child events)))
  (call (. child events clear))
  (= (. child source) 5)
  (call (. child close))
  (call (. root markup-batch-queue checkpoint))
  (print (. child events length))
  (let plain (call PlainActionOwner))
  (= (. plain source) 7)
  (print (. plain observed))
)

(type + OwnerActionBatchRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + This _
    (call markup-mount-root this this)
  )
)

(type + OwnerActionBatchChild (is Box)
  (prop + source Size (= 0))
  (prop + mirror Size (= 0))
  (prop + events (list String) (= (call (of List String))))
  (whenever-rule #source
    (+= events (str "source-first:" source))
    (= mirror source)
  )
  (whenever-rule #source
    (+= events (str "source-second:" source))
  )
  (whenever-rule #mirror
    (+= events (str "mirror:" mirror))
  )
)

(type + PlainActionOwner
  (prop + source Size (= 0))
  (prop + observed Size (= 0))
  (when-rule #source
    (= observed source)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)
```

```output
0
source-first:2|source-second:2|mirror:2
source-first:3|source-second:3|mirror:3
source-first:4|source-second:4|mirror:4
0
7
```

### Released markup holds its bindings until it is mounted again, and then catches up.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> HeldRoot (id: root))
  (let panel (call HeldPanel))
  (call markup-adopt panel null root)
  (call (. root markup-batch-queue checkpoint))
  (print (call text panel))
  (call markup-release panel)
  (= (. panel names) (call (of List String)))
  (print (call text panel))
  (= (. panel names) (call (of List String) "b"))
  (print (call text panel))
  (call markup-adopt panel null root)
  (call (. root markup-batch-queue checkpoint))
  (print (call text panel))
)

(fn-expr + text String
  (param panel HeldPanel)
  (. (as (!! (. panel %first-child)) TextBox) text)
)

(type + HeldRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + HeldPanel (is Box)
  (prop + names _ (= (call (of List String) "a")))
  (content
    (<> This
      (<> TextBox
        (= text ([] #names 0))
      )
    )
  )
)
```

```output
a
a
a
b
```

### Nested checkpoints remain ordered, late subscriptions begin with the next dispatch, and pass caps preserve reentrant work for a later checkpoint.

```brix
(import /core/markup)
(import /core/tracker)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let events (call (of List String)))
  (let inner (call NestedActionInner events))
  (let outer (call NestedActionOuter inner events))
  (call markup-mount-root inner inner)
  (call markup-mount-root outer outer)
  (call (. inner markup-batch-queue checkpoint))
  (call (. outer markup-batch-queue checkpoint))
  (call (. events clear))
  (= (. outer source) 1)
  (call (. outer markup-batch-queue checkpoint))
  (print (call (. "|" join) events))
  (blank)
  (let subscribed (call SubscribeDuringBatchRoot))
  (call markup-mount-root subscribed subscribed)
  (call (. subscribed markup-batch-queue checkpoint))
  (= (. subscribed early-runs) 0)
  (= (. subscribed late-runs) 0)
  (= (. subscribed armed) true)
  (= (. subscribed source) 1)
  (call (. subscribed markup-batch-queue checkpoint))
  (print (str (. subscribed early-runs) ":" (. subscribed late-runs)))
  (= (. subscribed source) 2)
  (print (str (. subscribed early-runs) ":" (. subscribed late-runs)))
  (call (. subscribed markup-batch-queue checkpoint))
  (blank)
  (let cycling (call CappedBatchRoot))
  (call markup-mount-root cycling cycling)
  (call (. cycling markup-batch-queue checkpoint))
  (= (. cycling runs) 0)
  (= (. cycling cycling) true)
  (= (. cycling source) 1)
  (call (. cycling markup-batch-queue checkpoint))
  (print (. cycling runs))
  (print (. cycling markup-batch-queue actions length))
  (= (. cycling cycling) false)
)

(type + NestedActionInner (is Box)
  (implements MarkupRootElement)
  (prop + source Size (= 0))
  (prop + events (list String))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + This _
    (param events _)
  )
  (when-rule #source
    (+= events (str "inner:" source))
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + NestedActionOuter (is Box)
  (implements MarkupRootElement)
  (prop + source Size (= 0))
  (prop + inner NestedActionInner)
  (prop + events (list String))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + This _
    (param inner _)
    (param events _)
  )
  (when-rule #source
    (+= events (str "outer-before:" source))
    (= (. inner source) source)
    (call (. inner markup-batch-queue checkpoint))
    (+= events (str "outer-after:" source))
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + SubscribeDuringBatchRoot (is Box)
  (implements MarkupRootElement)
  (prop + source Size (= 0))
  (prop + early-runs Size (= 0))
  (prop + late-runs Size (= 0))
  (prop + armed _ (= false))
  (prop + late-action (? LiveAction))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (when-rule #source
    (+= early-runs 1)
    (if armed
      (if (not late-action)
        (= late-action (call property-dependency-subscribe this "source" (. this run-late)))
      )
    )
  )
  (fn-do + run-late _
    (+= late-runs 1)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + CappedBatchRoot (is Box)
  (implements MarkupRootElement)
  (prop + source Size (= 0))
  (prop + mirror Size (= 0))
  (prop + runs Size (= 0))
  (prop + cycling _ (= false))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + This _
    (= (. markup-batch-queue max-passes-per-checkpoint) 2)
  )
  (when-rule #source
    (+= runs 1)
    (if cycling
      (= mirror (+ source 1))
    )
  )
  (when-rule #mirror
    (+= runs 1)
    (if cycling
      (= source (+ mirror 1))
    )
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)
```

```output
outer-before:1|inner:1|outer-after:1
1:0
1:1
2
1
```

### Rebinding publishes address, key, and item as one settled batch, with generation observed last.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let values (list String) (call (of List String)))
  (let generations (list String) (call (of List String)))
  (<> (of RebindPanel RowData) (id: panel)
    (= items (call (of List RowData) (list (call RowData "a"))))
    (<> Box (slot: row) (param: slot)
      (<> TextBox (value: (call observe-values values (. slot #address) (. slot #key) (. slot #item))))
      (<> TextBox (value: (call observe-generation generations (. slot #generation) slot)))
    )
  )
  (blank)
  (print (call (. "|" join) values))
  (print (call (. "|" join) generations))
  (blank)
  (call (. panel rebind) 0 7 70 (call RowData "b"))
  (call (. panel markup-batch-queue checkpoint))
  (blank)
  (print (call (. "|" join) values))
  (print (call (. "|" join) generations))
  (print (. panel flush-count))
)

(type + RowData
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(fn-do + observe-values String
  (param events (list String))
  (param address I64)
  (param key I64)
  (param item RowData)
  (let value (str address ":" key ":" (. item text)))
  (+= events value)
  (return value)
)

(fn-do + observe-generation String
  (param events (list String))
  (param generation U64)
  (param scope (of RebindRowScope RowData))
  (let value (str generation ":" (. scope address) ":" (. scope key) ":" (. scope item text)))
  (+= events value)
  (return value)
)
```

```output
0:0:a
0:0:0:a
0:0:a|7:70:b
0:0:0:a|1:7:70:b
1
```

### A mounted host's derived property is fresh when read before the checkpoint; its property handlers, of the derived property too, run as each write lands, and nothing runs after close

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> OwnRoot (id: root)
    (<> Meter (id: meter))
  )
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (print (str "start " (. meter double) " " (call (. "|" join) (. meter log))))
  (blank)
  (= (. meter level) 5)
  (= (. meter level) 6)
  (print (str "pending " (. meter double) " " (call (. "|" join) (. meter log))))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (print (str "settled " (. meter double) " " (call (. "|" join) (. meter log))))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (print (str "idle " (. meter double) " " (call (. "|" join) (. meter log))))
  (blank)
  (call (. meter close))
  (= (. meter level) 9)
  (call (. root markup-batch-queue checkpoint))
  (print (str "closed " (. meter double) " " (call (. "|" join) (. meter log))))
)

(type + OwnRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
)

(type + Meter (is Box)
  (prop + log (list String) (= (call (of List String))))
  (prop + level _ (= 1))
  (prop + double _ (= (* #level 2)))
  (blank)
  (depends level-changed level)
  (depends double-changed double)
  (fn-do + level-changed _
    (+= log (str "level:" level))
  )
  (fn-do + double-changed _
    (+= log (str "double:" double))
  )
)
```

```output
start 2 
pending 12 double:10|level:5|double:12|level:6
settled 12 double:10|level:5|double:12|level:6
idle 12 double:10|level:5|double:12|level:6
closed 12 double:10|level:5|double:12|level:6
```

### A host that moves to another batch root with a deferred own reaction settles in its new root, and the entry left behind in the old root does not run it again

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let first (call OwnRoot))
  (let second (call OwnRoot))
  (call markup-mount-root first first)
  (call markup-mount-root second second)
  (let meter (call Meter))
  (call markup-adopt meter null first)
  (call (. first markup-batch-queue checkpoint))
  (call (. second markup-batch-queue checkpoint))
  (blank)
  (= (. meter level) 5)
  (call markup-release meter)
  (call markup-adopt meter null second)
  (= (. meter level) 6)
  (print (str "moved " (call (. "|" join) (. meter log))))
  (blank)
  (call (. second markup-batch-queue checkpoint))
  (print (str "second " (call (. "|" join) (. meter log))))
  (blank)
  (call (. first markup-batch-queue checkpoint))
  (print (str "first " (call (. "|" join) (. meter log))))
)

(type + OwnRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
)

(type + Meter (is Box)
  (prop + log (list String) (= (call (of List String))))
  (prop + level _ (= 1))
  (whenever-rule #level
    (+= log (str "level:" level))
  )
)
```

```output
moved 
second level:6
first level:6
```

### A batched branch tears down a nested loop before that loop's source reruns, whether the branch built the loop at mount or the loop's own dependency changed first.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let guarded (call Guarded (call Item "a")))
  (call (. guarded markup-batch-queue checkpoint))
  (print (call label guarded))
  (= (. guarded focus) null)
  (call (. guarded markup-batch-queue checkpoint))
  (print (call label guarded))
  (blank)
  (let shown (call Shown (call Item "b")))
  (call (. shown markup-batch-queue checkpoint))
  (print (call label shown))
  (= (. shown focus) null)
  (= (. shown shows) false)
  (call (. shown markup-batch-queue checkpoint))
  (print (call label shown))
)

(type + Item
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
)

(type + Owner (is Box)
  (implements MarkupRootElement)
  (prop + focus (? Item))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + This _
    (param focus (? Item))
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
)

(type + Guarded (is Owner)
  (fn-do + This _
    (param focus (? Item))
    (call super focus)
  )
  (content
    (<> This
      (<> Box
        (if (!= #focus null)
          (for (in item (list (!! #focus)) (keyed-by (. item name)))
            (<> TextBox (value: (. item name)))
          )
        )
      )
    )
  )
)

(type + Shown (is Owner)
  (prop + shows _ (= true))
  (fn-do + This _
    (param focus (? Item))
    (call super focus)
  )
  (content
    (<> This
      (<> Box
        (if #shows
          (for (in item (list (!! #focus)) (keyed-by (. item name)))
            (<> TextBox (value: (. item name)))
          )
        )
      )
    )
  )
)

(fn-do + label String
  (param node Box)
  (let texts (list String))
  (call collect node texts)
  (return (str "[" (call (. "," join) texts) "]"))
)

(fn-do + collect _
  (param node Box)
  (param texts (list String))
  (for (in child (. node children))
    (if (is child TextBox)
      (+= texts (. (as child TextBox) text))
    )
    (call collect child texts)
  )
)
```

```output
[a]
[]
[b]
[]
```

### A lazy declaration in the markup of a class whose value reads something tracked is evaluated again through the batch of the element whose content first reads it.

```brix
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let done (call Semaphore))
  (blank)
  (<> Root (id: root)
    (<> Host (id: h))
  )
  (blank)
  (print (call (. h shown)))
  (= (. h box text) "b")
  (print "set")
  (print (call (. h shown)))
  (do-new
    (print (call (. h shown)))
    (call (. done post))
  )
  (call (. done wait))
)

(fn-do + noted String
  (param text String)
  (print (str "copy " text))
  (return text)
)

(type + Root (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (print "flushed")
  )
)

(type + Host (is Box)
  (fn-expr + shown String
    (str (. host-label text))
  )
  (content
    (<> Label (id: box) (text: "a"))
    (blank)
    (<> Label (id: host-label)
      (str "got " #copy)
    )
    (blank)
    (lazy copy (call noted (. box #text)))
  )
)
```

```output
copy a
got a
set
got a
copy b
flushed
got b
```

### A rule a branch tests and a property inside the branch both follow one value. When the value clears, the branch closes before the property updates, so the property never reads the cleared value.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let root (call Root))
  (call markup-mount-root root root)
  (call (. root markup-batch-queue checkpoint))
  (print (call dump root))
  (= (. root source) 0)
  (call (. root markup-batch-queue checkpoint))
  (print (call dump root))
  (= (. root source) 2)
  (call (. root markup-batch-queue checkpoint))
  (print (call dump root))
)

(fn-do + dump String
  (param root Root)
  (let region (!! (. root %first-child)))
  (let count (call count-children region))
  (if (== count 0)
    (return "0")
  )
  (return (str count ":" (. (as (!! (. region %first-child)) TextBox) text)))
)

(fn-do + count-children Size
  (param box Box)
  (let count Size 0)
  (for (in child (. box children))
    (+= count 1)
  )
  (return count)
)

(type + Counts (is Tuple)
  (prop + count Int)
)

(type + Root (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + source _ (= 1))
  (prop + value _ (= (only-if (call Counts #source) (if (> #source 0)))))
  (prop + has-value _ (= (> (?? (?. #value count) 0) 0)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (content
    (<> This
      (if #has-value
        (<> TextBox
          (= text (str "+" (. (!! #value) count)))
        )
      )
    )
  )
)
```

```output
1:+1
0
1:+2
```

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> RegionTriggerRoot (id: root))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (let region (call RegionTriggerBlock))
  (call markup-append root region)
  (call (. root markup-batch-queue checkpoint))
  (print (call (. "|" join) (. root events)))
  (let item (call RegionTriggerItem))
  (call markup-append region item)
  (call (. root markup-batch-queue checkpoint))
  (print (call (. "|" join) (. root events)))
  (call markup-remove region item)
  (call (. root markup-batch-queue checkpoint))
  (print (call (. "|" join) (. root events)))
)

(type + RegionTriggerItem (is Box))

(type + RegionTriggerBlock (is Box)
  (fn-expr + markup-is-transparent-region _ :get
    true
  )
)

(type + RegionTriggerRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (prop + events (list String) (= (call (of List String))))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (whenever-rule (trigger children RegionTriggerItem)
    (+= events "children")
  )
)
```

```output
children
children|children
```

### A deferred action runs at the checkpoint with the cause that was current when its write landed.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)
(prop + scoped _ (= (call MarkupCause)))
(blank)

(main
  (<> OwnRoot (id: root)
    (<> Meter (id: meter))
  )
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (= (. meter level) 2)
  (call (. root markup-batch-queue checkpoint))
  (let outer (call swap-markup-cause scoped))
  (= (. meter level) 3)
  (call swap-markup-cause outer)
  (call (. root markup-batch-queue checkpoint))
  (let quiet (call swap-markup-cause markup-cause-none))
  (= (. meter level) 4)
  (call swap-markup-cause quiet)
  (call (. root markup-batch-queue checkpoint))
  (print (call (. "|" join) (. meter log)))
  (print (call cause-name (call current-markup-cause)))
)

(fn-do + cause-name String
  (param cause MarkupCause)
  (return-if "scoped" (if (== (as cause Pointer) (as scoped Pointer))))
  (return-if "none" (if (== (as cause Pointer) (as markup-cause-none Pointer))))
  (return-if "write" (if (== (as cause Pointer) (as markup-cause-write Pointer))))
  (return "other")
)

(type + OwnRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
)

(type + Meter (is Box)
  (prop + log (list String) (= (call (of List String))))
  (prop + level _ (= 1))
  (prop + double _ (= (* #level 2)))
  (blank)
  (depends double-changed double)
  (fn-do + double-changed _
    (+= log (str double ":" (call cause-name (call current-markup-cause))))
  )
)
```

```output
4:write|6:scoped|8:none
write
```

### A derived property of a markup host is fresh when read before its batch runs, and its rule sees one consistent value.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> MemoRoot (id: root))
  (let child (call MemoChild))
  (call markup-adopt child null root)
  (call (. root markup-batch-queue checkpoint))
  (call (. child seen clear))
  (= (. child a) 2)
  (print (str "total=" (. child total) " b=" (. child b)))
  (call (. root markup-batch-queue checkpoint))
  (print (call (. "|" join) (. child seen)))
)

(type + MemoRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + MemoChild (is Box)
  (prop + a _ (= 1))
  (prop + total _ (= (+ #a #b)))
  (prop + b _ (= (* #a 10)))
  (prop + seen (list String) (= (call (of List String))))
  (whenever-rule #total
    (+= seen (str "total:" total))
  )
)
```

```output
total=22 b=20
total:22
```

### A rule on a derived property runs at the checkpoint with the cause that was current when the write it reads landed.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)
(prop + scoped _ (= (call MarkupCause)))
(blank)

(main
  (<> OwnRoot (id: root)
    (<> Meter (id: meter))
  )
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (= (. meter level) 2)
  (call (. root markup-batch-queue checkpoint))
  (let outer (call swap-markup-cause scoped))
  (= (. meter level) 3)
  (call swap-markup-cause outer)
  (call (. root markup-batch-queue checkpoint))
  (let quiet (call swap-markup-cause markup-cause-none))
  (= (. meter level) 4)
  (call swap-markup-cause quiet)
  (call (. root markup-batch-queue checkpoint))
  (print (call (. "|" join) (. meter log)))
  (print (call cause-name (call current-markup-cause)))
)

(fn-do + cause-name String
  (param cause MarkupCause)
  (return-if "scoped" (if (== (as cause Pointer) (as scoped Pointer))))
  (return-if "none" (if (== (as cause Pointer) (as markup-cause-none Pointer))))
  (return-if "write" (if (== (as cause Pointer) (as markup-cause-write Pointer))))
  (return "other")
)

(type + OwnRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
)

(type + Meter (is Box)
  (prop + log (list String) (= (call (of List String))))
  (prop + level _ (= 1))
  (prop + double _ (= (* #level 2)))
  (whenever-rule #double
    (+= log (str double ":" (call cause-name (call current-markup-cause))))
  )
)
```

```output
4:write|6:scoped|8:none
write
```

### A binding gives the element the place's value as it is made and follows the place, and stores what the element holds back in the place as it changes: under a batch root, at the checkpoint, as the batch runs every action it holds.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BindRoot (id: root))
  (let panel (call BindPanel))
  (call markup-adopt panel null root)
  (print (call state panel "adopted"))
  (call (. root markup-batch-queue checkpoint))
  (print (call state panel "checkpoint"))
  (call (. panel knob turn) 3)
  (print (call state panel "turned 3"))
  (call (. root markup-batch-queue checkpoint))
  (print (call state panel "checkpoint"))
  (= (. panel level) 2)
  (print (call state panel "model 2"))
  (call (. root markup-batch-queue checkpoint))
  (print (call state panel "checkpoint"))
)

(fn-do + state String
  (param panel BindPanel)
  (param step String)
  (return (str step ": level " (. panel level) " knob " (. panel knob value)))
)

(type + BindRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Knob (is Box)
  (prop + value _ (= 0))
  (fn-do + turn _
    (param next Int)
    (= value next)
  )
)

(type + BindPanel (is Box)
  (prop + level _ (= 5))
  (content
    (<> This
      (<> Knob (id: knob)
        (<-> value level)
      )
    )
  )
)
```

```output
adopted: level 5 knob 5
checkpoint: level 5 knob 5
turned 3: level 5 knob 3
checkpoint: level 3 knob 3
model 2: level 2 knob 3
checkpoint: level 2 knob 2
```
