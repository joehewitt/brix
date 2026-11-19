# Expression expressions

```brix
(main
  #foo
)
```

```output
<Id name="foo" tracked="true"/>
```

```brix
(main
  (. object #foo)
)
```

```output
<Get name="foo" tracked="true">
    <Id name="object"/>
</Get>
```

```brix
(main
  $foo
)
```

```output
<StyleRead name="foo"/>
```

```brix
(main
  (. object $foo)
)
```

```output
<StyleRead name="foo">
    <Id name="object"/>
</StyleRead>
```

```brix
(main
  (?. object $foo)
)
```

```output
<StyleRead name="foo" optional="true">
    <Id name="object"/>
</StyleRead>
```

```brix
(main
  (## foo)
)
```

```output
<Latest>
    <Id name="foo"/>
</Latest>
```

```brix
(main
  (call foo (:of (## foo)))
)
```

```output
<CallExpression>
    <Id name="foo"/>
    <Argument name="of">
        <Latest>
            <Id name="foo"/>
        </Latest>
    </Argument>
</CallExpression>
```

```brix
(main
  (stream foo)
)
```

```output
<Iterable>
    <Id name="foo"/>
</Iterable>
```
