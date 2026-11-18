# Structure patterns

Types

```yumi
. foo
```

```brix
(main
  (let foo)
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="foo"/>
    </TypeCapture>
</Declaration>
```

```yumi
. $foo
```

```brix
(main
  (let $foo)
)
```

```output
<Declaration>
    <TypeEquality>
        <Id name="foo"/>
    </TypeEquality>
</Declaration>
```

```yumi
. foo Bar
```

```brix
(main
  (let foo Bar)
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="foo"/>
        <TypeId name="Bar"/>
    </TypeCapture>
</Declaration>
```

```yumi
. foo Bar?
```

```brix
(main
  (let foo (? Bar))
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="foo"/>
        <TypeArguments>
            <TypeId name="Optional"/>
            <TypeId name="Bar"/>
        </TypeArguments>
    </TypeCapture>
</Declaration>
```

```yumi
. Foo'Bar?
```

```brix
(main
  (let (? (of Foo Bar)))
)
```

```output
<Declaration>
    <TypeArguments>
        <TypeId name="Optional"/>
        <TypeArguments>
            <TypeId name="Foo"/>
            <TypeId name="Bar"/>
        </TypeArguments>
    </TypeArguments>
</Declaration>
```

```yumi
[Foo]
```

```brix
(main
  (list Foo)
)
```

```output
<List>
    <TypeId name="Foo"/>
</List>
```

```yumi
. [foo, bar]
[foo, bar] = 1
```

```brix
(main
  (let (list foo bar))
  (= (list foo bar) 1)
)
```

```output
<Declaration>
    <TypeList>
        <TypeCapture>
            <Id name="foo"/>
        </TypeCapture>
        <TypeCapture>
            <Id name="bar"/>
        </TypeCapture>
    </TypeList>
</Declaration>
<AssignmentExpression op="=">
    <List>
        <Id name="foo"/>
        <Id name="bar"/>
    </List>
    <Integer value="1"/>
</AssignmentExpression>
```

```yumi
. [foo, _, ...rest, bar]
```

```brix
(main
  (let (list foo _ ...rest bar))
)
```

```output
<Declaration>
    <TypeList>
        <TypeCapture>
            <Id name="foo"/>
        </TypeCapture>
        <TypeWildcard/>
        <TypeRemainder>
            <Id name="rest"/>
        </TypeRemainder>
        <TypeCapture>
            <Id name="bar"/>
        </TypeCapture>
    </TypeList>
</Declaration>
```

```yumi
. [foo Foo, bar]
[foo, bar] = 1
```

```brix
(main
  (let (list (: foo Foo) bar))
  (= (list foo bar) 1)
)
```

```output
<Declaration>
    <TypeList>
        <TypeCapture>
            <Id name="foo"/>
            <TypeId name="Foo"/>
        </TypeCapture>
        <TypeCapture>
            <Id name="bar"/>
        </TypeCapture>
    </TypeList>
</Declaration>
<AssignmentExpression op="=">
    <List>
        <Id name="foo"/>
        <Id name="bar"/>
    </List>
    <Integer value="1"/>
</AssignmentExpression>
```

```yumi
. [Foo = Bar]
```

```brix
(main
  (let (map Foo Bar))
)
```

```output
<Declaration>
    <TypeMap>
        <MapItem>
            <TypeId name="Foo"/>
            <TypeId name="Bar"/>
        </MapItem>
    </TypeMap>
</Declaration>
```

```yumi
. [foo = bar, baz = boo]
[foo = bar, baz = boo] = 1
```

```brix
(main
  (let (list (= foo bar) (= baz boo)))
  (= (list (= foo bar) (= baz boo)) 1)
)
```

```output
<Declaration>
    <TypeMap>
        <MapItem>
            <TypeCapture>
                <Id name="foo"/>
            </TypeCapture>
            <TypeCapture>
                <Id name="bar"/>
            </TypeCapture>
        </MapItem>
        <MapItem>
            <TypeCapture>
                <Id name="baz"/>
            </TypeCapture>
            <TypeCapture>
                <Id name="boo"/>
            </TypeCapture>
        </MapItem>
    </TypeMap>
</Declaration>
<AssignmentExpression op="=">
    <List>
        <AssignmentExpression op="=">
            <Id name="foo"/>
            <Id name="bar"/>
        </AssignmentExpression>
        <AssignmentExpression op="=">
            <Id name="baz"/>
            <Id name="boo"/>
        </AssignmentExpression>
    </List>
    <Integer value="1"/>
</AssignmentExpression>
```

```yumi
. []
```

```brix
(main
  (let (list))
)
```

```output
<Declaration>
    <TypeList/>
</Declaration>
```

```yumi
. _
```

```brix
(main
  (let _)
)
```

```output
<Declaration>
    <TypeWildcard/>
</Declaration>
```

```yumi
. [first, ..., last]
```

```brix
(main
  (let (list first ... last))
)
```

```output
<Declaration>
    <TypeList>
        <TypeCapture>
            <Id name="first"/>
        </TypeCapture>
        <TypeRemainder/>
        <TypeCapture>
            <Id name="last"/>
        </TypeCapture>
    </TypeList>
</Declaration>
```

```yumi
. ["id" = value, $key = expected, ...rest]
```

```brix
(main
  (let (list (= "id" value) (= $key expected) ...rest))
)
```

```output
<Declaration>
    <TypeMap>
        <MapItem>
            <TypeEquality>
                <String text="id"/>
            </TypeEquality>
            <TypeCapture>
                <Id name="value"/>
            </TypeCapture>
        </MapItem>
        <MapItem>
            <TypeEquality>
                <Id name="key"/>
            </TypeEquality>
            <TypeCapture>
                <Id name="expected"/>
            </TypeCapture>
        </MapItem>
        <TypeRemainder>
            <Id name="rest"/>
        </TypeRemainder>
    </TypeMap>
</Declaration>
```

```yumi
. ['x' = value]
```

```brix
(main
  (let (map 'x' value))
)
```

```output
<Declaration>
    <TypeMap>
        <MapItem>
            <TypeEquality>
                <Integer value="120" unit="(char)"/>
            </TypeEquality>
            <TypeCapture>
                <Id name="value"/>
            </TypeCapture>
        </MapItem>
    </TypeMap>
</Declaration>
```

```yumi
. f "foo" ...middle b "bar"
```

```brix
(main
  (let (all (: f "foo") ...middle (: b "bar")))
)
```

```output
<Declaration>
    <TypeAnd>
        <TypeAnd>
            <TypeCapture>
                <Id name="f"/>
                <TypeEquality>
                    <String text="foo"/>
                </TypeEquality>
            </TypeCapture>
            <TypeRemainder>
                <Id name="middle"/>
            </TypeRemainder>
        </TypeAnd>
        <TypeCapture>
            <Id name="b"/>
            <TypeEquality>
                <String text="bar"/>
            </TypeEquality>
        </TypeCapture>
    </TypeAnd>
</Declaration>
```

### Double-underscore is not a pattern.

```yumi crash
. [foo, __, bar]
```

```output
Exception: Syntax error
<no file:2>
    . [foo, __, bar]
             ^
```

### Remainders are only legal inside sequence, map, and text pattern profiles.

```yumi crash
. ...
```

```output
Exception: Remainder pattern is only valid inside sequence, map, or text patterns
<no file:2>
    . ...
      ^^^
```

### ImmutableArray and map bracket forms cannot be mixed.

```yumi crash
. [first, "id" = value]
```

```output
Exception: Cannot mix sequence and map items in one bracket pattern
<no file:2>
    . [first, "id" = value]
              ^^^^^^^^^^^^
```

### Each structure profile can contain at most one remainder.

```yumi crash
. [first, ...middle, ..., last]
```

```output
Exception: Pattern may contain only one remainder
<no file:2>
    . [first, ...middle, ..., last]
                         ^^^
```

### Map patterns can contain at most one unmatched-entry remainder.

```yumi crash
. ["a" = a, ...rest, ...]
```

```output
Exception: Pattern may contain only one remainder
<no file:2>
    . ["a" = a, ...rest, ...]
                         ^^^
```

### Text patterns can contain at most one substring remainder.

```yumi crash
. "a" ...middle "b" ...tail
```

```output
Exception: Pattern may contain only one remainder
<no file:2>
    . "a" ...middle "b" ...tail
                        ^^^^^^^
```

### Double-underscore is not a captured pattern.

```yumi crash
. rest __
```

```output
Exception: Syntax error
<no file:2>
    . rest __
            ^
```

```yumi
. <Foo>
```

```brix
(main
  (let (stream Foo))
)
```

```output
<Declaration>
    <TypeArguments>
        <TypeId name="Stream"/>
        <TypeId name="Foo"/>
    </TypeArguments>
</Declaration>
```

```yumi
. Foo[]
```

```brix
(main
  (let ([] Foo))
)
```

```output
<Declaration>
    <TypeArguments>
        <TypeId name="Array"/>
        <TypeId name="Foo"/>
    </TypeArguments>
</Declaration>
```

```yumi
. Foo[]?
```

```brix
(main
  (let (? ([] Foo)))
)
```

```output
<Declaration>
    <TypeArguments>
        <TypeId name="Optional"/>
        <TypeArguments>
            <TypeId name="Array"/>
            <TypeId name="Foo"/>
        </TypeArguments>
    </TypeArguments>
</Declaration>
```

```yumi
. {1}
```

```brix
(main
  (let ({} 1))
)
```

```output
<Declaration>
    <TypePredicate>
        <Integer value="1"/>
    </TypePredicate>
</Declaration>
```

```yumi
. Foo {bar}
```

```brix
(main
  (let (all Foo ({} bar)))
)
```

```output
<Declaration>
    <TypeAnd>
        <TypeId name="Foo"/>
        <TypePredicate>
            <Id name="bar"/>
        </TypePredicate>
    </TypeAnd>
</Declaration>
```

```yumi
. foo {bar}
```

```brix
(main
  (let (: foo ({} bar)))
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="foo"/>
        <TypePredicate>
            <Id name="bar"/>
        </TypePredicate>
    </TypeCapture>
</Declaration>
```

## Type relations

```yumi
. Foo / Bar
```

```brix
(main
  (let (/ Foo Bar))
)
```

```output
<Declaration>
    <TypeRelation op="/">
        <TypeId name="Foo"/>
        <TypeId name="Bar"/>
    </TypeRelation>
</Declaration>
```

```yumi
. Foo \ Bar
```

```brix
(main
  (let (\ Foo Bar))
)
```

```output
<Declaration>
    <TypeRelation op="\">
        <TypeId name="Foo"/>
        <TypeId name="Bar"/>
    </TypeRelation>
</Declaration>
```

```yumi
. Foo */ Bar
```

```brix
(main
  (let (*/ Foo Bar))
)
```

```output
<Declaration>
    <TypeRelation op="*/">
        <TypeId name="Foo"/>
        <TypeId name="Bar"/>
    </TypeRelation>
</Declaration>
```

```yumi
. Foo *\ Bar
```

```brix
(main
  (let (*\ Foo Bar))
)
```

```output
<Declaration>
    <TypeRelation op="*\">
        <TypeId name="Foo"/>
        <TypeId name="Bar"/>
    </TypeRelation>
</Declaration>
```

```yumi
. Foo -- Bar
```

```brix
(main
  (let (-- Foo Bar))
)
```

```output
<Declaration>
    <TypeRelation op="--">
        <TypeId name="Foo"/>
        <TypeId name="Bar"/>
    </TypeRelation>
</Declaration>
```

```yumi
. Foo ++ Bar
```

```brix
(main
  (let (++ Foo Bar))
)
```

```output
<Declaration>
    <TypeRelation op="++">
        <TypeId name="Foo"/>
        <TypeId name="Bar"/>
    </TypeRelation>
</Declaration>
```

## Operator precedence

```yumi
. Foo / Bar -- Baz
```

```brix
(main
  (let (/ Foo (-- Bar Baz)))
)
```

```output
<Declaration>
    <TypeRelation op="/">
        <TypeId name="Foo"/>
        <TypeRelation op="--">
            <TypeId name="Bar"/>
            <TypeId name="Baz"/>
        </TypeRelation>
    </TypeRelation>
</Declaration>
```

```yumi
. Foo \ Bar ++ Baz
```

```brix
(main
  (let (\ Foo (++ Bar Baz)))
)
```

```output
<Declaration>
    <TypeRelation op="\">
        <TypeId name="Foo"/>
        <TypeRelation op="++">
            <TypeId name="Bar"/>
            <TypeId name="Baz"/>
        </TypeRelation>
    </TypeRelation>
</Declaration>
```

## And patterns

```yumi
. Foo Bar
```

```brix
(main
  (let (all Foo Bar))
)
```

```output
<Declaration>
    <TypeAnd>
        <TypeId name="Foo"/>
        <TypeId name="Bar"/>
    </TypeAnd>
</Declaration>
```

## Or patterns

```yumi
. Foo | Bar
```

```brix
(main
  (let (| Foo Bar))
)
```

```output
<Declaration>
    <TypeOr>
        <TypeId name="Foo"/>
        <TypeId name="Bar"/>
    </TypeOr>
</Declaration>
```

```yumi
. Foo | Bar | Baz
```

```brix
(main
  (let (| Foo Bar Baz))
)
```

```output
<Declaration>
    <TypeOr>
        <TypeOr>
            <TypeId name="Foo"/>
            <TypeId name="Bar"/>
        </TypeOr>
        <TypeId name="Baz"/>
    </TypeOr>
</Declaration>
```

```yumi
. Foo | Foo'Bar
```

```brix
(main
  (let (| Foo (of Foo Bar)))
)
```

```output
<Declaration>
    <TypeOr>
        <TypeId name="Foo"/>
        <TypeArguments>
            <TypeId name="Foo"/>
            <TypeId name="Bar"/>
        </TypeArguments>
    </TypeOr>
</Declaration>
```

```yumi
. Foo | (Foo'Bar)
```

```brix
(main
  (let (| Foo (of Foo Bar)))
)
```

```output
<Declaration>
    <TypeOr>
        <TypeId name="Foo"/>
        <TypeArguments>
            <TypeId name="Foo"/>
            <TypeId name="Bar"/>
        </TypeArguments>
    </TypeOr>
</Declaration>
```

```yumi
. Foo | Bar?
```

```brix
(main
  (let (| Foo (? Bar)))
)
```

```output
<Declaration>
    <TypeOr>
        <TypeId name="Foo"/>
        <TypeArguments>
            <TypeId name="Optional"/>
            <TypeId name="Bar"/>
        </TypeArguments>
    </TypeOr>
</Declaration>
```

## Tuple types

```yumi
. (Foo)
```

```brix
(main
  (let Foo)
)
```

```output
<Declaration>
    <TypeId name="Foo"/>
</Declaration>
```

```yumi
. (foo Foo)
```

```brix
(main
  (let foo Foo)
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="foo"/>
        <TypeId name="Foo"/>
    </TypeCapture>
</Declaration>
```

```yumi
. foo, bar
foo, bar = 1
```

```brix
(main
  (let (tuple foo bar))
  (= (tuple foo bar) 1)
)
```

```output
<Declaration>
    <TypeTuple>
        <TypeCapture>
            <Id name="foo"/>
        </TypeCapture>
        <TypeCapture>
            <Id name="bar"/>
        </TypeCapture>
    </TypeTuple>
</Declaration>
<AssignmentExpression op="=">
    <Tuple>
        <Id name="foo"/>
        <Id name="bar"/>
    </Tuple>
    <Integer value="1"/>
</AssignmentExpression>
```

```yumi
. (foo, bar)
(foo, bar) = 1
```

```brix
(main
  (let (tuple foo bar))
  (= (tuple foo bar) 1)
)
```

```output
<Declaration>
    <TypeTuple>
        <TypeCapture>
            <Id name="foo"/>
        </TypeCapture>
        <TypeCapture>
            <Id name="bar"/>
        </TypeCapture>
    </TypeTuple>
</Declaration>
<AssignmentExpression op="=">
    <Tuple>
        <Id name="foo"/>
        <Id name="bar"/>
    </Tuple>
    <Integer value="1"/>
</AssignmentExpression>
```

```yumi
. foo, _, bar
```

```brix
(main
  (let (tuple foo _ bar))
)
```

```output
<Declaration>
    <TypeTuple>
        <TypeCapture>
            <Id name="foo"/>
        </TypeCapture>
        <TypeWildcard/>
        <TypeCapture>
            <Id name="bar"/>
        </TypeCapture>
    </TypeTuple>
</Declaration>
```

### Remainders are invalid in tuple patterns.

```yumi crash
. foo, bar, ...
```

```output
Exception: Remainder pattern is only valid inside sequence, map, or text patterns
<no file:2>
    . foo, bar, ...
                ^^^
```

```yumi
. (Foo, Bar)
```

```brix
(main
  (let (tuple Foo Bar))
)
```

```output
<Declaration>
    <TypeTuple>
        <TypeId name="Foo"/>
        <TypeId name="Bar"/>
    </TypeTuple>
</Declaration>
```

```yumi
. (foo Foo, bar Bar, baz Baz)
```

```brix
(main
  (let (tuple (: foo Foo) (: bar Bar) (: baz Baz)))
)
```

```output
<Declaration>
    <TypeTuple>
        <TypeCapture>
            <Id name="foo"/>
            <TypeId name="Foo"/>
        </TypeCapture>
        <TypeCapture>
            <Id name="bar"/>
            <TypeId name="Bar"/>
        </TypeCapture>
        <TypeCapture>
            <Id name="baz"/>
            <TypeId name="Baz"/>
        </TypeCapture>
    </TypeTuple>
</Declaration>
```

```yumi
for foo {foo} in bar: 0
```

```brix
(main
  (for (in (: foo ({} foo)) bar)
    (inline 0)
  )
)
```

```output
<For>
    <In>
        <TypeCapture>
            <Id name="foo"/>
            <TypePredicate>
                <Id name="foo"/>
            </TypePredicate>
        </TypeCapture>
        <Id name="bar"/>
    </In>
    <Integer value="0"/>
</For>
```

```yumi
. foo Bar bar Baz
```

```brix
(main
  (let (all (: foo Bar) (: bar Baz)))
)
```

```output
<Declaration>
    <TypeAnd>
        <TypeCapture>
            <Id name="foo"/>
            <TypeId name="Bar"/>
        </TypeCapture>
        <TypeCapture>
            <Id name="bar"/>
            <TypeId name="Baz"/>
        </TypeCapture>
    </TypeAnd>
</Declaration>
```

```yumi
. ..foo = bar
```

```brix
(main
  (let ..foo bar)
)
```

```output
<Declaration>
    <TypeProperty name="foo"/>
    <Id name="bar"/>
</Declaration>
```

```yumi
. .foo = bar
```

```brix
(main
  (let .foo bar)
)
```

```output
<Declaration>
    <TypeCase name="foo"/>
    <Id name="bar"/>
</Declaration>
```

```yumi
. .foo(x, y) = bar
```

```brix
(main
  (let (call .foo x y) bar)
)
```

```output
<Declaration>
    <TypeCase name="foo">
        <TypeTuple>
            <TypeCapture>
                <Id name="x"/>
            </TypeCapture>
            <TypeCapture>
                <Id name="y"/>
            </TypeCapture>
        </TypeTuple>
    </TypeCase>
    <Id name="bar"/>
</Declaration>
```

```yumi
. Object.foo = bar
```

```brix
(main
  (let (. Object foo) bar)
)
```

```output
<Declaration>
    <TypeCase name="foo">
        <TypeId name="Object"/>
    </TypeCase>
    <Id name="bar"/>
</Declaration>
```

```yumi
. Object.foo(x, y) = bar
```

```brix
(main
  (let (call (. Object foo) x y) bar)
)
```

```output
<Declaration>
    <TypeCase name="foo">
        <TypeId name="Object"/>
        <TypeTuple>
            <TypeCapture>
                <Id name="x"/>
            </TypeCapture>
            <TypeCapture>
                <Id name="y"/>
            </TypeCapture>
        </TypeTuple>
    </TypeCase>
    <Id name="bar"/>
</Declaration>
```
