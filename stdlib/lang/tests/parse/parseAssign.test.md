# Assign

```yumi
foo = bar
```

```brix
(main
  (= foo bar)
)
```

```output
<AssignmentExpression op="=">
    <Id name="foo"/>
    <Id name="bar"/>
</AssignmentExpression>
```

```yumi
foo =? bar
```

```brix
(main
  (=? foo bar)
)
```

```output
<AssignmentExpression op="=">
    <Id name="foo"/>
    <OptionalChain>
        <Id name="bar"/>
    </OptionalChain>
</AssignmentExpression>
```

```yumi
a = b = c
```

```brix
(main
  (= a (= b c))
)
```

```output
<AssignmentExpression op="=">
    <Id name="a"/>
    <AssignmentExpression op="=">
        <Id name="b"/>
        <Id name="c"/>
    </AssignmentExpression>
</AssignmentExpression>
```

```yumi
foo += bar
```

```brix
(main
  (+= foo bar)
)
```

```output
<AssignmentExpression op="+=">
    <Id name="foo"/>
    <Id name="bar"/>
</AssignmentExpression>
```

```yumi
foo = bar += bar
```

```brix
(main
  (= foo (+= bar bar))
)
```

```output
<AssignmentExpression op="=">
    <Id name="foo"/>
    <AssignmentExpression op="+=">
        <Id name="bar"/>
        <Id name="bar"/>
    </AssignmentExpression>
</AssignmentExpression>
```

```yumi
foo.bar = baz
```

```brix
(main
  (= (. foo bar) baz)
)
```

```output
<AssignmentExpression op="=">
    <Get name="bar">
        <Id name="foo"/>
    </Get>
    <Id name="baz"/>
</AssignmentExpression>
```

```yumi
foo.bar.baz = z
```

```brix
(main
  (= (. foo bar baz) z)
)
```

```output
<AssignmentExpression op="=">
    <Get name="baz">
        <Get name="bar">
            <Id name="foo"/>
        </Get>
    </Get>
    <Id name="z"/>
</AssignmentExpression>
```

```yumi
foo[0] = baz
```

```brix
(main
  (= ([] foo 0) baz)
)
```

```output
<AssignmentExpression op="=">
    <Index>
        <Id name="foo"/>
        <Integer value="0"/>
    </Index>
    <Id name="baz"/>
</AssignmentExpression>
```

```yumi
foo =
    bar
```

```brix
(main
  (= foo
    (block
      bar
    )
  )
)
```

```output
<AssignmentExpression op="=">
    <Id name="foo"/>
    <Block>
        <Id name="bar"/>
    </Block>
</AssignmentExpression>
```

```yumi
foo <-* bar
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

```yumi
foo <-* ~ "f" in "ff"
```

```brix
(main
  (<-* foo (~ "f" (in: "ff")))
)
```

```output
<For>
    <In>
        <StreamMatch>
            <StreamString text="f"/>
        </StreamMatch>
        <String text="ff"/>
    </In>
    <BinaryExpression operator="<-write">
        <Id name="foo"/>
        <Id name="each"/>
    </BinaryExpression>
</For>
```

```yumi
x = foo =
    bar
```

```brix
(main
  (= x
    (= foo
      (block
        bar
      )
    )
  )
)
```

```output
<AssignmentExpression op="=">
    <Id name="x"/>
    <AssignmentExpression op="=">
        <Id name="foo"/>
        <Block>
            <Id name="bar"/>
        </Block>
    </AssignmentExpression>
</AssignmentExpression>
```

```yumi
x = -- foo
```

```output
Exception: A print is a step and gives no value; print on a line of its own, or after ":" as a one-step body
<no file:2>
    x = -- foo
          ^
```

```yumi
x, y = b
```

```brix
(main
  (= (tuple x y) b)
)
```

```output
<AssignmentExpression op="=">
    <Tuple>
        <Id name="x"/>
        <Id name="y"/>
    </Tuple>
    <Id name="b"/>
</AssignmentExpression>
```
