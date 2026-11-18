# Expression unary

```yumi
!foo
```

```brix
(main
  (not foo)
)
```

```output
<UnaryExpression operator="!">
    <Id name="foo"/>
</UnaryExpression>
```

```yumi
!!foo
```

```brix
(main
  (not (not foo))
)
```

```output
<UnaryExpression operator="!">
    <UnaryExpression operator="!">
        <Id name="foo"/>
    </UnaryExpression>
</UnaryExpression>
```

```yumi
! foo
```

```output
Exception: Syntax error
<no file:2>
    ! foo
     ^
```

```yumi
-foo
```

```brix
(main
  (- foo)
)
```

```output
<UnaryExpression operator="-neg">
    <Id name="foo"/>
</UnaryExpression>
```

```yumi
- foo
```

```output
<Argument>
    <Id name="foo"/>
</Argument>
```

```yumi
<-foo
```

```brix
(main
  (<- foo)
)
```

```output
<UnaryExpression operator="<-read">
    <Id name="foo"/>
</UnaryExpression>
```

```yumi
foo!
```

```brix
(main
  (!! foo)
)
```

```output
<UnaryExpression operator="!">
    <Id name="foo"/>
</UnaryExpression>
```

```yumi
foo?.bar
```

```brix
(main
  (?. foo bar)
)
```

```output
<Get name="bar" optional="true">
    <Id name="foo"/>
</Get>
```

```yumi
foo??
```

```output
Exception: Syntax error
<no file:2>
    foo??
        ^
```
