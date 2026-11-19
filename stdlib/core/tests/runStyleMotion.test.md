# Style motion

### A style property keyed with a dot is a property of its own. Class rules, `when` blocks and style sheet rules set it by its keyed name, and a rule written in a rule reaches generated content with it.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Dial (id: dial) (• adorned))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot dial))
  (= (. dial is-on) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot dial))
  (print (call style-computed-snapshot (!! (. (!! (. dial %first-child)) %first-child))))
)

(type + TintStyle (is Style)
  (prop + tone I64 (= 0))
  (prop + motion.tone I64 (= 0))
  (prop + motion.shade I64 (= 0))
  (blank)
  (style-domains tone (domain paint))
)

(style AppTheme
  (using TintStyle)
  (prop + app-speed I64 (= 5))
  (blank)
  (rule Dial
    (= (. motion shade) app-speed)
  )
  (blank)
  (rule adorned
    (rule host
      (<> Mark (slot: first-child) (• mark))
    )
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

(type + Dial (is Box)
  (using TintStyle)
  (using AppTheme)
  (prop + is-on _ (= false))
  (blank)
  (rule This
    (= (. motion tone) 2)
    (blank)
    (rule mark
      (= (. motion tone) 7)
    )
    (blank)
    (when-style #is-on
      (= (. motion tone) 3)
    )
  )
  (content
    (<> This
      (<> Box (• host))
    )
  )
)

(type + Mark (is Box)
  (using TintStyle)
)
```

```output
node TintStyle seg=owned gen=1 mask=6 tone=0 motion.tone=2 motion.shade=5
node TintStyle seg=owned gen=2 mask=2 tone=0 motion.tone=3 motion.shade=5
node TintStyle seg=owned gen=1 mask=2 tone=0 motion.tone=7 motion.shade=0
```

### Only a style declares a property keyed with a dot.

```brix crash
(main
  (let thing (call Thing))
  (print (. thing size))
)

(type + Thing
  (prop + size I64 (= 1))
  (prop + motion.size I64 (= 2))
)
```

```output
Exception: Only a Style may declare a keyed property such as "motion.size"
<no file:9>
      (prop + motion.size I64 (= 2))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Settling a mounted node reports each changed value to the motion host with its old and new bits, floats flagged as floats and objects as objects. A value the host presents is what `$property` reads until it unpresents it, and descendants that inherit the property follow it.

```brix
(import /core/animatable)
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let host (call Presenter))
  (call style-set-motion-host host)
  (blank)
  (<> Demo (id: demo)
    (<> Dial (id: dial)
      (<> Inner (id: inner))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "mounted: fade " (. dial $fade) " spot " (. dial $spot) " ink " (. dial $ink) " inner ink " (. inner $ink)))
  (blank)
  (= (. dial fade) 0.25)
  (= (. dial spot) (call Spot 4 8))
  (= (. dial ink) 9)
  (= (. dial helper) (call Helper 3))
  (call (. demo markup-batch-queue checkpoint))
  (print (str "presented: fade " (. dial $fade) " spot " (. dial $spot) " ink " (. dial $ink) " inner ink " (. inner $ink)))
  (print (str "helper " (?. (call (of style-read-object Helper) dial (!! (. host helper-info)) (. host helper-offset)) value)))
  (blank)
  (call (. host finish) dial)
  (print (str "settled: fade " (. dial $fade) " spot " (. dial $spot) " ink " (. dial $ink) " inner ink " (. inner $ink) " presenting " (call style-is-presenting dial) " " (call style-is-presenting inner)))
)

(type + Demo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Spot (is Tuple)
  (implements Animatable)
  (prop + x F64)
  (prop + y F64)
  (fn-do + This _
    (param vector (list F64))
    (return (call This ([] vector 0) ([] vector 1)))
  )
  (fn-expr + motion-vector (list F64) :get
    (list x y)
  )
)

(type + Helper
  (prop + value I64)
  (fn-do + This _
    (param value _)
  )
)

(type + DialStyle (is Style)
  (prop + fade F64 (= 1))
  (prop + spot _ (= (call Spot 0 0)))
  (prop + helper (? Helper) (= null))
  (blank)
  (style-domains fade (domain paint))
  (style-domains spot (domain paint))
  (style-domains helper (domain paint))
)

(type + InkStyle (is Style)
  (prop + ink I64 @inherited (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type + Dial (is Box)
  (using DialStyle)
  (using InkStyle)
)

(type + Inner (is Box)
  (using InkStyle)
)

(type + Presenter
  (implements StyleMotionHost)
  (prop + helper-info (? StyleInfo))
  (prop + helper-offset Size (= 0))
  (blank)
  (prop _ shown _ (= (call (of List StyleValueChange))))
  (fn-do + finish _
    (param owner MarkupElement)
    (for (in change shown)
      (call style-unpresent owner (. change info) (. change offset))
    )
  )
  (fn-do + style-values-changed _
    (param owner MarkupElement)
    (param changes (list StyleValueChange))
    (for (in change changes)
      (let property ([] (. change info properties) (. change offset)))
      (let from (?? (call style-motion-vector property (. change old-bits) (. change old-none)) (call style-motion-none-vector property (. change bits))))
      (let to (call style-motion-vector property (. change bits) (. change none)))
      (print (str (. property name) ": float " (. property is-float) " object " (. property is-object) " from " (call describe from) " to " (call describe to)))
      (if (. property is-object)
        (= helper-info (. change info))
        (= helper-offset (. change offset))
      )
      (if (and from to)
        (let middle (call (of List F64)))
        (for (in index (..< 0 (. to length)))
          (+= middle (/ (+ ([] from index) ([] to index)) 2))
        )
        (let presenter (call StylePresenter property (. change bits)))
        (call style-present owner (. change info) (. change offset) (call (. presenter bits) middle) false)
        (+= shown change)
        (else-if (== (. property name) "ink")
          (call style-present owner (. change info) (. change offset) 5 false)
          (+= shown change)
        )
      )
    )
  )
)

(fn-do _ describe String
  (param vector (? (list F64)))
  (return-if "none" (if (not vector)))
  (let text "")
  (for (in value vector)
    (= text (+ text (str " " value)))
  )
  (return text)
)
```

```output
mounted: fade 1 spot (0, 0) ink 0 inner ink 0
fade: float true object false from  1 to  0.25
spot: float false object false from  0 0 to  4 8
helper: float false object true from none to none
ink: float false object false from none to none
presented: fade 0.625 spot (2, 4) ink 5 inner ink 5
helper 3
settled: fade 0.25 spot (4, 8) ink 9 inner ink 9 presenting false false
```

### A presented value is built from its vector by the value it moves to, so what the vector leaves out comes from there. Two values mix only when the first `moves-to` the second.

```brix
(import /core/animatable)
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let host (call Presenter))
  (call style-set-motion-host host)
  (blank)
  (<> Demo (id: demo)
    (<> Dial (id: dial))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (= (. dial glow) (call Glow 1 4 8))
  (call (. demo markup-batch-queue checkpoint))
  (print (str "same kind: " (. dial $glow)))
  (= (. dial glow) (call Glow 2 0 0))
  (call (. demo markup-batch-queue checkpoint))
  (print (str "other kind: " (. dial $glow)))
)

(type + Demo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Glow (is Tuple)
  (implements Animatable)
  (prop + kind I64)
  (prop + x F64)
  (prop + y F64)
  (fn-expr + motion-vector (list F64) :get
    (list x y)
  )
  (fn-expr + with-motion-vector This
    (param vector (list F64))
    (call This kind ([] vector 0) ([] vector 1))
  )
  (fn-expr + moves-to Bool
    (param other This)
    (== kind (. other kind))
  )
)

(type + GlowStyle (is Style)
  (prop + glow _ (= (call Glow 1 0 0)))
  (blank)
  (style-domains glow (domain paint))
)

(type + Dial (is Box)
  (using GlowStyle)
)

(type + Presenter
  (implements StyleMotionHost)
  (fn-do + style-values-changed _
    (param owner MarkupElement)
    (param changes (list StyleValueChange))
    (for (in change changes)
      (let property ([] (. change info properties) (. change offset)))
      (let from (call style-motion-vector property (. change old-bits) false))
      (let to (call style-motion-vector property (. change bits) false))
      (if (and from to (call style-motion-moves property (. change old-bits) (. change bits)))
        (let middle (call (of List F64)))
        (for (in index (..< 0 (. to length)))
          (+= middle (/ (+ ([] from index) ([] to index)) 2))
        )
        (let presenter (call StylePresenter property (. change bits)))
        (call style-present owner (. change info) (. change offset) (call (. presenter bits) middle) false)
        (else
          (call style-unpresent owner (. change info) (. change offset))
        )
      )
    )
  )
)
```

```output
same kind: (1, 2, 4)
other kind: (2, 0, 0)
```

### A presented value keeps alive the value its bits name, as does each descendant that follows it, even after the presenter that made it moves on to another value without presenting again.

```brix
(import /BxGC/BxGC.h)
(import /core/animatable)
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let host (call Presenter))
  (call style-set-motion-host host)
  (blank)
  (<> Demo (id: demo)
    (<> Dial (id: dial)
      (<> Inner (id: inner))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (= (. dial spot) (call Spot 4 8))
  (call (. demo markup-batch-queue checkpoint))
  (call C:brixGCCollect)
  (print (str "presented: " (. dial $spot) " inner " (. inner $spot)))
)

(type + Demo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Spot (is Tuple)
  (implements Animatable)
  (prop + x F64)
  (prop + y F64)
  (fn-do + This _
    (param vector (list F64))
    (return (call This ([] vector 0) ([] vector 1)))
  )
  (fn-expr + motion-vector (list F64) :get
    (list x y)
  )
)

(type + SpotStyle (is Style)
  (prop + spot _ @inherited (= (call Spot 0 0)))
  (blank)
  (style-domains spot (domain paint))
)

(type + Dial (is Box)
  (using SpotStyle)
)

(type + Inner (is Box)
  (using SpotStyle)
)

(type + Presenter
  (implements StyleMotionHost)
  (prop _ presenters _ (= (call (of List StylePresenter))))
  (fn-do + style-values-changed _
    (param owner MarkupElement)
    (param changes (list StyleValueChange))
    (for (in change changes)
      (let property ([] (. change info properties) (. change offset)))
      (let from (call style-motion-vector property (. change old-bits) false))
      (let to (call style-motion-vector property (. change bits) false))
      (if (and from to)
        (let middle (call (of List F64)))
        (for (in index (..< 0 (. to length)))
          (+= middle (/ (+ ([] from index) ([] to index)) 2))
        )
        (let presenter (call StylePresenter property (. change bits)))
        (call style-present owner (. change info) (. change offset) (call (. presenter bits) middle) false)
        (call (. presenter retarget) (. change old-bits))
        (+= presenters presenter)
      )
    )
  )
)
```

```output
presented: (2, 4) inner (2, 4)
```
