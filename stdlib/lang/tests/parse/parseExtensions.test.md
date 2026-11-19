# Extensions

```brix
(extend + Thing
  (fn-expr + describe String
    "thing"
  )
)
```

```output
<Extension access="public-access">
    <TypeId name="Thing"/>
    <Function names="describe" access="public-access">
        <TypeId name="String"/>
        <Block>
            <String text="thing"/>
        </Block>
    </Function>
</Extension>
```

### Parses a style sheet extension with slots, usings, and rules.

```brix
(style-extend DefaultTheme
  (using BoxPaint)
  (prop + accent I64 (= 11))
  (blank)
  (rule panel
    (= ink accent)
  )
)
```

```output
<StyleSheetExtension>
    <TypeId name="DefaultTheme"/>
    <Using>
        <TypeId name="BoxPaint"/>
    </Using>
    <Property access="public-access">
        <TypeCapture>
            <Id name="accent"/>
            <TypeId name="I64"/>
        </TypeCapture>
        <Integer value="11"/>
    </Property>
    <StyleRule name="panel">
        <Block>
            <AssignmentExpression op="=">
                <Id name="ink"/>
                <Id name="accent"/>
            </AssignmentExpression>
        </Block>
    </StyleRule>
</StyleSheetExtension>
```

```brix
(extend + (of Box ''T)
  (where
    (constraint is T Printable)
  )
  (fn-expr + describe String
    "box"
  )
)
```

```output
<Extension access="public-access">
    <TypeArguments>
        <TypeId name="Box"/>
        <TypeId name="T" declare="true"/>
    </TypeArguments>
    <TypeConstraint op="convertible">
        <TypeId name="T"/>
        <TypeId name="Printable"/>
    </TypeConstraint>
    <Function names="describe" access="public-access">
        <TypeId name="String"/>
        <Block>
            <String text="box"/>
        </Block>
    </Function>
</Extension>
```

```brix
(extend + (of Box ''T) (as ImmutableArray)
  (where
    (constraint is T Printable)
  )
  (fn-expr + label String :get
    "label"
  )
)
```

```output
<Extension access="public-access">
    <TypeArguments>
        <TypeId name="Box"/>
        <TypeId name="T" declare="true"/>
    </TypeArguments>
    <As>
        <TypeId name="ImmutableArray"/>
    </As>
    <TypeConstraint op="convertible">
        <TypeId name="T"/>
        <TypeId name="Printable"/>
    </TypeConstraint>
    <Function names="label" access="public-access" getter="true">
        <TypeId name="String"/>
        <Block>
            <String text="label"/>
        </Block>
    </Function>
</Extension>
```

```brix
(extend + Thing
  (fn-expr + label String :get
    "label"
  )
  (fn-to-expr + String
    "converted"
  )
)
```

```output
<Extension access="public-access">
    <TypeId name="Thing"/>
    <Function names="label" access="public-access" getter="true">
        <TypeId name="String"/>
        <Block>
            <String text="label"/>
        </Block>
    </Function>
    <Converter access="public-access">
        <TypeId name="String"/>
        <Block>
            <String text="converted"/>
        </Block>
    </Converter>
</Extension>
```
