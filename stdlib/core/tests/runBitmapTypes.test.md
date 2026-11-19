# Bitmap types

```brix
(main
  (print (. I1 bit-width))
  (print (. I8 bit-width))
  (print (. U16 bit-width))
  (print (. I32 bit-width))
  (print (. U64 bit-width))
)
```

```output
1
8
16
32
64
```

```brix
(main
  (print (. Size bit-width))
  (print (. Byte bit-width))
  (print (. Bool bit-width))
)
```

```output
64
8
1
```

```brix
(main
  (print (. I1 is-integer))
  (print (. F64 is-integer))
)
```

```output
true
false
```

```brix
(main
  (print (. I1 min))
  (print (. I1 max))
)
```

```output
0
1
```

```brix
(import /core/bitmap)
(blank)

(main
  (let narrow (call (of Bitmap I8)))
  (print (. narrow capacity))
  (print (. (type-of narrow) size))
  (print (as narrow I8))
  (let wide (call (of Bitmap U64)))
  (print (. wide capacity))
  (print (. (type-of wide) size))
  (print (as wide U64))
)
```

```output
8
1
0
64
8
0
```

```brix
(import /core/bitmap)
(blank)

(main
  (print (. (call (of Bitmap Char)) capacity))
  (print (. (call (of Bitmap Char8)) capacity))
  (print (. (call (of Bitmap Index)) capacity))
  (print (. (call (of Bitmap Flags)) capacity))
)

(type _ Flags (is U16))
```

```output
32
8
64
16
```

```brix crash
(main
  (call (of Bits F32))
)

(type + (of Bits Integer)
  (fn-do + This _
    (. Integer bit-width)
  )
)
```

```output
Exception: Integer type required for bit-width
<no file:8>
        (. Integer bit-width)
        ^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (call (of Bits F64))
)

(type + (of Bits Integer)
  (fn-do + This _
    (. Integer bit-width)
  )
)
```

```output
Exception: Integer type required for bit-width
<no file:8>
        (. Integer bit-width)
        ^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (call (of Bits Float))
)

(type + (of Bits Integer)
  (fn-do + This _
    (. Integer bit-width)
  )
)
```

```output
Exception: Integer type required for bit-width
<no file:8>
        (. Integer bit-width)
        ^^^^^^^^^^^^^^^^^^^^^
```
