# Classes

```brix
(type + Foo)
```

```output
<Class name="Foo" access="public-access"/>
```

```brix
(type _ Foo)
```

```output
<Class name="Foo" access="private-access"/>
```

```brix
(type + Class1
  (fn-expr + func1 _
    body
  )
  (fn-expr + func2 _
    body
  )
)

(type + Class2
  (fn-expr + func1 _
    body
  )
)
```

```output
<Class name="Class1" access="public-access">
    <Function names="func1" access="public-access">
        <Block>
            <Id name="body"/>
        </Block>
    </Function>
    <Function names="func2" access="public-access">
        <Block>
            <Id name="body"/>
        </Block>
    </Function>
</Class>
<Class name="Class2" access="public-access">
    <Function names="func1" access="public-access">
        <Block>
            <Id name="body"/>
        </Block>
    </Function>
</Class>
```

```brix
(type + Class1
  (fn-expr + This _ (over: Foo)
    body
  )
)
```

```output
<Class name="Class1" access="public-access">
    <Function names="This" access="public-access">
        <pattern>
            <TypeId name="Foo"/>
        </pattern>
        <TypeId name="StreamItem"/>
        <ArgumentDecl>
            <TypeCapture>
                <Id name="(stream)"/>
                <TypeArguments>
                    <TypeId name="TokenStream"/>
                    <TypeId name="Foo"/>
                </TypeArguments>
            </TypeCapture>
        </ArgumentDecl>
        <TypeConstraint op="exactly">
            <TypeId name="StreamItem"/>
            <TypeId name="Foo"/>
        </TypeConstraint>
        <Block>
            <Id name="body"/>
        </Block>
    </Function>
</Class>
```

## Inheritance

```brix
(type + Class1 (is Foo))
```

```output
<Class name="Class1" access="public-access">
    <Implements inherited="true">
        <TypeId name="Foo"/>
    </Implements>
</Class>
```

```brix
(type + Class1
  (implements Foo)
  (fn-expr + func _
    body
  )
  (fn-expr + func _
    body
  )
)
```

```output
<Class name="Class1" access="public-access">
    <Implements>
        <TypeId name="Foo"/>
    </Implements>
    <Function names="func" access="public-access">
        <Block>
            <Id name="body"/>
        </Block>
    </Function>
    <Function names="func" access="public-access">
        <Block>
            <Id name="body"/>
        </Block>
    </Function>
</Class>
```

```brix
(type + (of Headered Item)
  (constraint is Item Printable)
  (implements Proto)
  (using Helpers)
)
```

```output
<Class name="Headered" access="public-access">
    <TypeId name="Item"/>
    <TypeConstraint op="convertible">
        <TypeId name="Item"/>
        <TypeId name="Printable"/>
    </TypeConstraint>
    <Implements>
        <TypeId name="Proto"/>
    </Implements>
    <Using>
        <TypeId name="Helpers"/>
    </Using>
</Class>
```

```brix
(type _ Part (facet-of Owner))
```

```output
<Class name="Part" access="private-access">
    <FacetOf>
        <TypeId name="Owner"/>
    </FacetOf>
</Class>
```

```brix
(type + BoxScroll (facet-of Box)
  (sees BoxMeasure)
  (sees BoxFocus)
)
```

```output
<Class name="BoxScroll" access="public-access">
    <FacetOf>
        <TypeId name="Box"/>
    </FacetOf>
    <Sees>
        <TypeId name="BoxMeasure"/>
    </Sees>
    <Sees>
        <TypeId name="BoxFocus"/>
    </Sees>
</Class>
```

## Members

```brix
(type + Class1
  (prop + foo Foo)
)
```

```output
<Class name="Class1" access="public-access">
    <Property access="public-access">
        <TypeCapture>
            <Id name="foo"/>
            <TypeId name="Foo"/>
        </TypeCapture>
    </Property>
</Class>
```

```brix
(type + Class1
  (prop _ foo Foo)
)
```

```output
<Class name="Class1" access="public-access">
    <Property access="private-access">
        <TypeCapture>
            <Id name="foo"/>
            <TypeId name="Foo"/>
        </TypeCapture>
    </Property>
</Class>
```

```brix
(type + Class1
  (prop ^ foo Foo)
  (fn-do ^ This _
    (param start Foo)
    (= foo start)
  )
  (fn-do ^ step _
    (call (. foo step))
  )
)
```

```output
<Class name="Class1" access="public-access">
    <Property access="family-access">
        <TypeCapture>
            <Id name="foo"/>
            <TypeId name="Foo"/>
        </TypeCapture>
    </Property>
    <Function names="This" access="family-access" imperative="true">
        <ArgumentDecl>
            <TypeCapture>
                <Id name="start"/>
                <TypeId name="Foo"/>
            </TypeCapture>
        </ArgumentDecl>
        <Block>
            <AssignmentExpression op="=">
                <Id name="foo"/>
                <Id name="start"/>
            </AssignmentExpression>
        </Block>
    </Function>
    <Function names="step" access="family-access" imperative="true">
        <Block>
            <CallExpression>
                <Get name="step">
                    <Id name="foo"/>
                </Get>
            </CallExpression>
        </Block>
    </Function>
</Class>
```

```brix
(type + Class1
  (prop + bar Bar (= 1))
)
```

```output
<Class name="Class1" access="public-access">
    <Property access="public-access">
        <TypeCapture>
            <Id name="bar"/>
            <TypeId name="Bar"/>
        </TypeCapture>
        <Integer value="1"/>
    </Property>
</Class>
```

```brix
(type + (of Class1 Foo))
```

```output
<Class name="Class1" access="public-access">
    <TypeId name="Foo"/>
</Class>
```

## Converters

```brix
(type + Class1
  (fn-from-expr + (: foo Foo))
)
```

```output
<Class name="Class1" access="public-access">
    <Fromverter access="public-access">
        <TypeCapture>
            <Id name="foo"/>
            <TypeId name="Foo"/>
        </TypeCapture>
    </Fromverter>
</Class>
```

```brix
(type + Class1
  (fn-to-expr + Foo)
  (fn-to-expr + Foo
    (constraint = A 42)
  )
)
```

```output
<Class name="Class1" access="public-access">
    <Converter access="public-access">
        <TypeId name="Foo"/>
    </Converter>
    <Converter access="public-access">
        <TypeId name="Foo"/>
        <ConstAssignment>
            <TypeId name="A"/>
            <Integer value="42"/>
        </ConstAssignment>
    </Converter>
</Class>
```

## Aliases

```brix
(type + Class1 (alias alias))
```

```output
<Class name="Class1" access="public-access" aliases="alias"/>
```

```brix
(type + Class1 (alias Alias))
```

```output
<Class name="Class1" access="public-access" aliases="Alias"/>
```

```brix
(type + Class1 (alias a b))
```

```output
<Class name="Class1" access="public-access" aliases="a, b"/>
```

```brix
(type + Foo
  (prop + bar _ (= baz))
  (prop + bar _ (:= baz))
  (prop + bar _ (=> baz))
)
```

```output
<Class name="Foo" access="public-access">
    <Property access="public-access">
        <TypeCapture>
            <Id name="bar"/>
        </TypeCapture>
        <Id name="baz"/>
    </Property>
    <Property access="public-access" lazy="true">
        <TypeCapture>
            <Id name="bar"/>
        </TypeCapture>
        <Id name="baz"/>
    </Property>
    <Function names="bar" access="public-access" getter="true">
        <Id name="baz"/>
    </Function>
</Class>
```

```brix
(type + Foo
  (depends bar baz)
  (depends bar a b c)
)
```

```output
<Class name="Foo" access="public-access">
    <Dependencies>
        <Id name="bar"/>
        <Id name="baz"/>
    </Dependencies>
    <Dependencies>
        <Id name="bar"/>
        <Id name="a"/>
        <Id name="b"/>
        <Id name="c"/>
    </Dependencies>
</Class>
```

```brix
(type-line + Foo (is Bar Baz))
```

```output
<Class name="Foo" access="public-access">
    <Implements inherited="true">
        <TypeId name="Or"/>
    </Implements>
    <Or>
        <TypeId name="Bar"/>
        <TypeId name="Baz"/>
    </Or>
</Class>
```

```brix
(type + (of Foo T)
  (constraint = T Char)
)
```

```output
<Class name="Foo" access="public-access">
    <TypeId name="T"/>
    <ConstAssignment>
        <TypeId name="T"/>
        <TypeId name="Char"/>
    </ConstAssignment>
</Class>
```

```brix
(unit + 1bar (%% 2 3foo))
```

```output
<Unit access="public-access">
    <Integer value="1" unit="bar"/>
    <BinaryExpression operator="%%">
        <Integer value="2"/>
        <Integer value="3" unit="foo"/>
    </BinaryExpression>
</Unit>
```

### Property annotations parse with the same pre-name decoration form as globals.

```brix
(type + TextStyle (is Style)
  (prop + color Color @inherited (= white))
)
```

```output
<Class name="TextStyle" access="public-access">
    <Implements inherited="true">
        <TypeId name="Style"/>
    </Implements>
    <Property access="public-access" annotations="inherited">
        <TypeCapture>
            <Id name="color"/>
            <TypeId name="Color"/>
        </TypeCapture>
        <Id name="white"/>
    </Property>
</Class>
```

### A style property's name may be keyed with a dot, as motion keys are.

```brix
(type + DialStyle (is Style)
  (prop + motion.fade (? Motion) (= null))
)
```

```output
<Class name="DialStyle" access="public-access">
    <Implements inherited="true">
        <TypeId name="Style"/>
    </Implements>
    <Property access="public-access">
        <TypeCapture>
            <Id name="motion.fade"/>
            <TypeArguments>
                <TypeId name="Optional"/>
                <TypeId name="Motion"/>
            </TypeArguments>
        </TypeCapture>
        <Id name="null"/>
    </Property>
</Class>
```

### The removed `. x is y` style flag annotation form no longer parses.

```brix
(type + Example
  (event + submitted SubmitEvent)
  (event + requested RequestEvent)
  (event + messages Token MessageId)
  (event _ internal-reset ResetEvent)
)
```

```output
<Class name="Example" access="public-access">
    <Event name="submitted" type="SubmitEvent" access="public-access"/>
    <Event name="requested" type="RequestEvent" access="public-access"/>
    <Event name="messages" type="Token" access="public-access" keyed="MessageId"/>
    <Event name="internal-reset" type="ResetEvent" access="private-access"/>
</Class>
```

```brix
(prop + false _ (= (call Bool 0)))
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="false"/>
    </TypeCapture>
    <CallExpression>
        <TypeId name="Bool"/>
        <Argument>
            <Integer value="0"/>
        </Argument>
    </CallExpression>
</Property>
```

```brix
(type + A
  (fn-expr + This _ (over: Char)
    (~ (seq (? "/") (+ (seq \file-word "/")) (? \file-word))
      (call This it)
    )
    (where
      (lazy file-word
        (pattern
          (~ (+ \file-char))
        )
      )
      (blank)
      (lazy file-char
        (pattern
          (~ (chars "a-zA-Z0-9."))
        )
      )
    )
  )
)
```

```output
<Class name="A" access="public-access">
    <Function names="This" access="public-access">
        <pattern>
            <TypeId name="Char"/>
        </pattern>
        <TypeId name="StreamItem"/>
        <ArgumentDecl>
            <TypeCapture>
                <Id name="(stream)"/>
                <TypeArguments>
                    <TypeId name="TokenStream"/>
                    <TypeId name="Char"/>
                </TypeArguments>
            </TypeCapture>
        </ArgumentDecl>
        <TypeConstraint op="exactly">
            <TypeId name="StreamItem"/>
            <TypeId name="Char"/>
        </TypeConstraint>
        <Block>
            <StreamMatch>
                <StreamTransform>
                    <StreamAnd>
                        <StreamAnd>
                            <UnaryStream op="?">
                                <StreamString text="/"/>
                            </UnaryStream>
                            <UnaryStream op="+">
                                <StreamAnd>
                                    <StreamCall name="file-word"/>
                                    <StreamString text="/"/>
                                </StreamAnd>
                            </UnaryStream>
                        </StreamAnd>
                        <UnaryStream op="?">
                            <StreamCall name="file-word"/>
                        </UnaryStream>
                    </StreamAnd>
                    <Block>
                        <CallExpression>
                            <TypeId name="This"/>
                            <Argument>
                                <Id name="it"/>
                            </Argument>
                        </CallExpression>
                    </Block>
                </StreamTransform>
            </StreamMatch>
            <Declaration lazy="true" pattern="true">
                <TypeCapture>
                    <Id name="file-word"/>
                </TypeCapture>
                <StreamMatch>
                    <UnaryStream op="+">
                        <StreamCall name="file-char"/>
                    </UnaryStream>
                </StreamMatch>
            </Declaration>
            <Declaration lazy="true" pattern="true">
                <TypeCapture>
                    <Id name="file-char"/>
                </TypeCapture>
                <StreamMatch>
                    <StreamOr>
                        <StreamOr>
                            <StreamOr>
                                <StreamCharRange>
                                    <Integer value="97" unit="(char)"/>
                                    <Integer value="122" unit="(char)"/>
                                </StreamCharRange>
                                <StreamCharRange>
                                    <Integer value="65" unit="(char)"/>
                                    <Integer value="90" unit="(char)"/>
                                </StreamCharRange>
                            </StreamOr>
                            <StreamCharRange>
                                <Integer value="48" unit="(char)"/>
                                <Integer value="57" unit="(char)"/>
                            </StreamCharRange>
                        </StreamOr>
                        <StreamCharRange>
                            <Integer value="46" unit="(char)"/>
                            <Integer value="46" unit="(char)"/>
                        </StreamCharRange>
                    </StreamOr>
                </StreamMatch>
            </Declaration>
        </Block>
    </Function>
</Class>
```
