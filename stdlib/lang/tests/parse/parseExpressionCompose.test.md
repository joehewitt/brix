# Expression compose

```brix
(main
  (|> foo bar)
)
```

```output
<BinaryExpression operator="|>">
    <Id name="foo"/>
    <Id name="bar"/>
</BinaryExpression>
```

```brix
(main
  (>> foo bar)
)
```

```output
<BinaryExpression operator=">>">
    <Id name="foo"/>
    <Id name="bar"/>
</BinaryExpression>
```

```brix
(main
  (<< bar foo)
)
```

```output
<BinaryExpression operator="<<">
    <Id name="bar"/>
    <Id name="foo"/>
</BinaryExpression>
```

```brix
(main
  (*> foo bar)
)
```

```output
<BinaryExpression operator="*>">
    <Id name="foo"/>
    <Id name="bar"/>
</BinaryExpression>
```

```brix
(main
  (?> bar foo)
)
```

```output
<BinaryExpression operator="?>">
    <Id name="bar"/>
    <Id name="foo"/>
</BinaryExpression>
```

```brix
(main
  (*> bar foo)
)
```

```output
<BinaryExpression operator="*>">
    <Id name="bar"/>
    <Id name="foo"/>
</BinaryExpression>
```

```brix
(main
  (?> bar foo)
)
```

```output
<BinaryExpression operator="?>">
    <Id name="bar"/>
    <Id name="foo"/>
</BinaryExpression>
```

```brix
(main
  (?> values (> each 0))
)
```

```output
<BinaryExpression operator="?>">
    <Id name="values"/>
    <BinaryExpression operator=">">
        <Id name="each"/>
        <Integer value="0"/>
    </BinaryExpression>
</BinaryExpression>
```

```brix
(main
  (<| foo bar)
)
```

```output
<BinaryExpression operator="<|">
    <Id name="foo"/>
    <Id name="bar"/>
</BinaryExpression>
```

```brix
(main
  (|=> bar foo)
)
```

```output
<BinaryExpression operator="|=>">
    <Id name="bar"/>
    <Id name="foo"/>
</BinaryExpression>
```

```brix
(main
  (let rows
    (block
      (spread (.. 1 2))
      (pipe *>
        (block
          (call List
            (block
              (spread (.. 1 2))
              (pipe *> (+ (* each 10) outer))
            )
          )
          (lazy outer each)
        )
      )
    )
  )
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="rows"/>
    </TypeCapture>
    <Block>
        <Pipeline op="*>">
            <Feed spread="true">
                <RangeExpression operator="thru">
                    <Integer value="1"/>
                    <Integer value="2"/>
                </RangeExpression>
            </Feed>
            <Block>
                <CallExpression>
                    <TypeId name="List"/>
                    <Argument>
                        <Block>
                            <Pipeline op="*>">
                                <Feed spread="true">
                                    <RangeExpression operator="thru">
                                        <Integer value="1"/>
                                        <Integer value="2"/>
                                    </RangeExpression>
                                </Feed>
                                <BinaryExpression operator="+">
                                    <BinaryExpression operator="*">
                                        <Id name="each"/>
                                        <Integer value="10"/>
                                    </BinaryExpression>
                                    <Id name="outer"/>
                                </BinaryExpression>
                            </Pipeline>
                        </Block>
                    </Argument>
                </CallExpression>
                <Declaration lazy="true">
                    <TypeCapture>
                        <Id name="outer"/>
                    </TypeCapture>
                    <Id name="each"/>
                </Declaration>
            </Block>
        </Pipeline>
    </Block>
</Declaration>
```

```brix
(main
  (*|> values (call action each))
)
```

```output
<BinaryExpression operator="*|>">
    <Id name="values"/>
    <CallExpression>
        <Id name="action"/>
        <Argument>
            <Id name="each"/>
        </Argument>
    </CallExpression>
</BinaryExpression>
```

```brix
(main
  (spread values)
  (pipe *|> (call action each))
)
```

```output
<Pipeline op="*|>">
    <Feed spread="true">
        <Id name="values"/>
    </Feed>
    <CallExpression>
        <Id name="action"/>
        <Argument>
            <Id name="each"/>
        </Argument>
    </CallExpression>
</Pipeline>
```
