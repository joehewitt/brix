# Stream call scopes

```brix
(main
  (print (call collect (*> (.. 1 3) (* each 2))))
  (print (call collect (?> (.. 1 4) (> each 2))))
)

(fn-expr + collect _
  (param source (stream Int))
  (call (. "," join) source)
)
```

```output
2,4,6
3,4
```

```brix
(main
  (print (call greatest (*> (.. 1 3) each)))
  (print (call greatest 4 2))
)

(fn-expr + (of greatest T) _
  (param source (stream T))
  "stream"
)

(fn-expr + (of greatest T) _
  (param a T)
  (param b T)
  "scalar"
)
```

```output
stream
scalar
```

```brix
(main
  (print (call greatest (*> (.. 1 3) each)))
  (print (call greatest 4 2))
)

(fn-expr + (of greatest T) _
  (param a T)
  (param b T)
  "scalar"
)

(fn-expr + (of greatest T) _
  (param source (stream T))
  "stream"
)
```

```output
stream
scalar
```

```brix
(main
  (print (catch (call fold-values)))
  (print (call fold 0 (list 1 2 3) (=> (left right) (+ left right))))
  (print (call fold 0 (call (of List Int)) (=> (left right) (+ left right))))
)

(type + FoldFailure (is Error))

(type + (of FoldAccumulator Accum Item Result) (is (fn-type Result (param accum Accum) (param item Item))))

(fn-do + (of fold Item Result) _ @inline
  (param seed Result)
  (param source (stream Item))
  (param accumulator (of FoldAccumulator Result Item))
  (let accum seed)
  (for (in item source)
    (= accum (call accumulator accum item))
  )
  (return accum)
)

(fn-expr + fold-values Int
  (call fold 0 (list 1 2 3)
    (:accumulator
      (=> (left right) :do
        (block
          (throw (call FoldFailure "remainder") (if (== right 2)))
          (return (+ left right))
        )
      )
    )
  )
)
```

```output
remainder
6
0
```

```brix
(main
  (print (. (call combined) value))
)

(fn-expr + combined _
  (list (call ScanValue 1) (call ScanValue 2))
  (pipe |> (call combine (:accumulator (=> (a b) (call ScanValue (+ (. a value) (. b value)))))))
)

(type + ScanValue (is Tuple)
  (prop + value Int)
)

(type + (of Combine Item Result) (is (fn-type Result (param a Item) (param b Item))))

(fn-expr + (of combine Item) _
  (param source (list Item))
  (param accumulator (of Combine Item))
  (call accumulator ([] source 0) ([] source 1))
)

(fn-expr + (of combine Item) _
  (param seed Item)
  (param source (list Item))
  (param accumulator (of Combine Item))
  (call accumulator seed ([] source 0))
)
```

```output
3
```

### Inline stages bind looser than comparisons and arithmetic, like stages on their own lines.

```brix
(main
  (let values (list 1 -2 3))
  (print (call List (?> values (> each 0))))
  (print (call List (*> values (* each 10))))
)
```

```output
1, 3
10, -20, 30
```
