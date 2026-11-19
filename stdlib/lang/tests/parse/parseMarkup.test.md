# Markup

```brix
(main
  (<> Foo)
)
```

```output
<Markup>
    <TypeId name="Foo"/>
</Markup>
```

```brix
(main
  (<> Foo
    (= a b)
  )
)
```

```output
<Markup>
    <TypeId name="Foo"/>
    <Block>
        <AssignmentExpression op="=">
            <Id name="a"/>
            <Id name="b"/>
        </AssignmentExpression>
    </Block>
</Markup>
```

```brix
(main
  (<> Foo
    (<-> a b)
    (<-> c (. d e f))
  )
)
```

```output
<Markup>
    <TypeId name="Foo"/>
    <Block>
        <AssignmentExpression op="<->">
            <Id name="a"/>
            <Id name="b" tracked="true"/>
        </AssignmentExpression>
        <AssignmentExpression op="<->">
            <Id name="c"/>
            <Get name="f" tracked="true">
                <Get name="e">
                    <Id name="d"/>
                </Get>
            </Get>
        </AssignmentExpression>
    </Block>
</Markup>
```

```brix
(main
  (<> Foo
    (<-> a (. b #c))
  )
)
```

```output
Exception: The place of “<->” is a name or a chain of members, written without “#”, as in “(<-> value (. settings volume))”
<no file:4>
        (<-> a (. b #c))
               ^^^^^^^
```

```brix
(main
  (<> Foo (text: "text"))
)
```

```output
<Markup>
    <TypeId name="Foo"/>
    <args>
        <String text="text"/>
    </args>
</Markup>
```

```brix
(main
  (<> Foo
    (= a b)
    (<> Foo)
    (<> Foo)
  )
)
```

```output
<Markup>
    <TypeId name="Foo"/>
    <Block>
        <AssignmentExpression op="=">
            <Id name="a"/>
            <Id name="b"/>
        </AssignmentExpression>
        <Markup>
            <TypeId name="Foo"/>
        </Markup>
        <Markup>
            <TypeId name="Foo"/>
        </Markup>
    </Block>
</Markup>
```

```brix
(main
  (<> Foo (id: foo))
)
```

```output
<Markup>
    <TypeId name="Foo"/>
    <Id name="foo"/>
</Markup>
```

```brix
(main
  (<> Panel
    (<> Box (slot: body))
    (<> Label (id: local-label) (slot: header))
  )
)
```

```output
<Markup>
    <TypeId name="Panel"/>
    <Block>
        <Markup>
            <TypeId name="Box"/>
            <SlotBinding>
                <Id name="body"/>
            </SlotBinding>
        </Markup>
        <Markup>
            <TypeId name="Label"/>
            <Id name="local-label"/>
            <SlotBinding>
                <Id name="header"/>
            </SlotBinding>
        </Markup>
    </Block>
</Markup>
```

```brix
(main
  (<> Foo (value: bar))
)
```

```output
<Markup>
    <TypeId name="Foo"/>
    <args>
        <Id name="bar"/>
    </args>
</Markup>
```

```brix
(main
  (<> Foo (value: bar) (• w 1) (• h 2))
)
```

```output
<Markup>
    <TypeId name="Foo"/>
    <args>
        <Id name="bar"/>
    </args>
    <StyleRuleApplications>
        <StyleRuleApplication name="w">
            <args>
                <Integer value="1"/>
            </args>
        </StyleRuleApplication>
        <StyleRuleApplication name="h">
            <args>
                <Integer value="2"/>
            </args>
        </StyleRuleApplication>
    </StyleRuleApplications>
</Markup>
```

```brix
(main
  (<> Foo (id: foo) (value: bar))
)
```

```output
<Markup>
    <TypeId name="Foo"/>
    <Id name="foo"/>
    <args>
        <Id name="bar"/>
    </args>
</Markup>
```

```brix
(main
  (<> Foo (id: foo) (value: bar))
)
```

```output
<Markup>
    <TypeId name="Foo"/>
    <Id name="foo"/>
    <args>
        <Id name="bar"/>
    </args>
</Markup>
```

```brix
(main
  (= foo (<> Foo))
)
```

```output
<AssignmentExpression op="=">
    <Id name="foo"/>
    <Markup>
        <TypeId name="Foo"/>
    </Markup>
</AssignmentExpression>
```

```brix
(main
  (item (<> Foo))
)
```

```output
<Feed>
    <Markup>
        <TypeId name="Foo"/>
    </Markup>
</Feed>
```

```brix
(main
  (call foo
    (:with
      (<> Foo
        (<> Bar)
      )
    )
  )
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="with">
        <Markup>
            <TypeId name="Foo"/>
            <Block>
                <Markup>
                    <TypeId name="Bar"/>
                </Markup>
            </Block>
        </Markup>
    </Argument>
</CallExpression>
```

```brix
(type + Foo
  (content
    (<> Bar)
  )
)
```

```output
<Class name="Foo" access="public-access">
    <Content>
        <Block>
            <Markup>
                <TypeId name="Bar"/>
            </Markup>
        </Block>
    </Content>
</Class>
```

```brix
(type + Foo
  (content
    (prop + foo _ (= bar))
  )
)
```

```output
<Class name="Foo" access="public-access">
    <Content>
        <Block>
            <Property access="public-access">
                <TypeCapture>
                    <Id name="foo"/>
                </TypeCapture>
                <Id name="bar"/>
            </Property>
        </Block>
    </Content>
</Class>
```

```brix
(type + Panel
  (content
    (+<> TextBox (id: header) (• selected)
      (= text "Fallback")
      (<> TextBox (value: "Body"))
    )
    (+<> MarkNode)
  )
)
```

```output
<Class name="Panel" access="public-access">
    <Content>
        <Block>
            <SlotOutlet>
                <Id name="header"/>
                <TypeId name="TextBox"/>
                <StyleRuleApplications>
                    <StyleRuleApplication name="selected"/>
                </StyleRuleApplications>
                <Block>
                    <AssignmentExpression op="=">
                        <Id name="text"/>
                        <String text="Fallback"/>
                    </AssignmentExpression>
                    <Markup>
                        <TypeId name="TextBox"/>
                        <args>
                            <String text="Body"/>
                        </args>
                    </Markup>
                </Block>
            </SlotOutlet>
            <SlotOutlet>
                <TypeId name="MarkNode"/>
            </SlotOutlet>
        </Block>
    </Content>
</Class>
```

```brix
(type + Panel
  (content
    (+<> Box (id: row) (param: item RowData))
  )
)

(main
  (<> Panel
    (<> TextBox (slot: row) (param: item) (value: (. item text)))
  )
)
```

```output
<Class name="Panel" access="public-access">
    <Content>
        <Block>
            <SlotOutlet>
                <Id name="row"/>
                <TypeId name="Box"/>
                <SlotParameter>
                    <Id name="item"/>
                    <TypeId name="RowData"/>
                </SlotParameter>
            </SlotOutlet>
        </Block>
    </Content>
</Class>
<Markup>
    <TypeId name="Panel"/>
    <Block>
        <Markup>
            <TypeId name="TextBox"/>
            <SlotBinding>
                <Id name="row"/>
                <SlotParameter>
                    <Id name="item"/>
                </SlotParameter>
            </SlotBinding>
            <args>
                <Get name="text">
                    <Id name="item"/>
                </Get>
            </args>
        </Markup>
    </Block>
</Markup>
```

```brix
(main
  (<> Panel
    (<+ row)
  )
)
```

```output
<Markup>
    <TypeId name="Panel"/>
    <Block>
        <MarkupSlotForward>
            <Id name="row"/>
            <SlotOutlet>
                <Id name="row"/>
            </SlotOutlet>
        </MarkupSlotForward>
    </Block>
</Markup>
```

```brix
(main
  (<> Panel
    (<> TextBox (slot: header) (value: "Title"))
  )
)
```

```output
<Markup>
    <TypeId name="Panel"/>
    <Block>
        <Markup>
            <TypeId name="TextBox"/>
            <SlotBinding>
                <Id name="header"/>
            </SlotBinding>
            <args>
                <String text="Title"/>
            </args>
        </Markup>
    </Block>
</Markup>
```
