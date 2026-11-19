# Mirsuccessor scheduling

### The optional alternative reaches a phi through nested successors and must preserve its checksum.

```brix
(main
  (print (call parse-optional-alternative "foo.net"))
)

(fn-expr + parse-optional-alternative _
  (param source String)
  (match source
    (~ (seq (: domain (+ (chars "a-z"))) "." (: tld (? (| "com" "net" "org"))))
      (str "domain: " domain ", tld: " tld)
    )
    (throw (call Panic "optional alternative did not parse"))
  )
)
```

```output
domain: foo, tld: net
```

### Property tracking may move an optional-assignment continuation before iterative MIR scheduling.

```brix
(main
  (let target (call TrackedTarget))
  (let controller (call TrackingController))
  (call (. controller apply) target)
  (let observer (call TrackedObserver target))
  (print (. observer mirror))
)

(type + TrackedTarget
  (prop + value _ (= false))
)

(type + TrackedObserver
  (prop + target TrackedTarget)
  (prop + mirror _ (= (. target #value)))
  (fn-do + This _
    (param target _)
  )
)

(type + TrackingController
  (prop + target (? TrackedTarget))
  (fn-do + apply _
    (param next (? TrackedTarget))
    (if (!= next target)
      (= (?. target value) false)
      (= target next)
      (= (?. next value) true)
    )
  )
)
```

```output
true
```
