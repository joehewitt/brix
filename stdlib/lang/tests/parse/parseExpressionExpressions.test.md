# Expression expressions

```yumi
#foo
```

```brix
(main
  #foo
)
```

```output
<Id name="foo" tracked="true"/>
```

```yumi
object.#foo
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

```yumi
$foo
```

```brix
(main
  $foo
)
```

```output
<StyleRead name="foo"/>
```

```yumi
object.$foo
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

```yumi
object?.$foo
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

```yumi
##foo
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

```yumi
foo • of ##foo
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

```yumi
<foo>
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
