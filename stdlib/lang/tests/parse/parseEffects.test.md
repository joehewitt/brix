# Effects

### An effect claims the C functions only its doors may call.

```yumi
━━━━━━━━━━━━━━━━━━━━
+ effect network
    - C:socket
    - C:connect
```

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

```yumi
+ effect paid-calls
```

```brix
(effect + paid-calls)
```

```output
<Effect name="paid-calls" access="public-access" claims=""/>
```

### A door names its effect.

```yumi
━━━━━━━━━━━━━━━━━━━━
+ do connect-to I32 @inline @door(network)
    - port I32

= C:connect(port)
```

```brix
(fn-do + connect-to I32 @inline
  (door network)
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

```yumi
━━━━━━━━━━━━━━━━━━━━
+ parse-document Int without network, heap

1
```

```brix
(fn-expr + parse-document Int (without network heap) 1)
```

```output
<Function names="parse-document" access="public-access" effects="no-heap" without="network">
    <TypeId name="Int"/>
    <Block>
        <Integer value="1"/>
    </Block>
</Function>
```

```yumi
do without network, files
    a()
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

### A door is a function.

```yumi crash
do @door(network)
    a()
```

```output
Exception: '@door' is only for a function
<no file:2>
    do @door(network)
    ^^^^^^^^^^^^^^^^
```

### A function is the door of one effect.

```yumi crash
━━━━━━━━━━━━━━━━━━━━
+ do connect-to @door(network) @door(files)

a()
```

```output
Exception: A function is the door of one effect
<no file:3>
    + do connect-to @door(network) @door(files)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```
