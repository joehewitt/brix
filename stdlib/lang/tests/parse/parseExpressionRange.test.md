# Expression range

```yumi
foo .. < bar
```

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

```yumi
foo .. bar
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

```yumi
foo .. < bar by baz
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

```yumi
call • from foo .. < bar by baz
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

```yumi
0 ...
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

```yumi
... 0
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
