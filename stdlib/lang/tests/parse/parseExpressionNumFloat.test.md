# Expression num float

Floating point numbers

```yumi
4.2
```

```brix
(main
  4.2
)
```

```output
<Float value="4.2"/>
```

```yumi
1.2'3
```

```brix
(main
  1.2'3
)
```

```output
<Float value="1.23"/>
```

```yumi
-4.2
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

```yumi
4.2e+0
```

```brix
(main
  4.2e+0
)
```

```output
<Float value="4.2e+0"/>
```

```yumi
4.2e+1
```

```brix
(main
  4.2e+1
)
```

```output
<Float value="4.2e+1"/>
```

```yumi
4.2e+2
```

```brix
(main
  4.2e+2
)
```

```output
<Float value="4.2e+2"/>
```

```yumi
1e1'0
```

```brix
(main
  1e1'0
)
```

```output
<Float value="1e10"/>
```

```yumi
4.2e+3'0
```

```brix
(main
  4.2e+3'0
)
```

```output
<Float value="4.2e+30"/>
```

```yumi
4.2e+20
```

```brix
(main
  4.2e+20
)
```

```output
<Float value="4.2e+20"/>
```

```yumi
4.2e+350
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

```yumi
1.7976931348623158e+308
```

```brix
(main
  1.7976931348623158e+308
)
```

```output
<Float value="1.7976931348623158e+308"/>
```

### More than maximum float

```yumi
1.7976931348623159e+308
```

```brix
(main
  1.7976931348623159e+308
)
```

```output
<Float value="1.7976931348623159e+308"/>
```

### Two more than maximum float

```yumi
2.7976931348623158e+308
```

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

```yumi
2.2250738585072014e-308
```

```brix
(main
  2.2250738585072014e-308
)
```

```output
<Float value="2.2250738585072014e-308"/>
```

### Less than minimum float

```yumi
-2.2250738585072015e+308
```

```brix
(main
  -2.2250738585072015e+308
)
```

```output
<Float value="-2.2250738585072015e+308"/>
```

### Two less than minimum float

```yumi
-2.2250738585072016e+308
```

```brix
(main
  -2.2250738585072016e+308
)
```

```output
<Float value="-2.2250738585072016e+308"/>
```

## Invalid separators

```yumi
1'.0
```

```output
Exception: Syntax error
<no file:2>
    1'.0
      ^
```

```yumi
1.'0
```

```output
Exception: Syntax error
<no file:2>
    1.'0
       ^
```

```yumi
1e'3
```

```output
Exception: Syntax error
<no file:2>
    1e'3
       ^
```

```yumi
1e+'3
```

```output
Exception: Syntax error
<no file:2>
    1e+'3
      ^
```
