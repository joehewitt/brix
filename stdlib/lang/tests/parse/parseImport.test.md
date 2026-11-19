# Import

```brix
(import /foo)
```

```output
<Import path="foo"/>
```

```brix
(import /foo/bar)
```

```output
<Import path="foo/bar"/>
```

```brix
(import /foo/bar/baz)
```

```output
<Import path="foo/bar/baz"/>
```

```brix
(import ./foo)
```

```output
<Import path="./foo"/>
```

```brix
(import ../foo)
```

```output
<Import path="../foo"/>
```

```brix
(import ./foo/bar)
```

```output
<Import path="./foo/bar"/>
```

```brix
(import /foo/bar.h)
```

```output
<Import path="foo/bar.h"/>
```

```brix
(export /foo)
```

```output
<Import path="foo" export="true"/>
```

```brix
(export ./foo)
```

```output
<Import path="./foo" export="true"/>
```
