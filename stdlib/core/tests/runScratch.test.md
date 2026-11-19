# Scratch

Scratch stacks

A Scratch is an arena-backed stack of plain values with marks, rewinds, and read-only spans
over finished runs.

### Scratch pushes, indexes, marks, and rewinds.

```brix
(import /core/arena)
(import /core/scratch)
(blank)

(main
  (let arena (use (call Arena)))
  (let scratch (call (of Scratch U32) arena 2))
  (call (. scratch push) 1)
  (call (. scratch push) 2)
  (let mark (. scratch mark))
  (call (. scratch push) 3)
  (call (. scratch push) 4)
  (print (. scratch length))
  (print ([] scratch 3))
  (= ([] scratch 1) 20)
  (print (call (. ", " join) (call (. scratch since) mark)))
  (print (call (. ", " join) (call (. scratch span) 1 2)))
  (call (. scratch rewind) mark)
  (print (. scratch length))
  (print (call (. ", " join) (call (. scratch since) 0)))
)
```

```output
4
4
3, 4
20, 3
2
1, 20
```

### A span taken before growth keeps reading the run it was taken from; later writes land in the current storage.

```brix
(import /core/arena)
(import /core/scratch)
(blank)

(main
  (let arena (use (call Arena)))
  (let scratch (call (of Scratch U32) arena 2))
  (call (. scratch push) 10)
  (call (. scratch push) 20)
  (let early (call (. scratch since) 0))
  (call (. scratch push) 30)
  (call (. scratch push) 40)
  (= ([] scratch 0) 99)
  (print (call (. ", " join) early))
  (print (call (. ", " join) (call (. scratch since) 0)))
  (print (. early length))
)
```

```output
10, 20
99, 20, 30, 40
2
```

### Rewinding below a span's mark and pushing again is a caller error; the span then reads whatever the current storage holds.

```brix
(import /core/arena)
(import /core/scratch)
(blank)

(main
  (let arena (use (call Arena)))
  (let scratch (call (of Scratch U32) arena 8))
  (call (. scratch push) 1)
  (call (. scratch push) 2)
  (let run (call (. scratch since) 0))
  (call (. scratch rewind) 0)
  (call (. scratch push) 7)
  (call (. scratch push) 8)
  (print (call (. ", " join) run))
)
```

```output
7, 8
```

### Scratch grows from its default capacity and reports its mark.

```brix
(import /core/arena)
(import /core/scratch)
(blank)

(main
  (let arena (use (call Arena)))
  (let scratch (call (of Scratch Int) arena))
  (for (in value (..< 0 5))
    (call (. scratch push) (* value value))
  )
  (print (call (. ", " join) (call (. scratch since) 0)))
  (print (. scratch mark))
)
```

```output
0, 1, 4, 9, 16
5
```
