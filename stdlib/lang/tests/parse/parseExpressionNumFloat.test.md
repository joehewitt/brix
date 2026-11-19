# Expression num float

Floating point numbers

```brix
(main
  4.2
)
```

```output
<Float value="4.2"/>
```

```brix
(main
  1.2'3
)
```

```output
<Float value="1.23"/>
```

```brix
(main
  -4.2
)
```

```output
<Float value="-4.2"/>
```

## Scientific notation

```brix
(main
  4.2e+0
)
```

```output
<Float value="4.2e+0"/>
```

```brix
(main
  4.2e+1
)
```

```output
<Float value="4.2e+1"/>
```

```brix
(main
  4.2e+2
)
```

```output
<Float value="4.2e+2"/>
```

```brix
(main
  1e1'0
)
```

```output
<Float value="1e10"/>
```

```brix
(main
  4.2e+3'0
)
```

```output
<Float value="4.2e+30"/>
```

```brix
(main
  4.2e+20
)
```

```output
<Float value="4.2e+20"/>
```

```brix
(main
  4.2e+350
)
```

```output
<Float value="4.2e+350"/>
```

## Maximum extreme

### Maximum float

```brix
(main
  1.7976931348623158e+308
)
```

```output
<Float value="1.7976931348623158e+308"/>
```

### More than maximum float

```brix
(main
  1.7976931348623159e+308
)
```

```output
<Float value="1.7976931348623159e+308"/>
```

### Two more than maximum float

```brix
(main
  2.7976931348623158e+308
)
```

```output
<Float value="2.7976931348623158e+308"/>
```

## Minimum extreme

### Minimum float

```brix
(main
  2.2250738585072014e-308
)
```

```output
<Float value="2.2250738585072014e-308"/>
```

### Less than minimum float

```brix
(main
  -2.2250738585072015e+308
)
```

```output
<Float value="-2.2250738585072015e+308"/>
```

### Two less than minimum float

```brix
(main
  -2.2250738585072016e+308
)
```

```output
<Float value="-2.2250738585072016e+308"/>
```
