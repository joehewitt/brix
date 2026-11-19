# Syntax

Syntax errors at a known trap say what Brix expects instead

### label given twice on one line

```brix crash
(main
  (print (call some (list 3 1) (:where (> each 1)) (:where (> each 2))))
)
```

```output
Exception: "where" is given twice in one call; a nested call takes its own named arguments inside its parentheses, as in "(call some groups (:where (call some (. each kids) (:where (> each 2)))))"
<no file:3>
      (print (call some (list 3 1) (:where (> each 1)) (:where (> each 2))))
                                                       ^^^^^^^^^^^^^^^^^^
```

### hash comment

```brix crash
(main
  #note
  (print 1)
)
```

```output
Exception: Name "note" not found
<no file:3>
      #note
      ^^^^^
```
