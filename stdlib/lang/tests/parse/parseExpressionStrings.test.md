# Expression strings

Basic strings

```brix
(main
  ""
)
```

```output
<String text=""/>
```

```brix
(main
  "foo"
)
```

```output
<String text="foo"/>
```

## Character literals

```brix
(main
  'A'
)
```

```output
<Integer value="65" unit="(char)"/>
```

```brix
(main
  (char "\n")
)
```

```output
<Integer value="10" unit="(char)"/>
```

```brix
(main
  (char "€")
)
```

```output
<Integer value="8364" unit="(char)"/>
```

```brix
(main
  (char "💜")
)
```

```output
<Integer value="128156" unit="(char)"/>
```

```brix
(main
  (char "'")
)
```

```output
<Integer value="39" unit="(char)"/>
```

```brix
(main
  (char "\x0b")
)
```

```output
<Integer value="11" unit="(char)"/>
```

```brix
(main
  (char "\x00")
)
```

```output
<Integer value="0" unit="(char)"/>
```

## Embedded expressions

```brix
(main
  "foo\${1}bar"
)
```

```output
<String text="foo${1}bar"/>
```

```brix
(main
  "foo\$bar"
)
```

```output
<String text="foo$bar"/>
```

```brix
(main
  (str "foo" 1)
)
```

```output
<BinaryExpression operator="++">
    <String text="foo"/>
    <Integer value="1"/>
</BinaryExpression>
```

```brix
(main
  (str 1 "bar")
)
```

```output
<BinaryExpression operator="++">
    <Integer value="1"/>
    <String text="bar"/>
</BinaryExpression>
```

```brix
(main
  (str "foo" 1 "bar")
)
```

```output
<BinaryExpression operator="++">
    <BinaryExpression operator="++">
        <String text="foo"/>
        <Integer value="1"/>
    </BinaryExpression>
    <String text="bar"/>
</BinaryExpression>
```

```brix
(main
  (str "foo" 1 "bar" 2 "baz")
)
```

```output
<BinaryExpression operator="++">
    <BinaryExpression operator="++">
        <BinaryExpression operator="++">
            <BinaryExpression operator="++">
                <String text="foo"/>
                <Integer value="1"/>
            </BinaryExpression>
            <String text="bar"/>
        </BinaryExpression>
        <Integer value="2"/>
    </BinaryExpression>
    <String text="baz"/>
</BinaryExpression>
```

```brix
(main
  (str "foo" bar)
)
```

```output
<BinaryExpression operator="++">
    <String text="foo"/>
    <Id name="bar"/>
</BinaryExpression>
```

```brix
(main
  (str "foo" (. bar baz))
)
```

```output
<BinaryExpression operator="++">
    <String text="foo"/>
    <Get name="baz">
        <Id name="bar"/>
    </Get>
</BinaryExpression>
```

```brix
(main
  (str "foo" bar ".#baz")
)
```

```output
<BinaryExpression operator="++">
    <BinaryExpression operator="++">
        <String text="foo"/>
        <Id name="bar"/>
    </BinaryExpression>
    <String text=".#baz"/>
</BinaryExpression>
```

```brix
(main
  "\n"
)
```

```output
<String text="
"/>
```

```brix
(main
  "\t"
)
```

```output
<String text="	"/>
```

```brix
(main
  "\"\""
)
```

```output
<String text=""""/>
```

```brix
(main
  "€"
)
```

```output
<String text="€"/>
```

```brix
(main
  "%"
)
```

```output
<String text="%"/>
```

### A line continuation in text adds no character.

```brix
(main
  "abcdef"
)
```

```output
<String text="abcdef"/>
```

### Text that is only line continuations is dropped beside an expression.

```brix
(main
  (str 1)
)
```

```output
<BinaryExpression operator="++">
    <String text=""/>
    <Integer value="1"/>
</BinaryExpression>
```

### Line continuations around text and an expression.

```brix
(main
  (str "pq" 1 "r")
)
```

```output
<BinaryExpression operator="++">
    <BinaryExpression operator="++">
        <String text="pq"/>
        <Integer value="1"/>
    </BinaryExpression>
    <String text="r"/>
</BinaryExpression>
```

```brix
(main
  "AB"
)
```

```output
<String text="AB"/>
```

```brix
(main
  "%f"
)
```

```output
<String text="%f"/>
```

```brix
(main
  "€a"
)
```

```output
<String text="€a"/>
```

```brix
(main
  "A2"
)
```

```output
<String text="A2"/>
```

```brix
(main
  "━"
)
```

```output
<String text="━"/>
```
