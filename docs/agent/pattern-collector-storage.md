# Pattern collector storage

A match helper can implement `PatternCollectorStorage'Backing` and expose
`pattern-collector-context` with type `Backing.Context`. The compiler uses that
backing for its accumulated captures, repeated captures, repeat results, and
collectors created during capture materialization. Matches without this capability
retain GC backing. Storage selection is independent of `PatternRollback` and does
not enable physical rollback.

The compiler forwards the concrete provider to direct pattern helpers, including
static pattern arguments. Borrowed accumulator arguments carry the exact item and
backing types in their pointer ABI. Their callee updates the caller's collector
variable; it does not finish or replace the caller's ownership.

Collectors use the private core `PatternCollector` tuple: an Array, logical size,
and backing context. Empty construction allocates nothing. Arena growth shares the
native sequence storage operation, without ArrayBuilder identity headers,
storage checkpoints, or mutation generations. Storage containing managed pointers,
including heap Strings, has described GC roots and traced writes. Root registration
belongs to the arena and ends at its reset or release.

A collector snapshot exposes the existing ordinary ImmutableArray view. This view borrows
the collector buffer; erasing the backing type does not copy storage or transfer
ownership. A helper that supplies scratch must keep that scratch alive through every
consumer of these views, and copy any escaping values into their final owner before
reset or release. Buffer lifetime and element lifetime are separate: GC elements
remain rooted while held by scratch, but scratch-owned text still needs its text
owner to remain alive.

A `~>` pipe matches many records under one helper, so a helper that supplies scratch also
implements `PatternRecordBoundary`. The pipe calls its `begin-pattern-record` where the stream has
more, before it marks the stream and matches the next record. By then every record before has been
delivered and none of the pipe's collectors is live, so the helper can rewind its scratch to where
it started; the records' values must already own their storage. A whole-input match never calls
it. `PatternScratch` in `core/pattern-helpers` stores collectors in an arena and rewinds it this
way; `JSONParser` delegates both capabilities to one, and JSON Lines readers pass one with
`~> \json-line with JSONParser(scratch)`.

Logical rollback restores accumulator sizes and clears discarded managed references.
Growth is monotonic. Failed alternatives can leave old buffers and unused capacity
behind until bulk cleanup; they do not create arena marks or rewind storage.
