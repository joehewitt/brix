# Expression identifiers

Identifiers

```yumi
foo
```

```brix
(main
  foo
)
```

```output
<Id name="foo"/>
```

```yumi
foo-bar
```

```brix
(main
  foo-bar
)
```

```output
<Id name="foo-bar"/>
```

```yumi
FooBar
```

```brix
(main
  FooBar
)
```

```output
<TypeId name="FooBar"/>
```

```yumi
FooBar
```

```brix
(main
  FooBar
)
```

```output
<TypeId name="FooBar"/>
```

```yumi
''FooBar
```

```brix
(main
  ''FooBar
)
```

```output
<TypeId name="FooBar" declare="true"/>
```

```yumi
fooBar
```

```output
Exception: Names are kebab-case; write "foo-bar", not "fooBar"
<no file:2>
    fooBar
       ^
```

```yumi
Foo-Bar
```

```output
Exception: Syntax error
<no file:2>
    Foo-Bar
       ^
```

## Greek alphabet

```yumi
μ
```

```brix
(main
  μ
)
```

```output
<Id name="μ"/>
```

```yumi
Δ
```

```brix
(main
  Δ
)
```

```output
<Id name="Δ"/>
```

```yumi
α
```

```brix
(main
  α
)
```

```output
<Id name="α"/>
```

```yumi
Φ
```

```brix
(main
  Φ
)
```

```output
<Id name="Φ"/>
```

```yumi
π
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

```yumi
º
```

```output
Exception: Syntax error
<no file:2>
    º
    ^
```

```yumi
C:fooBar1
```

```brix
(main
  C:fooBar1
)
```

```output
<CId name="fooBar1"/>
```

```yumi
C:_fooBar1
```

```brix
(main
  C:_fooBar1
)
```

```output
<CId name="_fooBar1"/>
```

```yumi
&foo
```

```brix
(main
  &foo
)
```

```output
<Ref name="foo"/>
```

```yumi
event
```

```brix
(main
  event
)
```

```output
<Id name="event"/>
```

```yumi
publish
```

```brix
(main
  publish
)
```

```output
<Id name="publish"/>
```
