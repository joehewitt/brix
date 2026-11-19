# Expression identifiers

Identifiers

```brix
(main
  foo
)
```

```output
<Id name="foo"/>
```

```brix
(main
  foo-bar
)
```

```output
<Id name="foo-bar"/>
```

```brix
(main
  FooBar
)
```

```output
<TypeId name="FooBar"/>
```

```brix
(main
  FooBar
)
```

```output
<TypeId name="FooBar"/>
```

```brix
(main
  ''FooBar
)
```

```output
<TypeId name="FooBar" declare="true"/>
```

## Greek alphabet

```brix
(main
  μ
)
```

```output
<Id name="μ"/>
```

```brix
(main
  Δ
)
```

```output
<Id name="Δ"/>
```

```brix
(main
  α
)
```

```output
<Id name="α"/>
```

```brix
(main
  Φ
)
```

```output
<Id name="Φ"/>
```

```brix
(main
  π
)
```

```output
<Id name="π"/>
```

## Other Unicode characters

```brix
(main
  C:fooBar1
)
```

```output
<CId name="fooBar1"/>
```

```brix
(main
  C:_fooBar1
)
```

```output
<CId name="_fooBar1"/>
```

```brix
(main
  &foo
)
```

```output
<Ref name="foo"/>
```

```brix
(main
  event
)
```

```output
<Id name="event"/>
```

```brix
(main
  publish
)
```

```output
<Id name="publish"/>
```
