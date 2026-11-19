# Assert

```brix
(main
  (throw error)
)
```

```output
<Assert>
    <Id name="error"/>
</Assert>
```

```brix
(main
  (throw error (if condition))
)
```

```output
<Assert>
    <TypePredicate>
        <Id name="condition"/>
    </TypePredicate>
    <Id name="error"/>
</Assert>
```

```brix
(main
  (throw (!! failure) (if failure))
)
```

```output
<Assert>
    <TypePredicate>
        <Id name="failure"/>
    </TypePredicate>
    <UnaryExpression operator="!">
        <Id name="failure"/>
    </UnaryExpression>
</Assert>
```
