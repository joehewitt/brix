# Stream match generic source

```yumi
-- parse-name("name:ada\n")
-- catch parse-name("invalid\n")

━━━━━━━━━━━━━━━━━━━━
+ InvalidRecord is Error

━━━━━━━━━━━━━━━━━━━━
+ parse-name String ?! InvalidRecord
    - source <Char>

match source
    ~ "name:" name [a-z]+ "\n"
        name
    ?! InvalidRecord("Expected a name record")
```

```output
ada
Expected a name record
```

```yumi
-- parse-name("name:ada\n")

. incoming Char<>
do new
    for char in "name:bob\n"
        incoming <- char
    incoming.close()

-- parse-name(incoming)

━━━━━━━━━━━━━━━━━━━━
+ InvalidRecord is Error

━━━━━━━━━━━━━━━━━━━━
+ parse-name String ?! InvalidRecord
    - source <Char>

match source
    ~ "name:" name [a-z]+ "\n"
        name
    ?! InvalidRecord("Expected a name record")
```

```output
ada
bob
```
