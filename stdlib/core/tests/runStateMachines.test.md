# State machines

### Declaration-bound machines expose a union-valued state initialized to the first untriggered case.

```brix
(prop + auth _
  (machine
    (state idle)
    (state authenticating)
    (state (call authenticated token))
  )
)
(blank)

(main
  (print (is auth .idle))
  (print (is auth .authenticating))
  (print (is auth .authenticated))
)
```

```output
true
false
false
```

### Top-level machines expose generated boolean projections for payloadless cases.

```brix
(main
  (state idle)
  (state ready)
  (blank)
  (print idle)
  (print ready)
)
```

```output
true
false
```

### Re-entering a class machine queues exactly one lowered activation per event publish

```brix
(main
  (let source (call EventReentryMachine))
  (<- (. source enter) true)
  (do-new
    (print (. source entries))
    (blank)
    (<- (. source leave) true)
    (do-new
      (print (is (. source flow) .idle))
      (blank)
      (<- (. source enter) true)
      (do-new
        (print (. source entries))
      )
    )
  )
)

(type + EventReentryMachine
  (event + enter Bool)
  (event + leave Bool)
  (prop + entries _ (= 0))
  (prop + flow _
    (machine
      (state idle)
      (state active (trigger on enter)
        (do
          (+= entries 1)
        )
        (blank)
        (then idle (trigger on leave))
      )
    )
  )
)
```

```output
1
true
2
```

### Top-level machines expose generated projections backed by one hidden mutually exclusive state.

```brix
(prop + submitted (of AsyncChannel (tuple String String)))
(blank)

(main
  (state idle)
  (state (call authenticating email password) (trigger for (tuple email password) submitted))
  (state (call authenticated token))
  (blank)
  (print idle)
  (print authenticating)
  (blank)
  (<- submitted (tuple "joe@example.com" "secret"))
  (blank)
  (do-new
    (print idle)
    (print authenticating)
  )
)
```

```output
true
unknown
false
(joe@example.com, secret)
```

### Immediate if transitions are evaluated when the owning machine enters.

```brix
(prop + has-token _ (= true))
(blank)
(prop + auth _
  (machine
    (state editing)
    (state authenticated (trigger if has-token))
  )
)
(blank)

(main
  (print (is auth .authenticated))
)
```

```output
true
```

### Source order decides between multiple immediate transitions from the same source.

```brix
(prop + allow-admin _ (= true))
(prop + allow-user _ (= true))
(blank)
(prop + auth _
  (machine
    (state guest)
    (state admin (trigger if allow-admin))
    (state user (trigger if allow-user))
  )
)
(blank)

(main
  (print (is auth .admin))
  (print (is auth .user))
)
```

```output
true
false
```

### Immediate transitions exit the initial state and run entry/finally actions in order.

```brix
(prop + has-token _ (= true))
(blank)
(prop + auth _
  (machine
    (state editing
      (do
        (print "enter editing")
      )
      (blank)
      (finally
        (print "exit editing")
      )
    )
    (blank)
    (state authenticated (trigger if has-token)
      (do
        (print "enter authenticated")
      )
    )
  )
)
(blank)

(main
  (print (is auth .authenticated))
)
```

```output
enter editing
exit editing
enter authenticated
true
```

### Immediate structure-match if transitions can select a case on entry.

```brix
(prop + login-state LoginState (= .valid))
(blank)
(prop + auth _
  (machine
    (state editing)
    (state authenticating (trigger if-match .valid login-state))
  )
)
(blank)

(main
  (print (is auth .authenticating))
)

(type + LoginState (is Union)
  (variant valid)
  (variant invalid)
)
```

```output
true
```

### Immediate fallbacks clear their target when no immediate transition matches.

```brix
(prop + has-token _ (= false))
(blank)

(main
  (state authenticating
    (state authenticated (trigger if has-token))
    (else-state authenticating)
  )
  (blank)
  (print authenticating)
  (print authenticated)
)
```

```output
false
false
```

### Self-targeting immediate fallbacks exit once and do not re-enter the same state.

```brix
(prop + should-finish _ (= false))
(blank)

(main
  (state loading
    (do
      (print "enter loading")
    )
    (blank)
    (finally
      (print "exit loading")
    )
    (blank)
    (state finished (trigger if should-finish))
    (else-state loading)
  )
  (blank)
  (print "after fallback")
  (print loading)
  (print finished)
)
```

```output
enter loading
exit loading
after fallback
false
false
```

### Immediate fallbacks after then continuations clear their target when no continuation matches.

```brix
(prop + is-authentic _ (= false))
(blank)

(main
  (state authenticating
    (then authenticated (trigger if is-authentic))
    (else-state authenticating)
  )
  (blank)
  (state authenticated)
  (blank)
  (print authenticating)
  (print authenticated)
)
```

```output
false
false
```

### Immediate fallbacks exit an active target subtree before clearing the target.

```brix
(prop + is-done _ (= false))
(blank)

(main
  (state loading
    (finally
      (print "exit loading")
    )
    (blank)
    (state spinner
      (finally
        (print "exit spinner")
      )
    )
    (blank)
    (state finished (trigger if is-done))
    (else-state loading)
  )
  (blank)
  (print loading)
  (print spinner)
  (print finished)
)
```

```output
exit spinner
exit loading
false
false
false
```

### Rejects duplicate state case declarations before lowering.

```brix crash
(prop + auth _
  (machine
    (state idle)
    (state idle)
  )
)
```

```output
Exception: Redefinition of state case 'idle'
<no file:5>
        (state idle)
        ^^^^^^^^^^^^
```

### Event transitions update the active case asynchronously and capture emitted payload data.

```brix
(prop + submitted (of AsyncChannel String))
(blank)
(prop + auth _
  (machine
    (state idle)
    (state (call authenticating email) (trigger for email submitted))
  )
)
(blank)

(main
  (<- submitted "joe@example.com")
  (blank)
  (do-new
    (print (is auth .authenticating))
    (print
      (match auth
        (case (call .authenticating email)
          email
        )
        (default
          "none"
        )
      )
    )
  )
)
```

```output
true
joe@example.com
```

### for-pattern transitions destructure union payloads and use captures in target tuple data.

```brix
(prop + login-submitted (of AsyncChannel LoginSubmission))
(blank)
(prop + auth _
  (machine
    (state idle)
    (state (call authenticating email password) (trigger for (call .valid email password) login-submitted))
    (state invalid-email (trigger for .invalid-email login-submitted))
    (state invalid-password (trigger for .invalid-password login-submitted))
  )
)
(blank)

(main
  (<- login-submitted .invalid-email)
  (blank)
  (do-new
    (print (is auth .invalid-email))
    (blank)
    (<- login-submitted (call .valid "joe@example.com" "secret"))
    (do-new
      (print
        (match auth
          (case (call .authenticating email password)
            (str email "/" password)
          )
          (default
            "none"
          )
        )
      )
    )
  )
)

(type + LoginSubmission (is Union)
  (variant invalid-email)
  (variant invalid-password)
  (variant valid (tuple String String))
)
```

```output
true
joe@example.com/secret
```

### when guards filter event transitions using each from the trigger stream.

```brix
(prop + results (of AsyncChannel AuthResult))
(blank)
(prop + auth _
  (machine
    (state authenticating)
    (state authenticated (trigger when-on (is each .authentic) results))
    (state rejected (trigger when-on (is each .incorrect-password) results))
  )
)
(blank)

(main
  (<- results .rate-limited)
  (blank)
  (do-new
    (print (is auth .authenticating))
    (blank)
    (<- results .authentic)
    (do-new
      (print (is auth .authenticated))
    )
  )
)

(type + AuthResult (is Union)
  (variant authentic)
  (variant incorrect-password)
  (variant rate-limited)
)
```

```output
true
true
```

### for-pattern plus when evaluates the pattern before the predicate and exposes captures to both.

```brix
(prop + submitted (of AsyncChannel LoginSubmission))
(blank)
(prop + auth _
  (machine
    (state idle)
    (state (call authenticating email password) (trigger for (call .valid email password) submitted (call (. email find) "@")))
    (state rejected (trigger for (call .valid email password) submitted (not (call (. email find) "@"))))
  )
)
(blank)

(main
  (<- submitted (call .valid "bad" "secret"))
  (blank)
  (do-new
    (print (is auth .rejected))
    (blank)
    (<- submitted (call .valid "joe@example.com" "secret"))
    (do-new
      (print
        (match auth
          (case (call .authenticating email password)
            (str email "/" password)
          )
          (default
            "none"
          )
        )
      )
    )
  )
)

(type + LoginSubmission (is Union)
  (variant valid (tuple String String))
)
```

```output
true
joe@example.com/secret
```

### Source order decides between multiple sibling transitions triggered by the same event.

```brix
(prop + submitted (of AsyncChannel Int))
(blank)
(prop + flow _
  (machine
    (state idle)
    (state first (trigger on submitted (> each 0)))
    (state second (trigger on submitted (> each 0)))
  )
)
(blank)

(main
  (<- submitted 1)
  (blank)
  (do-new
    (print (is flow .first))
    (print (is flow .second))
  )
)
```

```output
true
false
```

### A fallback belongs to its transition decision group and clears its target when no earlier case matches.

```brix
(prop + results (of AsyncChannel AuthResult))
(blank)

(main
  (state authenticating
    (then authenticated (trigger for .authentic results))
    (then rejected (trigger for .incorrect-password results))
    (else-state authenticating)
  )
  (blank)
  (state authenticated)
  (state rejected)
  (blank)
  (<- results .rate-limited)
  (blank)
  (do-new
    (print authenticating)
    (print authenticated)
    (print rejected)
  )
)

(type + AuthResult (is Union)
  (variant authentic)
  (variant incorrect-password)
  (variant rate-limited)
)
```

```output
false
false
false
```

### then continuations can consume streams declared in the owning state block.

```brix
(prop + submitted (of AsyncChannel (tuple String String)))
(prop + auth-results (of AsyncChannel AuthResult))
(prop + direct-results (of AsyncChannel String))
(blank)
(prop + auth _
  (machine
    (state idle)
    (state (call authenticating email password) (trigger for (tuple email password) submitted)
      (then (call authenticated token) (trigger for (call .authentic token) auth-result)
        (do
          (print (str "continued " token))
        )
      )
      (blank)
      (then rejected (trigger for .incorrect-password auth-result))
      (blank)
      (lazy auth-result auth-results)
      (blank)
      (finally
        (print "left authenticating")
      )
    )
    (blank)
    (state (call authenticated value) (trigger for value direct-results)
      (do
        (print (str "entered " value))
      )
    )
  )
)
(blank)

(main
  (<- submitted (tuple "joe@example.com" "secret"))
  (do-new
    (<- auth-results (call .authentic "token-123"))
    (do-new
      (print
        (match auth
          (case (call .authenticated token)
            token
          )
          (default
            "none"
          )
        )
      )
    )
  )
)

(type + AuthResult (is Union)
  (variant authentic (tuple String))
  (variant incorrect-password)
)
```

```output
left authenticating
continued token-123
entered token-123
token-123
```

### then continuations can use captures from a source state without payload data.

```brix
(prop + submitted (of AsyncChannel LoginSubmission))
(prop + results (of AsyncChannel AuthResult))
(prop + selected _ (= ""))
(blank)
(prop + auth _
  (machine
    (state idle)
    (state authenticating (trigger for (call .valid email password) submitted)
      (then (call authenticated email password token) (trigger for (call .authentic token) auth-result))
      (blank)
      (lazy pair (str email "/" password))
      (lazy auth-result results)
      (blank)
      (do
        (= selected pair)
      )
    )
  )
)
(blank)

(main
  (<- submitted (call .valid "joe@example.com" "secret"))
  (do-new
    (print selected)
    (blank)
    (<- results (call .authentic "token-123"))
    (do-new
      (print
        (match auth
          (case (call .authenticated email password token)
            (str email "/" password "/" token)
          )
          (default
            "none"
          )
        )
      )
    )
  )
)

(type + LoginSubmission (is Union)
  (variant valid (tuple String String))
)

(type + AuthResult (is Union)
  (variant authentic (tuple String))
)
```

```output
joe@example.com/secret
joe@example.com/secret/token-123
```

### then continuation captures without payload data can drive nested transition sources.

```brix
(prop + submitted (of AsyncChannel LoginSubmission))
(prop + results (of AsyncChannel AuthResult))
(blank)
(prop + auth _
  (machine
    (state idle)
    (state authenticating (trigger for (call .valid email password) submitted)
      (then authenticated (trigger for (call .authentic token) auth-result)
        (then finished (trigger for char token))
      )
      (blank)
      (lazy auth-result results)
    )
  )
)
(blank)

(main
  (<- submitted (call .valid "joe@example.com" "secret"))
  (do-new
    (<- results (call .authentic "token-123"))
    (do-new
      (print (is auth .finished))
    )
  )
)

(type + LoginSubmission (is Union)
  (variant valid (tuple String String))
)

(type + AuthResult (is Union)
  (variant authentic (tuple String))
)
```

```output
true
```

### Events from a source state after that state exits are ignored by that state's continuations.

```brix
(prop + submitted (of AsyncChannel (tuple String String)))
(prop + cancelled (of AsyncChannel Bool))
(prop + auth-results (of AsyncChannel AuthResult))
(blank)
(prop + auth _
  (machine
    (state idle)
    (state (call authenticating email password) (trigger for (tuple email password) submitted)
      (then (call authenticated token) (trigger for (call .authentic token) auth-result))
      (then idle (trigger on cancelled))
      (blank)
      (lazy auth-result auth-results)
    )
  )
)
(blank)

(main
  (<- submitted (tuple "joe@example.com" "secret"))
  (do-new
    (<- cancelled true)
    (<- auth-results (call .authentic "stale-token"))
    (do-new
      (print (is auth .idle))
      (print (is auth .authenticated))
    )
  )
)

(type + AuthResult (is Union)
  (variant authentic (tuple String))
)
```

```output
true
false
```

### State-local broadcastable sources detach from the exact source, and stale activation callbacks cannot transition a later activation.

```brix
(prop + unique-open (of AsyncChannel Bool))
(prop + unique-close (of AsyncChannel Bool))
(prop _ unique-source-factory _ (= (call UniqueSourceFactory)))
(blank)
(prop + unique-flow _
  (machine
    (state unique-idle)
    (state unique-active (trigger on unique-open)
      (then unique-finished (trigger on source))
      (then unique-idle (trigger on unique-close))
      (blank)
      (lazy source (call (. unique-source-factory make)))
    )
    (blank)
    (state unique-finished)
  )
)
(blank)
(prop + shared-open (of AsyncChannel Bool))
(prop + shared-close (of AsyncChannel Bool))
(prop _ shared-source-factory _ (= (call SharedSourceFactory)))
(blank)
(prop + shared-flow _
  (machine
    (state shared-idle)
    (state shared-active (trigger on shared-open)
      (then shared-finished (trigger on source))
      (then shared-idle (trigger on shared-close))
      (blank)
      (lazy source (call (. shared-source-factory get)))
    )
    (blank)
    (state shared-finished)
  )
)
(blank)

(main
  (<- unique-open true)
  (do-new
    (<- unique-close true)
    (do-new
      (<- unique-open true)
      (do-new
        (print (. unique-source-factory sources length))
        (print (. ([] (. unique-source-factory sources) 0) subscription-count))
        (print (. ([] (. unique-source-factory sources) 0) unsubscription-count))
        (print (. ([] (. unique-source-factory sources) 0) subscriber-count))
        (print (. ([] (. unique-source-factory sources) 1) subscriber-count))
        (blank)
        (call (. ([] (. unique-source-factory sources) 0) emit-current) true)
        (do-new
          (print (is unique-flow .unique-active))
          (blank)
          (call (. ([] (. unique-source-factory sources) 1) emit-current) true)
          (do-new
            (print (is unique-flow .unique-finished))
            (print (. ([] (. unique-source-factory sources) 1) unsubscription-count))
            (print (. ([] (. unique-source-factory sources) 1) subscriber-count))
            (blank)
            (<- shared-open true)
            (do-new
              (<- shared-close true)
              (do-new
                (<- shared-open true)
                (do-new
                  (print (. shared-source-factory calls))
                  (print (. shared-source-factory source subscription-count))
                  (print (. shared-source-factory source unsubscription-count))
                  (blank)
                  (call (. shared-source-factory source emit-stale) true)
                  (do-new
                    (print (is shared-flow .shared-active))
                    (blank)
                    (call (. shared-source-factory source emit-current) true)
                    (do-new
                      (print (is shared-flow .shared-finished))
                      (print (. shared-source-factory source unsubscription-count))
                      (print (. shared-source-factory source subscriber-count))
                    )
                  )
                )
              )
            )
          )
        )
      )
    )
  )
)

(type + UniqueSourceFactory
  (prop + sources (list TrackingSource))
  (fn-do + make TrackingSource
    (let source (call TrackingSource))
    (+= sources source)
    (return source)
  )
)

(type + SharedSourceFactory
  (prop + source _ (= (call TrackingSource)))
  (prop + calls _ (= 0))
  (fn-do + get TrackingSource
    (+= calls 1)
    (return source)
  )
)

(type + TrackingSource
  (implements (of Broadcastable Bool))
  (prop + subscription-count _ (= 0))
  (prop + unsubscription-count _ (= 0))
  (blank)
  (prop _ subscribers (list (of Subscriber Bool)))
  (prop _ detached-subscribers (list (of Subscriber Bool)))
  (fn-expr + subscriber-count Size :get
    (. subscribers length)
  )
  (fn-expr + is-closed Bool :get
    false
  )
  (fn-do + subscribe _ (?! Error)
    (param subscriber (of Subscriber Bool))
    (+= subscription-count 1)
    (+= subscribers subscriber)
  )
  (fn-do + unsubscribe _ (?! Error)
    (param subscriber (of Subscriber Bool))
    (+= unsubscription-count 1)
    (+= detached-subscribers subscriber)
    (-= subscribers subscriber)
  )
  (fn-do + subscribe _ (?! Error)
    (param subscriber (of Subscribable Bool))
  )
  (fn-do + unsubscribe _ (?! Error)
    (param subscriber (of Subscribable Bool))
  )
  (fn-do + emit-current _
    (param item Bool)
    (if subscribers
      (let subscriber ([] subscribers 0))
      (call subscriber item this)
    )
  )
  (fn-do + emit-stale _
    (param item Bool)
    (if detached-subscribers
      (let subscriber ([] detached-subscribers 0))
      (call subscriber item this)
    )
  )
)
```

```output
2
1
1
0
1
true
true
1
0
2
2
1
true
true
2
0
```

### State entry actions run on entry and do finally actions run when the state exits.

```brix
(prop + submitted (of AsyncChannel Bool))
(prop + results (of AsyncChannel AuthResult))
(blank)
(prop + auth _
  (machine
    (state idle)
    (state authenticating (trigger on submitted)
      (do
        (print "enter authenticating")
      )
      (blank)
      (finally
        (print "exit authenticating")
      )
      (blank)
      (then authenticated (trigger for .authentic results))
    )
    (blank)
    (state authenticated)
  )
)
(blank)

(main
  (<- submitted true)
  (do-new
    (<- results .authentic)
    (do-new
      (print "done")
    )
  )
)

(type + AuthResult (is Union)
  (variant authentic)
)
```

```output
enter authenticating
exit authenticating
done
```

### Actions that can raise compile when transitions lead back into the states that run them.

```brix
(prop + toggles (of AsyncChannel Bool))
(blank)
(prop + mode _
  (machine
    (state first
      (finally
        (call check false)
      )
      (blank)
      (then second (trigger on toggles))
    )
    (blank)
    (state second
      (do
        (print "entered second")
      )
      (blank)
      (then first (trigger on toggles))
    )
  )
)
(blank)

(main
  (<- toggles true)
  (do-new
    (print (is mode .second))
  )
)

(fn-do + check _
  (param fails Bool)
  (if fails
    (throw (call Error "check failed"))
  )
)
```

```output
entered second
true
```

### Re-entering an already active case exits the active case and subtree before entering it again.

```brix
(prop + submitted (of AsyncChannel Int))
(prop + child-events (of AsyncChannel Int))
(blank)
(prop + flow _
  (machine
    (state idle)
    (state (call editing count) (trigger for count submitted)
      (do
        (print (str "enter editing " count))
      )
      (blank)
      (finally
        (print (str "exit editing " count))
      )
      (blank)
      (state child (trigger on child-events)
        (do
          (print "enter child")
        )
        (blank)
        (finally
          (print "exit child")
        )
      )
    )
  )
)
(blank)

(main
  (<- submitted 1)
  (do-new
    (<- child-events 1)
    (do-new
      (<- submitted 2)
      (do-new
        (print
          (match flow
            (case (call .editing count)
              (str "editing " count)
            )
            (default
              "none"
            )
          )
        )
      )
    )
  )
)
```

```output
enter editing 1
enter child
exit child
exit editing 1
enter editing 2
editing 2
```

### Re-entering a state recreates immediate locals and lazy locals for the new state lifetime.

```brix
(prop + submitted (of AsyncChannel Int))
(prop _ counter _ (= 0))
(blank)
(prop + flow _
  (machine
    (state idle)
    (state (call running seed) (trigger for seed submitted)
      (let started-at (call next-counter))
      (lazy memoized (call next-counter))
      (blank)
      (do
        (print (str "enter seed=" seed " started=" started-at " memo=" memoized))
        (print (str "memo again " memoized))
      )
    )
  )
)
(blank)

(main
  (<- submitted 10)
  (do-new
    (<- submitted 20)
    (do-new
      (print (is flow .running))
    )
  )
)

(fn-do + next-counter _
  (+= counter 1)
  (return counter)
)
```

```output
enter seed=10 started=1 memo=2
memo again 2
enter seed=20 started=3 memo=4
memo again 4
true
```

### A transition to an outer state exits the active child subtree.

```brix
(prop + submit (of AsyncChannel Bool))
(prop + cancel (of AsyncChannel Bool))
(prop + begin-child (of AsyncChannel Bool))
(blank)
(prop + flow _
  (machine
    (state idle)
    (state submitted (trigger on submit)
      (do
        (print "enter submitted")
      )
      (blank)
      (finally
        (print "exit submitted")
      )
      (blank)
      (state validating (trigger on begin-child)
        (do
          (print "enter validating")
        )
        (blank)
        (finally
          (print "exit validating")
        )
      )
      (blank)
      (then idle (trigger on cancel))
    )
  )
)
(blank)

(main
  (<- submit true)
  (do-new
    (<- begin-child true)
    (do-new
      (<- cancel true)
      (do-new
        (print (is flow .idle))
      )
    )
  )
)
```

```output
enter submitted
enter validating
exit validating
exit submitted
true
```

### Nested child cases are active together with their parent case.

```brix
(prop + submit (of AsyncChannel Bool))
(prop + begin-child (of AsyncChannel Bool))
(blank)
(prop + flow _
  (machine
    (state idle)
    (state submitted (trigger on submit)
      (state validating (trigger on begin-child))
    )
  )
)
(blank)

(main
  (<- submit true)
  (do-new
    (<- begin-child true)
    (do-new
      (print (is flow .submitted))
      (print (is flow .validating))
      (print (is flow .idle))
    )
  )
)
```

```output
true
true
false
```

### Payload-bearing then continuations compile and retain their source payload in generic classes.

```brix
(main
  (let source (call (of GenericPayloadContinuation String)))
  (= (. source item) "ready")
  (<- (. source submitted) (call PayloadContinuationRequest 7 .keys (:does-complete true)))
  (do-new
    (print (is (. source flow) .awaiting))
    (blank)
    (<- (. source completed) true)
    (do-new
      (print
        (match (. source flow)
          (case (call .finished index cause does-complete)
            (str index "/" cause "/" does-complete)
          )
          (default
            "none"
          )
        )
      )
    )
  )
)

(type + (of GenericPayloadContinuation Value) (is PayloadContinuationBase)
  (prop + item (? Value))
  (event _ submitted PayloadContinuationRequest)
  (event _ completed Bool)
  (prop + flow _
    (machine
      (state idle)
      (state (call awaiting index cause does-complete) (trigger for (tuple index cause does-complete) submitted)
        (then (call finished index cause does-complete) (trigger if false))
        (then (call finished index cause does-complete) (trigger on completed (!= item null)))
      )
      (blank)
      (state (call finished index cause does-complete))
    )
  )
)

(type _ PayloadContinuationRequest (is Tuple)
  (prop + index Index)
  (prop + cause PayloadContinuationCause)
  (prop + does-complete Bool)
)

(type + PayloadContinuationBase)

(type + PayloadContinuationCause (is Union)
  (variant keys)
)
```

```output
true
7/keys/true
```

### Nested named machines are independent state axes scoped to the parent state lifetime.

```brix
(prop + auth-events (of AsyncChannel Bool))
(prop + network-events (of AsyncChannel Bool))
(blank)
(prop + app _
  (machine
    (state ready
      (prop + auth _
        (machine
          (state editing)
          (state authenticated (trigger on auth-events)
            (do
              (print "auth authenticated")
            )
          )
        )
      )
      (blank)
      (prop + network _
        (machine
          (state offline
            (do
              (print "network offline")
            )
          )
          (blank)
          (state online (trigger on network-events)
            (do
              (print "network online")
            )
          )
        )
      )
    )
    (blank)
    (state closed)
  )
)
(blank)

(main
  (<- auth-events true)
  (blank)
  (do-new
    (print (is app .ready))
    (blank)
    (<- network-events true)
    (do-new
      (print (is app .ready))
    )
  )
)
```

```output
network offline
auth authenticated
true
network online
true
```

### Leaving a parent state exits nested named machines and removes their subscriptions.

```brix
(prop + close-events (of AsyncChannel Bool))
(prop + auth-events (of AsyncChannel Bool))
(blank)
(prop + app _
  (machine
    (state ready
      (prop + auth _
        (machine
          (state editing
            (finally
              (print "exit auth editing")
            )
          )
          (blank)
          (state authenticated (trigger on auth-events)
            (do
              (print "enter authenticated")
            )
            (blank)
            (finally
              (print "exit authenticated")
            )
          )
        )
      )
      (blank)
      (then closed (trigger on close-events))
    )
    (blank)
    (state closed)
  )
)
(blank)

(main
  (<- auth-events true)
  (do-new
    (<- close-events true)
    (do-new
      (<- auth-events true)
      (do-new
        (print (is app .closed))
      )
    )
  )
)
```

```output
exit auth editing
enter authenticated
exit authenticated
true
```

### Transitioning back to the same state is reentrant, not an internal transition.

```brix
(prop + refresh (of AsyncChannel Bool))
(blank)
(prop + view _
  (machine
    (state showing
      (do
        (print "enter showing")
      )
      (blank)
      (finally
        (print "exit showing")
      )
      (blank)
      (then showing (trigger on refresh))
    )
  )
)
(blank)

(main
  (<- refresh true)
  (blank)
  (do-new
    (print (is view .showing))
  )
)
```

```output
enter showing
exit showing
enter showing
true
```

### A transition to an outer already-active state is reentrant and resets that outer state subtree.

```brix
(prop + begin-child (of AsyncChannel Bool))
(prop + reset (of AsyncChannel Bool))
(blank)
(prop + flow _
  (machine
    (state editing
      (do
        (print "enter editing")
      )
      (blank)
      (finally
        (print "exit editing")
      )
      (blank)
      (state focused (trigger on begin-child)
        (do
          (print "enter focused")
        )
        (blank)
        (finally
          (print "exit focused")
        )
      )
      (blank)
      (then editing (trigger on reset))
    )
  )
)
(blank)

(main
  (<- begin-child true)
  (do-new
    (<- reset true)
    (do-new
      (print (is flow .editing))
      (print (is flow .focused))
    )
  )
)
```

```output
enter editing
enter focused
exit focused
exit editing
enter editing
true
false
```

### Global declaration-bound state machines remain union-valued and async event driven.

```brix
(prop + submitted (of AsyncChannel String))
(blank)
(prop + auth _
  (machine
    (state idle)
    (state (call authenticating email) (trigger for email submitted))
  )
)
(blank)

(main
  (print (is auth .idle))
  (blank)
  (<- submitted "joe@example.com")
  (do-new
    (print (is auth .authenticating))
    (print
      (match auth
        (case (call .authenticating email)
          email
        )
        (default
          "none"
        )
      )
    )
  )
)
```

```output
true
true
joe@example.com
```

### Class eager property state machines support per-instance storage and markup content.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let first (call Widget "first"))
  (let second (call Widget "second"))
  (let header (call HeaderMarkupMachine))
  (let section (call SectionMarkupMachine))
  (let observed (call ObservedWidget))
  (blank)
  (print (is (. header phase) .idle))
  (print (is (. section phase) .idle))
  (blank)
  (<- (. observed advance) true)
  (print (is (. observed phase) .idle))
  (do-new
    (print (is (. observed phase) .active))
  )
  (blank)
  (<- (. first advance) true)
  (do-new
    (print (is (. first phase) .active))
    (print (is (. second phase) .idle))
    (blank)
    (<- (. second advance) true)
    (do-new
      (print (is (. first phase) .active))
      (print (is (. second phase) .active))
      (blank)
      (<- (. header advance) "header")
      (do-new
        (print
          (match (. header phase)
            (case (call .active value)
              value
            )
            (default
              "none"
            )
          )
        )
        (blank)
        (<- (. section advance) true)
        (do-new
          (print (is (. section phase) .active))
        )
      )
    )
  )
)

(type + Widget
  (prop + name String)
  (prop + advance (of AsyncChannel Bool))
  (prop + phase _
    (machine
      (state idle)
      (state active (trigger on advance))
    )
  )
  (fn-do + This _
    (param name _)
  )
)

(type + ObservedWidget
  (prop + advance (of AsyncChannel Bool))
  (prop + phase _
    (machine
      (state idle)
      (state active (trigger on advance)
        (do
          (print "observed active")
        )
      )
    )
  )
)

(type + HeaderMarkupMachine (is Box)
  (prop + advance (of AsyncChannel String))
  (prop + phase _
    (machine
      (state idle)
      (state (call active value) (trigger for value advance))
    )
  )
  (content
    (<> This)
  )
)

(type + SectionMarkupMachine (is Box)
  (prop + advance (of AsyncChannel Bool))
  (prop + phase _
    (machine
      (state idle)
      (state active (trigger on advance))
    )
  )
  (content
    (<> This)
  )
)
```

```output
true
true
true
observed active
true
true
true
true
true
header
true
```

### Local immediate declaration state machines use local storage for each function invocation.

```brix
(main
  (print (call run-local "one"))
  (print (call run-local "two"))
)

(fn-do + run-local String
  (param label String)
  (let flow
    (machine
      (state idle)
      (state ready (trigger if true)
        (do
          (print (str label " ready"))
        )
      )
    )
  )
  (blank)
  (print (is flow .ready))
  (return (str "done " label))
)
```

```output
one ready
true
done one
two ready
true
done two
```

### Local lazy declaration state machines initialize once per lazy declaration lifetime.

```brix
(main
  (call lazy-local "first")
  (call lazy-local "second")
)

(blank)
(prop _ hits _ (= 0))

(fn-do + lazy-local _
  (param label String)
  (lazy flow
    (machine
      (state idle)
      (state ready (trigger if (call mark label)))
    )
  )
  (blank)
  (print (str "before " label " " hits))
  (print (is flow .ready))
  (print (str "middle " label " " hits))
  (print (is flow .ready))
  (print (str "after " label " " hits))
)

(fn-do + mark Bool
  (param label String)
  (+= hits 1)
  (print (str "mark " label " " hits))
  (return true)
)
```

```output
before first 0
mark first 1
true
middle first 1
true
after first 1
before second 1
mark second 2
true
middle second 2
true
after second 2
```

### Anonymous module-top machines still expose module/main projections.

```brix
(prop + opened (of AsyncChannel Bool))
(blank)

(main
  (state closed)
  (state open (trigger on opened))
  (blank)
  (print closed)
  (blank)
  (<- opened true)
  (do-new
    (print open)
    (print closed)
  )
)
```

```output
true
true
false
```

### Anonymous class-content machines have per-instance projections and storage.

```brix
(main
  (let first (call Panel "first"))
  (let second (call Panel "second"))
  (blank)
  (<- (. first activate) true)
  (do-new
    (print (. first active))
    (print (. second active))
    (blank)
    (<- (. second activate) true)
    (do-new
      (print (. first active))
      (print (. second active))
    )
  )
)

(type + Panel
  (prop + name String)
  (prop + activate (of AsyncChannel Bool))
  (fn-do + This _
    (param name _)
  )
  (content
    (state idle)
    (state active (trigger on activate)
      (do
        (print (str name " active"))
      )
    )
  )
)
```

```output
first active
true
false
second active
true
true
```

### A case of an anonymous class-content machine that holds a value projects it as an optional: what the case holds while it is active, and nothing while it is not, which a tracked read follows.

```brix
(main
  (let watcher (call Watcher))
  (let holder (. watcher holder))
  (print (. holder renaming))
  (print (. watcher shown))
  (<- (. holder requested) "a")
  (print (. holder renaming))
  (print (?? (. holder renaming) "none"))
  (print (. watcher shown))
  (<- (. holder done) true)
  (print (?? (. holder renaming) "none"))
  (print (. watcher shown))
)

(type + Watcher
  (prop + holder _ (= (call Holder)))
  (prop + shown _ (= (?? (. holder #renaming) "none")))
)

(type + Holder
  (event + requested String)
  (event + done Bool)
  (content
    (state is-idle)
    (state (call renaming name) (trigger for name requested)
      (then is-idle (trigger on done))
    )
  )
)
```

```output
unknown
none
a
a
a
none
none
```

### A function of another module typed before any object of the class is filled reads the projection of a case that holds a value as the optional of what it holds.

```brix
(import ./lib/owner)
(import ./lib/reader)
(blank)

(main
  (let owner (call SubOwner))
  (print (call shown owner))
  (<- (. owner requested) (call Item "a"))
  (print (call shown owner))
  (print (!= (. owner renaming) null))
)
```

```brix file=lib/owner.bx
(type + Item
  (prop + key String)
  (fn-do + This _
    (param key _)
  )
)

(type + Owner
  (event + requested Item)
  (content
    (state is-idle)
    (state (call renaming item) (trigger for item requested))
  )
)
```

```brix file=lib/reader.bx
(import ./owner)

(type + SubOwner (is Owner))

(fn-do + shown _
  (param owner Owner)
  (return (?? (?. (. owner renaming) key) "none"))
)
```

```output
none
a
true
```

### Anonymous class-content machines without an untriggered case start inactive.

```brix
(main
  (let first (call TriggerOnlyPanel "first"))
  (let second (call TriggerOnlyPanel "second"))
  (blank)
  (print (. first active))
  (blank)
  (<- (. first activate) true)
  (do-new
    (print (. first active))
    (print (. second active))
  )
)

(type + TriggerOnlyPanel
  (prop + name String)
  (prop + activate (of AsyncChannel Bool))
  (fn-do + This _
    (param name _)
  )
  (content
    (state active (trigger on activate)
      (do
        (print (str name " active"))
      )
    )
  )
)
```

```output
false
first active
true
false
```

### Anonymous machines can transition from inactive state through inline one-shot streams.

```brix
(prop + submitted (of AsyncChannel LoginSubmission))
(blank)

(main
  (state authenticating (trigger for (call .valid email password) submitted)
    (then authenticated (trigger for .authentic (call authenticate email password)))
  )
  (blank)
  (print authenticating)
  (print authenticated)
  (blank)
  (<- submitted (call .valid "email" "secret"))
  (do-new
    (print authenticating)
    (print authenticated)
    (blank)
    (for (in i (..< 0 3))
      (call yield-once)
    )
    (print authenticating)
    (print authenticated)
  )
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)

(fn-do + authenticate _
  (param email String)
  (param password String)
  (call yield-once)
  (blank)
  (yield
    (block
      (case? (== password "secret")
        (call ..authentic email "token")
      )
      (default
        ..incorrect-password
      )
    )
  )
)

(type + LoginSubmission (is Union)
  (variant valid (tuple String String))
)
```

```output
false
false
true
false
false
true
```

### Inline constructor sources compile in top-level and class machines, and iterable sources are not re-evaluated when their owning state exits.

```brix
(prop + open (of AsyncChannel Bool))
(prop + close (of AsyncChannel Bool))
(prop + timer-open (of AsyncChannel Bool))
(prop + timer-close (of AsyncChannel Bool))
(prop _ source-calls _ (= 0))

(main
  (let class-timer (call InlineTimerMachine))
  (blank)
  (print (is (. class-timer phase) .waiting))
  (<- (. class-timer close) true)
  (do-new
    (print (is (. class-timer phase) .idle))
  )
  (blank)
  (state idle)
  (state active (trigger on open)
    (then done (trigger for true (call one-shot)))
    (then idle (trigger on close))
  )
  (blank)
  (state done)
)

(blank)
(prop + timer-flow _
  (machine
    (state idle)
    (state waiting (trigger on timer-open)
      (then idle (trigger on (call (of AsyncChannel Bool))))
      (then idle (trigger on timer-close))
    )
  )
)
(blank)

(main
  (print source-calls)
  (blank)
  (<- open true)
  (do-new
    (print source-calls)
    (blank)
    (<- close true)
    (do-new
      (print active)
      (print source-calls)
      (blank)
      (<- timer-open true)
      (do-new
        (print (is timer-flow .waiting))
        (blank)
        (<- timer-close true)
        (do-new
          (print (is timer-flow .idle))
        )
      )
    )
  )
)

(fn-do + one-shot _
  (+= source-calls 1)
  (yield true)
)

(type + InlineTimerMachine
  (prop + close (of AsyncChannel Bool))
  (prop + phase _
    (machine
      (state waiting
        (then idle (trigger on (call (of AsyncChannel Bool))))
        (then idle (trigger on close))
      )
      (blank)
      (state idle)
    )
  )
)
```

```output
true
0
true
1
false
1
true
true
```

### Nested named machines inside class-owned states are per-instance and unsubscribe on parent exit.

```brix
(main
  (let first (call Session "first"))
  (let second (call Session "second"))
  (blank)
  (<- (. first open) true)
  (do-new
    (<- (. first open) true)
    (do-new
      (<- (. first login) true)
      (do-new
        (<- (. second open) true)
        (do-new
          (print (is (. first app) .ready))
          (print (is (. second app) .ready))
          (blank)
          (<- (. first close) true)
          (do-new
            (<- (. first login) true)
            (do-new
              (print (is (. first app) .closed))
              (print (is (. second app) .ready))
            )
          )
        )
      )
    )
  )
)

(type + Session
  (prop + name String)
  (prop + open (of AsyncChannel Bool))
  (prop + close (of AsyncChannel Bool))
  (prop + login (of AsyncChannel Bool))
  (prop + app _
    (machine
      (state closed)
      (state ready (trigger on open)
        (prop + auth _
          (machine
            (state editing
              (finally
                (print (str name " auth editing exit"))
              )
            )
            (blank)
            (state authenticated (trigger on login)
              (do
                (print (str name " authenticated"))
              )
              (blank)
              (finally
                (print (str name " auth authenticated exit"))
              )
            )
          )
        )
        (blank)
        (then closed (trigger on close))
      )
    )
  )
  (fn-do + This _
    (param name _)
  )
)
```

```output
first auth editing exit
first auth editing exit
first authenticated
true
true
first auth authenticated exit
true
true
```

### State-machine cases from locals do not leak as projections in unrelated scopes.

```brix crash
(main
  (call make-local)
  (print idle)
)

(fn-do + make-local _
  (let flow
    (machine
      (state idle)
      (state ready)
    )
  )
  (blank)
  (print (is flow .idle))
)
```

```output
Exception: Name "idle" not found
<no file:4>
      (print idle)
             ^^^^
```

### Event transitions subscribe asynchronously and do not consume events emitted before entry.

```brix
(main
  (call check-async-subscribe)
)

(fn-do + check-async-subscribe _
  (let events (of AsyncChannel Bool))
  (blank)
  (<- events true)
  (blank)
  (let flow
    (machine
      (state idle)
      (state fired (trigger on events))
    )
  )
  (blank)
  (do-new
    (print (is flow .idle))
    (blank)
    (<- events true)
    (do-new
      (do-new
        (print (is flow .fired))
      )
    )
  )
)
```

```output
true
false
```

### Event state transitions finish before the publishing call returns

```brix
(main
  (let source (call EventMachine))
  (print (is (. source auth) .idle))
  (call (. source submit) "joe@example.com")
  (print (is (. source auth) .authenticating))
  (print
    (match (. source auth)
      (case (call .authenticating email)
        email
      )
      (default
        "none"
      )
    )
  )
  (blank)
  (let siblings (call EventSiblingMachine))
  (call (. siblings submit) 1)
  (print (is (. siblings flow) .first))
  (print (is (. siblings flow) .second))
  (blank)
  (let exited (call EventExitMachine))
  (<- (. exited start) true)
  (print (is (. exited flow) .active))
  (blank)
  (<- (. exited leave) true)
  (print (is (. exited flow) .done))
  (blank)
  (<- (. exited stale) true)
  (print (is (. exited flow) .done))
  (print (is (. exited flow) .stale))
)

(type + EventMachine
  (event + submitted String)
  (prop + auth _
    (machine
      (state idle)
      (state (call authenticating email) (trigger for email submitted))
    )
  )
  (fn-do + submit _
    (param email String)
    (<- submitted email)
  )
)

(type + EventSiblingMachine
  (event + submitted Int)
  (prop + flow _
    (machine
      (state idle)
      (state first (trigger on submitted (> each 0)))
      (state second (trigger on submitted (> each 0)))
    )
  )
  (fn-do + submit _
    (param value Int)
    (<- submitted value)
  )
)

(type + EventExitMachine
  (event + start Bool)
  (event + leave Bool)
  (event + stale Bool)
  (prop + flow _
    (machine
      (state idle)
      (state active (trigger on start)
        (then done (trigger on leave))
        (then stale (trigger on stale))
      )
    )
  )
)
```

```output
true
true
joe@example.com
true
false
true
true
true
false
```

### A parked or canceled event handler does not hold back another task's transitions

```brix
(main
  (let gate (call Semaphore))
  (let parked (call ParkingHandler gate))
  (let other (call EventToggle))
  (let parker
    (do-new
      (<- (. parked pings) true)
    )
  )
  (do-new
    (<- (. other go) true)
    (print (is (. other flow) .on))
    (call (. parker cancel))
    (<- (. other back) true)
    (print (is (. other flow) .idle))
  )
)

(type + ParkingHandler
  (prop + gate (of Semaphore Void))
  (event + pings Bool)
  (fn-do + This _
    (param gate _)
  )
  (subscribe-for (on event pings)
    (call (. gate wait))
  )
)

(type + EventToggle
  (event + go Bool)
  (event + back Bool)
  (prop + flow _
    (machine
      (state idle)
      (state on (trigger on go)
        (then idle (trigger on back))
      )
    )
  )
)
```

```output
true
true
```

### A transition canceled in its entry rows does not hold back later transitions

```brix
(main
  (let gate (call Semaphore))
  (let parked (call ParkingEntry gate))
  (let other (call EventToggle))
  (let parker
    (do-new
      (<- (. parked go) true)
    )
  )
  (do-new
    (call (. parker cancel))
    (<- (. other go) true)
    (print (is (. other flow) .on))
  )
)

(type + ParkingEntry
  (prop + gate (of Semaphore Void))
  (event + go Bool)
  (fn-do + This _
    (param gate _)
  )
  (prop + flow _
    (machine
      (state idle)
      (state on (trigger on go)
        (do
          (call (. gate wait))
        )
      )
    )
  )
)

(type + EventToggle
  (event + go Bool)
  (prop + flow _
    (machine
      (state idle)
      (state on (trigger on go))
    )
  )
)
```

```output
true
```

### Keyed event transitions queued during a transition are rejected after the receiver generation is released

```brix
(main
  (let mailbox (call KeyedMachineMailbox))
  (let previous (call KeyedMachineReceiver mailbox 7))
  (let courier (call KeyedMachineCourier mailbox previous))
  (<- (. courier deliveries) true)
  (print (is (. previous flow) .active))
  (blank)
  (let replacement (call KeyedMachineReceiver mailbox 7))
  (print (is (. replacement flow) .idle))
  (<- ([] (. mailbox messages) 7) true)
  (print (is (. replacement flow) .active))
  (blank)
  (let moving (call KeyedMachineReceiver mailbox 8))
  (call (. moving change-key) 9)
  (<- ([] (. mailbox messages) 8) true)
  (print (is (. moving flow) .idle))
  (<- ([] (. mailbox messages) 9) true)
  (print (is (. moving flow) .active))
)

(type + KeyedMachineMailbox
  (event + messages Bool U64)
)

(type + KeyedMachineReceiver
  (prop + mailbox KeyedMachineMailbox)
  (prop + id U64)
  (fn-do + This _
    (param mailbox _)
    (param id _)
  )
  (prop + flow _
    (machine
      (state idle)
      (state (call active value) (trigger for value ([] (. mailbox messages) id)))
    )
  )
  (fn-do + close _)
  (fn-do + change-key _
    (param next U64)
    (call (. ([] (. mailbox messages) id) rekey) next)
    (= id next)
  )
)

(type + KeyedMachineCourier
  (prop + mailbox KeyedMachineMailbox)
  (prop + receiver KeyedMachineReceiver)
  (event + deliveries Bool)
  (fn-do + This _
    (param mailbox _)
    (param receiver _)
  )
  (prop + flow _
    (machine
      (state idle)
      (state delivered (trigger on deliveries)
        (do
          (<- ([] (. mailbox messages) (. receiver id)) true)
          (call (. receiver close))
        )
      )
    )
  )
)
```

```output
false
true
true
true
true
```

### A queued keyed transition is rejected when its receiver rekeys before activation

```brix
(main
  (let mailbox (call KeyedMachineMailbox))
  (let moving (call KeyedMachineReceiver mailbox 8))
  (let courier (call KeyedMachineCourier mailbox moving))
  (<- (. courier deliveries) true)
  (print (is (. moving flow) .idle))
  (blank)
  (<- ([] (. mailbox messages) 9) true)
  (print (is (. moving flow) .active))
)

(type + KeyedMachineMailbox
  (event + messages Bool U64)
)

(type + KeyedMachineReceiver
  (prop + mailbox KeyedMachineMailbox)
  (prop + id U64)
  (fn-do + This _
    (param mailbox _)
    (param id _)
  )
  (prop + flow _
    (machine
      (state idle)
      (state (call active value) (trigger for value ([] (. mailbox messages) id)))
    )
  )
  (fn-do + change-key _
    (param next U64)
    (call (. ([] (. mailbox messages) id) rekey) next)
    (= id next)
  )
)

(type + KeyedMachineCourier
  (prop + mailbox KeyedMachineMailbox)
  (prop + receiver KeyedMachineReceiver)
  (event + deliveries Bool)
  (fn-do + This _
    (param mailbox _)
    (param receiver _)
  )
  (prop + flow _
    (machine
      (state idle)
      (state delivered (trigger on deliveries)
        (do
          (<- ([] (. mailbox messages) (. receiver id)) true)
          (call (. receiver change-key) 9)
        )
      )
    )
  )
)
```

```output
true
true
```

### A keyed machine keeps receiving after an ordinary subscription on its endpoint breaks

```brix
(main
  (let mailbox (call PersistentKeyedMachineMailbox))
  (let receiver (call PersistentKeyedMachineReceiver mailbox 7))
  (<- ([] (. mailbox messages) 7) true)
  (print (is (. receiver flow) .active))
  (blank)
  (<- ([] (. mailbox messages) 7) false)
  (print
    (match (. receiver flow)
      (case (call .active value)
        value
      )
      (default
        true
      )
    )
  )
)

(type + PersistentKeyedMachineMailbox
  (event + messages Bool U64)
)

(type + PersistentKeyedMachineReceiver
  (prop + mailbox PersistentKeyedMachineMailbox)
  (prop + id U64)
  (fn-do + This _
    (param mailbox _)
    (param id _)
  )
  (subscribe-for (on value ([] (. mailbox messages) id))
    (print (str "ordinary " value))
    (break)
  )
  (prop + flow _
    (machine
      (state idle)
      (state (call active value) (trigger for value ([] (. mailbox messages) id)))
    )
  )
)
```

```output
ordinary true
true
false
```

### Event transitions from one publication choose the first sibling in source order

```brix
(main
  (let source (call EventSiblingMachine))
  (call (. source submit) 1)
  (print (is (. source flow) .first))
  (print (is (. source flow) .second))
)

(type + EventSiblingMachine
  (event + submitted Int)
  (prop + flow _
    (machine
      (state idle)
      (state first (trigger on submitted (> each 0)))
      (state second (trigger on submitted (> each 0)))
    )
  )
  (fn-do + submit _
    (param value Int)
    (<- submitted value)
  )
)
```

```output
true
false
```

### Event subscriptions from an exited state reject later publications

```brix
(main
  (let source (call EventExitMachine))
  (<- (. source start) true)
  (do-new
    (print (is (. source flow) .active))
    (blank)
    (<- (. source leave) true)
    (do-new
      (print (is (. source flow) .done))
      (blank)
      (<- (. source stale) true)
      (do-new
        (print (is (. source flow) .done))
        (print (is (. source flow) .stale))
      )
    )
  )
)

(type + EventExitMachine
  (event + start Bool)
  (event + leave Bool)
  (event + stale Bool)
  (prop + flow _
    (machine
      (state idle)
      (state active (trigger on start)
        (then done (trigger on leave))
        (then stale (trigger on stale))
      )
    )
  )
)
```

```output
true
true
true
false
```

### Case tests read the state flags of the machine they are read from, inside or outside its class.

```brix
(main
  (let m (call Machine))
  (let other (call Machine))
  (<- (. m submit) true)
  (do-new
    (<- (. m begin-child) true)
    (do-new
      (print (str "outside " (is (. m flow) .submitted) " " (is (. m flow) .validating) " " (is (. m flow) .idle)))
      (print (str "other " (call (. m compare) other)))
      (let a (call Auth))
      (<- (. a results) .rate-limited)
      (do-new
        (print (str "auth " (is (. a auth) .authenticating) " " (is (. a auth) .authenticated) " " (is (. a auth) .rejected)))
      )
    )
  )
)

(type + Machine
  (event + submit Bool)
  (event + begin-child Bool)
  (prop + flow _
    (machine
      (state idle)
      (state submitted (trigger on submit)
        (state validating (trigger on begin-child))
      )
    )
  )
  (fn-do + compare String
    (param other Machine)
    (return (str (is flow .submitted) " " (is (. other flow) .submitted) " " (is (. other flow) .idle)))
  )
)

(type + Auth
  (event + results AuthResult)
  (prop + auth _
    (machine
      (state authenticating
        (then authenticated (trigger for .authentic results))
        (then rejected (trigger for .incorrect-password results))
        (else-state authenticating)
      )
      (blank)
      (state authenticated)
      (state rejected)
    )
  )
)

(type + AuthResult (is Union)
  (variant authentic)
  (variant incorrect-password)
  (variant rate-limited)
)
```

```output
outside true true false
other true false true
auth false false false
```

### A class machine payload can depend on the owner type parameter.

```brix
(main
  (let h (call (of Holder Int)))
  (<- (. h items) 5)
  (do-new
    (print
      (match (. h flow)
        (case (call .got v)
          (str "got " v)
        )
        (default
          "none"
        )
      )
    )
  )
)

(type + (of Holder Value)
  (event + items Value)
  (prop + flow _
    (machine
      (state idle)
      (state (call got v) (trigger for v items))
    )
  )
)
```

```output
got 5
```

```brix
(main
  (let host (call Host))
  (print (call (. host summary)))
  (blank)
  (<- (. host open-requests) true)
  (do-new
    (print (call (. host summary)))
    (blank)
    (<- (. host view-toggles) true)
    (do-new
      (print (call (. host summary)))
      (blank)
      (call (. host emitter fire))
      (do-new
        (print (call (. host summary)))
        (blank)
        (<- (. host close-requests) true)
        (do-new
          (print (call (. host summary)))
        )
      )
    )
  )
)

(type + Label
  (prop + text _ (= ""))
)

(type + Emitter (is Label)
  (event + activated Bool)
  (fn-do + fire _
    (<- activated true)
  )
)

(type + Host
  (prop + open-requests (of AsyncChannel Bool))
  (prop + close-requests (of AsyncChannel Bool))
  (prop + view-toggles (of AsyncChannel Bool))
  (prop + is-grid _ (= false))
  (prop + was-received _ (= false))
  (fn-do + (infix this += (param child Label)) _)
  (fn-expr + summary String
    (str (. root-label text) " " (. state-label text) " " (. view-label text) " " (. event-label text))
  )
  (content
    (<> This
      (<> Label (id: root-label)
        (= text (if-expr #running "running" "stopped"))
      )
      (<> Label (id: state-label)
        (= text (if-expr #is-open "open" "closed"))
      )
      (<> Label (id: view-label)
        (= text (if-expr #is-grid "grid" "list"))
      )
      (<> Label (id: event-label)
        (= text (if-expr #was-received "received" "idle"))
      )
      (<> Emitter (id: emitter))
    )
    (blank)
    (state running
      (state is-closed
        (then is-open (trigger on open-requests))
      )
      (blank)
      (state is-open
        (then is-closed (trigger on close-requests))
      )
      (blank)
      (prop + view-mode _
        (machine
          (state view-list
            (do
              (= is-grid (is view-mode .view-grid))
            )
            (blank)
            (then view-grid (trigger on view-toggles))
          )
          (blank)
          (state view-grid
            (do
              (= is-grid (is view-mode .view-grid))
            )
            (blank)
            (then view-list (trigger on view-toggles))
          )
        )
      )
      (blank)
      (prop + event-mode _
        (machine
          (state event-idle
            (then event-received (trigger on (. emitter activated)))
          )
          (blank)
          (state event-received
            (do
              (= was-received (is event-mode .event-received))
            )
          )
        )
      )
    )
  )
)
```

```output
running closed list idle
running open list idle
running open grid idle
running open grid received
running closed grid received
```

```brix
(import /core/style)
(import /core/style-debug)
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
  (print (call style-computed-snapshot panel))
  (print (. panel rule-count))
  (blank)
  (<- (. panel open-requests) true)
  (do-new
    (call (. demo markup-batch-queue checkpoint))
    (print (call style-computed-snapshot panel))
    (print (. panel rule-count))
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (<- (. panel close-requests) true)
  (do-new
    (call (. demo markup-batch-queue checkpoint))
    (print (call style-computed-snapshot panel))
    (print (. panel rule-count))
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

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(type + Panel (is Box)
  (using TextStyle)
  (prop + open-requests (of AsyncChannel Bool))
  (prop + close-requests (of AsyncChannel Bool))
  (prop + rule-count _ (= 0))
  (prop + is-open _ (= (is #phase .open)))
  (blank)
  (rule This
    (= ink 9)
    (when-style #is-open
      (= ink 11)
    )
  )
  (prop + phase _
    (machine
      (state closed)
      (state open (trigger on open-requests)
        (then closed (trigger on close-requests))
      )
    )
  )
  (when-rule #is-open
    (+= rule-count 1)
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=9
0
node TextStyle seg=owned gen=2 mask=1 ink=11
1
node TextStyle seg=owned gen=3 mask=1 ink=9
1
```

```brix crash
(main
  (let source (call FailingFlow))
  (<- (. source enter) true)
)

(type + FailingFlow
  (event + enter Bool)
  (prop + flow _
    (machine
      (state idle)
      (state active (trigger on enter)
        (do
          (print "enter active")
          (call fail)
          (print "not reached")
        )
      )
    )
  )
)

(fn-do + fail _
  (throw (call Error "action failed"))
)
```

```output
enter active
```

### Event transitions advance inside a loop that never yields to the task queue.

```brix
(main
  (let walker (call Walker))
  (for (in i (..< 0 4))
    (call (. walker step))
    (print (str i " " (is (. walker phase) .walking)))
  )
)

(type + Walker
  (event + ticks Bool)
  (prop + count Size (= 0))
  (prop + phase _
    (machine
      (state resting
        (then walking (trigger on ticks (>= count 2)))
      )
      (blank)
      (state walking
        (do
          (print (str "enter walking at " count))
        )
        (blank)
        (then resting (trigger on ticks (>= count 4)))
      )
    )
  )
  (fn-do + step _
    (+= count 1)
    (<- ticks true)
  )
)
```

```output
0 false
enter walking at 2
1 true
2 true
3 false
```

### Publications made by a transition queue their transitions until it finishes, and the outermost publication returns after all of them run.

```brix
(main
  (let first (call Relay "first"))
  (let second (call Relay "second"))
  (= (. first next) second)
  (call (. first poke))
  (print "returned")
  (print (is (. second phase) .lit))
)

(type + Relay
  (prop + name String)
  (prop + next (? Relay))
  (event + pokes Bool)
  (fn-do + This _
    (param name _)
  )
  (prop + phase _
    (machine
      (state idle)
      (state lit (trigger on pokes)
        (do
          (print (str name " enter"))
          (if next
            (call (. next poke))
          )
          (print (str name " entered"))
        )
      )
    )
  )
  (fn-do + poke _
    (<- pokes true)
  )
)
```

```output
first enter
first entered
second enter
second entered
returned
true
```

### A publication made while a transition runs is matched against the machine's state once the transition finishes, so the state it enters can take it.

```brix
(main
  (let door (call Door))
  (<- (. door open-requests) true)
  (print (. door is-open))
  (<- (. door close-requests) true)
  (print (. door reopened))
  (print (. door is-open))
)

(type + Door
  (event + open-requests Bool)
  (event + close-requests Bool)
  (event + closed Bool)
  (prop + reopened _ (= false))
  (content
    (state is-closed
      (then is-open (trigger on open-requests))
    )
    (state is-open
      (then is-closed (trigger for reason close-requests)
        (do
          (<- closed reason)
        )
      )
    )
  )
  (subscribe-for (on reason closed)
    (if (not reopened)
      (= reopened true)
      (<- open-requests true)
    )
  )
)
```

```output
true
true
true
```

### A nested named machine entered by an event transition runs its own continuations.

```brix
(prop + ticks (of AsyncChannel Bool))
(prop + wakes (of AsyncChannel Bool))
(blank)
(prop + life _
  (machine
    (state asleep)
    (state awake (trigger on wakes)
      (prop + voice _
        (machine
          (state quiet
            (then calling (trigger on ticks))
          )
          (blank)
          (state calling
            (do
              (print "calling")
            )
          )
        )
      )
    )
  )
)
(blank)

(main
  (<- wakes true)
  (do-new
    (<- ticks true)
    (do-new
      (print "end")
    )
  )
)
```

```output
calling
end
```

### A nested named machine subscribes its root triggers while its parent state enters.

```brix
(prop + ticks (of AsyncChannel Bool))
(prop + wakes (of AsyncChannel Bool))
(blank)
(prop + life _
  (machine
    (state asleep)
    (state awake (trigger on wakes)
      (prop + voice _
        (machine
          (state quiet)
          (state calling (trigger on ticks)
            (do
              (print "calling")
            )
          )
        )
      )
    )
  )
)
(blank)

(main
  (<- wakes true)
  (do-new
    (<- ticks true)
    (do-new
      (print "end")
    )
  )
)
```

```output
calling
end
```

### A continuation into another branch exits the source ancestors and enters the target ancestors.

```brix
(main
  (let m (call Machine))
  (<- (. m go) true)
  (print (is (. m phase) .swimming))
  (print (is (. m phase) .arriving))
  (print (is (. m phase) .ashore))
  (print (is (. m phase) .crawling))
)

(type + Machine
  (event + go Bool)
  (prop + phase _
    (machine
      (state swimming
        (finally
          (print "exit swimming")
        )
        (blank)
        (state arriving
          (then crawling (trigger on go))
        )
      )
      (blank)
      (state ashore
        (do
          (print "enter ashore")
        )
        (blank)
        (state resting
          (do
            (print "enter resting")
          )
        )
        (blank)
        (state crawling
          (do
            (print "enter crawling")
          )
        )
      )
    )
  )
)
```

```output
exit swimming
enter ashore
enter crawling
false
false
true
true
```

### An immediate child case chosen when its parent enters keeps the parent active.

```brix
(prop + ready _ (= true))
(blank)
(prop + phase _
  (machine
    (state parent
      (do
        (print "enter parent")
      )
      (blank)
      (state waiting)
      (state going (trigger if ready)
        (do
          (print "enter going")
        )
      )
    )
  )
)
(blank)

(main
  (print (is phase .parent))
  (print (is phase .going))
  (print (is phase .waiting))
)
```

```output
enter parent
enter going
true
true
false
```

### A parent case test compiled before its machine's owner is constructed reads the parent's flag.

```brix
(main
  (let holder (call Holder))
  (print (. holder is-outer))
  (= (. holder mother) (call Mother))
  (print (. holder is-outer))
)

(type + Holder
  (prop + mother (? Mother))
  (fn-do + is-outer Bool :get
    (let current mother)
    (return-if false (if (not current)))
    (return (is (. current phase) .outer))
  )
)

(type + Mother
  (prop + phase _
    (machine
      (state outer
        (state inner)
      )
    )
  )
)
```

```output
false
true
```

### A state handler runs only while its state is active.

```brix
(main
  (let mover (call Mover))
  (for (in i (..< 0 6))
    (call (. mover step) 0.5)
    (print (str i " " (is (. mover phase) .moving) " " (. mover x)))
  )
)

(type + Mover
  (event + ticks F64)
  (prop + x F64 (= 0))
  (prop + count Size (= 0))
  (prop + phase _
    (machine
      (state moving
        (for (on dt ticks)
          (+= x dt)
        )
        (blank)
        (then resting (trigger on ticks (>= count 3)))
      )
      (blank)
      (state resting)
    )
  )
  (fn-do + step _
    (param dt F64)
    (+= count 1)
    (<- ticks dt)
  )
)
```

```output
0 true 0.5
1 true 1
2 false 1.5
3 false 1.5
4 false 1.5
5 false 1.5
```

### A transition listed before a handler can leave the state before the handler sees the event.

```brix
(main
  (let gate (call Gate))
  (<- (. gate ticks) 1)
  (<- (. gate ticks) 1)
  (print (. gate seen))
)

(type + Gate
  (event + ticks F64)
  (prop + seen F64 (= 0))
  (prop + phase _
    (machine
      (state open
        (then closed (trigger on ticks (>= seen 1)))
        (blank)
        (for (on dt ticks (when (> dt 0)))
          (+= seen dt)
        )
      )
      (blank)
      (state closed)
    )
  )
)
```

```output
1
```

### A class machine's continuation payload can call the owner's methods and read its properties.

```brix
(main
  (let rover (call Rover))
  (<- (. rover pokes) true)
  (print
    (match (. rover phase)
      (case (call .heading spot)
        (str "heading " spot)
      )
      (default
        "idle"
      )
    )
  )
)

(type + Rover
  (event + pokes Bool)
  (prop + base Int (= 40))
  (prop + phase _
    (machine
      (state idle
        (then (call heading (+ (call pick-spot) base)) (trigger on pokes))
      )
      (blank)
      (state (call heading spot))
    )
  )
  (fn-do _ pick-spot Int
    (return 2)
  )
)
```

```output
heading 42
```

### A state case whose one payload value is a named tuple keeps the tuple whole.

```brix
(main
  (let holder (call Holder))
  (<- (. holder pairs) (call Pair 3 4))
  (print
    (match (. holder phase)
      (case (call .holding pair)
        (str "holding " (+ (. pair a) (. pair b)))
      )
      (default
        "empty"
      )
    )
  )
)

(type + Pair (is Tuple)
  (prop + a Int)
  (prop + b Int)
)

(type + Holder
  (event + pairs Pair)
  (prop + phase _
    (machine
      (state empty
        (then (call holding pair) (trigger for pair pairs))
      )
      (blank)
      (state (call holding pair))
    )
  )
)
```

```output
holding 7
```

### A transition into a declared state does not carry the source state's payload into it.

```brix
(main
  (let hunter (call Hunter))
  (<- (. hunter ticks) true)
  (<- (. hunter ticks) true)
)

(type + Hunter
  (event + ticks Bool)
  (prop + quarry (? Size) (= 3))
  (prop + phase _
    (machine
      (state searching
        (then (call hunting (!! quarry)) (trigger if quarry))
      )
      (blank)
      (state (call hunting person)
        (do
          (print (str "hunting " person))
          (= quarry (+ (!! quarry) 1))
        )
        (blank)
        (then searching (trigger on ticks))
      )
    )
  )
)
```

```output
hunting 3
hunting 4
hunting 5
```

### A continuation's payload can read a property the owner class inherits.

```brix
(import /graphics/point)
(blank)

(main
  (let swimmer (call Swimmer))
  (<- (. swimmer ticks) true)
  (print (is (. swimmer phase) .waiting))
)

(type + Thing
  (prop + position _ (= (call Point 3 4)))
)

(type + Swimmer (is Thing)
  (event + ticks Bool)
  (prop + phase _
    (machine
      (state swimming
        (then (call waiting position) (trigger on ticks))
      )
      (blank)
      (state (call waiting spot)
        (do
          (print (str "waiting at " (. spot x)))
        )
      )
    )
  )
)
```

```output
waiting at 3
true
```

### A continuation's payload takes its type from the arriving item and the owner state's payload.

```brix
(main
  (let counter (call Counter))
  (<- (. counter go) 1000)
  (<- (. counter next) true)
)

(type + Counter
  (event + go Size)
  (event + next Bool)
  (prop + phase _
    (machine
      (state idle
        (then (call counting each) (trigger on go))
      )
      (blank)
      (state (call counting total)
        (do
          (print total)
        )
        (blank)
        (then (call doubled (* total 2)) (trigger on next))
      )
      (blank)
      (state (call doubled value)
        (do
          (print value)
        )
      )
    )
  )
)
```

```output
1000
2000
```

### A continuation into a declared state runs its own nested continuation.

```brix
(main
  (let hunter (call Hunter))
  (<- (. hunter start) 5)
  (<- (. hunter rest) true)
  (<- (. hunter go) true)
)

(type + Hunter
  (event + start Int)
  (event + rest Bool)
  (event + go Bool)
  (prop + phase _
    (machine
      (state idle
        (then (call chasing each) (trigger on start))
      )
      (blank)
      (state (call chasing goal)
        (do
          (print (str "chasing " goal))
        )
        (blank)
        (then (call resting (+ goal 1)) (trigger on rest)
          (then (call chasing goal) (trigger on go))
        )
      )
      (blank)
      (state (call resting goal)
        (do
          (print (str "resting " goal))
        )
      )
    )
  )
)
```

```output
chasing 5
resting 6
chasing 5
```

### A state's trigger source is evaluated each time the state is entered.

```brix
(main
  (let lamp (call Lamp))
  (for (in i (..< 0 3))
    (<- (. lamp flips) true)
    (print (str (. lamp phase)))
  )
)

(type + Lamp
  (event + flips Bool)
  (prop + phase _
    (machine
      (state dark
        (then lit (trigger on (call watched "dark")))
      )
      (blank)
      (state lit
        (then dark (trigger on (call watched "lit")))
      )
    )
  )
  (fn-do _ watched (of Broadcastable Bool)
    (param name String)
    (print (str "watching from " name))
    (return flips)
  )
)
```

```output
watching from dark
watching from lit
lit
watching from dark
dark
watching from lit
lit
```

### An event passed to an untyped parameter arrives as a broadcastable.

```brix
(main
  (let clock (call Clock))
  (print (call is-open (. clock ticks)))
  (let holder (call Holder (. clock ticks)))
  (for (on dt (. holder source))
    (print (str "heard " dt))
  )
  (<- (. clock ticks) 0.5)
)

(fn-do + is-open Bool
  (param source _)
  (let stream (of Broadcastable F64) source)
  (return (not (. stream is-closed)))
)

(type + Holder
  (prop + source (of Broadcastable F64))
  (fn-do + This _
    (param source _)
  )
)

(type + Clock
  (event + ticks F64)
)
```

```output
true
heard 0.5
```

### Machine work from a source that relays an event waits for the event's publication, like work from the event itself.

```brix
(main
  (let kettle (call Kettle))
  (<- (. kettle ticks) 1)
  (<- (. kettle ticks) 1)
  (print (. kettle heat))
  (print (is (. kettle phase) .whistling))
)

(type + Kettle
  (event + ticks F64)
  (prop + heat F64 (= 0))
  (prop + phase _
    (machine
      (state heating
        (for (on dt ticks)
          (+= heat dt)
        )
        (blank)
        (then whistling (trigger on (call Relay ticks) (>= heat 2)))
      )
      (blank)
      (state whistling)
    )
  )
)

(type + Relay
  (implements (of Broadcastable F64))
  (prop + source (of Broadcastable F64))
  (prop _ forward (? (of Subscriber F64)))
  (fn-do + This _
    (param source _)
  )
  (fn-expr + is-closed Bool :get
    (. source is-closed)
  )
  (fn-do + subscribe _
    (param subscriber (of Subscriber F64))
    (let relay (of Subscriber F64)
      (=> (dt from) :do
        (block
          (call subscriber dt this)
        )
      )
    )
    (= forward relay)
    (call (. source subscribe) relay)
  )
  (fn-do + unsubscribe _
    (param subscriber (of Subscriber F64))
    (return-if (if (not forward)))
    (call (. source unsubscribe) forward)
    (= forward null)
  )
  (fn-do + subscribe _
    (param subscriber (of Subscribable F64))
  )
  (fn-do + unsubscribe _
    (param subscriber (of Subscribable F64))
  )
)
```

```output
2
true
```

### A base class machine keeps its nested transitions on subclass instances when an event enters the case.

```brix
(main
  (let thing (call Pebble))
  (<- (. thing tosses) true)
  (print (is (. thing body) .falling))
  (<- (. thing landings) true)
  (print (is (. thing body) .falling))
)

(type + Thing
  (event + tosses Bool)
  (event + landings Bool)
  (prop + body _
    (machine
      (state resting)
      (blank)
      (state falling (trigger on tosses)
        (then resting (trigger on landings))
      )
    )
  )
)

(type + Pebble (is Thing))
```

```output
true
false
```

### A state's own transition on a stream is not replaced by a continuation's nested transition on it.

```brix
(main
  (let hunter (call Hunter))
  (<- (. hunter nap) true)
  (<- (. hunter go) true)
)

(type + Hunter
  (event + rest Bool)
  (event + nap Bool)
  (event + go Bool)
  (prop + phase _
    (machine
      (state chasing
        (do
          (print "chasing")
        )
        (blank)
        (then resting (trigger on rest)
          (then chasing (trigger on go))
        )
        (blank)
        (then resting (trigger on nap))
      )
      (blank)
      (state resting
        (do
          (print "resting")
        )
        (blank)
        (then idle (trigger on go))
      )
      (blank)
      (state idle
        (do
          (print "idle")
        )
      )
    )
  )
)
```

```output
chasing
resting
idle
```

### A match binds the payload of a state case that a subclass's refinement adds.

```brix
(main
  (let turtle (call Turtle))
  (print (. turtle raft))
  (<- (. turtle boards) "red")
  (print (. turtle raft))
)

(type + Toy
  (prop + body _
    (machine
      (state free)
    )
  )
)

(type + Turtle (is Toy)
  (event + boards String)
  (prop + body _
    (machine
      (state free
        (state paddling)
        (blank)
        (state (call riding name) (trigger for name boards))
      )
    )
  )
  (fn-do + raft String :get
    (return
      (match body
        (case (call .riding name)
          name
        )
        (default
          "none"
        )
      )
    )
  )
)
```

```output
none
red
```

### A closure in a transition condition captures the state's payload.

```brix
(main
  (let toy (call Toy))
  (<- (. toy pokes) true)
  (<- (. toy ticks) 1.0)
  (print (is (. toy body) .idle))
)

(type + Toy
  (event + pokes Bool)
  (event + ticks F64)
  (prop + body _
    (machine
      (state idle
        (then (call moving 2) (trigger on pokes))
      )
      (blank)
      (state (call moving goal)
        (then idle (trigger on ticks (call check (=> (dt) (> goal dt)))))
      )
    )
  )
  (fn-expr + check Bool
    (param test (of Predicate F64))
    (call test 1)
  )
)
```

```output
true
```

### A closure in a transition source captures the state's payload, and the state's later transitions still fire.

```brix
(main
  (let toy (call Toy))
  (<- (. toy pokes) true)
  (<- (. toy ticks) 1.0)
  (print (is (. toy body) .idle))
)

(type + Toy
  (event + pokes Bool)
  (event + ticks F64)
  (prop + body _
    (machine
      (state idle
        (then (call moving 2) (trigger on pokes))
      )
      (blank)
      (state (call moving goal)
        (then idle (trigger on (call gated ticks (=> (dt) (> goal dt)))))
      )
    )
  )
  (fn-do + gated (of Broadcastable F64)
    (param source (of Broadcastable F64))
    (param test (of Predicate F64))
    (print (call test 1))
    (return source)
  )
)
```

```output
true
true
```

### Transitions on the same source share one subscription, with or without a when guard.

```brix
(main
  (let toy (call Toy))
  (<- (. toy pokes) true)
  (<- (. toy ticks) 1.0)
  (print (is (. toy body) .far))
)

(type + Toy
  (event + pokes Bool)
  (event + ticks F64)
  (prop + body _
    (machine
      (state idle
        (then (call moving 2) (trigger on pokes))
      )
      (blank)
      (state (call moving goal)
        (then far (trigger on (call watched) (< goal 4)))
        (then idle (trigger on (call watched)))
      )
      (blank)
      (state far)
    )
  )
  (fn-do + watched (of Broadcastable F64)
    (print "watch")
    (return ticks)
  )
)
```

```output
watch
true
```

### A then with no trigger is an entry decision that always passes, after the if decisions before it.

```brix
(main
  (let toy (call Toy))
  (print (. toy where))
  (<- (. toy again) true)
  (print (. toy where))
)

(type + Toy
  (event + again Bool)
  (prop + count Size (= 0))
  (prop + body _
    (machine
      (state free
        (then free (trigger on again))
        (blank)
        (state planning
          (do
            (+= count 1)
          )
          (blank)
          (then (call moving (* count 10)) (trigger if (> count 1)))
          (then (call moving 5))
          (blank)
          (state idling)
        )
        (blank)
        (state (call moving goal))
      )
    )
  )
  (fn-do + where String :get
    (return-if "idling" (if (is body .idling)))
    (return
      (match body
        (case (call .moving goal)
          (str "moving " goal)
        )
        (default
          "other"
        )
      )
    )
  )
)
```

```output
moving 5
moving 20
```

### Entry decisions that loop back to a state still being entered are rejected, since entering would never settle.

```brix crash
(main
  (let toy (call Toy))
)

(type + Toy
  (prop + ready _ (= false))
  (prop + body _
    (machine
      (state free
        (then resting (trigger if (not ready)))
        (blank)
        (state planning)
        (blank)
        (state resting
          (then free (trigger if ready))
        )
      )
    )
  )
)
```

```output
Exception: Entry decisions loop back to 'free' without a trigger
<no file:16>
              (then free (trigger if ready))
              ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A machine's path lists its active cases from outer to inner, including cases a subclass's refinement adds when read through the base type.

```brix
(main
  (let turtle (call Turtle))
  (let toy Toy turtle)
  (print (call (. " > " join) (. toy body path)))
  (<- (. turtle grabs) true)
  (print (call (. " > " join) (. toy body path)))
  (<- (. turtle releases) true)
  (<- (. turtle boards) "red")
  (print (call (. " > " join) (. toy body path)))
  (print (call (. turtle where)))
  (print (call (. " > " join) (. (call Toy) body path)))
)

(type + Toy
  (event + grabs Bool)
  (event + releases Bool)
  (prop + body _
    (machine
      (state free)
      (blank)
      (state held (trigger on grabs)
        (then free (trigger on releases))
      )
    )
  )
)

(type + Turtle (is Toy)
  (event + boards String)
  (prop + body _
    (machine
      (state free
        (state paddling
          (then riding (trigger on boards))
        )
        (blank)
        (state riding)
      )
    )
  )
  (fn-do + where String
    (return (call (. " > " join) (. body path)))
  )
)
```

```output
free > paddling
held
free > riding
free > riding
free
```

### Refinements of refinements merge into one machine: each level's cases nest under those it names again, its entries and handlers join the case, and a continuation into a case only a base writes moves there.

```brix
(main
  (let fisher (call Fisher))
  (print (. fisher body path))
  (<- (. fisher ticks) true)
  (print (. fisher body path))
  (<- (. fisher ticks) true)
  (print (. fisher body path))
  (<- (. fisher tosses) true)
  (print (. fisher body path))
  (print (str (is (. fisher body) .casting) " " (is (. fisher body) .held)))
)

(type + Thing
  (event + ticks Bool)
  (event + tosses Bool)
  (prop + body _
    (machine
      (state free
        (do
          (print "enter free")
        )
        (finally
          (print "exit free")
        )
      )
      (blank)
      (state held)
      (blank)
      (state falling (trigger on tosses))
    )
  )
)

(type + Person (is Thing)
  (prop + body _
    (machine
      (state free
        (for (on n ticks)
          (print "person tick")
        )
        (blank)
        (state routine
          (do
            (print "enter routine")
          )
          (finally
            (print "exit routine")
          )
        )
      )
    )
  )
)

(type + Fisher (is Person)
  (prop + body _
    (machine
      (state free
        (state routine
          (state casting
            (then fishing (trigger on ticks))
          )
          (blank)
          (state fishing
            (then free (trigger on ticks))
          )
        )
      )
      (blank)
      (state falling
        (do
          (print "fisher falling")
        )
        (then held)
      )
    )
  )
)
```

```output
enter free
enter routine
free, routine, casting
person tick
free, routine, fishing
person tick
exit routine
exit free
enter free
enter routine
free, routine, casting
exit routine
exit free
fisher falling
held
false true
```

### Event transitions on one stream that several levels of refinement write in one case form one group, the base's transitions tried first.

```brix
(main
  (let crab (call Crab))
  (<- (. crab ticks) 1)
  (<- (. crab ticks) 2)
  (print (. crab body path))
  (<- (. crab ticks) 5)
  (print (. crab body path))
)

(type + Animal
  (event + ticks Size)
  (prop + body _
    (machine
      (state free
        (then hiding (trigger on ticks (> each 3)))
        (blank)
        (state idle)
      )
      (blank)
      (state hiding)
    )
  )
)

(type + Crab (is Animal)
  (prop + body _
    (machine
      (state free
        (then dashing (trigger on ticks (> each 1)))
        (blank)
        (state dashing
          (do
            (print "dash")
          )
        )
      )
    )
  )
)
```

```output
dash
free, dashing
hiding
```

### Where no level's transition passes in a group three levels of refinement join, each level's guard is tried once.

```brix
(main
  (let hermit (call Hermit))
  (<- (. hermit ticks) 0)
  (print (. hermit body path))
)

(type + Animal
  (event + ticks Size)
  (prop + body _
    (machine
      (state free
        (then hiding (trigger on ticks (call said "animal" (> each 3))))
      )
      (blank)
      (state hiding)
    )
  )
  (fn-do + said Bool
    (param who String)
    (param passed Bool)
    (print (str "try " who))
    (return passed)
  )
)

(type + Crab (is Animal)
  (prop + body _
    (machine
      (state free
        (then dashing (trigger on ticks (call said "crab" (> each 1))))
        (blank)
        (state dashing)
      )
    )
  )
)

(type + Hermit (is Crab)
  (prop + body _
    (machine
      (state free
        (then shelled (trigger on ticks (call said "hermit" (== each 1))))
        (blank)
        (state shelled)
      )
    )
  )
)
```

```output
try animal
try crab
try hermit
free, dashing
```

### A handler in a refinement tests the cases the refinement adds.

```brix
(main
  (let crab (call Crab))
  (<- (. crab ticks) 1.0)
  (<- (. crab ticks) 1.0)
  (<- (. crab ticks) 1.0)
  (print (. crab body path))
)

(type + Thing
  (event + ticks F64)
  (prop + body _
    (machine
      (state free)
    )
  )
)

(type + Crab (is Thing)
  (prop + count Size (= 0))
  (prop + body _
    (machine
      (state free
        (for (on dt ticks)
          (+= count 1)
          (print (str count " " (is body .hiding) " " (is body .peeking)))
        )
        (blank)
        (then (call dashing 3.0) (trigger on ticks (== count 1)))
        (blank)
        (state idle)
        (blank)
        (state (call dashing speed)
          (then (call hiding speed) (trigger on ticks))
        )
        (blank)
        (state (call hiding rest)
          (then peeking (trigger on ticks (> rest 1)))
        )
        (blank)
        (state peeking)
      )
    )
  )
)
```

```output
1 false false
2 false false
3 true false
free, peeking
```

### A case test reads a refined machine through an index or a cast, evaluating the object once, for cases the refinement adds and cases the base writes.

```brix
(main
  (let dogs (list (call Dog) (call Dog)))
  (<- (. ([] dogs 0) chases) true)
  (print (str (is (. ([] dogs 0) body) .chasing) " " (is (. ([] dogs 1) body) .chasing) " " (is (. ([] dogs 1) body) .idle)))
  (print (is (. (call pick dogs 0) body) .chasing))
  (let pet Animal (call Dog))
  (<- (. (as pet Dog) calls) true)
  (print (str (is (. (as pet Dog) body) .answering) " " (is (. (as pet Dog) body) .chasing)))
)

(fn-expr + pick Dog
  (param dogs (list Dog))
  (param at Index)
  (before
    (print (str "pick " at))
  )
  ([] dogs at)
)

(type + Animal
  (event + calls Bool)
  (prop + body _
    (machine
      (state free
        (then answering (trigger on calls))
        (blank)
        (state idle)
        (blank)
        (state answering)
      )
    )
  )
)

(type + Dog (is Animal)
  (event + chases Bool)
  (prop + body _
    (machine
      (state free
        (then chasing (trigger on chases))
        (blank)
        (state chasing)
      )
    )
  )
)
```

```output
true false true
pick 0
true
true false
```

### A case two transitions give values holds them where the other is written in a case whose payload is settled only after it.

```brix
(prop + ticks (of AsyncChannel F64))
(prop + taps (of AsyncChannel F64))
(prop + arrivals (of AsyncChannel F64))
(blank)
(prop + crab _
  (machine
    (state free
      (then (call hiding 1.0 1.0) (trigger on taps))
      (then (call dashing 5.0 1.4) (trigger on ticks))
      (blank)
      (state idle)
      (blank)
      (state (call dashing rest glimpse)
        (then (call hiding rest glimpse) (trigger on arrivals))
      )
      (blank)
      (state (call hiding rest glimpse))
    )
  )
)
(blank)

(main
  (<- ticks 1.0)
  (do-new
    (<- arrivals 1.0)
    (do-new
      (print
        (match crab
          (case (call .hiding rest glimpse)
            (str rest " " glimpse)
          )
          (default
            "other"
          )
        )
      )
      (<- taps 1.0)
      (do-new
        (print
          (match crab
            (case (call .hiding rest glimpse)
              (str rest " " glimpse)
            )
            (default
              "other"
            )
          )
        )
      )
    )
  )
)
```

```output
5 1.4
1 1
```

### A top-level machine's path lists its active cases from outer to inner.

```brix
(prop + ready (of AsyncChannel Bool))
(blank)
(prop + flow _
  (machine
    (state waiting
      (state idle)
    )
    (state running (trigger on ready))
  )
)
(blank)

(main
  (print (call (. " > " join) (. flow path)))
  (<- ready true)
  (do-new
    (print (call (. " > " join) (. flow path)))
  )
)
```

```output
waiting > idle
running
```

### A match on a machine picks the case it last entered, while a case test also sees its active parent.

```brix
(prop + editor _
  (machine
    (state editing
      (state focused)
    )
  )
)
(blank)

(main
  (print (is editor .editing))
  (print
    (match editor
      (case .editing
        "editing"
      )
      (case .focused
        "focused"
      )
      (default
        "other"
      )
    )
  )
)
```

```output
true
focused
```

### A state machine a property holds runs on the object, so its cases call the object's methods.

```brix
(main
  (let worker (call Worker))
  (<- (. worker go) true)
  (do-new
    (do-new
      (print (. worker done))
    )
  )
)

(type + Worker
  (prop + done _ (= ""))
  (event + go Bool)
  (prop + phase _
    (machine
      (state idle)
      (state working (trigger on go)
        (do
          (call work "x")
          (call (. this work) "y")
        )
      )
    )
  )
  (fn-do + work _
    (param text String)
    (+= done text)
  )
)
```

```output
xy
```

### A state machine whose case reads an element of its class's markup holds nothing until the markup runs, then starts.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
  (print (is (. panel phase) .idle))
  (<- (. panel advance) true)
  (do-new
    (print (is (. panel phase) .active))
  )
)

(type + Panel (is Box)
  (prop + advance (of AsyncChannel Bool))
  (prop + phase _
    (machine
      (state idle)
      (state active (trigger on advance)
        (lazy shown (. inner text))
        (do
          (print (str "active " shown))
        )
      )
    )
  )
  (content
    (<> This
      (<> Label (id: inner) (text: "hello"))
    )
  )
)
```

```output
true
active hello
true
```

### A state machine written in the content of a class, on events of the object itself, opens, cleans up and closes as the events arrive.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let host (call Host))
  (print (call (. host summary)))
  (<- (. host open-requests) 1)
  (do-new
    (print (call (. host summary)))
    (<- (. host refresh-requests) 2)
    (do-new
      (print (call (. host summary)))
      (<- (. host close-requests) true)
      (do-new
        (print (call (. host summary)))
        (<- (. host open-requests) 3)
        (do-new
          (print (call (. host summary)))
        )
      )
    )
  )
)

(type + Host (is Box)
  (event + open-requests Int)
  (event + refresh-requests Int)
  (event + close-requests Bool)
  (prop + log _ (= ""))
  (fn-expr + summary String
    (str (. state-label text) ":" log)
  )
  (content
    (<> Label (id: state-label)
      (= text (if-expr #is-open "open" "closed"))
    )
    (blank)
    (state is-closed)
    (blank)
    (state is-open (trigger for n open-requests)
      (do
        (= log (+ log (str "[open" n "]")))
      )
      (blank)
      (finally
        (= log (+ log "[unload]"))
      )
      (blank)
      (then is-closed (trigger on close-requests))
    )
  )
  (subscribe-for (on n refresh-requests)
    (if is-open
      (= log (+ log (str "[refresh" n "]")))
    )
  )
)
```

```output
closed:
open:[open1]
open:[open1][refresh2]
closed:[open1][refresh2][unload]
open:[open1][refresh2][unload][open3]
```

### A class's handler and its state machine may both hear an event of the object itself: the handler runs first, then the transition.

```brix
(main
  (let knob (call Knob))
  (<- (. knob pokes) 1)
  (<- (. knob pokes) 2)
  (print (str (is (. knob phase) .poked) " " (. knob log)))
)

(type + Knob
  (event + pokes Int)
  (prop + log _ (= ""))
  (prop + phase _
    (machine
      (state idle)
      (state poked (trigger for n pokes)
        (do
          (+= log (str "[enter " n "]"))
        )
      )
    )
  )
  (subscribe-for (on n pokes)
    (+= log (str "[poke " n "]"))
  )
)
```

```output
true [poke 1][enter 1][poke 2][enter 2]
```

### A machine starts in a case that keeps an object a later transition captures, before any object has arrived.

```brix
(main
  (let knob (call Knob))
  (print (. knob is-idle))
  (<- (. knob downs) (call Press 1))
  (print (. knob is-dragging))
  (<- (. knob ups) (call Press 2))
  (print (str (. knob is-idle) " " (. knob log)))
)

(type + Press
  (prop + id Int)
  (fn-do + This _
    (param id _)
  )
)

(type + Knob
  (event + downs Press)
  (event + ups Press)
  (prop + log _ (= ""))
  (content
    (state is-idle)
    (state is-dragging (trigger for down downs)
      (then is-idle (trigger for up ups)
        (do
          (+= log (str "[" (. down id) " to " (. up id) "]"))
        )
      )
    )
  )
)
```

```output
true
true
true [1 to 2]
```

### A lazy local that the trigger and the transitions of a state machine read is evaluated once: the trigger evaluates its source when the machine starts, and the first transition that runs evaluates the other.

```brix
(main
  (let a (of AsyncChannel Int))
  (let b (of AsyncChannel Int))
  (lazy source (call noted a "source"))
  (lazy label (call noted "L" "label"))
  (blank)
  (state idle)
  (state one (trigger for n source)
    (do
      (print (str "one " n " " label))
    )
  )
  (state two (trigger for n b)
    (do
      (print (str "two " n " " label))
    )
  )
  (blank)
  (print "start")
  (<- a 1)
  (do-new
    (<- b 2)
    (do-new
      (<- a 3)
      (do-new
        (print "end")
      )
    )
  )
)

(fn-do + (of noted Value) Value
  (param value Value)
  (param name String)
  (print (str "evaluated " name))
  (return value)
)
```

```output
evaluated source
start
evaluated label
one 1 L
two 2 L
one 3 L
end
```

### A state machine in the markup of a class that is triggered by a lazy comprehension on an event of the object, whose arms read an element of the markup, enters as the comprehension gives.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let form (call Form))
  (print (call (. form summary)))
  (<- (. form submitted) 3)
  (do-new
    (do-new
      (print (call (. form summary)))
      (<- (. form submitted) -1)
      (do-new
        (do-new
          (print (call (. form summary)))
        )
      )
    )
  )
)

(type + Form (is Box)
  (event + submitted Int)
  (fn-expr + summary String
    (str (. shown text) ":" is-busy)
  )
  (content
    (<> Label (id: shown) (text: "got"))
    (blank)
    (state is-idle)
    (state is-busy (trigger for (call .valid n) checked)
      (do
        (= (. shown text) (str "busy " n))
      )
    )
    (blank)
    (lazy checked
      (iterate (on submitted)
        (block
          (case? (or (< it 0) (== (. shown text) "x"))
            (. Check negative)
          )
          (default
            (call (. Check valid) it)
          )
        )
      )
    )
  )
)

(type + Check (is Union)
  (variant negative)
  (variant valid (tuple Int))
)
```

```output
got:false
busy 3:true
busy 3:true
```

### A named machine written in a case of a class content machine subscribes where the case is entered.

```brix
(main
  (let t (call Thing))
  (<- (. t keys) 'x')
  (do-new
    (print (. t log))
  )
)

(type + Thing
  (event + keys Char)
  (prop + log _ (= ""))
  (content
    (state outer
      (prop + inner _
        (machine
          (state idle
            (do
              (= log (str log " i"))
            )
          )
          (blank)
          (state busy (trigger on keys)
            (do
              (= log (str log " b"))
            )
          )
        )
      )
    )
  )
)
```

```output
 i b
```

### A state machine whose transition waits on an event nothing publishes never takes it, and the cases around it enter and exit as written.

```brix
(main
  (let thing (call Thing))
  (print (. thing label))
  (call (. thing poke))
  (do-new
    (print (. thing label))
    (call (. thing poke))
    (do-new
      (print (. thing label))
      (call (. thing poke))
      (do-new
        (print (. thing label))
      )
    )
  )
)

(type + Thing
  (prop + label _ (= "none"))
  (event + go Bool)
  (event + other Bool)
  (fn-do + poke _
    (<- other true)
  )
  (content
    (state idle
      (do
        (= label "idle")
      )
      (blank)
      (then busy (trigger on other))
    )
    (blank)
    (state busy
      (do
        (= label "busy")
      )
      (blank)
      (then idle (trigger on go))
      (then done (trigger on other))
    )
    (blank)
    (state done
      (do
        (= label "done")
      )
      (blank)
      (then busy (trigger on go))
    )
  )
)
```

```output
idle
busy
done
done
```

### State machines of the same name, written with the same cases in two modules, are machines of their own: each module tests the case of its own.

```brix
(import ./lib/first)
(import ./lib/second)
(blank)

(main
  (call show-first)
  (call show-second)
)
```

```brix file=lib/first.bx
(prop _ auth _
  (machine
    (state editing)
    (state authenticated)
  )
)

(fn-do + show-first _
  (print (str "first " (is auth .editing) " " (is auth .authenticated)))
)
```

```brix file=lib/second.bx
(prop _ has-token _ (= true))
(blank)
(prop _ auth _
  (machine
    (state editing)
    (state authenticated (trigger if has-token))
  )
)

(fn-do + show-second _
  (print (str "second " (is auth .editing) " " (is auth .authenticated)))
)
```

```output
first true false
second false true
```

### An error the subscribe of a group of a state's transitions raises goes to unhandled-error, and the program goes on.

```brix
(import /core/stream)
(blank)

(main
  (= unhandled-error
    (=> (error) :do
      (block
        (print (str "handled " (. error message)))
      )
    )
  )
)

(blank)
(prop + opened (of AsyncChannel Bool))
(prop _ refusing _ (= (call RefusingSource)))
(blank)
(prop + flow _
  (machine
    (state idle)
    (state active (trigger on opened)
      (then finished (trigger on source))
      (blank)
      (lazy source refusing)
    )
    (blank)
    (state finished)
  )
)
(blank)

(main
  (<- opened true)
  (do-new
    (print (is flow .active))
    (print "done")
  )
)

(type + RefusingSource
  (implements (of Broadcastable Bool))
  (fn-expr + is-closed Bool :get
    false
  )
  (fn-do + subscribe _ (?! Error)
    (param subscriber (of Subscriber Bool))
    (throw (call Error "subscribe refused"))
  )
  (fn-do + unsubscribe _ (?! Error)
    (param subscriber (of Subscriber Bool))
  )
  (fn-do + subscribe _ (?! Error)
    (param subscriber (of Subscribable Bool))
  )
  (fn-do + unsubscribe _ (?! Error)
    (param subscriber (of Subscribable Bool))
  )
)
```

```output
handled subscribe refused
true
done
```

### A state payload and its substates can hold a value of an interface type.

```brix
(main
  (let rider (call Rider))
  (let horse (call Horse))
  (<- (. rider saddlings) horse)
  (print (. rider where))
)

(type + Steed (is Interface)
  (fn-expr + speed F64 :get)
)

(type + Horse
  (implements Steed)
  (fn-expr + speed _ :get
    12.0
  )
)

(type + Rider
  (event + saddlings Steed)
  (prop + body _
    (machine
      (state walking)
      (blank)
      (state (call mounted horse) (trigger for horse saddlings)
        (state trotting
          (then galloping (trigger if (> (. horse speed) 10.0)))
        )
        (blank)
        (state galloping)
      )
    )
  )
  (fn-do + where String :get
    (return
      (match body
        (case .galloping
          "galloping"
        )
        (case (call .mounted horse)
          (str "riding at " (. horse speed))
        )
        (default
          "walking"
        )
      )
    )
  )
)
```

```output
galloping
```

### An unconditional entry decision infers the payload type of the state it enters.

```brix
(main
  (let toy (call Toy))
  (print (. toy where))
)

(type + Toy
  (prop + body _
    (machine
      (state idle
        (then (call moving (call spot)))
      )
      (blank)
      (state (call moving goal))
    )
  )
  (fn-do + spot F64
    (return 2.5)
  )
  (fn-do + where String :get
    (return
      (match body
        (case (call .moving goal)
          (str "moving to " goal)
        )
        (default
          "idle"
        )
      )
    )
  )
)
```

```output
moving to 2.5
```

### An unconditional entry decision infers the payload type of a state declared before it.

```brix
(main
  (let toy (call Toy))
  (<- (. toy pokes) true)
  (print (. toy where))
)

(type + Toy
  (event + pokes Bool)
  (prop + body _
    (machine
      (state resting
        (then planning (trigger on pokes))
      )
      (blank)
      (state (call moving goal))
      (blank)
      (state planning
        (then (call moving (call spot)))
      )
    )
  )
  (fn-do + spot F64
    (return 2.5)
  )
  (fn-do + where String :get
    (return
      (match body
        (case (call .moving goal)
          (str "moving to " goal)
        )
        (default
          "resting"
        )
      )
    )
  )
)
```

```output
moving to 2.5
```

### A case its owner's continuation with no trigger enters, so that the owner never enters it as its first case, holds what a transition of its own with a trigger gives it.

```brix
(main
  (let toy (call Toy))
  (print (. toy where))
  (<- (. toy pokes) true)
  (print (. toy where))
)

(type + Toy
  (event + pokes Bool)
  (prop + body _
    (machine
      (state smarting
        (then (call darting (call spot)))
        (blank)
        (state (call darting goal)
          (then (call darting (call farther)) (trigger on pokes))
        )
      )
    )
  )
  (fn-do + spot F64
    (return 2.5)
  )
  (fn-do + farther F64
    (return 4.5)
  )
  (fn-do + where String :get
    (return
      (match body
        (case (call .darting goal)
          (str "darting to " goal)
        )
        (default
          "smarting"
        )
      )
    )
  )
)
```

```output
darting to 2.5
darting to 4.5
```

### Each guarded transition of a case on the same event reads what the case holds, though an earlier transition of the case would have left it.

```brix
(main
  (let counter (call Counter))
  (<- (. counter ticks) 0.1)
  (<- (. counter ticks) 0.1)
  (print (. counter log))
)

(type + Counter
  (event + ticks F64)
  (prop + log _ (= ""))
  (prop + phase _
    (machine
      (state waiting
        (then (call holding 7) (trigger on ticks))
      )
      (blank)
      (state (call holding value)
        (then waiting (trigger on ticks (call seen "a" value)))
        (then waiting (trigger on ticks (call seen "b" value)))
      )
    )
  )
  (fn-do _ seen Bool
    (param label String)
    (param value Int)
    (+= log (str label value ";"))
    (return false)
  )
)
```

```output
a7;b7;
```

### A transition of a case gives the case it enters a value made of what a case above it holds.

```brix
(main
  (let toy (call Toy))
  (<- (. toy ticks) 0.1)
  (<- (. toy ticks) 0.1)
  (print (. toy where))
)

(type + Toy
  (event + ticks F64)
  (prop + body _
    (machine
      (state waiting
        (then (call holding 7) (trigger on ticks))
      )
      (blank)
      (state (call holding value)
        (state early
          (then (call late (call twice value)) (trigger on ticks))
        )
        (blank)
        (state (call late spot))
      )
    )
  )
  (fn-expr _ twice Int
    (param value Int)
    (* value 2)
  )
  (fn-do + where String :get
    (return
      (match body
        (case (call .late spot)
          (str "late at " spot)
        )
        (default
          "elsewhere"
        )
      )
    )
  )
)
```

```output
late at 14
```

### A guard reads its own class machine by name while a case holds a value a later transition gives it.

```brix
(main
  (let knob (call Knob))
  (<- (. knob downs) 1)
  (<- (. knob downs) 2)
  (<- (. knob blurs) true)
  (print (. knob log))
)

(type + Knob
  (event + downs Int)
  (event + blurs Bool)
  (prop + log _ (= ""))
  (prop _ gesture _
    (machine
      (state idle)
      (state dragging (trigger on downs (not (is gesture .dragging)))
        (do
          (inline (+= log "[drag]"))
        )
        (then (call held 7) (trigger on blurs))
      )
      (blank)
      (state (call held id)
        (do
          (inline (+= log (str "[held " id "]")))
        )
      )
    )
  )
)
```

```output
[drag][held 7]
```

### A guard reads its own class machine by name while a case holds a value made from what its trigger captured.

```brix
(main
  (let knob (call Knob))
  (<- (. knob downs) 4)
  (<- (. knob downs) 9)
  (<- (. knob blurs) true)
  (print (. knob log))
)

(type + Knob
  (event + downs Int)
  (event + blurs Bool)
  (prop + log _ (= ""))
  (prop _ gesture _
    (machine
      (state idle)
      (state (call dragging n) (trigger for n downs (not (is gesture .dragging)))
        (do
          (inline (+= log (str "[drag " n "]")))
        )
        (then (call held (+ n 1)) (trigger on blurs))
      )
      (blank)
      (state (call held id)
        (do
          (inline (+= log (str "[held " id "]")))
        )
      )
    )
  )
)
```

```output
[drag 4][held 5]
```

### Cases whose values and kept captures each come from a case the others enter take their types from the triggers that capture them, however the cases enter one another.

```brix
(main
  (let knob (call Knob))
  (<- (. knob downs) (call Press 4))
  (<- (. knob blurs) true)
  (<- (. knob ups) (call Press 4))
  (<- (. knob downs) (call Press 5))
  (print (. knob log))
)

(type + Press
  (prop + id Int)
  (fn-do + This _
    (param id _)
  )
)

(type + Knob
  (event + downs Press)
  (event + ups Press)
  (event + blurs Bool)
  (prop + log _ (= ""))
  (prop _ gesture _
    (machine
      (state idle
        (then (call dragging (. press id)) (trigger for press downs))
      )
      (blank)
      (state (call dragging id)
        (do
          (inline (+= log (str "[drag " id "]")))
        )
        (then (call held id) (trigger on blurs))
      )
      (blank)
      (state (call held id)
        (do
          (inline (+= log (str "[held " id "]")))
        )
        (then idle (trigger for up ups))
      )
    )
  )
)
```

```output
[drag 4][held 4][drag 5]
```

### A case one transition gives a value and another gives null holds an optional of the value, so null stays null.

```brix
(main
  (let holder (call Holder))
  (<- (. holder modifiers) true)
  (<- (. holder codes) 0)
  (print (is (. holder hold) .held))
)

(type + Holder
  (event + codes U32)
  (event + modifiers Bool)
  (prop + hold _
    (machine
      (state idle
        (then (call held code) (trigger for code codes))
        (then (call held null) (trigger on modifiers))
      )
      (blank)
      (state (call held code)
        (then idle (trigger on codes (== each code)))
      )
    )
  )
)
```

```output
true
```

### An object's `close`, its class's own or one it inherits, stops each machine its properties hold: each exits its active cases, running their `do finally`, and no transition fires after, on the object's events or on another stream.

```brix
(main
  (let ticks (of AsyncChannel Int))
  (let holder (call Holder ticks))
  (<- (. holder started) true)
  (print (str "before " (is (. holder phase) .waiting)))
  (call (. holder close))
  (<- (. holder started) true)
  (<- ticks 1)
  (do-new
    (print (str "after " (is (. holder phase) .idle) " " (is (. holder phase) .waiting) " " (is (. holder phase) .done) " " (. holder log)))
  )
)

(type + Closer
  (fn-do + close _)
)

(type + Holder (is Closer)
  (event + started Bool)
  (prop + log _ (= ""))
  (prop _ ticks (of AsyncChannel Int))
  (fn-do + This _
    (param ticks _)
  )
  (prop + phase _
    (machine
      (state idle)
      (blank)
      (state waiting (trigger on started)
        (finally
          (+= log "[left waiting]")
        )
        (then done (trigger on ticks))
      )
      (blank)
      (state done
        (do
          (inline (+= log "[done]"))
        )
      )
    )
  )
)
```

```output
before true
after false false false [left waiting]
```

### An object's `close`, its class's own or one it inherits, stops the machine its class's markup writes: the active case exits, running its `do finally`, and no handler or transition fires after.

```brix
(main
  (let ticks (of AsyncChannel Int))
  (let holder (call Holder ticks))
  (<- (. holder started) true)
  (print (str "before " (. holder waiting)))
  (call (. holder close))
  (<- (. holder started) true)
  (<- ticks 1)
  (do-new
    (print (str "after " (. holder idle) " " (. holder waiting) " " (. holder done) " " (. holder log)))
  )
)

(type + Closer
  (fn-do + close _)
)

(type + Holder (is Closer)
  (event + started Bool)
  (prop + log _ (= ""))
  (prop _ ticks (of AsyncChannel Int))
  (fn-do + This _
    (param ticks _)
  )
  (content
    (state idle)
    (state waiting (trigger on started)
      (finally
        (+= log "[left waiting]")
      )
      (for (on tick ticks)
        (+= log (str "[tick " tick "]"))
      )
      (then done (trigger on ticks))
    )
    (blank)
    (state done
      (do
        (inline (+= log "[done]"))
      )
    )
  )
)
```

```output
before true
after false false false [left waiting]
```

### An error a case's `do finally` raises as an object's `close` stops its machine goes to unhandled-error, and the close goes on.

```brix
(main
  (= unhandled-error
    (=> (error) :do
      (block
        (print (str "handled " (. error message)))
      )
    )
  )
  (blank)
  (let holder (call Holder))
  (<- (. holder opens) true)
  (call (. holder close))
  (print (str "after " (. holder is-open) " " (. holder log)))
)

(type + Holder
  (event + opens Bool)
  (prop + log _ (= ""))
  (content
    (state is-closed)
    (state is-open (trigger on opens)
      (finally
        (call unload)
      )
    )
  )
  (fn-do + unload _ (?! Error)
    (+= log "[unload]")
    (throw (call Error "unload failed"))
  )
  (fn-do + close _)
)
```

```output
handled unload failed
after false [unload]
```

### An object's `close` stops its machines before its body runs, so their exit rows see what the body then closes.

```brix
(main
  (let holder (call Holder))
  (<- (. holder opens) true)
  (call (. holder close))
  (print (. holder log))
)

(type + Holder
  (event + opens Bool)
  (prop + resource (? String) (= "file"))
  (prop + log _ (= ""))
  (prop + phase _
    (machine
      (state idle)
      (blank)
      (state open (trigger on opens)
        (finally
          (+= log (str "[left open with " (?? resource "nothing") "]"))
        )
      )
    )
  )
  (fn-do + close _
    (+= log "[close]")
    (= resource null)
  )
)
```

```output
[left open with file][close]
```

### An object's `close` stops its machines before its body runs, and the body still publishes the object's events to its own handlers and to outside subscribers, but not to the stopped machines.

```brix
(main
  (let holder (call Holder))
  (for (on value (. holder ended))
    (+= (. holder log) "[outside heard]")
  )
  (<- (. holder opens) true)
  (call (. holder close))
  (print (. holder log))
)

(type + Holder
  (event + opens Bool)
  (event + ended Bool)
  (prop + log _ (= ""))
  (prop + phase _
    (machine
      (state idle)
      (blank)
      (state open (trigger on opens)
        (do
          (inline (+= log "[open]"))
        )
        (finally
          (+= log "[left open]")
        )
      )
      (blank)
      (state shut (trigger on ended)
        (do
          (inline (+= log "[shut]"))
        )
      )
    )
  )
  (subscribe-for (on value ended)
    (+= log "[heard ended]")
  )
  (fn-do + close _
    (+= log "[close]")
    (<- opens true)
    (<- ended true)
  )
)
```

```output
[open][left open][close][heard ended][outside heard]
```

### A named machine a subclass's refinement writes in a case the base writes starts as the case is entered, its transitions fire while the case is active, and it stops as the case exits.

```brix
(main
  (let sub (call Sub))
  (<- (. sub opens) true)
  (<- (. sub pokes) 7)
  (print (. sub label))
  (<- (. sub closes) true)
  (print (. sub label))
)

(type + Base
  (event + opens Bool)
  (event + closes Bool)
  (prop + presence _
    (machine
      (state closed
        (then open (trigger on opens))
      )
      (blank)
      (state open
        (then closed (trigger on closes))
      )
    )
  )
)

(type + Sub (is Base)
  (prop + label _ (= ""))
  (event + pokes Int)
  (prop + presence _
    (machine
      (state open
        (prop _ inner _
          (machine
            (state idle)
            (blank)
            (state (call busy n) (trigger for n pokes)
              (do
                (= label (str "busy " n))
              )
              (blank)
              (finally
                (= label (str "left busy " n))
              )
            )
          )
        )
      )
    )
  )
)
```

```output
busy 7
left busy 7
```

### A subclass's anonymous machine whose cases name a case at the root of the anonymous machine of the class above refines that machine, as a named machine refines its base's: its cases nest under those it names again, and the cases' flags stay the base's.

```brix
(main
  (let sub (call Sub))
  (<- (. sub opens) true)
  (<- (. sub pokes) 7)
  (print (str "opened " (. sub is-open) " " (. sub label)))
  (<- (. sub closes) true)
  (print (str "closed " (. sub is-open) " " (. sub is-closed) " " (. sub label)))
)

(type + Base
  (event + opens Bool)
  (event + closes Bool)
  (content
    (state is-closed
      (then is-open (trigger on opens))
    )
    (blank)
    (state is-open
      (then is-closed (trigger on closes))
    )
  )
)

(type + Sub (is Base)
  (prop + label _ (= ""))
  (event + pokes Int)
  (content
    (state is-open
      (prop _ inner _
        (machine
          (state idle)
          (blank)
          (state (call busy n) (trigger for n pokes)
            (do
              (= label (str "busy " n))
            )
            (blank)
            (finally
              (= label (str "left busy " n))
            )
          )
        )
      )
    )
  )
)
```

```output
opened true busy 7
closed false true left busy 7
```

### A refinement of an anonymous machine adds no case at its root.

```brix crash
(main
  (let sub (call Sub))
  (print (. sub is-open))
)

(type + Base
  (event + opens Bool)
  (content
    (state is-closed
      (then is-open (trigger on opens))
    )
    (blank)
    (state is-open)
  )
)

(type + Sub (is Base)
  (content
    (state is-open
      (do
        (print "open")
      )
    )
    (blank)
    (state is-marked)
  )
)
```

```output
Exception: Refinement cannot add top-level state case 'is-marked'
<no file:26>
        (state is-marked)
        ^^^^^^^^^^^^^^^^^
```

### A refinement that writes a case's trigger again is rejected, as the case keeps the trigger it has.

```brix crash
(main
  (let s (call Sub))
)

(type + Base
  (event + go Bool)
  (prop + body _
    (machine
      (state free)
      (state busy (trigger on go))
    )
  )
)

(type + Sub (is Base)
  (prop + body _
    (machine
      (state busy (trigger on go))
    )
  )
)
```

```output
Exception: Refined state case 'busy' cannot change its trigger or payload
<no file:19>
          (state busy (trigger on go))
          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A case a refinement adds under a case may come first with a payload, as the base's first case is the one entered there.

```brix
(main
  (let pet (call Dog))
  (print (. pet body))
  (<- (. pet chases) "cat")
)

(type + Animal
  (event + calls String)
  (prop + body _
    (machine
      (state free
        (then (call answering who) (trigger for who calls))
        (blank)
        (state idle)
        (blank)
        (state (call answering who)
          (do
            (print (str "answering " who))
          )
        )
      )
    )
  )
)

(type + Dog (is Animal)
  (event + chases String)
  (prop + body _
    (machine
      (state free
        (then (call chasing quarry) (trigger for quarry chases))
        (blank)
        (state (call chasing quarry)
          (do
            (print (str "chasing " quarry))
          )
        )
      )
    )
  )
)
```

```output
idle
chasing cat
```

### A machine's first case is entered with no transition to give it a payload, so a name it holds is rejected.

```brix crash
(main
  (let pet (call Animal))
)

(type + Animal
  (event + calls String)
  (prop + body _
    (machine
      (state free
        (then (call answering who) (trigger for who calls))
        (blank)
        (state (call answering who))
      )
    )
  )
)
```

```output
Exception: Initial state case 'answering' cannot take a payload, since nothing enters it to give it one; give it values, or write a case with no payload before it
<no file:13>
            (state (call answering who))
                                   ^^^
```

### An inherited anonymous machine's root transition on an event of its own class fires on an object of a subclass.

```brix
(main
  (let sub (call SubProbe))
  (call (. sub request))
  (print (. sub is-busy))
)

(type + Probe
  (event _ requested Bool)
  (content
    (state is-idle)
    (state is-busy (trigger on requested))
  )
  (fn-do + request _
    (<- requested true)
  )
)

(type + SubProbe (is Probe))
```

```output
true
```

### A subclass's anonymous machine written after a long section of markup refines the base's.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let made (call Mid))
  (<- (. made opens) true)
  (print (str "opened " (. made is-open) " " (. made label)))
)

(type + Base (is Box)
  (event + opens Bool)
  (content
    (state is-closed
      (then is-open (trigger on opens))
    )
    (state is-open)
  )
)

(type + Mid (is Base)
  (prop + label _ (= ""))
  (content
    (<> This
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
      (<> Item)
    )
    (state is-open
      (do
        (= label "entered")
      )
    )
  )
)

(type + Item (is Box))
```

```output
opened true entered
```

### A case keeps a named tuple a transition that enters it captures. Entered with nothing captured, it keeps the tuple its fields' first values make.

```brix
(main
  (let holder (call Holder))
  (<- (. holder poked) true)
  (<- (. holder requested) true)
  (<- (. holder committed) (call Commit "x" false))
  (<- (. holder poked) true)
  (print (. holder log))
)

(type _ Commit (is Tuple)
  (prop + name String)
  (prop + flag _ (= true))
)

(type + Holder
  (prop + log _ (= ""))
  (event + requested Bool)
  (event + committed Commit)
  (event + poked Bool)
  (content
    (state is-idle
      (for (on _ poked)
        (+= log (str "[" (. commit name) ":" (. commit flag) "]"))
      )
    )
    (state is-renaming (trigger on requested)
      (then is-idle (trigger for commit committed))
    )
  )
)
```

```output
[:true][x:false]
```

### A name a transition captures hides a method of the class of that name.

```brix
(main
  (let holder (call Holder))
  (<- (. holder requested) true)
  (<- (. holder committed) (call Commit "x" true))
  (print (. holder log))
)

(type _ Commit (is Tuple)
  (prop + name String)
  (prop + flag Bool)
)

(type + Holder
  (prop + log _ (= ""))
  (event + requested Bool)
  (event + committed Commit)
  (content
    (state is-idle)
    (state is-renaming (trigger on requested)
      (then is-idle (trigger for commit committed)
        (do
          (+= log (str "[" (. commit name) "]"))
        )
      )
    )
  )
  (fn-do + commit _
    (+= log "[method]")
  )
)
```

```output
[x]
```

### A transition on a routed event runs while the event is at the machine's owner, before the route moves on, so it reads the owner's frame and its cancel stops the route there.

```brix
(import /core/intrinsics)
(blank)

(main
  (let root (call RoutedNode "root"))
  (let leaf (call HoldingNode "leaf" root))
  (blank)
  (for (on event (. root changed))
    (print (str "root " (. event value)))
  )
  (blank)
  (print (str "canceled " (call (. leaf route) 1)))
  (print (str "canceled " (call (. leaf route) 2)))
)

(type + RoutedChange
  (implements RoutedEvent)
  (prop + value Int)
  (prop + current-name String)
  (prop + is-canceled _ (= false))
  (fn-do + This _
    (param value _)
    (param current-name _)
  )
  (fn-do + cancel _
    (= is-canceled true)
  )
  (fn-do + next-event-owner (? Pointer)
    (param raw-current Pointer)
    (let current (call (of gc-object-from-pointer RoutedNode) raw-current))
    (let next (. current parent))
    (if next
      (= current-name (. next name))
      (return (as next Pointer))
    )
  )
)

(type + RoutedNode
  (prop + name String)
  (prop + parent (? RoutedNode))
  (event + changed RoutedChange)
  (fn-do + This _
    (param name _)
    (param parent _ (= null))
  )
  (fn-do + route Bool
    (param value Int)
    (let event (call RoutedChange value name))
    (<- changed event)
    (return (. event is-canceled))
  )
)

(type + HoldingNode (is RoutedNode)
  (content
    (state is-idle)
    (state is-held (trigger for press changed (not is-held))
      (do
        (print (str "held " (. press value) " " (. press current-name)))
      )
      (blank)
      (then is-idle (trigger for release changed)
        (do
          (print (str "released " (. release value) " " (. release current-name)))
          (call (. release cancel))
        )
      )
    )
  )
)
```

```output
held 1 leaf
root 1
canceled false
released 2 leaf
canceled true
```

### A routed event that another task publishes while a transition waits in the middle of a drain stays held after that task ends, until the drain dispatches it to machines.

```brix
(import /core/intrinsics)
(blank)

(main
  (let leaf (call RoutedNode "leaf" (call RoutedNode "root")))
  (let done (call (of SyncBufferedChannel Bool)))
  (let holder (call Holder done))
  (blank)
  (for (on event (. (!! (. leaf parent)) changed))
    (print (str "root " (. event value)))
  )
  (blank)
  (do-new
    (print (str "canceled " (call (. leaf route) 1)))
    (do-new
      (<- done true)
    )
  )
  (blank)
  (<- (. holder requested) true)
  (print "settled")
)

(type + Holder
  (prop + done (of SyncBufferedChannel Bool))
  (event + requested Bool)
  (fn-do + This _
    (param done _)
  )
  (content
    (state is-idle)
    (state is-waiting (trigger on requested)
      (do
        (let ignored (<- done))
        (print "resumed")
      )
    )
  )
)

(type + RoutedChange
  (implements RoutedEvent)
  (prop + value Int)
  (prop + is-canceled _ (= false))
  (fn-do + This _
    (param value _)
  )
  (fn-do + cancel _
    (= is-canceled true)
  )
  (fn-do + next-event-owner (? Pointer)
    (param raw-current Pointer)
    (let next (. (call (of gc-object-from-pointer RoutedNode) raw-current) parent))
    (if next
      (return (as next Pointer))
    )
  )
)

(type + RoutedNode
  (prop + name String)
  (prop + parent (? RoutedNode))
  (event + changed RoutedChange)
  (fn-do + This _
    (param name _)
    (param parent _ (= null))
  )
  (fn-do + route Bool
    (param value Int)
    (let event (call RoutedChange value))
    (<- changed event)
    (return (. event is-canceled))
  )
)
```

```output
root 1
canceled false
resumed
settled
```

### A case a subclass's refinement adds, tested where the program constructs no object of the subclass, is not active, and an arm that names it matches nothing.

```brix
(main
  (let toy (call Toy))
  (print (is (. toy body) .free))
  (let turtles (list Turtle))
  (for (in turtle turtles)
    (print (str (. turtle is-riding) " " (. turtle raft)))
  )
  (print (. turtles length))
)

(type + Toy
  (prop + body _
    (machine
      (state free)
    )
  )
)

(type + Turtle (is Toy)
  (event + boards String)
  (prop + body _
    (machine
      (state free
        (state (call riding name) (trigger for name boards))
      )
    )
  )
  (fn-do + is-riding Bool :get
    (return (is body .riding))
  )
  (fn-do + raft String :get
    (return
      (match body
        (case (call .riding name)
          name
        )
        (default
          "none"
        )
      )
    )
  )
)
```

```output
true
0
```

### A case test and path on a copy of a machine read the case the copy holds, not the machine's.

```brix
(prop + go (of AsyncChannel Bool))
(blank)
(prop + m _
  (machine
    (state a
      (state b)
    )
    (state c (trigger on go))
  )
)
(blank)

(main
  (let before m)
  (let door (call Door))
  (let shut (. door phase))
  (<- go true)
  (<- (. door open-requests) "ada")
  (do-new
    (print (str (is before .a) " " (is before .b) " " (is before .c) " " (. before path)))
    (print (str (is shut .closed) " " (is shut .open) " " (is (. door phase) .open)))
  )
)

(type + Door
  (event + open-requests String)
  (prop + phase _
    (machine
      (state closed
        (then (call open each) (trigger on open-requests))
      )
      (blank)
      (state (call open who))
    )
  )
)
```

```output
true true false a, b
true false true
```

### Assigning a state machine is rejected, as it moves only by its transitions.

```brix crash
(prop + phase _
  (machine
    (state idle)
    (state busy)
  )
)
(blank)

(main
  (= phase .busy)
)
```

```output
Exception: State machine "phase" moves only by its transitions, so it cannot be assigned; add a transition that enters the case
<no file:11>
      (= phase .busy)
      ^^^^^^^^^^^^^^^
```

### Assigning the state machine of an object is rejected before its case is looked for.

```brix crash
(main
  (let door (call Door))
  (= (. door phase) .open)
)

(type + Door
  (event + pushes Bool)
  (prop + phase _
    (machine
      (state shut
        (then open (trigger on pushes))
      )
      (blank)
      (state open)
    )
  )
)
```

```output
Exception: State machine "phase" moves only by its transitions, so it cannot be assigned; add a transition that enters the case
<no file:4>
      (= (. door phase) .open)
      ^^^^^^^^^^^^^^^^^^^^^^^^
```

### A when rule that tests a case of the machine runs when the machine enters the case.

```brix
(main
  (let door (call Door))
  (<- (. door toggles) true)
  (<- (. door toggles) true)
  (<- (. door toggles) true)
  (do-new
    (print (. door log))
  )
)

(type + Door
  (event + toggles Bool)
  (prop + log _ (= ""))
  (when-rule (is #phase .open)
    (+= log "[open]")
  )
  (prop + phase _
    (machine
      (state closed
        (then open (trigger on toggles))
      )
      (blank)
      (state open
        (then closed (trigger on toggles))
      )
    )
  )
)
```

```output
[open][open]
```

### A state machine's handler that returns a value with "=" is rejected, as it gives none.

```brix crash
(prop + a (of AsyncChannel Int))
(blank)
(prop + phase _
  (machine
    (state idle
      (for (on x a)
        (return 3)
      )
    )
  )
)
```

```output
Exception: A state machine's handler gives no value, so it cannot return one with "="; end it early with "= alone"
<no file:8>
            (return 3)
            ^^^^^^^^^^
```

### A transition that gives a case fewer values than the case names is rejected.

```brix crash
(prop + go (of AsyncChannel Int))
(blank)
(prop + m _
  (machine
    (state idle
      (then held (trigger on go))
    )
    (blank)
    (state (call held v))
  )
)
```

```output
Exception: Case "held" holds one value, and this transition gives none; give it one value
<no file:7>
          (then held (trigger on go))
          ^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A transition that gives a case more values than the case names is rejected.

```brix crash
(prop + go (of AsyncChannel Int))
(blank)
(prop + m _
  (machine
    (state idle
      (then (call held each each) (trigger on go))
    )
    (blank)
    (state (call held v))
  )
)
```

```output
Exception: Case "held" holds one value, and this transition gives 2; give it one value
<no file:7>
          (then (call held each each) (trigger on go))
          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A class's machine whose initial case holds a value starts in it with the value it is given.

```brix
(main
  (let c (call Counter 4))
  (print (. c phase))
)

(type + Counter
  (prop + start Int)
  (fn-do + This _
    (param start _)
  )
  (prop + phase _
    (machine
      (state (call counting (* start 2)))
      (state done)
    )
  )
)
```

```output
8
```

### A case of a machine in a class's markup named like a property the class declares is rejected.

```brix crash
(main
  (let p (call P))
  (print (. p idle))
)

(type + P
  (prop + idle _ (= 3))
  (content
    (state idle)
    (state busy)
  )
)
```

```output
Exception: A case of a machine in a class's markup is read as a property of its name, and the class declares "idle" already; rename the case or the property
<no file:10>
        (state idle)
        ^^^^^^^^^^^^
```

### A message names a machine's union by its cases.

```brix crash
(prop + phase _
  (machine
    (state closed)
    (state open)
  )
)
(blank)

(main
  (print
    (match phase
      (case .closed
        1
      )
    )
  )
)
```

```output
Exception: This match on state machine (.closed | .open) does not handle .open; add a case for it, or "(default …)" for the rest
<no file:13>
          (case .closed
          ^^^^^^^^^^^^
```

### A case a match arm names that a machine does not have is reported with the machine's cases.

```brix crash
(prop + phase _
  (machine
    (state idle)
    (state busy)
  )
)
(blank)

(main
  (print
    (match phase
      (case .nope
        1
      )
      (default
        2
      )
    )
  )
)
```

```output
Exception: Case ".nope" not found in state machine (.idle | .busy)
<no file:13>
          (case .nope
                ^^^^^
```

### A machine compared with one of its cases by "==" is pointed to "is", which counts nested cases.

```brix crash
(prop + phase _
  (machine
    (state closed)
    (state open)
  )
)
(blank)

(main
  (print (== phase .closed))
)
```

```output
Exception: A state machine's case is tested with "is" or "is not", which count the cases nested in it; write "is .closed" in place of "== .closed"
<no file:11>
      (print (== phase .closed))
                       ^^^^^^^
```

### A trigger pattern naming a case its stream's items cannot have is reported.

```brix crash
(prop + nums (of AsyncChannel Int))
(blank)
(prop + m _
  (machine
    (state idle)
    (state b (trigger for (call .valid x) nums))
  )
)
```

```output
Exception: Case ".valid" not found: I64 has no cases
<no file:7>
        (state b (trigger for (call .valid x) nums))
                              ^^^^^^^^^^^^^^^
```

### A whenever trigger moves a machine as what its value reads with "#" changes, while its case is active. Entering the case, storing the value a property already holds, or a change that leaves the value as it was moves nothing, and a change while another case is active is dropped. The value is read only while its case is active.

```brix
(main
  (let w (call Watcher))
  (= (. w feed) 5)
  (<- (. w go) true)
  (do-new
    (print (str "entered " (is (. w flow) .watching)))
    (= (. w feed) 5)
    (do-new
      (print (str "same value " (is (. w flow) .watching)))
      (= (. w feed) 4)
      (do-new
        (print (str "same result " (is (. w flow) .watching)))
        (= (. w extra) 3)
        (do-new
          (print (str "changed " (is (. w flow) .idle)))
          (= (. w feed) 9)
          (do-new
            (print (str "while idle " (is (. w flow) .idle)))
            (<- (. w go) true)
            (do-new
              (print (str "entered again " (is (. w flow) .watching)))
              (= (. w feed) 1)
              (do-new
                (print (str "again " (is (. w flow) .idle)))
              )
            )
          )
        )
      )
    )
  )
)

(type + Watcher
  (event + go Bool)
  (prop + feed _ (= 1))
  (prop + extra _ (= 0))
  (prop + flow _
    (machine
      (state idle)
      (state watching (trigger on go)
        (then idle (trigger whenever (> (call noted (+ #feed #extra)) 6)))
      )
    )
  )
)

(fn-do + noted Int
  (param n Int)
  (print (str "read " n))
  (return n)
)
```

```output
read 5
entered true
same value true
read 4
same result true
read 7
changed true
while idle true
read 12
entered again true
read 4
again true
```

### A whenever trigger in a machine a function holds in a local follows another object's property.

```brix
(main
  (call run)
)

(fn-do + run _
  (let box (call Box))
  (let flow
    (machine
      (state watching
        (then done (trigger whenever (. box #feed)))
      )
      (state done
        (do
          (print "done")
        )
      )
    )
  )
  (print (is flow .watching))
  (do-new
    (= (. box feed) 2)
  )
)

(type + Box
  (prop + feed _ (= 1))
)
```

```output
true
done
```

### A whenever trigger that reads nothing with "#" is reported, as it would never fire.

```brix crash
(main
  (let w (call Watcher))
)

(type + Watcher
  (prop + feed _ (= 1))
  (prop + flow _
    (machine
      (state idle)
      (state watching
        (then idle (trigger whenever feed))
      )
    )
  )
)
```

```output
Exception: A "whenever" trigger fires as what it reads with "#" changes, and this one reads nothing with "#", so it would never fire; write "#" before what it follows, as in "whenever #feed"
<no file:12>
            (then idle (trigger whenever feed))
                       ^^^^^^^^^^^^^^^^^^^^^^^
```
