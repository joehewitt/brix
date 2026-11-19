# Expression binary

```brix
(main
  (+ foo bar)
)
```

```output
<BinaryExpression operator="+">
    <Id name="foo"/>
    <Id name="bar"/>
</BinaryExpression>
```

```brix
(main
  (- foo bar)
)
```

```output
<BinaryExpression operator="-">
    <Id name="foo"/>
    <Id name="bar"/>
</BinaryExpression>
```

```brix
(main
  (* foo bar)
)
```

```output
<BinaryExpression operator="*">
    <Id name="foo"/>
    <Id name="bar"/>
</BinaryExpression>
```

```brix
(main
  (/ foo bar)
)
```

```output
<BinaryExpression operator="/">
    <Id name="foo"/>
    <Id name="bar"/>
</BinaryExpression>
```

```brix
(main
  (% foo bar)
)
```

```output
<BinaryExpression operator="%">
    <Id name="foo"/>
    <Id name="bar"/>
</BinaryExpression>
```

```brix
(main
  (+ foo (* bar 2))
)
```

```output
<BinaryExpression operator="+">
    <Id name="foo"/>
    <BinaryExpression operator="*">
        <Id name="bar"/>
        <Integer value="2"/>
    </BinaryExpression>
</BinaryExpression>
```

```brix
(main
  (/bar/ foo baz)
)
```

```output
<CallExpression>
    <Id name="bar"/>
    <Argument>
        <Id name="foo"/>
    </Argument>
    <Argument>
        <Id name="baz"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (//bar// foo baz)
)
```

```output
<CallExpression>
    <Id name="bar"/>
    <Argument>
        <Id name="foo"/>
    </Argument>
    <Argument>
        <Id name="baz"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (///x/// foo baz)
)
```

```output
<CallExpression>
    <Id name="x"/>
    <Argument>
        <Id name="foo"/>
    </Argument>
    <Argument>
        <Id name="baz"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (///x/// a (/y/ b c))
)
```

```output
<CallExpression>
    <Id name="x"/>
    <Argument>
        <Id name="a"/>
    </Argument>
    <Argument>
        <CallExpression>
            <Id name="y"/>
            <Argument>
                <Id name="b"/>
            </Argument>
            <Argument>
                <Id name="c"/>
            </Argument>
        </CallExpression>
    </Argument>
</CallExpression>
```

```brix
(main
  (///y/// (/x/ a b) c)
)
```

```output
<CallExpression>
    <Id name="y"/>
    <Argument>
        <CallExpression>
            <Id name="x"/>
            <Argument>
                <Id name="a"/>
            </Argument>
            <Argument>
                <Id name="b"/>
            </Argument>
        </CallExpression>
    </Argument>
    <Argument>
        <Id name="c"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (== a b)
)
```

```output
<BinaryExpression operator="==">
    <Id name="a"/>
    <Id name="b"/>
</BinaryExpression>
```

```brix
(main
  (!= a b)
)
```

```output
<BinaryExpression operator="!=">
    <Id name="a"/>
    <Id name="b"/>
</BinaryExpression>
```

```brix
(main
  (> a b)
)
```

```output
<BinaryExpression operator=">">
    <Id name="a"/>
    <Id name="b"/>
</BinaryExpression>
```

```brix
(main
  (< a b)
)
```

```output
<BinaryExpression operator="<">
    <Id name="a"/>
    <Id name="b"/>
</BinaryExpression>
```

```brix
(main
  (>= a b)
)
```

```output
<BinaryExpression operator=">=">
    <Id name="a"/>
    <Id name="b"/>
</BinaryExpression>
```

```brix
(main
  (<= a b)
)
```

```output
<BinaryExpression operator="<=">
    <Id name="a"/>
    <Id name="b"/>
</BinaryExpression>
```

```brix
(main
  (and a b)
)
```

```output
<BinaryExpression operator="&&">
    <Id name="a"/>
    <Id name="b"/>
</BinaryExpression>
```

```brix
(main
  (or a b)
)
```

```output
<BinaryExpression operator="||">
    <Id name="a"/>
    <Id name="b"/>
</BinaryExpression>
```

```brix
(main
  (or (and a b) (and c d))
)
```

```output
<BinaryExpression operator="||">
    <BinaryExpression operator="&&">
        <Id name="a"/>
        <Id name="b"/>
    </BinaryExpression>
    <BinaryExpression operator="&&">
        <Id name="c"/>
        <Id name="d"/>
    </BinaryExpression>
</BinaryExpression>
```

```brix
(main
  (is-in foo bar)
)
```

```output
<BinaryExpression operator="is in">
    <Id name="foo"/>
    <Id name="bar"/>
</BinaryExpression>
```

```brix
(main
  (not-in foo bar)
)
```

```output
<BinaryExpression operator="not in">
    <Id name="foo"/>
    <Id name="bar"/>
</BinaryExpression>
```

```brix
(main
  (has foo bar)
)
```

```output
<BinaryExpression operator="has">
    <Id name="foo"/>
    <Id name="bar"/>
</BinaryExpression>
```

```brix
(main
  (has-not foo bar)
)
```

```output
<BinaryExpression operator="has not">
    <Id name="foo"/>
    <Id name="bar"/>
</BinaryExpression>
```

```brix
(main
  (as foo Foo)
)
```

```output
<CastExpression>
    <Id name="foo"/>
    <TypeId name="Foo"/>
</CastExpression>
```

```brix
(main
  (as foo Foo Bar Baz)
)
```

```output
<CastExpression>
    <CastExpression>
        <CastExpression>
            <Id name="foo"/>
            <TypeId name="Foo"/>
        </CastExpression>
        <TypeId name="Bar"/>
    </CastExpression>
    <TypeId name="Baz"/>
</CastExpression>
```

```brix
(main
  (is foo Bar)
)
```

```output
<BinaryExpression operator="is">
    <Id name="foo"/>
    <TypeId name="Bar"/>
</BinaryExpression>
```

```brix
(main
  (and (> i 0) (== i 0))
)
```

```output
<BinaryExpression operator="&&">
    <BinaryExpression operator=">">
        <Id name="i"/>
        <Integer value="0"/>
    </BinaryExpression>
    <BinaryExpression operator="==">
        <Id name="i"/>
        <Integer value="0"/>
    </BinaryExpression>
</BinaryExpression>
```

```brix
(main
  (> 10 i 0)
)
```

```output
<BinaryExpression operator="&&">
    <BinaryExpression operator=">">
        <Integer value="10"/>
        <Held>
            <Id name="i"/>
        </Held>
    </BinaryExpression>
    <BinaryExpression operator=">">
        <HeldRead/>
        <Integer value="0"/>
    </BinaryExpression>
</BinaryExpression>
```

```brix
(main
  (> (>= (> 9 i) j) 0)
)
```

```output
<BinaryExpression operator="&&">
    <BinaryExpression operator=">">
        <Integer value="9"/>
        <Held>
            <Id name="i"/>
        </Held>
    </BinaryExpression>
    <BinaryExpression operator="&&">
        <BinaryExpression operator=">=">
            <HeldRead/>
            <Held>
                <Id name="j"/>
            </Held>
        </BinaryExpression>
        <BinaryExpression operator=">">
            <HeldRead/>
            <Integer value="0"/>
        </BinaryExpression>
    </BinaryExpression>
</BinaryExpression>
```
