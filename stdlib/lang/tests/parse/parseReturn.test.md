# Return

```brix
(main
  (return foo)
)
```

```output
<Return>
    <Id name="foo"/>
</Return>
```

```brix
(main
  (yield foo)
)
```

```output
<Yield>
    <Id name="foo"/>
</Yield>
```

```brix
(main
  (yield-all foo)
)
```

```output
<For>
    <In>
        <TypeCapture>
            <Id name="-"/>
        </TypeCapture>
        <Id name="foo"/>
    </In>
    <Yield>
        <Id name="-"/>
    </Yield>
</For>
```

```brix
(main
  x
  (print foo)
)
```

```output
<Id name="x"/>
<Print>
    <Id name="foo"/>
</Print>
```

```brix
(main
  (break)
)
```

```output
<Break/>
```

```brix
(main
  (continue)
)
```

```output
<Continue/>
```

### A name that begins with break or continue is a name, not a break or a continue.

```brix
(main
  (call break-x)
  (call continue-x)
)
```

```output
<CallExpression>
    <Id name="break-x"/>
</CallExpression>
<CallExpression>
    <Id name="continue-x"/>
</CallExpression>
```
