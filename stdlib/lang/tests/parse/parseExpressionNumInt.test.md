# Expression num int

Integers

```yumi
0
```

```brix
(main
  0
)
```

```output
<Integer value="0"/>
```

```yumi
42
```

```brix
(main
  42
)
```

```output
<Integer value="42"/>
```

```yumi
1'000'000'000
```

```brix
(main
  1'000'000'000
)
```

```output
<Integer value="1000000000"/>
```

```yumi
0xff'ff
```

```brix
(main
  0xff'ff
)
```

```output
<Integer value="65535" unit="u16"/>
```

```yumi
0xf
```

```brix
(main
  0xf
)
```

```output
<Integer value="15" unit="u8"/>
```

```yumi
0x400
```

```brix
(main
  0x400
)
```

```output
<Integer value="1024" unit="u16"/>
```

```yumi
0x1'2345
```

```brix
(main
  0x1'2345
)
```

```output
<Integer value="74565" unit="u32"/>
```

```yumi
-5
```

```brix
(main
  -5
)
```

```output
<Integer value="-5"/>
```

## Extremes

### Maximum integer

```yumi
2147483647
```

```brix
(main
  2147483647
)
```

```output
<Integer value="2147483647"/>
```

### Minimum integer

```yumi
-2147483647
```

```brix
(main
  -2147483647
)
```

```output
<Integer value="-2147483647"/>
```

## Invalid separators

```yumi
1''000
```

```output
Exception: Syntax error
<no file:2>
    1''000
       ^
```

```yumi
1'000'
```

```output
Exception: Syntax error
<no file:2>
    1'000'
```

```yumi
0x'ff
```

```output
Exception: Syntax error
<no file:2>
    0x'ff
       ^
```

### A hex number's digits give its type, so a suffix after them is an error that says so.

```yumi
0x70u8
```

```output
Exception: A hex number takes no suffix: its digits give its type, two a U8, four a U16, eight a U32 and more a U64, so write leading zeros to widen it
<no file:2>
    0x70u8
    ^^^^^^
```
