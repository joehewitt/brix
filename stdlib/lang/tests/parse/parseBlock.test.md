# Block

```brix
(main
  a
  b
  c
)
```

```output
<Id name="a"/>
<Id name="b"/>
<Id name="c"/>
```

```brix
(main
  (do
    (call a)
    (blank)
    (call b)
  )
)
```

```output
<Do>
    <Block>
        <CallExpression>
            <Id name="a"/>
        </CallExpression>
        <CallExpression>
            <Id name="b"/>
        </CallExpression>
    </Block>
</Do>
```

```brix
(main
  (do (without safepoints)
    (call a)
  )
)
```

```output
<Do effects="no-safepoint">
    <Block>
        <CallExpression>
            <Id name="a"/>
        </CallExpression>
    </Block>
</Do>
```

```brix
(main
  (do (without safepoints)
    (call a)
  )
)
```

```output
<Do effects="no-safepoint">
    <Block>
        <CallExpression>
            <Id name="a"/>
        </CallExpression>
    </Block>
</Do>
```

```brix
(main
  (do
    ; Comment
    (call a)
  )
)
```

```output
<Do>
    <Block>
        <CallExpression>
            <Id name="a"/>
        </CallExpression>
    </Block>
</Do>
```

```brix
(main
  (do
    (call foo)
    (call bar)
  )
  (call baz)
)
```

```output
<Do>
    <Block>
        <CallExpression>
            <Id name="foo"/>
        </CallExpression>
        <CallExpression>
            <Id name="bar"/>
        </CallExpression>
    </Block>
</Do>
<CallExpression>
    <Id name="baz"/>
</CallExpression>
```

```brix
(main
  (case? y
    x
  )
)
```

```output
<StructureMatch>
    <TypeTransform>
        <TypePredicate>
            <Id name="y"/>
        </TypePredicate>
        <Block>
            <Id name="x"/>
        </Block>
    </TypeTransform>
</StructureMatch>
```

```brix
(main
  (do
    (do-use (call Thing)
      (print "using")
    )
  )
)
```

```output
<Do>
    <Block>
        <UseBlock>
            <CallExpression>
                <TypeId name="Thing"/>
            </CallExpression>
            <Block>
                <Print>
                    <String text="using"/>
                </Print>
            </Block>
        </UseBlock>
    </Block>
</Do>
```

```brix
(main
  (do
    (let a (use (call Thing)))
  )
)
```

```output
<Do>
    <Block>
        <Declaration>
            <TypeCapture>
                <Id name="a"/>
            </TypeCapture>
            <Use>
                <CallExpression>
                    <TypeId name="Thing"/>
                </CallExpression>
            </Use>
        </Declaration>
    </Block>
</Do>
```

```brix
(prop + request Int @stack)
(blank)

(main
  (do-with request 42
    (print request)
  )
)
```

```output
<Property access="public-access" decoration="stack">
    <TypeCapture>
        <Id name="request"/>
        <TypeId name="Int"/>
    </TypeCapture>
</Property>
<StackLocalBinding name="request">
    <Integer value="42"/>
    <Block>
        <Print>
            <Id name="request"/>
        </Print>
    </Block>
</StackLocalBinding>
```

```brix
(main
  (let term
    (use
      (call Thing
        (block
          bar
        )
      )
    )
  )
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="term"/>
    </TypeCapture>
    <Use>
        <CallExpression>
            <TypeId name="Thing"/>
            <Argument>
                <Block>
                    <Id name="bar"/>
                </Block>
            </Argument>
        </CallExpression>
    </Use>
</Declaration>
```

```brix
(fn-expr + text _
  (param arg _)
  (paragraph
    "foo"
    (blank)
    "bar"
    (blank)
    "baz"
  )
)

(fn-expr + other _)
```

```output
<Function names="text" access="public-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="arg"/>
        </TypeCapture>
    </ArgumentDecl>
    <Block>
        <String text="foo

bar

baz"/>
    </Block>
</Function>
<Function names="other" access="public-access"/>
```

```brix
(fn-expr + text _
  (paragraph
    "s01(alpha,123) s02[left:right] s03{one|two|three} s04<word-456>"
  )
)
```

```output
<Function names="text" access="public-access">
    <Block>
        <String text="s01(alpha,123) s02[left:right] s03{one|two|three} s04<word-456>"/>
    </Block>
</Function>
```

```brix
(fn-expr + text _
  (paragraph
    "a ≥ b ≤ c ≥ d"
    "ends ≥"
  )
)

(fn-expr + other _)
```

```output
<Function names="text" access="public-access">
    <Block>
        <String text="a ≥ b ≤ c ≥ d
ends ≥"/>
    </Block>
</Function>
<Function names="other" access="public-access"/>
```

```brix
(fn-expr + text _
  (param arg _)
  (paragraph
    (str arg)
  )
)

(fn-expr + other _)
```

```output
<Function names="text" access="public-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="arg"/>
        </TypeCapture>
    </ArgumentDecl>
    <Block>
        <BinaryExpression operator="++">
            <String text=""/>
            <Id name="arg"/>
        </BinaryExpression>
    </Block>
</Function>
<Function names="other" access="public-access"/>
```

```brix
(fn-expr + text _
  (param arg _)
  (paragraph
    (str arg " ≥ " (str "≥"))
  )
)
```

```output
<Function names="text" access="public-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="arg"/>
        </TypeCapture>
    </ArgumentDecl>
    <Block>
        <BinaryExpression operator="++">
            <BinaryExpression operator="++">
                <Id name="arg"/>
                <String text=" ≥ "/>
            </BinaryExpression>
            <String text="≥"/>
        </BinaryExpression>
    </Block>
</Function>
```

```brix
(fn-expr + text _
  (paragraph
    "first"
    "`kept`"
    "```"
  )
)
```

````output
<Function names="text" access="public-access">
    <Block>
        <String text="first
`kept`
```"/>
    </Block>
</Function>
````

```brix
(fn-expr + text _
  (param arg _)
  (paragraph
    (str "Foo " bar " baz")
  )
)
```

```output
<Function names="text" access="public-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="arg"/>
        </TypeCapture>
    </ArgumentDecl>
    <Block>
        <BinaryExpression operator="++">
            <BinaryExpression operator="++">
                <String text="Foo "/>
                <Id name="bar"/>
            </BinaryExpression>
            <String text=" baz"/>
        </BinaryExpression>
    </Block>
</Function>
```

### An embed after text may begin with a number or a string.

```brix
(fn-expr + text _
  (param arg _)
  (paragraph
    (str "Foo " 1 " bar " (. "x" length) " baz")
  )
)
```

```output
<Function names="text" access="public-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="arg"/>
        </TypeCapture>
    </ArgumentDecl>
    <Block>
        <BinaryExpression operator="++">
            <BinaryExpression operator="++">
                <BinaryExpression operator="++">
                    <BinaryExpression operator="++">
                        <String text="Foo "/>
                        <Integer value="1"/>
                    </BinaryExpression>
                    <String text=" bar "/>
                </BinaryExpression>
                <Get name="length">
                    <String text="x"/>
                </Get>
            </BinaryExpression>
            <String text=" baz"/>
        </BinaryExpression>
    </Block>
</Function>
```

```brix
(fn-expr + text String
  (param content String)
  (paragraph
    (str prefix " " content)
  )
  (where
    (lazy prefix "hi")
  )
)

(fn-expr + other String
  "ok"
)
```

```output
<Function names="text" access="public-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="content"/>
            <TypeId name="String"/>
        </TypeCapture>
    </ArgumentDecl>
    <TypeId name="String"/>
    <Block>
        <BinaryExpression operator="++">
            <BinaryExpression operator="++">
                <Id name="prefix"/>
                <String text=" "/>
            </BinaryExpression>
            <Id name="content"/>
        </BinaryExpression>
        <Declaration lazy="true">
            <TypeCapture>
                <Id name="prefix"/>
            </TypeCapture>
            <String text="hi"/>
        </Declaration>
    </Block>
</Function>
<Function names="other" access="public-access">
    <TypeId name="String"/>
    <Block>
        <String text="ok"/>
    </Block>
</Function>
```

```brix
(fn-expr _ tree-box-row-text String
  (param content String)
  (param suffix String)
  (paragraph
    (str prefix " " content " " suffix)
  )
  (where
    (lazy prefix "hi")
  )
)
```

```output
<Function names="tree-box-row-text" access="private-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="content"/>
            <TypeId name="String"/>
        </TypeCapture>
    </ArgumentDecl>
    <ArgumentDecl>
        <TypeCapture>
            <Id name="suffix"/>
            <TypeId name="String"/>
        </TypeCapture>
    </ArgumentDecl>
    <TypeId name="String"/>
    <Block>
        <BinaryExpression operator="++">
            <BinaryExpression operator="++">
                <BinaryExpression operator="++">
                    <BinaryExpression operator="++">
                        <Id name="prefix"/>
                        <String text=" "/>
                    </BinaryExpression>
                    <Id name="content"/>
                </BinaryExpression>
                <String text=" "/>
            </BinaryExpression>
            <Id name="suffix"/>
        </BinaryExpression>
        <Declaration lazy="true">
            <TypeCapture>
                <Id name="prefix"/>
            </TypeCapture>
            <String text="hi"/>
        </Declaration>
    </Block>
</Function>
```

```brix
(fn-expr + text String
  (paragraph
    (str "Hello " name)
  )
)
```

```output
<Function names="text" access="public-access">
    <TypeId name="String"/>
    <Block>
        <BinaryExpression operator="++">
            <String text="Hello "/>
            <Id name="name"/>
        </BinaryExpression>
    </Block>
</Function>
```

### Make sure indented text doesn't trigger the code indentation parser

```brix
(fn-expr + text _
  (param arg _)
  (paragraph
    "foo"
    "    bar"
    "        baz"
  )
)
```

```output
<Function names="text" access="public-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="arg"/>
        </TypeCapture>
    </ArgumentDecl>
    <Block>
        <String text="foo
    bar
        baz"/>
    </Block>
</Function>
```

### Parses a blank line between argument lines.

```brix
(fn-do + work _
  (param a Int)
  (param b Int (= 1))
  (blank)
  (let r (+ a b))
)
```

```output
<Function names="work" access="public-access" imperative="true">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="a"/>
            <TypeId name="Int"/>
        </TypeCapture>
    </ArgumentDecl>
    <ArgumentDecl>
        <TypeCapture>
            <Id name="b"/>
            <TypeId name="Int"/>
        </TypeCapture>
        <Integer value="1"/>
    </ArgumentDecl>
    <Block>
        <Declaration>
            <TypeCapture>
                <Id name="r"/>
            </TypeCapture>
            <BinaryExpression operator="+">
                <Id name="a"/>
                <Id name="b"/>
            </BinaryExpression>
        </Declaration>
    </Block>
</Function>
```

```brix
(fn-expr _ summary _
  (param reason Int)
  (match reason
    (case= 1
      "one"
    )
    (default
      "other"
    )
  )
)
```

```output
<Function names="summary" access="private-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="reason"/>
            <TypeId name="Int"/>
        </TypeCapture>
    </ArgumentDecl>
    <Block>
        <Match>
            <Id name="reason"/>
            <StructureMatch>
                <TypeOr>
                    <TypeTransform>
                        <TypeEquality>
                            <Integer value="1"/>
                        </TypeEquality>
                        <Block>
                            <String text="one"/>
                        </Block>
                    </TypeTransform>
                    <TypeTransform>
                        <Block>
                            <String text="other"/>
                        </Block>
                    </TypeTransform>
                </TypeOr>
            </StructureMatch>
        </Match>
    </Block>
</Function>
```

```brix
(main
  (do
    (inline
      (for (in x xs)
        (call x)
      )
    )
  )
)
```

```output
<Do>
    <For>
        <In>
            <TypeCapture>
                <Id name="x"/>
            </TypeCapture>
            <Id name="xs"/>
        </In>
        <Block>
            <CallExpression>
                <Id name="x"/>
            </CallExpression>
        </Block>
    </For>
</Do>
```

### A do block's with section binds stack values in turn, the first outermost; its without section keeps the block from what it names.

```brix
(prop + request Int @stack)
(prop + depth Int @stack)
(blank)

(main
  (do (with request 42) (with depth 2)
    (print request)
  )
  (do @feature-collection-observers (without gc) (with depth 3)
    (print depth)
  )
  (do (without cancel)
    (print depth)
  )
)
```

```output
<Property access="public-access" decoration="stack">
    <TypeCapture>
        <Id name="request"/>
        <TypeId name="Int"/>
    </TypeCapture>
</Property>
<Property access="public-access" decoration="stack">
    <TypeCapture>
        <Id name="depth"/>
        <TypeId name="Int"/>
    </TypeCapture>
</Property>
<StackLocalBinding name="request">
    <Integer value="42"/>
    <Block>
        <StackLocalBinding name="depth">
            <Integer value="2"/>
            <Block>
                <Print>
                    <Id name="request"/>
                </Print>
            </Block>
        </StackLocalBinding>
    </Block>
</StackLocalBinding>
<StackLocalBinding name="depth">
    <Integer value="3"/>
    <Block>
        <Do effects="no-gc" features="feature-collection-observers">
            <Block>
                <Print>
                    <Id name="depth"/>
                </Print>
            </Block>
        </Do>
    </Block>
</StackLocalBinding>
<Do cancel="held">
    <Block>
        <Print>
            <Id name="depth"/>
        </Print>
    </Block>
</Do>
```

### A text block keeps blank rows between lines and the spaces past the block's margin, and lines that look like comments or dividers are text.

```brix
(main
  (let greeting
    (paragraph
      (str "Hello " name ",")
      (blank)
      "  indented ` text"
      "━━━━━━━━━━━━━━━━━━━━"
    )
  )
  (blank)
  (let after 1)
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="greeting"/>
    </TypeCapture>
    <BinaryExpression operator="++">
        <BinaryExpression operator="++">
            <String text="Hello "/>
            <Id name="name"/>
        </BinaryExpression>
        <String text=",

  indented ` text
━━━━━━━━━━━━━━━━━━━━"/>
    </BinaryExpression>
</Declaration>
<Declaration>
    <TypeCapture>
        <Id name="after"/>
    </TypeCapture>
    <Integer value="1"/>
</Declaration>
```

### A text block may end an argument row, an item row or the right side of an operator. One with no rows below it is empty.

```brix
(main
  (call show
    (block
      (arg
        (paragraph
          "one"
        )
      )
      (arg 2)
    )
  )
  (let items
    (call (of List String)
      (block
        (item
          (paragraph
            "x"
          )
        )
      )
    )
  )
  (let joined
    (++ "a: "
      (paragraph
        "b"
      )
    )
  )
  (let empty (paragraph))
)
```

```output
<CallExpression>
    <Id name="show"/>
    <Argument>
        <String text="one"/>
    </Argument>
    <Argument>
        <Integer value="2"/>
    </Argument>
</CallExpression>
<Declaration>
    <TypeCapture>
        <Id name="items"/>
    </TypeCapture>
    <CallExpression>
        <TypeArguments>
            <TypeId name="List"/>
            <TypeId name="String"/>
        </TypeArguments>
        <Argument>
            <Block>
                <Feed>
                    <String text="x"/>
                </Feed>
            </Block>
        </Argument>
    </CallExpression>
</Declaration>
<Declaration>
    <TypeCapture>
        <Id name="joined"/>
    </TypeCapture>
    <BinaryExpression operator="++">
        <String text="a: "/>
        <String text="b"/>
    </BinaryExpression>
</Declaration>
<Declaration>
    <TypeCapture>
        <Id name="empty"/>
    </TypeCapture>
    <String text=""/>
</Declaration>
```

### A row that ends in a quote that opens a text takes the rows indented below it as the lines of a text. A line may hold quotes, unescaped, and end in one.

```brix
(main
  (let greeting
    (paragraph
      (str "say \"hi\" to " name " \"")
      "and \"escaped\""
    )
  )
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="greeting"/>
    </TypeCapture>
    <BinaryExpression operator="++">
        <BinaryExpression operator="++">
            <String text="say "hi" to "/>
            <Id name="name"/>
        </BinaryExpression>
        <String text=" "
and "escaped""/>
    </BinaryExpression>
</Declaration>
```

### A row that ends in the quote that closes a text, as `" "` does, keeps the rows indented below it as code.

```brix
(main
  (let label
    (scope (++ mark " ")
      (lazy mark "Bo")
    )
  )
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="label"/>
    </TypeCapture>
    <Block>
        <Declaration lazy="true">
            <TypeCapture>
                <Id name="mark"/>
            </TypeCapture>
            <String text="Bo"/>
        </Declaration>
        <Declaration lazy="true">
            <TypeCapture>
                <Id name="it"/>
            </TypeCapture>
            <BinaryExpression operator="++">
                <Id name="mark"/>
                <String text=" "/>
            </BinaryExpression>
        </Declaration>
        <Id name="it"/>
    </Block>
</Declaration>
```

### A function whose access mark is followed by a quote, as in `+ " text`, has a text body.

```brix
(fn-expr + text _
  (param name String)
  (paragraph
    (str "Hello, \"" name "\".")
  )
)
```

```output
<Function names="text" access="public-access">
    <ArgumentDecl>
        <TypeCapture>
            <Id name="name"/>
            <TypeId name="String"/>
        </TypeCapture>
    </ArgumentDecl>
    <Block>
        <BinaryExpression operator="++">
            <BinaryExpression operator="++">
                <String text="Hello, ""/>
                <Id name="name"/>
            </BinaryExpression>
            <String text=""."/>
        </BinaryExpression>
    </Block>
</Function>
```
