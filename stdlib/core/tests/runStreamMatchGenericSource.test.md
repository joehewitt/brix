# Stream match generic source

```brix
(main
  (print (call parse-name "name:ada\n"))
  (print (catch (call parse-name "invalid\n")))
)

(type + InvalidRecord (is Error))

(fn-expr + parse-name String (?! InvalidRecord)
  (param source (stream Char))
  (match source
    (~ (seq "name:" (: name (+ (chars "a-z"))) "\n")
      name
    )
    (throw (call InvalidRecord "Expected a name record"))
  )
)
```

```output
ada
Expected a name record
```

```brix
(main
  (print (call parse-name "name:ada\n"))
  (blank)
  (let incoming (channel Char))
  (do-new
    (for (in char "name:bob\n")
      (<- incoming char)
    )
    (call (. incoming close))
  )
  (blank)
  (print (call parse-name incoming))
)

(type + InvalidRecord (is Error))

(fn-expr + parse-name String (?! InvalidRecord)
  (param source (stream Char))
  (match source
    (~ (seq "name:" (: name (+ (chars "a-z"))) "\n")
      name
    )
    (throw (call InvalidRecord "Expected a name record"))
  )
)
```

```output
ada
bob
```
