# Path Watching

`Path` exposes two levels of filesystem observation.

The `changed`, `deleted`, and `renamed` streams observe one open filesystem object through vnode
events. Use them when the identity of that exact path matters and a change to a descendant should
not invalidate the consumer.

`watch(is-recursive)` observes a directory or tree and publishes `PathWatchEvent` invalidations.
On macOS it is backed by FSEvents on a private serial dispatch queue. File events can be coalesced,
and their order is best effort. Consumers should use each event path and flags to mark cached state
dirty, then stat or read the affected files again.

When `PathWatchEvent.must-rescan` is true, the native service could not preserve complete
per-path history. Rebuild the consumer's snapshot or index from the event root instead of trying to
apply the individual event as a precise edit.

```brix
(for (on event (call (. project-root watch) true))
  (if (. event must-rescan)
    (call rebuild-index (. event root))
    (else
      (call refresh-path (. event path))
    )
  )
)
```

Event paths are real paths: the native service resolves the watched root before it starts. Resolve
the root with `Path.resolved` before watching when event paths will be compared with paths built
from that root, or a symlinked root such as `/tmp` will never match.

`Path.entries` lists a directory in one pass and reports each entry's name, kind, and inode without
a `stat` per entry. Use it instead of `contents` when a consumer re-lists directories in response to
events; the inode lets it recognise a rename as the same item.
