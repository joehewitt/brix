# Syntax

Syntax errors at a known trap say what Brix expects instead

### label given twice on one line

```brix crash
(main
  (print (call some (list 3 1) (:where (> each 1)) (:where (> each 2))))
)
```

```output
Exception: "where" is given twice in one call; the bullets on a line all belong to one call, so give a nested call its bullets inside parentheses, as in "• where (some(each.kids) • where each > 2)"
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
