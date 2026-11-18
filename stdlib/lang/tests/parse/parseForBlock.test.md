# For block

While loops

```yumi
while true
    indented()
```

```brix
(main
  (while true
    (call indented)
  )
)
```

```output
<While>
    <Id name="true"/>
    <Block>
        <CallExpression>
            <Id name="indented"/>
        </CallExpression>
    </Block>
</While>
```

```yumi
while | Foo = bar
    body()
```

```brix
(main
  (while (= (| Foo) bar)
    (call body)
  )
)
```

```output
<While>
    <Match>
        <Id name="bar"/>
        <StructureMatch>
            <TypeTransform>
                <TypeId name="Foo"/>
                <Block>
                    <Do>
                        <Block>
                            <CallExpression>
                                <Id name="body"/>
                            </CallExpression>
                        </Block>
                    </Do>
                    <Id name="true"/>
                </Block>
            </TypeTransform>
        </StructureMatch>
    </Match>
    <Block/>
</While>
```

## For iterators

```yumi
for a in b
    indented()
```

```brix
(main
  (for (in a b)
    (call indented)
  )
)
```

```output
<For>
    <In>
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
        <Id name="b"/>
    </In>
    <Block>
        <CallExpression>
            <Id name="indented"/>
        </CallExpression>
    </Block>
</For>
```

```yumi
for a Foo in b
    indented()
```

```brix
(main
  (for (in (: a Foo) b)
    (call indented)
  )
)
```

```output
<For>
    <In>
        <TypeCapture>
            <Id name="a"/>
            <TypeId name="Foo"/>
        </TypeCapture>
        <Id name="b"/>
    </In>
    <Block>
        <CallExpression>
            <Id name="indented"/>
        </CallExpression>
    </Block>
</For>
```

```yumi
for a, b in b
    indented()
```

```brix
(main
  (for (in (tuple a b) b)
    (call indented)
  )
)
```

```output
<For>
    <In>
        <TypeTuple>
            <TypeCapture>
                <Id name="a"/>
            </TypeCapture>
            <TypeCapture>
                <Id name="b"/>
            </TypeCapture>
        </TypeTuple>
        <Id name="b"/>
    </In>
    <Block>
        <CallExpression>
            <Id name="indented"/>
        </CallExpression>
    </Block>
</For>
```

```yumi
for a Foo, b Bar in c
    indented()
```

```brix
(main
  (for (in (tuple (: a Foo) (: b Bar)) c)
    (call indented)
  )
)
```

```output
<For>
    <In>
        <TypeTuple>
            <TypeCapture>
                <Id name="a"/>
                <TypeId name="Foo"/>
            </TypeCapture>
            <TypeCapture>
                <Id name="b"/>
                <TypeId name="Bar"/>
            </TypeCapture>
        </TypeTuple>
        <Id name="c"/>
    </In>
    <Block>
        <CallExpression>
            <Id name="indented"/>
        </CallExpression>
    </Block>
</For>
```

```yumi
for a in x while b when c
    indented()
```

```brix
(main
  (for (in a x (while b) (when c))
    (call indented)
  )
)
```

```output
<For>
    <In>
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
        <Id name="x"/>
        <While>
            <Id name="b"/>
            <When>
                <Id name="c"/>
            </When>
        </While>
    </In>
    <Block>
        <CallExpression>
            <Id name="indented"/>
        </CallExpression>
    </Block>
</For>
```

```yumi
for a in b while c
    indented()
```

```brix
(main
  (for (in a b (while c))
    (call indented)
  )
)
```

```output
<For>
    <In>
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
        <Id name="b"/>
        <While>
            <Id name="c"/>
        </While>
    </In>
    <Block>
        <CallExpression>
            <Id name="indented"/>
        </CallExpression>
    </Block>
</For>
```

## Loop Logic

```yumi
for a in b
    body1()
or c in d
    body2()
```

```brix
(main
  (for (in a b)
    (call body1)
    (or-for (in c d)
      (call body2)
    )
  )
)
```

```output
<For>
    <In>
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
        <Id name="b"/>
    </In>
    <Block>
        <CallExpression>
            <Id name="body1"/>
        </CallExpression>
    </Block>
    <For>
        <In>
            <TypeCapture>
                <Id name="c"/>
            </TypeCapture>
            <Id name="d"/>
        </In>
        <Block>
            <CallExpression>
                <Id name="body2"/>
            </CallExpression>
        </Block>
    </For>
</For>
```

```yumi
for a in b
    body1
or 99
```

```output
Exception: Illegal loop section
<no file:4>
    or 99
      ^^^
```

```yumi
for a in b
    body1
or c
    body2
```

```output
Exception: Illegal loop section
<no file:4>
    or c
      ^
```

```yumi
for a in aa
and b in bb
    body1()
```

```brix
(main
  (for (in a aa)
    (and-for (in b bb)
      (call body1)
    )
  )
)
```

```output
<For>
    <In>
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
        <Id name="aa"/>
        <In>
            <TypeCapture>
                <Id name="b"/>
            </TypeCapture>
            <Id name="bb"/>
        </In>
    </In>
    <Block>
        <CallExpression>
            <Id name="body1"/>
        </CallExpression>
    </Block>
</For>
```

```yumi
for a in aa
and b in bb
    body1()
or c in cc
    body2()
```

```brix
(main
  (for (in a aa)
    (and-for (in b bb)
      (call body1)
    )
    (or-for (in c cc)
      (call body2)
    )
  )
)
```

```output
<For>
    <In>
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
        <Id name="aa"/>
        <In>
            <TypeCapture>
                <Id name="b"/>
            </TypeCapture>
            <Id name="bb"/>
        </In>
    </In>
    <Block>
        <CallExpression>
            <Id name="body1"/>
        </CallExpression>
    </Block>
    <For>
        <In>
            <TypeCapture>
                <Id name="c"/>
            </TypeCapture>
            <Id name="cc"/>
        </In>
        <Block>
            <CallExpression>
                <Id name="body2"/>
            </CallExpression>
        </Block>
    </For>
</For>
```

```yumi
for a in aa
    body1()
or c in cc
and b in bb
    body2()
```

```brix
(main
  (for (in a aa)
    (call body1)
    (or-for (in c cc))
    (and-for (in b bb)
      (call body2)
    )
  )
)
```

```output
<For>
    <In>
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
        <Id name="aa"/>
    </In>
    <Block>
        <CallExpression>
            <Id name="body1"/>
        </CallExpression>
    </Block>
    <For>
        <In>
            <TypeCapture>
                <Id name="c"/>
            </TypeCapture>
            <Id name="cc"/>
            <In>
                <TypeCapture>
                    <Id name="b"/>
                </TypeCapture>
                <Id name="bb"/>
            </In>
        </In>
        <Block>
            <CallExpression>
                <Id name="body2"/>
            </CallExpression>
        </Block>
    </For>
</For>
```

```yumi
for a in aa
    body1()
or c in cc
and b in bb
    body2()
or d in dd
    body3()
```

```brix
(main
  (for (in a aa)
    (call body1)
    (or-for (in c cc))
    (and-for (in b bb)
      (call body2)
    )
    (or-for (in d dd)
      (call body3)
    )
  )
)
```

```output
<For>
    <In>
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
        <Id name="aa"/>
    </In>
    <Block>
        <CallExpression>
            <Id name="body1"/>
        </CallExpression>
    </Block>
    <For>
        <In>
            <TypeCapture>
                <Id name="c"/>
            </TypeCapture>
            <Id name="cc"/>
            <In>
                <TypeCapture>
                    <Id name="b"/>
                </TypeCapture>
                <Id name="bb"/>
            </In>
        </In>
        <Block>
            <CallExpression>
                <Id name="body2"/>
            </CallExpression>
        </Block>
        <For>
            <In>
                <TypeCapture>
                    <Id name="d"/>
                </TypeCapture>
                <Id name="dd"/>
            </In>
            <Block>
                <CallExpression>
                    <Id name="body3"/>
                </CallExpression>
            </Block>
        </For>
    </For>
</For>
```

```yumi
for in stream
    indented()
```

```brix
(main
  (for (in stream)
    (call indented)
  )
)
```

```output
<For>
    <In>
        <Id name="stream"/>
    </In>
    <Block>
        <CallExpression>
            <Id name="indented"/>
        </CallExpression>
    </Block>
</For>
```

```yumi
for in stream when foo
    indented()
```

```brix
(main
  (for (in stream (when foo))
    (call indented)
  )
)
```

```output
<For>
    <In>
        <Id name="stream"/>
        <When>
            <Id name="foo"/>
        </When>
    </In>
    <Block>
        <CallExpression>
            <Id name="indented"/>
        </CallExpression>
    </Block>
</For>
```

```yumi
for in xs when ok
    f(each)
for on s
    g(each)
```

```brix
(main
  (for (in xs (when ok))
    (call f each)
  )
  (for (on s)
    (call g each)
  )
)
```

```output
<For>
    <In>
        <Id name="xs"/>
        <When>
            <Id name="ok"/>
        </When>
    </In>
    <Block>
        <CallExpression>
            <Id name="f"/>
            <Argument>
                <Id name="each"/>
            </Argument>
        </CallExpression>
    </Block>
</For>
<For>
    <On>
        <Id name="s"/>
    </On>
    <Block>
        <CallExpression>
            <Id name="g"/>
            <Argument>
                <Id name="each"/>
            </Argument>
        </CallExpression>
    </Block>
</For>
```

```yumi
for in ["u", "v"]
and in [1, 2]
    f(each, it)
for a in b
    f(a)
or in g
    h(each)
for _ in xs
    k()
```

```brix
(main
  (for (in (list "u" "v"))
    (and-for (in (list 1 2))
      (call f each it)
    )
  )
  (for (in a b)
    (call f a)
    (or-for (in g)
      (call h each)
    )
  )
  (for (in _ xs)
    (call k)
  )
)
```

```output
<For>
    <In>
        <List>
            <String text="u"/>
            <String text="v"/>
        </List>
        <In>
            <List>
                <Integer value="1"/>
                <Integer value="2"/>
            </List>
        </In>
    </In>
    <Block>
        <CallExpression>
            <Id name="f"/>
            <Argument>
                <Id name="each"/>
            </Argument>
            <Argument>
                <Id name="it"/>
            </Argument>
        </CallExpression>
    </Block>
</For>
<For>
    <In>
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
        <Id name="b"/>
    </In>
    <Block>
        <CallExpression>
            <Id name="f"/>
            <Argument>
                <Id name="a"/>
            </Argument>
        </CallExpression>
    </Block>
    <For>
        <In>
            <Id name="g"/>
        </In>
        <Block>
            <CallExpression>
                <Id name="h"/>
                <Argument>
                    <Id name="each"/>
                </Argument>
            </CallExpression>
        </Block>
    </For>
</For>
<For>
    <In>
        <TypeWildcard/>
        <Id name="xs"/>
    </In>
    <Block>
        <CallExpression>
            <Id name="k"/>
        </CallExpression>
    </Block>
</For>
```

```yumi
for ~ "foo" in bar
    --
```

```brix
(main
  (for (in (~ "foo") bar)
    (print)
  )
)
```

```output
<For>
    <In>
        <StreamString text="foo"/>
        <Id name="bar"/>
    </In>
    <Block/>
</For>
```

```yumi crash
. a Int? = 42
while a?
    -- a
```

```output
Exception: Syntax error
<no file:3>
    while a?
            ^
```

```yumi
for item in #items keyed by item.id
    item()
```

```brix
(main
  (for (in item #items (keyed-by (. item id)))
    (call item)
  )
)
```

```output
<For>
    <In>
        <TypeCapture>
            <Id name="item"/>
        </TypeCapture>
        <Id name="items" tracked="true"/>
        <Clause op="keyed-by-clause">
            <Get name="id">
                <Id name="item"/>
            </Get>
        </Clause>
    </In>
    <Block>
        <CallExpression>
            <Id name="item"/>
        </CallExpression>
    </Block>
</For>
```
