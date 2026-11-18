# State machines

### Declaration-bound machines expose a union-valued state initialized to the first untriggered case.

```yumi
+ auth ~>
    ~> idle
    ~> authenticating
    ~> authenticated(token)

-- auth is .idle
-- auth is .authenticating
-- auth is .authenticated
```

```output
true
false
false
```

### Top-level machines expose generated boolean projections for payloadless cases.

```yumi
~> idle
~> ready

-- idle
-- ready
```

```output
true
false
```

### Re-entering a class machine queues exactly one lowered activation per event publish

```yumi
. source = EventReentryMachine()
source.enter <- true
do new
    -- source.entries

    source.leave <- true
    do new
        -- source.flow is .idle

        source.enter <- true
        do new
            -- source.entries

━━━━━━━━━━━━━━━━━━━━
+ EventReentryMachine

+ event enter Bool
+ event leave Bool
+ entries = 0

────────────────────
+ flow ~>

~> idle
~> active on enter
    do
        entries += 1

    then idle on leave
```

```output
1
true
2
```

### Top-level machines expose generated projections backed by one hidden mutually exclusive state.

```yumi
+ submitted AsyncChannel'(String, String)

~> idle
~> authenticating(email, password) for (email, password) on submitted
~> authenticated(token)

-- idle
-- authenticating

submitted <- ("joe@example.com", "secret")

do new
    -- idle
    -- authenticating
```

```output
true
unknown
false
(joe@example.com, secret)
```

### Immediate if transitions are evaluated when the owning machine enters.

```yumi
+ has-token = true

+ auth ~>
    ~> editing
    ~> authenticated if has-token

-- auth is .authenticated
```

```output
true
```

### Source order decides between multiple immediate transitions from the same source.

```yumi
+ allow-admin = true
+ allow-user = true

+ auth ~>
    ~> guest
    ~> admin if allow-admin
    ~> user if allow-user

-- auth is .admin
-- auth is .user
```

```output
true
false
```

### Immediate transitions exit the initial state and run entry/finally actions in order.

```yumi
+ has-token = true

+ auth ~>
    ~> editing
        do
            -- "enter editing"

        do finally
            -- "exit editing"

    ~> authenticated if has-token
        do
            -- "enter authenticated"

-- auth is .authenticated
```

```output
enter editing
exit editing
enter authenticated
true
```

### Immediate structure-match if transitions can select a case on entry.

```yumi
+ login-state LoginState = .valid

+ auth ~>
    ~> editing
    ~> authenticating if | .valid = login-state

-- auth is .authenticating

━━━━━━━━━━━━━━━━━━━━
+ LoginState is Union

| valid
| invalid
```

```output
true
```

### Immediate fallbacks clear their target when no immediate transition matches.

```yumi
+ has-token = false

~> authenticating
    ~> authenticated if has-token
    ?? authenticating

-- authenticating
-- authenticated
```

```output
false
false
```

### Self-targeting immediate fallbacks exit once and do not re-enter the same state.

```yumi
+ should-finish = false

~> loading
    do
        -- "enter loading"

    do finally
        -- "exit loading"

    ~> finished if should-finish
    ?? loading

-- "after fallback"
-- loading
-- finished
```

```output
enter loading
exit loading
after fallback
false
false
```

### Immediate fallbacks after then continuations clear their target when no continuation matches.

```yumi
+ is-authentic = false

~> authenticating
    then authenticated if is-authentic
    ?? authenticating

~> authenticated

-- authenticating
-- authenticated
```

```output
false
false
```

### Immediate fallbacks exit an active target subtree before clearing the target.

```yumi
+ is-done = false

~> loading
    do finally
        -- "exit loading"

    ~> spinner
        do finally
            -- "exit spinner"

    ~> finished if is-done
    ?? loading

-- loading
-- spinner
-- finished
```

```output
exit spinner
exit loading
false
false
false
```

### Rejects duplicate state case declarations before lowering.

```yumi crash
+ auth ~>
    ~> idle
    ~> idle
```

```output
Exception: Redefinition of state case 'idle'
<no file:4>
        ~> idle
        ^^^^^^^
```

### Event transitions update the active case asynchronously and capture emitted payload data.

```yumi
+ submitted AsyncChannel'String

+ auth ~>
    ~> idle
    ~> authenticating(email) for email on submitted

submitted <- "joe@example.com"

do new
    -- auth is .authenticating
    -- match auth
        | .authenticating(email)
            email
        ??
            "none"
```

```output
true
joe@example.com
```

### for-pattern transitions destructure union payloads and use captures in target tuple data.

```yumi
+ login-submitted AsyncChannel'LoginSubmission

+ auth ~>
    ~> idle
    ~> authenticating(email, password) for .valid(email, password) on login-submitted
    ~> invalid-email for .invalid-email on login-submitted
    ~> invalid-password for .invalid-password on login-submitted

login-submitted <- .invalid-email

do new
    -- auth is .invalid-email

    login-submitted <- .valid("joe@example.com", "secret")
    do new
        -- match auth
            | .authenticating(email, password)
                "$email/$password"
            ??
                "none"

━━━━━━━━━━━━━━━━━━━━
+ LoginSubmission is Union

| invalid-email
| invalid-password
| valid(String, String)
```

```output
true
joe@example.com/secret
```

### when guards filter event transitions using each from the trigger stream.

```yumi
+ results AsyncChannel'AuthResult

+ auth ~>
    ~> authenticating
    ~> authenticated when each is .authentic on results
    ~> rejected when each is .incorrect-password on results

results <- .rate-limited

do new
    -- auth is .authenticating

    results <- .authentic
    do new
        -- auth is .authenticated

━━━━━━━━━━━━━━━━━━━━
+ AuthResult is Union

| authentic
| incorrect-password
| rate-limited
```

```output
true
true
```

### for-pattern plus when evaluates the pattern before the predicate and exposes captures to both.

```yumi
+ submitted AsyncChannel'LoginSubmission

+ auth ~>
    ~> idle
    ~> authenticating(email, password) for .valid(email, password) on submitted when email.find("@")
    ~> rejected for .valid(email, password) on submitted when !email.find("@")

submitted <- .valid("bad", "secret")

do new
    -- auth is .rejected

    submitted <- .valid("joe@example.com", "secret")
    do new
        -- match auth
            | .authenticating(email, password)
                "$email/$password"
            ??
                "none"

━━━━━━━━━━━━━━━━━━━━
+ LoginSubmission is Union

| valid(String, String)
```

```output
true
joe@example.com/secret
```

### Source order decides between multiple sibling transitions triggered by the same event.

```yumi
+ submitted AsyncChannel'Int

+ flow ~>
    ~> idle
    ~> first on submitted when each > 0
    ~> second on submitted when each > 0

submitted <- 1

do new
    -- flow is .first
    -- flow is .second
```

```output
true
false
```

### A fallback belongs to its transition decision group and clears its target when no earlier case matches.

```yumi
+ results AsyncChannel'AuthResult

~> authenticating
    then authenticated for .authentic on results
    then rejected for .incorrect-password on results
    ?? authenticating

~> authenticated
~> rejected

results <- .rate-limited

do new
    -- authenticating
    -- authenticated
    -- rejected

━━━━━━━━━━━━━━━━━━━━
+ AuthResult is Union

| authentic
| incorrect-password
| rate-limited
```

```output
false
false
false
```

### then continuations can consume streams declared in the owning state block.

```yumi
+ submitted AsyncChannel'(String, String)
+ auth-results AsyncChannel'AuthResult
+ direct-results AsyncChannel'String

+ auth ~>
    ~> idle
    ~> authenticating(email, password) for (email, password) on submitted
        then authenticated(token) for .authentic(token) on auth-result
            do
                -- "continued $token"

        then rejected for .incorrect-password on auth-result

        .. auth-result = auth-results

        do finally
            -- "left authenticating"

    ~> authenticated(value) for value on direct-results
        do
            -- "entered $value"

submitted <- ("joe@example.com", "secret")
do new
    auth-results <- .authentic("token-123")
    do new
        -- match auth
            | .authenticated(token)
                token
            ??
                "none"

━━━━━━━━━━━━━━━━━━━━
+ AuthResult is Union

| authentic(String)
| incorrect-password
```

```output
left authenticating
continued token-123
entered token-123
token-123
```

### then continuations can use captures from a source state without payload data.

```yumi
+ submitted AsyncChannel'LoginSubmission
+ results AsyncChannel'AuthResult
+ selected = ""

+ auth ~>
    ~> idle
    ~> authenticating for .valid(email, password) on submitted
        then authenticated(email, password, token) for .authentic(token) on auth-result

        .. pair = "$email/$password"
        .. auth-result = results

        do
            selected = pair

submitted <- .valid("joe@example.com", "secret")
do new
    -- selected

    results <- .authentic("token-123")
    do new
        -- match auth
            | .authenticated(email, password, token)
                "$email/$password/$token"
            ??
                "none"

━━━━━━━━━━━━━━━━━━━━
+ LoginSubmission is Union

| valid(String, String)

━━━━━━━━━━━━━━━━━━━━
+ AuthResult is Union

| authentic(String)
```

```output
joe@example.com/secret
joe@example.com/secret/token-123
```

### then continuation captures without payload data can drive nested transition sources.

```yumi
+ submitted AsyncChannel'LoginSubmission
+ results AsyncChannel'AuthResult

+ auth ~>
    ~> idle
    ~> authenticating for .valid(email, password) on submitted
        then authenticated for .authentic(token) on auth-result
            then finished for char on token

        .. auth-result = results

submitted <- .valid("joe@example.com", "secret")
do new
    results <- .authentic("token-123")
    do new
        -- auth is .finished

━━━━━━━━━━━━━━━━━━━━
+ LoginSubmission is Union

| valid(String, String)

━━━━━━━━━━━━━━━━━━━━
+ AuthResult is Union

| authentic(String)
```

```output
true
```

### Events from a source state after that state exits are ignored by that state's continuations.

```yumi
+ submitted AsyncChannel'(String, String)
+ cancelled AsyncChannel'Bool
+ auth-results AsyncChannel'AuthResult

+ auth ~>
    ~> idle
    ~> authenticating(email, password) for (email, password) on submitted
        then authenticated(token) for .authentic(token) on auth-result
        then idle on cancelled

        .. auth-result = auth-results

submitted <- ("joe@example.com", "secret")
do new
    cancelled <- true
    auth-results <- .authentic("stale-token")
    do new
        -- auth is .idle
        -- auth is .authenticated

━━━━━━━━━━━━━━━━━━━━
+ AuthResult is Union

| authentic(String)
```

```output
true
false
```

### State-local broadcastable sources detach from the exact source, and stale activation callbacks cannot transition a later activation.

```yumi
+ unique-open AsyncChannel'Bool
+ unique-close AsyncChannel'Bool
_ unique-source-factory = UniqueSourceFactory()

+ unique-flow ~>
    ~> unique-idle
    ~> unique-active on unique-open
        then unique-finished on source
        then unique-idle on unique-close

        .. source = unique-source-factory.make()

    ~> unique-finished

+ shared-open AsyncChannel'Bool
+ shared-close AsyncChannel'Bool
_ shared-source-factory = SharedSourceFactory()

+ shared-flow ~>
    ~> shared-idle
    ~> shared-active on shared-open
        then shared-finished on source
        then shared-idle on shared-close

        .. source = shared-source-factory.get()

    ~> shared-finished

unique-open <- true
do new
    unique-close <- true
    do new
        unique-open <- true
        do new
            -- unique-source-factory.sources.length
            -- unique-source-factory.sources[0].subscription-count
            -- unique-source-factory.sources[0].unsubscription-count
            -- unique-source-factory.sources[0].subscriber-count
            -- unique-source-factory.sources[1].subscriber-count

            unique-source-factory.sources[0].emit-current(true)
            do new
                -- unique-flow is .unique-active

                unique-source-factory.sources[1].emit-current(true)
                do new
                    -- unique-flow is .unique-finished
                    -- unique-source-factory.sources[1].unsubscription-count
                    -- unique-source-factory.sources[1].subscriber-count

                    shared-open <- true
                    do new
                        shared-close <- true
                        do new
                            shared-open <- true
                            do new
                                -- shared-source-factory.calls
                                -- shared-source-factory.source.subscription-count
                                -- shared-source-factory.source.unsubscription-count

                                shared-source-factory.source.emit-stale(true)
                                do new
                                    -- shared-flow is .shared-active

                                    shared-source-factory.source.emit-current(true)
                                    do new
                                        -- shared-flow is .shared-finished
                                        -- shared-source-factory.source.unsubscription-count
                                        -- shared-source-factory.source.subscriber-count

━━━━━━━━━━━━━━━━━━━━
+ UniqueSourceFactory

+ sources [TrackingSource]

────────────────────
+ do make TrackingSource

. source = TrackingSource()
sources += source
= source

━━━━━━━━━━━━━━━━━━━━
+ SharedSourceFactory

+ source = TrackingSource()
+ calls = 0

────────────────────
+ do get TrackingSource

calls += 1
= source

━━━━━━━━━━━━━━━━━━━━
+ TrackingSource
    is Broadcastable'Bool

+ subscription-count = 0
+ unsubscription-count = 0

_ subscribers [Subscriber'Bool]
_ detached-subscribers [Subscriber'Bool]

────────────────────
+ get subscriber-count Size

subscribers.length

────────────────────
+ get is-closed Bool

false

────────────────────
+ do subscribe ?! Error
    - subscriber Subscriber'Bool

subscription-count += 1
subscribers += subscriber

────────────────────
+ do unsubscribe ?! Error
    - subscriber Subscriber'Bool

unsubscription-count += 1
detached-subscribers += subscriber
subscribers -= subscriber

────────────────────
+ do subscribe ?! Error
    - subscriber Subscribable'Bool

────────────────────
+ do unsubscribe ?! Error
    - subscriber Subscribable'Bool

────────────────────
+ do emit-current
    - item Bool

if subscribers
    . subscriber = subscribers[0]
    subscriber(item, this)

────────────────────
+ do emit-stale
    - item Bool

if detached-subscribers
    . subscriber = detached-subscribers[0]
    subscriber(item, this)
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

```yumi
+ submitted AsyncChannel'Bool
+ results AsyncChannel'AuthResult

+ auth ~>
    ~> idle
    ~> authenticating on submitted
        do
            -- "enter authenticating"

        do finally
            -- "exit authenticating"

        then authenticated for .authentic on results

    ~> authenticated

submitted <- true
do new
    results <- .authentic
    do new
        -- "done"

━━━━━━━━━━━━━━━━━━━━
+ AuthResult is Union

| authentic
```

```output
enter authenticating
exit authenticating
done
```

### Actions that can raise compile when transitions lead back into the states that run them.

```yumi
+ toggles AsyncChannel'Bool

+ mode ~>
    ~> first
        do finally
            check(false)

        then second on toggles

    ~> second
        do
            -- "entered second"

        then first on toggles

toggles <- true
do new
    -- mode is .second

━━━━━━━━━━━━━━━━━━━━
+ do check
    - fails Bool

if fails
    ?! Error("check failed")
```

```output
entered second
true
```

### Re-entering an already active case exits the active case and subtree before entering it again.

```yumi
+ submitted AsyncChannel'Int
+ child-events AsyncChannel'Int

+ flow ~>
    ~> idle
    ~> editing(count) for count on submitted
        do
            -- "enter editing $count"

        do finally
            -- "exit editing $count"

        ~> child on child-events
            do
                -- "enter child"

            do finally
                -- "exit child"

submitted <- 1
do new
    child-events <- 1
    do new
        submitted <- 2
        do new
            -- match flow
                | .editing(count)
                    "editing $count"
                ??
                    "none"
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

```yumi
+ submitted AsyncChannel'Int
_ counter = 0

+ flow ~>
    ~> idle
    ~> running(seed) for seed on submitted
        . started-at = next-counter()
        .. memoized = next-counter()

        do
            -- "enter seed=$seed started=$started-at memo=$memoized"
            -- "memo again $memoized"

submitted <- 10
do new
    submitted <- 20
    do new
        -- flow is .running

━━━━━━━━━━━━━━━━━━━━
+ do next-counter

counter += 1
= counter
```

```output
enter seed=10 started=1 memo=2
memo again 2
enter seed=20 started=3 memo=4
memo again 4
true
```

### A transition to an outer state exits the active child subtree.

```yumi
+ submit AsyncChannel'Bool
+ cancel AsyncChannel'Bool
+ begin-child AsyncChannel'Bool

+ flow ~>
    ~> idle
    ~> submitted on submit
        do
            -- "enter submitted"

        do finally
            -- "exit submitted"

        ~> validating on begin-child
            do
                -- "enter validating"

            do finally
                -- "exit validating"

        then idle on cancel

submit <- true
do new
    begin-child <- true
    do new
        cancel <- true
        do new
            -- flow is .idle
```

```output
enter submitted
enter validating
exit validating
exit submitted
true
```

### Nested child cases are active together with their parent case.

```yumi
+ submit AsyncChannel'Bool
+ begin-child AsyncChannel'Bool

+ flow ~>
    ~> idle
    ~> submitted on submit
        ~> validating on begin-child

submit <- true
do new
    begin-child <- true
    do new
        -- flow is .submitted
        -- flow is .validating
        -- flow is .idle
```

```output
true
true
false
```

### Payload-bearing then continuations compile and retain their source payload in generic classes.

```yumi
. source = GenericPayloadContinuation'String()
source.item = "ready"
source.submitted <- (PayloadContinuationRequest(7, .keys) • does complete)
do new
    -- source.flow is .awaiting

    source.completed <- true
    do new
        -- match source.flow
            | .finished(index, cause, does-complete)
                "$index/$cause/$does-complete"
            ??
                "none"

━━━━━━━━━━━━━━━━━━━━
+ GenericPayloadContinuation'Value is PayloadContinuationBase

+ item Value?
_ event submitted PayloadContinuationRequest
_ event completed Bool

────────────────────
+ flow ~>

~> idle
~> awaiting(index, cause, does-complete) for (index, cause, does-complete) on submitted
    then finished(index, cause, does-complete) if false
    then finished(index, cause, does-complete) on completed when item != null

~> finished(index, cause, does-complete)

━━━━━━━━━━━━━━━━━━━━
_ PayloadContinuationRequest is Tuple

+ index Index
+ cause PayloadContinuationCause
+ does-complete Bool

━━━━━━━━━━━━━━━━━━━━
+ PayloadContinuationBase

━━━━━━━━━━━━━━━━━━━━
+ PayloadContinuationCause is Union

| keys
```

```output
true
7/keys/true
```

### Nested named machines are independent state axes scoped to the parent state lifetime.

```yumi
+ auth-events AsyncChannel'Bool
+ network-events AsyncChannel'Bool

+ app ~>
    ~> ready
        + auth ~>
            ~> editing
            ~> authenticated on auth-events
                do
                    -- "auth authenticated"

        + network ~>
            ~> offline
                do
                    -- "network offline"

            ~> online on network-events
                do
                    -- "network online"

    ~> closed

auth-events <- true

do new
    -- app is .ready

    network-events <- true
    do new
        -- app is .ready
```

```output
network offline
auth authenticated
true
network online
true
```

### Leaving a parent state exits nested named machines and removes their subscriptions.

```yumi
+ close-events AsyncChannel'Bool
+ auth-events AsyncChannel'Bool

+ app ~>
    ~> ready
        + auth ~>
            ~> editing
                do finally
                    -- "exit auth editing"

            ~> authenticated on auth-events
                do
                    -- "enter authenticated"

                do finally
                    -- "exit authenticated"

        then closed on close-events

    ~> closed

auth-events <- true
do new
    close-events <- true
    do new
        auth-events <- true
        do new
            -- app is .closed
```

```output
exit auth editing
enter authenticated
exit authenticated
true
```

### Transitioning back to the same state is reentrant, not an internal transition.

```yumi
+ refresh AsyncChannel'Bool

+ view ~>
    ~> showing
        do
            -- "enter showing"

        do finally
            -- "exit showing"

        then showing on refresh

refresh <- true

do new
    -- view is .showing
```

```output
enter showing
exit showing
enter showing
true
```

### A transition to an outer already-active state is reentrant and resets that outer state subtree.

```yumi
+ begin-child AsyncChannel'Bool
+ reset AsyncChannel'Bool

+ flow ~>
    ~> editing
        do
            -- "enter editing"

        do finally
            -- "exit editing"

        ~> focused on begin-child
            do
                -- "enter focused"

            do finally
                -- "exit focused"

        then editing on reset

begin-child <- true
do new
    reset <- true
    do new
        -- flow is .editing
        -- flow is .focused
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

```yumi
+ submitted AsyncChannel'String

+ auth ~>
    ~> idle
    ~> authenticating(email) for email on submitted

-- auth is .idle

submitted <- "joe@example.com"
do new
    -- auth is .authenticating
    -- match auth
        | .authenticating(email)
            email
        ??
            "none"
```

```output
true
true
joe@example.com
```

### Class eager property state machines support per-instance storage and markup content.

```yumi
/core/tests/support/markup-fixtures

. first = Widget("first")
. second = Widget("second")
. header = HeaderMarkupMachine()
. section = SectionMarkupMachine()
. observed = ObservedWidget()

-- header.phase is .idle
-- section.phase is .idle

observed.advance <- true
-- observed.phase is .idle
do new
    -- observed.phase is .active

first.advance <- true
do new
    -- first.phase is .active
    -- second.phase is .idle

    second.advance <- true
    do new
        -- first.phase is .active
        -- second.phase is .active

        header.advance <- "header"
        do new
            -- match header.phase
                | .active(value)
                    value
                ??
                    "none"

            section.advance <- true
            do new
                -- section.phase is .active

━━━━━━━━━━━━━━━━━━━━
+ Widget

+ name String
+ advance AsyncChannel'Bool

────────────────────
+ phase ~>

~> idle
~> active on advance

────────────────────
+ This
    - name

━━━━━━━━━━━━━━━━━━━━
+ ObservedWidget

+ advance AsyncChannel'Bool

────────────────────
+ phase ~>

~> idle
~> active on advance
    do
        -- "observed active"

━━━━━━━━━━━━━━━━━━━━
+ HeaderMarkupMachine is Box

+ advance AsyncChannel'String

────────────────────
+ phase ~>

~> idle
~> active(value) for value on advance

────────────────────

<This

━━━━━━━━━━━━━━━━━━━━
+ SectionMarkupMachine is Box

+ advance AsyncChannel'Bool

────────────────────
+ phase ~>

~> idle
~> active on advance

────────────────────

<This
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

```yumi
-- run-local("one")
-- run-local("two")

━━━━━━━━━━━━━━━━━━━━
+ do run-local String
    - label String

. flow ~>
    ~> idle
    ~> ready if true
        do
            -- "$label ready"

-- flow is .ready
= "done $label"
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

```yumi
lazy-local("first")
lazy-local("second")

_ hits = 0

━━━━━━━━━━━━━━━━━━━━
+ do lazy-local
    - label String

.. flow ~>
    ~> idle
    ~> ready if mark(label)

-- "before $label $hits"
-- flow is .ready
-- "middle $label $hits"
-- flow is .ready
-- "after $label $hits"

━━━━━━━━━━━━━━━━━━━━
+ do mark Bool
    - label String

hits += 1
-- "mark $label $hits"
= true
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

```yumi
+ opened AsyncChannel'Bool

~> closed
~> open on opened

-- closed

opened <- true
do new
    -- open
    -- closed
```

```output
true
true
false
```

### Anonymous class-content machines have per-instance projections and storage.

```yumi
. first = Panel("first")
. second = Panel("second")

first.activate <- true
do new
    -- first.active
    -- second.active

    second.activate <- true
    do new
        -- first.active
        -- second.active

━━━━━━━━━━━━━━━━━━━━
+ Panel

+ name String
+ activate AsyncChannel'Bool

────────────────────
+ This
    - name

────────────────────
~> idle
~> active on activate
    do
        -- "$name active"
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

```yumi
. watcher = Watcher()
. holder = watcher.holder
-- holder.renaming
-- watcher.shown
holder.requested <- "a"
-- holder.renaming
-- holder.renaming ?? "none"
-- watcher.shown
holder.done <- true
-- holder.renaming ?? "none"
-- watcher.shown

━━━━━━━━━━━━━━━━━━━━
+ Watcher

+ holder = Holder()
+ shown = holder.#renaming ?? "none"

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ event requested String
+ event done Bool

────────────────────
~> is-idle

~> renaming(name) for name on requested
    then is-idle on done
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

```yumi
./lib/owner
./lib/reader

. owner = SubOwner()
-- shown(owner)
owner.requested <- Item("a")
-- shown(owner)
-- owner.renaming != null
```

```yumi file=lib/owner.yumi

━━━━━━━━━━━━━━━━━━━━
+ Item

+ key String

────────────────────
+ This
    - key

━━━━━━━━━━━━━━━━━━━━
+ Owner

+ event requested Item

────────────────────
~> is-idle

~> renaming(item) for item on requested

```

```yumi file=lib/reader.yumi

./owner

━━━━━━━━━━━━━━━━━━━━
+ SubOwner is Owner

━━━━━━━━━━━━━━━━━━━━
+ do shown
    - owner Owner

= owner.renaming?.key ?? "none"

```

```output
none
a
true
```

### Anonymous class-content machines without an untriggered case start inactive.

```yumi
. first = TriggerOnlyPanel("first")
. second = TriggerOnlyPanel("second")

-- first.active

first.activate <- true
do new
    -- first.active
    -- second.active

━━━━━━━━━━━━━━━━━━━━
+ TriggerOnlyPanel

+ name String
+ activate AsyncChannel'Bool

────────────────────
+ This
    - name

────────────────────
~> active on activate
    do
        -- "$name active"
```

```output
false
first active
true
false
```

### Anonymous machines can transition from inactive state through inline one-shot streams.

```yumi
+ submitted AsyncChannel'LoginSubmission

~> authenticating for .valid(email, password) on submitted
    then authenticated for .authentic on authenticate(email, password)

-- authenticating
-- authenticated

submitted <- .valid("email", "secret")
do new
    -- authenticating
    -- authenticated

    for i in 0 .. < 3
        yield-once()
    -- authenticating
    -- authenticated

━━━━━━━━━━━━━━━━━━━━
+ do yield-once

. done = Semaphore()
do new
    done.post()
done.wait()

━━━━━━━━━━━━━━━━━━━━
+ do authenticate
    - email String
    - password String

yield-once()

<=
    |? password == "secret"
        ..authentic(email, "token")
    ??
        ..incorrect-password

━━━━━━━━━━━━━━━━━━━━
+ LoginSubmission is Union

| valid(String, String)
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

```yumi
+ open AsyncChannel'Bool
+ close AsyncChannel'Bool
+ timer-open AsyncChannel'Bool
+ timer-close AsyncChannel'Bool
_ source-calls = 0
. class-timer = InlineTimerMachine()

-- class-timer.phase is .waiting
class-timer.close <- true
do new
    -- class-timer.phase is .idle

~> idle
~> active on open
    then done for true on one-shot()
    then idle on close

~> done

+ timer-flow ~>
    ~> idle
    ~> waiting on timer-open
        then idle on AsyncChannel'Bool()
        then idle on timer-close

-- source-calls

open <- true
do new
    -- source-calls

    close <- true
    do new
        -- active
        -- source-calls

        timer-open <- true
        do new
            -- timer-flow is .waiting

            timer-close <- true
            do new
                -- timer-flow is .idle

━━━━━━━━━━━━━━━━━━━━
+ do one-shot

source-calls += 1
<= true

━━━━━━━━━━━━━━━━━━━━
+ InlineTimerMachine

+ close AsyncChannel'Bool

────────────────────
+ phase ~>

~> waiting
    then idle on AsyncChannel'Bool()
    then idle on close

~> idle
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

```yumi
. first = Session("first")
. second = Session("second")

first.open <- true
do new
    first.open <- true
    do new
        first.login <- true
        do new
            second.open <- true
            do new
                -- first.app is .ready
                -- second.app is .ready

                first.close <- true
                do new
                    first.login <- true
                    do new
                        -- first.app is .closed
                        -- second.app is .ready

━━━━━━━━━━━━━━━━━━━━
+ Session

+ name String
+ open AsyncChannel'Bool
+ close AsyncChannel'Bool
+ login AsyncChannel'Bool

────────────────────
+ app ~>

~> closed
~> ready on open
    + auth ~>
        ~> editing
            do finally
                -- "$name auth editing exit"

        ~> authenticated on login
            do
                -- "$name authenticated"

            do finally
                -- "$name auth authenticated exit"

    then closed on close

────────────────────
+ This
    - name
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

```yumi crash
make-local()
-- idle

━━━━━━━━━━━━━━━━━━━━
+ do make-local

. flow ~>
    ~> idle
    ~> ready

-- flow is .idle
```

```output
Exception: Name "idle" not found
<no file:3>
    -- idle
       ^^^^
```

### Event transitions subscribe asynchronously and do not consume events emitted before entry.

```yumi
check-async-subscribe()

━━━━━━━━━━━━━━━━━━━━
+ do check-async-subscribe

. events AsyncChannel'Bool

events <- true

. flow ~>
    ~> idle
    ~> fired on events

do new
    -- flow is .idle

    events <- true
    do new
        do new
            -- flow is .fired
```

```output
true
false
```

### Event state transitions finish before the publishing call returns

```yumi
. source = EventMachine()
-- source.auth is .idle
source.submit("joe@example.com")
-- source.auth is .authenticating
-- match source.auth
    | .authenticating(email)
        email
    ??
        "none"

. siblings = EventSiblingMachine()
siblings.submit(1)
-- siblings.flow is .first
-- siblings.flow is .second

. exited = EventExitMachine()
exited.start <- true
-- exited.flow is .active

exited.leave <- true
-- exited.flow is .done

exited.stale <- true
-- exited.flow is .done
-- exited.flow is .stale

━━━━━━━━━━━━━━━━━━━━
+ EventMachine

+ event submitted String

────────────────────
+ auth ~>

~> idle
~> authenticating(email) for email on submitted

────────────────────
+ do submit
    - email String

submitted <- email

━━━━━━━━━━━━━━━━━━━━
+ EventSiblingMachine

+ event submitted Int

────────────────────
+ flow ~>

~> idle
~> first on submitted when each > 0
~> second on submitted when each > 0

────────────────────
+ do submit
    - value Int

submitted <- value

━━━━━━━━━━━━━━━━━━━━
+ EventExitMachine

+ event start Bool
+ event leave Bool
+ event stale Bool

────────────────────
+ flow ~>

~> idle
~> active on start
    then done on leave
    then stale on stale
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

```yumi
. gate = Semaphore()
. parked = ParkingHandler(gate)
. other = EventToggle()
. parker = do new
    parked.pings <- true
do new
    other.go <- true
    -- other.flow is .on
    parker.cancel()
    other.back <- true
    -- other.flow is .idle

━━━━━━━━━━━━━━━━━━━━
+ ParkingHandler

+ gate Semaphore'Void
+ event pings Bool

────────────────────
+ This
    - gate

────────────────────
for event on pings

gate.wait()

━━━━━━━━━━━━━━━━━━━━
+ EventToggle

+ event go Bool
+ event back Bool

────────────────────
+ flow ~>

~> idle
~> on on go
    then idle on back
```

```output
true
true
```

### A transition canceled in its entry rows does not hold back later transitions

```yumi
. gate = Semaphore()
. parked = ParkingEntry(gate)
. other = EventToggle()
. parker = do new
    parked.go <- true
do new
    parker.cancel()
    other.go <- true
    -- other.flow is .on

━━━━━━━━━━━━━━━━━━━━
+ ParkingEntry

+ gate Semaphore'Void
+ event go Bool

────────────────────
+ This
    - gate

────────────────────
+ flow ~>

~> idle
~> on on go
    do
        gate.wait()

━━━━━━━━━━━━━━━━━━━━
+ EventToggle

+ event go Bool

────────────────────
+ flow ~>

~> idle
~> on on go
```

```output
true
```

### Keyed event transitions queued during a transition are rejected after the receiver generation is released

```yumi
. mailbox = KeyedMachineMailbox()
. previous = KeyedMachineReceiver(mailbox, 7)
. courier = KeyedMachineCourier(mailbox, previous)
courier.deliveries <- true
-- previous.flow is .active

. replacement = KeyedMachineReceiver(mailbox, 7)
-- replacement.flow is .idle
mailbox.messages[7] <- true
-- replacement.flow is .active

. moving = KeyedMachineReceiver(mailbox, 8)
moving.change-key(9)
mailbox.messages[8] <- true
-- moving.flow is .idle
mailbox.messages[9] <- true
-- moving.flow is .active

━━━━━━━━━━━━━━━━━━━━
+ KeyedMachineMailbox

+ event messages Bool keyed by U64

━━━━━━━━━━━━━━━━━━━━
+ KeyedMachineReceiver

+ mailbox KeyedMachineMailbox
+ id U64

────────────────────
+ This
    - mailbox
    - id

────────────────────
+ flow ~>

~> idle
~> active(value) for value on mailbox.messages[id]

────────────────────
+ do close

────────────────────
+ do change-key
    - next U64

mailbox.messages[id].rekey(next)
id = next

━━━━━━━━━━━━━━━━━━━━
+ KeyedMachineCourier

+ mailbox KeyedMachineMailbox
+ receiver KeyedMachineReceiver
+ event deliveries Bool

────────────────────
+ This
    - mailbox
    - receiver

────────────────────
+ flow ~>

~> idle
~> delivered on deliveries
    do
        mailbox.messages[receiver.id] <- true
        receiver.close()
```

```output
false
true
true
true
true
```

### A queued keyed transition is rejected when its receiver rekeys before activation

```yumi
. mailbox = KeyedMachineMailbox()
. moving = KeyedMachineReceiver(mailbox, 8)
. courier = KeyedMachineCourier(mailbox, moving)
courier.deliveries <- true
-- moving.flow is .idle

mailbox.messages[9] <- true
-- moving.flow is .active

━━━━━━━━━━━━━━━━━━━━
+ KeyedMachineMailbox

+ event messages Bool keyed by U64

━━━━━━━━━━━━━━━━━━━━
+ KeyedMachineReceiver

+ mailbox KeyedMachineMailbox
+ id U64

────────────────────
+ This
    - mailbox
    - id

────────────────────
+ flow ~>

~> idle
~> active(value) for value on mailbox.messages[id]

────────────────────
+ do change-key
    - next U64

mailbox.messages[id].rekey(next)
id = next

━━━━━━━━━━━━━━━━━━━━
+ KeyedMachineCourier

+ mailbox KeyedMachineMailbox
+ receiver KeyedMachineReceiver
+ event deliveries Bool

────────────────────
+ This
    - mailbox
    - receiver

────────────────────
+ flow ~>

~> idle
~> delivered on deliveries
    do
        mailbox.messages[receiver.id] <- true
        receiver.change-key(9)
```

```output
true
true
```

### A keyed machine keeps receiving after an ordinary subscription on its endpoint breaks

```yumi
. mailbox = PersistentKeyedMachineMailbox()
. receiver = PersistentKeyedMachineReceiver(mailbox, 7)
mailbox.messages[7] <- true
-- receiver.flow is .active

mailbox.messages[7] <- false
-- match receiver.flow
    | .active(value)
        value
    ??
        true

━━━━━━━━━━━━━━━━━━━━
+ PersistentKeyedMachineMailbox

+ event messages Bool keyed by U64

━━━━━━━━━━━━━━━━━━━━
+ PersistentKeyedMachineReceiver

+ mailbox PersistentKeyedMachineMailbox
+ id U64

────────────────────
+ This
    - mailbox
    - id

────────────────────
for value on mailbox.messages[id]

-- "ordinary $value"
break

────────────────────
+ flow ~>

~> idle
~> active(value) for value on mailbox.messages[id]
```

```output
ordinary true
true
false
```

### Event transitions from one publication choose the first sibling in source order

```yumi
. source = EventSiblingMachine()
source.submit(1)
-- source.flow is .first
-- source.flow is .second

━━━━━━━━━━━━━━━━━━━━
+ EventSiblingMachine

+ event submitted Int

────────────────────
+ flow ~>

~> idle
~> first on submitted when each > 0
~> second on submitted when each > 0

────────────────────
+ do submit
    - value Int

submitted <- value
```

```output
true
false
```

### Event subscriptions from an exited state reject later publications

```yumi
. source = EventExitMachine()
source.start <- true
do new
    -- source.flow is .active

    source.leave <- true
    do new
        -- source.flow is .done

        source.stale <- true
        do new
            -- source.flow is .done
            -- source.flow is .stale

━━━━━━━━━━━━━━━━━━━━
+ EventExitMachine

+ event start Bool
+ event leave Bool
+ event stale Bool

────────────────────
+ flow ~>

~> idle
~> active on start
    then done on leave
    then stale on stale
```

```output
true
true
true
false
```

### Case tests read the state flags of the machine they are read from, inside or outside its class.

```yumi
. m = Machine()
. other = Machine()
m.submit <- true
do new
    m.begin-child <- true
    do new
        -- "outside ${m.flow is .submitted} ${m.flow is .validating} ${m.flow is .idle}"
        -- "other ${m.compare(other)}"
        . a = Auth()
        a.results <- .rate-limited
        do new
            -- "auth ${a.auth is .authenticating} ${a.auth is .authenticated} ${a.auth is .rejected}"

━━━━━━━━━━━━━━━━━━━━
+ Machine

+ event submit Bool
+ event begin-child Bool

────────────────────
+ flow ~>

~> idle
~> submitted on submit
    ~> validating on begin-child

────────────────────
+ do compare String
    - other Machine

= "${flow is .submitted} ${other.flow is .submitted} ${other.flow is .idle}"

━━━━━━━━━━━━━━━━━━━━
+ Auth

+ event results AuthResult

────────────────────
+ auth ~>

~> authenticating
    then authenticated for .authentic on results
    then rejected for .incorrect-password on results
    ?? authenticating

~> authenticated
~> rejected

━━━━━━━━━━━━━━━━━━━━
+ AuthResult is Union

| authentic
| incorrect-password
| rate-limited
```

```output
outside true true false
other true false true
auth false false false
```

### A class machine payload can depend on the owner type parameter.

```yumi
. h = Holder'Int()
h.items <- 5
do new
    -- match h.flow
        | .got(v)
            "got $v"
        ??
            "none"

━━━━━━━━━━━━━━━━━━━━
+ Holder'Value

+ event items Value

────────────────────
+ flow ~>

~> idle
~> got(v) for v on items
```

```output
got 5
```

```yumi
. host = Host()
-- host.summary()

host.open-requests <- true
do new
    -- host.summary()

    host.view-toggles <- true
    do new
        -- host.summary()

        host.emitter.fire()
        do new
            -- host.summary()

            host.close-requests <- true
            do new
                -- host.summary()

━━━━━━━━━━━━━━━━━━━━
+ Label

+ text = ""

━━━━━━━━━━━━━━━━━━━━
+ Emitter is Label

+ event activated Bool

────────────────────
+ do fire

activated <- true

━━━━━━━━━━━━━━━━━━━━
+ Host

+ open-requests AsyncChannel'Bool
+ close-requests AsyncChannel'Bool
+ view-toggles AsyncChannel'Bool
+ is-grid = false
+ was-received = false

────────────────────
+ do this += child Label

────────────────────
+ summary String

"${root-label.text} ${state-label.text} ${view-label.text} ${event-label.text}"

────────────────────

<This
    <Label root-label
        text = if #running then "running" else "stopped"
    <Label state-label
        text = if #is-open then "open" else "closed"
    <Label view-label
        text = if #is-grid then "grid" else "list"
    <Label event-label
        text = if #was-received then "received" else "idle"
    <Emitter emitter

~> running
    ~> is-closed
        then is-open on open-requests

    ~> is-open
        then is-closed on close-requests

    + view-mode ~>
        ~> view-list
            do
                is-grid = view-mode is .view-grid

            then view-grid on view-toggles

        ~> view-grid
            do
                is-grid = view-mode is .view-grid

            then view-list on view-toggles

    + event-mode ~>
        ~> event-idle
            then event-received on emitter.activated

        ~> event-received
            do
                was-received = event-mode is .event-received
```

```output
running closed list idle
running open list idle
running open grid idle
running open grid received
running closed grid received
```

```yumi
/core/style
/core/style-debug
/core/markup
/core/task
/core/tests/support/markup-fixtures

. done = Semaphore()

<BatchDemo demo
    <Panel panel

demo.markup-batch-queue.checkpoint()
-- style-computed-snapshot(panel)
-- panel.rule-count

panel.open-requests <- true
do new
    demo.markup-batch-queue.checkpoint()
    -- style-computed-snapshot(panel)
    -- panel.rule-count
    done.post()
done.wait()

panel.close-requests <- true
do new
    demo.markup-batch-queue.checkpoint()
    -- style-computed-snapshot(panel)
    -- panel.rule-count
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
+ Panel is Box
    using TextStyle

+ open-requests AsyncChannel'Bool
+ close-requests AsyncChannel'Bool
+ rule-count = 0
+ is-open = #phase is .open

• This
    ink = 9
    when #is-open
        ink = 11

────────────────────
+ phase ~>

~> closed
~> open on open-requests
    then closed on close-requests

────────────────────
when #is-open

rule-count += 1
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=9
0
node TextStyle seg=owned gen=2 mask=1 ink=11
1
node TextStyle seg=owned gen=3 mask=1 ink=9
1
```

```yumi crash
. source = FailingFlow()
source.enter <- true

━━━━━━━━━━━━━━━━━━━━
+ FailingFlow

+ event enter Bool

────────────────────
+ flow ~>

~> idle
~> active on enter
    do
        -- "enter active"
        fail()
        -- "not reached"

━━━━━━━━━━━━━━━━━━━━
+ do fail

?! Error("action failed")
```

```output
enter active
```

### Event transitions advance inside a loop that never yields to the task queue.

```yumi
. walker = Walker()
for i in 0 .. < 4
    walker.step()
    -- "$i ${walker.phase is .walking}"

━━━━━━━━━━━━━━━━━━━━
+ Walker

+ event ticks Bool
+ count Size = 0

────────────────────
+ phase ~>

~> resting
    then walking on ticks when count >= 2

~> walking
    do
        -- "enter walking at $count"

    then resting on ticks when count >= 4

────────────────────
+ do step

count += 1
ticks <- true
```

```output
0 false
enter walking at 2
1 true
2 true
3 false
```

### Publications made by a transition queue their transitions until it finishes, and the outermost publication returns after all of them run.

```yumi
. first = Relay("first")
. second = Relay("second")
first.next = second
first.poke()
-- "returned"
-- second.phase is .lit

━━━━━━━━━━━━━━━━━━━━
+ Relay

+ name String
+ next Relay?
+ event pokes Bool

────────────────────
+ This
    - name

────────────────────
+ phase ~>

~> idle
~> lit on pokes
    do
        -- "$name enter"
        if next
            next.poke()
        -- "$name entered"

────────────────────
+ do poke

pokes <- true
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

```yumi
. door = Door()
door.open-requests <- true
-- door.is-open
door.close-requests <- true
-- door.reopened
-- door.is-open

━━━━━━━━━━━━━━━━━━━━
+ Door

+ event open-requests Bool
+ event close-requests Bool
+ event closed Bool
+ reopened = false

────────────────────
~> is-closed
    then is-open on open-requests

~> is-open
    then is-closed for reason on close-requests
        do
            closed <- reason

────────────────────
for reason on closed

if !reopened
    reopened = true
    open-requests <- true
```

```output
true
true
true
```

### A nested named machine entered by an event transition runs its own continuations.

```yumi
+ ticks AsyncChannel'Bool
+ wakes AsyncChannel'Bool

+ life ~>
    ~> asleep
    ~> awake on wakes
        + voice ~>
            ~> quiet
                then calling on ticks

            ~> calling
                do
                    -- "calling"

wakes <- true
do new
    ticks <- true
    do new
        -- "end"
```

```output
calling
end
```

### A nested named machine subscribes its root triggers while its parent state enters.

```yumi
+ ticks AsyncChannel'Bool
+ wakes AsyncChannel'Bool

+ life ~>
    ~> asleep
    ~> awake on wakes
        + voice ~>
            ~> quiet
            ~> calling on ticks
                do
                    -- "calling"

wakes <- true
do new
    ticks <- true
    do new
        -- "end"
```

```output
calling
end
```

### A continuation into another branch exits the source ancestors and enters the target ancestors.

```yumi
. m = Machine()
m.go <- true
-- m.phase is .swimming
-- m.phase is .arriving
-- m.phase is .ashore
-- m.phase is .crawling

━━━━━━━━━━━━━━━━━━━━
+ Machine

+ event go Bool

────────────────────
+ phase ~>

~> swimming
    do finally
        -- "exit swimming"

    ~> arriving
        then crawling on go

~> ashore
    do
        -- "enter ashore"

    ~> resting
        do
            -- "enter resting"

    ~> crawling
        do
            -- "enter crawling"
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

```yumi
+ ready = true

+ phase ~>
    ~> parent
        do
            -- "enter parent"

        ~> waiting
        ~> going if ready
            do
                -- "enter going"

-- phase is .parent
-- phase is .going
-- phase is .waiting
```

```output
enter parent
enter going
true
true
false
```

### A parent case test compiled before its machine's owner is constructed reads the parent's flag.

```yumi
. holder = Holder()
-- holder.is-outer
holder.mother = Mother()
-- holder.is-outer

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ mother Mother?

────────────────────
+ do get is-outer Bool

. current = mother
=? false if !current
= current.phase is .outer

━━━━━━━━━━━━━━━━━━━━
+ Mother

────────────────────
+ phase ~>

~> outer
    ~> inner
```

```output
false
true
```

### A state handler runs only while its state is active.

```yumi
. mover = Mover()
for i in 0 .. < 6
    mover.step(0.5)
    -- "$i ${mover.phase is .moving} ${mover.x}"

━━━━━━━━━━━━━━━━━━━━
+ Mover

+ event ticks F64
+ x F64 = 0
+ count Size = 0

────────────────────
+ phase ~>

~> moving
    for dt on ticks
        x += dt

    then resting on ticks when count >= 3

~> resting

────────────────────
+ do step
    - dt F64

count += 1
ticks <- dt
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

```yumi
. gate = Gate()
gate.ticks <- 1
gate.ticks <- 1
-- gate.seen

━━━━━━━━━━━━━━━━━━━━
+ Gate

+ event ticks F64
+ seen F64 = 0

────────────────────
+ phase ~>

~> open
    then closed on ticks when seen >= 1

    for dt on ticks when dt > 0
        seen += dt

~> closed
```

```output
1
```

### A class machine's continuation payload can call the owner's methods and read its properties.

```yumi
. rover = Rover()
rover.pokes <- true
-- match rover.phase
    | .heading(spot)
        "heading $spot"
    ??
        "idle"

━━━━━━━━━━━━━━━━━━━━
+ Rover

+ event pokes Bool
+ base Int = 40

────────────────────
+ phase ~>

~> idle
    then heading(pick-spot() + base) on pokes

~> heading(spot)

────────────────────
_ do pick-spot Int

= 2
```

```output
heading 42
```

### A state case whose one payload value is a named tuple keeps the tuple whole.

```yumi
. holder = Holder()
holder.pairs <- Pair(3, 4)
-- match holder.phase
    | .holding(pair)
        "holding ${pair.a + pair.b}"
    ??
        "empty"

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ a Int
+ b Int

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ event pairs Pair

────────────────────
+ phase ~>

~> empty
    then holding(pair) for pair on pairs

~> holding(pair)
```

```output
holding 7
```

### A transition into a declared state does not carry the source state's payload into it.

```yumi
. hunter = Hunter()
hunter.ticks <- true
hunter.ticks <- true

━━━━━━━━━━━━━━━━━━━━
+ Hunter

+ event ticks Bool
+ quarry Size? = 3

────────────────────
+ phase ~>

~> searching
    then hunting(quarry!) if quarry

~> hunting(person)
    do
        -- "hunting $person"
        quarry = quarry! + 1

    then searching on ticks
```

```output
hunting 3
hunting 4
hunting 5
```

### A continuation's payload can read a property the owner class inherits.

```yumi
/graphics/point

. swimmer = Swimmer()
swimmer.ticks <- true
-- swimmer.phase is .waiting

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ position = Point(3, 4)

━━━━━━━━━━━━━━━━━━━━
+ Swimmer is Thing

+ event ticks Bool

────────────────────
+ phase ~>

~> swimming
    then waiting(position) on ticks

~> waiting(spot)
    do
        -- "waiting at ${spot.x}"
```

```output
waiting at 3
true
```

### A continuation's payload takes its type from the arriving item and the owner state's payload.

```yumi
. counter = Counter()
counter.go <- 1000
counter.next <- true

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ event go Size
+ event next Bool

────────────────────
+ phase ~>

~> idle
    then counting(each) on go

~> counting(total)
    do
        -- total

    then doubled(total * 2) on next

~> doubled(value)
    do
        -- value
```

```output
1000
2000
```

### A continuation into a declared state runs its own nested continuation.

```yumi
. hunter = Hunter()
hunter.start <- 5
hunter.rest <- true
hunter.go <- true

━━━━━━━━━━━━━━━━━━━━
+ Hunter

+ event start Int
+ event rest Bool
+ event go Bool

────────────────────
+ phase ~>

~> idle
    then chasing(each) on start

~> chasing(goal)
    do
        -- "chasing $goal"

    then resting(goal + 1) on rest
        then chasing(goal) on go

~> resting(goal)
    do
        -- "resting $goal"
```

```output
chasing 5
resting 6
chasing 5
```

### A state's trigger source is evaluated each time the state is entered.

```yumi
. lamp = Lamp()
for i in 0 .. < 3
    lamp.flips <- true
    -- "${lamp.phase}"

━━━━━━━━━━━━━━━━━━━━
+ Lamp

+ event flips Bool

────────────────────
+ phase ~>

~> dark
    then lit on watched("dark")

~> lit
    then dark on watched("lit")

────────────────────
_ do watched Broadcastable'Bool
    - name String

-- "watching from $name"
= flips
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

```yumi
. clock = Clock()
-- is-open(clock.ticks)
. holder = Holder(clock.ticks)
for dt on holder.source
    -- "heard $dt"
clock.ticks <- 0.5

━━━━━━━━━━━━━━━━━━━━
+ do is-open Bool
    - source

. stream Broadcastable'F64 = source
= !stream.is-closed

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ source Broadcastable'F64

────────────────────
+ This
    - source

━━━━━━━━━━━━━━━━━━━━
+ Clock

+ event ticks F64
```

```output
true
heard 0.5
```

### Machine work from a source that relays an event waits for the event's publication, like work from the event itself.

```yumi
. kettle = Kettle()
kettle.ticks <- 1
kettle.ticks <- 1
-- kettle.heat
-- kettle.phase is .whistling

━━━━━━━━━━━━━━━━━━━━
+ Kettle

+ event ticks F64
+ heat F64 = 0

────────────────────
+ phase ~>

~> heating
    for dt on ticks
        heat += dt

    then whistling on Relay(ticks) when heat >= 2

~> whistling

━━━━━━━━━━━━━━━━━━━━
+ Relay
    is Broadcastable'F64

+ source Broadcastable'F64
_ forward Subscriber'F64?

────────────────────
+ This
    - source

────────────────────
+ get is-closed Bool

source.is-closed

────────────────────
+ do subscribe
    - subscriber Subscriber'F64

. relay Subscriber'F64 = :: dt, from => do
    subscriber(dt, this)
forward = relay
source.subscribe(relay)

────────────────────
+ do unsubscribe
    - subscriber Subscriber'F64

=? if !forward
source.unsubscribe(forward)
forward = null

────────────────────
+ do subscribe
    - subscriber Subscribable'F64

────────────────────
+ do unsubscribe
    - subscriber Subscribable'F64
```

```output
2
true
```

### A base class machine keeps its nested transitions on subclass instances when an event enters the case.

```yumi
. thing = Pebble()
thing.tosses <- true
-- thing.body is .falling
thing.landings <- true
-- thing.body is .falling

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ event tosses Bool
+ event landings Bool

────────────────────
+ body ~>

~> resting

~> falling on tosses
    then resting on landings

━━━━━━━━━━━━━━━━━━━━
+ Pebble is Thing
```

```output
true
false
```

### A state's own transition on a stream is not replaced by a continuation's nested transition on it.

```yumi
. hunter = Hunter()
hunter.nap <- true
hunter.go <- true

━━━━━━━━━━━━━━━━━━━━
+ Hunter

+ event rest Bool
+ event nap Bool
+ event go Bool

────────────────────
+ phase ~>

~> chasing
    do
        -- "chasing"

    then resting on rest
        then chasing on go

    then resting on nap

~> resting
    do
        -- "resting"

    then idle on go

~> idle
    do
        -- "idle"
```

```output
chasing
resting
idle
```

### A match binds the payload of a state case that a subclass's refinement adds.

```yumi
. turtle = Turtle()
-- turtle.raft
turtle.boards <- "red"
-- turtle.raft

━━━━━━━━━━━━━━━━━━━━
+ Toy

────────────────────
+ body ~>

~> free

━━━━━━━━━━━━━━━━━━━━
+ Turtle is Toy

+ event boards String

────────────────────
+ body ~>

~> free
    ~> paddling

    ~> riding(name) for name on boards

────────────────────
+ do get raft String

= match body
    | .riding(name)
        name
    ??
        "none"
```

```output
none
red
```

### A closure in a transition condition captures the state's payload.

```yumi
. toy = Toy()
toy.pokes <- true
toy.ticks <- 1.0
-- toy.body is .idle

━━━━━━━━━━━━━━━━━━━━
+ Toy

+ event pokes Bool
+ event ticks F64

────────────────────
+ body ~>

~> idle
    then moving(2) on pokes

~> moving(goal)
    then idle on ticks when check(:: dt => goal > dt)

────────────────────
+ check Bool
    - test Predicate'F64

test(1)
```

```output
true
```

### A closure in a transition source captures the state's payload, and the state's later transitions still fire.

```yumi
. toy = Toy()
toy.pokes <- true
toy.ticks <- 1.0
-- toy.body is .idle

━━━━━━━━━━━━━━━━━━━━
+ Toy

+ event pokes Bool
+ event ticks F64

────────────────────
+ body ~>

~> idle
    then moving(2) on pokes

~> moving(goal)
    then idle on gated(ticks, :: dt => goal > dt)

────────────────────
+ do gated Broadcastable'F64
    - source Broadcastable'F64
    - test Predicate'F64

-- test(1)
= source
```

```output
true
true
```

### Transitions on the same source share one subscription, with or without a when guard.

```yumi
. toy = Toy()
toy.pokes <- true
toy.ticks <- 1.0
-- toy.body is .far

━━━━━━━━━━━━━━━━━━━━
+ Toy

+ event pokes Bool
+ event ticks F64

────────────────────
+ body ~>

~> idle
    then moving(2) on pokes

~> moving(goal)
    then far on watched() when goal < 4
    then idle on watched()

~> far

────────────────────
+ do watched Broadcastable'F64

-- "watch"
= ticks
```

```output
watch
true
```

### A then with no trigger is an entry decision that always passes, after the if decisions before it.

```yumi
. toy = Toy()
-- toy.where
toy.again <- true
-- toy.where

━━━━━━━━━━━━━━━━━━━━
+ Toy

+ event again Bool
+ count Size = 0

────────────────────
+ body ~>

~> free
    then free on again

    ~> planning
        do
            count += 1

        then moving(count * 10) if count > 1
        then moving(5)

        ~> idling

    ~> moving(goal)

────────────────────
+ do get where String

=? "idling" if body is .idling
= match body
    | .moving(goal)
        "moving $goal"
    ??
        "other"
```

```output
moving 5
moving 20
```

### Entry decisions that loop back to a state still being entered are rejected, since entering would never settle.

```yumi crash
. toy = Toy()

━━━━━━━━━━━━━━━━━━━━
+ Toy

+ ready = false

────────────────────
+ body ~>

~> free
    then resting if !ready

    ~> planning

    ~> resting
        then free if ready
```

```output
Exception: Entry decisions loop back to 'free' without a trigger
<no file:18>
            then free if ready
            ^^^^^^^^^^^^^^^^^^
```

### A machine's path lists its active cases from outer to inner, including cases a subclass's refinement adds when read through the base type.

```yumi
. turtle = Turtle()
. toy Toy = turtle
-- " > ".join(toy.body.path)
turtle.grabs <- true
-- " > ".join(toy.body.path)
turtle.releases <- true
turtle.boards <- "red"
-- " > ".join(toy.body.path)
-- turtle.where()
-- " > ".join(Toy().body.path)

━━━━━━━━━━━━━━━━━━━━
+ Toy

+ event grabs Bool
+ event releases Bool

────────────────────
+ body ~>

~> free

~> held on grabs
    then free on releases

━━━━━━━━━━━━━━━━━━━━
+ Turtle is Toy

+ event boards String

────────────────────
+ body ~>

~> free
    ~> paddling
        then riding on boards

    ~> riding

────────────────────
+ do where String

= " > ".join(body.path)
```

```output
free > paddling
held
free > riding
free > riding
free
```

### Refinements of refinements merge into one machine: each level's cases nest under those it names again, its entries and handlers join the case, and a continuation into a case only a base writes moves there.

```yumi
. fisher = Fisher()
-- fisher.body.path
fisher.ticks <- true
-- fisher.body.path
fisher.ticks <- true
-- fisher.body.path
fisher.tosses <- true
-- fisher.body.path
-- "${fisher.body is .casting} ${fisher.body is .held}"

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ event ticks Bool
+ event tosses Bool

────────────────────
+ body ~>

~> free
    do
        -- "enter free"
    do finally
        -- "exit free"

~> held

~> falling on tosses

━━━━━━━━━━━━━━━━━━━━
+ Person is Thing

────────────────────
+ body ~>

~> free
    for n on ticks
        -- "person tick"

    ~> routine
        do
            -- "enter routine"
        do finally
            -- "exit routine"

━━━━━━━━━━━━━━━━━━━━
+ Fisher is Person

────────────────────
+ body ~>

~> free
    ~> routine
        ~> casting
            then fishing on ticks

        ~> fishing
            then free on ticks

~> falling
    do
        -- "fisher falling"
    then held
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

```yumi
. crab = Crab()
crab.ticks <- 1
crab.ticks <- 2
-- crab.body.path
crab.ticks <- 5
-- crab.body.path

━━━━━━━━━━━━━━━━━━━━
+ Animal

+ event ticks Size

────────────────────
+ body ~>

~> free
    then hiding on ticks when each > 3

    ~> idle

~> hiding

━━━━━━━━━━━━━━━━━━━━
+ Crab is Animal

────────────────────
+ body ~>

~> free
    then dashing on ticks when each > 1

    ~> dashing
        do
            -- "dash"
```

```output
dash
free, dashing
hiding
```

### Where no level's transition passes in a group three levels of refinement join, each level's guard is tried once.

```yumi
. hermit = Hermit()
hermit.ticks <- 0
-- hermit.body.path

━━━━━━━━━━━━━━━━━━━━
+ Animal

+ event ticks Size

────────────────────
+ body ~>

~> free
    then hiding on ticks when said("animal", each > 3)

~> hiding

────────────────────
+ do said Bool
    - who String
    - passed Bool

-- "try $who"
= passed

━━━━━━━━━━━━━━━━━━━━
+ Crab is Animal

────────────────────
+ body ~>

~> free
    then dashing on ticks when said("crab", each > 1)

    ~> dashing

━━━━━━━━━━━━━━━━━━━━
+ Hermit is Crab

────────────────────
+ body ~>

~> free
    then shelled on ticks when said("hermit", each == 1)

    ~> shelled
```

```output
try animal
try crab
try hermit
free, dashing
```

### A handler in a refinement tests the cases the refinement adds.

```yumi
. crab = Crab()
crab.ticks <- 1.0
crab.ticks <- 1.0
crab.ticks <- 1.0
-- crab.body.path

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ event ticks F64

────────────────────
+ body ~>

~> free

━━━━━━━━━━━━━━━━━━━━
+ Crab is Thing

+ count Size = 0

────────────────────
+ body ~>

~> free
    for dt on ticks
        count += 1
        -- "${count} ${body is .hiding} ${body is .peeking}"

    then dashing(3.0) on ticks when count == 1

    ~> idle

    ~> dashing(speed)
        then hiding(speed) on ticks

    ~> hiding(rest)
        then peeking on ticks when rest > 1

    ~> peeking
```

```output
1 false false
2 false false
3 true false
free, peeking
```

### A case test reads a refined machine through an index or a cast, evaluating the object once, for cases the refinement adds and cases the base writes.

```yumi
. dogs = [Dog(), Dog()]
dogs[0].chases <- true
-- "${dogs[0].body is .chasing} ${dogs[1].body is .chasing} ${dogs[1].body is .idle}"
-- pick(dogs, 0).body is .chasing
. pet Animal = Dog()
(pet as Dog).calls <- true
-- "${(pet as Dog).body is .answering} ${(pet as Dog).body is .chasing}"

━━━━━━━━━━━━━━━━━━━━
+ pick Dog
    - dogs [Dog]
    - at Index

-- "pick $at"
dogs[at]

━━━━━━━━━━━━━━━━━━━━
+ Animal

+ event calls Bool

────────────────────
+ body ~>

~> free
    then answering on calls

    ~> idle

    ~> answering

━━━━━━━━━━━━━━━━━━━━
+ Dog is Animal

+ event chases Bool

────────────────────
+ body ~>

~> free
    then chasing on chases

    ~> chasing
```

```output
true false true
pick 0
true
true false
```

### A case two transitions give values holds them where the other is written in a case whose payload is settled only after it.

```yumi
+ ticks AsyncChannel'F64
+ taps AsyncChannel'F64
+ arrivals AsyncChannel'F64

+ crab ~>
    ~> free
        then hiding(1.0, 1.0) on taps
        then dashing(5.0, 1.4) on ticks

        ~> idle

        ~> dashing(rest, glimpse)
            then hiding(rest, glimpse) on arrivals

        ~> hiding(rest, glimpse)

ticks <- 1.0
do new
    arrivals <- 1.0
    do new
        -- match crab
            | .hiding(rest, glimpse)
                "$rest $glimpse"
            ??
                "other"
        taps <- 1.0
        do new
            -- match crab
                | .hiding(rest, glimpse)
                    "$rest $glimpse"
                ??
                    "other"
```

```output
5 1.4
1 1
```

### A top-level machine's path lists its active cases from outer to inner.

```yumi
+ ready AsyncChannel'Bool

+ flow ~>
    ~> waiting
        ~> idle
    ~> running on ready

-- " > ".join(flow.path)
ready <- true
do new
    -- " > ".join(flow.path)
```

```output
waiting > idle
running
```

### A match on a machine picks the case it last entered, while a case test also sees its active parent.

```yumi
+ editor ~>
    ~> editing
        ~> focused

-- editor is .editing
-- match editor
    | .editing
        "editing"
    | .focused
        "focused"
    ??
        "other"
```

```output
true
focused
```

### A state machine a property holds runs on the object, so its cases call the object's methods.

```yumi
. worker = Worker()
worker.go <- true
do new
    do new
        -- worker.done

━━━━━━━━━━━━━━━━━━━━
+ Worker

+ done = ""
+ event go Bool

────────────────────
+ phase ~>

~> idle
~> working on go
    do
        work("x")
        this.work("y")

────────────────────
+ do work
    - text String

done += text
```

```output
xy
```

### A state machine whose case reads an element of its class's markup holds nothing until the markup runs, then starts.

```yumi
/core/tests/support/markup-fixtures

. panel = Panel()
-- panel.phase is .idle
panel.advance <- true
do new
    -- panel.phase is .active

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ advance AsyncChannel'Bool

────────────────────
+ phase ~>

~> idle
~> active on advance
    .. shown = inner.text
    do
        -- "active $shown"

────────────────────

<This
    <Label inner "hello"
```

```output
true
active hello
true
```

### A state machine written in the content of a class, on events of the object itself, opens, cleans up and closes as the events arrive.

```yumi
/core/tests/support/markup-fixtures

. host = Host()
-- host.summary()
host.open-requests <- 1
do new
    -- host.summary()
    host.refresh-requests <- 2
    do new
        -- host.summary()
        host.close-requests <- true
        do new
            -- host.summary()
            host.open-requests <- 3
            do new
                -- host.summary()

━━━━━━━━━━━━━━━━━━━━
+ Host is Box

+ event open-requests Int
+ event refresh-requests Int
+ event close-requests Bool
+ log = ""

────────────────────
+ summary String

"${state-label.text}:$log"

────────────────────

<Label state-label
    text = if #is-open then "open" else "closed"

~> is-closed

~> is-open for n on open-requests
    do
        log = log + "[open$n]"

    do finally
        log = log + "[unload]"

    then is-closed on close-requests

────────────────────
for n on refresh-requests

if is-open
    log = log + "[refresh$n]"
```

```output
closed:
open:[open1]
open:[open1][refresh2]
closed:[open1][refresh2][unload]
open:[open1][refresh2][unload][open3]
```

### A class's handler and its state machine may both hear an event of the object itself: the handler runs first, then the transition.

```yumi
. knob = Knob()
knob.pokes <- 1
knob.pokes <- 2
-- "${knob.phase is .poked} ${knob.log}"

━━━━━━━━━━━━━━━━━━━━
+ Knob

+ event pokes Int
+ log = ""

────────────────────
+ phase ~>

~> idle
~> poked for n on pokes
    do
        log += "[enter $n]"

────────────────────
for n on pokes

log += "[poke $n]"
```

```output
true [poke 1][enter 1][poke 2][enter 2]
```

### A machine starts in a case that keeps an object a later transition captures, before any object has arrived.

```yumi
. knob = Knob()
-- knob.is-idle
knob.downs <- Press(1)
-- knob.is-dragging
knob.ups <- Press(2)
-- "${knob.is-idle} ${knob.log}"

━━━━━━━━━━━━━━━━━━━━
+ Press

+ id Int

────────────────────
+ This
    - id

━━━━━━━━━━━━━━━━━━━━
+ Knob

+ event downs Press
+ event ups Press
+ log = ""

────────────────────
~> is-idle
~> is-dragging for down on downs
    then is-idle for up on ups
        do
            log += "[${down.id} to ${up.id}]"
```

```output
true
true
true [1 to 2]
```

### A lazy local that the trigger and the transitions of a state machine read is evaluated once: the trigger evaluates its source when the machine starts, and the first transition that runs evaluates the other.

```yumi
. a AsyncChannel'Int
. b AsyncChannel'Int
.. source = noted(a, "source")
.. label = noted("L", "label")

~> idle
~> one for n on source
    do
        -- "one $n $label"
~> two for n on b
    do
        -- "two $n $label"

-- "start"
a <- 1
do new
    b <- 2
    do new
        a <- 3
        do new
            -- "end"

━━━━━━━━━━━━━━━━━━━━
+ do noted'Value Value
    - value Value
    - name String

-- "evaluated $name"
= value
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

```yumi
/core/tests/support/markup-fixtures

. form = Form()
-- form.summary()
form.submitted <- 3
do new
    do new
        -- form.summary()
        form.submitted <- -1
        do new
            do new
                -- form.summary()

━━━━━━━━━━━━━━━━━━━━
+ Form is Box

+ event submitted Int

────────────────────
+ summary String

"${shown.text}:${is-busy}"

────────────────────

<Label shown "got"

~> is-idle
~> is-busy for .valid(n) on checked
    do
        shown.text = "busy $n"

.. checked = on submitted
    |? it < 0 || shown.text == "x"
        Check.negative
    ??
        Check.valid(it)

━━━━━━━━━━━━━━━━━━━━
+ Check is Union

| negative
| valid(Int)
```

```output
got:false
busy 3:true
busy 3:true
```

### A named machine written in a case of a class content machine subscribes where the case is entered.

```yumi
. t = Thing()
t.keys <- 'x'
do new
    -- t.log

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ event keys Char
+ log = ""

────────────────────

~> outer
    + inner ~>
        ~> idle
            do
                log = "$log i"

        ~> busy on keys
            do
                log = "$log b"
```

```output
 i b
```

### A state machine whose transition waits on an event nothing publishes never takes it, and the cases around it enter and exit as written.

```yumi
. thing = Thing()
-- thing.label
thing.poke()
do new
    -- thing.label
    thing.poke()
    do new
        -- thing.label
        thing.poke()
        do new
            -- thing.label

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ label = "none"
+ event go Bool
+ event other Bool

────────────────────
+ do poke

other <- true

────────────────────

~> idle
    do
        label = "idle"

    then busy on other

~> busy
    do
        label = "busy"

    then idle on go
    then done on other

~> done
    do
        label = "done"

    then busy on go
```

```output
idle
busy
done
done
```

### State machines of the same name, written with the same cases in two modules, are machines of their own: each module tests the case of its own.

```yumi
./lib/first
./lib/second

show-first()
show-second()
```

```yumi file=lib/first.yumi

_ auth ~>
    ~> editing
    ~> authenticated

━━━━━━━━━━━━━━━━━━━━
+ do show-first

-- "first ${auth is .editing} ${auth is .authenticated}"

```

```yumi file=lib/second.yumi

_ has-token = true

_ auth ~>
    ~> editing
    ~> authenticated if has-token

━━━━━━━━━━━━━━━━━━━━
+ do show-second

-- "second ${auth is .editing} ${auth is .authenticated}"

```

```output
first true false
second false true
```

### An error the subscribe of a group of a state's transitions raises goes to unhandled-error, and the program goes on.

```yumi
/core/stream

unhandled-error = :: error => do
    -- "handled ${error.message}"

+ opened AsyncChannel'Bool
_ refusing = RefusingSource()

+ flow ~>
    ~> idle
    ~> active on opened
        then finished on source

        .. source = refusing

    ~> finished

opened <- true
do new
    -- flow is .active
    -- "done"

━━━━━━━━━━━━━━━━━━━━
+ RefusingSource
    is Broadcastable'Bool

────────────────────
+ get is-closed Bool

false

────────────────────
+ do subscribe ?! Error
    - subscriber Subscriber'Bool

?! Error("subscribe refused")

────────────────────
+ do unsubscribe ?! Error
    - subscriber Subscriber'Bool

────────────────────
+ do subscribe ?! Error
    - subscriber Subscribable'Bool

────────────────────
+ do unsubscribe ?! Error
    - subscriber Subscribable'Bool
```

```output
handled subscribe refused
true
done
```

### A state payload and its substates can hold a value of an interface type.

```yumi
. rider = Rider()
. horse = Horse()
rider.saddlings <- horse
-- rider.where

━━━━━━━━━━━━━━━━━━━━
+ Steed is Interface

────────────────────
+ get speed F64

━━━━━━━━━━━━━━━━━━━━
+ Horse
    is Steed

────────────────────
+ get speed

12.0

━━━━━━━━━━━━━━━━━━━━
+ Rider

+ event saddlings Steed

────────────────────
+ body ~>

~> walking

~> mounted(horse) for horse on saddlings
    ~> trotting
        then galloping if horse.speed > 10.0

    ~> galloping

────────────────────
+ do get where String

= match body
    | .galloping
        "galloping"
    | .mounted(horse)
        "riding at ${horse.speed}"
    ??
        "walking"
```

```output
galloping
```

### An unconditional entry decision infers the payload type of the state it enters.

```yumi
. toy = Toy()
-- toy.where

━━━━━━━━━━━━━━━━━━━━
+ Toy

────────────────────
+ body ~>

~> idle
    then moving(spot())

~> moving(goal)

────────────────────
+ do spot F64

= 2.5

────────────────────
+ do get where String

= match body
    | .moving(goal)
        "moving to $goal"
    ??
        "idle"
```

```output
moving to 2.5
```

### An unconditional entry decision infers the payload type of a state declared before it.

```yumi
. toy = Toy()
toy.pokes <- true
-- toy.where

━━━━━━━━━━━━━━━━━━━━
+ Toy

+ event pokes Bool

────────────────────
+ body ~>

~> resting
    then planning on pokes

~> moving(goal)

~> planning
    then moving(spot())

────────────────────
+ do spot F64

= 2.5

────────────────────
+ do get where String

= match body
    | .moving(goal)
        "moving to $goal"
    ??
        "resting"
```

```output
moving to 2.5
```

### A case its owner's continuation with no trigger enters, so that the owner never enters it as its first case, holds what a transition of its own with a trigger gives it.

```yumi
. toy = Toy()
-- toy.where
toy.pokes <- true
-- toy.where

━━━━━━━━━━━━━━━━━━━━
+ Toy

+ event pokes Bool

────────────────────
+ body ~>

~> smarting
    then darting(spot())

    ~> darting(goal)
        then darting(farther()) on pokes

────────────────────
+ do spot F64

= 2.5

────────────────────
+ do farther F64

= 4.5

────────────────────
+ do get where String

= match body
    | .darting(goal)
        "darting to $goal"
    ??
        "smarting"
```

```output
darting to 2.5
darting to 4.5
```

### Each guarded transition of a case on the same event reads what the case holds, though an earlier transition of the case would have left it.

```yumi
. counter = Counter()
counter.ticks <- 0.1
counter.ticks <- 0.1
-- counter.log

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ event ticks F64
+ log = ""

────────────────────
+ phase ~>

~> waiting
    then holding(7) on ticks

~> holding(value)
    then waiting on ticks when seen("a", value)
    then waiting on ticks when seen("b", value)

────────────────────
_ do seen Bool
    - label String
    - value Int

log += "$label$value;"
= false
```

```output
a7;b7;
```

### A transition of a case gives the case it enters a value made of what a case above it holds.

```yumi
. toy = Toy()
toy.ticks <- 0.1
toy.ticks <- 0.1
-- toy.where

━━━━━━━━━━━━━━━━━━━━
+ Toy

+ event ticks F64

────────────────────
+ body ~>

~> waiting
    then holding(7) on ticks

~> holding(value)
    ~> early
        then late(twice(value)) on ticks

    ~> late(spot)

────────────────────
_ twice Int
    - value Int

value * 2

────────────────────
+ do get where String

= match body
    | .late(spot)
        "late at $spot"
    ??
        "elsewhere"
```

```output
late at 14
```

### A guard reads its own class machine by name while a case holds a value a later transition gives it.

```yumi
. knob = Knob()
knob.downs <- 1
knob.downs <- 2
knob.blurs <- true
-- knob.log

━━━━━━━━━━━━━━━━━━━━
+ Knob

+ event downs Int
+ event blurs Bool
+ log = ""

────────────────────
_ gesture ~>

~> idle
~> dragging on downs when !(gesture is .dragging)
    do log += "[drag]"
    then held(7) on blurs

~> held(id)
    do log += "[held $id]"
```

```output
[drag][held 7]
```

### A guard reads its own class machine by name while a case holds a value made from what its trigger captured.

```yumi
. knob = Knob()
knob.downs <- 4
knob.downs <- 9
knob.blurs <- true
-- knob.log

━━━━━━━━━━━━━━━━━━━━
+ Knob

+ event downs Int
+ event blurs Bool
+ log = ""

────────────────────
_ gesture ~>

~> idle
~> dragging(n) for n on downs when !(gesture is .dragging)
    do log += "[drag $n]"
    then held(n + 1) on blurs

~> held(id)
    do log += "[held $id]"
```

```output
[drag 4][held 5]
```

### Cases whose values and kept captures each come from a case the others enter take their types from the triggers that capture them, however the cases enter one another.

```yumi
. knob = Knob()
knob.downs <- Press(4)
knob.blurs <- true
knob.ups <- Press(4)
knob.downs <- Press(5)
-- knob.log

━━━━━━━━━━━━━━━━━━━━
+ Press

+ id Int

────────────────────
+ This
    - id

━━━━━━━━━━━━━━━━━━━━
+ Knob

+ event downs Press
+ event ups Press
+ event blurs Bool
+ log = ""

────────────────────
_ gesture ~>

~> idle
    then dragging(press.id) for press on downs

~> dragging(id)
    do log += "[drag $id]"
    then held(id) on blurs

~> held(id)
    do log += "[held $id]"
    then idle for up on ups
```

```output
[drag 4][held 4][drag 5]
```

### A case one transition gives a value and another gives null holds an optional of the value, so null stays null.

```yumi
. holder = Holder()
holder.modifiers <- true
holder.codes <- 0
-- holder.hold is .held

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ event codes U32
+ event modifiers Bool

────────────────────
+ hold ~>

~> idle
    then held(code) for code on codes
    then held(null) on modifiers

~> held(code)
    then idle on codes when each == code
```

```output
true
```

### An object's `close`, its class's own or one it inherits, stops each machine its properties hold: each exits its active cases, running their `do finally`, and no transition fires after, on the object's events or on another stream.

```yumi
. ticks AsyncChannel'Int
. holder = Holder(ticks)
holder.started <- true
-- "before ${holder.phase is .waiting}"
holder.close()
holder.started <- true
ticks <- 1
do new
    -- "after ${holder.phase is .idle} ${holder.phase is .waiting} ${holder.phase is .done} ${holder.log}"

━━━━━━━━━━━━━━━━━━━━
+ Closer

────────────────────
+ do close

━━━━━━━━━━━━━━━━━━━━
+ Holder is Closer

+ event started Bool
+ log = ""
_ ticks AsyncChannel'Int

────────────────────
+ This
    - ticks

────────────────────
+ phase ~>

~> idle

~> waiting on started
    do finally
        log += "[left waiting]"
    then done on ticks

~> done
    do log += "[done]"
```

```output
before true
after false false false [left waiting]
```

### An object's `close`, its class's own or one it inherits, stops the machine its class's markup writes: the active case exits, running its `do finally`, and no handler or transition fires after.

```yumi
. ticks AsyncChannel'Int
. holder = Holder(ticks)
holder.started <- true
-- "before $holder.waiting"
holder.close()
holder.started <- true
ticks <- 1
do new
    -- "after $holder.idle $holder.waiting $holder.done $holder.log"

━━━━━━━━━━━━━━━━━━━━
+ Closer

────────────────────
+ do close

━━━━━━━━━━━━━━━━━━━━
+ Holder is Closer

+ event started Bool
+ log = ""
_ ticks AsyncChannel'Int

────────────────────
+ This
    - ticks

────────────────────
~> idle

~> waiting on started
    do finally
        log += "[left waiting]"
    for tick on ticks
        log += "[tick $tick]"
    then done on ticks

~> done
    do log += "[done]"
```

```output
before true
after false false false [left waiting]
```

### An error a case's `do finally` raises as an object's `close` stops its machine goes to unhandled-error, and the close goes on.

```yumi
unhandled-error = :: error => do
    -- "handled ${error.message}"

. holder = Holder()
holder.opens <- true
holder.close()
-- "after $holder.is-open $holder.log"

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ event opens Bool
+ log = ""

────────────────────
~> is-closed

~> is-open on opens
    do finally
        unload()

────────────────────
+ do unload ?! Error

log += "[unload]"
?! Error("unload failed")

────────────────────
+ do close
```

```output
handled unload failed
after false [unload]
```

### An object's `close` stops its machines before its body runs, so their exit rows see what the body then closes.

```yumi
. holder = Holder()
holder.opens <- true
holder.close()
-- holder.log

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ event opens Bool
+ resource String? = "file"
+ log = ""

────────────────────
+ phase ~>

~> idle

~> open on opens
    do finally
        log += "[left open with ${resource ?? "nothing"}]"

────────────────────
+ do close

log += "[close]"
resource = null
```

```output
[left open with file][close]
```

### An object's `close` stops its machines before its body runs, and the body still publishes the object's events to its own handlers and to outside subscribers, but not to the stopped machines.

```yumi
. holder = Holder()
for value on holder.ended
    holder.log += "[outside heard]"
holder.opens <- true
holder.close()
-- holder.log

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ event opens Bool
+ event ended Bool
+ log = ""

────────────────────
+ phase ~>

~> idle

~> open on opens
    do log += "[open]"
    do finally
        log += "[left open]"

~> shut on ended
    do log += "[shut]"

────────────────────
for value on ended

log += "[heard ended]"

────────────────────
+ do close

log += "[close]"
opens <- true
ended <- true
```

```output
[open][left open][close][heard ended][outside heard]
```

### A named machine a subclass's refinement writes in a case the base writes starts as the case is entered, its transitions fire while the case is active, and it stops as the case exits.

```yumi
. sub = Sub()
sub.opens <- true
sub.pokes <- 7
-- sub.label
sub.closes <- true
-- sub.label

━━━━━━━━━━━━━━━━━━━━
+ Base

+ event opens Bool
+ event closes Bool

────────────────────
+ presence ~>

~> closed
    then open on opens

~> open
    then closed on closes

━━━━━━━━━━━━━━━━━━━━
+ Sub is Base

+ label = ""
+ event pokes Int

────────────────────
+ presence ~>

~> open
    _ inner ~>
        ~> idle

        ~> busy(n) for n on pokes
            do
                label = "busy $n"

            do finally
                label = "left busy $n"
```

```output
busy 7
left busy 7
```

### A subclass's anonymous machine whose cases name a case at the root of the anonymous machine of the class above refines that machine, as a named machine refines its base's: its cases nest under those it names again, and the cases' flags stay the base's.

```yumi
. sub = Sub()
sub.opens <- true
sub.pokes <- 7
-- "opened ${sub.is-open} ${sub.label}"
sub.closes <- true
-- "closed ${sub.is-open} ${sub.is-closed} ${sub.label}"

━━━━━━━━━━━━━━━━━━━━
+ Base

+ event opens Bool
+ event closes Bool

────────────────────

~> is-closed
    then is-open on opens

~> is-open
    then is-closed on closes

━━━━━━━━━━━━━━━━━━━━
+ Sub is Base

+ label = ""
+ event pokes Int

────────────────────

~> is-open
    _ inner ~>
        ~> idle

        ~> busy(n) for n on pokes
            do
                label = "busy $n"

            do finally
                label = "left busy $n"
```

```output
opened true busy 7
closed false true left busy 7
```

### A refinement of an anonymous machine adds no case at its root.

```yumi crash
. sub = Sub()
-- sub.is-open

━━━━━━━━━━━━━━━━━━━━
+ Base

+ event opens Bool

────────────────────

~> is-closed
    then is-open on opens

~> is-open

━━━━━━━━━━━━━━━━━━━━
+ Sub is Base

────────────────────

~> is-open
    do
        -- "open"

~> is-marked
```

```output
Exception: Refinement cannot add top-level state case 'is-marked'
<no file:26>
    ~> is-marked
    ^^^^^^^^^^^^
```

### A refinement that writes a case's trigger again is rejected, as the case keeps the trigger it has.

```yumi crash
. s = Sub()

━━━━━━━━━━━━━━━━━━━━
+ Base

+ event go Bool

────────────────────
+ body ~>

~> free
~> busy on go

━━━━━━━━━━━━━━━━━━━━
+ Sub is Base

────────────────────
+ body ~>

~> busy on go
```

```output
Exception: Refined state case 'busy' cannot change its trigger or payload
<no file:21>
    ~> busy on go
    ^^^^^^^^^^^^^
```

### A case a refinement adds under a case may come first with a payload, as the base's first case is the one entered there.

```yumi
. pet = Dog()
-- pet.body
pet.chases <- "cat"

━━━━━━━━━━━━━━━━━━━━
+ Animal

+ event calls String

────────────────────
+ body ~>

~> free
    then answering(who) for who on calls

    ~> idle

    ~> answering(who)
        do
            -- "answering ${who}"

━━━━━━━━━━━━━━━━━━━━
+ Dog is Animal

+ event chases String

────────────────────
+ body ~>

~> free
    then chasing(quarry) for quarry on chases

    ~> chasing(quarry)
        do
            -- "chasing ${quarry}"
```

```output
idle
chasing cat
```

### A machine's first case is entered with no transition to give it a payload, so a name it holds is rejected.

```yumi crash
. pet = Animal()

━━━━━━━━━━━━━━━━━━━━
+ Animal

+ event calls String

────────────────────
+ body ~>

~> free
    then answering(who) for who on calls

    ~> answering(who)
```

```output
Exception: Initial state case 'answering' cannot take a payload, since nothing enters it to give it one; give it values, or write a case with no payload before it
<no file:15>
        ~> answering(who)
                     ^^^
```

### An inherited anonymous machine's root transition on an event of its own class fires on an object of a subclass.

```yumi
. sub = SubProbe()
sub.request()
-- sub.is-busy

━━━━━━━━━━━━━━━━━━━━
+ Probe

_ event requested Bool

────────────────────
~> is-idle

~> is-busy on requested

────────────────────
+ do request

requested <- true

━━━━━━━━━━━━━━━━━━━━
+ SubProbe is Probe
```

```output
true
```

### A subclass's anonymous machine written after a long section of markup refines the base's.

```yumi
/core/markup
/core/tests/support/markup-fixtures

. made = Mid()
made.opens <- true
-- "opened ${made.is-open} ${made.label}"

━━━━━━━━━━━━━━━━━━━━
+ Base is Box

+ event opens Bool

────────────────────
~> is-closed
    then is-open on opens

~> is-open

━━━━━━━━━━━━━━━━━━━━
+ Mid is Base

+ label = ""

────────────────────
<This
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item
    <Item

~> is-open
    do
        label = "entered"

━━━━━━━━━━━━━━━━━━━━
+ Item is Box
```

```output
opened true entered
```

### A case keeps a named tuple a transition that enters it captures. Entered with nothing captured, it keeps the tuple its fields' first values make.

```yumi
. holder = Holder()
holder.poked <- true
holder.requested <- true
holder.committed <- Commit("x", false)
holder.poked <- true
-- holder.log

━━━━━━━━━━━━━━━━━━━━
_ Commit is Tuple

+ name String
+ flag = true

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ log = ""
+ event requested Bool
+ event committed Commit
+ event poked Bool

────────────────────
~> is-idle
    for _ on poked
        log += "[${commit.name}:${commit.flag}]"

~> is-renaming on requested
    then is-idle for commit on committed
```

```output
[:true][x:false]
```

### A name a transition captures hides a method of the class of that name.

```yumi
. holder = Holder()
holder.requested <- true
holder.committed <- Commit("x", true)
-- holder.log

━━━━━━━━━━━━━━━━━━━━
_ Commit is Tuple

+ name String
+ flag Bool

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ log = ""
+ event requested Bool
+ event committed Commit

────────────────────
~> is-idle

~> is-renaming on requested
    then is-idle for commit on committed
        do
            log += "[${commit.name}]"

────────────────────
+ do commit

log += "[method]"
```

```output
[x]
```

### A transition on a routed event runs while the event is at the machine's owner, before the route moves on, so it reads the owner's frame and its cancel stops the route there.

```yumi
/core/intrinsics

. root = RoutedNode("root")
. leaf = HoldingNode("leaf", root)

for event on root.changed
    -- "root ${event.value}"

-- "canceled ${leaf.route(1)}"
-- "canceled ${leaf.route(2)}"

━━━━━━━━━━━━━━━━━━━━
+ RoutedChange
    is RoutedEvent

+ value Int
+ current-name String
+ is-canceled = false

────────────────────
+ This
    - value
    - current-name

────────────────────
+ do cancel

is-canceled = true

────────────────────
+ do next-event-owner Pointer?
    - raw-current Pointer

. current = gc-object-from-pointer'RoutedNode(raw-current)
. next = current.parent
if next
    current-name = next.name
    = next as Pointer

━━━━━━━━━━━━━━━━━━━━
+ RoutedNode

+ name String
+ parent RoutedNode?
+ event changed RoutedChange

────────────────────
+ This
    - name
    - parent = null

────────────────────
+ do route Bool
    - value Int

. event = RoutedChange(value, name)
changed <- event
= event.is-canceled

━━━━━━━━━━━━━━━━━━━━
+ HoldingNode is RoutedNode

────────────────────
~> is-idle

~> is-held for press on changed when !is-held
    do
        -- "held ${press.value} ${press.current-name}"

    then is-idle for release on changed
        do
            -- "released ${release.value} ${release.current-name}"
            release.cancel()
```

```output
held 1 leaf
root 1
canceled false
released 2 leaf
canceled true
```

### A routed event that another task publishes while a transition waits in the middle of a drain stays held after that task ends, until the drain dispatches it to machines.

```yumi
/core/intrinsics

. leaf = RoutedNode("leaf", RoutedNode("root"))
. done = SyncBufferedChannel'Bool()
. holder = Holder(done)

for event on leaf.parent!.changed
    -- "root ${event.value}"

do new
    -- "canceled ${leaf.route(1)}"
    do new
        done <- true

holder.requested <- true
-- "settled"

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ done SyncBufferedChannel'Bool
+ event requested Bool

────────────────────
+ This
    - done

────────────────────
~> is-idle

~> is-waiting on requested
    do
        . ignored = <- done
        -- "resumed"

━━━━━━━━━━━━━━━━━━━━
+ RoutedChange
    is RoutedEvent

+ value Int
+ is-canceled = false

────────────────────
+ This
    - value

────────────────────
+ do cancel

is-canceled = true

────────────────────
+ do next-event-owner Pointer?
    - raw-current Pointer

. next = gc-object-from-pointer'RoutedNode(raw-current).parent
if next
    = next as Pointer

━━━━━━━━━━━━━━━━━━━━
+ RoutedNode

+ name String
+ parent RoutedNode?
+ event changed RoutedChange

────────────────────
+ This
    - name
    - parent = null

────────────────────
+ do route Bool
    - value Int

. event = RoutedChange(value)
changed <- event
= event.is-canceled
```

```output
root 1
canceled false
resumed
settled
```

### A case a subclass's refinement adds, tested where the program constructs no object of the subclass, is not active, and an arm that names it matches nothing.

```yumi
. toy = Toy()
-- toy.body is .free
. turtles [Turtle]
for turtle in turtles
    -- "${turtle.is-riding} ${turtle.raft}"
-- turtles.length

━━━━━━━━━━━━━━━━━━━━
+ Toy

────────────────────
+ body ~>

~> free

━━━━━━━━━━━━━━━━━━━━
+ Turtle is Toy

+ event boards String

────────────────────
+ body ~>

~> free
    ~> riding(name) for name on boards

────────────────────
+ do get is-riding Bool

= body is .riding

────────────────────
+ do get raft String

= match body
    | .riding(name)
        name
    ??
        "none"
```

```output
true
0
```

### A case test and path on a copy of a machine read the case the copy holds, not the machine's.

```yumi
+ go AsyncChannel'Bool

+ m ~>
    ~> a
        ~> b
    ~> c on go

. before = m
. door = Door()
. shut = door.phase
go <- true
door.open-requests <- "ada"
do new
    -- "${before is .a} ${before is .b} ${before is .c} ${before.path}"
    -- "${shut is .closed} ${shut is .open} ${door.phase is .open}"

━━━━━━━━━━━━━━━━━━━━
+ Door

+ event open-requests String

────────────────────
+ phase ~>

~> closed
    then open(each) on open-requests

~> open(who)
```

```output
true true false a, b
true false true
```

### Assigning a state machine is rejected, as it moves only by its transitions.

```yumi crash
+ phase ~>
    ~> idle
    ~> busy

phase = .busy
```

```output
Exception: State machine "phase" moves only by its transitions, so it cannot be assigned; add a transition that enters the case
<no file:6>
    phase = .busy
    ^^^^^^^^^^^^^
```

### Assigning the state machine of an object is rejected before its case is looked for.

```yumi crash
. door = Door()
door.phase = .open

━━━━━━━━━━━━━━━━━━━━
+ Door

+ event pushes Bool

────────────────────
+ phase ~>

~> shut
    then open on pushes

~> open
```

```output
Exception: State machine "phase" moves only by its transitions, so it cannot be assigned; add a transition that enters the case
<no file:3>
    door.phase = .open
    ^^^^^^^^^^^^^^^^^^
```

### A when rule that tests a case of the machine runs when the machine enters the case.

```yumi
. door = Door()
door.toggles <- true
door.toggles <- true
door.toggles <- true
do new
    -- door.log

━━━━━━━━━━━━━━━━━━━━
+ Door

+ event toggles Bool
+ log = ""

────────────────────
when #phase is .open

log += "[open]"

────────────────────
+ phase ~>

~> closed
    then open on toggles

~> open
    then closed on toggles
```

```output
[open][open]
```

### A state machine's handler that returns a value with "=" is rejected, as it gives none.

```yumi crash
+ a AsyncChannel'Int

+ phase ~>
    ~> idle
        for x on a
            = 3
```

```output
Exception: A state machine's handler gives no value, so it cannot return one with "="; end it early with "= alone"
<no file:7>
                = 3
                ^^^
```

### A transition that gives a case fewer values than the case names is rejected.

```yumi crash
+ go AsyncChannel'Int

+ m ~>
    ~> idle
        then held on go

    ~> held(v)
```

```output
Exception: Case "held" holds one value, and this transition gives none; give it one value
<no file:6>
            then held on go
            ^^^^^^^^^^^^^^^
```

### A transition that gives a case more values than the case names is rejected.

```yumi crash
+ go AsyncChannel'Int

+ m ~>
    ~> idle
        then held(each, each) on go

    ~> held(v)
```

```output
Exception: Case "held" holds one value, and this transition gives 2; give it one value
<no file:6>
            then held(each, each) on go
            ^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A class's machine whose initial case holds a value starts in it with the value it is given.

```yumi
. c = Counter(4)
-- c.phase

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ start Int

────────────────────
+ This
    - start

────────────────────
+ phase ~>

~> counting(start * 2)
~> done
```

```output
8
```

### A case of a machine in a class's markup named like a property the class declares is rejected.

```yumi crash
. p = P()
-- p.idle

━━━━━━━━━━━━━━━━━━━━
+ P

+ idle = 3

────────────────────
~> idle
~> busy
```

```output
Exception: A case of a machine in a class's markup is read as a property of its name, and the class declares "idle" already; rename the case or the property
<no file:11>
    ~> idle
    ^^^^^^^
```

### A message names a machine's union by its cases.

```yumi crash
+ phase ~>
    ~> closed
    ~> open

-- match phase
    | .closed
        1
```

```output
Exception: This match on state machine (.closed | .open) does not handle .open; add a case for it, or "??" for the rest
<no file:7>
        | .closed
        ^^^^^^^^
```

### A case a match arm names that a machine does not have is reported with the machine's cases.

```yumi crash
+ phase ~>
    ~> idle
    ~> busy

-- match phase
    | .nope
        1
    ??
        2
```

```output
Exception: Case ".nope" not found in state machine (.idle | .busy)
<no file:7>
        | .nope
          ^^^^^
```

### A machine compared with one of its cases by "==" is pointed to "is", which counts nested cases.

```yumi crash
+ phase ~>
    ~> closed
    ~> open

-- phase == .closed
```

```output
Exception: A state machine's case is tested with "is" or "is not", which count the cases nested in it; write "is .closed" in place of "== .closed"
<no file:6>
    -- phase == .closed
                ^^^^^^^
```

### A trigger pattern naming a case its stream's items cannot have is reported.

```yumi crash
+ nums AsyncChannel'Int

+ m ~>
    ~> idle
    ~> b for .valid(x) on nums
```

```output
Exception: Case ".valid" not found: I64 has no cases
<no file:6>
        ~> b for .valid(x) on nums
                 ^^^^^^^^^
```

### A whenever trigger moves a machine as what its value reads with "#" changes, while its case is active. Entering the case, storing the value a property already holds, or a change that leaves the value as it was moves nothing, and a change while another case is active is dropped. The value is read only while its case is active.

```yumi
. w = Watcher()
w.feed = 5
w.go <- true
do new
    -- "entered ${w.flow is .watching}"
    w.feed = 5
    do new
        -- "same value ${w.flow is .watching}"
        w.feed = 4
        do new
            -- "same result ${w.flow is .watching}"
            w.extra = 3
            do new
                -- "changed ${w.flow is .idle}"
                w.feed = 9
                do new
                    -- "while idle ${w.flow is .idle}"
                    w.go <- true
                    do new
                        -- "entered again ${w.flow is .watching}"
                        w.feed = 1
                        do new
                            -- "again ${w.flow is .idle}"

━━━━━━━━━━━━━━━━━━━━
+ Watcher

+ event go Bool
+ feed = 1
+ extra = 0

────────────────────
+ flow ~>

~> idle
~> watching on go
    then idle whenever noted(#feed + #extra) > 6

━━━━━━━━━━━━━━━━━━━━
+ do noted Int
    - n Int

-- "read $n"
= n
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

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

. box = Box()
. flow ~>
    ~> watching
        then done whenever box.#feed
    ~> done
        do
            -- "done"
-- flow is .watching
do new
    box.feed = 2

━━━━━━━━━━━━━━━━━━━━
+ Box

+ feed = 1
```

```output
true
done
```

### A whenever trigger that reads nothing with "#" is reported, as it would never fire.

```yumi crash
. w = Watcher()

━━━━━━━━━━━━━━━━━━━━
+ Watcher

+ feed = 1

────────────────────
+ flow ~>

~> idle
~> watching
    then idle whenever feed
```

```output
Exception: A "whenever" trigger fires as what it reads with "#" changes, and this one reads nothing with "#", so it would never fire; write "#" before what it follows, as in "whenever #feed"
<no file:14>
        then idle whenever feed
                 ^^^^^^^^^^^^^^
```
