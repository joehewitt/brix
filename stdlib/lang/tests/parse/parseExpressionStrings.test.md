# Expression strings

Basic strings

```yumi
""
```

```brix
(main
  ""
)
```

```output
<String text=""/>
```

```yumi
"foo"
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

```yumi
'A'
```

```brix
(main
  'A'
)
```

```output
<Integer value="65" unit="(char)"/>
```

```yumi
'\n'
```

```brix
(main
  (char "\n")
)
```

```output
<Integer value="10" unit="(char)"/>
```

```yumi
'\u20AC'
```

```brix
(main
  (char "€")
)
```

```output
<Integer value="8364" unit="(char)"/>
```

```yumi
'💜'
```

```brix
(main
  (char "💜")
)
```

```output
<Integer value="128156" unit="(char)"/>
```

```yumi
'\''
```

```brix
(main
  (char "'")
)
```

```output
<Integer value="39" unit="(char)"/>
```

```yumi
'\v'
```

```brix
(main
  (char "\x0b")
)
```

```output
<Integer value="11" unit="(char)"/>
```

```yumi
'\0'
```

```brix
(main
  (char "\x00")
)
```

```output
<Integer value="0" unit="(char)"/>
```

```yumi
'ab'
```

```output
Exception: Syntax error
<no file:2>
    'ab'
      ^
```

## Embedded expressions

```yumi
"foo\${1}bar"
```

```brix
(main
  "foo\${1}bar"
)
```

```output
<String text="foo${1}bar"/>
```

```yumi
"foo\$bar"
```

```brix
(main
  "foo\$bar"
)
```

```output
<String text="foo$bar"/>
```

```yumi
"foo${1}"
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

```yumi
"${1}bar"
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

```yumi
"foo${1}bar"
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

```yumi
"foo${1}bar${2}baz"
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

```yumi
"foo$bar"
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

```yumi
"foo$bar.baz"
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

```yumi
"foo$bar.#baz"
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

```yumi
"\n"
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

```yumi
"\t"
```

```brix
(main
  "\t"
)
```

```output
<String text="	"/>
```

```yumi
"\"\""
```

```brix
(main
  "\"\""
)
```

```output
<String text=""""/>
```

```yumi
"\u20AC"
```

```brix
(main
  "€"
)
```

```output
<String text="€"/>
```

```yumi
"\x25"
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

```yumi
"abc\
def"
```

```brix
(main
  "abcdef"
)
```

```output
<String text="abcdef"/>
```

### Text that is only line continuations is dropped beside an expression.

```yumi
"\
${1}\
"
```

```brix
(main
  (str "" 1 "")
)
```

```output
<BinaryExpression operator="++">
    <String text=""/>
    <Integer value="1"/>
</BinaryExpression>
```

### Line continuations around text and an expression.

```yumi
"p\
\
q${1}\
r"
```

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

```yumi
"\x41B"
```

```brix
(main
  "AB"
)
```

```output
<String text="AB"/>
```

```yumi
"\x25f"
```

```brix
(main
  "%f"
)
```

```output
<String text="%f"/>
```

```yumi
"\u20ACa"
```

```brix
(main
  "€a"
)
```

```output
<String text="€a"/>
```

```yumi
"\u00412"
```

```brix
(main
  "A2"
)
```

```output
<String text="A2"/>
```

```yumi
"\━"
```

```brix
(main
  "━"
)
```

```output
<String text="━"/>
```
