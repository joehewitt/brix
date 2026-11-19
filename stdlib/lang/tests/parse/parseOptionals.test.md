# Optionals

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

```brix
(main
  (?. (?. foo bar) baz)
)
```

```output
<Get name="baz" optional="true">
    <Get name="bar" optional="true">
        <Id name="foo"/>
    </Get>
</Get>
```

```brix
(main
  (?. (. foo bar) baz)
)
```

```output
<Get name="baz" optional="true">
    <Get name="bar">
        <Id name="foo"/>
    </Get>
</Get>
```

```brix
(main
  (?. ([] foo 0) baz)
)
```

```output
<Get name="baz" optional="true">
    <Index>
        <Id name="foo"/>
        <Integer value="0"/>
    </Index>
</Get>
```

```brix
(main
  ([] (?. foo bar) 0)
)
```

```output
<Index>
    <Get name="bar" optional="true">
        <Id name="foo"/>
    </Get>
    <Integer value="0"/>
</Index>
```

```brix
(main
  (?. (call foo) bar)
)
```

```output
<Get name="bar" optional="true">
    <CallExpression>
        <Id name="foo"/>
    </CallExpression>
</Get>
```

```brix
(main
  (?. (call (?. foo bar)) baz)
)
```

```output
<Get name="baz" optional="true">
    <CallExpression>
        <Get name="bar" optional="true">
            <Id name="foo"/>
        </Get>
    </CallExpression>
</Get>
```

```brix
(main
  (=? foo bar)
  (+=? foo bar)
  (return? foo)
  (yield? foo)
  (use? foo)
  (print? foo)
)
```

```output
<AssignmentExpression op="=">
    <Id name="foo"/>
    <OptionalChain>
        <Id name="bar"/>
    </OptionalChain>
</AssignmentExpression>
<AssignmentExpression op="+=">
    <Id name="foo"/>
    <OptionalChain>
        <Id name="bar"/>
    </OptionalChain>
</AssignmentExpression>
<Return>
    <OptionalChain>
        <Id name="foo"/>
    </OptionalChain>
</Return>
<Yield>
    <OptionalChain>
        <Id name="foo"/>
    </OptionalChain>
</Yield>
<Use>
    <OptionalChain>
        <Id name="foo"/>
    </OptionalChain>
</Use>
<Print>
    <OptionalChain>
        <Id name="foo"/>
    </OptionalChain>
</Print>
```

```brix
(main
  (return-if (if (not foo)))
  (return (not foo))
)
```

```output
<ReturnIfTrue>
    <UnaryExpression operator="!">
        <Id name="foo"/>
    </UnaryExpression>
</ReturnIfTrue>
<Return>
    <UnaryExpression operator="!">
        <Id name="foo"/>
    </UnaryExpression>
</Return>
```

```brix
(main
  (return-if true (if foo))
)
```

```output
<ReturnIfTrue>
    <Id name="true"/>
    <Id name="foo"/>
</ReturnIfTrue>
```

```brix
(main
  (return-if (if foo))
)
```

```output
<ReturnIfTrue>
    <Id name="foo"/>
</ReturnIfTrue>
```

```brix
(main
  (return-if (if foo))
)
```

```output
<ReturnIfTrue>
    <Id name="foo"/>
</ReturnIfTrue>
```

```brix
(main
  (item? foo)
)
```

```output
<Feed optional="true">
    <Id name="foo"/>
</Feed>
```

```brix
(main
  (. (!! (. foo bar)) baz)
)
```

```output
<Get name="baz">
    <UnaryExpression operator="!">
        <Get name="bar">
            <Id name="foo"/>
        </Get>
    </UnaryExpression>
</Get>
```

```brix
(main
  (. (!! (call foo)) bar)
)
```

```output
<Get name="bar">
    <UnaryExpression operator="!">
        <CallExpression>
            <Id name="foo"/>
        </CallExpression>
    </UnaryExpression>
</Get>
```

```brix
(main
  (. (!! ([] foo 0)) bar)
)
```

```output
<Get name="bar">
    <UnaryExpression operator="!">
        <Index>
            <Id name="foo"/>
            <Integer value="0"/>
        </Index>
    </UnaryExpression>
</Get>
```

```brix
(main
  ([] (!! (. foo bar)) 0)
)
```

```output
<Index>
    <UnaryExpression operator="!">
        <Get name="bar">
            <Id name="foo"/>
        </Get>
    </UnaryExpression>
    <Integer value="0"/>
</Index>
```

```brix
(main
  (. (!! (. (!! (call (. foo bar))) baz)) qux)
)
```

```output
<Get name="qux">
    <UnaryExpression operator="!">
        <Get name="baz">
            <UnaryExpression operator="!">
                <CallExpression>
                    <Get name="bar">
                        <Id name="foo"/>
                    </Get>
                </CallExpression>
            </UnaryExpression>
        </Get>
    </UnaryExpression>
</Get>
```

```brix
(main
  (!! (. foo bar))
  (!= (. foo bar) baz)
)
```

```output
<UnaryExpression operator="!">
    <Get name="bar">
        <Id name="foo"/>
    </Get>
</UnaryExpression>
<BinaryExpression operator="!=">
    <Get name="bar">
        <Id name="foo"/>
    </Get>
    <Id name="baz"/>
</BinaryExpression>
```
