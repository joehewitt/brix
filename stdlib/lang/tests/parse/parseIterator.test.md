# Iterator

```yumi
result2 in (result1 in input)
```

```brix
(main
  (in result2 (in result1 input))
)
```

```output
<Iterator>
    <In>
        <Iterator>
            <In>
                <Id name="input"/>
            </In>
            <Id name="result1"/>
        </Iterator>
    </In>
    <Id name="result2"/>
</Iterator>
```

```yumi
result in input while a when b
```

```brix
(main
  (in result input (while a) (when b))
)
```

```output
<Iterator>
    <In>
        <Id name="input"/>
        <While>
            <Id name="a"/>
            <When>
                <Id name="b"/>
            </When>
        </While>
    </In>
    <Id name="result"/>
</Iterator>
```

```yumi
in b
    c
```

```brix
(main
  (iterate (in b)
    (block
      c
    )
  )
)
```

```output
<Iterator>
    <In>
        <Id name="b"/>
    </In>
    <Id name="c"/>
</Iterator>
```

```yumi
in b
and c
    d
```

```brix
(main
  (iterate (in b) (and-for (each c))
    (block
      d
    )
  )
)
```

```output
<Iterator>
    <In>
        <Id name="b"/>
        <In>
            <TypeCapture>
                <Id name="c"/>
            </TypeCapture>
        </In>
    </In>
    <Id name="d"/>
</Iterator>
```

```yumi
in b when d
    c
```

```brix
(main
  (iterate (in b (when d))
    (block
      c
    )
  )
)
```

```output
<Iterator>
    <In>
        <Id name="b"/>
        <When>
            <Id name="d"/>
        </When>
    </In>
    <Id name="c"/>
</Iterator>
```

```yumi
in b when a
    c
or in g
    f
```

```brix
(main
  (iterate (in b (when a))
    (block
      c
    )
    (or-for (in g)
      f
    )
  )
)
```

```output
<Iterator>
    <In>
        <Id name="b"/>
        <When>
            <Id name="a"/>
        </When>
    </In>
    <Id name="c"/>
    <Iterator>
        <In>
            <Id name="g"/>
        </In>
        <Block>
            <Id name="f"/>
        </Block>
    </Iterator>
</Iterator>
```

## each autoclosure

```yumi
each in foo
```

```brix
(main
  (in each foo)
)
```

```output
<Iterator>
    <In>
        <Id name="foo"/>
    </In>
    <Id name="each"/>
</Iterator>
```

```yumi
result *> binding in input
```

```brix
(main
  (in (*> result binding) input)
)
```

```output
<Iterator>
    <In>
        <TypeCapture>
            <Id name="binding"/>
        </TypeCapture>
        <Id name="input"/>
    </In>
    <Id name="result"/>
</Iterator>
```

```yumi
each in foo when each
```

```brix
(main
  (in each foo (when each))
)
```

```output
<Iterator>
    <In>
        <Id name="foo"/>
        <When>
            <Id name="each"/>
        </When>
    </In>
    <Id name="each"/>
</Iterator>
```

```yumi
a = each in foo
```

```brix
(main
  (= a (in each foo))
)
```

```output
<AssignmentExpression op="=">
    <Id name="a"/>
    <Iterator>
        <In>
            <Id name="foo"/>
        </In>
        <Id name="each"/>
    </Iterator>
</AssignmentExpression>
```

```yumi
foo • of each in foo
```

```brix
(main
  (call foo (:of (in each foo)))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="of">
        <Iterator>
            <In>
                <Id name="foo"/>
            </In>
            <Id name="each"/>
        </Iterator>
    </Argument>
</CallExpression>
```

```yumi
foo • of each in foo when bar
```

```brix
(main
  (call foo (:of (in each foo (when bar))))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="of">
        <Iterator>
            <In>
                <Id name="foo"/>
                <When>
                    <Id name="bar"/>
                </When>
            </In>
            <Id name="each"/>
        </Iterator>
    </Argument>
</CallExpression>
```

```yumi
<* binding in input when clause
    result
```

```brix
(main
  (case-each (in binding input (when clause))
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
        <When>
            <Id name="clause"/>
        </When>
    </In>
    <Id name="result"/>
</Iterator>
```
