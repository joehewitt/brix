# Import

```yumi
/foo
```

```brix
(import /foo)
```

```output
<Import path="foo"/>
```

```yumi
/foo/bar
```

```brix
(import /foo/bar)
```

```output
<Import path="foo/bar"/>
```

```yumi
/foo/bar/baz
```

```brix
(import /foo/bar/baz)
```

```output
<Import path="foo/bar/baz"/>
```

```yumi
./foo
```

```brix
(import ./foo)
```

```output
<Import path="./foo"/>
```

```yumi
../foo
```

```brix
(import ../foo)
```

```output
<Import path="../foo"/>
```

```yumi
./foo/bar
```

```brix
(import ./foo/bar)
```

```output
<Import path="./foo/bar"/>
```

```yumi
/foo/bar.h
```

```brix
(import /foo/bar.h)
```

```output
<Import path="foo/bar.h"/>
```

```yumi
+ /foo
```

```brix
(export /foo)
```

```output
<Import path="foo" export="true"/>
```

```yumi
+ ./foo
```

```brix
(export ./foo)
```

```output
<Import path="./foo" export="true"/>
```
