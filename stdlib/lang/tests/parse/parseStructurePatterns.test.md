# Structure patterns

Types

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

### Double-underscore is not a captured pattern.

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
