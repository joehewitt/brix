# Tasks

```yumi
do new @priority
    work()
```

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

```yumi
do new @normal
    work()
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

```yumi
do new thread @priority
    work()
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

```yumi
do new @large-stack
    work()
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

```yumi
do new thread @priority @large-stack
    work()
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

```yumi
do new @detached
    work()
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

```yumi crash
do new @large-stack @heap
    work()
```

```output
Exception: Unsupported do task annotation 'heap'
<no file:2>
    do new @large-stack @heap
    ^^^^^^^^^^^^^^^^^^^^^^^^
```

```yumi
do new thread(work)
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

```yumi
do (worker) new @priority
    work()
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

```yumi
do new
    serve(connection)
catch failure Panic
    log(failure)
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

```yumi
do new serve(connection)
catch failure OutOfBoundsError
    log(failure)
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

```yumi
do (worker) new thread
    serve(connection)

catch failure
    log(failure)
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

```yumi
do new
    serve(connection)
    catch error
        log(error)
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

```yumi
. drain = do new @priority
    work()
catch failure
    report(failure)
drain.wait()
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

```yumi
. pooled = do (worker) new step()
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
