# Markup

```yumi
<Foo
```

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

```yumi crash
━━━━━━━━━━━━━━━━━━━━
+ Panel

────────────────────
+ <''T header
```

```output
Exception: Slot type inference is not supported yet
<no file:6>
    + <''T header
    ^^^^^^^^^^^^^
```

```yumi crash
━━━━━━━━━━━━━━━━━━━━
+ Panel

────────────────────
<header'TextBox
```

```output
Exception: Syntax error
<no file:6>
    <header'TextBox
```

```yumi
<Foo
    a = b
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

```yumi
<Foo
    a <-> b
    c <-> d.e.f
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

```yumi crash
<Foo
    a <-> #b
```

```output
Exception: The place of "<->" is a name or a chain of members, written without "#", as in "value <-> settings.volume"
<no file:3>
        a <-> #b
              ^
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

```yumi
<Foo "text"
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

```yumi
<Foo
    a = b
    <Foo
    <Foo
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

```yumi
<Foo foo
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

```yumi
<Panel
    <Box (body)
    <Label local-label(header)
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

```yumi
<Foo = bar
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

```yumi
<Foo = bar • w:1 h:2
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

```yumi
<Foo foo = bar
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

```yumi
<Foo foo = bar
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

```yumi
foo = <Foo
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

```yumi
* <Foo
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

```yumi
foo • with <Foo
    <Bar
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

```yumi
━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────

<Bar
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

```yumi
━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────

+ foo = bar
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

```yumi
━━━━━━━━━━━━━━━━━━━━
+ Panel

────────────────────
+ <TextBox header • selected
    text = "Fallback"
    <TextBox = "Body"
+ <MarkNode
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

```yumi
━━━━━━━━━━━━━━━━━━━━
+ Panel

────────────────────
+ <Box row - item RowData

━━━━━━━━━━━━━━━━━━━━

<Panel
    <TextBox (row) - item = item.text
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

```yumi
<Panel
    <+ (row)
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

```yumi
<Panel
    <TextBox (header) = "Title"
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

### Markup inside an expression is refused with the forms it can take.

```yumi crash
. label = show(<Text "x")
```

```output
Exception: Markup cannot be written inside an expression. Give it as a row: an argument row, as in "- <Text", a named argument, as in "• title <Text", or an item, as in "* <Text"; or name it first, as in ". label = <Text"
<no file:2>
    . label = show(<Text "x")
                   ^
```

### A header part after one that follows it is out of order.

```yumi crash
<Text "hi" stage
```

```output
Exception: This part of the element's header is out of order. After the type come its name, then (slot), - parameter, = value or "text", • style rules, and for tags, as in "<Text title(heading) "Hi" • bold for dark"
<no file:2>
    <Text "hi" stage
               ^
```

### Style rules come before theme tags.

```yumi crash
<Box for night • page
```

```output
Exception: This part of the element's header is out of order. After the type come its name, then (slot), - parameter, = value or "text", • style rules, and for tags, as in "<Text title(heading) "Hi" • bold for dark"
<no file:2>
    <Box for night • page
                   ^
```
