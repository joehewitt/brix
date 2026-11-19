# If block

If blocks

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
