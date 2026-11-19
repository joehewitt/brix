# Markup

```brix
(main
  (let direct (call MarkupDefaults))
  (let inherited (call InheritedDefaults))
  (let overridden (call OverriddenDefaults))
  (let configured (call ConfiguredDefaults 900))
  (let fallback (call ConfiguredDefaults null))
  (let markup (call OverriddenMarkupDefaults))
  (print (. direct width))
  (print (. inherited width))
  (print (. overridden width))
  (print (. configured width))
  (print (. fallback width))
  (print (. markup width))
)

(type + DefaultsBase
  (prop + width _ (= 640))
)

(type + MarkupDefaults (is DefaultsBase)
  (content
    (<> This
      (= width 1120)
    )
  )
)

(type + InheritedDefaults (is MarkupDefaults))

(type + OverriddenDefaults (is MarkupDefaults)
  (prop + width _ (= 800))
)

(type + ConfiguredDefaults (is MarkupDefaults)
  (fn-do + This _
    (param width (? I64))
  )
)

(type + OverriddenMarkupDefaults (is MarkupDefaults)
  (content
    (<> This
      (= width 720)
    )
  )
)
```

```output
1120
1120
800
900
1120
720
```

```brix
(import ./defaults)
(blank)

(main
  (let value (call Derived))
  (print (. value width))
)

(type + Derived (is MarkupDefaults)
  (prop + multiplier _ (= 3))
)
```

```brix file=defaults.bx
(type + DefaultsBase
  (prop + width _ (= 640))
)

(type + MarkupDefaults (is DefaultsBase)
  (prop + multiplier _ (= 2))
  (content
    (<> This
      (= width (* (call preferred-width) multiplier))
    )
  )
)

(fn-expr _ preferred-width _
  400
)
```

```output
1200
```

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let root (call IdentityQueryRoot))
  (call markup-mount-root root root)
  (let owner (call EqualQueryHost))
  (let first (call EqualQueryHost))
  (let middle (call DifferentQueryHost))
  (let last (call EqualQueryHost))
  (call markup-append root owner)
  (call markup-append root (call DifferentQueryHost))
  (call markup-append owner first)
  (call markup-append owner middle)
  (call markup-append owner last)
  (call (. root markup-batch-queue checkpoint))
  (blank)
  (print (== (as (call markup-query-first-descendant-host owner) Pointer) (as first Pointer)))
  (print (== (as (call markup-query-next-descendant-host owner first) Pointer) (as middle Pointer)))
  (print (== (as (call markup-query-next-descendant-host owner middle) Pointer) (as last Pointer)))
  (print (not (call markup-query-next-descendant-host owner last)))
)

(type + EqualQueryHost (is Box)
  (fn-expr + (infix this == (param other EqualQueryHost)) _
    true
  )
)

(type + DifferentQueryHost (is Box))

(type + IdentityQueryRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
)
```

```output
true
true
true
true
```

```brix
(main
  (let x (call foo))
  (print (. x a))
  (print (. x contents))
)

(fn-do + foo _
  (return
    (<> Foo
      (= a 10)
      (<> Foo)
    )
  )
)

(type + Foo
  (prop + a _ (= 0))
  (prop + contents (? Foo))
  (fn-do + (infix this += (param child Foo)) _
    (= contents child)
  )
)
```

```output
10
[Object __source__:Foo]
```

### Descendant queries see targets declared in a module that has no markup syntax of its own.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import ./lib/marker)
(blank)

(main
  (<> MarkerRoot (id: root))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (print (. root markers length))
)

(type + MarkerRoot (is Box)
  (implements MarkupRootElement)
  (prop + markers (of Descendants Marker))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (content
    (<> This
      (<> Marker)
      (<> Box
        (<> Marker)
      )
    )
  )
)
```

```brix file=lib/marker.bx
(import /core/tests/support/markup-fixtures)

(type + Marker (is Box))
```

```output
2
```

### Descendant queries stop at an explicit boundary when their owner is outside it, while a query owned by the boundary continues to observe its own structural subtree across target toggles.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BoundaryQueryRoot (id: root))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (print (. root markers length))
  (print (call boundary-marker-list (. root boundary markers)))
  (blank)
  (= (. root boundary show-extra) true)
  (call (. root markup-batch-queue checkpoint))
  (print (. root markers length))
  (print (call boundary-marker-list (. root boundary markers)))
  (blank)
  (= (. root boundary show-extra) false)
  (call (. root markup-batch-queue checkpoint))
  (print (. root markers length))
  (print (call boundary-marker-list (. root boundary markers)))
)

(type + BoundaryMarker (is Box)
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + BoundaryQueryHost (is Box)
  (prop + show-extra Bool (= false))
  (prop + markers (of Descendants BoundaryMarker))
  (fn-expr + markup-query-boundary Bool :get
    true
  )
  (content
    (<> This
      (<> BoundaryMarker (value: "steady"))
      (if #show-extra
        (<> BoundaryMarker (value: "extra"))
      )
    )
  )
)

(type + BoundaryQueryRoot (is Box)
  (implements MarkupRootElement)
  (prop + boundary BoundaryQueryHost)
  (prop + markers (of Descendants BoundaryMarker))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (content
    (<> This
      (<> BoundaryQueryHost (id: boundary))
    )
  )
)

(fn-do + boundary-marker-list String
  (param markers (list BoundaryMarker))
  (return (call (. "|" join) (in (. each text) markers)))
)
```

```output
0
steady
0
steady|extra
0
steady
```

### A live owner keeps one markup attachment across ordinary unmount/remount. Dropping the mounted, parentless owner lets reclamation clear lane 3, and deterministic cleanup drain completes the detached cleanup token without firing a visible unmounted callback.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import /BxGC/BxGC.h)
(blank)

(main
  (let lane (call C:brixGCMarkupAttachmentsSidePayloadLane))
  (let root (call MarkupCleanupRoot))
  (call markup-mount-root root root)
  (call (. root markup-batch-queue checkpoint))
  (let opening (call C:brixGCObjectSidePayloadCount lane))
  (blank)
  (do
    (inline (call exercise-markup-owner-lifetime root lane opening))
  )
  (call C:brixCollect)
  (do
    (inline (call C:brixGCDrainCleanups))
  )
  (blank)
  (print (str "reclaimed=" (- (call C:brixGCObjectSidePayloadCount lane) opening)))
)

(fn-do + exercise-markup-owner-lifetime _
  (param root MarkupCleanupRoot)
  (param lane U64)
  (param opening U64)
  (lazy child (? MarkupCleanupChild) (call MarkupCleanupChild))
  (blank)
  (call markup-adopt (!! child) null root)
  (call (. root markup-batch-queue checkpoint))
  (print (str "mounted=" (. (!! child) events) " lane=" (- (call C:brixGCObjectSidePayloadCount lane) opening)))
  (blank)
  (call markup-release (!! child))
  (call (. root markup-batch-queue checkpoint))
  (print (str "unmounted=" (. (!! child) events) " lane=" (- (call C:brixGCObjectSidePayloadCount lane) opening)))
  (blank)
  (call markup-adopt (!! child) null root)
  (call (. root markup-batch-queue checkpoint))
  (print (str "remounted=" (. (!! child) events) " lane=" (- (call C:brixGCObjectSidePayloadCount lane) opening)))
  (blank)
  (= child null)
)

(type + MarkupCleanupRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + MarkupCleanupChild (is Box)
  (prop + events String (= ""))
  (fn-do + mounted _
    (+= events "M")
  )
  (fn-do + unmounted _
    (+= events "U")
  )
)
```

```output
mounted=M lane=1
unmounted=MU lane=1
remounted=MUM lane=1
reclaimed=0
```

```brix crash
(main
  (<> Foo
    "bar"
  )
)

(type + Foo
  (fn-do + (infix this += (param child Foo)) _)
)
```

```output
Exception: Content not accepted
<no file:4>
        "bar"
        ^^^^^
```

```brix
(main
  (let foo
    (block
      (<> Foo (value: "bar"))
    )
  )
  (blank)
  (print foo)
)

(type + Foo
  (prop + content String)
  (fn-do + This _
    (param content _)
  )
  (fn-to-expr + String
    (str "[Foo " content "]")
  )
)
```

```output
[Foo bar]
```

```brix crash
(main
  (<> Foo
    (= b 10)
  )
)

(type + Foo
  (prop + a _ (= 0))
)
```

```output
Exception: Property "b" not found; an assignment in an element's body sets the element's property. To assign a local, put it in a "(do …)": "(do (= b …))"
<no file:4>
        (= b 10)
           ^
```

```brix crash
(main
  (<> Foo (text: "")
    (= text "x")
  )
)

(type + Foo
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)
```

```output
Exception: Parameter "text" is given twice, by the element's header and by this attribute; give it once
<no file:4>
        (= text "x")
        ^^^^^^^^^^^^
```

### An attribute that fills a constructor parameter an earlier attribute filled.

```brix crash
(main
  (<> Foo
    (= text "x")
    (= text "y")
  )
)

(type + Foo
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)
```

```output
Exception: Parameter "text" is given twice, by an attribute above and by this one; remove one
<no file:5>
        (= text "y")
        ^^^^^^^^^^^^
```

```brix
(main
  (<> Foo (id: foo))
  (print foo)
)

(type + Foo)
```

```output
[Object __source__:Foo]
```

```brix
(main
  (<> Foo (id: foo)
    (print (str "inner " foo))
    (<> Bar (id: bar)
      (print (str "inner " bar))
    )
  )
  (print foo)
  (print bar)
)

(type + Foo
  (fn-do + (infix this += (param child _)) _)
)

(type + Bar)
```

```output
inner [Object __source__:Foo]
inner [Object __source__:Bar]
[Object __source__:Foo]
[Object __source__:Bar]
```

### Names can be referenced out of order due to lazy declarations

```brix
(main
  (<> Foo
    (print foo)
    (<> Foo (id: foo))
  )
)

(type + Foo
  (fn-do + (infix this += (param child _)) _)
)
```

```output
[Object __source__:Foo]
```

```brix
(main
  (<> Window (id: win)
    (for (in text (list "foo" "bar" "baz"))
      (<> Label (value: text))
    )
  )
  (blank)
  (print (. win contents))
)

(type + Window
  (prop + contents (list Label))
  (fn-do + (infix this += (param child Label)) _
    (+= contents child)
  )
)

(type + Label
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
  (fn-to-expr + String
    (str "[Label " text "]")
  )
)
```

```output
[Label foo], [Label bar], [Label baz]
```

```brix
(main
  (<> Window (id: win)
    (if true
      (<> Label (value: "a"))
    )
    (if false
      (<> Label (value: "b"))
      (else-if true
        (<> Label (value: "c"))
      )
    )
  )
  (blank)
  (print (. win contents))
)

(type + Window
  (prop + contents (list Label))
  (fn-do + (infix this += (param child Label)) _
    (+= contents child)
  )
)

(type + Label
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
  (fn-to-expr + String
    (str "[Label " text "]")
  )
)
```

```output
[Label a], [Label c]
```

```brix
(main
  (print (call test))
)

(fn-do + test _
  (return (<> Window))
)

(type + Window)
```

```output
[Object __source__:Window]
```

```brix
(main
  (<> Window
    (print "a")
    (finally
      (print "b")
    )
    (print "c")
  )
)

(type + Window)
```

```output
a
c
b
```

```brix
(main
  (<> Window (id: window)
    (= a b)
  )
  (lazy b 42)
  (blank)
  (print (. window a))
)

(type + Window
  (prop + a _ (= 0))
)
```

```output
42
```

### Lazy variables accessible throughout the subtree (and not reevaluated obviously)

```brix
(main
  (<> Foo (id: foo)
    (lazy b (call bar))
    (<> Foo
      (= a (+ b 1))
    )
  )
  (blank)
  (print (?. (. foo child) a))
)

(type + Foo
  (prop + a _ (= 0))
  (prop + child (? Foo))
  (fn-do + (infix this += (param foo _)) _
    (= child foo)
  )
)

(fn-do + bar _
  (print "called")
  (return 42)
)
```

```output
called
43
```

```brix
(main
  (<> Window
    (call foo)
  )
)

(type + Window)

(fn-do + foo _
  (print "foo")
)
```

```output
foo
```

```brix
(main
  (do-new
    (<> Window
      (call foo)
    )
    (on-failure failure
      (print failure)
    )
  )
)

(type + Window)

(fn-do + foo _
  (throw (call Panic "fail"))
)
```

```output
fail
```

```brix crash
(main
  (print (call test))
)

(fn-expr + test _
  (<> Window
    (<> Window
      (<> Window
        (return 42)
      )
    )
  )
)

(type + Window
  (fn-do + (infix this += (param child _)) _)
)
```

```output
Exception: Function "test" is not declared with fn-do, so its body is one expression and cannot return with "(return …)"; write "(fn-do + test _ …)"
<no file:10>
            (return 42)
            ^^^^^^^^^^^
```

```brix
(main
  (<> Window
    (<> Window)
    (do
      (<> Window)
    )
  )
)

(type + Window
  (fn-do + (infix this += (param child _)) _
    (print child)
  )
)
```

```output
[Object __source__:Window]
```

```brix
(main
  (print
    (<> (of List Int)
      1
      2
      3
    )
  )
)
```

```output
1, 2, 3
```

```brix
(main
  (print
    (<> (of Map String Int)
      (tuple "a" 1)
      (tuple "b" 2)
      (tuple "c" 3)
    )
  )
)
```

```output
[b = 2, a = 1, c = 3]
```

### Just verifying that closures work inside markup

```brix
(main
  (let clicks 0)
  (blank)
  (<> Foo (id: foo)
    (for (on event click)
      (+= clicks 1)
    )
  )
  (blank)
  (<- (. foo click) 1)
  (print clicks)
)

(type + Foo
  (prop + click (of AsyncChannel Int))
)
```

```output
0
```

### Class content

```brix
(main
  (<> Window (id: win))
  (blank)
  (print (. win child))
)

(type + Window (is Box)
  (content
    (<> Box (id: box))
  )
)

(type + Box
  (prop + child (? Box))
  (fn-do + (infix this += (param box Box)) _
    (= child box)
  )
)
```

```output
[Object __source__:Box]
```

### Class-level subscriptions can reference mounted class-content ids

```brix
(main
  (let exported (call ExportedIdHost))
  (<- (. exported target emits) 1)
  (call yield-once)
  (call yield-once)
  (print (str "exported=" (. exported count)))
  (blank)
  (let hidden (call PrivateIdHost))
  (<- (. hidden child emits) 1)
  (call yield-once)
  (call yield-once)
  (print (str "private=" (. hidden count)))
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)

(type + EventNode
  (prop + emits (of AsyncChannel Int))
)

(type + Box
  (prop + child _ (:= (call EventNode)))
  (fn-do + (infix this += (param child EventNode)) _
    (= (. this child) child)
    (return child)
  )
)

(type + ExportedIdHost (is Box)
  (prop + count _ (= 0))
  (prop + target _ (:= (call EventNode)))
  (content
    (<> EventNode (id: target))
    (blank)
    (for (on n (. target emits))
      (+= count n)
    )
  )
)

(type + PrivateIdHost (is Box)
  (prop + count _ (= 0))
  (content
    (<> EventNode (id: node))
    (blank)
    (for (on n (. node emits))
      (+= count n)
    )
  )
)
```

```output
exported=1
private=1
```

### Closure binds to content

```brix
(main
  (<> Window)
)

(type + Window (is Box)
  (content
    (<> Box
      (<> Box (id: b)
        (= cb (=> () (call foo)))
        (blank)
        (print (call (!! (. b cb))))
      )
    )
  )
  (fn-do + This _
    ; foo()
  )
  (fn-do + foo _
    (print "foo")
    (print child)
    (return 42)
  )
)

(type + Box
  (prop + child (? Box))
  (prop + cb (? (of Getter Int)))
  (fn-do + (infix this += (param box Box)) _
    (= child box)
  )
)
```

```output
foo
unknown
42
```

### Class content

```brix
(main
  (<> Window)
)

(type + Window (is Box)
  (content
    (<> Box
      (<> Box
        (= b (call submit))
        (print (call submit))
      )
    )
  )
  (fn-do + This _
    ; foo()
  )
  (fn-do + submit _
    (print (tuple "submit" child))
    (blank)
    (return 42)
  )
)

(type + Box
  (prop + child (? Box))
  (prop + b Int)
  (fn-do + (infix this += (param box Box)) _
    (= child box)
  )
)
```

```output
(submit, unknown)
(submit, unknown)
42
```

```brix
(main
  (<> Foo
    (= bar (call Bar "Joe"))
  )
)

(type + Foo
  (prop + bar Bar)
)

(type + Bar
  (prop + name String)
  (fn-do + This _
    (print "create Bar")
  )
  (fn-do + This _
    (param name _)
    (print (str "create Bar " name))
  )
)
```

```output
create Bar Joe
```

```brix
(main
  (<> Foo (id: foo)
    (= b (call Bar "Joe"))
  )
  (blank)
  (print (. foo a))
)

(type + Foo
  (prop + a _ (= (str (. b name) " Blow")))
  (prop + b Bar)
)

(type + Bar
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
)
```

```output
Joe Blow
```

### A named element of top-level markup is made where it is written, so it reads the locals declared above it.

```brix
(main
  (print "before")
  (let label "local label")
  (<> Thing (id: thing)
    (= name label)
  )
  (print (. thing name))
)

(type + Thing
  (prop + name _ (= "unnamed"))
  (fn-do + This _
    (print "constructed")
  )
)
```

```output
before
constructed
local label
```

```brix
(main
  (<> Foo (id: a) (value: 1)
    (<> Foo (id: b) (value: 2)
      (<> Foo (id: c) (value: 3))
    )
  )
)

(type + Foo
  (prop + a _ (= 0))
  (prop + contents (? Foo))
  (fn-do + This _
    (param a _)
    (print (str "create " a))
  )
  (fn-do + (infix this += (param child Foo)) _
    (= contents child)
  )
)
```

```output
create 1
create 2
create 3
```

### Nested named markup updates matching properties after mount

```brix
(main
  (<> Demo (id: demo))
  (blank)
  (= (. demo child value) 7)
  (let mounted (as (?. (. demo first) first) Child))
  (print (== mounted (. demo child)))
  (print (. mounted value))
  (print (. demo child value))
)

(type + Box
  (prop + first (? Box))
  (fn-do + (infix this += (param child Box)) _
    (if (not first)
      (= first child)
    )
    (return child)
  )
)

(type + Demo (is Box)
  (prop + child Child)
  (content
    (<> Box
      (<> Child (id: child))
    )
  )
)

(type + Child (is Box)
  (prop + value _ (= 0))
)
```

```output
true
7
7
```

### A named markup element nested in an element whose class has a member of the same name keeps its own identity instead of resolving to that member.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print (. demo rows))
  (+= (. demo items) "c")
  (print (. demo rows))
  (print (== (. demo holder list) null))
)

(type + Holder (is Box)
  (fn-expr + list (? Box) :get
    null
  )
)

(type + Demo (is Box)
  (prop + items (list String))
  (fn-do + This _
    (= items (call (of List String) (list "a" "b")))
  )
  (content
    (<> This
      (<> Holder (id: holder)
        (<> Box (id: list)
          (for (in item #items)
            (<> TextBox (value: item))
          )
        )
      )
    )
  )
  (fn-do + rows _ :get
    (let count 0)
    (for (in child (. (!! (. list %first-child)) children))
      (+= count 1)
    )
    (return count)
  )
)
```

```output
2
3
true
```

### Class-content markup ids are private properties that methods can read and assign through

```brix
(main
  (<> Demo (id: demo))
  (blank)
  (call (. demo update) "hi")
  (print (call (. demo read)))
  (let panel (as (. (!! (. demo first)) first) Panel))
  (print (call (. panel read)))
)

(type + Box
  (prop + first (? Box))
  (fn-do + (infix this += (param child Box)) _
    (if (not first)
      (= first child)
    )
    (return child)
  )
)

(type + Label (is Box)
  (prop + text _ (= ""))
)

(type + Panel (is Box)
  (content
    (<> Label (id: status)
      (= text "inner")
    )
  )
  (fn-expr + read String
    (. status text)
  )
)

(type + Demo (is Box)
  (content
    (<> Box
      (<> Panel
        (<> Label (id: status)
          (= text "outer")
        )
        (print (. status text))
      )
    )
  )
  (fn-do + update _
    (param text String)
    (= (. status text) text)
  )
  (fn-expr + read String
    (. status text)
  )
)
```

```output
outer
hi
inner
```

### Class-content markup ids construct each named element once, in reference order

```brix
(main
  (print (call (. (call Demo) read)))
)

(type + Box
  (fn-do + (infix this += (param child Box)) _
    (return child)
  )
)

(type + Label (is Box)
  (prop + text _ (= ""))
  (fn-do + This _
    (print "constructed Label")
  )
)

(type + Echo (is Box)
  (prop + source (? Label))
)

(type + Demo (is Box)
  (content
    (<> Box
      (<> Echo (id: echo)
        (= source status)
      )
      (<> Label (id: status)
        (= text "named")
      )
    )
  )
  (fn-expr + read String
    (str (. status text) " " (?? (?. (. echo source) text) "none"))
  )
)
```

```output
constructed Label
named named
```

### Property initializers that read class-content markup ids initialize after the markup constructs them

```brix
(main
  (print (. (call Demo) early))
)

(type + Box
  (fn-do + (infix this += (param child Box)) _
    (return child)
  )
)

(type + Label (is Box)
  (prop + text _ (= ""))
)

(type + Demo (is Box)
  (prop + early _ (= (. status text)))
  (content
    (<> Label (id: status)
      (= text "named")
    )
  )
)
```

```output
named
```

### Deferred property initializers construct each named element once

```brix
(main
  (let demo (call Demo))
  (print (?? (. demo selected) "none"))
)

(type + Box
  (fn-do + (infix this += (param child Box)) _
    (return child)
  )
)

(type + Label (is Box)
  (prop + text _ (= ""))
  (fn-do + This _
    (print "constructed Label")
  )
)

(type + Demo (is Box)
  (prop + selected (? String) (= (. status text)))
  (content
    (<> Label (id: status)
      (= text "named")
    )
  )
)
```

```output
constructed Label
named
```

### Tracked readers of a deferred property follow the named element it reads

```brix
(main
  (let demo (call Demo))
  (print (. demo shout))
  (call (. demo rename))
  (print (?? (. demo current) "none"))
  (print (. demo shout))
)

(type + Box
  (fn-do + (infix this += (param child Box)) _
    (return child)
  )
)

(type + Label (is Box)
  (prop + text _ (= ""))
)

(type + Demo (is Box)
  (prop + current (? String) (= (. status #text)))
  (prop + shout _ (= (str (?? #current "none") "!")))
  (content
    (<> Label (id: status)
      (= text "named")
    )
  )
  (fn-do + rename _
    (= (. status text) "renamed")
  )
)
```

```output
named!
renamed
renamed!
```

### Initializers that read a deferred property initialize after it

```brix
(main
  (print (. (call Demo) echo))
)

(type + Box
  (fn-do + (infix this += (param child Box)) _
    (return child)
  )
)

(type + Label (is Box)
  (prop + text _ (= ""))
)

(type + Demo (is Box)
  (prop + label String (= (. status text)))
  (prop + echo _ (= label))
  (content
    (<> Label (id: status)
      (= text "named")
    )
  )
)
```

```output
named
```

### Deferred property readers initialize after their sources regardless of declaration order

```brix
(main
  (let demo (call Demo))
  (print (. demo loud))
  (print (. demo echo))
)

(type + Box
  (fn-do + (infix this += (param child Box)) _
    (return child)
  )
)

(type + Label (is Box)
  (prop + text _ (= ""))
)

(type + Demo (is Box)
  (prop + loud _ (= (str echo "!")))
  (prop + echo _ (= label))
  (prop + label String (= (. status text)))
  (content
    (<> Label (id: status)
      (= text "named")
    )
  )
)
```

```output
named!
named
```

### Markup bindings that read a deferred property follow it once it is given

```brix
(main
  (print (. (call Demo) echo text))
)

(type + Box
  (fn-do + (infix this += (param child Box)) _
    (return child)
  )
)

(type + Label (is Box)
  (prop + text _ (= ""))
)

(type + Demo (is Box)
  (prop + shout _ (= (str (. status #text) "!")))
  (content
    (<> Label (id: echo)
      (= text #shout)
    )
    (<> Label (id: status)
      (= text "named")
    )
  )
)
```

```output
named!
```

### A value with locals of its own gives the properties it reads their values first

```brix
(main
  (<> Demo (id: demo)
    (= client (call Client))
  )
  (print (call (. ", " join) (. demo names)))
)

(type + Box
  (fn-do + (infix this += (param child Box)) _
    (return child)
  )
)

(type + Base (is Box)
  (prop + names (list String))
)

(type + Client
  (prop + names _ (= (list "a" "b")))
)

(type + Demo (is Base)
  (prop + client Client)
  (content
    (<> This
      (= names (call (of List String) (in (str "<" each ">") (. client names))))
    )
  )
)
```

```output
<a>, <b>
```

### Properties that depend on each other through deferred properties are rejected

```brix compile-error
(main
  (print (. (call Demo) label))
)

(type + Box
  (fn-do + (infix this += (param child Box)) _
    (return child)
  )
)

(type + Label (is Box)
  (prop + text _ (= ""))
)

(type + Demo (is Box)
  (prop + label String (= (str (. status text) echo)))
  (prop + echo _ (= label))
  (content
    (<> Label (id: status)
      (= text "named")
    )
  )
)
```

```output
Exception: Property "label" depends on itself through properties that initialize after its markup
<no file:17>
      (prop + label String (= (str (. status text) echo)))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Properties that initialize after their markup must have a type that has a value until then

```brix compile-error
(main
  (print (. (call Demo) words length))
)

(type + Box
  (fn-do + (infix this += (param child Box)) _
    (return child)
  )
)

(type + Label (is Box)
  (prop + text _ (= ""))
)

(type + Demo (is Box)
  (prop + label (? String) (= (. status text)))
  (prop + words _ (= (list (?? label ""))))
  (content
    (<> Label (id: status)
      (= text "named")
    )
  )
)
```

```output
Exception: Property "words" initializes after its markup, so its type must be optional
<no file:18>
      (prop + words _ (= (list (?? label ""))))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### When rules that read a named markup element run after the markup constructs it

```brix
(main
  (let demo (call Demo))
  (print (. demo runs))
  (= (. demo status text) "")
  (= (. demo status text) "again")
  (print (. demo runs))
)

(type + Box
  (fn-do + (infix this += (param child Box)) _
    (return child)
  )
)

(type + Label (is Box)
  (prop + text _ (= ""))
)

(type + Demo (is Box)
  (prop + runs _ (= 0))
  (content
    (<> Label (id: status)
      (= text "named")
    )
  )
  (when-rule (. status #text)
    (+= runs 1)
  )
)
```

```output
1
2
```

### Constructor bodies cannot read class-content markup ids before the markup constructs them

```brix compile-error
(main
  (let demo (call Demo))
)

(type + Box
  (fn-do + (infix this += (param child Box)) _
    (return child)
  )
)

(type + Label (is Box)
  (prop + text _ (= ""))
)

(type + Demo (is Box)
  (content
    (<> Label (id: status)
      (= text "named")
    )
  )
  (fn-do + This _
    (print (. status text))
  )
)
```

```output
Exception: Named markup element "status" is read before its markup constructs it
<no file:23>
        (print (. status text))
                  ^^^^^^
```

### A class-content markup id assigns an inherited property instead of declaring its own

```brix
(main
  (print (call (. (call Sub) read)))
)

(type + Box
  (fn-do + (infix this += (param child Box)) _
    (return child)
  )
)

(type + Label (is Box)
  (prop + text _ (= ""))
)

(type + Base (is Box)
  (prop + status (? Label))
  (fn-expr + read String
    (?? (?. status text) "none")
  )
)

(type + Sub (is Base)
  (content
    (<> Label (id: status)
      (= text "sub")
    )
  )
)
```

```output
sub
```

### A class-content markup id cannot share its name with a property of the class that cannot hold the element, where the element's body would read the element for the property.

```brix compile-error
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let status (call Status))
  (print (. status activity))
)

(type + Status (is Box)
  (prop + activity _ (= "busy"))
  (content
    (<> Label (id: activity)
      (= text #activity)
    )
  )
)
```

```output
Exception: Named markup element "activity" clashes with property "activity", which holds String, not Label; rename the element
<no file:13>
        (<> Label (id: activity)
        ^^^^^^^^^^^^^^^^^^^^^^^
```

### A class-content markup id in a branch of the markup cannot share its name with a property of the class that cannot hold the element.

```brix compile-error
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let status (call Status))
  (print (. status activity))
)

(type + Status (is Box)
  (prop + busy _ (= true))
  (prop + activity _ (= "busy"))
  (content
    (<> This
      (if #busy
        (<> Label (id: activity)
          (= text #activity)
        )
        (else
          (<> Label
            (= text "idle")
          )
        )
      )
    )
  )
)
```

```output
Exception: Named markup element "activity" clashes with property "activity", which holds String, not Label; rename the element
<no file:16>
            (<> Label (id: activity)
            ^^^^^^^^^^^^^^^^^^^^^^^
```

### Property in content section

```brix
(main
  (<> Thing (id: foo))
  (print (. foo bar))
  (print (. foo baz))
)

(type + Thing
  (content
    (prop + bar _ (= xxx))
    (prop + baz _ (= this))
    (blank)
    (lazy xxx 42)
  )
)
```

```output
42
[Object __source__:Thing]
```

### Broadcaster property in content section

```brix
(main
  (<> Thing (id: foo))
  (for (on item (. foo bar))
    (print item)
  )
)

(type + Thing
  (prop + a _ (= 99))
  (content
    (prop + bar _ (= (on (select (str "hi " a " " y " " x) (<* x)) xxx)))
    (blank)
    (lazy xxx (list 1 2 3))
    (lazy y false)
  )
)
```

```output
hi 99 false 1
hi 99 false 2
hi 99 false 3
```

### Class content

```brix
(main
  (print (call Foo))
)

(type + Foo
  (fn-do + (infix this += (param content _)) _
    (print content)
  )
  (content
    (<> Bar)
  )
)

(type + Bar)
```

```output
[Object __source__:Bar]
[Object __source__:Foo]
```

### Structural % reads are sidecar-backed and infer concrete MarkupParent child types.

```brix
(import /core/markup)
(blank)

(main
  (let root (call Node "root"))
  (let a (call Node "a"))
  (let b (call Node "b"))
  (let c (call Node "c"))
  (blank)
  (call markup-append root a)
  (call markup-append root b)
  (call markup-append a c)
  (blank)
  (print (call (. root describe)))
  (print (call (. a describe)))
  (print (call (. b describe)))
  (print (call (. c describe)))
  (blank)
  (call markup-release root)
  (blank)
  (print (call (. c describe)))
  (blank)
  (call markup-remove root a)
  (blank)
  (print (call (. root describe)))
  (print (call (. a describe)))
  (print (call (. c describe)))
)

(type + Node
  (implements (of MarkupParent Node))
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
  (fn-expr + markup-content Node
    (param child Node)
    child
  )
  (fn-do + markup-attach-child _
    (param child Node)
    (param anchor (? Node))
  )
  (fn-do + markup-detach-child _
    (param child Node)
  )
  (fn-do + describe String
    (let parent (? Node) %parent)
    (let first (? Node) (. this %first-child))
    (let last (? Node) %last-child)
    (let previous (? Node) (. this %previous-sibling))
    (let next (? Node) %next-sibling)
    (blank)
    (return (str name " parent=" (call name-of parent) " first=" (call name-of first) " last=" (call name-of last) " previous=" (call name-of previous) " next=" (call name-of next)))
  )
  (fn-do + name-of String
    (param node (? Node))
    (return-if (. node name) (if node))
    (return "-")
  )
)
```

```output
root parent=- first=a last=b previous=- next=-
a parent=root first=c last=c previous=- next=b
b parent=root first=- last=- previous=a next=-
c parent=a first=- last=- previous=- next=-
c parent=a first=- last=- previous=- next=-
root parent=- first=b last=b previous=- next=-
a parent=- first=c last=c previous=- next=-
c parent=a first=- last=- previous=- next=-
```

### Mounted callbacks run after the committed structural tree is coherent.

```brix
(import /core/markup)
(blank)

(main
  (let log (call MountLog))
  (let root (call MountNode "root" log))
  (let parent (call MountNode "parent" log))
  (let child (call MountNode "child" log))
  (blank)
  (call markup-append root parent)
  (call markup-append parent child)
  (print (str "before=" (. log text)))
  (blank)
  (call markup-mount-root root root)
  (print (. log text))
  (blank)
  (call (. log clear))
  (let late (call MountNode "late" log))
  (let late-child (call MountNode "late-child" log))
  (call markup-append late late-child)
  (call markup-append root late)
  (print (. log text))
  (blank)
  (call (. log clear))
  (let replacement (call MountNode "replacement" log))
  (call markup-replace root parent replacement)
  (print (. log text))
)

(type + MountLog
  (prop + text _ (= ""))
  (fn-do + clear _
    (= text "")
  )
  (fn-do + record _
    (param line String)
    (if (. text length)
      (+= text " | ")
    )
    (+= text line)
  )
)

(type + MountNode
  (implements (of MarkupParent MountNode))
  (implements MarkupRootElement)
  (prop + name String)
  (prop + log MountLog)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + This _
    (param name _)
    (param log _)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (fn-do + markup-attach-child _
    (param child MountNode)
    (param anchor (? MountNode))
  )
  (fn-do + markup-detach-child _
    (param child MountNode)
  )
  (fn-do + mounted _
    (call (. log record) (call describe))
  )
  (fn-expr + describe String
    (str name " parent=" (call name-of %parent) " first=" (call name-of %first-child) " next=" (call name-of %next-sibling) " root=" (!= markup-batch-root null))
  )
  (fn-do + name-of String
    (param node (? MountNode))
    (return-if (. node name) (if node))
    (return "-")
  )
)
```

```output
before=
root parent=- first=parent next=- root=true | parent parent=root first=child next=- root=true | child parent=parent first=- next=- root=true
late parent=root first=late-child next=- root=true | late-child parent=late first=- next=- root=true
replacement parent=root first=- next=late root=true
```

### A host with hook-shaped methods but no MarkupParent conformance is rejected at the mutation site; the structural hooks are an interface contract, not duck typing.

```brix crash
(import /core/markup)
(blank)

(main
  (let host (call DuckNode))
  (call markup-append host (call DuckNode))
)

(type + DuckNode
  (implements MarkupElement)
  (fn-do + markup-attach-child _
    (param child DuckNode)
    (param anchor (? DuckNode))
  )
  (fn-do + markup-detach-child _
    (param child DuckNode)
  )
)
```

```output
Exception: No function "markup-append" accepts (DuckNode, DuckNode); it is declared as "markup-append(parent Host, child Child)"
<no file:7>
      (call markup-append host (call DuckNode))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A root <This> in class content configures existing properties as construction defaults and lowers its body as normal content.

```brix
(main
  (let plain (call Card))
  (let child (call ChildCard "Message"))
  (<> Card (id: custom)
    (= title "Custom")
  )
  (blank)
  (print (. plain title))
  (print (. custom title))
  (print (. child title))
  (print (!! (. plain first)))
  (print (!! (. child first)))
)

(type + Box
  (prop + first (? PanelLabel))
  (fn-do + (infix this += (param child PanelLabel)) _
    (= first child)
  )
)

(type + Label)

(type + Card (is Box)
  (prop + title _ (= "Default"))
  (content
    (<> This
      (<> PanelLabel
        (= title title)
      )
    )
  )
)

(type + PanelLabel (is Label)
  (prop + title _ (= "Panel Default"))
  (fn-to-expr + String
    (str "[Label " title "]")
  )
)

(type + ChildCard (is Card)
  (prop + message String)
  (prop + title _ (= "Child"))
  (fn-do + This _
    (param message _)
  )
)
```

```output
Default
Custom
Child
[Label Default]
[Label Child]
```

### A root <This> property default that reads tracked state stays live, like a declared default.

```brix
(main
  (let source (call Source))
  (let card (call Card source))
  (let child (call ChildCard source))
  (print (. card label))
  (print (. card declared))
  (print (. child label))
  (= (. source count) 5)
  (print (. card label))
  (print (. card declared))
  (print (. child label))
)

(type + Source
  (prop + count _ (= 1))
)

(type + Box)

(type + Card (is Box)
  (prop + source Source)
  (prop + label Int)
  (prop + declared _ (= (* (. source #count) 10)))
  (fn-do + This _
    (param source _)
  )
  (content
    (<> This
      (= label (* (. source #count) 10))
    )
  )
)

(type + ChildCard (is Card)
  (prop + scale _ (= 100))
  (fn-do + This _
    (param source _)
  )
  (content
    (<> This
      (= label (* (. source #count) scale))
    )
  )
)
```

```output
10
10
100
50
50
500
```

### A subclass default for a property overrides an inherited constructor argument's default, but an argument the caller passes still wins.

```brix
(main
  (let declared (call DeclaredButton))
  (let rooted (call RootedButton))
  (let given (call RootedButton "Given"))
  (<> RootedButton (id: named)
    (= title "Named")
  )
  (blank)
  (print (. declared title))
  (print (. rooted title))
  (print (. given title))
  (print (. named title))
)

(type + Button
  (prop + title _ (= ""))
  (fn-do + This _
    (param title _ (= ""))
  )
)

(type + DeclaredButton (is Button)
  (prop + title _ (= "Declared"))
)

(type + RootedButton (is Button)
  (content
    (<> This
      (= title "Rooted")
    )
  )
)
```

```output
Declared
Rooted
Given
Named
```

### A lambda a line of a root <This> gives a property reads the object when it runs, as one the class's markup gives a child does: its members, its methods and the elements its markup names.

```brix
(main
  (let panel (call Panel))
  (call (. panel fire))
  (call (. panel fire))
  (print (. panel seen))
)

(type + Box
  (prop + run (? Doer))
  (fn-do + (infix this += (param child Note)) _)
  (fn-do + fire _
    (let callback run)
    (if callback
      (call callback)
    )
  )
)

(type + Note
  (prop + text _ (= "note"))
)

(type + Panel (is Box)
  (prop + count _ (= 0))
  (prop + seen _ (= ""))
  (content
    (<> This
      (= run (=> () :do (call bump (. note text))))
      (<> Note (id: note))
    )
  )
  (fn-do + bump _
    (param text String)
    (+= count 1)
    (= seen (str text " " count))
  )
)
```

```output
note 2
```

### An anonymous state machine may sit beside an explicit root <This> and runs after earlier markup.

```brix
(main
  (let card (call Card))
  (blank)
  (print (!= (. card first) null))
  (print (. card initialized))
)

(type + Box
  (prop + first (? PanelLabel))
  (fn-do + (infix this += (param child PanelLabel)) _
    (= first child)
  )
)

(type + PanelLabel)

(type + Card (is Box)
  (prop + initialized _ (= false))
  (content
    (<> This
      (<> PanelLabel)
    )
    (blank)
    (state ready
      (do
        (= initialized (!= first null))
      )
    )
  )
)
```

```output
true
true
```

### An anonymous state machine may sit beside a root <This> with no lines.

```brix
(main
  (let knob (call Knob))
  (blank)
  (print (. knob is-idle))
)

(type + Knob
  (content
    (<> This)
    (blank)
    (state is-idle)
  )
)
```

```output
true
```

### Root <This> must wrap all class content.

```brix crash
(main
  (<> Card)
)

(type + Card
  (content
    (<> This)
    (<> Card)
  )
)
```

```output
Exception: Root <This> must wrap all class content
<no file:8>
        (<> This)
        ^^^^^^^^^
```

### Root <This> assignments do not declare new properties.

```brix crash
(main
  (<> Card)
)

(type + Card
  (content
    (<> This
      (= missing 1)
    )
  )
)
```

```output
Exception: Property "missing" not found
<no file:9>
          (= missing 1)
             ^^^^^^^
```

### Root <This> configures an existing object; it does not call a constructor.

```brix crash
(main
  (<> Card)
)

(type + Card
  (content
    (<> This (value: 1))
  )
)
```

```output
Exception: Root <This> cannot pass constructor arguments
<no file:8>
        (<> This (value: 1))
                         ^
```

### Root <This> uses normal markup body rules: "when" belongs only inside style rules.

```brix crash
(main
  (<> Card)
)

(type + Card
  (prop + is-on _ (= false))
  (content
    (<> This
      (when-style #is-on
        (= is-on true)
      )
    )
  )
)
```

```output
Exception: (when-style …) cannot be a `row` of (<> …), which takes Child.
<no file:10>
          (when-style #is-on
          ^
```

### Markup list properties materialize heterogeneous union feeds once before initial assignment.

```brix
(main
  (<> MarkupListHost (id: host)
    (= items
      (block
        (item (call NumberItem 7))
        (item (call TextItem "seven"))
      )
    )
  )
  (blank)
  (print (. host items length))
  (print (is ([] (. host items) 0) NumberItem))
  (print (is ([] (. host items) 1) TextItem))
)

(type + NumberItem (is Tuple)
  (prop + value Int)
)

(type + TextItem (is Tuple)
  (prop + value String)
)

(type + MarkupListItem (is Union)
  (variant NumberItem)
  (variant TextItem)
)

(type + MarkupListHost
  (prop + items (list MarkupListItem))
)
```

```output
2
true
true
```

```brix
(main
  (print (. (call null-stream) children length))
)

(fn-expr + null-stream ChildHost
  (<> ChildHost
    (call no-children)
  )
)

(fn-do + no-children (? (of ImmutableArray ChildHost))
  (return-if (if true))
)

(type + ChildHost
  (prop + children (list This))
  (fn-do + (infix this += (param kids (? (stream This)))) _
    (if kids
      (for (in kid kids)
        (+= children (as kid This))
      )
    )
  )
)
```

```output
0
```

```brix crash
(main
  (let sink (call make-optional-stream-sink))
)

(type + OptionalStreamSink
  (prop + name String)
  (prop + children (list This))
  (fn-do + This _
    (param name String)
  )
  (fn-do + (infix this += (param kids (stream This))) _
    (for (in kid kids)
      (+= children (as kid This))
    )
  )
)

(fn-expr + make-optional-stream-sink OptionalStreamSink
  (<> OptionalStreamSink (text: "root")
    (call no-optional-stream-children)
  )
)

(fn-do + no-optional-stream-children (? (of ImmutableArray OptionalStreamSink))
  (return-if (if true))
)
```

```output
Exception: Content not accepted
<no file:21>
        (call no-optional-stream-children)
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(import /core/markup)
(blank)

(main
  (let root (call Node))
  (let child (call Node))
  (call markup-attach-new-child root child null)
)

(type + Node
  (implements (of MarkupParent Node))
  (fn-expr + markup-content Node
    (param child Node)
    child
  )
  (fn-do + markup-attach-child _
    (param child Node)
    (param anchor (? Node))
    (return)
  )
  (fn-do + markup-detach-child _
    (param child Node)
    (return)
  )
)
```

```output
Exception: Function "markup-attach-new-child" is private, declared with _, so only its own module can call it
<no file:8>
      (call markup-attach-new-child root child null)
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(import /core/markup)
(blank)
; @markup-runtime-internal
(blank)

(main
  (let root (call Node))
  (let child (call Node))
  (call markup-attach-new-child root child null)
)

(type + Node
  (implements (of MarkupParent Node))
  (fn-expr + markup-content Node
    (param child Node)
    child
  )
  (fn-do + markup-attach-child _
    (param child Node)
    (param anchor (? Node))
    (return)
  )
  (fn-do + markup-detach-child _
    (param child Node)
    (return)
  )
)
```

```output
Exception: Function "markup-attach-new-child" is private, declared with _, so only its own module can call it
<no file:10>
      (call markup-attach-new-child root child null)
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Collection content goes through ++= when += takes a single child.

```brix
(main
  (let extra (list (call Label "b") (call Label "c")))
  (<> Window (id: win)
    (<> Label (value: "a"))
    extra
  )
  (blank)
  (print (. win contents))
)

(type + Window
  (prop + contents (list Label))
  (fn-do + (infix this += (param child Label)) _
    (+= contents child)
  )
  (fn-do + (infix this ++= (param children (stream Label))) _
    (++= contents children)
  )
)

(type + Label
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
  (fn-to-expr + String
    (str "[Label " text "]")
  )
)
```

```output
[Label a], [Label b], [Label c]
```

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> QueryRoot (id: root))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (print (. root summary))
  (blank)
  (= (. root show-extra) true)
  (call (. root markup-batch-queue checkpoint))
  (print (. root summary))
  (blank)
  (= (. root show-first) false)
  (call (. root markup-batch-queue checkpoint))
  (print (. root summary))
  (blank)
  (= (. root show-extra) false)
  (call (. root markup-batch-queue checkpoint))
  (print (. root summary))
  (blank)
  (= (. root show-first) true)
  (call (. root markup-batch-queue checkpoint))
  (print (. root summary))
)

(type + Marker (is Box)
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + QueryRoot (is Box)
  (implements MarkupRootElement)
  (prop + show-first _ (= true))
  (prop + show-extra _ (= false))
  (prop + markers (of Descendants Marker))
  (prop + summary _ (= (call (. "|" join) (in (. each text) #markers))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (content
    (<> This
      (if #show-first
        (<> Marker (value: "first"))
      )
      (<> Marker (value: "base"))
      (if #show-extra
        (<> Marker (value: "extra"))
      )
    )
  )
)
```

```output
first|base
first|base|extra
base|extra
base
first|base
```

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> QueryRoot (id: root)
    (<> Box
      (<> Mirror (id: mirror))
    )
  )
  (blank)
  (print (. mirror text))
  (call (. root markup-batch-queue checkpoint))
  (print (. mirror text))
  (blank)
  (= (. root title) "renamed")
  (print (. mirror text))
  (call (. root markup-batch-queue checkpoint))
  (print (. mirror text))
  (blank)
  (= (. root suffix) "!")
  (call (. root markup-batch-queue checkpoint))
  (print (. mirror text))
)

(type + Mirror (is Box)
  (prop + owner (of Ancestor QueryRoot))
  (prop + text _ (= (?? (?. #owner #label) "none")))
)

(type + QueryRoot (is Box)
  (implements MarkupRootElement)
  (prop + title _ (= "root"))
  (prop + suffix _ (= ""))
  (prop + label _ (= (+ #title #suffix)))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)
```

```output
root
root
renamed
renamed
renamed!
```

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let root (call ProbeRoot))
  (let parent (call Probe))
  (let child (call Probe))
  (call markup-append root parent)
  (call markup-append parent child)
  (print (str (. parent mounted-count) " " (. child mounted-count)))
  (blank)
  (call markup-mount-root root root)
  (print (str (. parent mounted-count) " " (. child mounted-count)))
  (blank)
  (call markup-mount-root root root)
  (print (str (. parent mounted-count) " " (. child mounted-count)))
)

(type + ProbeRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Probe (is Box)
  (prop + mounted-count _ (= 0))
  (fn-do + mounted _
    (+= mounted-count 1)
  )
)
```

```output
0 0
1 1
1 1
```

### An if line that matches a pattern runs its body as content of the element.

```brix
(main
  (let item Item (call Note "one"))
  (<> Window (id: win)
    (if (= (| (: note Note)) item)
      (<> Label (value: (. note text)))
      (else-if (= (| (: mark Mark)) item)
        (<> Label (value: "mark"))
      )
    )
  )
  (blank)
  (print (. win contents))
)

(type + Item (is Union)
  (variant Note)
  (variant Mark)
)

(type + Note
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Mark
  (prop + count Int)
  (fn-do + This _
    (param count _)
  )
)

(type + Window
  (prop + contents (list Label))
  (fn-do + (infix this += (param child Label)) _
    (+= contents child)
  )
)

(type + Label
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
  (fn-to-expr + String
    (str "[Label " text "]")
  )
)
```

```output
[Label one]
```

### Rejects a section of a class with nothing written in it, whether or not the class is used.

```brix crash
(main
  (print 1)
)

(type + Thing
  (prop + count _ (= 1))
  (content)
  (fn-expr + doubled _ :get
    (* count 2)
  )
)
```

```output
Exception: Empty content section not permitted
<no file:8>
      (content)
      ^^^^^^^^^
```

### Rejects a second content section of a class, whether or not the class is used.

```brix crash
(main
  (print 1)
)

(type + Thing
  (content
    (<> This)
  )
  (content
    (<> This)
  )
)
```

```output
Exception: Only one content section permitted
<no file:10>
      (content
      ^^^^^^^
```

### Reads a property a markup element's name declares from another module.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import ./meter)
(blank)

(main
  (let meter (call Meter))
  (let fill (. meter fill))
  (print "read")
)
```

```brix file=meter.bx
(import /core/markup)
(import /core/tests/support/markup-fixtures)

(type + Meter (is Box)
  (content
    (<> This
      (<> Box (id: fill))
    )
  )
)
```

```output
read
```

### An error the markup of a class raises as it fills an object goes to unhandled-error, and the lines before it stay.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (= unhandled-error
    (=> (error) :do
      (block
        (print (str "handled " (. error message)))
      )
    )
  )
  (blank)
  (let panel (call Panel))
  (print (is (!! (. panel %first-child)) Label))
  (print "made")
)

(fn-do + parse Int
  (param text String)
  (throw (call SyntaxError "bad") (if (== text "x")))
  (return (. text length))
)

(type + Panel (is Box)
  (prop + value _ (= "x"))
  (content
    (<> This
      (<> Label
        (= text "a")
      )
      (<> Label
        (= text (str (call parse value)))
      )
    )
  )
)
```

```output
handled bad
true
made
```

### A closure an attribute of a markup element gives reads a member of an element around it through that element's name, from its object when it runs.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
  (let outer (as (!! (. panel %first-child)) Item))
  (let inner (as (!! (. outer %first-child)) Item))
  (call (. inner fire))
  (print (. panel seen))
  (= (. outer title) "changed")
  (call (. inner fire))
  (print (. panel seen))
)

(type + Item (is Box)
  (prop + title _ (= ""))
  (prop + run (? Doer))
  (fn-do + This _
    (param title _ (= ""))
  )
  (fn-do + fire _
    (let callback run)
    (if callback
      (call callback)
    )
  )
)

(type + Panel (is Box)
  (prop + seen _ (= ""))
  (content
    (<> This
      (<> Item (id: outer) (text: "outer")
        (<> Item (text: "inner")
          (= run (=> () :do (call note (. outer title))))
        )
      )
    )
  )
  (fn-do + note _
    (param text String)
    (= seen text)
  )
)
```

```output
outer
changed
```

### A read of a markup link written after `?` reads from what the link before it holds, and gives nothing where that holds nothing.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
  (print (!= (call (. panel grandchild)) null))
  (print (!= (call (. panel great-grandchild)) null))
)

(type + Panel (is Box)
  (content
    (<> This
      (<> Box
        (<> Box)
      )
    )
  )
  (fn-do + grandchild (? Box)
    (return (?. %first-child %first-child))
  )
  (fn-do + great-grandchild (? Box)
    (return (?. (?. %first-child %first-child) %first-child))
  )
)
```

```output
true
false
```

### A loop on a stream in the body of a markup element subscribes to the element's event, and reads the scope outside the element as its attributes do: the item of the loop around it, not the element's member of that name.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
  (let first (as (!! (. panel %first-child)) Item))
  (let second (as (!! (. first %next-sibling)) Item))
  (<- (. first clicked) 3)
  (<- (. second clicked) 4)
  (do-new
    (print (. panel seen))
    (= (. second tag) "changed")
    (<- (. second clicked) 5)
    (do-new
      (print (. panel seen))
    )
  )
)

(type + Item (is Box)
  (prop + tag _ (= ""))
  (event + clicked Int)
)

(type + Panel (is Box)
  (prop + seen _ (= ""))
  (content
    (<> This
      (for (in tag (list "a" "b"))
        (<> Item
          (= tag (++ tag "!"))
          (for (on n clicked)
            (call note (str tag " " n))
          )
        )
      )
    )
  )
  (fn-do + note _
    (param text String)
    (= seen (++ seen text ";"))
  )
)
```

```output
a 3;b 4;
a 3;b 4;b 5;
```

### A descendant query of a class with type parameters looks for its target for the parameters the object's class gives it.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> (of MarkerRoot Int) (id: root))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (print (. root markers length))
  (print (call (. "|" join) (in (str (. each value)) (. root markers))))
)

(type + (of Marker T) (is Box)
  (prop + value T)
  (fn-do + This _
    (param value T)
  )
)

(type + (of MarkerRoot T) (is Box)
  (implements MarkupRootElement)
  (prop + markers (of Descendants (of Marker T)))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (content
    (<> This
      (<> (of Marker T) (value: (as 1 T)))
      (<> Box
        (<> (of Marker T) (value: (as 2 T)))
      )
    )
  )
)
```

```output
2
1|2
```

### A descendant query of a class with type parameters, made by a call, finds the objects of a class whose constructor was compiled before the query's class was made for it.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let early (call Leaf))
  (let root (call (of Finder Leaf)))
  (blank)
  (call (. root markup-batch-queue checkpoint))
  (print (. root found length))
)

(type + Leaf (is Box))

(type + (of Finder T) (is Box)
  (implements MarkupRootElement)
  (prop + found (of Descendants T))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
  (content
    (<> This
      (<> Leaf)
      (<> Box
        (<> Leaf)
      )
    )
  )
)
```

```output
2
```

### A tracked read in a closure an attribute gives is an error, as the closure runs outside the attribute's live value.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
  (let item (as (!! (. panel %first-child)) Item))
  (call (. item fire))
  (print (. panel seen))
  (= (. panel count) 4)
  (call (. item fire))
  (print (. panel seen))
  (= (. panel count) 6)
  (call (. item fire))
  (print (. panel seen))
)

(type + Item (is Box)
  (prop + run (? Doer))
  (fn-do + fire _
    (let callback run)
    (if callback
      (call callback)
    )
  )
)

(type + Panel (is Box)
  (prop + seen _ (= ""))
  (prop + count _ (= 1))
  (content
    (<> This
      (<> Item
        (= run (=> () :do (call note (str "count " #count))))
      )
    )
  )
  (fn-do + note _
    (param text String)
    (= seen text)
  )
)
```

```output
Exception: "#count" tracks nothing here: a read is followed only where it is written directly in a live value, such as markup, a derived property, a rule or a live statement. Write "count" to read it plainly, or read "#count" in the live value and pass it in as an argument
<no file:34>
            (= run (=> () :do (call note (str "count " #count))))
                                                       ^^^^^^
```

### An attribute a setter answers is given again when the tracked state its value reads changes.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
  (let item (as (!! (. panel %first-child)) Item))
  (print (str (. item calls) " " (. item last)))
  (= (. panel count) 4)
  (print (str (. item calls) " " (. item last)))
)

(type + Item (is Box)
  (prop + calls _ (= 0))
  (prop + last _ (= 0))
  (fn-expr + shown Int :get
    last
  )
  (fn-do + shown _ :set (param value Int)
    (+= calls 1)
    (= last value)
  )
)

(type + Panel (is Box)
  (prop + count _ (= 1))
  (content
    (<> This
      (<> Item
        (= shown (* #count 10))
      )
    )
  )
)
```

```output
1 10
2 40
```

### The body of a `whenever` rule on a value runs only once the value changes, after the markup has made the element it reads by name.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print (str "made " (. demo inner count)))
  (= (. demo owner stamp) 5)
  (print (str "after " (. demo inner count)))
)

(type + Owner
  (prop + stamp U64 (= 0))
)

(type + Inner (is Box)
  (prop + count _ (= 0))
  (fn-do + reload _
    (+= count 1)
  )
)

(type + Demo (is Box)
  (prop + owner _ (= (call Owner)))
  (content
    (<> This
      (<> Inner (id: inner))
    )
  )
  (whenever-rule (. owner #stamp)
    (print "rule ran")
    (call (. inner reload))
  )
)
```

```output
made 0
rule ran
after 1
```

### A property a markup element sets that also names an untyped argument of the constructor of a class above the element's class is given to the constructor.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Row (id: row)
    (= note "first")
    (= title "hello")
  )
  (blank)
  (print (str (. row note) " " (. row title)))
)

(type + Tag (is Box)
  (prop + title String)
  (fn-do + This _
    (param title _ (= ""))
    (= (. this title) title)
  )
)

(type + Row (is Tag)
  (prop + note String)
)
```

```output
first hello
```

### An event of a markup element is no value: a handler in the element's body that declares a local of it is an error at the event.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
)

(type + Panel (is Box)
  (prop + seen _ (= ""))
  (content
    (<> This
      (<> Button (id: button)
        (for (on event press)
          (let pressed (. button press))
          (= seen "pressed")
        )
      )
    )
  )
)

(type + Button (is Box)
  (event + press Int)
)
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:15>
              (let pressed (. button press))
                           ^^^^^^^^^^^^^^^^
```

### An event of a markup element is no value: a handler in the element's body that prints it is an error at the event.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
)

(type + Panel (is Box)
  (content
    (<> This
      (<> Button (id: button)
        (for (on event press)
          (print (. button press))
        )
      )
    )
  )
)

(type + Button (is Box)
  (event + press Int)
)
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:14>
              (print (. button press))
                     ^^^^^^^^^^^^^^^^
```

### An event given to a parameter of a type its endpoint is not, which only converting it as a value could give, is an error at the event.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let button (call Button))
  (call (. button describe))
)

(type + Button (is Box)
  (event + press Int)
  (fn-do + describe _
    (call note press)
  )
  (fn-do + note _
    (param items (of Iterable Int))
    (for (in item items)
      (print item)
    )
  )
)
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:13>
        (call note press)
                   ^^^^^
```

### An event of a markup element is no value: a handler in the element's body that assigns it to a property of another type is an error at the event.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
)

(type + Panel (is Box)
  (prop + count _ (= 0))
  (content
    (<> This
      (<> Button (id: button)
        (for (on event press)
          (= count (. button press))
        )
      )
    )
  )
)

(type + Button (is Box)
  (event + press Int)
)
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:15>
              (= count (. button press))
                       ^^^^^^^^^^^^^^^^
```

### An event of a markup element is no value: a handler in the element's body that makes it an item of a list is an error at the event.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
)

(type + Panel (is Box)
  (prop + seen _ (= 0))
  (content
    (<> This
      (<> Button (id: button)
        (for (on event press)
          (let all (list (. button press)))
          (= seen (. all length))
        )
      )
    )
  )
)

(type + Button (is Box)
  (event + press Int)
)
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:15>
              (let all (list (. button press)))
                             ^^^^^^^^^^^^^^^^
```

### An event is no value: one made a field of a tuple is an error at the event.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let button (call Button))
  (call (. button describe))
)

(type + Button (is Box)
  (event + press Int)
  (fn-do + describe _
    (let pair (tuple press 1))
    (print "made")
  )
)
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:13>
        (let pair (tuple press 1))
                         ^^^^^
```

### An event of a markup element is no value: a handler in the element's body that casts it to a type its endpoint is not is an error at the event.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
)

(type + Panel (is Box)
  (prop + seen _ (= ""))
  (content
    (<> This
      (<> Button (id: button)
        (for (on event press)
          (= seen (as (. button press) String))
        )
      )
    )
  )
)

(type + Button (is Box)
  (event + press Int)
)
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:15>
              (= seen (as (. button press) String))
                          ^^^^^^^^^^^^^^^^
```

### An event of a markup element is no value: a handler in the element's body that gives it to a parameter of a type its endpoint is not is an error at the event.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
)

(type + Panel (is Box)
  (prop + seen _ (= ""))
  (content
    (<> This
      (<> Button (id: button)
        (for (on event press)
          (call record (. button press))
        )
      )
    )
  )
  (fn-do + record _
    (param items (of Iterable Int))
    (for (in item items)
      (= seen (str item))
    )
  )
)

(type + Button (is Box)
  (event + press Int)
)
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:15>
              (call record (. button press))
                           ^^^^^^^^^^^^^^^^
```

### An event is no value: an attribute of a markup element that gives it to a property of another type is an error at the event.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
)

(type + Panel (is Box)
  (event + poke Int)
  (content
    (<> This
      (<> Button
        (= count poke)
      )
    )
  )
)

(type + Button (is Box)
  (prop + count _ (= 0))
)
```

```output
Exception: "poke" is an event of Panel; it cannot be used as a value
<no file:14>
            (= count poke)
                     ^^^^
```

### A tracked property of the class, tested in an element nested in another, holds a value later in the same expression.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (blank)
  (print (. (as (!! (. (!! (. demo %first-child)) %first-child)) TextBox) text))
  (= (. demo limit) 1)
  (print (. (as (!! (. (!! (. demo %first-child)) %first-child)) TextBox) text))
)

(type + Demo (is Box)
  (prop + limit (? Int))
  (content
    (<> This
      (<> Box
        (<> TextBox
          (= text (str (and (!= #limit null) (>= 2 #limit))))
        )
      )
    )
  )
)
```

```output
false
true
```

### A markup element whose class declares only constructors that take arguments must give them; a name after the class names the element, even where a local of that name is in scope.

```brix compile-error
(main
  (call Host)
)

(type + Box
  (fn-do + (infix this += (param child Box)) _
    (return child)
  )
)

(type + Row (is Box)
  (prop + index Int)
  (fn-do + This _
    (param index _)
  )
)

(type + Host (is Box)
  (content
    (<> This
      (for (in index (..< 0 3))
        (<> Row (id: index))
      )
    )
  )
)
```

```output
Exception: Markup element <Row gives no value for "index", which the constructor of Row requires; "index" after <Row names the element
<no file:23>
            (<> Row (id: index))
                ^^^
```

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
)

(type + Pane (is Box)
  (prop + value Int (= 2))
)

(type + Demo (is Box)
  (prop + count _ (=> (. pane value)))
  (content
    (<> This
      (<> Pane (id: pane)
        (for (in index (..< 0 count))
          (<> Label (text: (str index)))
        )
      )
    )
  )
)
```

```stderr
Unhandled panic: Named markup element "pane" is read before its markup constructs it
<no file:14>

Task 1

Stack trace:
  0  __source__:Demo:count no file:14:26
  1  __source__:content no file:18:31
  2  __source__:Demo:Demo no file:13:1
  3  __source__:main no file:6:13
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A statement above a named element that reads its name makes the element there, once.

```brix
(main
  (print "before")
  (print (. thing name))
  (<> Thing (id: thing)
    (= name "made early")
  )
  (print (. thing name))
)

(type + Thing
  (prop + name _ (= "unnamed"))
  (fn-do + This _
    (print "constructed")
  )
)
```

```output
before
constructed
made early
made early
```

### A named element in a function body reads the locals declared above it, and so do the named lines of its body.

```brix
(main
  (print (. (!! (. (call make) inner)) name))
)

(fn-do + make Holder
  (let label "inner label")
  (<> Holder (id: holder)
    (= title label)
    (<> Thing (id: inner)
      (= name (str label "!"))
    )
  )
  (print (. holder title))
  (return holder)
)

(type + Holder
  (prop + title _ (= ""))
  (prop + inner (? Thing))
  (fn-do + (infix this += (param item Thing)) _
    (= inner item)
  )
)

(type + Thing
  (prop + name _ (= "unnamed"))
)
```

```output
inner label
inner label!
```

### An optional chain reads a named element another class's markup declares.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let reader (call Reader))
  (print (call (. reader inner-text)))
  (= (. reader holder) (call Holder))
  (print (call (. reader inner-text)))
)

(type + Inner (is Box)
  (prop + text _ (= "inner"))
)

(type + Holder (is Box)
  (content
    (<> This
      (<> Inner (id: inner))
    )
  )
)

(type + Reader
  (prop + holder (? Holder))
  (fn-do + inner-text String
    (return (?? (. (?. holder inner) text) "none"))
  )
)
```

```output
none
inner
```

### A live statement that sets a property of a markup element follows what its value reads: the element's action runs at once where the element has no batch root, and with its root's batch where it has one.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let counter (call Counter))
  (let loose (call Label))
  (= (. loose text) (str "loose " (. counter #count)))
  (blank)
  (<> Demo (id: demo)
    (<> Label (id: label))
  )
  (blank)
  (= (. label text) (str "held " (. counter #count)))
  (call (. demo markup-batch-queue checkpoint))
  (= (. counter count) 3)
  (print (. loose text))
  (print (. label text))
  (call (. demo markup-batch-queue checkpoint))
  (print (. label text))
)

(type + Counter
  (prop + count Int)
)

(type + Demo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)
```

```output
loose 3
held 0
held 3
```

### A tracked read of each item of a pipeline that a call in an attribute works out is followed: the attribute is given again as an item changes.

```brix
(import /core/tests/support/markup-fixtures)
(import /stream/aggregate)
(blank)

(main
  (let source (call Source))
  (<> Counter (id: c)
    (= value (call count (in each (. source #items) (when (. each #is-pending)))))
  )
  (blank)
  (print (. c value))
  (= (. ([] (. source items) 0) is-pending) false)
  (print (. c value))
  (= (. ([] (. source items) 1) is-pending) false)
  (print (. c value))
  (let added (call Item))
  (+= (. source items) added)
  (print (. c value))
  (= (. added is-pending) false)
  (print (. c value))
  (= (. ([] (. source items) 0) is-pending) true)
  (print (. c value))
)

(type + Item
  (prop + is-pending _ (= true))
)

(type + Source
  (prop + items _ (= (list (call Item) (call Item))))
)

(type + Counter (is Box)
  (prop + value _ (= 0))
)
```

```output
2
1
0
1
0
1
```

### A closure in a function's markup reads a named element the function makes after it, a child of its own element included, when it runs. So does a task the closure starts.

```brix
(main
  (call run)
)

(fn-do + run _
  (<> Box (id: tool)
    (= run (=> () :do (print (str "tool reads " (. seed text)))))
    (<> Box (id: seed)
      (= text "seed")
    )
    (<> Box (id: go)
      (= run
        (=> () :do
          (block
            (do-new
              (print (str "task reads " (. note text)))
            )
          )
        )
      )
    )
    (<> Box (id: note)
      (= text "note")
    )
  )
  (call (. tool fire))
  (call (. go fire))
)

(type + Box
  (prop + run (? Doer))
  (prop + text _ (= "box"))
  (fn-do + (infix this += (param child Box)) _)
  (fn-do + fire _
    (let callback run)
    (if callback
      (call callback)
    )
  )
)
```

```output
tool reads seed
task reads note
```

### Two closures in a function's markup read each other's elements, and a closure reads the element it is an attribute of.

```brix
(main
  (call run)
)

(fn-do + run _
  (<> Box
    (<> Box (id: yes)
      (= text "yes")
      (= run (=> () :do (print (str "yes reads " (. no text)))))
    )
    (<> Box (id: no)
      (= text "no")
      (= run (=> () :do (print (str "no reads " (. yes text) " and " (. no text)))))
    )
  )
  (call (. yes fire))
  (call (. no fire))
)

(type + Box
  (prop + run (? Doer))
  (prop + text _ (= "box"))
  (fn-do + (infix this += (param child Box)) _)
  (fn-do + fire _
    (let callback run)
    (if callback
      (call callback)
    )
  )
)
```

```output
yes reads no
no reads yes and no
```

### A closure in a function's markup that runs before the element it reads is made panics.

```brix crash
(main
  (call run)
)

(fn-do + run _
  (<> Box
    (<> Box (id: first)
      (= run (=> () :do (print (str "first reads " (. second text)))))
    )
    (<> Box (id: middle)
      (= text (call call-first first))
    )
    (<> Box (id: second)
      (= text "second")
    )
  )
)

(fn-do + call-first String
  (param box Box)
  (call (. box fire))
  (return "middle")
)

(type + Box
  (prop + run (? Doer))
  (prop + text _ (= "box"))
  (fn-do + (infix this += (param child Box)) _)
  (fn-do + fire _
    (let callback run)
    (if callback
      (call callback)
    )
  )
)
```

```stderr
Unhandled panic: Named markup element "second" is read before its markup constructs it
<no file:9>

Task 1

Stack trace:
  0  __source__:run no file:9:55
  1  __source__:runCasting no file:9:7
  2  __source__:Box:fire no file:33:7
  3  __source__:call-first no file:22:3
  4  __source__:run no file:12:15
  5  __source__:main no file:3:3
  6  __source__:init.__source__ no file
  7  __source__:brix.main no file
```

### A closure in class content that runs before the element it reads is made panics.

```brix crash
(main
  (let panel (call Panel))
)

(type + Panel (is Box)
  (content
    (<> This
      (<> Box (id: first)
        (= run (=> () :do (print (str "first reads " (. second text)))))
      )
      (<> Box (id: middle)
        (= text (call call-first first))
      )
      (<> Box (id: second)
        (= text "second")
      )
    )
  )
)

(fn-do + call-first String
  (param box Box)
  (call (. box fire))
  (return "middle")
)

(type + Box
  (prop + run (? Doer))
  (prop + text _ (= "box"))
  (fn-do + (infix this += (param child Box)) _)
  (fn-do + fire _
    (let callback run)
    (if callback
      (call callback)
    )
  )
)
```

```stderr
Unhandled panic: Named markup element "second" is read before its markup constructs it
<no file:10>

Task 1

Stack trace:
  0  __source__:run no file:10:57
  1  __source__:runCasting no file:10:9
  2  __source__:Box:fire no file:35:7
  3  __source__:call-first no file:24:3
  4  __source__:content no file:13:17
  5  __source__:Panel:Panel no file:6:1
  6  __source__:main no file:3:14
  7  __source__:init.__source__ no file
  8  __source__:brix.main no file
```

### A task a function starts before its markup cannot read an element the markup makes, as the task keeps what it reads when it starts.

```brix compile-error
(main
  (call run)
)

(fn-do + run _
  (do-new
    (print (. note text))
  )
  (<> Box
    (<> Box (id: note)
      (= text "note")
    )
  )
)

(type + Box
  (prop + run (? Doer))
  (prop + text _ (= "box"))
  (fn-do + (infix this += (param child Box)) _)
  (fn-do + fire _
    (let callback run)
    (if callback
      (call callback)
    )
  )
)
```

```output
Exception: Named markup element "note" is read in a task started before its markup constructs it, and a task keeps what it reads as it starts; start the task after the markup
<no file:8>
        (print (. note text))
                  ^^^^
```

### A closure that reads an element made after it needs the element's type arguments written.

```brix compile-error
(main
  (call run)
)

(fn-do + run _
  (<> Box
    (<> Box (id: go)
      (= run (=> () :do (print (str "go reads " (. held value)))))
    )
    (<> Holder (id: held) (value: 3))
  )
  (call (. go fire))
)

(type + Box
  (prop + run (? Doer))
  (prop + text _ (= "box"))
  (fn-do + (infix this += (param child Box)) _)
  (fn-do + fire _
    (let callback run)
    (if callback
      (call callback)
    )
  )
)

(type + (of Holder T) (is Box)
  (prop + value T)
  (fn-do + This _
    (param value T)
  )
)
```

```output
Exception: Named markup element "held" is read in a closure made before its markup constructs it, so it needs its type arguments written after "Holder"
<no file:9>
          (= run (=> () :do (print (str "go reads " (. held value)))))
                                                       ^^^^
```

### A binding with no batch root stores back as the element changes, before the statement that changed it goes on. Its place may be a local.

```brix
(main
  (let level 5)
  (<> Knob (id: knob)
    (<-> value level)
  )
  (print (str level " " (. knob value)))
  (call (. knob turn) 3)
  (print (str level " " (. knob value)))
  (= level 7)
  (print (str level " " (. knob value)))
)

(type + Knob
  (prop + value _ (= 0))
  (fn-do + turn _
    (param next Int)
    (= value next)
  )
)
```

```output
5 5
3 3
7 7
```

### A binding's place may be a member of another object. A binding converts each way, and where the element holds a number the conversion loses something of, the element is given back what the place holds.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let settings (call Settings))
  (let panel (call Panel settings))
  (print (str (. settings volume) " " (. panel dial value) " " (. panel steps n) " " (. panel knob value)))
  (call (. panel dial turn) 3)
  (call (. panel knob turn) 2.6)
  (print (str (. settings volume) " " (. panel dial value) " " (. panel steps n) " " (. panel knob value)))
  (call (. panel knob turn) 2.2)
  (print (str (. settings volume) " " (. panel dial value) " " (. panel steps n) " " (. panel knob value)))
  (= (. settings volume) 9)
  (print (str (. settings volume) " " (. panel dial value) " " (. panel steps n) " " (. panel knob value)))
)

(type + Settings
  (prop + volume _ (= 5))
)

(type + Steps (is (tuple (: n Int)))
  (fn-to-expr + F64
    (as n F64)
  )
  (fn-from-expr + (: value F64)
    (call Steps (as value Int))
  )
)

(type + Dial (is Box)
  (prop + value _ (= 0))
  (fn-do + turn _
    (param next Int)
    (= value next)
  )
)

(type + Knob (is Box)
  (prop + value _ (= 0.0))
  (fn-do + turn _
    (param next F64)
    (= value next)
  )
)

(type + Panel (is Box)
  (prop + settings Settings)
  (prop + steps _ (= (call Steps 1)))
  (fn-do + This _
    (param settings _)
  )
  (content
    (<> This
      (<> Dial (id: dial)
        (<-> value (. settings volume))
      )
      (<> Knob (id: knob)
        (<-> value steps)
      )
    )
  )
)
```

```output
5 5 1 1
3 3 2 2
3 3 2 2
9 9 2 2
```

### A binding in a row of a keyed loop binds that row's item, also after the rows move.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let list (call Mixer))
  (print (call (. list state)))
  (call (. (call (. list knob) 1) turn) 9)
  (print (call (. list state)))
  (= (. list tracks) (call (. list tracks reversed)))
  (print (call (. list state)))
  (call (. (call (. list knob) 0) turn) 4)
  (print (call (. list state)))
  (= (. ([] (. list tracks) 1) level) 6)
  (print (call (. list state)))
)

(type + Track
  (prop + name String)
  (prop + level _ (= 0))
  (fn-do + This _
    (param name _)
    (param level _)
  )
)

(type + Knob (is Box)
  (prop + value _ (= 0))
  (fn-do + turn _
    (param next Int)
    (= value next)
  )
)

(type + Mixer (is Box)
  (prop + tracks _ (= (list (call Track "a" 1) (call Track "b" 2))))
  (fn-do + knob Knob
    (param index Int)
    (return (as (!! (call (. (!! %first-child) child-at) index)) Knob))
  )
  (fn-do + state String
    (let parts (list String))
    (for (in track tracks)
      (+= parts (str (. track name) "=" (. track level)))
    )
    (for (in index (..< 0 2))
      (+= parts (str "k" (. (call knob index) value)))
    )
    (return (call (. " " join) parts))
  )
  (content
    (<> This
      (for (in track #tracks (keyed-by (. track name)))
        (<> Knob
          (<-> value (. track level))
        )
      )
    )
  )
)
```

```output
a=1 b=2 k1 k2
a=1 b=9 k1 k9
b=9 a=1 k9 k1
b=4 a=1 k4 k1
b=4 a=6 k4 k6
```

### A non-optional place bound to an optional property stores back what the property holds, and nothing where it holds nothing.

```brix
(main
  (let miles false)
  (<> Picker (id: picker)
    (<-> selected miles)
  )
  (print (str miles " " (. picker selected)))
  (call (. picker pick) true)
  (print (str miles " " (. picker selected)))
  (call (. picker clear))
  (print (str miles " " (. picker selected)))
  (= miles false)
  (print (str miles " " (. picker selected)))
)

(type + Picker
  (prop + selected (? Bool))
  (fn-do + pick _
    (param next Bool)
    (= selected next)
  )
  (fn-do + clear _
    (= selected null)
  )
)
```

```output
false false
true true
true unknown
false false
```

### An optional property may bind a place that is a member of another object, and an optional place still takes nothing.

```brix
(main
  (let settings (call Settings))
  (let maybe (? Int) 4)
  (<> Chooser (id: first)
    (<-> chosen (. settings size))
  )
  (<> Chooser (id: second)
    (<-> chosen maybe)
  )
  (call (. first choose) 7)
  (call (. second choose) 8)
  (print (str (. settings size) " " maybe))
  (call (. first clear))
  (call (. second clear))
  (print (str (. settings size) " " maybe))
  (= (. settings size) 2)
  (print (str (. settings size) " " (. first chosen)))
)

(type + Settings
  (prop + size _ (= 3))
)

(type + Chooser
  (prop + chosen (? Int))
  (fn-do + choose _
    (param next Int)
    (= chosen next)
  )
  (fn-do + clear _
    (= chosen null)
  )
)
```

```output
7 8
7 unknown
2 2
```

### A binding written outside a markup element's body is reported.

```brix crash
(main
  (let a 1)
  (let b 2)
  (<-> a b)
  (print a)
)
```

```output
Exception: (<-> …) cannot be a `row` of (main …), which takes Step. This body runs steps, and each row is a step: a statement form, a `call`, markup, or a value that `pipe` stages after it work on. To return a value, write `(return value)`. If the body is only a value, write `fn-expr`, or remove `:do` from a closure.
<no file:5>
      (<-> a b)
      ^
```

### A root <This> cannot bind, since its attributes give the object's own properties their first values.

```brix crash
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let p (call Panel))
  (print (. p level))
)

(type + Panel (is Box)
  (prop + level _ (= 1))
  (prop + other _ (= 2))
  (content
    (<> This
      (<-> level other)
    )
  )
)
```

```output
Exception: A root <This> cannot bind with "<->", since its attributes give the object's own properties their first values; bind the property where the object is made
<no file:16>
          (<-> level other)
          ^^^^^^^^^^^^^^^^^
```

### A binding follows a property the element stores, so one a getter answers is reported, and so is a name of no property, as a parameter of the constructor alone is.

```brix crash
(main
  (let level 1)
  (<> Knob (id: knob)
    (<-> shown level)
  )
)

(type + Knob
  (prop + stored _ (= 0))
  (fn-expr + shown Int :get
    stored
  )
)
```

```output
Exception: "<->" binds a property the element stores, and Knob answers "shown" with a getter or a setter, which a binding cannot follow; bind a property Knob stores
<no file:5>
        (<-> shown level)
             ^^^^^
```

```brix crash
(main
  (let level 1)
  (<> Knob (id: knob)
    (<-> start level)
  )
)

(type + Knob
  (prop + value _ (= 0))
  (fn-do + This _
    (param start Int)
    (= value start)
  )
)
```

```output
Exception: "<->" binds a property of the element, and Knob has no property "start"
<no file:5>
        (<-> start level)
             ^^^^^
```

### A binding converts each way on its own, and a value that converts only when asked is reported where the binding is written, or where it names the property for the way back.

```brix crash
(main
  (let level 1)
  (<> Knob (id: knob)
    (<-> value level)
  )
)

(type + Knob
  (prop + value _ (= 0.0))
)
```

```output
Exception: "<->" needs each side to become the other on its own, and I64 becomes F64 only when asked; bind a place of the property's type
<no file:5>
        (<-> value level)
        ^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let level 1i32)
  (<> Knob (id: knob)
    (<-> value level)
  )
)

(type + Knob
  (prop + value _ (= 0))
)
```

```output
Exception: "<->" needs each side to become the other on its own, and I64 becomes I32 only when asked; bind a place of the property's type
<no file:5>
        (<-> value level)
             ^^^^^
```

### A slot outlet cannot bind, since what fills the slot is made elsewhere.

```brix crash
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let p (call Panel))
)

(type + Panel (is Box)
  (prop + caption _ (= "a"))
  (content
    (+<> TextBox (id: footer)
      (<-> text caption)
    )
  )
)
```

```output
Exception: A slot outlet cannot bind with "<->", since what fills the slot is made elsewhere; bind the property in the element that fills it
<no file:14>
          (<-> text caption)
          ^^^^^^^^^^^^^^^^^^
```

### A handler in the body of a markup element reads and assigns the names around the element, though the element has members of those names, and reads the element through its name.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
  (<- (. panel button press) 1)
  (print (str (. panel count) " " (. panel button count)))
)

(type + Panel (is Box)
  (prop + count _ (= 7))
  (content
    (<> This
      (<> Button (id: button)
        (for (on event press)
          (= count (+ count (. button count)))
        )
      )
    )
  )
)

(type + Button (is Box)
  (prop + count _ (= 3))
  (event + press Int)
)
```

```output
10 3
```
