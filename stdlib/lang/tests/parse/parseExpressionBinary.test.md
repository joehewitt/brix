# Expression binary

```yumi
foo + bar
```

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

```yumi
foo - bar
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

```yumi
foo * bar
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

```yumi
foo / bar
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

```yumi
foo % bar
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

```yumi
foo + bar * 2
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

```yumi
foo /bar/ baz
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

```yumi
foo //bar// baz
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

```yumi
foo ///x/// baz
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

```yumi
a ///x/// b /y/ c
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

```yumi
a /x/ b ///y/// c
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

```yumi
a == b
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

```yumi
a != b
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

```yumi
a > b
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

```yumi
a < b
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

```yumi
a >= b
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

```yumi
a <= b
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

```yumi
a && b
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

```yumi
a || b
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

```yumi
a && b || c && d
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

```yumi
foo is in bar
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

```yumi
foo not in bar
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

```yumi
foo has bar
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

```yumi
foo has not bar
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

```yumi
foo as Foo
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

```yumi
foo as Foo as Bar as Baz
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

```yumi
foo is Bar
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

```yumi
i > 0 && i == 0
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

```yumi
10 > i > 0
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

```yumi
9 > i >= j > 0
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
