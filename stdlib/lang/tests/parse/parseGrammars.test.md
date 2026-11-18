# Grammars

Patterns

```yumi
~ Foo
```

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

```yumi
~ foo Foo
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

```yumi
~ foo Foo bar Bar
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

```yumi
~ Foo?
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

```yumi
~ Foo*
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

```yumi
~ Foo+
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

```yumi
~ Foo (Bar Baz)+
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

```yumi
~ sc (Foo Bar)
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

```yumi
~ sc Foo+
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

```yumi
~ !Foo
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

```yumi
~ Foo | Bar | Baz
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

```yumi
~ [a-z0-9|]
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

```yumi
~ [\u03b1]
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

```yumi
~ [\uD800-\uDFFF]
```

```output
Exception: "\uD800" is half of a surrogate pair, which is no character by itself; write the character itself in the text
<no file:2>
    ~ [\uD800-\uDFFF]
         ^
```

```yumi
~ \foo()
~ \foo(~ .)
~ \foo(~ ., ~ .)
~ \foo(~ ., ~ ., ~ .)
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

```yumi
~ Foo
~ Bar
```

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

```yumi
~ Foo
?? bar
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

```yumi
~ Foo
?! bar
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

```yumi
|? expr
    ~ Foo
??
    ~ Bar
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

```yumi
x = ~ Bar = foo
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

```yumi
match foo
    ~ \foo
    .. foo ~ @rollback
        ~ Foo
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

```yumi
match foo
    ~ \bar
        expr1
    .. bar ~
        ~ Pattern
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

```yumi
~ \foo'Bar() = foo
~ \foo'Bar() = foo
```

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

```yumi
match foo
    ~ .
        42
    ?! Error()
    .. a = 1

match foo
    ~ .
        42
    .. a = 1
    ?! Error()
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

```yumi
.. domain ~
    ~ "x"
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

```yumi
.. domain ~
    ~ 'x'
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

```yumi
.. gesture ~
    ~ .press . .release
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

```yumi
.. domain ~
    ~ \letter

    .. foo = 1
```

```output
Exception: A local rule holds only its arms; declare a lazy local it reads below the action of an arm, or beside the match
<no file:5>
        .. foo = 1
        ^^^^^^^^^
```

```yumi
~ [a-z
-- "after"
```

```output
Exception: Syntax error
<no file:2>
    ~ [a-z
          ^
```

```yumi
~ "a"+*
```

```output
Exception: A part of a pattern takes one of ?, * or +, not "+*"; to repeat a repeated part, wrap it in parentheses first
<no file:2>
    ~ "a"+*
          ^
```

```yumi
~ \{foo}
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

```yumi
~ ...values Foo
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

```yumi
~ \helper(into values)
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

```yumi
~ \expression(into terms: outer-terms, into operators: outer-operators)
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
