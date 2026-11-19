# Expression match if

```brix
(main
  (only-if foo (if bar))
)
```

```output
<BinaryExpression operator="if">
    <Id name="foo"/>
    <Id name="bar"/>
</BinaryExpression>
```

```brix
(main
  (if-expr foo c2 c1)
)
```

```output
<BinaryExpression operator="else">
    <BinaryExpression operator="if">
        <Id name="c2"/>
        <Id name="foo"/>
    </BinaryExpression>
    <Id name="c1"/>
</BinaryExpression>
```

```brix
(main
  (?? c2 c1)
)
```

```output
<BinaryExpression operator="??">
    <Id name="c2"/>
    <Id name="c1"/>
</BinaryExpression>
```

```brix
(main
  (if-expr foo c3 (if-expr bar c2 c1))
)
```

```output
<BinaryExpression operator="else">
    <BinaryExpression operator="if">
        <Id name="c3"/>
        <Id name="foo"/>
    </BinaryExpression>
    <BinaryExpression operator="else">
        <BinaryExpression operator="if">
            <Id name="c2"/>
            <Id name="bar"/>
        </BinaryExpression>
        <Id name="c1"/>
    </BinaryExpression>
</BinaryExpression>
```

### An `is` test against a list pattern ends before `then`, so it can be the condition of an if expression.

```brix
(main
  (if-expr (is value (list String)) c2 c1)
)
```

```output
<BinaryExpression operator="else">
    <BinaryExpression operator="if">
        <Id name="c2"/>
        <BinaryExpression operator="is">
            <Id name="value"/>
            <TypeList>
                <TypeId name="String"/>
            </TypeList>
        </BinaryExpression>
    </BinaryExpression>
    <Id name="c1"/>
</BinaryExpression>
```
