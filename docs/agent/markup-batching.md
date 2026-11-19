# Markup Batching and the Mutation Protocol

This document is the authoritative guide to Brix's markup batching runtime
(`stdlib/core/markup.bx`) for library and framework authors. It covers the batch lifecycle, the
contracts each tier of client must uphold, the `checkpoint()` boundary, and the compiler-driven
mutation protocol that parents implement.

Application code never needs anything in this document. Declarative markup, region loops and
branches, and styled property writes are fully driven by compiler-generated code; there are no
lifecycle calls on the app-facing surface.

## The batch lifecycle

Every markup tree hangs under a *batch root*: an object implementing `MarkupRootElement`, which owns a
`MarkupBatchQueue`. Reactive work flows through the queue in one direction:

```
dirty ──> enqueue ──> schedule ──> flush ──> settle ──> did-flush
```

1. **Dirty.** A tracked value changes. Compiler-generated live actions (`MarkupBatchAction`)
   fire from tracker notifications and coalesce themselves into their root's queue. Styled
   property writes mark sidecar records dirty in the same store the settle pass drains.
2. **Enqueue.** `MarkupBatchQueue.enqueue` appends the action and requests a flush. Actions
   coalesce: an action already enqueued stays put; an action that dirties itself *while running*
   re-enqueues for the next pass.
3. **Schedule.** The queue spawns a single `@priority` task to flush. Scheduling is idempotent —
   one scheduled flush serves any number of enqueues, and a flush already in progress absorbs
   re-entrant schedule requests.
4. **Flush.** Queued actions run in passes until the queue drains or the safety caps trip
   (`max-actions-per-checkpoint`, default 4096 actions; `max-passes-per-checkpoint`, default 16
   passes). Actions that enqueue new actions extend the same flush; runaway feedback loops hit
   the caps and the remainder is rescheduled rather than looping forever.
5. **Settle.** After the action passes, the style settle hook runs computed style state to a
   fixed point. Dirty transitions raised *during* settling are drained by the same settle pass —
   the flush stays in progress through settling, so they never schedule a second flush.
6. **Did-flush.** If any actions applied or settling did work, the root's
   `markup-batch-did-flush(flush)` is called with a `MarkupBatchFlush` summary: the number of
   actions applied plus compiler-generated pending-domain accessors (e.g. `flush.is-layout-invalid`,
   `flush.is-paint-invalid`). This is the push path the root uses to translate batch work into
   phase work.

## The three client tiers

The batching contract is deliberately asymmetric: almost everyone is in tier 1, and each tier
down is rarer and carries a slightly larger contract.

| Tier | Who | Contract |
|---|---|---|
| 1. App / component authors | Anyone writing markup, styles, regions | **None.** No lifecycle calls; everything is compiler-generated. |
| 2. Markup parent implementors | Box-like classes that store markup children | Implement the mutation protocol hooks (below). No adopt/release, root, or settle calls. |
| 3. Batch root implementors | Window-like phase controllers | Implement `MarkupRootElement` (witness-enforced), translate `did-flush` into phase scheduling, and call `checkpoint()` at phase boundaries. |

A tree that needs no layout or paint, only styles and live markup, uses core's ready-made node
and root instead of joining tiers 2 and 3: its nodes extend `MarkupNode`, which makes the regions
of live loops and branches, and its outermost node extends `MarkupRoot`, which owns the batch,
checkpoints it once its markup has been built, and checkpoints again on `settle()`. The terminal
library's `Screen` is the same kind of root for boxes painted off screen: `frame(width, height)`
checkpoints, lays out, paints and returns the frame as text.

### Tier 2: the mutation protocol

Every object in a live tree implements `MarkupElement`: it mounts and unmounts, reads styles and
themes, and closes. An element that holds children implements `MarkupParent'Child`, which extends
`MarkupElement` and is the interface between markup and domain-specific tree storage. It mirrors
the collection interface from the opposite direction: where `Collection` describes how *data*
binds into markup (regions iterate it and build rows), `MarkupParent'Child` describes how *markup*
binds into a domain's tree representation (Box sibling links today; a DOM or scene graph tomorrow).
The domain keeps full control of its data structures; the core keeps full control of the
lifecycle.

Parents declare five kinds of hooks **with their own concrete types**. The compiler and the generic
core entry points dispatch them statically on the parent, so an unaccepted child type is a
*compile error at the mutation site* — never a silent no-op or a runtime panic. `MarkupParent'Child`
gives a default for each hook a parent with nothing of its own to do would write: an identity
`markup-content` for a `Child`, empty attach and detach hooks, and a `children` generator over the
structural links that gives a region's rows in place of the region. A parent declares only the hooks
that do something.

`children` means those rows for every parent, the terminal and graphical boxes included, so code
that asks a node for its children gets the same answer from any library. A walk that must visit
the region nodes themselves, such as layout, paint or close, uses `structural-children`, which
those boxes declare over their raw sibling links.

The hooks:

- `markup-content` overloads — acceptance and coercion. Declare which child values the parent
  accepts and return the node that actually links, e.g. Box declares `(child Box) Box` identity
  and `(content String) Box` returning a `TextBox`. The returned node feeds markup `id` bindings
  and live replacers.
- `markup-absorb` overloads — consumption without linking. Content the parent folds into its own
  state instead of linking a child, e.g. `TextBox` assigns string content to its `text` property.
  Absorb wins over `markup-content` when both match, and re-dispatches on dynamic content updates.
- `markup-attach-child(child, anchor)` — link `child` immediately before `anchor` (null
  means link as last child). Pure link bookkeeping plus parent-side invalidation.
- `markup-detach-child(child)` — unlink `child`. Same scope.
- `markup-make-region()` — return an empty node of the child class to hold the rows of a live loop
  or the arm of a live branch. Core links it among the parent's children and gives it the controller
  that builds, reconciles and closes the rows, so a parent writes no loop or branch logic of its own.
  The region's `close` calls `markup-close-region(this)` before closing as any other node does. The
  checker reports a live loop or branch under a parent that doesn't declare this hook.

The structural hooks are members of the generic `MarkupParent'Child` interface, and a class
with linked children **must declare conformance** with its own child type:

```yumi
+ Box
    is MarkupParent'Box
```

The declaration is the parent's compliance witness: the core entry points require their parent to
conform, so a type that merely happens to define hook-shaped methods is rejected rather than
duck-typed, and conformance is queryable wherever the type system needs it. The interface does
not introduce dynamic dispatch — the entry points are generic over the concrete parent, so the
hooks still compile to direct, statically dispatched calls with the parent's own child types, and
no witness tables or boxing are involved. (Content/absorb hooks remain overload sets rather than
interface members because they are heterogeneous: one parent commonly accepts several unrelated
content types.)

Traversal reads the markup sidecar's structural links directly, so parents do not own parent,
child, or sibling storage.

The structural hooks run **only inside the core entry points** — `markup-append`,
`markup-insert`, `markup-remove`, `markup-replace` — which own the choreography the hooks must
never perform themselves:

- Link precedes mount callbacks: the sidecar structure links first, then the completed subtree
  adopts under the parent's root, publishes query/style context, and finally runs `mounted`
  callbacks. A callback can rely on `%parent`, ancestor lookup, descendants queries, and root
  context being coherent.
- Unlink precedes release: `markup-detach-child` runs *before* the subtree releases, so the
  released subtree never observes itself half-linked.
- Replace composes the two: link and mount the replacement, detach and release the old child, then
  run the replacement's `mounted` callbacks against the final structure.

Parents therefore contain **zero** `markup-adopt`/`markup-release` calls. If you find yourself
wanting one, you are either mutating a tree outside the entry points (use them instead) or
presenting out-of-tree structure (see the portal API below).

Compiler-generated construction calls the same entry points: child objects are built and linked
while unmounted, then the parent append or root commit mounts the completed subtree. The pruned
mount walk still makes appends into unmounted subtrees and same-root rehomes O(1), not O(subtree).

### The portal API: `markup-adopt` / `markup-release`

Some elements present structure that lives *beside* their child list rather than in it. `VectorImage`
(`stdlib/gui/vector-image.bx`) mounts its vector group, a separate graphics tree, when it mounts
and releases it when it unmounts, and `VectorGroup` (`stdlib/graphics/vector-group.bx`) does the
same for its vector nodes. Such structure never flows through attach/detach, so the element mounts it
explicitly:

- `markup-adopt(node, parent, root)` mounts, unmounts, or rehomes a node and its subtree without
  changing any parent links.
- `markup-release(node)` unmounts the subtree; the counterpart on teardown.

This is a narrow API for elements with out-of-tree structure. Ordinary tree mutation must never call
it — the entry points own that lifecycle.

### Tier 3: batch root implementors

A batch root is the object that owns a tree's queue and converts batch completion into externally
scheduled phase work (layout, paint, commit — whatever the domain's phases are). The contract:

1. **Implement `MarkupRootElement`** — provide `markup-batch-queue` (typically
   `:= MarkupBatchQueue(this)`). This half is witness-enforced: forgetting it is a compile
   error. `markup-batch-did-flush` is optional; a root with no phases to schedule leaves it out.
2. **Translate did-flush into phase scheduling.** The flush summary carries pending-domain
   accessors; the root schedules exactly the phases whose domains raised:

   ```yumi
   + do markup-batch-did-flush
       - flush MarkupBatchFlush

   if flush.is-layout-invalid || flush.is-paint-invalid
       catch schedule-paint()
   ```

3. **Checkpoint at phase boundaries.** Before running a phase that reads computed state, drain
   the batch synchronously so the phase observes stable values (next section).
4. **Mount the tree.** Imperatively constructed roots mount themselves once with
   `markup-mount-root(this, this)`; markup-constructed roots are mounted by generated code, and
   adoption prunes already-mounted subtrees so doing both is harmless.

## `checkpoint()` — the synchronous batch boundary

`MarkupBatchQueue.checkpoint()` is the rendezvous between two independently scheduled worlds: the
queue's flush task and the root's externally driven phases (terminal paint events, vsync, commit
requests). Two async schedules with a data dependency need exactly one explicit synchronization
point; checkpoint is its spelling.

**Semantics.** Checkpoint drains queued actions and settles style to a fixed point, *now*, on the
caller's task. A clean queue and a clean store both exit immediately, so calling it
unconditionally at a phase boundary costs nothing when there is no pending work. After checkpoint
returns, computed style records are stable and the pending-domain state reflects exactly the work
the phases must do.

**Fail-soft when omitted.** A root that never calls checkpoint still converges: dirty state
schedules the flush task, the flush's did-flush summary schedules the phase, and the phase
eventually runs against settled state. What checkpoint buys is *exactness* — no stale frame
painted and then corrected by a second pass, and a deterministic first paint. The mandatory half
of the root contract (`markup-batch-did-flush`) is compile-enforced; checkpoint is the
optimization layered on top. This asymmetry is intentional.

### Which live actions batch

Batching is an ownership property, not a global change to `LiveAction`. Compiler-generated
rules, tracked initializers, markup bindings, and property-dependency callbacks whose owner
implements `MarkupElement` use `MarkupBatchAction`; ordinary actions keep their synchronous dispatch
semantics. An unmounted markup owner also runs synchronously because it has no batch root.

Direct property-dependency callbacks are slightly narrower. They run synchronously for ordinary
property writes, preserving invalidation and subscription behavior outside a batch. If a callback
is dispatched while its mounted root is already flushing, it joins that root's dirty-action queue
and runs in a later pass. This prevents dependency fanout from recursively expanding the current
callback while keeping standalone property notification unchanged.

Generated callbacks reuse two action-owned dependency buffers. Property memberships are keyed by
the observed owner identity and a compiler-interned property id, so an unchanged or reordered
dependency retains its subscriber handle instead of detaching and re-registering. Conditional
reads still remove dependencies that disappear and attach new dependencies before the next
dispatch. Runtime-provided property names use a String-equality fallback without sharing identity
with compiler-interned ids. The buffers are action-owned, while their active bindings are
task-scoped during callback evaluation, restored across nested callbacks, scrubbed after use and
on error, and never shared between concurrent tasks.

Property dispatch walks stable removable slots up to the generation present at dispatch start; it
does not allocate a subscriber snapshot. Removing self, an earlier subscriber, or a later
subscriber is safe. Subscribers added during a callback wait until the next dispatch, and nested
dispatches take their own generation boundary. Large dependency sets use pointer-keyed membership
tables for reconciliation so removal does not degrade to quadratic list scans.

Within a queue, the first notification fixes an action's position. Later notifications before it
runs only mark the same action dirty, so the callback observes the latest property state. An action
that dirties itself while running is appended to the next pass. Subscription changes made by a
callback affect later dispatches, never the dispatch already in progress; closing an owner removes
its pending actions through the normal owned-action teardown.

**Error containment.** Queued actions run client callbacks, and client code can throw.
The two paths handle this differently, on purpose:

- The *scheduled* flush task has no caller to receive an error, so the queue wraps it in `catch`
  and contains failures.
- A *synchronous* `checkpoint()` propagates client errors to its caller. Tests usually want
  propagation; phase controllers never do — a bad style callback must not kill the paint loop.
  Phase controllers therefore call `catch markup-batch-queue.checkpoint()`.

**Runaway protection.** Both checkpoint and the scheduled flush honor the queue caps:
`max-actions-per-checkpoint` (4096) bounds total actions per flush, and
`max-passes-per-checkpoint` (16) bounds feedback passes, so an action loop that keeps re-dirtying
itself yields back to the scheduler instead of hanging the boundary. Remaining work reschedules.

**Where to call it.** At the top of any externally triggered phase that reads computed style or
layout state — once per boundary, not per node, not per mutation. Calling checkpoint from inside
a batch action or a settle callback is re-entrant and will be absorbed by the in-progress flush
(it exits immediately); do not rely on it for ordering within a flush.

## Worked example: Window

`stdlib/tui/window.bx` is the reference root implementation. The complete batching surface of
a production root is three small pieces:

```yumi
+ Window is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)
```

1. **Mount** — once, when the terminal is ready. Adoption prunes already-mounted subtrees, so
   markup-constructed windows are unaffected:

   ```yumi
   on terminal.ready

   markup-mount-root(this, this)
   ```

2. **Push path** — settling raised persistent domain bits on the nodes whose declared properties
   changed; the summary tells the window whether a repaint is warranted at all:

   ```yumi
   + do markup-batch-did-flush
       - flush MarkupBatchFlush

   if flush.is-layout-invalid || flush.is-paint-invalid
       catch schedule-paint()
   ```

3. **Phase boundary** — the paint event arrives on the terminal's schedule, not the queue's, so
   the window checkpoints before running its phases. Error containment keeps a throwing client
   action from killing the paint loop:

   ```yumi
   + paint
       - canvas Canvas

   catch markup-batch-queue.checkpoint()
   update-layout()
   ` ... paint children ...
   ```

Note what is *absent*: no settle calls, no root propagation, no per-node mount bookkeeping, no
explicit flush requests. The first paint settles the initial styles because mount dirtiness
scheduled the queue and the paint checkpoints it.

## Quick reference

| API | Tier | Purpose |
|---|---|---|
| `markup-append(parent, child)` | core entry point | Link child as last child; owns link-before-mounted commit |
| `markup-insert(parent, child, anchor)` | core entry point | Link child before anchor (null = last) |
| `markup-remove(parent, child)` | core entry point | Unlink child; owns release-after-unlink |
| `markup-replace(parent, old, new)` | core entry point | Swap: link replacement, release old, then run replacement `mounted` |
| `markup-content` / `markup-absorb` | parent hook | Typed acceptance / coercion / consumption |
| `markup-attach-child` / `markup-detach-child` | `MarkupParent'Child` hook | Pure link bookkeeping + invalidation; conformance required, empty by default |
| `markup-make-region` | `MarkupParent'Child` hook | An empty child node to hold a live loop's rows or a branch's arm; core owns its controller |
| `markup-close-region(node)` | region `close` | Closes the controller a region holds |
| `MarkupNode` / `MarkupRoot` | core classes | A ready-made node with live regions, and a root that settles when built and on `settle()` |
| `markup-mount-root(node, root)` | root / codegen | Mount a node (and subtree) under a batch root |
| `markup-adopt` / `markup-release` | portal | Out-of-tree structure only (such as a vector image's graphics tree) |
| `markup-root-of(node)` | query | The batch root a node is mounted under (sidecar state) |
| `MarkupBatchQueue.checkpoint()` | root | Synchronous drain + settle at a phase boundary |
| `MarkupBatchQueue.enqueue` / `schedule` | runtime | Action coalescing; clients never call these |
