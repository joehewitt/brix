# Return

```yumi
= foo
```

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

```yumi
<= foo
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

```yumi
<<= foo
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

```yumi
x
-- foo
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

```yumi
break
```

```brix
(main
  (break)
)
```

```output
<Break/>
```

```yumi
continue
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

```yumi
break-x()
continue-x()
```

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
