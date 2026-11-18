# Catch

```yumi
?! 42
```

```brix
(main
  (throw 42)
)
```

```output
<Assert>
    <Integer value="42"/>
</Assert>
```

```yumi
catch read-file(path)
```

```brix
(main
  (catch (call read-file path))
)
```

```output
<CatchBlock rawResult="true">
    <CallExpression>
        <Id name="read-file"/>
        <Argument>
            <Id name="path"/>
        </Argument>
    </CallExpression>
</CatchBlock>
```

```yumi
catch? read-file(path)
```

```brix
(main
  (catch? (call read-file path))
)
```

```output
<CatchOptional>
    <CallExpression>
        <Id name="read-file"/>
        <Argument>
            <Id name="path"/>
        </Argument>
    </CallExpression>
</CatchOptional>
```

```yumi
?! AppError("bad")
    catch err IOError <- root TypeError
```

```brix
(main
  (throw (call AppError "bad") (on-error (caught (: err IOError) (: root TypeError))))
)
```

```output
<CatchBlock>
    <Assert>
        <CallExpression>
            <TypeId name="AppError"/>
            <Argument>
                <String text="bad"/>
            </Argument>
        </CallExpression>
    </Assert>
    <Catch>
        <TypeCapture>
            <Id name="err"/>
            <TypeId name="IOError"/>
        </TypeCapture>
        <TypeCapture>
            <Id name="root"/>
            <TypeId name="TypeError"/>
        </TypeCapture>
    </Catch>
</CatchBlock>
```

```yumi
. config = read-config()
    catch FileNotFound
        default-config()
```

```brix
(main
  (let config
    (call read-config
      (block
        (on-error FileNotFound
          (call default-config)
        )
      )
    )
  )
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="config"/>
    </TypeCapture>
    <CatchBlock>
        <CallExpression>
            <Id name="read-config"/>
        </CallExpression>
        <Catch>
            <TypeId name="FileNotFound"/>
            <Block>
                <CallExpression>
                    <Id name="default-config"/>
                </CallExpression>
            </Block>
        </Catch>
    </CatchBlock>
</Declaration>
```

```yumi
read-config()
    catch FileNotFound
        default-config()
    catch PermissionDenied
        permission-default()
```

```brix
(main
  (call read-config
    (block
      (on-error FileNotFound
        (call default-config)
      )
      (on-error PermissionDenied
        (call permission-default)
      )
    )
  )
)
```

```output
<CatchBlock>
    <CallExpression>
        <Id name="read-config"/>
    </CallExpression>
    <Catch>
        <TypeId name="FileNotFound"/>
        <Block>
            <CallExpression>
                <Id name="default-config"/>
            </CallExpression>
        </Block>
    </Catch>
    <Catch>
        <TypeId name="PermissionDenied"/>
        <Block>
            <CallExpression>
                <Id name="permission-default"/>
            </CallExpression>
        </Block>
    </Catch>
</CatchBlock>
```

```yumi
catch read-config()
    catch FileNotFound
        default-config()
```

```brix
(main
  (scope (catch (call read-config))
    (on-error FileNotFound
      (call default-config)
    )
  )
)
```

```output
<CatchBlock rawResult="true">
    <CallExpression>
        <Id name="read-config"/>
    </CallExpression>
    <Catch>
        <TypeId name="FileNotFound"/>
        <Block>
            <CallExpression>
                <Id name="default-config"/>
            </CallExpression>
        </Block>
    </Catch>
</CatchBlock>
```

```yumi
read-config()
    catch err FileNotFound
        default-config(err)
```

```brix
(main
  (call read-config
    (block
      (on-error (: err FileNotFound)
        (call default-config err)
      )
    )
  )
)
```

```output
<CatchBlock>
    <CallExpression>
        <Id name="read-config"/>
    </CallExpression>
    <Catch>
        <TypeCapture>
            <Id name="err"/>
            <TypeId name="FileNotFound"/>
        </TypeCapture>
        <Block>
            <CallExpression>
                <Id name="default-config"/>
                <Argument>
                    <Id name="err"/>
                </Argument>
            </CallExpression>
        </Block>
    </Catch>
</CatchBlock>
```

```yumi
catch? set-mode(1) • to false
```

```brix
(main
  (catch? (call set-mode 1 (:to false)))
)
```

```output
<CatchOptional>
    <CallExpression>
        <Id name="set-mode"/>
        <Argument>
            <Integer value="1"/>
        </Argument>
        <Argument name="to">
            <Id name="false"/>
        </Argument>
    </CallExpression>
</CatchOptional>
```

```yumi
catch? foo + bar
```

```brix
(main
  (catch? (+ foo bar))
)
```

```output
<CatchOptional>
    <BinaryExpression operator="+">
        <Id name="foo"/>
        <Id name="bar"/>
    </BinaryExpression>
</CatchOptional>
```
