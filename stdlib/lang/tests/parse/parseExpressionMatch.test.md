# Expression match

Apply without input

```brix
(main
  (select result (<* binding))
)
```

```output
<StructureMatch>
    <TypeTransform>
        <TypeCapture>
            <Id name="binding"/>
        </TypeCapture>
        <Id name="result"/>
    </TypeTransform>
</StructureMatch>
```

```brix
(main
  (select result (~ Binding))
)
```

```output
<StreamMatch>
    <StreamTransform>
        <StreamType>
            <TypeId name="Binding"/>
        </StreamType>
        <Id name="result"/>
    </StreamTransform>
</StreamMatch>
```

## Apply without binding

```brix
(main
  (= result input)
)
```

```output
<AssignmentExpression op="=">
    <Id name="result"/>
    <Id name="input"/>
</AssignmentExpression>
```

```brix
(main
  (in result input)
)
```

```output
<Iterator>
    <In>
        <Id name="input"/>
    </In>
    <Id name="result"/>
</Iterator>
```

## Structural match with binding and input

```brix
(main
  (= (select result (| binding)) input)
)
```

```output
<Match>
    <Id name="input"/>
    <StructureMatch>
        <TypeTransform>
            <TypeCapture>
                <Id name="binding"/>
            </TypeCapture>
            <Id name="result"/>
        </TypeTransform>
    </StructureMatch>
</Match>
```

```brix
(main
  (in (select result (<* binding)) input)
)
```

```output
<Iterator>
    <In>
        <StructureMatch>
            <TypeTransform>
                <TypeCapture>
                    <Id name="binding"/>
                </TypeCapture>
                <Id name="result"/>
            </TypeTransform>
        </StructureMatch>
        <Id name="input"/>
    </In>
    <Id name="each"/>
</Iterator>
```

```brix
(main
  (in (select result (<* binding)) input (until later))
)
```

```output
<Iterator>
    <In>
        <StructureMatch>
            <TypeTransform>
                <TypeCapture>
                    <Id name="binding"/>
                </TypeCapture>
                <Id name="result"/>
            </TypeTransform>
        </StructureMatch>
        <Id name="input"/>
        <Until>
            <Id name="later"/>
        </Until>
    </In>
    <Id name="each"/>
</Iterator>
```

## Pattern match with binding and input

```brix
(main
  (= (select result (~ Binding)) input)
)
```

```output
<Match>
    <Id name="input"/>
    <StreamMatch>
        <StreamTransform>
            <StreamType>
                <TypeId name="Binding"/>
            </StreamType>
            <Id name="result"/>
        </StreamTransform>
    </StreamMatch>
</Match>
```

```brix
(main
  (in (select result (~ Binding)) input)
)
```

```output
<Iterator>
    <In>
        <StreamTransform>
            <StreamType>
                <TypeId name="Binding"/>
            </StreamType>
            <Id name="result"/>
        </StreamTransform>
        <Id name="input"/>
    </In>
    <Id name="each"/>
</Iterator>
```
