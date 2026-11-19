# Expression calls

```brix
(main
  (call foo)
)
```

```output
<CallExpression>
    <Id name="foo"/>
</CallExpression>
```

```brix
(main
  (call foo a)
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument>
        <Id name="a"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (call foo a b)
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument>
        <Id name="a"/>
    </Argument>
    <Argument>
        <Id name="b"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (call (call foo a) b)
)
```

```output
<CallExpression>
    <CallExpression>
        <Id name="foo"/>
        <Argument>
            <Id name="a"/>
        </Argument>
    </CallExpression>
    <Argument>
        <Id name="b"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (. foo bar)
)
```

```output
<Get name="bar">
    <Id name="foo"/>
</Get>
```

```brix
(main
  (. foo bar baz)
)
```

```output
<Get name="baz">
    <Get name="bar">
        <Id name="foo"/>
    </Get>
</Get>
```

```brix
(main
  ([] foo 1)
)
```

```output
<Index>
    <Id name="foo"/>
    <Integer value="1"/>
</Index>
```

```brix
(main
  ([] ([] foo 1) 2)
)
```

```output
<Index>
    <Index>
        <Id name="foo"/>
        <Integer value="1"/>
    </Index>
    <Integer value="2"/>
</Index>
```

```brix
(main
  ([] (. foo bar) 1)
)
```

```output
<Index>
    <Get name="bar">
        <Id name="foo"/>
    </Get>
    <Integer value="1"/>
</Index>
```

```brix
(main
  (call ([] (. foo bar) 1) 2)
)
```

```output
<CallExpression>
    <Index>
        <Get name="bar">
            <Id name="foo"/>
        </Get>
        <Integer value="1"/>
    </Index>
    <Argument>
        <Integer value="2"/>
    </Argument>
</CallExpression>
```

```brix
(main
  ([] (call (. foo bar)) 0)
)
```

```output
<Index>
    <CallExpression>
        <Get name="bar">
            <Id name="foo"/>
        </Get>
    </CallExpression>
    <Integer value="0"/>
</Index>
```

```brix
(main
  (call (of foo Bar))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <params>
        <TypeId name="Bar"/>
    </params>
</CallExpression>
```

```brix
(main
  (call foo (:by bar))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="by">
        <Id name="bar"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (call foo (:of bar) (:by bar))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="of">
        <Id name="bar"/>
    </Argument>
    <Argument name="by">
        <Id name="bar"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (call (of foo Bar) (:at bar))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <params>
        <TypeId name="Bar"/>
    </params>
    <Argument name="at">
        <Id name="bar"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (call (of (. foo bar) Bar) (:at bar))
)
```

```output
<CallExpression>
    <Get name="bar">
        <Id name="foo"/>
    </Get>
    <params>
        <TypeId name="Bar"/>
    </params>
    <Argument name="at">
        <Id name="bar"/>
    </Argument>
</CallExpression>
```

```brix
(main
  ([] (of Foo Bar Baz) 0)
)
```

```output
<Index>
    <TypeArguments>
        <TypeId name="Foo"/>
        <TypeId name="Bar"/>
        <TypeId name="Baz"/>
    </TypeArguments>
    <Integer value="0"/>
</Index>
```

```brix
(main
  (<- (?. foo bar) 1)
)
```

```output
<CallExpression>
    <Get name="<-write">
        <Get name="bar" optional="true">
            <Id name="foo"/>
        </Get>
    </Get>
    <Argument>
        <Integer value="1"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (?<- foo bar)
)
```

```output
<CallExpression>
    <Get name="<-write" optional="true">
        <Id name="foo"/>
    </Get>
    <Argument>
        <Id name="bar"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (of foo Bar)
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <params>
        <TypeId name="Bar"/>
    </params>
</CallExpression>
```

## Named arguments

```brix
(main
  (call foo (:bar))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="bar">
        <Id name="bar"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (call foo (:bar 42))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="bar">
        <Integer value="42"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (call foo (:where 42))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="where">
        <Integer value="42"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (call foo (:is-recursive true))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="is-recursive">
        <Id name="true"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (call foo (:is-recursive false))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="is-recursive">
        <Id name="false"/>
    </Argument>
</CallExpression>
```

```brix
(main
  (call foo ...)
)
```

```output
<CallExpression curried="true">
    <Id name="foo"/>
</CallExpression>
```

```brix
(main
  (call foo 1 ...)
)
```

```output
<CallExpression curried="true">
    <Id name="foo"/>
    <Argument>
        <Integer value="1"/>
    </Argument>
</CallExpression>
```
