# Types

```brix
(main
  ''Bar
)
```

```output
<TypeId name="Bar" declare="true"/>
```

```brix
(main
  Bar
)
```

```output
<TypeId name="Bar"/>
```

## Paths

```brix
(main
  (of Foo ''Bar)
)
```

```output
<TypeArguments>
    <TypeId name="Foo"/>
    <TypeId name="Bar" declare="true"/>
</TypeArguments>
```

```brix
(main
  (of Foo Bar)
)
```

```output
<TypeArguments>
    <TypeId name="Foo"/>
    <TypeId name="Bar"/>
</TypeArguments>
```

```brix
(main
  (of Foo Bar Baz)
)
```

```output
<TypeArguments>
    <TypeId name="Foo"/>
    <TypeId name="Bar"/>
    <TypeId name="Baz"/>
</TypeArguments>
```

```brix
(main
  (of Foo Bar)
)
```

```output
<TypeArguments>
    <TypeId name="Foo"/>
    <TypeId name="Bar"/>
</TypeArguments>
```

```brix
(main
  (of Foo Bar)
)
```

```output
<TypeArguments>
    <TypeId name="Foo"/>
    <TypeId name="Bar"/>
</TypeArguments>
```

```brix
(main
  (of Foo Bar Baz)
)
```

```output
<TypeArguments>
    <TypeId name="Foo"/>
    <TypeId name="Bar"/>
    <TypeId name="Baz"/>
</TypeArguments>
```

```brix
(main
  (of Foo (of Bar Baz))
)
```

```output
<TypeArguments>
    <TypeId name="Foo"/>
    <TypeArguments>
        <TypeId name="Bar"/>
        <TypeId name="Baz"/>
    </TypeArguments>
</TypeArguments>
```

```brix
(main
  (. Storage (of Backing Item))
)
```

```output
<AssociatedType name="Backing">
    <TypeId name="Storage"/>
    <TypeId name="Item"/>
</AssociatedType>
```

## Optional types

```brix
(main
  (? Foo)
)
```

```output
<TypeArguments>
    <TypeId name="Optional"/>
    <TypeId name="Foo"/>
</TypeArguments>
```

## Compound type shorthands

```brix
(type-line + Foo (is (list Foo)))
```

```output
<Class name="Foo" access="public-access">
    <Implements inherited="true">
        <TypeArguments>
            <TypeId name="List"/>
            <TypeId name="Foo"/>
        </TypeArguments>
    </Implements>
</Class>
```

```brix
(type-line + Foo (is (map Foo Bar)))
```

```output
<Class name="Foo" access="public-access">
    <Implements inherited="true">
        <TypeArguments>
            <TypeId name="Map"/>
            <TypeId name="Foo"/>
            <TypeId name="Bar"/>
        </TypeArguments>
    </Implements>
</Class>
```

```brix
(type-line + Foo (is (stream Foo)))
```

```output
<Class name="Foo" access="public-access">
    <Implements inherited="true">
        <TypeArguments>
            <TypeId name="Stream"/>
            <TypeId name="Foo"/>
        </TypeArguments>
    </Implements>
</Class>
```

## Function types

```brix
(type + Foo (is (fn-type Baz (param foo Foo))))
```

```output
<Class name="Foo" access="public-access">
    <Implements inherited="true">
        <TypeFunction>
            <TypeId name="Baz"/>
            <ArgumentDecl>
                <TypeCapture>
                    <Id name="foo"/>
                    <TypeId name="Foo"/>
                </TypeCapture>
            </ArgumentDecl>
        </TypeFunction>
    </Implements>
</Class>
```

```brix
(type + Foo (is (fn-type Baz (param foo Foo))))
```

```output
<Class name="Foo" access="public-access">
    <Implements inherited="true">
        <TypeFunction>
            <TypeId name="Baz"/>
            <ArgumentDecl>
                <TypeCapture>
                    <Id name="foo"/>
                    <TypeId name="Foo"/>
                </TypeCapture>
            </ArgumentDecl>
        </TypeFunction>
    </Implements>
</Class>
```

```brix
(fn-expr + fn _
  (param arg (fn-type Baz (param foo Foo)))
)
```

```output
<Function names="fn" access="public-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="arg"/>
            <TypeFunction>
                <TypeId name="Baz"/>
                <ArgumentDecl>
                    <TypeCapture>
                        <Id name="foo"/>
                        <TypeId name="Foo"/>
                    </TypeCapture>
                </ArgumentDecl>
            </TypeFunction>
        </TypeCapture>
    </ArgumentDecl>
</Function>
```

```brix
(type + (of Statement Item) (is (fn-type Void (param a Int))))
```

```output
<Class name="Statement" access="public-access">
    <TypeId name="Item"/>
    <Implements inherited="true">
        <TypeFunction>
            <TypeId name="Void"/>
            <ArgumentDecl>
                <TypeCapture>
                    <Id name="a"/>
                    <TypeId name="Int"/>
                </TypeCapture>
            </ArgumentDecl>
        </TypeFunction>
    </Implements>
</Class>
```

```brix
(main
  (is x Foo)
  (is! x Foo)
  (is~ x Foo)
)
```

```output
<BinaryExpression operator="is">
    <Id name="x"/>
    <TypeId name="Foo"/>
</BinaryExpression>
<BinaryExpression operator="is!">
    <Id name="x"/>
    <TypeId name="Foo"/>
</BinaryExpression>
<BinaryExpression operator="is~">
    <Id name="x"/>
    <TypeId name="Foo"/>
</BinaryExpression>
```
