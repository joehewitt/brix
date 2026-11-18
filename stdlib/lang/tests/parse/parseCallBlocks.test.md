# Call blocks

```yumi
foo • at bar
    ?! err
```

```brix
(main
  (call foo (:at bar)
    (block
      (throw err)
    )
  )
)
```

```output
<Block>
    <Assert>
        <Id name="err"/>
    </Assert>
    <Declaration lazy="true">
        <TypeCapture>
            <Id name="it"/>
        </TypeCapture>
        <CallExpression>
            <Id name="foo"/>
            <Argument name="at">
                <Id name="bar"/>
            </Argument>
        </CallExpression>
    </Declaration>
    <Id name="it"/>
</Block>
```

```yumi
Map
    •? "strokeWidth" width
    •? (property) value
```

```brix
(main
  (call Map
    (block
      (entry? "strokeWidth" width)
      (entry? property value)
    )
  )
)
```

```output
<CallExpression>
    <TypeId name="Map"/>
    <Argument optional="true">
        <Tuple>
            <String text="strokeWidth"/>
            <Id name="width"/>
        </Tuple>
    </Argument>
    <Argument optional="true">
        <Tuple>
            <Id name="property"/>
            <Id name="value"/>
        </Tuple>
    </Argument>
</CallExpression>
```

```yumi
a • of b in c when c
```

```brix
(main
  (call a (:of (in b c (when c))))
)
```

```output
<CallExpression>
    <Id name="a"/>
    <Argument name="of">
        <Iterator>
            <In>
                <Id name="c"/>
                <When>
                    <Id name="c"/>
                </When>
            </In>
            <Id name="b"/>
        </Iterator>
    </Argument>
</CallExpression>
```

```yumi
call
    a
```

```brix
(main
  (call call
    (block
      a
    )
  )
)
```

```output
<CallExpression>
    <Id name="call"/>
    <Argument>
        <Block>
            <Id name="a"/>
        </Block>
    </Argument>
</CallExpression>
```

```yumi
call: a
```

```brix
(main
  (call call (inline a))
)
```

```output
<CallExpression>
    <Id name="call"/>
    <Argument>
        <Id name="a"/>
    </Argument>
</CallExpression>
```

```yumi
call'Foo
    a
```

```brix
(main
  (call (of call Foo)
    (block
      a
    )
  )
)
```

```output
<CallExpression>
    <Id name="call"/>
    <params>
        <TypeId name="Foo"/>
    </params>
    <Argument>
        <Block>
            <Id name="a"/>
        </Block>
    </Argument>
</CallExpression>
```

```yumi
call • with
    a
```

```brix
(main
  (call call
    (:with
      (block
        a
      )
    )
  )
)
```

```output
<CallExpression>
    <Id name="call"/>
    <Argument name="with">
        <Block>
            <Id name="a"/>
        </Block>
    </Argument>
</CallExpression>
```

```yumi
call • with a • at
    b
```

```brix
(main
  (call call (:with a)
    (:at
      (block
        b
      )
    )
  )
)
```

```output
<CallExpression>
    <Id name="call"/>
    <Argument name="with">
        <Id name="a"/>
    </Argument>
    <Argument name="at">
        <Block>
            <Id name="b"/>
        </Block>
    </Argument>
</CallExpression>
```

```yumi
call
    • with this
        | .alpha
            "A"
        ??
            "B"
```

```brix
(main
  (call call
    (block
      (:with
        (call this
          (block
            (case .alpha
              "A"
            )
            (default
              "B"
            )
          )
        )
      )
    )
  )
)
```

```output
<CallExpression>
    <Id name="call"/>
    <Argument name="with">
        <CallExpression>
            <Id name="this"/>
            <Argument>
                <Block>
                    <StructureMatch>
                        <TypeOr>
                            <TypeTransform>
                                <TypeCase name="alpha"/>
                                <Block>
                                    <String text="A"/>
                                </Block>
                            </TypeTransform>
                            <TypeTransform>
                                <Block>
                                    <String text="B"/>
                                </Block>
                            </TypeTransform>
                        </TypeOr>
                    </StructureMatch>
                </Block>
            </Argument>
        </CallExpression>
    </Argument>
</CallExpression>
```

```yumi
call(a) • with
    b
```

```brix
(main
  (call call a
    (:with
      (block
        b
      )
    )
  )
)
```

```output
<CallExpression>
    <Id name="call"/>
    <Argument>
        <Id name="a"/>
    </Argument>
    <Argument name="with">
        <Block>
            <Id name="b"/>
        </Block>
    </Argument>
</CallExpression>
```

```yumi
call
    .. a = b
```

```brix
(main
  (scope call
    (lazy a b)
  )
)
```

```output
<Block>
    <Declaration lazy="true">
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
        <Id name="b"/>
    </Declaration>
    <Declaration lazy="true">
        <TypeCapture>
            <Id name="it"/>
        </TypeCapture>
        <Id name="call"/>
    </Declaration>
    <Id name="it"/>
</Block>
```

```yumi
call
    b
    .. a = b
```

```brix
(main
  (call call
    (block
      b
      (lazy a b)
    )
  )
)
```

```output
<CallExpression>
    <Id name="call"/>
    <Argument>
        <Block>
            <Id name="b"/>
            <Declaration lazy="true">
                <TypeCapture>
                    <Id name="a"/>
                </TypeCapture>
                <Id name="b"/>
            </Declaration>
        </Block>
    </Argument>
</CallExpression>
```

```yumi
call • of ex • with
    indented
```

```brix
(main
  (call call (:of ex)
    (:with
      (block
        indented
      )
    )
  )
)
```

```output
<CallExpression>
    <Id name="call"/>
    <Argument name="of">
        <Id name="ex"/>
    </Argument>
    <Argument name="with">
        <Block>
            <Id name="indented"/>
        </Block>
    </Argument>
</CallExpression>
```

```yumi
a = :: a =>
    indented
```

```brix
(main
  (= a
    (=> (a)
      (block
        indented
      )
    )
  )
)
```

```output
<AssignmentExpression op="=">
    <Id name="a"/>
    <Function names="(closure)" access="private-access">
        <ArgumentDecl>
            <TypeCapture>
                <Id name="a"/>
            </TypeCapture>
        </ArgumentDecl>
        <Block>
            <Id name="indented"/>
        </Block>
    </Function>
</AssignmentExpression>
```

```yumi
. a = call • of :: a =>
    indented
```

```brix
(main
  (let a
    (call call
      (:of
        (=> (a)
          (block
            indented
          )
        )
      )
    )
  )
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="a"/>
    </TypeCapture>
    <CallExpression>
        <Id name="call"/>
        <Argument name="of">
            <Function names="(closure)" access="private-access">
                <ArgumentDecl>
                    <TypeCapture>
                        <Id name="a"/>
                    </TypeCapture>
                </ArgumentDecl>
                <Block>
                    <Id name="indented"/>
                </Block>
            </Function>
        </Argument>
    </CallExpression>
</Declaration>
```

```yumi
a = call • of :: a =>
    indented
```

```brix
(main
  (= a
    (call call
      (:of
        (=> (a)
          (block
            indented
          )
        )
      )
    )
  )
)
```

```output
<AssignmentExpression op="=">
    <Id name="a"/>
    <CallExpression>
        <Id name="call"/>
        <Argument name="of">
            <Function names="(closure)" access="private-access">
                <ArgumentDecl>
                    <TypeCapture>
                        <Id name="a"/>
                    </TypeCapture>
                </ArgumentDecl>
                <Block>
                    <Id name="indented"/>
                </Block>
            </Function>
        </Argument>
    </CallExpression>
</AssignmentExpression>
```

## Pronouns

```yumi
foo • at bar
```

```brix
(main
  (call foo (:at bar))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="at">
        <Id name="bar"/>
    </Argument>
</CallExpression>
```

```yumi
join(this) • with ", "
```

```brix
(main
  (call join this (:with ", "))
)
```

```output
<CallExpression>
    <Id name="join"/>
    <Argument>
        <Id name="this"/>
    </Argument>
    <Argument name="with">
        <String text=", "/>
    </Argument>
</CallExpression>
```

```yumi
this += foo • at ", "
```

```brix
(main
  (+= this (call foo (:at ", ")))
)
```

```output
<AssignmentExpression op="+=">
    <Id name="this"/>
    <CallExpression>
        <Id name="foo"/>
        <Argument name="at">
            <String text=", "/>
        </Argument>
    </CallExpression>
</AssignmentExpression>
```

## Nested declarations and asserts

```yumi
foo(a)
    .. a = 5
```

```brix
(main
  (call foo a
    (block
      (lazy a 5)
    )
  )
)
```

```output
<Block>
    <Declaration lazy="true">
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
        <Integer value="5"/>
    </Declaration>
    <Declaration lazy="true">
        <TypeCapture>
            <Id name="it"/>
        </TypeCapture>
        <CallExpression>
            <Id name="foo"/>
            <Argument>
                <Id name="a"/>
            </Argument>
        </CallExpression>
    </Declaration>
    <Id name="it"/>
</Block>
```

```yumi
foo(a)
    .. a = 5
    ?! foo if bar
```

```brix
(main
  (call foo a
    (block
      (lazy a 5)
      (throw foo (if bar))
    )
  )
)
```

```output
<Block>
    <Declaration lazy="true">
        <TypeCapture>
            <Id name="a"/>
        </TypeCapture>
        <Integer value="5"/>
    </Declaration>
    <Assert>
        <TypePredicate>
            <Id name="bar"/>
        </TypePredicate>
        <Id name="foo"/>
    </Assert>
    <Declaration lazy="true">
        <TypeCapture>
            <Id name="it"/>
        </TypeCapture>
        <CallExpression>
            <Id name="foo"/>
            <Argument>
                <Id name="a"/>
            </Argument>
        </CallExpression>
    </Declaration>
    <Id name="it"/>
</Block>
```

```yumi
call • where :: foo Foo => bar
```

```brix
(main
  (call call (:where (=> ((: foo Foo)) bar)))
)
```

```output
<CallExpression>
    <Id name="call"/>
    <Argument name="where">
        <Function names="(closure)" access="private-access">
            <ArgumentDecl>
                <TypeCapture>
                    <Id name="foo"/>
                    <TypeId name="Foo"/>
                </TypeCapture>
            </ArgumentDecl>
            <Id name="bar"/>
        </Function>
    </Argument>
</CallExpression>
```

## Indented arguments

```yumi
foo
    • name
```

```brix
(main
  (call foo
    (block
      (:name)
    )
  )
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="name">
        <Id name="name"/>
    </Argument>
</CallExpression>
```

```yumi
foo
    • name true
```

```brix
(main
  (call foo
    (block
      (:name true)
    )
  )
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="name">
        <Id name="true"/>
    </Argument>
</CallExpression>
```

```yumi
partition
    • until 1
```

```brix
(main
  (call partition
    (block
      (:until 1)
    )
  )
)
```

```output
<CallExpression>
    <Id name="partition"/>
    <Argument name="until">
        <Integer value="1"/>
    </Argument>
</CallExpression>
```

```yumi
partition
    - a
    - b
```

```brix
(main
  (call partition
    (block
      (arg a)
      (arg b)
    )
  )
)
```

```output
<CallExpression>
    <Id name="partition"/>
    <Argument>
        <Id name="a"/>
    </Argument>
    <Argument>
        <Id name="b"/>
    </Argument>
</CallExpression>
```

```yumi
partition
    - a
    - b
```

```brix
(main
  (call partition
    (block
      (arg a)
      (arg b)
    )
  )
)
```

```output
<CallExpression>
    <Id name="partition"/>
    <Argument>
        <Id name="a"/>
    </Argument>
    <Argument>
        <Id name="b"/>
    </Argument>
</CallExpression>
```

```yumi
find • is recursive
    "foo"
```

```brix
(main
  (call find (:is-recursive true)
    (block
      "foo"
    )
  )
)
```

```output
<CallExpression>
    <Id name="find"/>
    <Argument name="is-recursive">
        <Id name="true"/>
    </Argument>
    <Argument>
        <Block>
            <String text="foo"/>
        </Block>
    </Argument>
</CallExpression>
```

```yumi
find("foo")
    • is recursive
```

```brix
(main
  (call find "foo"
    (block
      (:is-recursive true)
    )
  )
)
```

```output
<CallExpression>
    <Id name="find"/>
    <Argument>
        <String text="foo"/>
    </Argument>
    <Argument name="is-recursive">
        <Id name="true"/>
    </Argument>
</CallExpression>
```

```yumi
find
    - "foo"
    • is recursive
```

```brix
(main
  (call find
    (block
      (arg "foo")
      (:is-recursive true)
    )
  )
)
```

```output
<CallExpression>
    <Id name="find"/>
    <Argument>
        <String text="foo"/>
    </Argument>
    <Argument name="is-recursive">
        <Id name="true"/>
    </Argument>
</CallExpression>
```

## Bracketed calls

```yumi
(foo() • at bar)
```

```brix
(main
  (call foo (:at bar))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="at">
        <Id name="bar"/>
    </Argument>
</CallExpression>
```

## Closure as preoosition argument

```yumi
fn • from arg • using :: binding => result
```

```brix
(main
  (call fn (:from arg) (:using (=> (binding) result)))
)
```

```output
<CallExpression>
    <Id name="fn"/>
    <Argument name="from">
        <Id name="arg"/>
    </Argument>
    <Argument name="using">
        <Function names="(closure)" access="private-access">
            <ArgumentDecl>
                <TypeCapture>
                    <Id name="binding"/>
                </TypeCapture>
            </ArgumentDecl>
            <Id name="result"/>
        </Function>
    </Argument>
</CallExpression>
```
