# Declare

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
