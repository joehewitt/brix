# Grammars

Patterns

```brix
(main
  (~ Foo)
)
```

```output
<StreamMatch>
    <StreamType>
        <TypeId name="Foo"/>
    </StreamType>
</StreamMatch>
```

```brix
(main
  (~ (: foo Foo))
)
```

```output
<StreamMatch>
    <StreamCapture>
        <Id name="foo"/>
        <StreamType>
            <TypeId name="Foo"/>
        </StreamType>
    </StreamCapture>
</StreamMatch>
```

```brix
(main
  (~ (seq (: foo Foo) (: bar Bar)))
)
```

```output
<StreamMatch>
    <StreamAnd>
        <StreamCapture>
            <Id name="foo"/>
            <StreamType>
                <TypeId name="Foo"/>
            </StreamType>
        </StreamCapture>
        <StreamCapture>
            <Id name="bar"/>
            <StreamType>
                <TypeId name="Bar"/>
            </StreamType>
        </StreamCapture>
    </StreamAnd>
</StreamMatch>
```

```brix
(main
  (~ (? Foo))
)
```

```output
<StreamMatch>
    <UnaryStream op="?">
        <StreamType>
            <TypeId name="Foo"/>
        </StreamType>
    </UnaryStream>
</StreamMatch>
```

```brix
(main
  (~ (* Foo))
)
```

```output
<StreamMatch>
    <UnaryStream op="*">
        <StreamType>
            <TypeId name="Foo"/>
        </StreamType>
    </UnaryStream>
</StreamMatch>
```

```brix
(main
  (~ (+ Foo))
)
```

```output
<StreamMatch>
    <UnaryStream op="+">
        <StreamType>
            <TypeId name="Foo"/>
        </StreamType>
    </UnaryStream>
</StreamMatch>
```

```brix
(main
  (~ (seq Foo (+ (seq Bar Baz))))
)
```

```output
<StreamMatch>
    <StreamAnd>
        <StreamType>
            <TypeId name="Foo"/>
        </StreamType>
        <UnaryStream op="+">
            <StreamAnd>
                <StreamType>
                    <TypeId name="Bar"/>
                </StreamType>
                <StreamType>
                    <TypeId name="Baz"/>
                </StreamType>
            </StreamAnd>
        </UnaryStream>
    </StreamAnd>
</StreamMatch>
```

```brix
(main
  (~ (: sc (seq Foo Bar)))
)
```

```output
<StreamMatch>
    <StreamCapture>
        <Id name="sc"/>
        <StreamAnd>
            <StreamType>
                <TypeId name="Foo"/>
            </StreamType>
            <StreamType>
                <TypeId name="Bar"/>
            </StreamType>
        </StreamAnd>
    </StreamCapture>
</StreamMatch>
```

```brix
(main
  (~ (: sc (+ Foo)))
)
```

```output
<StreamMatch>
    <StreamCapture>
        <Id name="sc"/>
        <UnaryStream op="+">
            <StreamType>
                <TypeId name="Foo"/>
            </StreamType>
        </UnaryStream>
    </StreamCapture>
</StreamMatch>
```

```brix
(main
  (~ (! Foo))
)
```

```output
<StreamMatch>
    <UnaryStream op="!">
        <StreamType>
            <TypeId name="Foo"/>
        </StreamType>
    </UnaryStream>
</StreamMatch>
```

```brix
(main
  (~ (| Foo Bar Baz))
)
```

```output
<StreamMatch>
    <StreamOr>
        <StreamOr>
            <StreamType>
                <TypeId name="Foo"/>
            </StreamType>
            <StreamType>
                <TypeId name="Bar"/>
            </StreamType>
        </StreamOr>
        <StreamType>
            <TypeId name="Baz"/>
        </StreamType>
    </StreamOr>
</StreamMatch>
```

```brix
(main
  (~ (chars "a-z0-9|"))
)
```

```output
<StreamMatch>
    <StreamOr>
        <StreamOr>
            <StreamCharRange>
                <Integer value="97" unit="(char)"/>
                <Integer value="122" unit="(char)"/>
            </StreamCharRange>
            <StreamCharRange>
                <Integer value="48" unit="(char)"/>
                <Integer value="57" unit="(char)"/>
            </StreamCharRange>
        </StreamOr>
        <StreamCharRange>
            <Integer value="124" unit="(char)"/>
            <Integer value="124" unit="(char)"/>
        </StreamCharRange>
    </StreamOr>
</StreamMatch>
```

```brix
(main
  (~ (chars "\\u03b1"))
)
```

```output
<StreamMatch>
    <StreamCharRange>
        <Integer value="945" unit="(char)"/>
        <Integer value="945" unit="(char)"/>
    </StreamCharRange>
</StreamMatch>
```

```brix
(main
  (~ (call \foo))
  (~ (call \foo (~ .)))
  (~ (call \foo (~ .) (~ .)))
  (~ (call \foo (~ .) (~ .) (~ .)))
)
```

```output
<StreamMatch>
    <StreamOr>
        <StreamOr>
            <StreamOr>
                <StreamCall name="foo"/>
                <StreamCall name="foo">
                    <args>
                        <StreamMatch>
                            <StreamAny/>
                        </StreamMatch>
                    </args>
                </StreamCall>
            </StreamOr>
            <StreamCall name="foo">
                <args>
                    <StreamMatch>
                        <StreamAny/>
                    </StreamMatch>
                    <StreamMatch>
                        <StreamAny/>
                    </StreamMatch>
                </args>
            </StreamCall>
        </StreamOr>
        <StreamCall name="foo">
            <args>
                <StreamMatch>
                    <StreamAny/>
                </StreamMatch>
                <StreamMatch>
                    <StreamAny/>
                </StreamMatch>
                <StreamMatch>
                    <StreamAny/>
                </StreamMatch>
            </args>
        </StreamCall>
    </StreamOr>
</StreamMatch>
```

## Combinations

```brix
(main
  (~ Foo)
  (~ Bar)
)
```

```output
<StreamMatch>
    <StreamOr>
        <StreamType>
            <TypeId name="Foo"/>
        </StreamType>
        <StreamType>
            <TypeId name="Bar"/>
        </StreamType>
    </StreamOr>
</StreamMatch>
```

```brix
(main
  (~ Foo)
  (default
    (inline bar)
  )
)
```

```output
<StreamMatch>
    <StreamOr>
        <StreamType>
            <TypeId name="Foo"/>
        </StreamType>
        <StreamTransform>
            <Id name="bar"/>
        </StreamTransform>
    </StreamOr>
</StreamMatch>
```

```brix
(main
  (~ Foo)
  (throw bar)
)
```

```output
<StreamMatch>
    <StreamOr>
        <StreamType>
            <TypeId name="Foo"/>
        </StreamType>
        <StreamPanic>
            <StreamTransform>
                <Id name="bar"/>
            </StreamTransform>
        </StreamPanic>
    </StreamOr>
</StreamMatch>
```

```brix
(main
  (case? expr
    (~ Foo)
  )
  (default
    (~ Bar)
  )
)
```

```output
<StructureMatch>
    <TypeOr>
        <TypeTransform>
            <TypePredicate>
                <Id name="expr"/>
            </TypePredicate>
            <Block>
                <StreamMatch>
                    <StreamType>
                        <TypeId name="Foo"/>
                    </StreamType>
                </StreamMatch>
            </Block>
        </TypeTransform>
        <TypeTransform>
            <Block>
                <StreamMatch>
                    <StreamType>
                        <TypeId name="Bar"/>
                    </StreamType>
                </StreamMatch>
            </Block>
        </TypeTransform>
    </TypeOr>
</StructureMatch>
```

```brix
(main
  (= x (~ Bar (of: foo)))
)
```

```output
<AssignmentExpression op="=">
    <Id name="x"/>
    <Match>
        <Id name="foo"/>
        <StreamMatch>
            <StreamType>
                <TypeId name="Bar"/>
            </StreamType>
        </StreamMatch>
    </Match>
</AssignmentExpression>
```

```brix
(main
  (match foo
    (~ \foo)
    (lazy foo
      (pattern
        @rollback
        (~ Foo)
      )
    )
  )
)
```

```output
<Match>
    <Id name="foo"/>
    <Block>
        <StreamMatch>
            <StreamCall name="foo"/>
        </StreamMatch>
        <Declaration lazy="true" pattern="true" rollback="true">
            <TypeCapture>
                <Id name="foo"/>
            </TypeCapture>
            <StreamMatch>
                <StreamType>
                    <TypeId name="Foo"/>
                </StreamType>
            </StreamMatch>
        </Declaration>
    </Block>
</Match>
```

```brix
(main
  (match foo
    (~ \bar
      expr1
    )
    (lazy bar
      (pattern
        (~ Pattern)
      )
    )
  )
)
```

```output
<Match>
    <Id name="foo"/>
    <Block>
        <StreamMatch>
            <StreamTransform>
                <StreamCall name="bar"/>
                <Block>
                    <Id name="expr1"/>
                </Block>
            </StreamTransform>
        </StreamMatch>
        <Declaration lazy="true" pattern="true">
            <TypeCapture>
                <Id name="bar"/>
            </TypeCapture>
            <StreamMatch>
                <StreamType>
                    <TypeId name="Pattern"/>
                </StreamType>
            </StreamMatch>
        </Declaration>
    </Block>
</Match>
```

### Consecutive applied matches don't combine

```brix
(main
  (~ (call (of \foo Bar)) (of: foo))
  (~ (call (of \foo Bar)) (of: foo))
)
```

```output
<Match>
    <Id name="foo"/>
    <StreamMatch>
        <StreamCall name="foo">
            <params>
                <TypeId name="Bar"/>
            </params>
        </StreamCall>
    </StreamMatch>
</Match>
<Match>
    <Id name="foo"/>
    <StreamMatch>
        <StreamCall name="foo">
            <params>
                <TypeId name="Bar"/>
            </params>
        </StreamCall>
    </StreamMatch>
</Match>
```

```brix
(main
  (match foo
    (~ .
      42
    )
    (throw (call Error))
    (lazy a 1)
  )
  (blank)
  (match foo
    (~ .
      42
    )
    (lazy a 1)
    (throw (call Error))
  )
)
```

```output
<Match>
    <Id name="foo"/>
    <Block>
        <StreamMatch>
            <StreamOr>
                <StreamTransform>
                    <StreamAny/>
                    <Block>
                        <Integer value="42"/>
                    </Block>
                </StreamTransform>
                <StreamPanic>
                    <StreamTransform>
                        <CallExpression>
                            <TypeId name="Error"/>
                        </CallExpression>
                    </StreamTransform>
                </StreamPanic>
            </StreamOr>
        </StreamMatch>
        <Declaration lazy="true">
            <TypeCapture>
                <Id name="a"/>
            </TypeCapture>
            <Integer value="1"/>
        </Declaration>
    </Block>
</Match>
<Match>
    <Id name="foo"/>
    <Block>
        <StreamMatch>
            <StreamTransform>
                <StreamAny/>
                <Block>
                    <Integer value="42"/>
                </Block>
            </StreamTransform>
        </StreamMatch>
        <Declaration lazy="true">
            <TypeCapture>
                <Id name="a"/>
            </TypeCapture>
            <Integer value="1"/>
        </Declaration>
        <Assert>
            <CallExpression>
                <TypeId name="Error"/>
            </CallExpression>
        </Assert>
    </Block>
</Match>
```

```brix
(main
  (lazy domain
    (pattern
      (~ "x")
    )
  )
)
```

```output
<Declaration lazy="true" pattern="true">
    <TypeCapture>
        <Id name="domain"/>
    </TypeCapture>
    <StreamMatch>
        <StreamString text="x"/>
    </StreamMatch>
</Declaration>
```

```brix
(main
  (lazy domain
    (pattern
      (~ 'x')
    )
  )
)
```

```output
<Declaration lazy="true" pattern="true">
    <TypeCapture>
        <Id name="domain"/>
    </TypeCapture>
    <StreamMatch>
        <StreamEquality>
            <Integer value="120" unit="(char)"/>
        </StreamEquality>
    </StreamMatch>
</Declaration>
```

```brix
(main
  (lazy gesture
    (pattern
      (~ (seq .press . .release))
    )
  )
)
```

```output
<Declaration lazy="true" pattern="true">
    <TypeCapture>
        <Id name="gesture"/>
    </TypeCapture>
    <StreamMatch>
        <StreamAnd>
            <StreamAnd>
                <StreamEquality>
                    <Symbol name="press"/>
                </StreamEquality>
                <StreamAny/>
            </StreamAnd>
            <StreamEquality>
                <Symbol name="release"/>
            </StreamEquality>
        </StreamAnd>
    </StreamMatch>
</Declaration>
```

```brix
(main
  (~ (\{} foo))
)
```

```output
<StreamMatch>
    <StreamPredicate>
        <Id name="foo"/>
    </StreamPredicate>
</StreamMatch>
```

```brix
(main
  (~ (: ...values Foo))
)
```

```output
<StreamMatch>
    <StreamCapture accumulating="true">
        <Id name="values"/>
        <StreamType>
            <TypeId name="Foo"/>
        </StreamType>
    </StreamCapture>
</StreamMatch>
```

```brix
(main
  (~ (call \helper (into values)))
)
```

```output
<StreamMatch>
    <StreamCall name="helper">
        <collectorMappings>
            <CollectorMapping caller="values"/>
        </collectorMappings>
    </StreamCall>
</StreamMatch>
```

```brix
(main
  (~ (call \expression (into terms outer-terms) (into operators outer-operators)))
)
```

```output
<StreamMatch>
    <StreamCall name="expression">
        <collectorMappings>
            <CollectorMapping callee="terms" caller="outer-terms"/>
            <CollectorMapping callee="operators" caller="outer-operators"/>
        </collectorMappings>
    </StreamCall>
</StreamMatch>
```
