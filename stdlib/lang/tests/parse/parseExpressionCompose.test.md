# Expression compose

```yumi
foo |> bar
```

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

```yumi
foo >> bar
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

```yumi
bar << foo
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

```yumi
foo *> bar
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

```yumi
bar ?> foo
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

```yumi
bar *> foo
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

```yumi
bar ?> foo
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

```yumi
values ?> each > 0
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

```yumi
foo <| bar
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

```yumi
bar |=> foo
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

```yumi
. rows =
    ** 1 .. 2
    *>
        List
            ** 1 .. 2
            *> each * 10 + outer
        .. outer = each
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

```yumi
values *|> action(each)
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

```yumi
** values
*|> action(each)
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
