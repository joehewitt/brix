# Chunked read all

```brix
(import /core/tokenstream)
(blank)

(main
  (let source (* "a" 5000000))
  (blank)
  (for (in size (list 0 1024 1500 4194304))
    (let stream (call (of ChunkedTokenStream Char) (. ([] source (..< 0 size)) codepoints)))
    (let chars (call (. stream read)))
    (print (== (. chars size) size))
    (print (. stream has-more))
    (print (. (call (. stream read)) size))
  )
  (blank)
  (let unicode (* "aé中🙂" 400))
  (let stream (call (of ChunkedTokenStream Char) (. unicode codepoints)))
  (let chars (call (. stream read)))
  (print (== (call (. chars slice-string)) unicode))
)
```

```output
true
false
0
true
false
0
true
false
0
true
false
0
true
```
