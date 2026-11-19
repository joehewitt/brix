# Effects

### An effect claims the C functions only its doors may call.

```brix
(effect + network
  (claim C:socket)
  (claim C:connect)
)
```

```output
<Effect name="network" access="public-access" claims="socket, connect"/>
```

### An effect may claim no C function.

```brix
(effect + paid-calls)
```

```output
<Effect name="paid-calls" access="public-access" claims=""/>
```

### A door names its effect.

```brix
(fn-do + connect-to I32 @inline (door network)
  (param port I32)
  (return (call C:connect port))
)
```

```output
<Function names="connect-to" access="public-access" imperative="true" inline="true" door="network">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="port"/>
            <TypeId name="I32"/>
        </TypeCapture>
    </ArgumentDecl>
    <TypeId name="I32"/>
    <Block>
        <Return>
            <CallExpression>
                <CId name="connect"/>
                <Argument>
                    <Id name="port"/>
                </Argument>
            </CallExpression>
        </Return>
    </Block>
</Function>
```

### A name after without that is no restriction bans the effect of that name.

```brix
(fn-expr + parse-document Int (without network heap)
  1
)
```

```output
<Function names="parse-document" access="public-access" effects="no-heap" without="network">
    <TypeId name="Int"/>
    <Block>
        <Integer value="1"/>
    </Block>
</Function>
```

```brix
(main
  (do (without network files)
    (call a)
  )
)
```

```output
<Do without="network, files">
    <Block>
        <CallExpression>
            <Id name="a"/>
        </CallExpression>
    </Block>
</Do>
```
