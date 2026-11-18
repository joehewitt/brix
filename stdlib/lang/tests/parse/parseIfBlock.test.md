# If block

If blocks

```yumi
if foo
    body()
```

```brix
(main
  (if foo
    (call body)
  )
)
```

```output
<If>
    <TypePredicate>
        <Id name="foo"/>
    </TypePredicate>
    <Block>
        <CallExpression>
            <Id name="body"/>
        </CallExpression>
    </Block>
</If>
```

```yumi
if foo
    body()
else if bar
    body2()
```

```brix
(main
  (if foo
    (call body)
    (else-if bar
      (call body2)
    )
  )
)
```

```output
<If>
    <TypePredicate>
        <Id name="foo"/>
    </TypePredicate>
    <Block>
        <CallExpression>
            <Id name="body"/>
        </CallExpression>
    </Block>
    <If>
        <TypePredicate>
            <Id name="bar"/>
        </TypePredicate>
        <Block>
            <CallExpression>
                <Id name="body2"/>
            </CallExpression>
        </Block>
    </If>
</If>
```

```yumi
if foo
    body()
else
    body2()
```

```brix
(main
  (if foo
    (call body)
    (else
      (call body2)
    )
  )
)
```

```output
<If>
    <TypePredicate>
        <Id name="foo"/>
    </TypePredicate>
    <Block>
        <CallExpression>
            <Id name="body"/>
        </CallExpression>
    </Block>
    <If>
        <Block>
            <CallExpression>
                <Id name="body2"/>
            </CallExpression>
        </Block>
    </If>
</If>
```

```yumi
if foo
    body()
else if bar
    body2()
else
    body3()
```

```brix
(main
  (if foo
    (call body)
    (else-if bar
      (call body2)
    )
    (else
      (call body3)
    )
  )
)
```

```output
<If>
    <TypePredicate>
        <Id name="foo"/>
    </TypePredicate>
    <Block>
        <CallExpression>
            <Id name="body"/>
        </CallExpression>
    </Block>
    <If>
        <TypePredicate>
            <Id name="bar"/>
        </TypePredicate>
        <Block>
            <CallExpression>
                <Id name="body2"/>
            </CallExpression>
        </Block>
        <If>
            <Block>
                <CallExpression>
                    <Id name="body3"/>
                </CallExpression>
            </Block>
        </If>
    </If>
</If>
```

## Match/Bind Tests

```yumi
if | Foo = bar
    body()
```

```brix
(main
  (if (= (| Foo) bar)
    (call body)
  )
)
```

```output
<If>
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
</If>
```

```yumi
if ~ Foo = bar
    body()
```

```brix
(main
  (if (= (~ Foo) bar)
    (call body)
  )
)
```

```output
<If>
    <Match>
        <Id name="bar"/>
        <StreamMatch>
            <StreamTransform>
                <StreamType>
                    <TypeId name="Foo"/>
                </StreamType>
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
            </StreamTransform>
        </StreamMatch>
    </Match>
</If>
```

```yumi
if ~ Foo = bar
    f()
```

```brix
(main
  (if (= (~ Foo) bar)
    (call f)
  )
)
```

```output
<If>
    <Match>
        <Id name="bar"/>
        <StreamMatch>
            <StreamTransform>
                <StreamType>
                    <TypeId name="Foo"/>
                </StreamType>
                <Block>
                    <Do>
                        <Block>
                            <CallExpression>
                                <Id name="f"/>
                            </CallExpression>
                        </Block>
                    </Do>
                    <Id name="true"/>
                </Block>
            </StreamTransform>
        </StreamMatch>
    </Match>
</If>
```

```yumi crash
. a Int? = 42
if a?
    -- a
```

```output
Exception: Syntax error
<no file:3>
    if a?
         ^
```
