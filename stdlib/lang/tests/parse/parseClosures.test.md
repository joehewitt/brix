# Closures

```brix
(main
  (=> () :do (call f 1))
)
```

```output
<Function names="(closure)" access="private-access" imperative="true">
    <CallExpression>
        <Id name="f"/>
        <Argument>
            <Integer value="1"/>
        </Argument>
    </CallExpression>
</Function>
```

```brix
(main
  (=> (item) :do (call f item))
)
```

```output
<Function names="(closure)" access="private-access" imperative="true">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="item"/>
        </TypeCapture>
    </ArgumentDecl>
    <CallExpression>
        <Id name="f"/>
        <Argument>
            <Id name="item"/>
        </Argument>
    </CallExpression>
</Function>
```

```brix
(main
  (=> () result)
)
```

```output
<Function names="(closure)" access="private-access">
    <Id name="result"/>
</Function>
```

```brix
(main
  (=> (binding) result)
)
```

```output
<Function names="(closure)" access="private-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="binding"/>
        </TypeCapture>
    </ArgumentDecl>
    <Id name="result"/>
</Function>
```

```brix
(main
  (=> () result)
)
```

```output
<Function names="(closure)" access="private-access">
    <Id name="result"/>
</Function>
```

```brix
(main
  (=> (a) result)
)
```

```output
<Function names="(closure)" access="private-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
    </ArgumentDecl>
    <Id name="result"/>
</Function>
```

```brix
(main
  (=> (a b) result)
)
```

```output
<Function names="(closure)" access="private-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
    </ArgumentDecl>
    <ArgumentDecl>
        <TypeCapture>
            <Id name="b"/>
        </TypeCapture>
    </ArgumentDecl>
    <Id name="result"/>
</Function>
```

```brix
(main
  (=> (a)
    (block
      b
    )
  )
)
```

```output
<Function names="(closure)" access="private-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
    </ArgumentDecl>
    <Block>
        <Id name="b"/>
    </Block>
</Function>
```

```brix
(main
  (=> (a) b)
)
```

```output
<Function names="(closure)" access="private-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
    </ArgumentDecl>
    <Id name="b"/>
</Function>
```

```brix
(main
  (=> (a b)
    (block
      c
    )
  )
)
```

```output
<Function names="(closure)" access="private-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
    </ArgumentDecl>
    <ArgumentDecl>
        <TypeCapture>
            <Id name="b"/>
        </TypeCapture>
    </ArgumentDecl>
    <Block>
        <Id name="c"/>
    </Block>
</Function>
```

```brix
(main
  (=> ((: a Int) (: b Float))
    (block
      c
    )
  )
)
```

```output
<Function names="(closure)" access="private-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="a"/>
            <TypeId name="Int"/>
        </TypeCapture>
    </ArgumentDecl>
    <ArgumentDecl>
        <TypeCapture>
            <Id name="b"/>
            <TypeId name="Float"/>
        </TypeCapture>
    </ArgumentDecl>
    <Block>
        <Id name="c"/>
    </Block>
</Function>
```

```brix
(main
  (=> () a)
)
```

```output
<Function names="(closure)" access="private-access">
    <Id name="a"/>
</Function>
```

```brix
(main
  (call foo (:where (* each 2)))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="where">
        <Function names="(closure)" access="private-access">
            <ArgumentDecl>
                <TypeCapture>
                    <Id name="each"/>
                </TypeCapture>
            </ArgumentDecl>
            <BinaryExpression operator="*">
                <Id name="each"/>
                <Integer value="2"/>
            </BinaryExpression>
        </Function>
    </Argument>
</CallExpression>
```

```brix
(main
  (call foo (:of (*> xs (* each 2))))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="of">
        <BinaryExpression operator="*>">
            <Id name="xs"/>
            <BinaryExpression operator="*">
                <Id name="each"/>
                <Integer value="2"/>
            </BinaryExpression>
        </BinaryExpression>
    </Argument>
</CallExpression>
```

```brix
(main
  (call foo (:where (=> () a)))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="where">
        <Function names="(closure)" access="private-access">
            <Id name="a"/>
        </Function>
    </Argument>
</CallExpression>
```

```brix
(main
  (call foo (:where (=> (a) a)))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="where">
        <Function names="(closure)" access="private-access">
            <ArgumentDecl>
                <TypeCapture>
                    <Id name="a"/>
                </TypeCapture>
            </ArgumentDecl>
            <Id name="a"/>
        </Function>
    </Argument>
</CallExpression>
```

```brix
(main
  (=> ((: ...foo (list Int))) foo)
)
```

```output
<Function names="(closure)" access="private-access">
    <ArgumentDecl variadic="true">
        <TypeCapture>
            <Id name="foo"/>
            <TypeArguments>
                <TypeId name="List"/>
                <TypeId name="Int"/>
            </TypeArguments>
        </TypeCapture>
    </ArgumentDecl>
    <Id name="foo"/>
</Function>
```
