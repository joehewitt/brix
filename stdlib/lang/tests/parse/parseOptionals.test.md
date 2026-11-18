# Optionals

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
foo?.bar?.baz
```

```brix
(main
  (?. foo bar baz)
)
```

```output
<Get name="baz" optional="true">
    <Get name="bar" optional="true">
        <Id name="foo"/>
    </Get>
</Get>
```

```yumi
foo.bar?.baz
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

```yumi
foo[0]?.baz
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

```yumi
foo?.bar[0]
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

```yumi
foo()?.bar
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

```yumi
foo?.bar()?.baz
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

```yumi
foo =? bar
foo +=? bar
=? foo
<=? foo
use? foo
--? foo
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

```yumi
=? if !foo
= !foo
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

```yumi
=? true
    if foo
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

```yumi
=? if foo
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

```yumi
=?
    if foo
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

```yumi
*? foo
```

```brix
(main
  (bullet? foo)
)
```

```output
<Bullet optional="true">
    <Id name="foo"/>
</Bullet>
```

```yumi
foo.bar!.baz
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

```yumi
foo()!.bar
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

```yumi
foo[0]!.bar
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

```yumi
foo.bar![0]
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

```yumi
foo.bar()!.baz!.qux
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

```yumi
foo.bar!
foo.bar != baz
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
