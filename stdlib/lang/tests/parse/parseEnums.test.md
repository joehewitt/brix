# Enums

```brix
(type + Foo
  (variant bar)
)
```

```output
<Class name="Foo" access="public-access">
    <Enum name="bar"/>
</Class>
```

```brix
(type + Foo
  (variant foo (= -1))
  (variant bar (= 1))
)
```

```output
<Class name="Foo" access="public-access">
    <Enum name="foo">
        <Integer value="-1"/>
    </Enum>
    <Enum name="bar">
        <Integer value="1"/>
    </Enum>
</Class>
```

```brix
(type + Foo
  (variant bar
    (=
      (call foo
        (:where
          (block
            bar
          )
        )
      )
    )
  )
)
```

```output
<Class name="Foo" access="public-access">
    <Enum name="bar">
        <CallExpression>
            <Id name="foo"/>
            <Argument name="where">
                <Block>
                    <Id name="bar"/>
                </Block>
            </Argument>
        </CallExpression>
    </Enum>
</Class>
```

```brix
(type + Foo
  (variant bar
    (=
      (block
        bar
      )
    )
  )
)
```

```output
<Class name="Foo" access="public-access">
    <Enum name="bar">
        <Block>
            <Id name="bar"/>
        </Block>
    </Enum>
</Class>
```

```brix
(type + Foo
  (variant Type)
  (variant name Type)
  (variant name Type (= 1))
  (variant name (tuple (: bar String)))
  (variant name (= 1))
)
```

```output
<Class name="Foo" access="public-access">
    <Enum>
        <TypeId name="Type"/>
    </Enum>
    <Enum name="name">
        <TypeId name="Type"/>
    </Enum>
    <Enum name="name">
        <TypeId name="Type"/>
        <Integer value="1"/>
    </Enum>
    <Enum name="name">
        <TypeCapture>
            <Id name="bar"/>
            <TypeId name="String"/>
        </TypeCapture>
    </Enum>
    <Enum name="name">
        <Integer value="1"/>
    </Enum>
</Class>
```
