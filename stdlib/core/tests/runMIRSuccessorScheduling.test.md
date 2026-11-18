# Mirsuccessor scheduling

### The optional alternative reaches a phi through nested successors and must preserve its checksum.

```yumi
-- parse-optional-alternative("foo.net")

━━━━━━━━━━━━━━━━━━━━
+ parse-optional-alternative
    - source String

match source
    ~ domain [a-z]+ "." tld ("com" | "net" | "org")?
        "domain: $domain, tld: $tld"
    ?! Panic("optional alternative did not parse")
```

```output
domain: foo, tld: net
```

### Property tracking may move an optional-assignment continuation before iterative MIR scheduling.

```yumi
. target = TrackedTarget()
. controller = TrackingController()
controller.apply(target)
. observer = TrackedObserver(target)
-- observer.mirror

━━━━━━━━━━━━━━━━━━━━
+ TrackedTarget

+ value = false

━━━━━━━━━━━━━━━━━━━━
+ TrackedObserver

+ target TrackedTarget
+ mirror = target.#value

────────────────────
+ This
    - target

━━━━━━━━━━━━━━━━━━━━
+ TrackingController

+ target TrackedTarget?

────────────────────
+ do apply
    - next TrackedTarget?

if next != target
    target?.value = false
    target = next
    next?.value = true
```

```output
true
```
