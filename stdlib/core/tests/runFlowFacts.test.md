# Flow facts

Compiler flow facts

### Arena handles are ordinary copied arguments.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (print (call borrow-arena arena))
)

(fn-do + borrow-arena U32
  (param arena Arena)
  (let value (call (of (. arena allocate) U32) 1))
  (= ([] value 0) 42u32)
  (return ([] value 0))
)
```

```output
42
```

### Arena handles may be copied into aliases.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (let alias arena)
  (let value (call (of (. alias allocate) U32) 1))
  (= ([] value 0) 17u32)
  (print ([] value 0))
  (call (. alias release))
  (call (. arena release))
)
```

```output
17
```

### Arena handles may be returned, reassigned, and ignored without ownership flow facts.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call make-arena 21u32))
  (let cleanup arena)
  (let first (call (of (. arena allocate) U32) 1))
  (= ([] first 0) 22u32)
  (print ([] first 0))
  (= arena (call Arena))
  (let second (call (of (. arena allocate) U32) 1))
  (= ([] second 0) 23u32)
  (print ([] second 0))
  (call ignored-arena)
  (call (. cleanup release))
  (call (. arena release))
)

(fn-do + make-arena Arena
  (param value U32)
  (let arena (call Arena))
  (let stored (call (of (. arena allocate) U32) 1))
  (= ([] stored 0) value)
  (return arena)
)

(fn-do + ignored-arena Arena
  (return (call Arena))
)
```

```output
22
23
```

### A copied Arena argument may explicitly reset its caller's storage.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (print (call borrow-and-reset arena))
)

(fn-do + borrow-and-reset Size
  (param arena Arena)
  (call (. arena reset))
  (return (. arena state))
)
```

```output
1
```

### Arena allocation pointers may be returned from helpers.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let returned (call returned-pointer arena))
  (print ([] returned 0))
)

(fn-do + returned-pointer (of CPointer U32)
  (param arena Arena)
  (let result (call (of (. arena allocate) U32) 1))
  (= ([] result 0) 1u32)
  (return result)
)
```

```output
1
```

### Arena pointer returns require no unique input provenance.

```brix
(import /core/arena)
(blank)

(main
  (let first (call Arena))
  (let second (call Arena))
  (let returned (call ambiguous-pointer first second))
  (print ([] returned 0))
  (call (. first release))
  (call (. second release))
)

(fn-do + ambiguous-pointer (of CPointer U32)
  (param first Arena)
  (param second Arena)
  (let result (call (of (. first allocate) U32) 1))
  (= ([] result 0) 1u32)
  (return result)
)
```

```output
1
```

### A function may use copied and locally created Arena handles independently.

```brix
(import /core/arena)
(blank)

(main
  (let borrowed (call Arena))
  (finally
    (call (. borrowed release))
  )
  (print (call local-owner-lifecycle borrowed))
)

(fn-do + local-owner-lifecycle U32
  (param borrowed Arena)
  (let local (call Arena))
  (finally
    (call (. local release))
  )
  (call (. local reset))
  (let result (call (of (. borrowed allocate) U32) 1))
  (= ([] result 0) 9u32)
  (return ([] result 0))
)
```

```output
9
```

### Arena allocation pointers may be nested in storage owned by the same Arena.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let item (call (of (. arena allocate) U32) 1))
  (= ([] item 0) 7u32)
  (let holder (call (of (. arena allocate) ArenaHolder) 1))
  (= ([] holder 0) (call ArenaHolder item))
  (print ([] (. ([] holder 0) item) 0))
)

(type + ArenaHolder (is Tuple)
  (prop + item (of CPointer U32))
)
```

```output
7
```

### Cross-Arena allocation pointers are legal pointer-free payload.

```brix
(import /core/arena)
(blank)

(main
  (let first (call Arena))
  (let second (call Arena))
  (let item (call (of (. second allocate) U32) 1))
  (= ([] item 0) 7u32)
  (let holder (call (of (. first allocate) ArenaHolder) 1))
  (= ([] holder 0) (call ArenaHolder item))
  (print ([] (. ([] holder 0) item) 0))
  (call (. first release))
  (print ([] item 0))
  (call (. second release))
)

(type + ArenaHolder (is Tuple)
  (prop + item (of CPointer U32))
)
```

```output
7
7
```

### Slot reuse changes the incarnation, so releasing a stale alias cannot affect its replacement.

```brix
(import /core/arena)
(blank)

(main
  (let original (call Arena))
  (let alias original)
  (call (. original release))
  (let replacement (call Arena))
  (call (. alias release))
  (let value (call (of (. replacement allocate) U32) 1))
  (= ([] value 0) 2u32)
  (print ([] value 0))
  (call (. replacement release))
)
```

```output
2
```

```brix
(main
  (let name (? Atom) (as "name" Atom))
  (let key (? Atom) (as "key" Atom))
  (if (and name (!= name key))
    (print (. name text))
  )
)
```

```output
name
```

```brix
(main
  (let owner (call Owner))
  (call (. owner test))
  (print "published")
)

(type + Owner
  (prop _ target (? Target) (= (call Target)))
  (fn-do + test _
    (if target
      (<- (. target changed) true)
    )
  )
)

(type + Target
  (event + changed Bool)
)
```

```output
published
```

```brix
(main
  (let owner (call Owner))
  (call (. owner test))
  (print "released")
)

(type + Owner
  (prop _ target (? Target) (= (call Target)))
  (fn-do + test _
    (if target
      (call (. ([] (. target messages) 1) release))
    )
  )
)

(type + Target
  (event + messages Int U64)
)
```

```output
released
```

```brix
(main
  (let owner (call Owner))
  (if false
    (call (. owner test))
  )
  (print "rekeyed")
)

(type + Owner
  (prop _ target (? Target) (= (call Target)))
  (fn-do + test _
    (if target
      (call (. ([] (. target messages) 1) rekey) 2)
    )
  )
)

(type + Target
  (event + messages Int U64)
)
```

```output
rekeyed
```
