# Match

Inline match

```yumi
a if b
```

```brix
(main
  (only-if a (if b))
)
```

```output
<BinaryExpression operator="if">
    <Id name="a"/>
    <Id name="b"/>
</BinaryExpression>
```

```yumi
if b then a else d
```

```brix
(main
  (if-expr b a d)
)
```

```output
<BinaryExpression operator="else">
    <BinaryExpression operator="if">
        <Id name="a"/>
        <Id name="b"/>
    </BinaryExpression>
    <Id name="d"/>
</BinaryExpression>
```

```yumi
| foo
    c2
?? c1
```

```output
<StructureMatch>
    <TypeOr>
        <TypeTransform>
            <TypeCapture>
                <Id name="foo"/>
            </TypeCapture>
            <Block>
                <Id name="c2"/>
            </Block>
        </TypeTransform>
        <TypeTransform>
            <Id name="c1"/>
        </TypeTransform>
    </TypeOr>
</StructureMatch>
```

```yumi
|? c2
?? c1
```

```brix
(main
  (case? c2)
  (default
    (inline c1)
  )
)
```

```output
<StructureMatch>
    <TypeOr>
        <TypeTransform>
            <TypePredicate>
                <Id name="c2"/>
            </TypePredicate>
            <UnaryExpression operator="!">
                <Id name="c2"/>
            </UnaryExpression>
        </TypeTransform>
        <TypeTransform>
            <Id name="c1"/>
        </TypeTransform>
    </TypeOr>
</StructureMatch>
```

```yumi
|? foo
    c3
| bar
    c2
|? c4
?? c1
```

```output
<StructureMatch>
    <TypeOr>
        <TypeOr>
            <TypeOr>
                <TypeTransform>
                    <TypePredicate>
                        <Id name="foo"/>
                    </TypePredicate>
                    <Block>
                        <Id name="c3"/>
                    </Block>
                </TypeTransform>
                <TypeTransform>
                    <TypeCapture>
                        <Id name="bar"/>
                    </TypeCapture>
                    <Block>
                        <Id name="c2"/>
                    </Block>
                </TypeTransform>
            </TypeOr>
            <TypeTransform>
                <TypePredicate>
                    <Id name="c4"/>
                </TypePredicate>
                <UnaryExpression operator="!">
                    <Id name="c4"/>
                </UnaryExpression>
            </TypeTransform>
        </TypeOr>
        <TypeTransform>
            <Id name="c1"/>
        </TypeTransform>
    </TypeOr>
</StructureMatch>
```

## Anonymous match cases

```yumi
|? missing-email
    ..invalid-email
??
    ..valid(email, password)
```

```brix
(main
  (case? missing-email
    ..invalid-email
  )
  (default
    (call ..valid email password)
  )
)
```

```output
<StructureMatch>
    <TypeOr>
        <TypeTransform>
            <TypePredicate>
                <Id name="missing-email"/>
            </TypePredicate>
            <Block>
                <AnonymousCase name="invalid-email"/>
            </Block>
        </TypeTransform>
        <TypeTransform>
            <Block>
                <AnonymousCase name="valid">
                    <Id name="email"/>
                    <Id name="password"/>
                </AnonymousCase>
            </Block>
        </TypeTransform>
    </TypeOr>
</StructureMatch>
```

```yumi
~ "ok"
    ..ok
??
    ..failed(code)
```

```brix
(main
  (~ "ok"
    ..ok
  )
  (default
    (call ..failed code)
  )
)
```

```output
<StreamMatch>
    <StreamOr>
        <StreamTransform>
            <StreamString text="ok"/>
            <Block>
                <AnonymousCase name="ok"/>
            </Block>
        </StreamTransform>
        <StreamTransform>
            <Block>
                <AnonymousCase name="failed">
                    <Id name="code"/>
                </AnonymousCase>
            </Block>
        </StreamTransform>
    </StreamOr>
</StreamMatch>
```

```yumi
. result = match input
    |? missing-email
        ..invalid-email
    ??
        ..valid(email, password)
```

```brix
(main
  (let result
    (match input
      (case? missing-email
        ..invalid-email
      )
      (default
        (call ..valid email password)
      )
    )
  )
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="result"/>
    </TypeCapture>
    <Match>
        <Id name="input"/>
        <StructureMatch>
            <TypeOr>
                <TypeTransform>
                    <TypePredicate>
                        <Id name="missing-email"/>
                    </TypePredicate>
                    <Block>
                        <AnonymousCase name="invalid-email" owner="result"/>
                    </Block>
                </TypeTransform>
                <TypeTransform>
                    <Block>
                        <AnonymousCase name="valid" owner="result">
                            <Id name="email"/>
                            <Id name="password"/>
                        </AnonymousCase>
                    </Block>
                </TypeTransform>
            </TypeOr>
        </StructureMatch>
    </Match>
</Declaration>
```

```yumi
.. login-submission = on submitted
    |? !email-box.text
        ..invalid-email
    |? !password-box.text
        ..invalid-password
    ??
        ..valid(email-box.text, password-box.text)
```

```brix
(main
  (lazy login-submission
    (iterate (on submitted)
      (block
        (case? (not (. email-box text))
          ..invalid-email
        )
        (case? (not (. password-box text))
          ..invalid-password
        )
        (default
          (call ..valid (. email-box text) (. password-box text))
        )
      )
    )
  )
)
```

```output
<Declaration lazy="true">
    <TypeCapture>
        <Id name="login-submission"/>
    </TypeCapture>
    <Iterator>
        <On>
            <StructureMatch>
                <TypeOr>
                    <TypeOr>
                        <TypeTransform>
                            <TypePredicate>
                                <UnaryExpression operator="!">
                                    <Get name="text">
                                        <Id name="email-box"/>
                                    </Get>
                                </UnaryExpression>
                            </TypePredicate>
                            <Block>
                                <AnonymousCase name="invalid-email" owner="login-submission"/>
                            </Block>
                        </TypeTransform>
                        <TypeTransform>
                            <TypePredicate>
                                <UnaryExpression operator="!">
                                    <Get name="text">
                                        <Id name="password-box"/>
                                    </Get>
                                </UnaryExpression>
                            </TypePredicate>
                            <Block>
                                <AnonymousCase name="invalid-password" owner="login-submission"/>
                            </Block>
                        </TypeTransform>
                    </TypeOr>
                    <TypeTransform>
                        <Block>
                            <AnonymousCase name="valid" owner="login-submission">
                                <Get name="text">
                                    <Id name="email-box"/>
                                </Get>
                                <Get name="text">
                                    <Id name="password-box"/>
                                </Get>
                            </AnonymousCase>
                        </Block>
                    </TypeTransform>
                </TypeOr>
            </StructureMatch>
            <Id name="submitted"/>
        </On>
        <Id name="each"/>
    </Iterator>
</Declaration>
```

```yumi
. event = match input
    ~ "ok"
        ..ok
    ??
        ..failed(code)
```

```brix
(main
  (let event
    (match input
      (~ "ok"
        ..ok
      )
      (default
        (call ..failed code)
      )
    )
  )
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="event"/>
    </TypeCapture>
    <Match>
        <Id name="input"/>
        <StreamMatch>
            <StreamOr>
                <StreamTransform>
                    <StreamString text="ok"/>
                    <Block>
                        <AnonymousCase name="ok" owner="event"/>
                    </Block>
                </StreamTransform>
                <StreamTransform>
                    <Block>
                        <AnonymousCase name="failed" owner="event">
                            <Id name="code"/>
                        </AnonymousCase>
                    </Block>
                </StreamTransform>
            </StreamOr>
        </StreamMatch>
    </Match>
</Declaration>
```

```yumi
|? foo
```

```brix
(main
  (case? foo)
)
```

```output
<StructureMatch>
    <TypeTransform>
        <TypePredicate>
            <Id name="foo"/>
        </TypePredicate>
        <UnaryExpression operator="!">
            <Id name="foo"/>
        </UnaryExpression>
    </TypeTransform>
</StructureMatch>
```

```yumi
|? foo
    c2
```

```brix
(main
  (case? foo
    c2
  )
)
```

```output
<StructureMatch>
    <TypeTransform>
        <TypePredicate>
            <Id name="foo"/>
        </TypePredicate>
        <Block>
            <Id name="c2"/>
        </Block>
    </TypeTransform>
</StructureMatch>
```

```yumi
|? foo
    c2
?? c1
```

```brix
(main
  (case? foo
    c2
  )
  (default
    (inline c1)
  )
)
```

```output
<StructureMatch>
    <TypeOr>
        <TypeTransform>
            <TypePredicate>
                <Id name="foo"/>
            </TypePredicate>
            <Block>
                <Id name="c2"/>
            </Block>
        </TypeTransform>
        <TypeTransform>
            <Id name="c1"/>
        </TypeTransform>
    </TypeOr>
</StructureMatch>
```

```yumi
| foo
    c2
?? c1
```

```output
<StructureMatch>
    <TypeOr>
        <TypeTransform>
            <TypeCapture>
                <Id name="foo"/>
            </TypeCapture>
            <Block>
                <Id name="c2"/>
            </Block>
        </TypeTransform>
        <TypeTransform>
            <Id name="c1"/>
        </TypeTransform>
    </TypeOr>
</StructureMatch>
```

## Other combinations

```yumi
|? foo
    y
    .. y = 2
|? foo
    y
    .. y = 2
??
    y
    .. y = 2
```

```brix
(main
  (case? foo
    y
    (lazy y 2)
  )
  (case? foo
    y
    (lazy y 2)
  )
  (default
    y
    (lazy y 2)
  )
)
```

```output
<StructureMatch>
    <TypeOr>
        <TypeOr>
            <TypeTransform>
                <TypePredicate>
                    <Id name="foo"/>
                </TypePredicate>
                <Block>
                    <Id name="y"/>
                    <Declaration lazy="true">
                        <TypeCapture>
                            <Id name="y"/>
                        </TypeCapture>
                        <Integer value="2"/>
                    </Declaration>
                </Block>
            </TypeTransform>
            <TypeTransform>
                <TypePredicate>
                    <Id name="foo"/>
                </TypePredicate>
                <Block>
                    <Id name="y"/>
                    <Declaration lazy="true">
                        <TypeCapture>
                            <Id name="y"/>
                        </TypeCapture>
                        <Integer value="2"/>
                    </Declaration>
                </Block>
            </TypeTransform>
        </TypeOr>
        <TypeTransform>
            <Block>
                <Id name="y"/>
                <Declaration lazy="true">
                    <TypeCapture>
                        <Id name="y"/>
                    </TypeCapture>
                    <Integer value="2"/>
                </Declaration>
            </Block>
        </TypeTransform>
    </TypeOr>
</StructureMatch>
```

```yumi
|? first
    foo • at bar
```

```brix
(main
  (case? first
    (call foo (:at bar))
  )
)
```

```output
<StructureMatch>
    <TypeTransform>
        <TypePredicate>
            <Id name="first"/>
        </TypePredicate>
        <Block>
            <CallExpression>
                <Id name="foo"/>
                <Argument name="at">
                    <Id name="bar"/>
                </Argument>
            </CallExpression>
        </Block>
    </TypeTransform>
</StructureMatch>
```

```yumi
| a Bar
    foo
| a Bar!
    foo
| a Bar~
    foo
```

```output
<StructureMatch>
    <TypeOr>
        <TypeOr>
            <TypeTransform>
                <TypeCapture>
                    <Id name="a"/>
                    <TypeId name="Bar"/>
                </TypeCapture>
                <Block>
                    <Id name="foo"/>
                </Block>
            </TypeTransform>
            <TypeTransform>
                <TypeCapture>
                    <Id name="a"/>
                    <TypeMatch op="exactly">
                        <TypeId name="Bar"/>
                    </TypeMatch>
                </TypeCapture>
                <Block>
                    <Id name="foo"/>
                </Block>
            </TypeTransform>
        </TypeOr>
        <TypeTransform>
            <TypeCapture>
                <Id name="a"/>
                <TypeMatch op="similar">
                    <TypeId name="Bar"/>
                </TypeMatch>
            </TypeCapture>
            <Block>
                <Id name="foo"/>
            </Block>
        </TypeTransform>
    </TypeOr>
</StructureMatch>
```

## Unionized matches

```yumi
||= foo
    1
||? foo
    1
|| Type
    1
```

```output
<StructureMatch>
    <TypeOr>
        <TypeOr>
            <TypeTransform isUnion="true">
                <TypeEquality>
                    <Id name="foo"/>
                </TypeEquality>
                <Block>
                    <Integer value="1"/>
                </Block>
            </TypeTransform>
            <TypeTransform isUnion="true">
                <TypePredicate>
                    <Id name="foo"/>
                </TypePredicate>
                <Block>
                    <Integer value="1"/>
                </Block>
            </TypeTransform>
        </TypeOr>
        <TypeTransform isUnion="true">
            <TypeId name="Type"/>
            <Block>
                <Integer value="1"/>
            </Block>
        </TypeTransform>
    </TypeOr>
</StructureMatch>
```

```yumi crash
. found = match? n
    |= 1
        "one"
    ??
        "other"
```

```output
Exception: "match?" differs from "match" only where no arm is taken, and this default arm is taken wherever no arm before it is; write "match"
<no file:5>
        ??
        ^
```

```yumi crash
do match? n
    |= 1
        step()
    ?! Panic("none")
```

```output
Exception: "match?" differs from "match" only where no arm is taken, and this arm raises wherever no arm before it is taken; write "match"
<no file:5>
        ?! Panic("none")
        ^^^^^^^^^^^^^^^
```
