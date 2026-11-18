# Types

```yumi
''Bar
```

```brix
(main
  ''Bar
)
```

```output
<TypeId name="Bar" declare="true"/>
```

```yumi
Bar
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

```yumi
Foo''Bar
```

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

```yumi
Foo'Bar
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

```yumi
Foo'Bar'Baz
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

```yumi
Foo'Bar
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

```yumi
Foo'Bar
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

```yumi
Foo'Bar'Baz
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

```yumi
Foo'(Bar'Baz)
```

```brix
(main
  (of Foo (tuple (of Bar Baz)))
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

```yumi
Storage.Backing'Item
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

```yumi
Foo?
```

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

```yumi
+ Foo is [Foo]
```

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

```yumi
+ Foo is [Foo = Bar]
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

```yumi
+ Foo is <Foo>
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

```yumi
+ Foo is () Baz
    - foo Foo
```

```brix
(type-line + Foo (is (fn-type Baz (param foo Foo))))
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

```yumi
━━━━━━━━━━━━━━━━━━━━
+ Foo is () Baz
    - foo Foo
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

```yumi
━━━━━━━━━━━━━━━━━━━━
+ fn
    - arg () Baz
        - foo Foo
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

```yumi
━━━━━━━━━━━━━━━━━━━━
+ Statement'Item is () Void
    - a Int
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

```yumi
x is Foo
x is! Foo
x is~ Foo
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
