# Pattern helper captures

```brix
(main
  (let masked
    (match (call (of Array Byte) (list 1u8 129u8 97u8 98u8 99u8 100u8))
      (~ (: packet Packet)
        packet
      )
      (throw (call Panic "Expected masked packet"))
    )
  )
  (print (. masked value))
  (print (. masked mask-length))
  (let unmasked
    (match (call (of Array Byte) "\x01\x01")
      (~ (: packet Packet)
        packet
      )
      (throw (call Panic "Expected unmasked packet"))
    )
  )
  (print (. unmasked value))
  (print (. unmasked mask-length))
  (let longer
    (match (call (of Array Byte) "\x01\x03")
      (~ (: packet Packet)
        packet
      )
      (throw (call Panic "Expected longer packet"))
    )
  )
  (print (. longer value))
  (print (. longer mask-length))
)

(type + Packet (is Tuple)
  (prop + is-masked _ (= true))
  (prop + value U64)
  (prop + mask-length Size)
  (fn-expr + This _ (over: Byte)
    (~ (seq (: b1 U8) (: b2 U8) (: len \length) (: msk \mask))
      (call This true len (. msk length))
      (blank)
      (lazy is-masked (/zrshift/ b2 7u8))
      (lazy length8 decoded-length)
      (lazy decoded-length (/and/ b2 127u8))
      (lazy length
        (pattern
          (~ (: value \raw-length)
            value
          )
        )
      )
      (blank)
      (lazy raw-length
        (pattern
          (~ ({} U8 0)
            (as length8 U64)
          )
        )
      )
      (blank)
      (lazy mask
        (pattern
          (~ ({} Byte (if-expr is-masked 4 0)))
        )
      )
    )
  )
)
```

```output
1
4
1
0
3
0
```

```brix
(main
  (let parsed
    (match (call (of Array Byte) "\x01\x03")
      (~ (: packet Packet)
        packet
      )
      (throw (call Panic "Expected packet"))
    )
  )
  (print (. parsed value))
)

(type + Packet (is Tuple)
  (prop + value U64)
  (fn-expr + This _ (over: Byte)
    (~ (seq (: b2 U8) (: len \length))
      (call This len)
      (blank)
      (lazy length
        (pattern
          (~ (: b2 U8)
            (as b2 U64)
          )
        )
      )
    )
  )
)
```

```output
3
```

```brix
(main
  (let parsed
    (match (call (of Array Byte) "\x01")
      (~ (: packet Packet)
        packet
      )
      (throw (call Panic "Expected packet"))
    )
  )
  (print (. parsed value))
)

(type + Packet (is Tuple)
  (prop + value U64)
  (fn-expr + This _ (over: Byte)
    (~ (seq (: b2 U8) (: len \length))
      (call This len)
      (blank)
      (lazy length
        (pattern
          (~ ({} U8 0)
            (as b2 U64)
            (blank)
            (lazy b2 5u8)
          )
        )
      )
    )
  )
)
```

```output
5
```

```brix
(main
  (let parsed
    (match (call (of Array Byte) "\x01\x03")
      (~ (: packet Packet)
        packet
      )
      (throw (call Panic "Expected packet"))
    )
  )
  (print (. parsed value))
)

(type + Packet (is Tuple)
  (prop + value U64)
  (fn-expr + This _ (over: Byte)
    (~ (seq (: ignored U8) (: len \length))
      (call This len)
      (blank)
      (lazy length
        (pattern
          (~ (seq (: b2 U8) (: value \inner))
            value
            (blank)
            (lazy inner
              (pattern
                (~ ({} U8 0)
                  (as b2 U64)
                )
              )
            )
          )
        )
      )
    )
  )
)
```

```output
3
```
