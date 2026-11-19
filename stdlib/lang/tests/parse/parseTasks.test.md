# Tasks

```brix
(main
  (do-new @priority
    (call work)
  )
)
```

```output
<DoTask scheduling="priority">
    <Block>
        <CallExpression>
            <Id name="work"/>
        </CallExpression>
    </Block>
</DoTask>
```

```brix
(main
  (do-new @normal
    (call work)
  )
)
```

```output
<DoTask>
    <Block>
        <CallExpression>
            <Id name="work"/>
        </CallExpression>
    </Block>
</DoTask>
```

```brix
(main
  (do-new-thread @priority
    (call work)
  )
)
```

```output
<DoTask parallel="true" scheduling="priority">
    <Block>
        <CallExpression>
            <Id name="work"/>
        </CallExpression>
    </Block>
</DoTask>
```

```brix
(main
  (do-new @large-stack
    (call work)
  )
)
```

```output
<DoTask stack="large">
    <Block>
        <CallExpression>
            <Id name="work"/>
        </CallExpression>
    </Block>
</DoTask>
```

```brix
(main
  (do-new-thread @priority @large-stack
    (call work)
  )
)
```

```output
<DoTask parallel="true" scheduling="priority" stack="large">
    <Block>
        <CallExpression>
            <Id name="work"/>
        </CallExpression>
    </Block>
</DoTask>
```

```brix
(main
  (do-new @detached
    (call work)
  )
)
```

```output
<DoTask detached="true">
    <Block>
        <CallExpression>
            <Id name="work"/>
        </CallExpression>
    </Block>
</DoTask>
```

```brix
(main
  (do-new
    (inline (call thread work))
  )
)
```

```output
<DoTask>
    <CallExpression>
        <Id name="thread"/>
        <Argument>
            <Id name="work"/>
        </Argument>
    </CallExpression>
</DoTask>
```

```brix
(main
  (do-new (worker: worker) @priority
    (call work)
  )
)
```

```output
<DoTask scheduling="priority">
    <worker>
        <Id name="worker"/>
    </worker>
    <Block>
        <CallExpression>
            <Id name="work"/>
        </CallExpression>
    </Block>
</DoTask>
```

```brix
(main
  (do-new
    (call serve connection)
    (on-failure (: failure Panic)
      (call log failure)
    )
  )
)
```

```output
<DoTask>
    <Block>
        <CallExpression>
            <Id name="serve"/>
            <Argument>
                <Id name="connection"/>
            </Argument>
        </CallExpression>
    </Block>
    <Catch>
        <TypeCapture>
            <Id name="failure"/>
            <TypeId name="Panic"/>
        </TypeCapture>
        <Block>
            <CallExpression>
                <Id name="log"/>
                <Argument>
                    <Id name="failure"/>
                </Argument>
            </CallExpression>
        </Block>
    </Catch>
</DoTask>
```

```brix
(main
  (do-new
    (inline (call serve connection))
    (on-failure (: failure OutOfBoundsError)
      (call log failure)
    )
  )
)
```

```output
<DoTask>
    <CallExpression>
        <Id name="serve"/>
        <Argument>
            <Id name="connection"/>
        </Argument>
    </CallExpression>
    <Catch>
        <TypeCapture>
            <Id name="failure"/>
            <TypeId name="OutOfBoundsError"/>
        </TypeCapture>
        <Block>
            <CallExpression>
                <Id name="log"/>
                <Argument>
                    <Id name="failure"/>
                </Argument>
            </CallExpression>
        </Block>
    </Catch>
</DoTask>
```

```brix
(main
  (do-new-thread (worker: worker)
    (call serve connection)
    (on-failure failure
      (call log failure)
    )
  )
)
```

```output
<DoTask parallel="true">
    <worker>
        <Id name="worker"/>
    </worker>
    <Block>
        <CallExpression>
            <Id name="serve"/>
            <Argument>
                <Id name="connection"/>
            </Argument>
        </CallExpression>
    </Block>
    <Catch>
        <TypeCapture>
            <Id name="failure"/>
        </TypeCapture>
        <Block>
            <CallExpression>
                <Id name="log"/>
                <Argument>
                    <Id name="failure"/>
                </Argument>
            </CallExpression>
        </Block>
    </Catch>
</DoTask>
```

```brix
(main
  (do-new
    (call serve connection)
    (on-error error
      (call log error)
    )
  )
)
```

```output
<DoTask>
    <Block>
        <CatchBlock>
            <CallExpression>
                <Id name="serve"/>
                <Argument>
                    <Id name="connection"/>
                </Argument>
            </CallExpression>
            <Catch>
                <TypeCapture>
                    <Id name="error"/>
                </TypeCapture>
                <Block>
                    <CallExpression>
                        <Id name="log"/>
                        <Argument>
                            <Id name="error"/>
                        </Argument>
                    </CallExpression>
                </Block>
            </Catch>
        </CatchBlock>
    </Block>
</DoTask>
```

```brix
(main
  (let drain
    (do-new @priority
      (call work)
      (on-failure failure
        (call report failure)
      )
    )
  )
  (call (. drain wait))
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="drain"/>
    </TypeCapture>
    <DoTask scheduling="priority">
        <Block>
            <CallExpression>
                <Id name="work"/>
            </CallExpression>
        </Block>
        <Catch>
            <TypeCapture>
                <Id name="failure"/>
            </TypeCapture>
            <Block>
                <CallExpression>
                    <Id name="report"/>
                    <Argument>
                        <Id name="failure"/>
                    </Argument>
                </CallExpression>
            </Block>
        </Catch>
    </DoTask>
</Declaration>
<CallExpression>
    <Get name="wait">
        <Id name="drain"/>
    </Get>
</CallExpression>
```

```brix
(main
  (let pooled
    (do-new (worker: worker)
      (inline (call step))
    )
  )
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="pooled"/>
    </TypeCapture>
    <DoTask>
        <worker>
            <Id name="worker"/>
        </worker>
        <CallExpression>
            <Id name="step"/>
        </CallExpression>
    </DoTask>
</Declaration>
```
