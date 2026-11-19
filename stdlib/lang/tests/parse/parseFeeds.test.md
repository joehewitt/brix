# Feeds

```brix
(main
  (item foo)
  (item bar)
  (item baz)
)
```

```output
<Feed>
    <Id name="foo"/>
    <Feed>
        <Id name="bar"/>
        <Feed>
            <Id name="baz"/>
        </Feed>
    </Feed>
</Feed>
```

```brix
(main
  (item? foo)
  (spread? bar)
)
```

```output
<Feed optional="true">
    <Id name="foo"/>
    <Feed spread="true" optional="true">
        <Id name="bar"/>
    </Feed>
</Feed>
```

```brix
(main
  (item foo)
  (spread bar)
  (item baz)
)
```

```output
<Feed>
    <Id name="foo"/>
    <Feed spread="true">
        <Id name="bar"/>
        <Feed>
            <Id name="baz"/>
        </Feed>
    </Feed>
</Feed>
```

```brix
(main
  (item foo)
  (pipe |> bar)
)
```

```output
<Pipeline op="|>">
    <Feed>
        <Id name="foo"/>
    </Feed>
    <Id name="bar"/>
</Pipeline>
```

```brix
(main
  1
  (pipe |> a)
  (pipe *> b)
  (pipe ?> c)
  (pipe |=> d)
)
```

```output
<Pipeline op="|=>">
    <Pipeline op="?>">
        <Pipeline op="*>">
            <Pipeline op="|>">
                <Integer value="1"/>
                <Id name="a"/>
            </Pipeline>
            <Id name="b"/>
        </Pipeline>
        <Id name="c"/>
    </Pipeline>
    <Id name="d"/>
</Pipeline>
```

```brix
(main
  source
  (pipe ~> \record)
  (pipe ?> (. each relevant))
  (pipe *-> destination)
)
```

```output
<Pipeline op="*->">
    <Pipeline op="?>">
        <Pipeline op="~>">
            <Id name="source"/>
            <StreamMatch>
                <StreamCall name="record"/>
            </StreamMatch>
        </Pipeline>
        <Get name="relevant">
            <Id name="each"/>
        </Get>
    </Pipeline>
    <Id name="destination"/>
</Pipeline>
```

```brix
(main
  (~> source (+ \record))
)
```

```output
<BinaryExpression operator="~>">
    <Id name="source"/>
    <StreamMatch>
        <UnaryStream op="+">
            <StreamCall name="record"/>
        </UnaryStream>
    </StreamMatch>
</BinaryExpression>
```

```brix
(main
  source
  (pipe ~> (seq (: line (* .)) (? \nl))
    line
  )
  (pipe |> print-all)
)
```

```output
<Pipeline op="|>">
    <Pipeline op="~>">
        <Id name="source"/>
        <StreamMatch>
            <StreamTransform>
                <StreamAnd>
                    <StreamCapture>
                        <Id name="line"/>
                        <UnaryStream op="*">
                            <StreamAny/>
                        </UnaryStream>
                    </StreamCapture>
                    <UnaryStream op="?">
                        <StreamCall name="nl"/>
                    </UnaryStream>
                </StreamAnd>
                <Block>
                    <Id name="line"/>
                </Block>
            </StreamTransform>
        </StreamMatch>
    </Pipeline>
    <Id name="print-all"/>
</Pipeline>
```

```brix
(main
  (item
    (call (in foo set)
      (:with
        (block
          indented
        )
      )
    )
  )
  (item bar)
)
```

```output
<Feed>
    <CallExpression>
        <Iterator>
            <In>
                <Id name="set"/>
            </In>
            <Id name="foo"/>
        </Iterator>
        <Argument name="with">
            <Block>
                <Id name="indented"/>
            </Block>
        </Argument>
    </CallExpression>
    <Feed>
        <Id name="bar"/>
    </Feed>
</Feed>
```

```brix
(main
  (<-* foo bar)
)
```

```output
<For>
    <In>
        <TypeCapture>
            <Id name="(item)"/>
        </TypeCapture>
        <Id name="bar"/>
    </In>
    <BinaryExpression operator="<-write">
        <Id name="foo"/>
        <Id name="(item)"/>
    </BinaryExpression>
</For>
```

```brix
(main
  1
  (pipe *> (* each 2))
)
```

```output
<Pipeline op="*>">
    <Integer value="1"/>
    <BinaryExpression operator="*">
        <Id name="each"/>
        <Integer value="2"/>
    </BinaryExpression>
</Pipeline>
```

```brix
(main
  1
  (pipe ?> (* each 2))
)
```

```output
<Pipeline op="?>">
    <Integer value="1"/>
    <BinaryExpression operator="*">
        <Id name="each"/>
        <Integer value="2"/>
    </BinaryExpression>
</Pipeline>
```

```brix
(main
  1
  (pipe |=> (* it 2))
)
```

```output
<Pipeline op="|=>">
    <Integer value="1"/>
    <BinaryExpression operator="*">
        <Id name="it"/>
        <Integer value="2"/>
    </BinaryExpression>
</Pipeline>
```

```brix
(main
  1
  (pipe |> (call double (:where (% each 2))))
)
```

```output
<Pipeline op="|>">
    <Integer value="1"/>
    <CallExpression>
        <Id name="double"/>
        <Argument name="where">
            <Function names="(closure)" access="private-access">
                <ArgumentDecl>
                    <TypeCapture>
                        <Id name="each"/>
                    </TypeCapture>
                </ArgumentDecl>
                <BinaryExpression operator="%">
                    <Id name="each"/>
                    <Integer value="2"/>
                </BinaryExpression>
            </Function>
        </Argument>
    </CallExpression>
</Pipeline>
```

```brix
(main
  (header foo)
  bar
  baz
)
```

```output
<Header>
    <Id name="foo"/>
    <items>
        <Id name="bar"/>
        <Id name="baz"/>
    </items>
</Header>
```

```brix
(main
  (header foo)
  (=> () bar)
  (=> () baz)
)
```

```output
<Header>
    <Id name="foo"/>
    <items>
        <Function names="(closure)" access="private-access">
            <Id name="bar"/>
        </Function>
        <Function names="(closure2)" access="private-access">
            <Id name="baz"/>
        </Function>
    </items>
</Header>
```

```brix
(main
  (header foo)
  bar
  (lazy bar 1)
  (header baz)
  qux
)
```

```output
<Declaration lazy="true">
    <TypeCapture>
        <Id name="bar"/>
    </TypeCapture>
    <Integer value="1"/>
</Declaration>
<Header>
    <Id name="foo"/>
    <items>
        <Id name="bar"/>
        <Header>
            <Id name="baz"/>
            <items/>
        </Header>
        <Id name="qux"/>
    </items>
</Header>
```
