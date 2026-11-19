# Expression num int

Integers

```brix
(main
  0
)
```

```output
<Integer value="0"/>
```

```brix
(main
  42
)
```

```output
<Integer value="42"/>
```

```brix
(main
  1'000'000'000
)
```

```output
<Integer value="1000000000"/>
```

```brix
(main
  0xff'ff
)
```

```output
<Integer value="65535" unit="u16"/>
```

```brix
(main
  0xf
)
```

```output
<Integer value="15" unit="u8"/>
```

```brix
(main
  0x400
)
```

```output
<Integer value="1024" unit="u16"/>
```

```brix
(main
  0x1'2345
)
```

```output
<Integer value="74565" unit="u32"/>
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

```brix
(main
  2147483647
)
```

```output
<Integer value="2147483647"/>
```

### Minimum integer

```brix
(main
  -2147483647
)
```

```output
<Integer value="-2147483647"/>
```
