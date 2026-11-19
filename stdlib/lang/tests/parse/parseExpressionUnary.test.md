# Expression unary

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
