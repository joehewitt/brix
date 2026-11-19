# Expression range

```brix
(main
  (..< foo bar)
)
```

```output
<RangeExpression operator="..">
    <Id name="foo"/>
    <Id name="bar"/>
</RangeExpression>
```

```brix
(main
  (.. foo bar)
)
```

```output
<RangeExpression operator="thru">
    <Id name="foo"/>
    <Id name="bar"/>
</RangeExpression>
```

```brix
(main
  (..< foo bar (by: baz))
)
```

```output
<RangeExpression operator="..">
    <Id name="foo"/>
    <Id name="bar"/>
    <Id name="baz"/>
</RangeExpression>
```

```brix
(main
  (call call (:from (..< foo bar (by: baz))))
)
```

```output
<CallExpression>
    <Id name="call"/>
    <Argument name="from">
        <RangeExpression operator="..">
            <Id name="foo"/>
            <Id name="bar"/>
            <Id name="baz"/>
        </RangeExpression>
    </Argument>
</CallExpression>
```

```brix
(main
  (... 0)
)
```

```output
<RangeExpression operator="..">
    <Integer value="0"/>
    <Get name="end">
        <TypeId name="Position"/>
    </Get>
</RangeExpression>
```

```brix
(main
  (... _ 0)
)
```

```output
<RangeExpression operator="..">
    <Get name="first">
        <TypeId name="Position"/>
    </Get>
    <Integer value="0"/>
</RangeExpression>
```
