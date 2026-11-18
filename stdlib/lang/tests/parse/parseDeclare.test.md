# Declare

```yumi
. foo
```

```brix
(main
  (let foo)
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="foo"/>
    </TypeCapture>
</Declaration>
```

```yumi
. foo = bar
```

```brix
(main
  (let foo bar)
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="foo"/>
    </TypeCapture>
    <Id name="bar"/>
</Declaration>
```

```yumi
. foo
    bar
```

```brix
(main
  (let foo
    (block
      bar
    )
  )
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="foo"/>
    </TypeCapture>
    <Block>
        <Id name="bar"/>
    </Block>
</Declaration>
```

```yumi
.. foo = bar
```

```brix
(main
  (lazy foo bar)
)
```

```output
<Declaration lazy="true">
    <TypeCapture>
        <Id name="foo"/>
    </TypeCapture>
    <Id name="bar"/>
</Declaration>
```

```yumi
. foo Foo = bar
```

```brix
(main
  (let foo Foo bar)
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="foo"/>
        <TypeId name="Foo"/>
    </TypeCapture>
    <Id name="bar"/>
</Declaration>
```

```yumi
. foo [Foo] = bar
```

```brix
(main
  (let foo (list Foo) bar)
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="foo"/>
        <TypeList>
            <TypeId name="Foo"/>
        </TypeList>
    </TypeCapture>
    <Id name="bar"/>
</Declaration>
```

```yumi
.. name ~
    ~ Foo
```

```brix
(main
  (lazy name
    (pattern
      (~ Foo)
    )
  )
)
```

```output
<Declaration lazy="true" pattern="true">
    <TypeCapture>
        <Id name="name"/>
    </TypeCapture>
    <StreamMatch>
        <StreamType>
            <TypeId name="Foo"/>
        </StreamType>
    </StreamMatch>
</Declaration>
```

```yumi
. x =
    -- foo
    foo
```

```output
<Declaration>
    <TypeCapture>
        <Id name="x"/>
    </TypeCapture>
    <Block>
        <Print>
            <Id name="foo"/>
        </Print>
        <Id name="foo"/>
    </Block>
</Declaration>
```
