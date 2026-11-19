# Match with

Apply without input

```brix
(main
  (select result (| binding))
)
```

```output
<StructureMatch>
    <TypeTransform>
        <TypeCapture>
            <Id name="binding"/>
        </TypeCapture>
        <Id name="result"/>
    </TypeTransform>
</StructureMatch>
```

## Pattern helper clauses

```brix
(main
  (match input
    (with helper)
    (~ "x")
  )
)
```

```output
<Match>
    <Id name="input"/>
    <PatternHelper>
        <Id name="helper"/>
    </PatternHelper>
    <StreamMatch>
        <StreamString text="x"/>
    </StreamMatch>
</Match>
```

```brix
(main
  (match input
    (~ "x" @rollback
      1
    )
  )
)
```

```output
<Match>
    <Id name="input"/>
    <StreamMatch>
        <StreamTransform rollback="true">
            <StreamString text="x"/>
            <Block>
                <Integer value="1"/>
            </Block>
        </StreamTransform>
    </StreamMatch>
</Match>
```

```brix
(main
  (select result (~ Binding))
)
```

```output
<StreamMatch>
    <StreamTransform>
        <StreamType>
            <TypeId name="Binding"/>
        </StreamType>
        <Id name="result"/>
    </StreamTransform>
</StreamMatch>
```

```brix
(main
  (~> source \record (with helper))
)
```

```output
<BinaryExpression operator="~>">
    <Id name="source"/>
    <StreamMatch>
        <StreamCall name="record"/>
        <PatternHelper>
            <Id name="helper"/>
        </PatternHelper>
    </StreamMatch>
</BinaryExpression>
```

```brix
(main
  source
  (pipe ~> (: item .)
    (with helper)
    item
  )
)
```

```output
<Pipeline op="~>">
    <Id name="source"/>
    <StreamMatch>
        <StreamTransform>
            <StreamCapture>
                <Id name="item"/>
                <StreamAny/>
            </StreamCapture>
            <Block>
                <Id name="item"/>
            </Block>
        </StreamTransform>
        <PatternHelper>
            <Id name="helper"/>
        </PatternHelper>
    </StreamMatch>
</Pipeline>
```

```brix
(main
  (~> source \record (with (call Helper scratch)))
)
```

```output
<BinaryExpression operator="~>">
    <Id name="source"/>
    <StreamMatch>
        <StreamCall name="record"/>
        <PatternHelper>
            <CallExpression>
                <TypeId name="Helper"/>
                <Argument>
                    <Id name="scratch"/>
                </Argument>
            </CallExpression>
        </PatternHelper>
    </StreamMatch>
</BinaryExpression>
```

## Apply without binding

```brix
(main
  (= result input)
)
```

```output
<AssignmentExpression op="=">
    <Id name="result"/>
    <Id name="input"/>
</AssignmentExpression>
```

```brix
(main
  (= result
    (block
      input
    )
  )
)
```

```output
<AssignmentExpression op="=">
    <Id name="result"/>
    <Block>
        <Id name="input"/>
    </Block>
</AssignmentExpression>
```

```brix
(main
  (in result input)
)
```

```output
<Iterator>
    <In>
        <Id name="input"/>
    </In>
    <Id name="result"/>
</Iterator>
```

## Structural match with binding and input

```brix
(main
  (= (select result (| binding)) input)
)
```

```output
<Match>
    <Id name="input"/>
    <StructureMatch>
        <TypeTransform>
            <TypeCapture>
                <Id name="binding"/>
            </TypeCapture>
            <Id name="result"/>
        </TypeTransform>
    </StructureMatch>
</Match>
```

```brix
(main
  (= (select result (| binding))
    (block
      input
    )
  )
)
```

```output
<Match>
    <Block>
        <Id name="input"/>
    </Block>
    <StructureMatch>
        <TypeTransform>
            <TypeCapture>
                <Id name="binding"/>
            </TypeCapture>
            <Id name="result"/>
        </TypeTransform>
    </StructureMatch>
</Match>
```

```brix
(main
  (in (select result (| binding)) input)
)
```

```output
<Iterator>
    <In>
        <StructureMatch>
            <TypeTransform>
                <TypeCapture>
                    <Id name="binding"/>
                </TypeCapture>
                <Id name="result"/>
            </TypeTransform>
        </StructureMatch>
        <Id name="input"/>
    </In>
    <Id name="each"/>
</Iterator>
```

## Pattern match with binding and input

```brix
(main
  (= (select result (~ Binding)) input)
)
```

```output
<Match>
    <Id name="input"/>
    <StreamMatch>
        <StreamTransform>
            <StreamType>
                <TypeId name="Binding"/>
            </StreamType>
            <Id name="result"/>
        </StreamTransform>
    </StreamMatch>
</Match>
```

```brix
(main
  (= (select result (~ Binding))
    (block
      input
    )
  )
)
```

```output
<Match>
    <Block>
        <Id name="input"/>
    </Block>
    <StreamMatch>
        <StreamTransform>
            <StreamType>
                <TypeId name="Binding"/>
            </StreamType>
            <Id name="result"/>
        </StreamTransform>
    </StreamMatch>
</Match>
```

```brix
(main
  (in (select result (~ Binding)) input)
)
```

```output
<Iterator>
    <In>
        <StreamTransform>
            <StreamType>
                <TypeId name="Binding"/>
            </StreamType>
            <Id name="result"/>
        </StreamTransform>
        <Id name="input"/>
    </In>
    <Id name="each"/>
</Iterator>
```

## Structural pattern first

```brix
(main
  (case (= binding input))
)
```

```output
<Match>
    <Id name="input"/>
    <StructureMatch>
        <TypeCapture>
            <Id name="binding"/>
        </TypeCapture>
    </StructureMatch>
</Match>
```

```brix
(main
  (case-each (in binding input))
)
```

```output
<Iterator>
    <In>
        <StructureMatch>
            <TypeCapture>
                <Id name="binding"/>
            </TypeCapture>
        </StructureMatch>
        <Id name="input"/>
    </In>
    <Id name="each"/>
</Iterator>
```

```brix
(main
  (case (= binding input)
    result
  )
)
```

```output
<Match>
    <Id name="input"/>
    <StructureMatch>
        <TypeTransform>
            <TypeCapture>
                <Id name="binding"/>
            </TypeCapture>
            <Block>
                <Id name="result"/>
            </Block>
        </TypeTransform>
    </StructureMatch>
</Match>
```

```brix
(main
  (case-each (in binding input)
    result
  )
)
```

```output
<Iterator>
    <In>
        <StructureMatch>
            <TypeCapture>
                <Id name="binding"/>
            </TypeCapture>
        </StructureMatch>
        <Id name="input"/>
    </In>
    <Id name="result"/>
</Iterator>
```

## Grammar first

```brix
(main
  (~ Binding)
)
```

```output
<StreamMatch>
    <StreamType>
        <TypeId name="Binding"/>
    </StreamType>
</StreamMatch>
```

```brix
(main
  (~ Binding (of: input))
)
```

```output
<Match>
    <Id name="input"/>
    <StreamMatch>
        <StreamType>
            <TypeId name="Binding"/>
        </StreamType>
    </StreamMatch>
</Match>
```

```brix
(main
  (~ Binding (in: input))
)
```

```output
<Iterator>
    <In>
        <StreamMatch>
            <StreamType>
                <TypeId name="Binding"/>
            </StreamType>
        </StreamMatch>
        <Id name="input"/>
    </In>
    <Id name="each"/>
</Iterator>
```

```brix
(main
  (~ Binding
    result
  )
)
```

```output
<StreamMatch>
    <StreamTransform>
        <StreamType>
            <TypeId name="Binding"/>
        </StreamType>
        <Block>
            <Id name="result"/>
        </Block>
    </StreamTransform>
</StreamMatch>
```

```brix
(main
  (~ Binding (of: input)
    result
  )
)
```

```output
<Match>
    <Id name="input"/>
    <StreamMatch>
        <StreamTransform>
            <StreamType>
                <TypeId name="Binding"/>
            </StreamType>
            <Block>
                <Id name="result"/>
            </Block>
        </StreamTransform>
    </StreamMatch>
</Match>
```

```brix
(main
  (~ Binding (in: input)
    result
  )
)
```

```output
<Iterator>
    <In>
        <StreamMatch>
            <StreamTransform>
                <StreamType>
                    <TypeId name="Binding"/>
                </StreamType>
                <Block>
                    <Id name="result"/>
                </Block>
            </StreamTransform>
        </StreamMatch>
        <Id name="input"/>
    </In>
    <Id name="each"/>
</Iterator>
```

## Pattern matching

```brix
(main
  (match input
    (case binding
      result
    )
    (case binding)
  )
)
```

```output
<Match>
    <Id name="input"/>
    <StructureMatch>
        <TypeOr>
            <TypeTransform>
                <TypeCapture>
                    <Id name="binding"/>
                </TypeCapture>
                <Block>
                    <Id name="result"/>
                </Block>
            </TypeTransform>
            <TypeCapture>
                <Id name="binding"/>
            </TypeCapture>
        </TypeOr>
    </StructureMatch>
</Match>
```

```brix
(main
  (match input
    (~ Binding
      result
    )
    (~ Binding)
    (default
      (inline result)
    )
  )
)
```

```output
<Match>
    <Id name="input"/>
    <StreamMatch>
        <StreamOr>
            <StreamOr>
                <StreamTransform>
                    <StreamType>
                        <TypeId name="Binding"/>
                    </StreamType>
                    <Block>
                        <Id name="result"/>
                    </Block>
                </StreamTransform>
                <StreamType>
                    <TypeId name="Binding"/>
                </StreamType>
            </StreamOr>
            <StreamTransform>
                <Id name="result"/>
            </StreamTransform>
        </StreamOr>
    </StreamMatch>
</Match>
```

```brix
(main
  (match input
    (case? binding
      result
    )
    (case? binding)
    (default
      (inline result)
    )
  )
)
```

```output
<Match>
    <Id name="input"/>
    <StructureMatch>
        <TypeOr>
            <TypeOr>
                <TypeTransform>
                    <TypePredicate>
                        <Id name="binding"/>
                    </TypePredicate>
                    <Block>
                        <Id name="result"/>
                    </Block>
                </TypeTransform>
                <TypeTransform>
                    <TypePredicate>
                        <Id name="binding"/>
                    </TypePredicate>
                    <UnaryExpression operator="!">
                        <Id name="binding"/>
                    </UnaryExpression>
                </TypeTransform>
            </TypeOr>
            <TypeTransform>
                <Id name="result"/>
            </TypeTransform>
        </TypeOr>
    </StructureMatch>
</Match>
```

```brix
(main
  (iterate (in input)
    (block
      (case binding
        result
      )
      (case binding)
      (default
        (inline result)
      )
    )
  )
)
```

```output
<Iterator>
    <In>
        <StructureMatch>
            <TypeOr>
                <TypeOr>
                    <TypeTransform>
                        <TypeCapture>
                            <Id name="binding"/>
                        </TypeCapture>
                        <Block>
                            <Id name="result"/>
                        </Block>
                    </TypeTransform>
                    <TypeCapture>
                        <Id name="binding"/>
                    </TypeCapture>
                </TypeOr>
                <TypeTransform>
                    <Id name="result"/>
                </TypeTransform>
            </TypeOr>
        </StructureMatch>
        <Id name="input"/>
    </In>
    <Id name="each"/>
</Iterator>
```

```brix
(main
  (iterate (in input)
    (block
      (~ Binding
        result
      )
      (~ Binding)
      (default
        (inline result)
      )
    )
  )
)
```

```output
<Iterator>
    <In>
        <StreamOr>
            <StreamOr>
                <StreamTransform>
                    <StreamType>
                        <TypeId name="Binding"/>
                    </StreamType>
                    <Block>
                        <Id name="result"/>
                    </Block>
                </StreamTransform>
                <StreamType>
                    <TypeId name="Binding"/>
                </StreamType>
            </StreamOr>
            <StreamTransform>
                <Id name="result"/>
            </StreamTransform>
        </StreamOr>
        <Id name="input"/>
    </In>
    <Id name="each"/>
</Iterator>
```

```brix
(main
  (iterate (in input)
    (block
      (case? binding
        result
      )
      (case? binding)
      (default
        (inline result)
      )
    )
  )
)
```

```output
<Iterator>
    <In>
        <StructureMatch>
            <TypeOr>
                <TypeOr>
                    <TypeTransform>
                        <TypePredicate>
                            <Id name="binding"/>
                        </TypePredicate>
                        <Block>
                            <Id name="result"/>
                        </Block>
                    </TypeTransform>
                    <TypeTransform>
                        <TypePredicate>
                            <Id name="binding"/>
                        </TypePredicate>
                        <UnaryExpression operator="!">
                            <Id name="binding"/>
                        </UnaryExpression>
                    </TypeTransform>
                </TypeOr>
                <TypeTransform>
                    <Id name="result"/>
                </TypeTransform>
            </TypeOr>
        </StructureMatch>
        <Id name="input"/>
    </In>
    <Id name="each"/>
</Iterator>
```

## Function body match and iterator block header

```brix
(fn-expr + foo _
  (iterates in input)
  result
)
```

```output
<Function names="foo" access="public-access">
    <Iterator>
        <In>
            <Id name="input"/>
        </In>
        <Id name="result"/>
    </Iterator>
</Function>
```

```brix
(fn-expr + foo _
  (match input
    (~ Binding)
  )
)
```

```output
<Function names="foo" access="public-access">
    <Block>
        <Match>
            <Id name="input"/>
            <StreamMatch>
                <StreamType>
                    <TypeId name="Binding"/>
                </StreamType>
            </StreamMatch>
        </Match>
    </Block>
</Function>
```

```brix
(fn-expr + foo _
  (iterates in input)
  (~ .)
)
```

```output
<Function names="foo" access="public-access">
    <Iterator>
        <In>
            <StreamAny/>
            <Id name="input"/>
        </In>
        <Id name="each"/>
    </Iterator>
</Function>
```

## "on" variations

```brix
(main
  (on result input)
  (on (select result (| binding)) input)
  (case-each (on binding input))
  (iterate (on input)
    (block
      result
    )
  )
)

(fn-expr + foo _
  (iterates on input)
  x
)
```

```output
<Iterator>
    <On>
        <Id name="input"/>
    </On>
    <Id name="result"/>
</Iterator>
<Iterator>
    <On>
        <StructureMatch>
            <TypeTransform>
                <TypeCapture>
                    <Id name="binding"/>
                </TypeCapture>
                <Id name="result"/>
            </TypeTransform>
        </StructureMatch>
        <Id name="input"/>
    </On>
    <Id name="each"/>
</Iterator>
<Iterator>
    <On>
        <StructureMatch>
            <TypeCapture>
                <Id name="binding"/>
            </TypeCapture>
        </StructureMatch>
        <Id name="input"/>
    </On>
    <Id name="each"/>
</Iterator>
<Iterator>
    <On>
        <Id name="input"/>
    </On>
    <Id name="result"/>
</Iterator>
<Function names="foo" access="public-access">
    <Iterator>
        <On>
            <Id name="input"/>
        </On>
        <Id name="x"/>
    </Iterator>
</Function>
```
