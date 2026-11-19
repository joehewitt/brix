# Stream match

```brix crash
(main
  (print (~ "foo"))
)
```

```output
Exception: Pattern has nothing to match
<no file:3>
      (print (~ "foo"))
             ^^^^^^^^^
```

### A pattern rule declared where no stream reaches it has nothing to match, used or not.

```brix crash
(main
  (print (call parse "12"))
)

(fn-expr + parse String
  (param text String)
  text
  (where
    (lazy num Int
      (pattern
        (~ (+ (chars "0-9")))
      )
    )
  )
)
```

```output
Exception: Pattern has nothing to match
<no file:10>
        (lazy num Int
              ^^^
```

### A typed match helper materializes a faithful source capture at the declared Primary boundary.

```brix
(main
  (let helper (call CaptureHelper))
  (let result
    (match "alpha!"
      (with helper)
      (~ \retained-word)
      (throw (call Panic "typed helper match failed"))
      (blank)
      (lazy retained-word String
        (pattern
          (~ (seq (: text (+ (chars "a-z"))) "!")
            text
          )
        )
      )
    )
  )
  (blank)
  (let module-result
    (match "beta!"
      (with helper)
      (~ \module-materialized-word)
      (throw (call Panic "module typed helper match failed"))
    )
  )
  (blank)
  (print result)
  (print module-result)
  (print (. helper materializations))
)

(fn-expr + module-materialized-word String (over: Char)
  (~ (seq (: text (+ (chars "a-z"))) "!")
    text
  )
)

(type + CaptureHelper
  (implements (of PatternMaterializationRoot String))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternRollback Size))
  (prop + materializations Size)
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= materializations 1)
    (return (call (. source slice-string) (..< start end)))
  )
  (fn-expr + mark Size
    (+ materializations 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (= materializations (- mark 1))
  )
)
```

```output
alpha
beta
2
```

### Faithful captures are materialized elementwise through Optional and ImmutableArray containers.

```brix
(main
  (let helper (call ContainerCaptureMaterializer))
  (let result
    (match "a|b,c!"
      (with helper)
      (~ (seq (? (: optional-word (chars "a-z"))) "|" (+ (seq (: words (chars "a-z")) (? ","))) "!")
        (call MaterializedCaptures optional-word words)
      )
      (throw (call Panic "capture materializer match failed"))
    )
  )
  (blank)
  (print (?? (. result optional-word) "missing"))
  (print (call (. "," join) (. result words)))
  (print (. helper materializations))
  (blank)
  (let missing
    (match "|d!"
      (with helper)
      (~ (seq (: optional-word (? (chars "a-z"))) "|" (+ (seq (: words (chars "a-z")) (? ","))) "!")
        (call MaterializedCaptures optional-word words)
      )
      (throw (call Panic "wrapped optional capture materializer match failed"))
    )
  )
  (blank)
  (print (?? (. missing optional-word) "missing"))
  (print (call (. "," join) (. missing words)))
  (print (. helper materializations))
)

(type + MaterializedCaptures (is (tuple (: optional-word (? String)) (: words (of ImmutableArray String)))))

(type + ContainerCaptureMaterializer
  (implements (of PatternMaterializationRoot MaterializedCaptures))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternRollback Size))
  (prop + materializations Size)
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= materializations 1)
    (return (call (. source slice-string) (..< start end)))
  )
  (fn-expr + mark Size
    (+ materializations 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (= materializations (- mark 1))
  )
)
```

```output
a
b,c
3
missing
d
4
```

### Semantic captures are materialized elementwise through Optional and ImmutableArray containers.

```brix
(main
  (let helper (call ContainerValueMaterializer))
  (let result
    (match "7:1,2,3!"
      (with helper)
      (~ (seq (? (: optional-value \semantic-value)) ":" (+ (seq (: values \semantic-value) (? ","))) "!")
        (call MaterializedValues (!! optional-value) values)
      )
      (throw (call Panic "value materializer match failed"))
      (blank)
      (lazy semantic-value I32
        (pattern
          (~ "1"
            1i32
          )
          (~ "2"
            2i32
          )
          (~ "3"
            3i32
          )
          (~ "7"
            7i32
          )
        )
      )
    )
  )
  (blank)
  (print (. result optional-value))
  (print (call (. "," join) (. result values)))
  (print (. helper materializations))
)

(type + MaterializedValues (is (tuple (: optional-value String) (: values (of ImmutableArray String)))))

(type + ContainerValueMaterializer
  (implements (of PatternMaterializationRoot MaterializedValues))
  (implements (of PatternValueMaterializer I32 String))
  (implements (of PatternRollback Size))
  (prop + materializations Size)
  (fn-do + materialize-value String
    (param input I32)
    (+= materializations 1)
    (return (as input String))
  )
  (fn-expr + mark Size
    (+ materializations 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (= materializations (- mark 1))
  )
)
```

```output
7
1,2,3
4
```

### Under a helper that makes values of another type, a capture keeps its own type where an action uses it as that type; the helper makes it only where it is wanted as the type it makes.

```brix
(main
  (let helper (call Converter))
  (print
    (match "7"
      (with helper)
      (~ (: v \seven)
        (+ v 1)
      )
      (default
        (inline 0i32)
      )
    )
  )
  (print
    (match "7"
      (with helper)
      (~ (: v \seven)
        (str "got " v)
      )
      (default
        (inline "none")
      )
    )
  )
  (print (. helper materializations))
)

(fn-expr + seven I32 (over: Char)
  (~ "7"
    7i32
  )
)

(type + Converter
  (implements (of PatternMaterializationRoot String))
  (implements (of PatternValueMaterializer I32 String))
  (implements (of PatternRollback Size))
  (prop + materializations Size)
  (fn-do + materialize-value String
    (param input I32)
    (+= materializations 1)
    (return (+ "<" (as input String) ">"))
  )
  (fn-expr + mark Size
    (+ materializations 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (= materializations (- mark 1))
  )
)
```

```output
8
got 7
0
```

### An explicitly produced CaptureSpan is semantic data and bypasses source-capture materialization.

```brix
(main
  (let helper (call ExplicitSpanMaterializer))
  (let result
    (match (. "alpha!" codepoints)
      (with helper)
      (~ (seq (: span \explicit-span) "!")
        (call ExplicitSpanResult span)
      )
      (throw (call Panic "explicit span match failed"))
      (blank)
      (lazy explicit-span (of CaptureSpan Char)
        (pattern
          (~ (+ (chars "a-z"))
            (call (of CaptureSpan Char) (. "kept" codepoints) 0 4)
          )
        )
      )
    )
  )
  (blank)
  (print result)
  (print (. helper materializations))
)

(type + ExplicitSpanResult (is (tuple (: span String))))

(type + ExplicitSpanMaterializer
  (implements (of PatternMaterializationRoot ExplicitSpanResult))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternRollback Size))
  (prop + materializations Size)
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= materializations 1)
    (return (call (. source slice-string) (..< start end)))
  )
  (fn-expr + mark Size
    (+ materializations 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (= materializations (- mark 1))
  )
)
```

```output
(alpha)
0
```

### A materializing helper without PatternRollback materializes without rolling back.

```brix
(main
  (let helper (call PlainMaterializer))
  (print
    (match "alpha!"
      (with helper)
      (~ (seq (: text (+ (chars "a-z"))) "!")
        text
      )
      (default
        (inline "none")
      )
    )
  )
)

(type + PlainMaterializer
  (implements (of PatternMaterializationRoot String))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (return (++ "<" (call (. source slice-string) (..< start end)) ">"))
  )
)
```

```output
<alpha>
```

### Overlapping valid materializer instantiations are rejected instead of chosen by declaration order.

```brix crash
(main
  (let helper (call AmbiguousMaterializer))
  (print
    (match (. "alpha!" codepoints)
      (with helper)
      (~ (seq (: text (+ (chars "a-z"))) "!")
        text
      )
    )
  )
)

(type + AmbiguousMaterializer
  (implements (of PatternMaterializationRoot String))
  (implements (of PatternCaptureMaterializer Number String))
  (implements (of PatternCaptureMaterializer U64 String))
  (implements (of PatternRollback Size))
  (fn-expr + materialize-capture String
    (param source ([] U64))
    (param start Index)
    (param end Index)
    "wide"
  )
  (fn-expr + materialize-capture String
    (param source ([] Number))
    (param start Index)
    (param end Index)
    "number"
  )
  (fn-expr + mark Size
    1
  )
  (fn-do + rewind _
    (param mark Size)
  )
)
```

```output
Exception: Pattern helper has ambiguous PatternCaptureMaterializer capabilities for input Char, output String: PatternCaptureMaterializer'Number'String, PatternCaptureMaterializer'U64'String
<no file:7>
          (~ (seq (: text (+ (chars "a-z"))) "!")
          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An unprefixed helper and an explicit rule marker protect a non-Primary side effect.

```brix
(main
  (let journal (call ExplicitRollbackJournal))
  (let result
    (match "go!"
      (with journal)
      (~ (seq \explicit-candidate "?")
        1
      )
      (~ (seq \explicit-candidate "!")
        2
      )
      (default
        (inline 0)
      )
      (blank)
      (lazy explicit-candidate I32
        (pattern
          @rollback
          (~ "go"
            (call (. journal increment))
          )
        )
      )
    )
  )
  (let after-local (. journal count))
  (blank)
  (let module-result
    (match "go!"
      (with journal)
      (~ (seq (call \module-explicit-candidate journal) "?")
        1
      )
      (~ (seq (call \module-explicit-candidate journal) "!")
        2
      )
      (default
        (inline 0)
      )
    )
  )
  (let after-module (. journal count))
  (blank)
  (let action-result
    (match "x"
      (with journal)
      (~ "x" @rollback
        (call (. journal increment))
      )
    )
  )
  (blank)
  (print result)
  (print module-result)
  (print action-result)
  (print after-local)
  (print after-module)
  (print (. journal count))
)

(fn-expr + module-explicit-candidate _ (over: Char) @rollback
  (param journal ExplicitRollbackJournal)
  (~ "go"
    (call (. journal increment))
  )
)

(type + ExplicitRollbackJournal
  (implements (of PatternRollback Size))
  (prop + count Size)
  (fn-expr + mark Size
    (+ count 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (= count (- mark 1))
  )
  (fn-do + increment I32
    (+= count 1)
    (return 7)
  )
)
```

```output
2
2
7
1
2
3
```

### An explicit action marker diagnoses a missing helper before lowering rollback paths.

```brix crash
(main
  (print
    (match "x"
      (~ "x" @rollback
        1
      )
    )
  )
)
```

```output
Exception: Pattern @rollback requires a with helper
<no file:5>
          (~ "x" @rollback
          ^^^^^^^^^^^^^^^
```

### A participating match rewinds a successful nested rule when its caller arm fails, then retains only the winning arm's mutation.

```brix
(main
  (let journal (call ParseJournal))
  (let result
    (match "go!"
      (with journal)
      (~ (seq \candidate "?")
        1
      )
      (~ (seq \candidate "!")
        2
      )
      (default
        (inline 0)
      )
      (blank)
      (lazy candidate I32
        (pattern
          @rollback
          (~ "go"
            (call (. journal increment))
          )
        )
      )
    )
  )
  (blank)
  (print result)
  (print (. journal count))
)

(type + ParseJournal
  (implements (of PatternRollback Size))
  (prop + count Size)
  (fn-expr + mark Size
    (+ count 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (= count (- mark 1))
  )
  (fn-do + increment I32
    (+= count 1)
    (return 7)
  )
)
```

```output
2
1
```

### Direct module-pattern helpers expose ordinary source arguments while inferring nested-rule transform result types under rollback.

```brix
(main
  (let journal (call ParseJournal))
  (let result
    (match "go!"
      (with journal)
      (~ (call \owned-candidate journal 40)
        (+ (. journal count) 40)
      )
      (default
        (inline 0)
      )
    )
  )
  (blank)
  (print result)
  (print (. journal count))
)

(fn-expr + owned-candidate _ (over: Char) @rollback
  (param journal ParseJournal)
  (param offset I32)
  (~ (seq \candidate "!")
    it
  )
  (where
    (lazy candidate
      (pattern
        (~ "go"
          (+ (call (. journal increment)) offset)
        )
      )
    )
  )
)

(type + ParseJournal
  (implements (of PatternRollback Size))
  (prop + count Size)
  (fn-expr + mark Size
    (+ count 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (= count (- mark 1))
  )
  (fn-do + increment I32
    (+= count 1)
    (return 1)
  )
)
```

```output
41
1
```

### Rollback follows optional, repetition, lookahead, root failure, nested matches, and chunked-source retries while preserving successful mutations.

```brix
(import /core/patterns)
(import /core/tokenstream)
(blank)

(main
  (let optional-journal (call ParseJournal))
  (print
    (match "a"
      (with optional-journal)
      (~ (seq (? (seq (call \touch-empty optional-journal) "x")) "a")
        (. optional-journal count)
      )
      (default
        (inline 99)
      )
    )
  )
  (blank)
  (let repeat-journal (call ParseJournal))
  (print
    (match "aaab"
      (with repeat-journal)
      (~ (seq (* (seq (call \touch-empty repeat-journal) "a")) "b")
        (. repeat-journal count)
      )
      (default
        (inline 99)
      )
    )
  )
  (blank)
  (let minimum-journal (call ParseJournal))
  (print
    (match "a"
      (with minimum-journal)
      (~ ({} (seq (call \touch-empty minimum-journal) "a") 2)
        (. minimum-journal count)
      )
      (default
        (inline (. minimum-journal count))
      )
    )
  )
  (blank)
  (let positive-journal (call ParseJournal))
  (print
    (match "a"
      (with positive-journal)
      (~ (seq (& (seq (call \touch-empty positive-journal) "a")) "a")
        (. positive-journal count)
      )
      (default
        (inline 99)
      )
    )
  )
  (blank)
  (let negative-journal (call ParseJournal))
  (print
    (match "a"
      (with negative-journal)
      (~ (seq (! (seq (call \touch-empty negative-journal) "a")) "a")
        99
      )
      (~ "a"
        (. negative-journal count)
      )
      (default
        (inline 98)
      )
    )
  )
  (blank)
  (let failure-journal (call ParseJournal))
  (print
    (match "a"
      (with failure-journal)
      (~ (seq (call \touch-empty failure-journal) "x")
        99
      )
      (default
        (inline (. failure-journal count))
      )
    )
  )
  (blank)
  (let nested-journal (call ParseJournal))
  (print
    (match "a!"
      (with nested-journal)
      (~ (seq (call \nested-action nested-journal) "?")
        99
      )
      (~ (seq (call \nested-action nested-journal) "!")
        (. nested-journal count)
      )
      (default
        (inline 98)
      )
    )
  )
)

(blank)
(prop + combinator-journal _ (= (call ParseJournal)))

(main
  (print
    (match "a,b!"
      (with combinator-journal)
      (~ (seq (call \delimited "," (~ (call \touch-word combinator-journal))) "?")
        99
      )
      (~ (seq (call \delimited "," (~ (call \touch-word combinator-journal))) "!")
        (. combinator-journal count)
      )
      (default
        (inline 98)
      )
    )
  )
  (blank)
  (let refill-journal (call ParseJournal))
  (let refill-tail (* "x" 70))
  (let refill-source (++ "p" refill-tail "!"))
  (let refill-stream (call (of ChunkedTokenStream Char) (. refill-source codepoints)))
  (print
    (match refill-stream
      (with refill-journal)
      (~ (seq (call \touch-prefix refill-journal) $refill-tail "!")
        (. refill-journal count)
      )
      (throw (call Panic "refill pattern did not parse"))
    )
  )
)

(type + ParseJournal
  (implements (of PatternRollback Size))
  (prop + count Size)
  (fn-expr + mark Size
    (+ count 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (= count (- mark 1))
  )
  (fn-do + increment I32
    (+= count 1)
    (return 7)
  )
)

(fn-expr + touch-empty _ (over: Char) @rollback
  (param journal ParseJournal)
  (~ ""
    (call (. journal increment))
  )
)

(fn-expr + touch-prefix _ (over: Char) @rollback
  (param journal ParseJournal)
  (~ "p"
    (call (. journal increment))
  )
)

(fn-expr + touch-word _ (over: Char) @rollback
  (param journal ParseJournal)
  (~ (+ (chars "a-z"))
    (call (. journal increment))
  )
)

(fn-expr + nested-action _ (over: Char) @rollback
  (param journal ParseJournal)
  (~ "a"
    (call inner-touch journal)
  )
)

(fn-expr + inner-touch I32
  (param journal ParseJournal)
  (match "z"
    (with journal)
    (~ (seq (call \touch-empty journal) "z")
      (as (. journal count) I32)
    )
    (throw (call Panic "inner pattern did not parse"))
  )
)
```

```output
0
3
0
0
0
0
1
2
1
```

### A local rule of several arms marked to roll back rewinds each arm's effect when its caller's arm fails, as a module rule does.

```brix
(main
  (let journal (call Journal))
  (let found
    (match "go!"
      (with journal)
      (~ (seq \word "?")
        1
      )
      (~ (seq \word "!")
        2
      )
      (default
        (inline 0)
      )
      (blank)
      (lazy word I32
        (pattern
          @rollback
          (~ "x"
            (call (. journal increment))
          )
          (~ "go"
            (call (. journal increment))
          )
        )
      )
    )
  )
  (print (str found " count " (. journal count)))
)

(type + Journal
  (implements (of PatternRollback Size))
  (prop + count Size)
  (fn-expr + mark Size
    (+ count 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (= count (- mark 1))
  )
  (fn-do + increment I32
    (+= count 1)
    (return 7)
  )
)
```

```output
2 count 1
```

### A match helper that implements no pattern capability is an error.

```brix crash
(main
  (print
    (match "a"
      (with 1)
      (~ "a")
    )
  )
)
```

```output
Exception: Pattern helper implements no pattern capability
<no file:5>
          (with 1)
                ^
```

### A match helper clause outside a stream match is an error.

```brix crash
(main
  (let n 5)
  (with n)
  (print "after")
)
```

```output
Exception: with helper must be the first clause of a stream match
<no file:4>
      (with n)
      ^^^^^^^^
```

### A static pattern argument crosses a recursive module combinator with one active frame. Failure restores that frame, and a retained result remains stable across a later match.

```brix
(import /core/array-builder)
(blank)
(import /core/patterns)
(blank)

(main
  (let retained (call parse-words "alpha, beta!"))
  (let failed
    (match "alpha,beta?"
      (~ (seq (: items (call \delimited "," (~ (+ (chars "a-z"))))) "!")
        items
      )
      (default
        (inline (call failed-words))
      )
    )
  )
  (let trailing (call parse-words-before-trailing "alpha,beta,!"))
  (let later (call parse-words "gamma!"))
  (blank)
  (print retained)
  (print failed)
  (print trailing)
  (print later)
  (print retained)
)

(fn-expr + parse-words (of ImmutableArray String)
  (param source String)
  (match source
    (~ (seq (: items (call \delimited "," (~ \word))) "!")
      items
    )
    (throw (call Panic "word list did not parse"))
    (blank)
    (lazy word String
      (pattern
        (~ (+ (chars "a-z")))
      )
    )
  )
)

(fn-do _ failed-words (of ImmutableArray String)
  (let builder (call (of ArrayBuilder String)))
  (call (. builder append) "failed")
  (return (call (. builder finish-immutable)))
)

(fn-expr + parse-words-before-trailing (of ImmutableArray String)
  (param source String)
  (match source
    (~ (seq (: items (call \delimited "," (~ \word))) ",!")
      items
    )
    (throw (call Panic "word list with trailing delimiter did not parse"))
    (blank)
    (lazy word String
      (pattern
        (~ (+ (chars "a-z")))
      )
    )
  )
)
```

```output
alpha, beta
failed
alpha, beta
gamma
alpha, beta
```

```brix crash
(main
  (print (select each (~ "foo")))
)
```

```output
Exception: Pattern has nothing to match
<no file:3>
      (print (select each (~ "foo")))
             ^^^^^^^^^^^^^^^^^^^^^^^
```

### Failure due to extra trailing characters

```brix
(main
  (let attempt
    (do-new
      (print
        (match "foo."
          (~ "foo")
          (throw (call Panic "Extra characters"))
        )
      )
      (on-failure failure
        (print (str "error: " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
error: Extra characters
```

```brix
(main
  (print
    (match "1"
      (~ (chars "a-z"))
    )
  )
)
```

```output
unknown
```

```brix
(main
  (print
    (match "c"
      (~ (chars "a-z"))
    )
  )
)
```

```output
c
```

```brix
(main
  (print (~ "foo" (of: "foo")))
)
```

```output
foo
```

```brix
(main
  (print
    (match "foo"
      (select (* it 2) (~ "foo"))
    )
  )
)
```

```output
foofoo
```

```brix
(main
  (print (~ "foo" (of: "foo")))
  (print
    (~ "foo" (of: "foo")
      (* it 2)
    )
  )
  (print (= (select (* it 3) (~ "foo")) "foo"))
)
```

```output
foo
foofoo
foofoofoo
```

```brix
(main
  (print (call (. "," join) (in (~ "foo") "foofoofoo")))
  (print
    (call (. "," join)
      (:stream
        (~ "foo" (in: "foofoofoo")
          (* it 2)
        )
      )
    )
  )
  (print (call (. "," join) (in (select (* it 3) (~ "foo")) "foofoofoo")))
)
```

```output
foo,foo,foo
foofoo,foofoo,foofoo
foofoofoo,foofoofoo,foofoofoo
```

```brix
(main
  (print (~ (* "foo") (of: "foofoofoo")))
)
```

```output
foofoofoo
```

```brix
(main
  (print (~ (seq "f" ({} "o" 3)) (of: "fooo")))
)
```

```output
fooo
```

```brix
(main
  (print (~ (seq "f" ({} "o" 3 5)) (of: "fooo")))
  (print (~ (seq "f" ({} "o" 3 5)) (of: "foooo")))
  (print (~ (seq "f" ({} "o" 3 5)) (of: "fooooo")))
  (print (~ (seq "f" ({} "o" 3 5)) (of: "fooooo")))
)
```

```output
fooo
foooo
fooooo
fooooo
```

```brix
(main
  (print
    (match "fooo"
      (~ (seq "f" ({} "o" len)))
      (blank)
      (lazy len 3)
    )
  )
)
```

```output
fooo
```

```brix
(main
  (let len 3)
  (print
    (match "fooo"
      (~ (seq "f" ({} "o" len)))
    )
  )
)
```

```output
fooo
```

```brix
(main
  (print
    (match "foo"
      (~ "foo"
        42
      )
    )
  )
)
```

```output
42
```

```brix
(main
  (print
    (match "foo"
      (~ (: x "foo")
        x
      )
    )
  )
)
```

```output
foo
```

```brix
(main
  (print
    (match "foo.com"
      (~ (seq (* (chars "a-z")) ".com"))
    )
  )
)
```

```output
foo.com
```

```brix
(main
  (print
    (match ".com"
      (~ (seq (* (chars "a-z")) ".com"))
    )
  )
)
```

```output
.com
```

```brix
(main
  (print
    (match "foo.com"
      (~ (seq (: domain (* (chars "a-z"))) ".com")
        (inline domain)
      )
    )
  )
)
```

```output
foo
```

```brix
(main
  (print
    (match "foo.com"
      (~ (seq (+ (chars "a-z")) ".com"))
    )
  )
)
```

```output
foo.com
```

```brix
(main
  (print
    (match ".com"
      (~ (seq (+ (chars "a-z")) ".com"))
    )
  )
)
```

```output
unknown
```

```brix
(main
  (print
    (match "foo.com"
      (~ (seq (: domain (+ (chars "a-z"))) ".com")
        domain
      )
    )
  )
)
```

```output
foo
```

```brix
(main
  (print
    (match "foo1.com"
      (~ (seq (+ (chars "a-z")) (? "1") ".com"))
    )
  )
)
```

```output
foo1.com
```

```brix
(main
  (print
    (match "a1b2|c3"
      (~ (+ (chars "a-z0-9|")))
    )
  )
)
```

```output
a1b2|c3
```

```brix
(main
  (print
    (match ".b"
      (~ (seq "." (| "a" "b" "c")))
    )
  )
)
```

```output
.b
```

```brix
(main
  (print
    (match "foo.net"
      (~ (seq (: domain (+ (chars "a-z"))) "." (: tld (? (| "com" "net" "org"))))
        (str "domain: " domain ", tld: " tld)
      )
    )
  )
)
```

```output
domain: foo, tld: net
```

```brix
(main
  (print
    (match "foo.com"
      (~ (seq (+ (chars "a-z")) (! ".") . "com"))
    )
  )
  (print
    (match "foo-com"
      (~ (seq (+ (chars "a-z")) (! ".") . "com"))
    )
  )
)
```

```output
unknown
foo-com
```

```brix
(main
  (print
    (match "crapola"
      (~ "foo.com"
        43
      )
      (~ "poop"
        42.5
      )
      (~ "crapola"
        41
      )
    )
  )
)
```

```output
41
```

```brix
(main
  (print
    (match "foo.com"
      (~ (seq (: d \domain) ".com")
        d
      )
      (blank)
      (lazy domain
        (pattern
          (~ (+ (chars "a-z")))
        )
      )
    )
  )
)
```

```output
foo
```

```brix
(main
  (print
    (match "f"
      (~ \domain)
      (blank)
      (lazy domain
        (pattern
          (~ \letter)
        )
      )
      (blank)
      (lazy letter
        (pattern
          (~ (chars "a-z")
            42
          )
        )
      )
    )
  )
)
```

```output
42
```

```brix
(main
  (print
    (match "http://me:pw@foo.com:42?pp#xx"
      (~ (seq (: sc \scheme) ":" (: hs \has-slashes) (: au (? \auth)) (: ho \host) (: s (? \search)) (: h (? \hash)))
        (tuple sc hs au ho s h)
      )
      (blank)
      (lazy scheme
        (pattern
          (~ (+ (| (chars "a-z") "+" "-" ".")))
        )
      )
      (lazy has-slashes
        (pattern
          (~ (* "/")
            (> (. it length) 0)
          )
        )
      )
      (lazy auth
        (pattern
          (~ (seq (: user (+ (chars "a-z"))) ":" (: pass (+ (chars "a-z"))) "@")
            (tuple user pass)
          )
        )
      )
      (lazy host
        (pattern
          (~ (seq (: ad \address) ":" (: p \port))
            (tuple ad p)
          )
          (~ (: ad \address)
            (tuple ad 0)
          )
        )
      )
      (lazy address
        (pattern
          (~ (+ (chars "a-z.")))
        )
      )
      (lazy port
        (pattern
          (~ (+ (chars "0-9"))
            (as it Int)
          )
        )
      )
      (lazy search
        (pattern
          (~ (seq "?" (: q (+ (seq (! "#") .))))
            q
          )
        )
      )
      (lazy hash
        (pattern
          (~ (seq "#" (: h (+ .)))
            h
          )
        )
      )
    )
  )
)
```

```output
(http, true, (me, pw), (foo.com, 42), pp, xx)
```

### Right tail recursion

```brix
(main
  (print
    (match "a,bob, fred , tom"
      (~ \items)
      (blank)
      (lazy items (list String)
        (pattern
          (~ (seq (: head \item) \delimiter (: tail \items))
            (++ head tail)
          )
          (~ \item)
        )
      )
      (blank)
      (lazy item
        (pattern
          (~ (+ (chars "a-z"))
            (list (+ it "_"))
          )
        )
      )
      (blank)
      (lazy delimiter
        (pattern
          (~ (seq (* " ") "," (* " ")))
        )
      )
    )
  )
)
```

```output
a_, bob_, fred_, tom_
```

### A grammar reads its items in a source; one written on a stream a subscription is given is an error.

```brix compile-error
(main
  (let events (channel Int))
  (let pairs
    (~ (seq (: a .) (: b .)) (on: events)
      (+ a b)
    )
  )
)
```

```output
Exception: A grammar matches items it reads with "in", not items a subscription with "on" is given; write "in" in place of "on"
<no file:5>
        (~ (seq (: a .) (: b .)) (on: events)
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### The local rules of a match cannot call one another in a cycle, which those of a rule a module declares can.

```brix compile-error
(main
  (print
    (match "aaaa"
      (~ (: n \even)
        n
      )
      (default
        (inline 0)
      )
      (blank)
      (lazy even Int
        (pattern
          (~ (seq "a" (: n \odd))
            (+ n 1)
          )
          (~ ""
            0
          )
        )
      )
      (blank)
      (lazy odd Int
        (pattern
          (~ (seq "a" (: n \even))
            (+ n 1)
          )
        )
      )
    )
  )
)
```

```output
Exception: The local rules of a match cannot call one another in a cycle; declare them below a rule the module declares, whose local rules can
<no file:25>
              (~ (seq "a" (: n \even))
                               ^^^^^
```

### No capture is named it, which names what an arm matched.

```brix compile-error
(main
  (print
    (match "ab"
      (~ (seq (: it "a") "b")
        it
      )
    )
  )
)
```

```output
Exception: A capture cannot be named "it", which names what the arm matched; choose another name
<no file:5>
          (~ (seq (: it "a") "b")
                     ^^
```

### A rule that calls itself before it reads anything, directly or through a rule it calls first, is an error.

```brix compile-error
(main
  (print
    (match "1+1"
      (~ \sum)
      (default
        (inline "none")
      )
      (blank)
      (lazy sum String
        (pattern
          (~ (seq \term "+" "1"))
          (~ "1")
        )
      )
      (blank)
      (lazy term String
        (pattern
          (~ (seq (* " ") \sum))
        )
      )
    )
  )
)
```

```output
Exception: "sum" calls itself before reading any input, so it never ends; read input before this call, or write the repetition with "*"
<no file:19>
              (~ (seq (* " ") \sum))
                              ^^^^
```

### A rule a module declares that calls itself before it reads anything is an error.

```brix compile-error
(main
  (print
    (match "1+1"
      (~ \sum)
      (default
        (inline "none")
      )
    )
  )
)

(fn-expr + sum String (over: Char)
  (~ (seq \sum "+" "1"))
  (~ "1")
)
```

```output
Exception: "sum" calls itself before reading any input, so it never ends; read input before this call, or write the repetition with "*"
<no file:14>
      (~ (seq \sum "+" "1"))
              ^^^^
```

```brix
(main
  (print
    (match "http://me:pw@foo.com:42"
      (~ (seq "http://" (: auth (? Auth)) (: ho Host))
        (tuple ho auth)
      )
    )
  )
)

(type + Host (is Tuple)
  (prop + address String)
  (prop + port Int)
  (fn-expr + This _ (over: Char)
    (~ (seq (: ad \address) ":" (: p \port))
      (call This ad p)
    )
    (~ (: ad \address)
      (call This ad 0)
    )
    (where
      (lazy address
        (pattern
          (~ (+ (chars "a-z.")))
        )
      )
      (lazy port
        (pattern
          (~ (+ (chars "0-9"))
            (as it Int)
          )
        )
      )
    )
  )
)

(type + Auth (is Tuple)
  (prop + user String)
  (prop + pass String)
  (fn-expr + This _ (over: Char)
    (~ (seq (: user (+ (chars "a-z"))) ":" (: pass (+ (chars "a-z"))) "@")
      (call This user pass)
    )
  )
)
```

```output
((foo.com, 42), (me, pw))
```

```brix
(main
  (print (~ URLHost (of: "foo.bAr.com:42")))
)

(type + URLHost (is (tuple (: address String) (: port Int)))
  (fn-expr + This _ (over: Char)
    (~ \host)
    (where
      (lazy host
        (pattern
          (~ (seq (: addr \address) ":" (: p \port))
            (call This addr p)
          )
          (~ \address
            (call This it 0)
          )
        )
      )
      (lazy address String
        (pattern
          (~ (seq \name "." \address))
          (~ \name)
        )
      )
      (lazy name
        (pattern
          (~ (seq Alpha (* AlphaNumeric)))
        )
      )
      (lazy port Int
        (pattern
          (~ (+ (chars "0-9")))
        )
      )
    )
  )
)

(type + Alpha (is String)
  (fn-expr + This _ (over: Char)
    (~ (chars "a-zA-Z"))
  )
)

(type + AlphaNumeric (is String)
  (fn-expr + This _ (over: Char)
    (~ (chars "a-zA-Z0-9"))
  )
)

(type + Numeric (is Int)
  (fn-expr + This _ (over: Char)
    (~ (chars "0-9"))
  )
)
```

```output
(foo.bAr.com, 42)
```

```brix
(main
  (print (~ URLHost (of: "foo")))
)

(type + URLHost (is String)
  (fn-expr + This _ (over: Char)
    (~ \address)
    (where
      (lazy address
        (pattern
          (~ \name)
        )
      )
      (blank)
      (lazy name
        (pattern
          (~ (+ (chars "a-z")))
        )
      )
    )
  )
)
```

```output
foo
```

```brix
(main
  (print
    (match "foo"
      (~ Foo)
    )
  )
)

(type + Foo
  (fn-expr + This _ (over: Char)
    (~ "foo"
      42
    )
  )
)
```

```output
42
```

```brix
(main
  (print
    (match "foo"
      (~ (call \foo))
    )
  )
)

(fn-expr + foo _ (over: Char)
  (~ "foo"
    42
  )
)
```

```output
42
```

```brix
(main
  (print
    (match "foo"
      (~ (call \foo 42))
    )
  )
)

(fn-expr + foo _ (over: Char)
  (param n _)
  (~ "foo"
    n
  )
)
```

```output
42
```

```brix
(main
  (print
    (match "fooooo"
      (~ (seq "f" (call \letter (~ "o") 5)))
    )
  )
)

(fn-expr + letter _ (over: Char)
  (param letter _)
  (param n _)
  (~ ({} $letter n))
)
```

```output
o, o, o, o, o
```

```brix
(main
  (print (~ (of \hex Char8) (of: "41")))
  (print (~ (of \hex U8) (of: "FF")))
  (print (~ (of \hex U16) (of: "FFFF")))
  (print (~ (of \hex U32) (of: "FFFFFFFF")))
  (blank)
  (print
    (match "414243"
      (~ (seq (: a (of \hex Char8)) (: b (of \hex Char8)) (: c (of \hex Char8)))
        (str a " " b " " c)
      )
    )
  )
)

(fn-expr + (of hex T) _ (over: Char)
  (~ ({} (chars "a-fA-F0-9") size)
    (as (call Hex it) T)
  )
  (where
    (lazy size (* (. T size) 2))
  )
)
```

```output
A
255
65535
4294967295
A B C
```

```brix
(main
  (print
    (match "foo=bar%20baz&doo=joby&x="
      (~ (call \delim "&" (~ URLPair)))
    )
  )
)

(type + URLPair (is (tuple (: key String) (: value String)))
  (fn-expr + This _ (over: Char)
    (~ (seq (: key \parts) "=" (: value (? \parts)))
      (call This key (?? value ""))
    )
    (where
      (lazy parts String
        (pattern
          (~ (seq (: head \part) (: tail \parts))
            (+ head tail)
          )
          (~ \part)
        )
      )
      (blank)
      (lazy part
        (pattern
          (~ (seq "%" (: hx (of \hex Char8)))
            (as hx String)
          )
          (~ "\\%"
            "%"
          )
          (~ (chars "a-zA-z0-9"))
        )
      )
    )
  )
)

(fn-expr + (of delim Result) _ (over: Char)
  (param delimiter _)
  (param pattern (of PatternFunc Char Result))
  (~ \items)
  (where
    (lazy items (list Result)
      (pattern
        (~ (seq (: head \item) $delimiter (: tail \items))
          (++ head tail)
        )
        (~ \item)
      )
    )
    (blank)
    (lazy item (list Result)
      (pattern
        (~ (: item $pattern)
          (list item)
        )
      )
    )
  )
)
```

```output
(foo, bar baz), (doo, joby), (x, )
```

```brix
(main
  (let foo "food")
  (print (~ $foo (of: "foo")))
  (blank)
  (= foo "foo")
  (print (~ $foo (of: "foo")))
  (blank)
  (= foo "fog")
  (print (~ $foo (of: "food")))
)
```

```output
unknown
foo
unknown
```

```brix
(main
  (print
    (match "al ,bob, fred"
      (~ (call (of \delim String) (~ (seq (* " ") "," (* " "))) (~ (+ (chars "a-z")))))
    )
  )
)

(fn-expr + (of delim Result) _ (over: Char)
  (param delimiter _)
  (param pattern _)
  (~ \items)
  (where
    (lazy items (list Result)
      (pattern
        (~ (seq (: head \item) $delimiter (: tail \items))
          (++ head tail)
        )
        (~ \item)
      )
    )
    (blank)
    (lazy item
      (pattern
        (~ (: ii \pattern)
          (list ii)
        )
      )
    )
  )
)
```

```output
al, bob, fred
```

```brix
(main
  (let text "foo")
  (print
    (match "foo"
      (~ \item)
      (blank)
      (lazy item
        (pattern
          (~ $text)
        )
      )
    )
  )
)
```

```output
foo
```

```brix
(main
  (print
    (match "foo"
      (~ (call \foo "foo"))
    )
  )
)

(fn-expr + foo _ (over: Char)
  (param text _)
  (~ \item)
  (where
    (lazy item
      (pattern
        (~ $text)
      )
    )
  )
)
```

```output
foo
```

```brix
(main
  (print
    (match "abc"
      (~ (+ (seq (! (| "," ";")) .)))
    )
  )
)
```

```output
abc
```

```brix
(main
  (print
    (match "x"
      (~ (: value (? \optional-record))
        (call optional-record-value value)
      )
      (blank)
      (lazy optional-record OptionalRecord
        (pattern
          (~ "x"
            (call OptionalRecord 7)
          )
        )
      )
    )
  )
)

(type + OptionalRecord (is Tuple)
  (prop + value I32)
)

(fn-expr + optional-record-value I32
  (param value (? OptionalRecord))
  (?? (?. value value) 0)
)
```

```output
7
```

```brix
(import /core/atom-pool)
(import /core/arena)
(blank)

(main
  (let output (call Arena))
  (let helper (call AtomPatternHelper output))
  (let direct
    (match "alpha!"
      (with helper)
      (~ \atom-word)
      (throw (call Panic "direct atom failed"))
      (blank)
      (lazy atom-word Atom
        (pattern
          (~ (seq (: word (+ (chars "a-z"))) "!")
            word
          )
        )
      )
    )
  )
  (let duplicate
    (match "alpha!"
      (with helper)
      (~ \atom-word)
      (throw (call Panic "duplicate atom failed"))
      (blank)
      (lazy atom-word Atom
        (pattern
          (~ (seq (: word (+ (chars "a-z"))) "!")
            word
          )
        )
      )
    )
  )
  (let transformed
    (match "alpha!"
      (with helper)
      (~ \atom-word)
      (throw (call Panic "transformed atom failed"))
      (blank)
      (lazy atom-word Atom
        (pattern
          (~ (seq (: word (+ (chars "a-z"))) "!")
            (+ word "-x")
          )
        )
      )
    )
  )
  (let optional
    (match "beta!"
      (with helper)
      (~ \maybe-atom)
      (throw (call Panic "optional atom failed"))
      (blank)
      (lazy maybe-atom (? Atom)
        (pattern
          (~ (seq (? (: word (+ (chars "a-z")))) "!")
            word
          )
        )
      )
    )
  )
  (let missing
    (match "!"
      (with helper)
      (~ \maybe-atom)
      (throw (call Panic "missing atom failed"))
      (blank)
      (lazy maybe-atom (? Atom)
        (pattern
          (~ (seq (? (: word (+ (chars "a-z")))) "!")
            word
          )
        )
      )
    )
  )
  (let words
    (match "a,b,a!"
      (with helper)
      (~ \many-atoms)
      (throw (call Panic "atom sequence failed"))
      (blank)
      (lazy many-atoms (of ImmutableArray Atom)
        (pattern
          (~ (seq (+ (seq (: words (chars "a-z")) (? ","))) "!")
            words
          )
        )
      )
    )
  )
  (let rolled-back
    (match "gamma!"
      (with helper)
      (~ \rollback-atom)
      (throw (call Panic "atom rollback failed"))
      (blank)
      (lazy rollback-atom Atom
        (pattern
          (~ (seq (: failed \failed-atom) "?")
            failed
          )
          (~ (seq (: winner \winner-atom) "!")
            winner
          )
        )
      )
      (blank)
      (lazy failed-atom Atom
        (pattern
          (~ (: word (+ (chars "a-z")))
            (+ word "-failed")
          )
        )
      )
      (blank)
      (lazy winner-atom Atom
        (pattern
          (~ (: word (+ (chars "a-z")))
            (+ word "-winner")
          )
        )
      )
    )
  )
  (blank)
  (print (. direct text))
  (print (== direct duplicate))
  (print (. transformed text))
  (print (. (!! optional) text))
  (print (== missing null))
  (print (. ([] words 0) text))
  (print (. ([] words 1) text))
  (print (. ([] words 2) text))
  (print (. rolled-back text))
  (print (. ([] (. helper pool) 5) text))
  (print (. helper pool length))
  (print (. helper string-capture-count))
  (print (. helper string-value-count))
  (print (. helper atom-capture-count))
  (print (. helper atom-value-count))
  (print (. helper mark-count))
  (call (. helper pool release))
  (call (. output release))
)

(type + AtomPatternHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot String))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + pool AtomPool)
  (prop + string-capture-count Size)
  (prop + string-value-count Size)
  (prop + atom-capture-count Size)
  (prop + atom-value-count Size)
  (prop + mark-count Size)
  (blank)
  (delegate (of PatternCaptureMaterializer Char Atom) (of PatternCaptureMaterializer Byte Atom))
  (delegate (of PatternValueMaterializer String Atom))
  (prop _ atom-materializer _ (:= (call AtomAtomPatternMaterializer this)))
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + This _
    (param source Arena)
    (= arena source)
    (= pool (call AtomPool))
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= string-capture-count 1)
    (return (call (. arena copy-span) source start end))
  )
  (fn-do + materialize-value String
    (param input String)
    (+= string-value-count 1)
    (return (call (. arena copy-string) input))
  )
  (fn-expr + mark Size
    (before
      (do
        (+= mark-count 1)
      )
    )
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)

(type _ AtomAtomPatternMaterializer (is Tuple)
  (implements (of PatternCaptureMaterializer Char Atom))
  (implements (of PatternCaptureMaterializer Byte Atom))
  (implements (of PatternValueMaterializer String Atom))
  (prop _ helper AtomPatternHelper)
  (fn-do + This _
    (param source AtomPatternHelper)
    (= helper source)
  )
  (fn-do + (of materialize-capture Token) Atom
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= (. helper atom-capture-count) 1)
    (return (call (. helper pool store) source start end))
  )
  (fn-do + materialize-value Atom
    (param input String)
    (+= (. helper atom-value-count) 1)
    (return (call (. helper pool store) input))
  )
)
```

```output
alpha
true
alpha-x
beta
true
a
b
a
gamma-winner
gamma-failed
7
3
0
6
3
0
```

### A type named as a part of a pattern runs its stream constructor under the match's helper.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (let helper (call CollectorCounter arena))
  (let found
    (match "[1,2,3]"
      (with helper)
      (~ (: digits Digits)
        digits
      )
      (throw (call Panic "no digits"))
    )
  )
  (print (. found items))
  (print (> (. helper asked) 0))
  (call (. arena release))
)

(type + Digits (is Tuple)
  (prop + items (list Int))
  (fn-expr + This _ (over: Char)
    (~ (seq "[" (: items (call \delimited (~ ",") (~ \digit))) "]")
      (call This items)
    )
    (where
      (lazy digit Int
        (pattern
          (~ (chars "0-9")
            (as it Int)
          )
        )
      )
    )
  )
)

(type + CollectorCounter
  (implements (of PatternCollectorStorage ArenaBacking))
  (prop + arena Arena)
  (prop + asked Size)
  (fn-do + This _
    (param source Arena)
    (= arena source)
  )
  (fn-do + pattern-collector-context Arena :get
    (+= asked 1)
    (return arena)
  )
)
```

```output
1, 2, 3
true
```

### A local rule a pattern given to a rule calls under a helper makes the type it states from the text it matched where the helper has no materializer for that type.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (let helper (call Storage arena))
  (let value
    (match "[1.5,2.5]"
      (with helper)
      (~ \numbers)
      (throw (call Panic "no numbers"))
    )
  )
  (print value)
  (call (. arena release))
)

(fn-expr + numbers (list Float) (over: Char)
  (~ (seq "[" (: items (call \delimited (~ ",") (~ \float))) "]")
    items
  )
  (where
    (lazy float Float
      (pattern
        (~ (seq (+ (chars "0-9")) "." (+ (chars "0-9"))))
      )
    )
  )
)

(type + Storage
  (implements (of PatternCollectorStorage ArenaBacking))
  (prop + arena Arena)
  (fn-do + This _
    (param source Arena)
    (= arena source)
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
)
```

```output
1.5, 2.5
```

### A local rule of a match that runs an action, given alone as a pattern to a rule, runs in the function made of the pattern.

```brix
(main
  (print
    (match "1,2"
      (~ (call \delimited (~ ",") (~ \digit)))
      (throw (call Panic "no digits"))
      (blank)
      (lazy digit Int
        (pattern
          (~ (chars "0-9")
            (* (call Int it) 10)
          )
        )
      )
    )
  )
)
```

```output
10, 20
```

### Reference variable captured earlier in pattern

```brix
(main
  (print
    (match "5ooooo"
      (~ (seq (: len \length) (: o ({} "o" len)))
        o
      )
      (blank)
      (lazy length
        (pattern
          (~ (: d (+ (chars "0-9")))
            (as d Int)
          )
        )
      )
    )
  )
)
```

```output
ooooo
```

```brix
(main
  (let x
    (match "foo"
      (~ "bar"
        42
      )
      (default
        (inline 41)
      )
    )
  )
  (print (type-of x))
  (print x)
)
```

```output
[TypeInfo I64]
41
```

```brix
(main
  (let x (call parse "bar"))
  (print (type-of x))
  (print x)
  (blank)
  (print (catch (call parse "foo")))
)

(fn-expr + parse _
  (param text _)
  (match text
    (~ "bar"
      42
    )
    (throw (call SyntaxError "No bar"))
  )
)
```

```output
[TypeInfo I64]
42
No bar
```

```brix
(main
  (print (~ (of \num I16) (of: "0")))
  (print (~ (of \num I16) (of: "-0")))
  (print (~ (of \num I16) (of: "-5")))
  (print (~ (of \num I16) (of: "05")))
)
```

```output
0
0
-5
unknown
```

### An unsigned number has no minus sign to read, so a number written with one does not match it.

```brix
(main
  (print (~ (of \num U16) (of: "7")))
  (print (~ (of \num U16) (of: "-7")))
  (print (~ (of \num U16) (of: "-0")))
  (print (call unsigned "-3"))
  (print (call unsigned "12"))
)

(fn-expr _ unsigned String
  (param text String)
  (match text
    (~ (: parsed U64)
      (str parsed)
    )
    (default
      (inline "rejected")
    )
  )
)
```

```output
7
unknown
unknown
rejected
12
```

```brix
(main
  (print (~ (of \dec Float) (of: "0")))
  (print (~ (of \dec Float) (of: "-0")))
  (print (~ (of \dec Float) (of: ".5")))
  (print (~ (of \dec Float) (of: "-.5")))
  (print (~ (of \dec Float) (of: "0.5")))
  (print (~ (of \dec Float) (of: "-0.5")))
)
```

```output
0
-0
0.5
-0.5
0.5
-0.5
```

```brix
(main
  (print (~ Float (of: "-0.5")))
  (print (~ I8 (of: "-5")))
  (print (~ I8 (of: "5")))
  (print (~ I8 (of: "257")))
  (print (~ Foo (of: "2")))
)

(type + Foo (is U8)
  (fn-expr + This _ (over: Char)
    (~ (chars "0-9")
      42
    )
  )
)
```

```output
-0.5
-5
5
127
42
```

```brix
(main
  (print (~ (seq "foo" / "bar" / "baz") (of: "foo barbaz")))
  (print (~ (seq "foo" / "bar" // "baz") (of: "foo barbaz")))
  (print (~ (seq "foo" / "bar") (of: "foo   bar")))
)
```

```output
foo barbaz
unknown
foo   bar
```

```brix
(main
  (print (~ (of \hex U32) (of: "F")))
  (print (= (select v (~ (seq (: v (of \hex U16)) "F"))) "FFFFF"))
)
```

```output
15
65535
```

```brix
(main
  (print
    (match "A"
      (~ (seq (! \nl) .))
    )
  )
)
```

```output
A
```

## Captures inside of groups

### Captures inside optional group should bind to an optional value

```brix
(main
  (print
    (match "foob"
      (~ (seq "foo" (? (: bar "bar")))
        bar
      )
    )
  )
)
```

```output
unknown
```

### Captures inside repeat group should bind to a list

```brix
(main
  (print
    (match "foobarbar"
      (~ (seq "foo" (* (: bar "bar")))
        bar
      )
    )
  )
)
```

```output
bar, bar
```

## Pattern functions as normal functions

```brix crash
(main
  (print (~ \foo (of: "foo")))
)

(fn-expr + foo _
  42
)
```

```output
Exception: "foo" is a function, not a rule; call it outside the pattern
<no file:3>
      (print (~ \foo (of: "foo")))
                ^^^^
```

```brix
(main
  (print (~ \foo (of: "foo")))
)

(fn-expr + foo _ (over: Char)
  (~ "foo"
    42
  )
)
```

```output
42
```

```brix
(main
  (print (~ (call \foo 5) (of: "foo")))
  (print (~ (call \foo 5) (of: "bar")))
  (print (~ (call \foo 1) (of: "foo")))
  (print (~ (call \foo 1) (of: "bar")))
)

(fn-expr + foo _ (over: Char)
  (param n _)
  (case? (> 3 n)
    (~ "foo")
  )
  (default
    (~ "bar")
  )
)
```

```output
unknown
bar
foo
unknown
```

```brix
(main
  (let (tuple aa bb)
    (match "foo bar"
      (~ (seq (: f (+ (chars "a-z"))) // (: b (+ (chars "a-z"))))
        (tuple f b)
      )
      (default
        (inline (tuple "x" "x"))
      )
    )
  )
  (print aa)
  (print bb)
)
```

```output
foo
bar
```

```brix
(import /core/tests/support/byte-source)
(blank)

(main
  (let stream (call TestByteSource (call (of Array Char) "foo")))
  (print (~ "foo" (of: stream)))
)
```

```output
foo
```

```brix
(main
  (print (call (. "\n" join) (in (~ (seq "f" (+ (chars "o")))) "fofooooofoo")))
)
```

```output
fo
fooooo
foo
```

```brix
(main
  (let m
    (iterate (in "fogooooofoo")
      (block
        (~ (seq "f" (+ (chars "o")))
          10.5
        )
        (~ (seq "g" (+ (chars "o")))
          20
        )
      )
    )
  )
  (for (in x m)
    (print x)
  )
)
```

```output
10.5
20
10.5
```

```brix
(main
  (let m
    (iterate (in "fogooooofoo")
      (block
        (~ (seq "f" (+ (chars "o")))
          10.5
        )
        (~ (seq "g" (+ (chars "o")))
          20
        )
      )
    )
  )
  (for (in x m)
    (print x)
  )
)
```

```output
10.5
20
10.5
```

```brix
(main
  (let m
    (iterate (in "fogooooofoo")
      (block
        (~ \foo)
      )
    )
  )
  (for (in x m)
    (print x)
  )
)

(fn-expr + foo _ (over: Char)
  (~ (seq "f" (+ (chars "o")))
    10.5
  )
  (~ (seq "g" (+ (chars "o")))
    20
  )
)
```

```output
10.5
20
10.5
```

```brix
(main
  (for (in (~ (seq "f" (: a (+ "o")))) "fofoofoooofo")
    (print each)
  )
)
```

```output
fo
foo
foooo
fo
```

```brix
(main
  (for (in (~ (seq "f" (: a (+ "o")))) "fofoofoooofo")
    (print a)
  )
)
```

```output
o
oo
oooo
o
```

```brix
(main
  (for (in (~ (seq "f" (: a (+ "o")))) "fofoofoooofo")
    (and-for (in i (..< 0 3))
      (print (tuple i a))
    )
  )
)
```

```output
(0, o)
(1, oo)
(2, oooo)
```

```brix
(main
  (print
    (match "f"
      (~ (: c "x")
        1
      )
      (~ (: c \char)
        c
      )
      (blank)
      (lazy char
        (pattern
          (~ (: c (chars "a-z"))
            2
          )
        )
      )
    )
  )
)
```

```output
2
```

### Span-backed captures convert through ordinary constructor argument dispatch.

```brix
(main
  (let result CapturedText
    (match "alpha!"
      (~ (seq (: text (+ (chars "a-z"))) "!")
        (call CapturedText text)
      )
      (throw (call Panic "constructor capture did not parse"))
    )
  )
  (blank)
  (print result)
)

(type + CapturedText (is (tuple (: text String))))
```

```output
(alpha)
```

### Capture-span equality can be the transform result without changing its Bool return mode.

```brix
(main
  (print
    (match "alpha!"
      (~ (seq (: text (+ (chars "a-z"))) "!")
        (== text "alpha")
      )
      (default
        (inline false)
      )
    )
  )
  (blank)
  (print
    (match "alpha:alpha!"
      (~ (seq (: left (+ (chars "a-z"))) ":" (: right (+ (chars "a-z"))) "!")
        (== left right)
      )
      (default
        (inline false)
      )
    )
  )
)
```

```output
true
true
```

### Source-equivalent choice joins preserve span results without changing other choice result types.

```brix
(main
  (print
    (match "alpha!"
      (~ (seq (: text \choice-text) "!")
        (. text length)
      )
      (default
        (inline 0)
      )
    )
  )
  (blank)
  (print
    (match "42!"
      (~ (seq (: text \choice-text) "!")
        (== text "42")
      )
      (default
        (inline false)
      )
    )
  )
  (blank)
  (print
    (match "nested!"
      (~ (seq (: text \outer-choice-text) "!")
        (. text length)
      )
      (default
        (inline 0)
      )
    )
  )
  (blank)
  (print
    (match "sized!"
      (~ (seq (: text (call \sized-choice-text 5)) "!")
        (. text length)
      )
      (default
        (inline 0)
      )
    )
  )
  (blank)
  (print
    (match "((x))!"
      (~ (seq (: text \recursive-source-text) "!")
        (. text length)
      )
      (default
        (inline 0)
      )
      (blank)
      (lazy recursive-source-text String
        (pattern
          (~ (chars "a-z"))
          (~ (seq "(" \recursive-source-text ")"))
        )
      )
    )
  )
  (blank)
  (print
    (match "x"
      (~ "x"
        7
      )
      (~ "y"
        false
      )
      (default
        (inline 0)
      )
    )
  )
)

(fn-expr + choice-text _ (over: Char)
  (~ (+ (chars "a-z")))
  (~ (+ (chars "0-9")))
)

(fn-expr + outer-choice-text _ (over: Char)
  (~ \choice-text)
)

(fn-expr + sized-choice-text _ (over: Char)
  (param count Size)
  (~ ({} (chars "a-z") count))
)
```

```output
5
true
6
5
5
7
```

### A capture may have the name of the rule it calls, and a later call of that rule still calls it.

```brix
(main
  (print (call parse "ab=cd"))
)

(fn-expr + parse String
  (param text String)
  (match text
    (~ \assignments
      it
    )
    (default
      (inline "none")
    )
  )
)

(fn-expr + assignments String (over: Char)
  (~ \assignment)
  (where
    (lazy word String
      (pattern
        (~ (+ (chars "a-z")))
      )
    )
    (blank)
    (lazy assignment String
      (pattern
        (~ (seq (: word \word) "=" (: value \word))
          (str word ":" value)
        )
      )
    )
  )
)
```

```output
ab:cd
```

### An action gives a rule its type where it reads an optional field through an optional capture, as `?.` of an optional field is that field's own optional.

```brix
(main
  (print (call parse "!x:7"))
  (print (call parse "!:7"))
)

(type + Selector (is (tuple (: object (? Int)) (: key String))))

(fn-expr + parse Int
  (param text String)
  (match text
    (~ \lines
      it
    )
    (default
      (inline -1)
    )
  )
)

(fn-expr + lines Int (over: Char)
  (~ \line)
  (where
    (lazy line
      (pattern
        (~ (seq "!" (: selector (? \key-selector)) ":" (chars "0-9"))
          (call or-zero (?. selector object))
        )
      )
    )
    (blank)
    (lazy key-selector Selector
      (pattern
        (~ (chars "a-z")
          (call Selector 5 "k")
        )
      )
    )
  )
)

(fn-expr _ or-zero Int
  (param value (? Int))
  (if-expr value value 0)
)
```

```output
5
0
```

### Repeated capture names use spans when every occurrence is source-equivalent, and otherwise retain their original concrete String type without changing eligibility during type inference.

```brix
(main
  (print
    (match "alpha!"
      (~ (seq (| (: text (+ (chars "a-z"))) (: text (+ (chars "0-9")))) "!")
        (. text length)
      )
      (default
        (inline 0)
      )
    )
  )
  (blank)
  (print
    (match "42!"
      (~ (seq (| (: text (+ (chars "a-z"))) (: text (+ (chars "0-9")))) "!")
        (== text "42")
      )
      (default
        (inline false)
      )
    )
  )
  (blank)
  (print
    (match "alpha!"
      (~ (seq (| (: text (+ (chars "0-9"))) (: text \transformed-conflict-text)) "!")
        (== text "alpha")
      )
      (default
        (inline false)
      )
    )
  )
)

(fn-expr + transformed-conflict-text _ (over: Char)
  (~ (+ (chars "a-z"))
    it
  )
)
```

```output
5
true
true
```

### A transform that does not reference `it` must not need the whole-match value.

```brix
(main
  (print
    (match "alpha!"
      (~ (seq (+ (chars "a-z")) "!")
        7
      )
      (default
        (inline 0)
      )
    )
  )
)
```

```output
7
```

```brix
(main
  (print
    (match "ooo"
      (~ (: ohs (+ \o))
        ohs
      )
      (blank)
      (lazy o
        (pattern
          (~ "o"
            42
          )
        )
      )
    )
  )
)
```

```output
42, 42, 42
```

## Optional subcaptures

```brix
(main
  (print
    (match "foo/bar"
      (~ (seq "foo" (? (seq "/" (: bar "bar"))))
        bar
      )
    )
  )
)
```

```output
bar
```

### Joined optional exits must preserve the path-specific Some/None capture.

```brix
(main
  (print (call parse-optional-capture "foo/bar"))
  (print (call parse-optional-capture "foo"))
)

(fn-expr + parse-optional-capture _
  (param source String)
  (match source
    (~ (seq "foo" (? (seq "/" (: bar "bar"))))
      (?? bar "missing")
    )
    (throw (call Panic "optional capture did not parse"))
  )
)
```

```output
bar
missing
```

```brix
(main
  (let result
    (match "foo/bar"
      (~ (seq "foo" (? (seq "/" (: bar \poop))))
        bar
      )
      (blank)
      (lazy poop
        (pattern
          (~ "bar"
            42
          )
        )
      )
    )
  )
  (blank)
  (print (type-of result))
  (print result)
)
```

```output
[TypeInfo Optional'Optional'I64]
42
```

### Fails due to lazy variable issues

```brix
(main
  (print
    (match "foo bar"
      (~ (seq "foo" (? (seq // (: bar "bar"))))
        bar
      )
    )
  )
)
```

```output
bar
```

```brix
(main
  (print
    (match "2"
      (~ (seq (: num "2") (: exp (? "e")) (: u (? "u")))
        exp
      )
    )
  )
)
```

```output
unknown
```

```brix
(main
  (print
    (match "foo bar bez"
      (~ (seq (: foo "foo") (: bar (? \bars)) (: baz (? \bars)) (* .))
        (tuple bar baz)
      )
      (blank)
      (lazy bars
        (pattern
          (~ " bar")
        )
      )
    )
  )
)
```

```output
( bar, unknown)
```

```brix
(main
  (print
    (match "foaoaoa"
      (~ (seq "f" (* (seq "o" (: a "a"))))
        (call (. " " join) a)
      )
    )
  )
)
```

```output
a a a
```

```brix
(main
  (print
    (match "foaoaoa"
      (~ (seq "f" (+ (seq "o" (: a "a"))))
        (call (. " " join) a)
      )
    )
  )
)
```

```output
a a a
```

```brix
(main
  (print
    (match "df *f * * *f * *"
      (~ (seq "d" (* (seq "f" (* (seq " " (: asterisks "*"))))))
        ; type-of asterisks
        (call (. " " join) (in (call merge each) asterisks)
          (block
            (lazy merge (=> (x) (call (. "/" join) x)))
          )
        )
      )
    )
  )
)
```

```output
* */*/* */*
```

```brix
(main
  (print
    (match "df *f * * *f * *"
      (~ (seq "d" (+ (seq "f" (+ (seq " " (: asterisks "*"))))))
        ; type-of asterisks
        (call (. " " join) (in (call merge each) asterisks)
          (block
            (lazy merge (=> (x) (call (. "/" join) x)))
          )
        )
      )
    )
  )
)
```

```output
* */*/* */*
```

## Imperative conditional matches

```brix
(main
  (if (= (~ (seq "f" (: o ({} "o" 3)))) "fooo")
    (print (str "pass " o))
    (print "ok")
    (else
      (print "fail")
    )
  )
)
```

```output
pass ooo
ok
```

```brix
(main
  (if (= (~ (seq "f" (: o ({} "o" 8)))) "foooo")
    (print (str "pass " o))
    (print "ok")
    (else
      (print "fail")
    )
  )
)
```

```output
fail
```

```brix
(main
  (let x 4)
  (while (= (~ (seq "f" (: o ({} "o" x)) (* .))) "foooo")
    (print (str "pass " o))
    (-= x 1)
  )
)
```

```output
pass oooo
pass ooo
pass oo
pass o
pass
```

## Location tracking

```brix
(main
  (match "xfoo\nxxfooo\nfo\n"
    (~ (+ (seq (* "x") \foo \nl)))
    (blank)
    (lazy foo
      (pattern
        (~ (seq "f" (+ "o"))
          (do
            (inline (print location))
          )
          ""
        )
      )
    )
  )
)
```

```output
((0, 1), (0, 3))
((1, 2), (1, 5))
((2, 0), (2, 1))
```

## Object streams

```brix
(main
  (let nums (list 1 2 3 4))
  (print
    (match nums
      (~ (+ (seq (: odd (\{} (% it 2))) (: even (\{} (== (% it 2) 0)))))
        (tuple odd even)
      )
    )
  )
)
```

```output
(1, 3, 2, 4)
```

### Any-item patterns match object items, not only characters.

```brix
(main
  (let words (list String) (list "-o" "a" "b"))
  (print
    (match words
      (~ (seq . (: rest (* .)))
        (. rest length)
      )
    )
  )
  (print
    (match words
      (~ (seq . . . (! .))
        "three"
      )
      (default
        (inline "not three")
      )
    )
  )
)
```

```output
2
three
```

### A capture over items that aren't characters is read only for its length or an item.

```brix compile-error
(main
  (let numbers (list Int) (list 1 2 3))
  (print
    (match numbers
      (~ (seq . (: rest (* .)))
        (str rest)
      )
      (default
        (inline "none")
      )
    )
  )
)
```

```output
Exception: A capture over items that aren't characters can only be read for its length or an item
<no file:7>
            (str rest)
                 ^^^^
```

### Character classes need character or integer items.

```brix compile-error
(main
  (let words (list String) (list "a"))
  (print
    (match words
      (~ (chars "a-z")
        "letter"
      )
      (default
        (inline "none")
      )
    )
  )
)
```

```output
Exception: Character patterns can only match character or integer items
<no file:6>
          (~ (chars "a-z")
          ^^^^^^^^^^^^^^^
```

### Text and character classes test integer items against their code points.

```brix
(main
  (let bytes (call (of Array Byte) "GET /x"))
  (print
    (match bytes
      (~ (seq "GET " (: rest (* .)))
        (. rest length)
      )
      (default
        (inline 0)
      )
    )
  )
  (let points (list Int) (list 104 105 33))
  (print
    (match points
      (~ (seq (: word (+ (chars "a-z"))) "!")
        (. word length)
      )
      (default
        (inline 0)
      )
    )
  )
)
```

```output
2
2
```

### Characters in a pattern over bytes are ASCII.

```brix compile-error
(main
  (print
    (match (call (of Array Byte) "é")
      (~ "é"
        "found"
      )
      (default
        (inline "none")
      )
    )
  )
)
```

```output
Exception: Characters in a pattern over bytes must be ASCII; match other bytes by number
<no file:5>
          (~ "é"
             ^^^
```

### Text needs character or integer items.

```brix compile-error
(main
  (let words (list String) (list "-o" "a"))
  (print
    (match words
      (~ (seq "-o" .)
        "flag"
      )
      (default
        (inline "none")
      )
    )
  )
)
```

```output
Exception: Text in a pattern can only match character or integer items
<no file:6>
          (~ (seq "-o" .)
                  ^^^^
```

```brix
(main
  (let events (channel Event))
  (do-new
    (<- events (call ClickEvent))
    (<- events (call ClickEvent))
    (<- events (call KeyEvent))
    (call (. events close))
  )
  (blank)
  (print
    (match events
      (~ (seq (: click (* ClickEvent)) (: key KeyEvent))
        click
      )
    )
  )
)

(type + Event)

(type + ClickEvent (is Event))

(type + KeyEvent (is Event))
```

```output
[Object __source__:ClickEvent], [Object __source__:ClickEvent]
```

```brix
(import /core/tests/support/byte-source)
(blank)

(main
  (let stream-data ([] Byte 5))
  (let stream-size Size 0)
  (call (. stream-data write) 4u32 stream-size)
  (+= stream-size 4)
  (= ([] stream-data stream-size) 2u8)
  (+= stream-size 1)
  (let stream (call TestByteSource ([] stream-data (..< 0 stream-size))))
  (blank)
  (print
    (~ (seq (: a U32) (: b U8)) (of: stream)
      (tuple a b)
    )
  )
)
```

```output
(4, 2)
```

```brix
(import /core/tests/support/byte-source)
(blank)

(main
  (let stream-data ([] Byte 8))
  (let stream-size Size 0)
  (call (. stream-data write) 4u32 stream-size)
  (+= stream-size 4)
  (= ([] stream-data stream-size) 1u8)
  (+= stream-size 1)
  (= ([] stream-data stream-size) 2u8)
  (+= stream-size 1)
  (= ([] stream-data stream-size) 3u8)
  (+= stream-size 1)
  (= ([] stream-data stream-size) 4u8)
  (+= stream-size 1)
  (let stream (call TestByteSource ([] stream-data (..< 0 stream-size))))
  (blank)
  (print
    (~ (seq (: a U32) (: b ({} U8 a))) (of: stream)
      (tuple ([] b 0) ([] b 1) ([] b 2) ([] b 3))
    )
  )
)
```

```output
(1, 2, 3, 4)
```

```brix
(import /core/tests/support/byte-source)
(blank)

(main
  (let stream-data ([] Byte 8))
  (let stream-size Size 0)
  (call (. stream-data write) 4u32 stream-size)
  (+= stream-size 4)
  (= ([] stream-data stream-size) 1u8)
  (+= stream-size 1)
  (= ([] stream-data stream-size) 2u8)
  (+= stream-size 1)
  (= ([] stream-data stream-size) 3u8)
  (+= stream-size 1)
  (= ([] stream-data stream-size) 4u8)
  (+= stream-size 1)
  (let stream (call TestByteSource ([] stream-data (..< 0 stream-size))))
  (blank)
  (print
    (~ (seq (: a U32) (: b (+ U8))) (of: stream)
      b
    )
  )
)
```

```output
1, 2, 3, 4
```

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (let short-data ([] Byte 3))
  (let short-size Size 0)
  (= ([] short-data short-size) 2u8)
  (+= short-size 1)
  (= ([] short-data short-size) 40u8)
  (+= short-size 1)
  (= ([] short-data short-size) 41u8)
  (+= short-size 1)
  (let short (call TestByteSource ([] short-data (..< 0 short-size))))
  (let short-stream (call (of ChunkedTokenStream Byte) short))
  (blank)
  (let wide16-data ([] Byte 6))
  (let wide16-size Size 0)
  (= ([] wide16-data wide16-size) 126u8)
  (+= wide16-size 1)
  (= ([] wide16-data wide16-size) 0u8)
  (+= wide16-size 1)
  (= ([] wide16-data wide16-size) 3u8)
  (+= wide16-size 1)
  (= ([] wide16-data wide16-size) 5u8)
  (+= wide16-size 1)
  (= ([] wide16-data wide16-size) 6u8)
  (+= wide16-size 1)
  (= ([] wide16-data wide16-size) 7u8)
  (+= wide16-size 1)
  (let wide16 (call TestByteSource ([] wide16-data (..< 0 wide16-size))))
  (let wide16-stream (call (of ChunkedTokenStream Byte) wide16))
  (blank)
  (let wide64-data ([] Byte 13))
  (let wide64-size Size 0)
  (= ([] wide64-data wide64-size) 127u8)
  (+= wide64-size 1)
  (= ([] wide64-data wide64-size) 0u8)
  (+= wide64-size 1)
  (= ([] wide64-data wide64-size) 0u8)
  (+= wide64-size 1)
  (= ([] wide64-data wide64-size) 0u8)
  (+= wide64-size 1)
  (= ([] wide64-data wide64-size) 0u8)
  (+= wide64-size 1)
  (= ([] wide64-data wide64-size) 0u8)
  (+= wide64-size 1)
  (= ([] wide64-data wide64-size) 0u8)
  (+= wide64-size 1)
  (= ([] wide64-data wide64-size) 0u8)
  (+= wide64-size 1)
  (= ([] wide64-data wide64-size) 4u8)
  (+= wide64-size 1)
  (= ([] wide64-data wide64-size) 8u8)
  (+= wide64-size 1)
  (= ([] wide64-data wide64-size) 9u8)
  (+= wide64-size 1)
  (= ([] wide64-data wide64-size) 10u8)
  (+= wide64-size 1)
  (= ([] wide64-data wide64-size) 11u8)
  (+= wide64-size 1)
  (let wide64 (call TestByteSource ([] wide64-data (..< 0 wide64-size))))
  (let wide64-stream (call (of ChunkedTokenStream Byte) wide64))
  (blank)
  (print (~ ShortPacket (of: short-stream)))
  (print (~ Wide16Packet (of: wide16-stream)))
  (print (~ Wide64Packet (of: wide64-stream)))
)

(type + ShortPacket (is Tuple)
  (prop + len U64)
  (prop + skipped Size)
  (prop + size Size)
  (prop + first Byte)
  (prop + last Byte)
  (fn-expr + This _ (over: Byte)
    (~ (seq (: len U8) (: empty ({} U8 0)) (: payload ({} Byte len)))
      (call This (as len U64) (. empty length) (. payload length) ([] payload 0) ([] payload (- (. payload length) 1)))
    )
  )
)

(type + Wide16Packet (is Tuple)
  (prop + marker U8)
  (prop + len U64)
  (prop + size Size)
  (prop + first Byte)
  (prop + last Byte)
  (fn-expr + This _ (over: Byte)
    (~ (seq (: marker \read-u8) (: len \read-u16) (: payload ({} Byte (as len U64))))
      (call This marker (as len U64) (. payload length) ([] payload 0) ([] payload (- (. payload length) 1)))
    )
  )
)

(type + Wide64Packet (is Tuple)
  (prop + marker U8)
  (prop + len U64)
  (prop + size Size)
  (prop + first Byte)
  (prop + last Byte)
  (fn-expr + This _ (over: Byte)
    (~ (seq (: marker \read-u8) (: len \read-u64) (: payload ({} Byte len)))
      (call This marker len (. payload length) ([] payload 0) ([] payload (- (. payload length) 1)))
    )
  )
)

(fn-do + (of read-u8 Item) U8
  (param stream (of TokenStream Byte))
  (let buf ([] Byte 1))
  (call (. stream read) 1 buf)
  (return ([] buf 0))
)

(fn-do + (of read-u16 Item) U16
  (param stream (of TokenStream Byte))
  (let buf ([] Byte 2))
  (call (. stream read) 2 buf)
  (return (/or/ (/lshift/ (as ([] buf 0) U16) 8) (as ([] buf 1) U16)))
)

(fn-do + (of read-u64 Item) U64
  (param stream (of TokenStream Byte))
  (let value (as 0 U64))
  (let buf ([] Byte 8))
  (call (. stream read) 8 buf)
  (for (in byte buf)
    (= value (/or/ (/lshift/ value 8) (as byte U64)))
  )
  (return value)
)
```

```output
(2, 0, 2, 40, 41)
(126, 3, 3, 5, 7)
(127, 4, 4, 8, 11)
```

```brix
(import /core/tests/support/byte-source)
(blank)

(main
  (let masked-data ([] Byte 11))
  (let masked-size Size 0)
  (= ([] masked-data masked-size) 1u8)
  (+= masked-size 1)
  (= ([] masked-data masked-size) 5u8)
  (+= masked-size 1)
  (= ([] masked-data masked-size) 10u8)
  (+= masked-size 1)
  (= ([] masked-data masked-size) 20u8)
  (+= masked-size 1)
  (= ([] masked-data masked-size) 30u8)
  (+= masked-size 1)
  (= ([] masked-data masked-size) 40u8)
  (+= masked-size 1)
  (= ([] masked-data masked-size) 11u8)
  (+= masked-size 1)
  (= ([] masked-data masked-size) 22u8)
  (+= masked-size 1)
  (= ([] masked-data masked-size) 29u8)
  (+= masked-size 1)
  (= ([] masked-data masked-size) 44u8)
  (+= masked-size 1)
  (= ([] masked-data masked-size) 15u8)
  (+= masked-size 1)
  (let masked (call TestByteSource ([] masked-data (..< 0 masked-size))))
  (blank)
  (let plain-data ([] Byte 5))
  (let plain-size Size 0)
  (= ([] plain-data plain-size) 0u8)
  (+= plain-size 1)
  (= ([] plain-data plain-size) 3u8)
  (+= plain-size 1)
  (= ([] plain-data plain-size) 8u8)
  (+= plain-size 1)
  (= ([] plain-data plain-size) 9u8)
  (+= plain-size 1)
  (= ([] plain-data plain-size) 10u8)
  (+= plain-size 1)
  (let plain (call TestByteSource ([] plain-data (..< 0 plain-size))))
  (blank)
  (let empty-data ([] Byte 7))
  (let empty-size Size 0)
  (= ([] empty-data empty-size) 1u8)
  (+= empty-size 1)
  (= ([] empty-data empty-size) 0u8)
  (+= empty-size 1)
  (= ([] empty-data empty-size) 10u8)
  (+= empty-size 1)
  (= ([] empty-data empty-size) 20u8)
  (+= empty-size 1)
  (= ([] empty-data empty-size) 30u8)
  (+= empty-size 1)
  (= ([] empty-data empty-size) 40u8)
  (+= empty-size 1)
  (= ([] empty-data empty-size) 99u8)
  (+= empty-size 1)
  (let empty (call TestByteSource ([] empty-data (..< 0 empty-size))))
  (blank)
  (print (call decode-keyed masked))
  (print (call decode-keyed plain))
  (print (call decode-empty empty))
)

(fn-expr + decode-keyed _
  (param stream _)
  (match stream
    (~ (seq (: keyed U8) (: len U8) (: key ({} Byte (if-expr keyed 4 0))) (: payload ({} Byte len)))
      (case? (and keyed len)
        (call decoded-checksum key payload)
      )
      (case? len
        (call checksum payload)
      )
      (default
        0
      )
    )
    (throw (call Panic "keyed packet did not parse"))
  )
)

(fn-expr + decode-empty _
  (param stream _)
  (match stream
    (~ (seq (: keyed U8) (: len U8) (: key ({} Byte (if-expr keyed 4 0))) (: payload ({} Byte len)) (: tail U8))
      (case? len
        (call decoded-checksum key payload)
      )
      (default
        (as tail Int)
      )
    )
    (throw (call Panic "empty packet did not parse"))
  )
)

(fn-do + checksum _
  (param data (stream Byte))
  (let total 0)
  (for (in byte data)
    (+= total (as byte Int))
  )
  (blank)
  (return total)
)

(fn-expr + decode-byte _
  (param key (of ImmutableArray Byte))
  (param byte Byte)
  (param index Size)
  (/xor/ byte ([] key (% index 4)))
)

(fn-do + decoded-checksum _
  (param key (of ImmutableArray Byte))
  (param payload (of ImmutableArray Byte))
  (let total 0)
  (let index Size 0)
  (for (in byte payload)
    (+= total (as (call decode-byte key byte index) Int))
    (+= index 1)
  )
  (return total)
)
```

```output
15
27
99
```

### Acyclic local Byte rules can share one frame without returning views into its reusable buffer.

```brix
(import /core/tests/support/byte-source)
(blank)

(main
  (let stream-data ([] Byte 4))
  (let stream-size Size 0)
  (= ([] stream-data stream-size) 1u8)
  (+= stream-size 1)
  (= ([] stream-data stream-size) 2u8)
  (+= stream-size 1)
  (= ([] stream-data stream-size) 3u8)
  (+= stream-size 1)
  (= ([] stream-data stream-size) 4u8)
  (+= stream-size 1)
  (let stream (call TestByteSource ([] stream-data (..< 0 stream-size))))
  (blank)
  (print
    (match stream
      (~ (seq (: a \item) (: b \item))
        (tuple ([] a 0) ([] a 1) ([] b 0) ([] b 1))
      )
      (blank)
      (lazy item (of ImmutableArray Byte)
        (pattern
          (~ ({} Byte 2))
        )
      )
    )
  )
)
```

```output
(1, 2, 3, 4)
```

```brix
(main
  (print
    (match "ooo"
      (~ ({} "o" x)
        (lazy x 3)
      )
    )
  )
)
```

```output
ooo
```

### Fails due to lazy variable issues

```brix
(main
  (print
    (match "foooo"
      (~ (seq (: f "f") ({} "o" x))
        (lazy x (+ 3 (. f length)))
      )
    )
  )
)
```

```output
foooo
```

```brix
(main
  (print
    (match "foooo"
      (~ (seq (: f (+ "f")) (: o (call \foo x)))
        o
        (lazy x (+ 3 (. f length)))
      )
    )
  )
)

(fn-expr + foo _ (over: Char)
  (param x _)
  (~ ({} "o" x))
)
```

```output
oooo
```

### A count written inline may compare and add what the match reads, a rule's parameters among them, since each gives the same number every time the count is read.

```brix
(main
  (for (in size (~> (call (of Array Byte) (list 3u8 7u8 8u8 9u8 2u8 5u8)) (call \kept 4)))
    (print size)
  )
)

(fn-expr + kept Size (over: Byte)
  (param limit Size)
  (~ (seq (: size U8) (: data ({} Byte (if-expr (<= size (as limit U8)) (- size 1) 0))))
    (. data size)
  )
)
```

```output
2
0
1
```

### A count that may give another number each time it is read is an error that says to name it.

```brix compile-error
(main
  (print
    (match (call (of Array Byte) (list 2u8 7u8))
      (~ (seq (: size U8) (: data ({} Byte (call half size))))
        (. data size)
      )
    )
  )
)

(fn-expr + half U8
  (param n U8)
  (/ n 2)
)
```

```output
Exception: A count in a pattern is read each time the part repeats, and this one may change between reads; compute it in a "lazy" local and count by its name
<no file:5>
          (~ (seq (: size U8) (: data ({} Byte (call half size))))
                                               ^^^^^^^^^^^^^^^^
```

## Type inference

```brix
(main
  (print
    (match "fo"
      (~ (seq (: f \eff) (: d (call \doop (/or/ f 1))))
        d
      )
      (blank)
      (lazy eff
        (pattern
          (~ "f"
            1
          )
        )
      )
    )
  )
)

(fn-expr + doop _ (over: Char)
  (param x _)
  (~ (: o "o")
    (* x 2)
  )
)
```

```output
2
```

### Verifies that type inference works on return type of a function

```brix
(main
  (print
    (match "03aaa"
      (~ (seq (: size (of \hex U8)) (: chunk ({} . size)))
        chunk
      )
    )
  )
)
```

```output
aaa
```

```brix
(main
  (print
    (match "foo"
      (~ (seq (: f "f") (: o \os))
        o
        (blank)
        (lazy os
          (pattern
            (case? (== f "f")
              (~ (+ "o"))
            )
            (default
              (~ "u")
            )
          )
        )
      )
    )
  )
)
```

```output
oo
```

```brix
(main
  (print
    (match "foaoaoa"
      (~ (: a (? (seq "f" (+ "oa"))))
        a
      )
    )
  )
)
```

```output
foaoaoa
```

### A GC-free region can read and write scalar stream state while its collection remains live.

```brix
(import /core/tokenstream)
(import /BxCore/BxCore.h)
(import /BxGC/BxGC.h)
(blank)

(main
  (let bytes ([] Byte 4))
  (= ([] bytes 0) 1u8)
  (= ([] bytes 1) 2u8)
  (= ([] bytes 2) 3u8)
  (= ([] bytes 3) 4u8)
  (let stream (call (of IndexableTokenStream Byte) bytes))
  (blank)
  (print (call zone-token-checksum stream))
)

(fn-do + zone-token-checksum Size (without gc)
  (param stream (of IndexableTokenStream Byte))
  (= (. stream cursor) 1)
  (let cursor-snapshot (+ (. stream length) (. stream cursor)))
  (call C:brixCollect)
  (+= (. stream cursor) 2)
  (return (+ cursor-snapshot (. stream cursor)))
)
```

```output
8
```

### String, iterable, and byte streams preserve the same buffered-lookahead result.

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (let source (++ "rec:abc:1\nrec:" (* "x" 70) ":22\nrec:tiny:333\n"))
  (let array-rows
    (match source
      (~ \buffered-document)
      (throw (call Panic "array buffered parse failed"))
    )
  )
  (blank)
  (let iterable (call (of ChunkedTokenStream Char) (. source codepoints)))
  (let iterable-rows
    (match iterable
      (~ \buffered-document)
      (throw (call Panic "iterable buffered parse failed"))
    )
  )
  (blank)
  (let data (call (of TestByteSource Char) (. source codepoints)))
  (call (. data rewind))
  (let byte-stream (call (of ChunkedTokenStream Char) data))
  (let byte-rows
    (match byte-stream
      (~ \buffered-document)
      (throw (call Panic "byte buffered parse failed"))
    )
  )
  (blank)
  (print (+ ([] array-rows 0) ([] array-rows 1) ([] array-rows 2)))
  (print (+ ([] iterable-rows 0) ([] iterable-rows 1) ([] iterable-rows 2)))
  (print (+ ([] byte-rows 0) ([] byte-rows 1) ([] byte-rows 2)))
)

(fn-expr + buffered-document _ (over: Char)
  (~ (: rows (+ \buffered-row))
    rows
  )
  (where
    (lazy buffered-row Size
      (pattern
        (~ (seq (& (seq "rec:" (* (seq (! "\n") .)) "\n")) "rec:" (: name \buffered-word) ":" (: value \buffered-digits) "\n")
          (+ (* (. name length) 29) (. value length))
        )
      )
    )
    (blank)
    (lazy buffered-word String
      (pattern
        (~ (seq (chars "a-zA-Z_") (* (chars "a-zA-Z0-9_"))))
      )
    )
    (blank)
    (lazy buffered-digits String
      (pattern
        (~ (+ (chars "0-9")))
      )
    )
  )
)
```

```output
2239
2239
2239
```

### Compile-time literals compare against canonical source windows without changing mismatch, Unicode, EOF, nested-backtracking, or generic-indexable behavior.

```brix
(import /core/tokenstream)
(blank)

(main
  (let samples (list String))
  (+= samples "abcdef")
  (+= samples "xbcdef")
  (+= samples "axcdef")
  (+= samples "abxdef")
  (+= samples "abcxef")
  (+= samples "abcdxf")
  (+= samples "abcdex")
  (+= samples "abcde")
  (+= samples "abcdefx")
  (blank)
  (for (in source samples)
    (print
      (match source
        (~ "abcdef"
          "hit"
        )
        (default
          (inline "miss")
        )
      )
    )
  )
  (blank)
  (print (~ "" (of: "")))
  (print (~ "café 東京 😀" (of: "café 東京 😀")))
  (blank)
  (print
    (match "abcdeg"
      (~ "abcdef"
        "wrong"
      )
      (~ "abcdeg"
        "restored"
      )
      (default
        (inline "miss")
      )
    )
  )
  (blank)
  (let generic (call (of IndexableTokenStream Char) (. "abcdeg" codepoints)))
  (print (call (. generic match-literal) (call (of Array Char) "abcdef")))
  (print (. generic cursor))
  (print (call (. generic match-literal) (call (of Array Char) "abcdeg")))
  (print (. generic cursor))
  (blank)
  (let generic2 (call (of IndexableTokenStream Char) (. "abcdeg" codepoints)))
  (print
    (match generic2
      (~ "abcdef"
        "wrong"
      )
      (~ "abcdeg"
        "generic restored"
      )
      (default
        (inline "miss")
      )
    )
  )
  (blank)
  (let byte-source (call (of Array Byte) (list 1u8 2u8 3u8 4u8)))
  (let byte-literal (call (of Array Byte) (list 1u8 2u8 3u8)))
  (let byte-stream (call (of ArrayTokenStream Byte) byte-source))
  (print (call (. byte-stream match-literal) byte-literal))
  (print (. byte-stream cursor))
  (blank)
  (let mismatch-source (call (of Array Byte) (list 1u8 9u8 3u8 4u8)))
  (let mismatch-stream (call (of ArrayTokenStream Byte) mismatch-source))
  (print (call (. mismatch-stream match-literal) byte-literal))
  (print (. mismatch-stream cursor))
  (print (. mismatch-stream cursor-extent))
  (print (. mismatch-stream error-location column))
)
```

```output
hit
miss
miss
miss
miss
miss
miss
miss
miss

café 東京 😀
restored
false
0
true
6
generic restored
true
3
false
0
3
0
```

### Stable non-String token streams keep raw repeated captures as spans until array escape.

```brix
(import /core/tokenstream)
(blank)

(main
  (let bytes ([] Byte 3))
  (= ([] bytes 0) 1u8)
  (= ([] bytes 1) 2u8)
  (= ([] bytes 2) 3u8)
  (let captured
    (match (call (of IndexableTokenStream Byte) bytes)
      (~ (: payload \payload-bytes)
        payload
      )
      (default
        (inline (call (of ImmutableArray Byte)))
      )
      (blank)
      (lazy payload-bytes (of ImmutableArray Byte)
        (pattern
          (~ ({} Byte 3))
        )
      )
    )
  )
  (blank)
  (print (. captured length))
  (print ([] captured 0))
  (print ([] captured 2))
)
```

```output
3
1
3
```

### Repeated source-backed captures materialize their public String list only on escape.

```brix
(import /core/array-builder)
(blank)

(main
  (let repeated
    (match "a,b,c!"
      (~ (seq (+ (seq (: part (chars "a-z")) (? ","))) "!")
        part
      )
      (default
        (inline (call empty-string-sequence))
      )
    )
  )
  (blank)
  (print repeated)
)

(fn-do _ empty-string-sequence (of ImmutableArray String)
  (return (call (. (call (of ArrayBuilder String)) finish-immutable)))
)
```

```output
a, b, c
```

### A direct String capture materializes only when it crosses a String call boundary.

```brix
(main
  (let result
    (match "capture!"
      (~ (seq (: text (+ (chars "a-z"))) "!")
        (call echo-text text)
      )
      (default
        (inline "miss")
      )
    )
  )
  (blank)
  (print result)
)

(fn-expr + echo-text String
  (param text String)
  text
)
```

```output
capture
```

### Capture spans preserve empty, ASCII, multibyte, equality, array, and source-lifetime semantics.

```brix
(main
  (let source (. "café 東京 😀" codepoints))
  (let empty (call (of CaptureSpan Char) source 0 0))
  (let latin (call (of CaptureSpan Char) source 0 4))
  (let city (call (of CaptureSpan Char) source 5 7))
  (let same-latin (call (of CaptureSpan Char) (. "café" codepoints) 0 4))
  (let latin-text (str latin))
  (let city-array ([] Char) city)
  (let retained (call retained-capture-text))
  (let retained-text (str retained))
  (blank)
  (print (. empty length))
  (print (. latin length))
  (print (== latin "café"))
  (print (== latin same-latin))
  (print (. city-array length))
  (print ([] city-array 0))
  (print ([] city-array 1))
  (print latin-text)
  (print (. retained length))
  (print retained-text)
)

(fn-do + retained-capture-text (of CaptureSpan Char)
  (let local-source (. "lifetime-✓" codepoints))
  (return (call (of CaptureSpan Char) local-source 0 (. local-source length)))
)
```

```output
0
4
true
true
2
東
京
café
10
lifetime-✓
```

### Eligible capture operations keep their public length, equality, hash, and numeric semantics.

```brix
(import /core/map)
(blank)

(main
  (let result
    (match "12345!"
      (~ (seq (: digits (+ (chars "0-9"))) "!")
        (call capture-operation-summary (. digits length) (== digits "12345") (call hash-for digits) (call I32 digits))
      )
      (default
        (inline "miss")
      )
    )
  )
  (blank)
  (print result)
)

(fn-expr + capture-operation-summary String
  (param length Size)
  (param equal Bool)
  (param hash Hash)
  (param number I32)
  (str length " " equal " " (== hash (call hash-for "12345")) " " number)
)
```

```output
5 true true 12345
```

### Stable and noncanonical sources agree for empty, failed, backtracked, and multibyte captures.

```brix
(main
  (let stable-empty
    (match "!"
      (~ (seq (: text (* (chars "a-z"))) "!")
        (. text length)
      )
      (default
        (inline 99)
      )
    )
  )
  (blank)
  (let stable-backtrack
    (match "café!"
      (~ (seq (: text (+ (chars "a-z"))) "?")
        false
      )
      (~ (seq (: text (+ (chars "a-zé"))) "!")
        (== text "café")
      )
      (default
        (inline false)
      )
    )
  )
  (blank)
  (let generic (call (of IndexableTokenStream Char) (. "東京!" codepoints)))
  (let generic-result
    (match generic
      (~ (seq (: text (+ (chars "東京"))) "!")
        (. text length)
      )
      (default
        (inline 99)
      )
    )
  )
  (blank)
  (print stable-empty)
  (print stable-backtrack)
  (print generic-result)
)
```

```output
0
true
2
```

### The direct cursor ABI preserves results and failed-probe extent across stream sources.

```brix
(import /core/tokenstream)
(blank)

(main
  (let array-hit (call (of ArrayTokenStream Char) (. "before" codepoints)))
  (let iterable-hit (call (of ChunkedTokenStream Char) (. "before" codepoints)))
  (let array-miss (call (of ArrayTokenStream Char) (. "zzzzzzzz" codepoints)))
  (let iterable-miss (call (of ChunkedTokenStream Char) (. "zzzzzzzz" codepoints)))
  (let array-backtrack (call (of ArrayTokenStream Char) (. "before!" codepoints)))
  (let iterable-backtrack (call (of ChunkedTokenStream Char) (. "before!" codepoints)))
  (blank)
  (print (call pilot-result array-hit))
  (print (call pilot-result iterable-hit))
  (print (call pilot-result array-miss))
  (print (call pilot-result iterable-miss))
  (print (call pilot-backtrack array-backtrack))
  (print (call pilot-backtrack iterable-backtrack))
  (print (. array-miss cursor-extent))
  (print (. iterable-miss cursor-extent))
)

(fn-expr + pilot-result String
  (param stream (of TokenStream Char))
  (match stream
    (~ (: value \pilot-word)
      value
    )
    (default
      (inline "miss")
    )
  )
)

(fn-expr + pilot-word _ (over: Char)
  (~ "about")
  (~ "after")
  (~ "and")
  (~ "at")
  (~ "before")
  (~ "between")
  (~ "both")
  (~ "by")
  (~ "during")
  (~ "despite")
  (~ "except")
  (~ "every")
  (~ "from")
  (~ "into")
  (~ "like")
  (~ "near")
  (~ "of")
  (~ "onto")
  (~ "or")
  (~ "over")
)

(fn-expr + pilot-backtrack String
  (param stream (of TokenStream Char))
  (match stream
    (~ (seq (: value \pilot-word) "?")
      "wrong"
    )
    (~ (seq (: value \pilot-word) "!")
      value
    )
    (default
      (inline "miss")
    )
  )
)
```

```output
before
before
miss
miss
before
before
7
7
```

### Chunked stream matching refills beyond the first buffer without putback or eager tracking.

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (let source (++ (* "a" 2500) "!"))
  (let iterable (call (of ChunkedTokenStream Char) (. source codepoints)))
  (let data (call (of TestByteSource Char) (. source codepoints)))
  (call (. data rewind))
  (let bytes (call (of ChunkedTokenStream Char) data))
  (blank)
  (print (call long-a-run iterable))
  (print (call long-a-run bytes))
)

(fn-expr + long-a-run Size
  (param stream (of TokenStream Char))
  (match stream
    (~ (seq (: text (+ (chars "a"))) "!")
      (. text length)
    )
    (default
      (inline 0)
    )
  )
)
```

```output
2500
2500
```

### Chunked repeated rules retain a temporary iterable source across refills.

```brix
(import /core/tokenstream)
(blank)

(main
  (let stream (call (of ChunkedTokenStream Char) (. (call refill-source) codepoints)))
  (let rows
    (match stream
      (~ \refill-document)
      (throw (call Panic "chunked iterable refill parse failed"))
    )
  )
  (blank)
  (let checksum (as 0 Size))
  (for (in row rows)
    (+= checksum row)
  )
  (print checksum)
)

(fn-expr + refill-document _ (over: Char)
  (~ (: rows (+ \refill-row))
    rows
  )
  (where
    (lazy refill-row Size
      (pattern
        (~ (seq "src:" (: name \refill-word) ":" (: value \refill-digits) "\n")
          (+ (* (. name length) 37) (. value length))
        )
      )
    )
    (blank)
    (lazy refill-word String
      (pattern
        (~ (seq (chars "a-zA-Z_") (* (chars "a-zA-Z0-9_"))))
      )
    )
    (blank)
    (lazy refill-digits String
      (pattern
        (~ (+ (chars "0-9")))
      )
    )
  )
)

(fn-do _ refill-source _
  (let result "")
  (for (in index (..< 0 180))
    (+= result (str "src:name_" index ":" (* index 47) "\n"))
  )
  (return result)
)
```

```output
49904
```

### Chunked repeated byte rules survive a large-capacity source across refills.

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(import /BxGC/BxGC.h)
(blank)

(main
  (let bytes-data ([] Byte 60000))
  (let bytes-size Size 0)
  (for (in row (..< 0 220))
    (let length (as (+ (% row 17) 3) U8))
    (= ([] bytes-data bytes-size) (as (+ 160 (% row 7)) Byte))
    (+= bytes-size 1)
    (= ([] bytes-data bytes-size) length)
    (+= bytes-size 1)
    (for (in offset (..< 0 length))
      (= ([] bytes-data bytes-size) (as (% (+ (* row 13) (* offset 7)) 256) Byte))
      (+= bytes-size 1)
    )
  )
  (let bytes (call TestByteSource ([] bytes-data (..< 0 bytes-size))))
  (blank)
  (let stream (call (of ChunkedTokenStream Byte) bytes))
  (let rows
    (match stream
      (~ \refill-byte-document)
      (throw (call Panic "chunked byte refill parse failed"))
    )
  )
  (blank)
  (let checksum (as 0 Size))
  (for (in row rows)
    (+= checksum row)
  )
  (print checksum)
)

(fn-expr + refill-byte-document _ (over: Byte)
  (~ (: rows (+ \refill-byte-row))
    rows
  )
  (where
    (lazy refill-byte-row Size
      (pattern
        (~ (seq (: magic U8) (: length U8) (: payload ({} Byte length)))
          (call refill-byte-row-result magic payload)
        )
      )
    )
  )
)

(fn-do _ refill-byte-row-result Size
  (param magic U8)
  (param payload (of ImmutableArray Byte))
  (call C:brixCollect)
  (return (+ (* (as magic Size) 17) (. payload length)))
)
```

```output
611930
```

### A chunked error arm waits for refill, then fires after the source is truly exhausted.

```brix
(import /core/tokenstream)
(blank)

(main
  (let attempt
    (do-new
      (let stream (call (of ChunkedTokenStream Char) (. (* "a" 1100) codepoints)))
      (print
        (match stream
          (~ (seq (+ (chars "a")) "!"))
          (throw (call Panic "missing chunked terminator"))
        )
      )
      (on-failure failure
        (print (str "error: " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
error: missing chunked terminator
```

### An optional semantic rule preserves its ImmutableArray value instead of substituting source text.

```brix
(import /core/array-builder)
(blank)

(main
  (let values
    (match "a"
      (~ (: items (? \optional-sequence-items))
        (?? items (call (of ImmutableArray I32)))
      )
      (default
        (inline (call (of ImmutableArray I32)))
      )
      (blank)
      (lazy optional-sequence-items (of ImmutableArray I32)
        (pattern
          (~ "a"
            (call single-integer-sequence 7)
          )
        )
      )
    )
  )
  (blank)
  (print (. values length))
  (print ([] values 0))
)

(fn-do _ single-integer-sequence (of ImmutableArray I32)
  (param value I32)
  (let builder (call (of ArrayBuilder I32)))
  (call (. builder append) value)
  (return (call (. builder finish-immutable)))
)
```

```output
1
7
```

### Accumulating captures preserve source order and duplicates.

```brix
(main
  (let flat
    (match "aba!"
      (~ (seq (+ (: ...items (chars "a-z"))) "!")
        items
      )
      (default
        (inline (call (of ImmutableArray String)))
      )
    )
  )
  (print flat)
)
```

```output
a, b, a
```

### Zero successful contributions produce an empty ImmutableArray.

```brix
(main
  (let empty
    (match "!"
      (~ (seq (* (: ...items (chars "a-z"))) "!")
        items
      )
      (default
        (inline (call (of ImmutableArray String)))
      )
    )
  )
  (print (. empty length))
)
```

```output
0
```

### Failed ordered-choice arms rewind accumulated managed values before trying the next arm.

```brix
(main
  (let choice
    (match "ab"
      (~ (| (seq (: ...items "a") "x") (seq (: ...items "a") (: ...items "b")))
        items
      )
      (default
        (inline (call (of ImmutableArray String)))
      )
    )
  )
  (print choice)
)
```

```output
a, b
```

### Optional and repetition failures restore collector lengths before later matches append.

```brix
(main
  (let optional
    (match "a"
      (~ (seq (? (seq (: ...items "a") "x")) (: ...items "a"))
        items
      )
      (default
        (inline (call (of ImmutableArray String)))
      )
    )
  )
  (let repeated
    (match "aaab"
      (~ (seq (* (seq (: ...items "a") "x")) (: ...items "a") (: ...items "a") (: ...items "a") "b")
        items
      )
      (default
        (inline (call (of ImmutableArray String)))
      )
    )
  )
  (print optional)
  (print repeated)
)
```

```output
a
a, a, a
```

### Positive and negative lookahead never leak speculative contributions.

```brix
(main
  (let positive
    (match "a"
      (~ (seq (& (: ...items "a")) (: ...items "a"))
        items
      )
      (default
        (inline (call (of ImmutableArray String)))
      )
    )
  )
  (let negative
    (match "a"
      (~ (| (seq (! (: ...items "a")) "x") (: ...items "a"))
        items
      )
      (default
        (inline (call (of ImmutableArray String)))
      )
    )
  )
  (print positive)
  (print negative)
)
```

```output
a
a
```

### Joined lookahead failures preserve polarity, cursor state, nested outcomes, and refills.

```brix
(import /core/tokenstream)
(blank)

(main
  (let positive-success
    (match "a"
      (~ (seq (& "a") (: value "a"))
        value
      )
      (default
        (inline "failed")
      )
    )
  )
  (let positive-mismatch
    (match "b"
      (~ (seq (& "a") "b")
        "wrong"
      )
      (~ (: value "b")
        value
      )
      (default
        (inline "failed")
      )
    )
  )
  (let positive-eof
    (match ""
      (~ (seq (& "a") "")
        "wrong"
      )
      (~ ""
        "eof"
      )
      (default
        (inline "failed")
      )
    )
  )
  (let negative-success
    (match "a"
      (~ (seq (! "a") "a")
        "wrong"
      )
      (~ (: value "a")
        value
      )
      (default
        (inline "failed")
      )
    )
  )
  (let negative-mismatch
    (match "b"
      (~ (seq (! "a") (: value "b"))
        value
      )
      (default
        (inline "failed")
      )
    )
  )
  (let negative-eof
    (match ""
      (~ (seq (! "a") "")
        "eof"
      )
      (default
        (inline "failed")
      )
    )
  )
  (let nested-choice
    (match "b"
      (~ (seq (! (| "a" (& (seq (! "c") "c")))) (: value "b"))
        value
      )
      (default
        (inline "failed")
      )
    )
  )
  (let chained
    (match "abcdefgh!"
      (~ (seq (! "0") (! "1") (! "2") (! "3") (! "4") (! "5") (: value "abcdefgh!"))
        (. value length)
      )
      (default
        (inline 0)
      )
    )
  )
  (blank)
  (let refill-tail (* "x" 70))
  (let refill-stream (call (of ChunkedTokenStream Char) (. (++ refill-tail "!") codepoints)))
  (let refill-mismatch
    (match refill-stream
      (~ (seq (! (seq $refill-tail "?")) $refill-tail "!")
        1
      )
      (default
        (inline 0)
      )
    )
  )
  (let eof-stream (call (of ChunkedTokenStream Char) (. refill-tail codepoints)))
  (let refill-eof
    (match eof-stream
      (~ (seq (! (seq $refill-tail "?")) $refill-tail)
        1
      )
      (default
        (inline 0)
      )
    )
  )
  (blank)
  (print positive-success)
  (print positive-mismatch)
  (print positive-eof)
  (print negative-success)
  (print negative-mismatch)
  (print negative-eof)
  (print nested-choice)
  (print chained)
  (print refill-mismatch)
  (print refill-eof)
)
```

```output
a
b
eof
a
b
eof
b
9
1
1
```

### Multiple named accumulators retain independent types, order, and duplicate values.

```brix
(main
  (let split
    (match "a1b2a3"
      (~ (+ (seq (: ...letters (chars "a-z")) (: ...digits (chars "0-9"))))
        (str letters "|" digits)
      )
      (default
        (inline "failed")
      )
    )
  )
  (print split)
)
```

```output
a, b, a|1, 2, 3
```

### An ordinary call owns a fresh same-named accumulator in its nested rule.

```brix
(main
  (let nested
    (match "axb"
      (~ (seq (: ...items "a") (: inner \inner-items) (: ...items "b"))
        (str items "|" inner)
      )
      (default
        (inline "failed")
      )
    )
  )
  (print nested)
)

(fn-expr + inner-items _ (over: Char)
  (~ (: ...items "x")
    items
  )
)
```

```output
a, b|x
```

### Named collector forwarding borrows the caller accumulator without finishing it.

```brix
(main
  (let forwarded
    (match "za"
      (~ (seq (: ...outer "z") (call \append-one (into values outer)))
        outer
      )
      (default
        (inline (call (of ImmutableArray String)))
      )
    )
  )
  (print forwarded)
)

(fn-expr + append-one _ (over: Char)
  (~ (: ...values "a")
    values
  )
)
```

```output
z, a
```

### Multiple collector mappings and a partial mapping update only the selected caller owners.

```brix
(main
  (let multiple
    (match "a1b2"
      (~ (seq (* (: ...outer-letters "z")) (* (: ...outer-digits "9")) (+ (call \letter-digit (into letters outer-letters) (into digits outer-digits))))
        (str outer-letters "|" outer-digits)
      )
      (default
        (inline "failed")
      )
    )
  )
  (let partial
    (match "a1b2"
      (~ (seq (* (: ...outer-letters "z")) (+ (call \letter-digit (into letters outer-letters))))
        outer-letters
      )
      (default
        (inline (call (of ImmutableArray String)))
      )
    )
  )
  (print multiple)
  (print partial)
)

(fn-expr + letter-digit String (over: Char)
  (~ (seq (: ...letters (chars "a-z")) (: ...digits (chars "0-9")))
    it
  )
)
```

```output
a, b|1, 2
a, b
```

### Explicit collector forwarding keeps one borrowed accumulator alive across recursive calls.

```brix
(main
  (let recursive
    (match "a,a,a"
      (~ (: values \recursive-items)
        values
      )
      (default
        (inline "")
      )
    )
  )
  (print recursive)
)

(fn-expr + recursive-items String (over: Char)
  (~ (seq (: ...values (chars "a-z")) (? (seq "," (call \recursive-items (into values)))))
    (str values)
  )
)
```

```output
a, a, a
```

### A transformed repeated Char capture implicitly converts to its rule's declared String result.

```brix
(main
  (print
    (match "abc"
      (~ (: text \text)
        text
      )
      (blank)
      (lazy text String
        (pattern
          (~ (: chars (+ \character))
            chars
          )
        )
      )
      (blank)
      (lazy character
        (pattern
          (~ (seq (! (| \reserved-character "\n")) .))
        )
      )
      (blank)
      (lazy reserved-character
        (pattern
          (~ (| "*" "[" "]" "!" "`"))
        )
      )
    )
  )
)
```

```output
abc
```

```brix
(import /core/arena)
(import /core/tokenstream)
(import /BxCore/BxCore.h)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let source (call (of Array Char) (call (of (. arena allocate) Char) 3) 3))
  (= ([] source 0) 'a')
  (= ([] source 1) 'b')
  (= ([] source 2) 'c')
  (let stream (call (of ArrayTokenStream Char) source))
  (let (tuple view-source view-cursor view-length view-extent) (!! (. stream pattern-view)))
  (print (== (. view-source data) (. source data)))
  (print (== (. stream buffer data) (. source data)))
  (print (== view-length (. source size)))
  (= ([] source 1) 'z')
  (call C:brixCollect)
  (print
    (match stream
      (~ "azc"
        true
      )
      (default
        (inline false)
      )
    )
  )
)
```

```output
true
true
true
true
```

```brix
(import /BxCore/BxCore.h)
(import /core/arena)
(blank)

(main
  (let scratch (use (call Arena)))
  (let helper (call CaptureScratch scratch))
  (let source "")
  (for (in index (..< 0 2048))
    (+= source "a")
  )
  (+= source "!")
  (let accumulated
    (match source
      (with helper)
      (~ \scratch-accumulated)
      (throw (call Panic "Scratch accumulator failed"))
    )
  )
  (print (. accumulated length))
  (print ([] accumulated 2047))
  (print (> (. scratch active-bytes) 0))
  (call (. scratch reset))
  (let repeated
    (match source
      (with helper)
      (~ (seq (: values (+ \scratch-one)) "!")
        values
      )
      (throw (call Panic "Scratch repetition failed"))
    )
  )
  (print (. repeated length))
  (print ([] repeated 2047))
  (print (> (. scratch active-bytes) 0))
  (call (. scratch reset))
  (let nested
    (match "aa;aaa;!"
      (with helper)
      (~ (seq (+ (seq (: values (+ \scratch-one)) ";")) "!")
        values
      )
      (throw (call Panic "Nested scratch repetition failed"))
    )
  )
  (print (. nested length))
  (print (. ([] nested 0) length))
  (print (. ([] nested 1) length))
  (print (> (. scratch active-bytes) 0))
  (call (. scratch reset))
  (let text
    (match "λ😀λ!"
      (with helper)
      (~ \scratch-text)
      (throw (call Panic "Scratch text failed"))
    )
  )
  (print (> (. scratch active-bytes) 0))
  (call (. scratch reset))
  (print text)
  (let objects
    (match "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa!"
      (with helper)
      (~ (seq (: values (+ \scratch-object)) "!")
        values
      )
      (throw (call Panic "Managed scratch capture failed"))
    )
  )
  (call C:brixCollect)
  (print (. objects length))
  (print (. ([] objects 31) value))
  (print (> (. scratch active-bytes) 0))
  (call (. scratch reset))
  (let strings
    (match "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa!"
      (with helper)
      (~ (seq (+ (: ...values \scratch-heap-string)) "!")
        values
      )
      (throw (call Panic "Heap string capture failed"))
    )
  )
  (print (. strings length))
  (call C:brixCollect)
  (print ([] strings 31))
  (print (> (. scratch active-bytes) 0))
  (call (. scratch reset))
  (print (. scratch active-bytes))
)

(type + CaptureScratch
  (implements (of PatternCollectorStorage ArenaBacking))
  (prop + pattern-collector-context Arena)
  (fn-do + This _
    (param pattern-collector-context _)
  )
)

(fn-expr + scratch-accumulated (of ImmutableArray Int) (over: Char)
  (~ (seq (? (: ...values \scratch-one)) (* (| (call \scratch-failed (into values)) (: ...values \scratch-one))) "!")
    values
  )
)

(fn-expr + scratch-failed (of ImmutableArray Int) (over: Char)
  (~ (seq (: ...values \scratch-one) "x")
    values
  )
)

(fn-expr + scratch-one Int (over: Char)
  (~ "a"
    1
  )
)

(fn-expr + scratch-text String (over: Char)
  (~ (seq (+ (| (: ...values "λ") (: ...values "😀"))) "!")
    (str values)
  )
)

(fn-expr + scratch-object CaptureObject (over: Char)
  (~ "a"
    (call CaptureObject 7)
  )
)

(fn-expr + scratch-heap-string String (over: Char)
  (~ (: text "a")
    (str "heap-" text)
  )
)

(type + CaptureObject
  (prop + value Int)
  (fn-do + This _
    (param value _)
  )
)
```

```output
2048
1
true
2048
1
true
2
2
3
true
true
λ, 😀, λ
32
7
true
32
heap-a
true
0
```

```brix
(import /core/tokenstream)
(blank)

(main
  (call check-dynamic "abcdef" "abcdef" true 6 6)
  (call check-dynamic "xbcdef" "abcdef" false 0 1)
  (call check-dynamic "abcdex" "abcdef" false 0 6)
  (call check-dynamic "x" "abcdef" false 0 1)
  (call check-dynamic "" "abcdef" false 0 0)
  (call check-dynamic "" "" true 0 0)
  (call check-dynamic "abc" "" true 0 0)
  (call check-dynamic "é東京😀" "é東京😀" true 4 4)
  (call check-dynamic "é京東😀" "é東京😀" false 0 2)
  (call check-dynamic "é東京x" "é東京😀" false 0 4)
  (call check-dynamic "é東" "é東京😀" false 0 2)
  (call check-dynamic (* "é東京😀" 512) (* "é東京😀" 512) true 2048 2048)
  (call check-dynamic (* "é東京😀" 512) (++ (* "é東京😀" 511) "é東京x") false 0 2048)
  (for (in literal (list "alpha" "beta" "😀" "" "alpha"))
    (call check-dynamic literal literal true (. literal length) (. literal length))
  )
  (print "dynamic extents ok")
)

(fn-do + check-dynamic _
  (param source String)
  (param literal String)
  (param expected Bool)
  (param cursor Index)
  (param extent Index)
  (let stream (call (of ArrayTokenStream Char) (. source codepoints)))
  (let result
    (match stream
      (~ (: value $literal)
        value
      )
    )
  )
  (throw (call Panic "Dynamic result changed") (if (!= (. result has-value) expected)))
  (if result
    (throw (call Panic "Dynamic capture changed") (if (!= result literal)))
  )
  (throw (call Panic "Dynamic cursor changed") (if (!= (. stream cursor) cursor)))
  (throw (call Panic (str "Dynamic extent changed: " source " / " literal " / " (. stream cursor-extent) " / " extent)) (if (!= (. stream cursor-extent) extent)))
)
```

```output
dynamic extents ok
```

```brix
(import /core/tokenstream)
(blank)

(main
  (let journal (call LiteralJournal))
  (print
    (match "é東京😀!"
      (~ (seq (call \runtime-literal (. journal next)) "?")
        "wrong"
      )
      (~ (seq (& (call \runtime-literal (. journal next))) (: value (call \runtime-literal (. journal next))) "!")
        value
      )
      (default
        (inline "miss")
      )
    )
  )
  (print (. journal calls))
  (let long (* "é東京😀" 512))
  (let chunked (call (of ChunkedTokenStream Char) (. (++ long "!") codepoints)))
  (print
    (match chunked
      (~ (seq (! (seq $long "?")) (: value $long) "!")
        (== value long)
      )
      (default
        (inline false)
      )
    )
  )
  (let generic (call (of IndexableTokenStream Char) (. "é東京😀" codepoints)))
  (let wrong "é京東😀")
  (print
    (match generic
      (~ $wrong
        "wrong"
      )
      (~ (: value (call \runtime-literal (. journal next)))
        value
      )
    )
  )
  (print (. journal calls))
)

(fn-expr + runtime-literal _ (over: Char)
  (param literal String)
  (~ $literal)
)

(type + LiteralJournal
  (prop + calls Size)
  (fn-do + next _ :get
    (+= calls 1)
    (return "é東京😀")
  )
)
```

```output
é東京😀
3
true
é東京😀
4
```

```brix
(import /core/tokenstream)
(blank)

(main
  (let bytes ([] Byte 4))
  (= ([] bytes 0) 97u8)
  (= ([] bytes 1) 255u8)
  (= ([] bytes 2) 0u8)
  (= ([] bytes 3) 0u8)
  (let literal (as (as (. bytes data) (of CPointer Char8)) String))
  (for (in source (list "" "x" "aa"))
    (call match-invalid source literal)
  )
)

(fn-do + match-invalid _
  (param source String)
  (param literal String)
  (let attempt
    (do-new
      (print
        (match source
          (~ $literal
            "wrong"
          )
          (default
            (inline "miss")
          )
        )
      )
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Unable to read UTF8 content
Unable to read UTF8 content
Unable to read UTF8 content
```

```brix
(import /core/tokenstream)
(blank)

(main
  (let stream (call (of ArrayTokenStream Char) (. "é東京😀" codepoints)))
  (for (in literal (list "é京東😀" "é東京x" "é東京😀"))
    (= (. stream cursor) 0)
    (print (call gc-free-literal stream literal))
  )
)

(fn-do + gc-free-literal Bool (without gc)
  (param stream (of ArrayTokenStream Char))
  (param literal String)
  (return
    (match stream
      (~ $literal
        true
      )
      (default
        (inline false)
      )
    )
  )
)
```

```output
false
false
true
```

```brix
(import /core/arena)
(import /core/tokenstream)
(blank)

(main
  (let arena (call Arena))
  (let helper (call SourceOwner arena))
  (let stream (call (of ArrayTokenStream Char) (. "éabc😀!" codepoints)))
  (let checksum Size)
  (for (in i (..< 0 1))
    (= (. stream cursor) 0)
    (+= checksum (. (call source-result stream helper) length))
    (= (. stream cursor) 0)
    (+= checksum (. (call it-result stream helper) length))
    (= (. stream cursor) 0)
    (+= checksum (. (call atom-result stream helper) text length))
    (call (. arena reset))
  )
  (blank)
  (= (. stream cursor) 0)
  (let optional (call optional-result stream helper))
  (= (. stream cursor) 0)
  (let borrowed (call helper-result stream helper "!"))
  (print (. (!! optional) text))
  (print borrowed)
  (= (. stream cursor) 0)
  (print (call length-result stream helper))
  (= (. stream cursor) (. stream length))
  (print (== (call optional-result stream helper) null))
  (call (. arena release))
  (print (str "bench-result parser/implicit-results checksum=" checksum))
)

(fn-do + source-result String (without gc)
  (param stream (of TokenStream Char))
  (param helper SourceOwner)
  (return
    (match stream
      (with helper)
      (~ \source-word)
      (default
        (inline "")
      )
      (blank)
      (lazy source-word String
        (pattern
          (~ (seq . (+ (chars "a-z")) . "!"))
        )
      )
    )
  )
)

(fn-do + it-result String (without gc)
  (param stream (of TokenStream Char))
  (param helper SourceOwner)
  (return
    (match stream
      (with helper)
      (~ \source-word)
      (default
        (inline "")
      )
      (blank)
      (lazy source-word String
        (pattern
          (~ (seq . (+ (chars "a-z")) . "!")
            it
          )
        )
      )
    )
  )
)

(fn-do + atom-result Atom (without gc)
  (param stream (of TokenStream Char))
  (param helper SourceOwner)
  (return
    (match stream
      (with helper)
      (~ \source-word)
      (default
        (inline (as "" Atom))
      )
      (blank)
      (lazy source-word Atom
        (pattern
          (~ (seq . (+ (chars "a-z")) . "!"))
        )
      )
    )
  )
)

(fn-do + optional-result (? Atom)
  (param stream (of TokenStream Char))
  (param helper SourceOwner)
  (return
    (match stream
      (with helper)
      (~ \source-word)
      (default
        (inline null)
      )
      (blank)
      (lazy source-word (? Atom)
        (pattern
          (~ (seq . (+ (chars "a-z")) . "!"))
        )
      )
    )
  )
)

(fn-do + helper-result String
  (param stream (of TokenStream Char))
  (param helper SourceOwner)
  (param suffix String)
  (return
    (match stream
      (with helper)
      (~ (call \source-forward (~ (seq . (+ (chars "a-z")) . $suffix))))
      (default
        (inline "")
      )
    )
  )
)

(fn-expr + source-forward String (over: Char)
  (param pattern _)
  (~ \pattern)
)

(fn-do + length-result Size
  (param stream (of TokenStream Char))
  (param helper SourceOwner)
  (return
    (match stream
      (with helper)
      (~ \source-size)
      (default
        (inline 0)
      )
      (blank)
      (lazy source-size Size
        (pattern
          (~ (seq . (+ (chars "a-z")) . "!")
            (. it length)
          )
        )
      )
    )
  )
)

(type + SourceOwner
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (blank)
  (delegate (of PatternCaptureMaterializer Char Atom) (of PatternCaptureMaterializer Byte Atom))
  (delegate (of PatternValueMaterializer String Atom))
  (prop _ atoms SourceAtoms)
  (fn-do + This _
    (param arena Arena)
    (= atoms (call SourceAtoms arena))
  )
  (fn-expr + (of materialize-capture Token) String (without gc)
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (call (. arena copy-span) source start end)
  )
  (fn-expr + materialize-value String (without gc)
    (param source String)
    (call (. arena copy-string) source)
  )
  (fn-expr + mark Size (without gc)
    (throw (call Panic "Unexpected source materialization mark"))
  )
  (fn-do + rewind _ (without gc)
    (param mark Size)
    (throw (call Panic "Unexpected source materialization rewind"))
  )
)

(type _ SourceAtoms (is Tuple)
  (implements (of PatternCaptureMaterializer Char Atom))
  (implements (of PatternCaptureMaterializer Byte Atom))
  (implements (of PatternValueMaterializer String Atom))
  (prop _ arena Arena)
  (fn-do + This _
    (param arena Arena)
  )
  (fn-expr + (of materialize-capture Token) Atom (without gc)
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (as (call (. arena copy-span) source start end) Atom)
  )
  (fn-expr + materialize-value Atom (without gc)
    (param source String)
    (as (call (. arena copy-string) source) Atom)
  )
)
```

```output
éabc😀!
éabc😀!
6
true
bench-result parser/implicit-results checksum=18
```

```brix
(import /core/arena)
(import /core/tokenstream)
(import /BxGC/BxGC.h)
(blank)

(main
  (let scratch (call Arena))
  (let arena (call Arena))
  (let helper (call SourceOwner arena))
  (let source (call (of Array Char) (call (of (. scratch allocate) Char) 6) 6))
  (let input (. "éabc😀!" codepoints))
  (for (in i (..< 0 (. input length)))
    (= ([] source i) ([] input i))
  )
  (let stream (call (of ArrayTokenStream Char) source))
  (let text
    (match stream
      (with helper)
      (~ (seq . (+ (chars "a-z")) . "!")
        it
      )
      (throw (call Panic "source it failed"))
    )
  )
  (= (. stream cursor) 0)
  (let atom
    (match stream
      (with helper)
      (~ \atom-word)
      (throw (call Panic "atom it failed"))
      (blank)
      (lazy atom-word Atom
        (pattern
          (~ (seq . (+ (chars "a-z")) . "!")
            it
          )
        )
      )
    )
  )
  (= (. stream cursor) 0)
  (let optional
    (match stream
      (with helper)
      (~ \optional-word)
      (throw (call Panic "optional source failed"))
      (blank)
      (lazy optional-word (? Atom)
        (pattern
          (~ (seq . (+ (chars "a-z")) . "!")
            it
          )
        )
      )
    )
  )
  (= (. stream cursor) 0)
  (let nested
    (match stream
      (with helper)
      (~ (seq (! (| "x" "y")) \nested-word))
      (throw (call Panic "nested source failed"))
      (blank)
      (lazy nested-word Atom
        (pattern
          (~ (seq \source-word "?"))
          (~ \source-word)
        )
      )
      (blank)
      (lazy source-word Atom
        (pattern
          (~ (seq . (+ (chars "a-z")) . "!"))
        )
      )
    )
  )
  (blank)
  (let empty
    (match ""
      (with helper)
      (~ (* (chars "a-z"))
        it
      )
      (throw (call Panic "empty source failed"))
    )
  )
  (let transformed
    (match "ab!"
      (with helper)
      (~ (seq (+ (chars "a-z")) "!")
        (+ it "-changed")
      )
      (throw (call Panic "transformed source failed"))
    )
  )
  (blank)
  (call (. scratch reset))
  (for (in i (..< 0 20))
    (let later
      (match "later!"
        (with helper)
        (~ (seq (+ (chars "a-z")) "!")
          it
        )
      )
    )
    (call C:brixGCCollect)
  )
  (blank)
  (print text)
  (print (. atom text))
  (print (. (!! optional) text))
  (print (. nested text))
  (print (. empty length))
  (print transformed)
  (print (call (. arena owns) (as (. (as text Atom) address) Pointer)))
  (print (call (. arena owns) (as (. atom address) Pointer)))
  (print (call (. arena owns) (as (. (!! optional) address) Pointer)))
  (print (call (. arena owns) (as (. nested address) Pointer)))
  (call (. scratch release))
  (call (. arena release))
)

(type + SourceOwner
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (blank)
  (delegate (of PatternCaptureMaterializer Char Atom) (of PatternCaptureMaterializer Byte Atom))
  (delegate (of PatternValueMaterializer String Atom))
  (prop _ atoms SourceAtoms)
  (fn-do + This _
    (param arena Arena)
    (= atoms (call SourceAtoms arena))
  )
  (fn-expr + (of materialize-capture Token) String (without gc)
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (call (. arena copy-span) source start end)
  )
  (fn-expr + materialize-value String (without gc)
    (param source String)
    (call (. arena copy-string) source)
  )
  (fn-expr + mark Size (without gc)
    (throw (call Panic "Unexpected source materialization mark"))
  )
  (fn-do + rewind _ (without gc)
    (param mark Size)
    (throw (call Panic "Unexpected source materialization rewind"))
  )
)

(type _ SourceAtoms (is Tuple)
  (implements (of PatternCaptureMaterializer Char Atom))
  (implements (of PatternCaptureMaterializer Byte Atom))
  (implements (of PatternValueMaterializer String Atom))
  (prop _ arena Arena)
  (fn-do + This _
    (param arena Arena)
  )
  (fn-expr + (of materialize-capture Token) Atom (without gc)
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (as (call (. arena copy-span) source start end) Atom)
  )
  (fn-expr + materialize-value Atom (without gc)
    (param source String)
    (as (call (. arena copy-string) source) Atom)
  )
)
```

```output
éabc😀!
éabc😀!
éabc😀!
éabc😀!
0
ab!-changed
true
true
true
true
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (let helper (call SourceOnlyOwner arena))
  (print (call source-only-length "éabc😀!" helper))
  (print (call source-only-length "bad" helper))
  (print (call source-only-helper "éabc😀!" helper))
  (call (. arena release))
)

(fn-do + source-only-length Size
  (param source String)
  (param helper SourceOnlyOwner)
  (return
    (match source
      (with helper)
      (~ \source-length)
      (default
        (inline 0)
      )
      (blank)
      (lazy source-length Size
        (pattern
          (~ (seq . (+ (chars "a-z")) . "!")
            (. it length)
          )
        )
      )
    )
  )
)

(fn-do + source-only-helper Size
  (param source String)
  (param helper SourceOnlyOwner)
  (return
    (match source
      (with helper)
      (~ (call \source-only-measure (~ (seq . (+ (chars "a-z")) . "!"))))
      (default
        (inline 0)
      )
    )
  )
)

(fn-expr + source-only-measure Size (over: Char)
  (param pattern (of PatternFunc Char String))
  (~ \local-value
    (. it length)
  )
  (where
    (lazy local-value String
      (pattern
        (~ $pattern)
      )
    )
  )
)

(type + SourceOnlyOwner
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (prop + arena Arena)
  (fn-do + This _
    (param arena Arena)
  )
  (fn-expr + (of materialize-capture Token) String (without gc)
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (call (. arena copy-span) source start end)
  )
)
```

```output
6
0
6
```

```brix
(main
  (print
    (match "ab"
      (~ \collector-scope)
    )
  )
)

(fn-expr + collector-scope String (over: Char)
  (~ (seq (: ...items "a") (: child (call \borrowed (into items))))
    (str items "|" child)
  )
  (where
    (lazy borrowed String
      (pattern
        (~ (seq (* (: ...items "z")) (: child \owned))
          child
        )
      )
    )
    (blank)
    (lazy owned String
      (pattern
        (~ (: ...items "b")
          (str items)
        )
      )
    )
  )
)
```

```output
a|b
```

```brix
(import /core/tokenstream)
(blank)

(main
  (let source (call SingleChunkSource))
  (let stream (call (of ChunkedTokenStream Char) source))
  (let events (~ \single-char (in: stream)))
  (print (<- events))
  (print (. source read-count))
  (let choice-source (call SingleChunkSource))
  (let choice-stream (call (of ChunkedTokenStream Char) choice-source))
  (let choices (~ \single-choice (in: choice-stream)))
  (print (<- choices))
  (print (. choice-source read-count))
)

(fn-expr + single-choice Char (over: Char)
  (~ "\x1b[200~"
    (as 0 Char)
  )
  (~ "a"
    (as 97 Char)
  )
)

(fn-expr + single-char Char (over: Char)
  (~ "\x1b[200~"
    (as 0 Char)
  )
  (~ (: c .)
    ([] c 0)
  )
)

(type + SingleChunkSource
  (implements ByteReadable)
  (prop + read-count Size)
  (fn-do + read Size
    (param size Size)
    (param buffer ([] Byte))
    (throw (call Panic "A complete match must not read another chunk") (if read-count))
    (+= read-count 1)
    (= ([] buffer 0) 97u8)
    (return 1)
  )
)
```

```output
a
1
a
1
```

```brix
(main
  (let source (call LetterSource))
  (for (in letter (in (~ \next-letter) source))
    (print letter)
  )
)

(fn-expr + next-letter Char (over: Char)
  (~ (: c .)
    ([] c 0)
  )
)

(type + LetterSource
  (implements ByteReadable)
  (implements (of Decodable Char))
  (prop + letters _ (= (call (of Array Byte) "ab")))
  (prop + cursor Size)
  (fn-do + read Size
    (param size Size)
    (param buffer ([] Byte))
    (return-if 0 (if (>= cursor (. letters size))))
    (= ([] buffer 0) ([] letters cursor))
    (+= cursor 1)
    (return 1)
  )
)
```

```output
a
b
```

```brix
(import /core/tokenstream)
(blank)

(main
  (let stream (call ObservableTokenStream (. "aabb" codepoints)))
  (let records (~> stream \pair))
  (print (. stream cursor))
  (print (<- records))
  (print (. stream cursor))
  (print (<- records))
  (print (. stream cursor))
)

(type + ObservableTokenStream (is (of IndexableTokenStream Char))
  (implements (of Iterable Char))
  (fn-do + This _
    (param buffer ([] Char))
    (call super buffer)
  )
)

(fn-expr + pair String (over: Char)
  (~ (seq (: left .) (: right .))
    (str left right)
  )
)
```

```output
0
aa
2
bb
4
```

```brix
(main
  (let records (~> "aabbxccaadd" \pair))
  (print (call (. "," join) records))
)

(fn-expr + pair String (over: Char)
  (~ (seq (: left (chars "abcd")) (: right (chars "abcd")))
    (str left right)
  )
)
```

```output
aa,bb
```

```brix
(main
  (let destination (call (of List String)))
  (*-> (~> "aabb" \pair) destination)
  (print (call (. "," join) destination))
)

(fn-expr + pair String (over: Char)
  (~ (seq (: left .) (: right .))
    (str left right)
  )
)
```

```output
aa,bb
```

```brix
(import /data/stream)
(blank)

(main
  (let source (call (of DataStream Char) (. "one\ntwo\n" codepoints)))
  source
  (pipe ~> (seq (: line (+ .)) (? \nl))
    line
  )
  (pipe |> print-stream-lines)
)

(fn-do + (of print-stream-lines Item) _
  (param items (stream Item))
  (for (in item items)
    (print item)
  )
)
```

```output
one
two
```

```brix
(import /os/process)
(import /core/parsing)
(blank)

(main
  (use
    (call Process "printf"
      (block
        (arg "one\ntwo\n")
      )
    )
  )
  (pipe ~> (seq (: line (+ .)) (? \nl))
    line
  )
  (pipe |> print-process-lines)
)

(fn-do + (of print-process-lines Item) _
  (param items (stream Item))
  (for (in item items)
    (print item)
  )
)
```

```output
one
two
```

```brix
(main
  (spread "ab")
  (pipe ~> (: pair (+ .))
    pair
  )
  (pipe |> print-generator-pairs)
)

(fn-do + (of print-generator-pairs Item) _
  (param items (stream Item))
  (for (in item items)
    (print item)
  )
)
```

```output
ab
```

### A pipe or a loop over a pattern that ends in an optional part and runs no expression gives the text each match read.

```brix
(main
  (print (call List (~> "ab cd" (seq (+ (chars "a-z")) (? " ")))))
  (for (in w (in (~ (seq (+ (chars "a-z")) (? " "))) "ab cd"))
    (print (str "<" w ">"))
  )
)
```

```output
ab , cd
<ab >
<cd>
```

### A pipe over a pattern whose matches give an optional value is an error.

```brix compile-error
(main
  (print (call List (~> "ab cd" \word)))
)

(fn-expr + word _ (over: Char)
  (~ (seq (+ (chars "a-z")) (? " ")))
)
```

```output
Exception: Grammar pipeline can't give an optional value; add an action that gives one
<no file:3>
      (print (call List (~> "ab cd" \word)))
                                    ^^^^^
```

```brix crash
(main
  (print (~> (call List "abc") ""))
)
```

```output
Exception: Grammar pipeline must consume input
<no file:3>
      (print (~> (call List "abc") ""))
                                   ^^
```

### A pipe over a pattern that only looks ahead is an error, as it never reads.

```brix crash
(main
  (print (call List (~> "ab" (& "a"))))
)
```

```output
Exception: Grammar pipeline must consume input
<no file:3>
      (print (call List (~> "ab" (& "a"))))
                                 ^^^^^^^
```

```brix crash
(main
  (print "started")
  (print (<- (~> "abc" \empty)))
)

(fn-expr + empty String (over: Char)
  (~ ""
    "empty"
  )
)
```

```output
started
```

### A loop over the matches of a pattern ends where the stream has no more, and panics where a match did not move the stream on. A pattern that can match nothing is an error, as in a pipe.

```brix
(main
  (call words "xy zz")
  (call words "xy zz!")
)

(fn-do + words _
  (param text String)
  (let attempt
    (do-new
      (let found "")
      (finally
        (print found)
      )
      (for (in w (in (~ \word) text))
        (+= found (str "[" w "]"))
      )
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-expr + word String (over: Char)
  (~ (seq (* (chars "a-z")) (? " "))
    it
  )
)
```

```output
[xy ][zz]
[xy ][zz]
Grammar loop made no input progress
```

```brix compile-error
(main
  (for (in w (in (~ (* "a")) "aab"))
    (print w)
  )
)
```

```output
Exception: Grammar loop must consume input
<no file:3>
      (for (in w (in (~ (* "a")) "aab"))
                        ^^^^^^^
```

```brix
(main
  (spread (call Bytes))
  (pipe ~> Byte
    it
  )
  (pipe |> print-all)
)

(type + Bytes
  (fn-to-do + (of Iterable Byte)
    (yield 65u8)
  )
)
```

```output
65
```

```brix
(main
  (spread (call Values))
  (pipe ~> \integer)
  (pipe |> print-all)
)

(type + Values
  (fn-to-do + (of Iterable Char)
    (yield 'x')
  )
  (fn-to-do + (of Iterable I32)
    (yield 7)
  )
)

(fn-expr + integer _ (over: I32)
  (~ I32)
)
```

```output
7
```

```brix crash
(main
  (print
    (match "ab"
      (~ (seq (: ...items "a") (: items "b"))
        items
      )
    )
  )
)
```

```output
Exception: Capture items mixes ordinary and accumulating capture modes
<no file:5>
          (~ (seq (: ...items "a") (: items "b"))
                                   ^^^^^^^^^^^^^
```

```brix crash
(main
  (print
    (match "a1"
      (~ (seq (: ...items \text) (: ...items \number))
        items
      )
      (blank)
      (lazy text String
        (pattern
          (~ "a"
            it
          )
        )
      )
      (blank)
      (lazy number I32
        (pattern
          (~ "1"
            1
          )
        )
      )
    )
  )
)
```

```output
Exception: Accumulating capture items has incompatible contribution types String and I32
<no file:5>
          (~ (seq (: ...items \text) (: ...items \number))
                                     ^^^^^^^^^^^^^^^^^^^^
```

### A pattern action infers its type from a qualified union case construction.

```brix
(main
  (let parts
    (match (list "alpha" "beta")
      (~ (: words (* \word))
        words
      )
      (throw (call Panic "words failed"))
      (blank)
      (lazy word
        (pattern
          (~ (: text String)
            (call (. Part word) text)
          )
        )
      )
    )
  )
  (blank)
  (for (in part parts)
    (match part
      (case (call (. Part word) text)
        (print text)
      )
      (case (. Part other)
        (print "other")
      )
    )
  )
)

(type _ Part (is Union)
  (variant word (tuple (: text String)))
  (variant other)
)
```

```output
alpha
beta
```

### A text-valued repeat yields the matched source, whether its item is an inline group, a rule call, a rule with an inner capture, or a counted or empty run

```brix
(main
  (print (str "1 [" (call capture-group "héllo 😀 wörld\nrest") "]"))
  (print (str "2 [" (call result-repeat "a\\|b `c|d` e|f") "]"))
  (print (str "3 [" (call wrapped "<one><two><three>!") "]"))
  (print (str "4 [" (call counted "aaaaab") "]"))
  (print (str "5 [" (call nested "ab cd  ef!") "]"))
  (print (str "6 [" (call empty-run "!!") "]"))
)

(fn-do + capture-group String
  (param text String)
  (return
    (match text
      (~ (seq (: line (* (seq (! "\n") .))) "\n" (* .))
        (str line "|")
      )
      (default
        (inline "<none>")
      )
    )
  )
)

(fn-do + result-repeat String
  (param text String)
  (return
    (match text
      (~ (seq (: first \cell) "|" (: rest (* .)))
        (str first "|" rest)
      )
      (default
        (inline "<none>")
      )
      (blank)
      (lazy cell String
        (pattern
          (~ (* \cell-part))
        )
      )
      (blank)
      (lazy cell-part
        (pattern
          (~ "\\|")
          (~ (seq "`" (* (seq (! "`") (! "\n") .)) "`"))
          (~ (seq (! (| "|" "\n")) .))
        )
      )
    )
  )
)

(fn-do + wrapped String
  (param text String)
  (return
    (match text
      (~ (seq (: all \tags) "!")
        all
      )
      (default
        (inline "<none>")
      )
      (blank)
      (lazy tags
        (pattern
          (~ (+ \tag))
        )
      )
      (blank)
      (lazy tag
        (pattern
          (~ (seq "<" (: inner \word) ">"))
        )
      )
      (blank)
      (lazy word
        (pattern
          (~ (+ (chars "a-z")))
        )
      )
    )
  )
)

(fn-do + counted String
  (param text String)
  (return
    (match text
      (~ (seq (: head ({} "a" 2 3)) (: tail (* "a")) "b")
        (str head "|" tail)
      )
      (default
        (inline "<none>")
      )
    )
  )
)

(fn-do + nested String
  (param text String)
  (return
    (match text
      (~ (seq (: words (+ (seq (+ (chars "a-z")) (* " ")))) "!")
        words
      )
      (default
        (inline "<none>")
      )
    )
  )
)

(fn-do + empty-run String
  (param text String)
  (return
    (match text
      (~ (seq (: letters (* (chars "a-z"))) (: marks (+ "!")))
        (str letters "|" marks)
      )
      (default
        (inline "<none>")
      )
    )
  )
)
```

```output
1 [héllo 😀 wörld|]
2 [a\|b `c|d` e|f]
3 [<one><two><three>]
4 [aaa|aa]
5 [ab cd  ef]
6 [|!!]
```

### A pattern pipeline ends when its source is exhausted, so a rule's error arm reports only input that is present and unmatched.

```brix
(main
  (print (?? (catch? (call read-pairs "aabb")) "error"))
  (print (?? (catch? (call read-pairs "aabbx")) "error"))
)

(type + PairError (is Error))

(fn-do + read-pairs String (?! PairError)
  (param text String)
  (let seen (call (of List String)))
  (for (in pair (~> text \pair))
    (+= seen pair)
  )
  (return (call (. "," join) seen))
)

(fn-expr + pair String (over: Char) (?! PairError)
  (~ (seq (: left (chars "ab")) (: right (chars "ab")))
    (str left right)
  )
  (throw (call PairError "unpaired"))
)
```

```output
aa,bb
error
```

### A called rule's error arm waits for input that has not arrived yet, and reports once the source is complete.

```brix
(import /core/tokenstream)
(blank)

(main
  (print (?? (catch? (call read-records (call StagedSource "ab" ";cd;"))) "error"))
  (print (?? (catch? (call read-records (call StagedSource "ab;" "cd"))) "error"))
)

(type + RecordError (is Error))

(fn-do + read-records String (?! RecordError)
  (param source StagedSource)
  (let seen (call (of List String)))
  (for (in name (~> (call (of ChunkedTokenStream Char) source) \record))
    (+= seen name)
  )
  (return (call (. "," join) seen))
)

(fn-expr + record String (over: Char) (?! RecordError)
  (~ (seq (: name (+ (chars "a-z"))) ";")
    name
  )
  (throw (call RecordError "unterminated"))
)

(type + StagedSource
  (implements ByteReadable)
  (prop + first String)
  (prop + second String)
  (prop + read-count Size)
  (fn-do + This _
    (param first _)
    (param second _)
  )
  (fn-do + read Size
    (param size Size)
    (param buffer ([] Byte))
    (let stage "")
    (if (== read-count 0)
      (= stage first)
      (else-if (== read-count 1)
        (= stage second)
      )
    )
    (+= read-count 1)
    (let bytes (as stage ([] Byte)))
    (if (. bytes size)
      (= ([] buffer (..< 0 (. bytes size))) bytes)
    )
    (return (. bytes size))
  )
)
```

```output
ab,cd
error
```

### A byte source is asked for more at a time while a record stays unfinished, and a record is whole however little each read returns.

```brix
(import /core/tokenstream)
(blank)

(main
  (let eager (call CountingSource (as (++ (* "a" 5000) ";bb;") ([] Byte)) 100000))
  (print (call read-lengths eager))
  (print (call request-sizes eager))
  (let slow (call CountingSource (as (++ (* "日本語" 1000) ";bb;") ([] Byte)) 7))
  (print (call read-lengths slow))
)

(fn-do + read-lengths String
  (param source CountingSource)
  (let seen (call (of List String)))
  (for (in name (~> (call (of ChunkedTokenStream Char) source) \record))
    (+= seen (str (. name length)))
  )
  (return (call (. "," join) seen))
)

(fn-expr + request-sizes String
  (param source CountingSource)
  (call (. "," join) sizes)
  (where
    (lazy sizes (in (str each) (. source requests)))
  )
)

(fn-expr + record String (over: Char)
  (~ (seq (: name (+ (seq (! ";") .))) ";")
    name
  )
)

(type + CountingSource
  (implements ByteReadable)
  (prop + data ([] Byte))
  (prop + limit Size)
  (prop + cursor Index)
  (prop + requests _ (= (call (of List Size))))
  (fn-do + This _
    (param data _)
    (param limit _)
  )
  (fn-do + read Size
    (param size Size)
    (param buffer ([] Byte))
    (+= requests size)
    (let remaining (- (. data size) cursor))
    (let wanted (if-expr (< size limit) size limit))
    (let amount (if-expr (< remaining wanted) remaining wanted))
    (if amount
      (= ([] buffer (..< 0 amount)) ([] data (..< cursor (+ cursor amount))))
      (+= cursor amount)
    )
    (return amount)
  )
)
```

```output
5000,2
1024,1024,2048,4096,1024
3000,2
```

### A chunked stream asked to release delivered input keeps a bounded buffer, loses nothing unread, and still counts lines and columns from the start of the source.

```brix
(import /core/tokenstream)
(blank)

(main
  (print (call released-lines))
  (print (call released-fields))
)

(fn-do + released-lines String
  (let lines (call (of List String)))
  (for (in index (..< 0 20000))
    (+= lines (str "line-" index "\n"))
  )
  (let text (call (. "" join) lines))
  (let stream (call (of ChunkedTokenStream Char) (call TextSource (as text ([] Byte)))))
  (let total Size 0)
  (let largest Size 0)
  (for (in name (~> stream \line))
    (+= total (. name length))
    (call (. stream pattern-release))
    (if (> (. stream buffer size) largest)
      (= largest (. stream buffer size))
    )
  )
  (let location (. stream location))
  (return (str total " " largest " " (. location line) ":" (. location column)))
)

(fn-do + released-fields String
  (let stream (call (of ChunkedTokenStream Char) (call TextSource (as (* "ab;" 30000) ([] Byte)))))
  (let count Size 0)
  (for (in name (~> stream \field))
    (+= count 1)
    (call (. stream pattern-release))
  )
  (let location (. stream location))
  (return (str count " " (. location line) ":" (. location column)))
)

(fn-expr + line String (over: Char)
  (~ (seq (: name (+ (seq (! "\n") .))) "\n")
    name
  )
)

(fn-expr + field String (over: Char)
  (~ (seq (: name (+ (chars "a-z"))) ";")
    name
  )
)

(type + TextSource
  (implements ByteReadable)
  (prop + data ([] Byte))
  (prop + cursor Index)
  (fn-do + This _
    (param data _)
  )
  (fn-do + read Size
    (param size Size)
    (param buffer ([] Byte))
    (let remaining (- (. data size) cursor))
    (let amount (if-expr (< remaining size) remaining size))
    (if amount
      (= ([] buffer (..< 0 amount)) ([] data (..< cursor (+ cursor amount))))
      (+= cursor amount)
    )
    (return amount)
  )
)
```

```output
188890 65536 20000:0
30000 0:90000
```

### A pipeline over a caller's chunked stream releases delivered input that no mark holds.

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (print (call piped-lines 20000 false))
  (print (call piped-lines 80000 false))
  (print (call piped-lines 20000 true))
)

(fn-do + piped-lines String
  (param count Size)
  (param marked Bool)
  (let lines (call (of List String)))
  (for (in index (..< 0 count))
    (+= lines (str "line-" index "\n"))
  )
  (let source (call TestByteSource (as (call (. "" join) lines) ([] Byte))))
  (= (. source is-encoded) true)
  (let stream (call (of ChunkedTokenStream Char) source))
  (let start (? StreamMark) (if-expr marked (call (. stream mark)) null))
  (let largest Size 0)
  (for (in name (~> stream \line))
    (if (> (. stream buffer size) largest)
      (= largest (. stream buffer size))
    )
  )
  (let location (. stream location))
  (if start
    (call (. stream rewind) start)
    (return (str (> largest 65536) " " (call (. (call (. stream read) 4) slice-string))))
  )
  (return (str largest " " (. location line) ":" (. location column)))
)

(fn-expr + line String (over: Char)
  (~ (seq (: name (+ (seq (! "\n") .))) "\n")
    name
  )
)
```

```output
65536 20000:0
65536 80000:0
true line
```

### A pipeline over tokens that are not characters releases delivered input too.

```brix
(main
  (let words (call (of List String)))
  (for (in index (..< 0 3000))
    (+= words (str "w" index))
  )
  (let count Size 0)
  (for (in pair (~> (call word-source words) \word-pair))
    (+= count 1)
  )
  (print count)
)

(fn-do + word-source (of Iterable String)
  (param words (list String))
  (for (in word words)
    (yield word)
  )
)

(fn-expr + word-pair String (over: String)
  (~ (seq (: left .) (: right .))
    (str left " " right)
  )
)
```

```output
1500
```

### Pulls, bulk reads and delimiter reads release consumed input unless a mark is held.

```brix
(import /core/tokenstream)
(blank)

(main
  (print (call direct-reads))
  (print (call marked-reads))
  (print (call split-line-end))
)

(fn-do + direct-reads String
  (let stream (call (of ChunkedTokenStream Byte) (call TextSource (as (* "abcdefghij\n" 30000) ([] Byte)))))
  (let block ([] Byte 3))
  (let good true)
  (let largest Size 0)
  (for (in record (..< 0 30000))
    (let pulled (!! (<- stream)))
    (let read (call (. stream read) 3 block))
    (let head (call (. stream read) "g"))
    (let tail (call (. stream read) "\n"))
    (if (or (!= pulled 97u8) (!= read 3) (!= (call (. block slice-string) (..< 0 3)) "bcd") (!= head "ef") (!= tail "hij"))
      (= good false)
    )
    (if (and (== record 20000) (!= (. stream location line) 20001))
      (= good false)
    )
    (if (> (. stream buffer size) largest)
      (= largest (. stream buffer size))
    )
  )
  (let location (. stream location))
  (return (str good " " largest " " (. location line) ":" (. location column)))
)

(fn-do + marked-reads String
  (let stream (call (of ChunkedTokenStream Byte) (call TextSource (as (* "abcdefghij\n" 30000) ([] Byte)))))
  (<- stream)
  (let mark (call (. stream mark)))
  (let count Size 0)
  (while (call (. stream read) "\n")
    (+= count 1)
  )
  (call (. stream rewind) mark)
  (let location (. stream location))
  (return (str count " " (== (!! (<- stream)) 98u8) " " (. location line) ":" (. location column)))
)

(fn-do + split-line-end String
  (let stream (call (of ChunkedTokenStream Byte) (call TextSource (as (+ (* "a" 1023) "\r\nb") ([] Byte)))))
  (for (in index (..< 0 1026))
    (<- stream)
  )
  (let location (. stream location))
  (return (str (. location line) ":" (. location column)))
)

(type + TextSource
  (implements ByteReadable)
  (prop + data ([] Byte))
  (prop + cursor Index)
  (fn-do + This _
    (param data _)
  )
  (fn-do + read Size
    (param size Size)
    (param buffer ([] Byte))
    (let remaining (- (. data size) cursor))
    (let amount (if-expr (< remaining size) remaining size))
    (if amount
      (= ([] buffer (..< 0 amount)) ([] data (..< cursor (+ cursor amount))))
      (+= cursor amount)
    )
    (return amount)
  )
)
```

```output
true 65536 30000:0
30000 true 0:1
1:1
```

### A captured sequence is its whole matched text, whichever kinds of repetition it contains.

```brix
(main
  (print (call captured "c:d83d"))
  (print (call captured "t:d83d;"))
  (print (call captured "o:ab12"))
  (print (call captured "o:xyz"))
  (print (call captured "a:ab12"))
  (print (call captured "a:"))
)

(fn-do + captured String
  (param text String)
  (return
    (match text
      (~ (| \counted \terminated \choice \absent))
      (default
        (inline "unmatched")
      )
    )
  )
)

(fn-expr + counted String (over: Char)
  (~ (seq "c:" (: digits (seq (chars "dD") (chars "89abAB") ({} (chars "0-9a-fA-F") 2))))
    (str "counted " digits)
  )
)

(fn-expr + terminated String (over: Char)
  (~ (seq "t:" (: digits (seq (chars "dD") ({} (chars "0-9a-fA-F") 3))) ";")
    (str "terminated " digits " " (. digits length))
  )
)

(fn-expr + choice String (over: Char)
  (~ (seq "o:" (: text (| (seq ({} (chars "a-f") 2) ({} (chars "0-9") 2)) ({} (chars "x-z") 3))))
    (str "choice " text)
  )
)

(fn-expr + absent String (over: Char)
  (~ (seq "a:" (: text (? (seq ({} (chars "a-f") 2) ({} (chars "0-9") 2)))))
    (str "absent " (?? text "none"))
  )
)
```

```output
counted d83d
terminated d83d 4
choice ab12
choice xyz
absent ab12
absent none
```

### Named text rules keep their match results when callers discard them or read only span lengths.

```brix
(import /core/tokenstream)
(blank)

(main
  (for (in source (list "héllo 42 wörld" "abc" "7 é" "héllo!" "'abc'" "12ab"))
    (print (str (call measured source) " " (call discarded source) " " (call mixed source) " " (call deferred source)))
    (print (str (call backtracked source) " " (call quoted source) " " (call nested source) " " (call module-measured source)))
  )
  (blank)
  (let slow (call SlowSource (as (++ (* "日本語" 300) ";bb;é;") ([] Byte)) 7))
  (print (call record-lengths (call (of ChunkedTokenStream Char) slow)))
)

(fn-do + measured Size
  (param source String)
  (return
    (match source
      (~ (seq (? \digits) (: first \word) (? \space) (: second (? \word)))
        (+ (* (. first length) 10) (?? (?. second length) 0))
      )
      (default
        (inline 0)
      )
      (blank)
      (lazy digits String
        (pattern
          (~ (seq (+ (chars "0-9")) (? " ")))
        )
      )
      (blank)
      (lazy word String
        (pattern
          (~ (+ (chars "a-zé-ö")))
        )
      )
      (blank)
      (lazy space
        (pattern
          (~ (+ (| " " (chars "0-9"))))
        )
      )
    )
  )
)

(fn-do + discarded Size
  (param source String)
  (return
    (match source
      (~ \module-word
        2
      )
      (~ \module-number
        3
      )
      (~ \module-space
        5
      )
      (default
        (inline 0)
      )
    )
  )
)

(fn-do + mixed String
  (param source String)
  (return
    (match source
      (~ (seq (: word \module-word) \module-space (: number \module-number))
        (str word ":" (. word length) "#" (. number length))
      )
      (~ (: word \module-word)
        (str "<" (. word length) ">")
      )
      (default
        (inline "miss")
      )
    )
  )
)

(fn-do + deferred Size
  (param source String)
  (return
    (match source
      (~ (seq (: word \module-word) (: rest (* .)))
        (+ doubled (. rest length))
        (lazy doubled (* (. word length) 2))
      )
      (default
        (inline 0)
      )
    )
  )
)

(fn-do + backtracked String
  (param source String)
  (return
    (match source
      (~ (seq (: word \module-word) "!")
        (str "bang " (. word length))
      )
      (~ (seq (: word \module-word) " ")
        (str "gap " (+ (. word length) 100))
      )
      (~ (: word \module-word)
        (str "plain " word)
      )
      (default
        (inline "miss")
      )
    )
  )
)

(fn-do + quoted Size
  (param source String)
  (return
    (match source
      (~ (seq (: quote \module-quote) (: body \module-word) $quote)
        (+ (. body length) (* (. quote length) 100))
      )
      (default
        (inline 0)
      )
    )
  )
)

(fn-do + nested Size
  (param source String)
  (return
    (match source
      (~ (: outer \prefixed-word)
        (. outer length)
      )
      (default
        (inline 0)
      )
      (blank)
      (lazy prefixed-word String
        (pattern
          (~ (seq (? \module-number) \module-word))
        )
      )
    )
  )
)

(fn-do + module-measured Size
  (param source String)
  (let total (as 0 Size))
  (let tokens
    (match source
      (~ \measured-document)
      (default
        (inline (call (of ImmutableArray Size)))
      )
    )
  )
  (for (in token tokens)
    (+= total token)
  )
  (return total)
)

(fn-do + record-lengths String
  (param stream (of ChunkedTokenStream Char))
  (let seen (call (of List String)))
  (for (in length (~> stream \record-length))
    (+= seen (str length))
  )
  (return (call (. "," join) seen))
)

(fn-expr + measured-document _ (over: Char)
  (~ (: tokens (+ \measured-token))
    tokens
  )
)

(fn-expr + measured-token Size (over: Char)
  (~ (: word \module-word)
    (* (. word length) 2)
  )
  (~ (: number \module-number)
    (* (. number length) 3)
  )
  (~ \module-space
    100
  )
)

(fn-expr + record-length Size (over: Char)
  (~ (seq (: name \record-name) ";")
    (. name length)
  )
)

(fn-expr + record-name String (over: Char)
  (~ (+ (seq (! ";") .)))
)

(fn-expr + module-word String (over: Char)
  (~ (+ (chars "a-zé-ö")))
)

(fn-expr + module-number String (over: Char)
  (~ (+ (chars "0-9")))
)

(fn-expr + module-quote String (over: Char)
  (~ (| "'" "\""))
)

(fn-expr + module-space _ (over: Char)
  (~ (+ " "))
)

(type + SlowSource
  (implements ByteReadable)
  (prop + data ([] Byte))
  (prop + limit Size)
  (prop + cursor Index)
  (fn-do + This _
    (param data _)
    (param limit _)
  )
  (fn-do + read Size
    (param size Size)
    (param buffer ([] Byte))
    (let remaining (- (. data size) cursor))
    (let wanted (if-expr (< size limit) size limit))
    (let amount (if-expr (< remaining wanted) remaining wanted))
    (if amount
      (= ([] buffer (..< 0 amount)) ([] data (..< cursor (+ cursor amount))))
      (+= cursor amount)
    )
    (return amount)
  )
)
```

```output
55 0 miss 19
miss 0 0 226
30 2 <3> 6
plain abc 0 3 6
10 0 miss 0
miss 0 0 105
0 0 miss 11
bang 5 0 0 0
0 0 miss 0
miss 103 0 0
20 0 miss 0
miss 0 4 10
900,2,1
```

### Discarded and length-only named text results never reach the participant materializer, and retained text outlives its source.

```brix
(import /core/arena)
(import /core/tokenstream)
(import /BxGC/BxGC.h)
(blank)

(main
  (let scratch (call Arena))
  (let arena (call Arena))
  (let owner (call CountingOwner arena))
  (let input (. "héllo 42!" codepoints))
  (let source (call (of Array Char) (call (of (. scratch allocate) Char) (. input length)) (. input length)))
  (for (in i (..< 0 (. input length)))
    (= ([] source i) ([] input i))
  )
  (let stream (call (of ArrayTokenStream Char) source))
  (blank)
  (print (call discarded stream owner))
  (print (. owner materialized))
  (= (. stream cursor) 0)
  (print (call measured stream owner))
  (print (. owner materialized))
  (= (. stream cursor) 0)
  (print (call deferred stream owner))
  (print (. owner materialized))
  (= (. stream cursor) 0)
  (print (call retried stream owner))
  (print (. owner materialized))
  (= (. stream cursor) 0)
  (let kept (call kept stream owner))
  (print (. owner materialized))
  (= (. stream cursor) 0)
  (let forwarded (call forwarded stream owner))
  (print (. owner materialized))
  (= (. stream cursor) 0)
  (let unowned (call unowned stream))
  (print (. owner materialized))
  (blank)
  (call (. scratch reset))
  (for (in i (..< 0 20))
    (let later
      (match "later 7!"
        (with owner)
        (~ (seq (: word \counted-word) \counted-space (: number \counted-number) "!")
          (str word "/" (. number length))
        )
        (throw (call Panic "later match failed"))
      )
    )
    (call C:brixGCCollect)
  )
  (blank)
  (print kept)
  (print forwarded)
  (print unowned)
  (print (call (. arena owns) (as (. (as kept Atom) address) Pointer)))
  (print (call (. arena owns) (as (. (as forwarded Atom) address) Pointer)))
  (call (. scratch release))
  (call (. arena release))
)

(fn-do + discarded Size
  (param stream (of TokenStream Char))
  (param owner CountingOwner)
  (return
    (match stream
      (with owner)
      (~ \counted-word
        7
      )
      (default
        (inline 0)
      )
    )
  )
)

(fn-do + measured Size
  (param stream (of TokenStream Char))
  (param owner CountingOwner)
  (return
    (match stream
      (with owner)
      (~ (seq (: word \counted-word) \counted-space (: number \counted-number) "!")
        (+ (* (. word length) 10) (. number length))
      )
      (default
        (inline 0)
      )
    )
  )
)

(fn-do + deferred Size
  (param stream (of TokenStream Char))
  (param owner CountingOwner)
  (return
    (match stream
      (with owner)
      (~ (seq (: word \counted-word) \counted-space (: number \counted-number) "!")
        (+ weight (. number length))
        (lazy weight (* (. word length) 10))
      )
      (default
        (inline 0)
      )
    )
  )
)

(fn-do + retried Size
  (param stream (of TokenStream Char))
  (param owner CountingOwner)
  (return
    (match stream
      (with owner)
      (~ (seq (: word \counted-word) \counted-space (: number \counted-number) "?")
        (. word length)
      )
      (~ (seq (: word \counted-word) \counted-space (: number \counted-number) "!")
        (+ (* (. word length) 100) (. number length))
      )
      (default
        (inline 0)
      )
    )
  )
)

(fn-do + kept String
  (param stream (of TokenStream Char))
  (param owner CountingOwner)
  (return
    (match stream
      (with owner)
      (~ (seq (: word \counted-word) \counted-space (: number \counted-number) "!")
        (str word "/" (. number length))
      )
      (default
        (inline "")
      )
    )
  )
)

(fn-do + forwarded String
  (param stream (of TokenStream Char))
  (param owner CountingOwner)
  (return
    (match stream
      (with owner)
      (~ (seq (: word \counted-word) \counted-space \counted-number "!")
        word
      )
      (default
        (inline "")
      )
    )
  )
)

(fn-do + unowned String
  (param stream (of TokenStream Char))
  (return
    (match stream
      (~ (seq (: word \counted-word) \counted-space (: number \counted-number) "!")
        (str word "/" (. number length))
      )
      (default
        (inline "")
      )
    )
  )
)

(fn-expr + counted-word String (over: Char)
  (~ (+ (chars "a-zé-ö")))
)

(fn-expr + counted-number String (over: Char)
  (~ (+ (chars "0-9")))
)

(fn-expr + counted-space _ (over: Char)
  (~ (+ " "))
)

(type + CountingOwner
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + materialized Size)
  (fn-do + This _
    (param arena Arena)
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= materialized 1)
    (return (call (. arena copy-span) source start end))
  )
  (fn-do + materialize-value String
    (param source String)
    (+= materialized 1)
    (return (call (. arena copy-string) source))
  )
  (fn-expr + mark Size
    (throw (call Panic "Unexpected source materialization mark"))
  )
  (fn-do + rewind _
    (param mark Size)
    (throw (call Panic "Unexpected source materialization rewind"))
  )
)
```

```output
7
0
52
0
52
0
502
0
2
4
4
héllo/2
héllo
héllo/2
true
true
```

### An action that reads only numbers of `it`, an optional name read only through `?.`, and a name captured over alternatives that end with a sequence never reach the participant materializer.

```brix
(import /core/arena)
(import /core/tokenstream)
(blank)

(main
  (let scratch (call Arena))
  (let arena (call Arena))
  (let owner (call CountingOwner arena))
  (let stream (call text-stream scratch "héllo 42!"))
  (let short (call text-stream scratch "héllo !"))
  (blank)
  (print (call it-measured stream owner))
  (print (. owner materialized))
  (= (. stream cursor) 0)
  (print (call optional-measured stream owner))
  (print (. owner materialized))
  (print (call optional-measured short owner))
  (print (. owner materialized))
  (= (. stream cursor) 0)
  (print (call choice-measured stream owner))
  (print (. owner materialized))
  (call (. scratch release))
  (call (. arena release))
)

(fn-do + text-stream (of ArrayTokenStream Char)
  (param scratch Arena)
  (param text String)
  (let input (. text codepoints))
  (let source (call (of Array Char) (call (of (. scratch allocate) Char) (. input length)) (. input length)))
  (for (in i (..< 0 (. input length)))
    (= ([] source i) ([] input i))
  )
  (return (call (of ArrayTokenStream Char) source))
)

(fn-do + it-measured Size
  (param stream (of TokenStream Char))
  (param owner CountingOwner)
  (return
    (match stream
      (with owner)
      (~ \counted-word
        (* (. it length) 2)
      )
      (default
        (inline 0)
      )
    )
  )
)

(fn-do + optional-measured Size
  (param stream (of TokenStream Char))
  (param owner CountingOwner)
  (return
    (match stream
      (with owner)
      (~ (seq (: word \counted-word) \counted-space (: number (? \counted-number)) "!")
        (+ (* (. word length) 10) (?? (?. number length) 0))
      )
      (default
        (inline 0)
      )
    )
  )
)

(fn-do + choice-measured Size
  (param stream (of TokenStream Char))
  (param owner CountingOwner)
  (return
    (match stream
      (with owner)
      (~ (seq (: token (| \counted-number (seq \counted-word \counted-space \counted-number))) (: rest (* .)))
        (+ (* (. token length) 10) (. rest length))
      )
      (default
        (inline 0)
      )
    )
  )
)

(fn-expr + counted-word String (over: Char)
  (~ (+ (chars "a-zé-ö")))
)

(fn-expr + counted-number String (over: Char)
  (~ (+ (chars "0-9")))
)

(fn-expr + counted-space _ (over: Char)
  (~ (+ " "))
)

(type + CountingOwner
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + materialized Size)
  (fn-do + This _
    (param arena Arena)
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= materialized 1)
    (return (call (. arena copy-span) source start end))
  )
  (fn-do + materialize-value String
    (param source String)
    (+= materialized 1)
    (return (call (. arena copy-string) source))
  )
  (fn-expr + mark Size
    (throw (call Panic "Unexpected source materialization mark"))
  )
  (fn-do + rewind _
    (param mark Size)
    (throw (call Panic "Unexpected source materialization rewind"))
  )
)
```

```output
10
0
52
0
50
0
81
0
```

### Length-only captures of named text rules leave rollback of a failed alternative intact.

```brix
(main
  (let journal (call ParseJournal))
  (let result
    (match "héllo 42!"
      (with journal)
      (~ (seq (: word \journal-word) " " \journal-count "?")
        (. word length)
      )
      (~ (seq (: word \journal-word) " " \journal-count "!")
        (+ (. word length) 10)
      )
      (default
        (inline 0)
      )
      (blank)
      (lazy journal-count I32
        (pattern
          @rollback
          (~ (: digits \journal-digits)
            (+ (call (. journal increment)) (as (. digits length) I32))
          )
        )
      )
    )
  )
  (blank)
  (print result)
  (print (. journal count))
)

(fn-expr + journal-word String (over: Char)
  (~ (+ (chars "a-zé-ö")))
)

(fn-expr + journal-digits String (over: Char)
  (~ (+ (chars "0-9")))
)

(type + ParseJournal
  (implements (of PatternRollback Size))
  (prop + count Size)
  (fn-expr + mark Size
    (+ count 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (= count (- mark 1))
  )
  (fn-do + increment I32
    (+= count 1)
    (return 7)
  )
)
```

```output
15
1
```

```brix
(import /core/tokenstream)
(blank)

(main
  (let source (call StagedSource (list "ab;" "c" "d;e" ";")))
  (let records (~> (call (of ChunkedTokenStream Char) source) \record))
  (print (<- records))
  (print (. source read-count))
  (print (<- records))
  (print (. source read-count))
  (print (<- records))
  (print (. source read-count))
  (print (== (<- records) null))
)

(fn-expr + record String (over: Char)
  (~ (seq (: name (+ (chars "a-z"))) ";")
    name
  )
)

(type + StagedSource
  (implements ByteReadable)
  (prop + stages (list String))
  (prop + read-count Size)
  (fn-do + This _
    (param stages _)
  )
  (fn-do + read Size
    (param size Size)
    (param buffer ([] Byte))
    (let index read-count)
    (+= read-count 1)
    (if (>= index (. stages length))
      (return 0)
    )
    (let bytes (as ([] stages index) ([] Byte)))
    (= ([] buffer (..< 0 (. bytes size))) bytes)
    (return (. bytes size))
  )
)
```

```output
ab
1
cd
3
e
4
true
```

```brix crash
(main
  (print
    (match "a"
      (~ (seq (* (: ...outer "z")) (call \callee (into missing outer)))
        outer
      )
      (blank)
      (lazy callee (of ImmutableArray String)
        (pattern
          (~ (: ...values "a")
            values
          )
        )
      )
    )
  )
)
```

```output
Exception: Pattern callee has no accumulating capture missing
<no file:5>
          (~ (seq (* (: ...outer "z")) (call \callee (into missing outer)))
                                                     ^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (print
    (match "a"
      (~ (call \callee (into values outer)))
      (blank)
      (lazy callee (of ImmutableArray String)
        (pattern
          (~ (: ...values "a")
            values
          )
        )
      )
    )
  )
)
```

```output
Exception: Collector mapping target outer is not an active accumulating capture
<no file:5>
          (~ (call \callee (into values outer)))
                           ^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (let result
    (match "x"
      (~ \mixed)
    )
  )
  (throw (call Panic "mixed choice failed") (if (not result)))
  (blank)
  (print (str result))
)

(fn-expr + mixed _ (over: Char)
  (~ "x"
    "value"
  )
  (~ .)
)
```

```output
value
```

```brix
(import /core/tokenstream)
(blank)

(main
  (let text (* "ab;" 30000))
  (let stream (call (of ChunkedTokenStream Char) (call TextSource (as text ([] Byte)))))
  (let count Size 0)
  (for (in name (~> stream \field))
    (+= count 1)
  )
  (print count)
  (print (< (. stream buffer size) (. text size)))
)

(fn-expr + field String (over: Char)
  (~ (seq (: name (+ (chars "a-z"))) ";")
    name
  )
)

(type + TextSource
  (implements ByteReadable)
  (prop + data ([] Byte))
  (prop + cursor Index)
  (fn-do + This _
    (param data _)
  )
  (fn-do + read Size
    (param size Size)
    (param buffer ([] Byte))
    (let remaining (- (. data size) cursor))
    (let amount (if-expr (< remaining size) remaining size))
    (if amount
      (= ([] buffer (..< 0 amount)) ([] data (..< cursor (+ cursor amount))))
      (+= cursor amount)
    )
    (return amount)
  )
)
```

```output
30000
true
```

```brix
(import /core/arena)
(import /core/tokenstream)
(blank)

(main
  (let arena (call Arena))
  (let helper (call SourceOnlyOwner arena))
  (let stream (call (of ArrayTokenStream Char) (. "éabc😀!" codepoints)))
  (print (call source-only-helper stream helper))
  (call (. arena release))
)

(fn-do + source-only-helper Size (without gc)
  (param stream (of TokenStream Char))
  (param helper SourceOnlyOwner)
  (return
    (match stream
      (with helper)
      (~ (call \source-only-measure (~ (seq . (+ (chars "a-z")) . "!"))))
      (default
        (inline 0)
      )
    )
  )
)

(fn-expr + source-only-measure Size (over: Char)
  (param pattern (of PatternFunc Char String))
  (~ \local-value
    (. it length)
  )
  (where
    (lazy local-value String
      (pattern
        (~ $pattern)
      )
    )
  )
)

(type + SourceOnlyOwner
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (prop + arena Arena)
  (fn-do + This _
    (param arena Arena)
  )
  (fn-expr + (of materialize-capture Token) String (without gc)
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (call (. arena copy-span) source start end)
  )
)
```

```output
6
```

### Each arm of a match on text must match all of it. An arm that leaves text over fails, and the next arm is tried.

```brix
(main
  (print
    (match "ab"
      (~ "a")
      (~ "ab")
    )
  )
  (print
    (match "ab"
      (~ "a"
        1
      )
      (~ "ab"
        2
      )
    )
  )
  (print
    (match "abc"
      (~ "a"
        "a"
      )
      (default
        (inline "none")
      )
    )
  )
)
```

```output
ab
2
none
```

```brix
(main
  (print (call counted "abc"))
  (print (call counted "a"))
)

(fn-do + counted Size
  (param source String)
  (return
    (match source
      (~ "a"
        5
      )
      (default
        (inline 0)
      )
    )
  )
)
```

```output
0
5
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (let helper (call Owner arena))
  (print (call measure "abc" helper))
  (print (. helper count))
  (call (. arena release))
)

(fn-do + measure Size
  (param source String)
  (param helper Owner)
  (return
    (match source
      (with helper)
      (~ \outer)
      (default
        (inline 0)
      )
    )
  )
)

(fn-expr + outer Size (over: Char)
  (~ \inner
    (. it length)
  )
)

(fn-expr + inner String (over: Char)
  (~ (+ (chars "a-z"))
    (+ it "!")
  )
)

(type + Owner
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (prop + arena Arena)
  (prop + count I32)
  (fn-do + This _
    (param arena Arena)
  )
  (fn-expr + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (before
      (do
        (+= count 1)
      )
    )
    (call (. arena copy-span) source start end)
  )
)
```

```output
4
1
```

```brix
(main
  (let h (call H))
  (print
    (match "abc"
      (with h)
      (~ \word)
      (default
        (inline "none")
      )
      (blank)
      (lazy word String
        (pattern
          (~ (+ (chars "a-z")))
        )
      )
    )
  )
)

(type + H
  (implements (of PatternMaterializationRoot String))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (prop + count Size)
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (print (str "  mc " start " " end))
    (return (+ (call (. source slice-string) (..< start end)) "*"))
  )
)
```

```output
  mc 0 3
abc*
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (let helper (call Owner arena))
  (print (call run "abc!" helper))
  (print (. helper count))
)

(fn-do + run Size
  (param source String)
  (param helper Owner)
  (return
    (match (. source codepoints)
      (with helper)
      (~ (call \measure (~ \word)))
      (default
        (inline 0)
      )
    )
  )
)

(fn-expr + word String (over: Char)
  (~ (+ (chars "a-z")))
)

(fn-expr + measure Size (over: Char)
  (param pattern (of PatternFunc Char String))
  (~ (seq $pattern "!")
    (. it length)
  )
)

(type + Owner
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (prop + arena Arena)
  (prop + count I32)
  (fn-do + This _
    (param arena Arena)
  )
  (fn-expr + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (before
      (do
        (+= count 1)
      )
    )
    (call (. arena copy-span) source start end)
  )
)
```

```output
4
1
```

### A pattern given to a rule and matched as `$pattern` gives what the pattern gives, as `\pattern` does, when it is all of the rule's arm under a helper that makes text.

```brix
(main
  (let helper (call Owner))
  (print
    (match "ab"
      (with helper)
      (~ (call \wrap (~ \loud)))
      (default
        (inline "none")
      )
    )
  )
  (print (. helper count))
)

(fn-expr + wrap String (over: Char)
  (param pattern (of PatternFunc Char String))
  (~ $pattern)
)

(fn-expr + loud String (over: Char)
  (~ (+ (chars "a-z"))
    "LOUD"
  )
)

(type + Owner
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternRollback Int))
  (prop + count _ (= 0))
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= count 1)
    (return (+ "<" (call (. source slice-string) (..< start end)) ">"))
  )
  (fn-expr + mark Int
    count
  )
  (fn-do + rewind _
    (param mark Int)
    (= count mark)
  )
)
```

```output
LOUD
0
```

```brix
(main
  (print
    (match "aa"
      (~ ({} (seq \r "a") 2)
        1
      )
      (default
        (inline 0)
      )
    )
  )
  (print
    (match "a"
      (~ ({} (seq \r "a") 2)
        1
      )
      (default
        (inline 0)
      )
    )
  )
  (print
    (match "aab"
      (~ (seq ({} (seq \r "a") 1 2) "b")
        3
      )
      (default
        (inline 0)
      )
    )
  )
)

(fn-expr + r I32 (over: Char)
  (~ ""
    7
  )
)
```

```output
1
0
3
```

```brix
(main
  (print (call g 4))
)

(fn-expr + g I32
  (param n Size)
  (match "z"
    (~ "z"
      (as n I32)
    )
    (throw (call Panic "x"))
  )
)
```

```output
4
```

```brix crash
(main
  (print (call run "abc!"))
)

(fn-do + run Size
  (param source String)
  (return
    (match source
      (~ (call \outer (~ (+ (chars "a-z")))))
      (default
        (inline 0)
      )
    )
  )
)

(fn-expr + outer Size (over: Char)
  (param pattern (of PatternFunc Char String))
  (~ (call \measure pattern))
)

(fn-expr + measure Size (over: Char)
  (param pattern (of PatternFunc Char String))
  (~ (seq $pattern "!")
    (. it length)
  )
)
```

```output
Exception: Name "pattern" not found
<no file:20>
      (~ (call \measure pattern))
                        ^^^^^^^
```

```brix crash
(main
  (print (call run "abc!"))
)

(fn-do + run Size
  (param source String)
  (return
    (match source
      (~ (call \measure (~ (+ (chars "a-z")))))
      (default
        (inline 0)
      )
    )
  )
)

(fn-do + keep Size
  (param p (of PatternFunc Char String))
  (return 5)
)

(fn-expr + measure Size (over: Char)
  (param pattern (of PatternFunc Char String))
  (~ (seq $pattern "!")
    (+ (. it length) (call keep pattern))
  )
)
```

```output
Exception: Unable to compile pattern action: Name not found
<no file:25>
      (~ (seq $pattern "!")
      ^^^^^^^^^^^^^^^^^^^^
```

### A match's local rule that captures a name and runs an action can be given to a rule as a pattern.

```brix
(main
  (print
    (match "[x]"
      (~ (call \bracketed (~ \shout)))
      (lazy shout
        (pattern
          (~ (: letter (chars "a-z"))
            (+ letter "!")
          )
        )
      )
    )
  )
)

(fn-expr + bracketed String (over: Char)
  (param inner (of PatternFunc Char String))
  (~ (seq "[" (: value $inner) "]")
    value
  )
)
```

```output
x!
```

### A pattern given to a rule as an argument can't capture names.

```brix compile-error
(main
  (print
    (match "[x]"
      (~ (call \bracketed (~ (: letter (chars "a-z")))))
    )
  )
)

(fn-expr + bracketed String (over: Char)
  (param inner (of PatternFunc Char String))
  (~ (seq "[" (: value $inner) "]")
    value
  )
)
```

```output
Exception: A pattern given as an argument can't capture names
<no file:5>
          (~ (call \bracketed (~ (: letter (chars "a-z")))))
                                 ^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (let words
    (match "ab,c,de!"
      (~ (seq (* (| \letter \digit)) (* (| \letter ",")) "!")
        "done"
      )
      (default
        (inline "none")
      )
    )
  )
  (print words)
)

(fn-expr + letter Int (over: Char)
  (~ (chars "a-z")
    1
  )
)

(fn-expr + digit String (over: Char)
  (~ (chars "0-9")
    it
  )
)
```

```output
done
```

```brix
(main
  (let helper (call Helper))
  (let word
    (match "alpha!"
      (with helper)
      (~ \tagged)
      (throw (call Panic "failed"))
      (blank)
      (lazy tagged Tag
        (pattern
          (~ (seq (: text (+ (chars "a-z"))) "!")
            text
          )
        )
      )
    )
  )
  (print (. word size))
  (print (. helper count))
)

(type + Tag (is Tuple)
  (prop + size Size)
)

(type + Helper
  (implements (of PatternMaterializationRoot String))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternRollback Size))
  (prop + count Size)
  (blank)
  (delegate (of PatternCaptureMaterializer Char Tag) (of PatternCaptureMaterializer Byte Tag))
  (prop _ tags _ (:= (call Tags this)))
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (return "?")
  )
  (fn-expr + mark Size
    1
  )
  (fn-do + rewind _
    (param mark Size)
  )
)

(type _ Tags (is Tuple)
  (implements (of PatternCaptureMaterializer Char Tag))
  (implements (of PatternCaptureMaterializer Byte Tag))
  (prop _ helper Helper)
  (fn-do + This _
    (param source Helper)
    (= helper source)
  )
  (fn-do + (of materialize-capture Token) Tag
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= (. helper count) 1)
    (return (call Tag (- end start)))
  )
)
```

```output
5
1
```

```brix crash
(main
  (let helper (call Helper))
  (let word
    (match "alpha!"
      (with helper)
      (~ \suffixed)
      (throw (call Panic "failed"))
      (blank)
      (lazy suffixed String
        (pattern
          (~ (seq (: text (+ (chars "a-z"))) "!")
            (+ (call checked text) "-x")
          )
        )
      )
    )
  )
  (print word)
)

(fn-do + checked String
  (param text String)
  (if (== text "alpha")
    (throw (call Panic (str "checked " text)))
  )
  (return text)
)

(type + Helper
  (implements (of PatternMaterializationRoot String))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (return "alpha")
  )
  (fn-do + materialize-value String
    (param input String)
    (return input)
  )
  (fn-expr + mark Size
    1
  )
  (fn-do + rewind _
    (param mark Size)
  )
)
```

```stderr
Unhandled panic: checked alpha
<no file:25>

Task 1

Stack trace:
  0  __source__:checked no file:25:5
  1  __source__:main no file:13:16
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

```brix
(import /core/arena)
(blank)

(main
  (let output (call Arena))
  (let helper (call Helper output))
  (let words
    (match "a,b,c!"
      (with helper)
      (~ (seq (+ (seq (: values (chars "a-z")) (? ","))) "!")
        values
      )
      (throw (call Panic "failed"))
    )
  )
  (print (. words length))
  (print ([] words 2))
  (print (. helper reads))
  (print (> (. output active-bytes) 0))
  (let counted
    (match "aaa!"
      (with helper)
      (~ \ones)
      (throw (call Panic "failed"))
    )
  )
  (print (. counted length))
  (print (. helper reads))
  (print (> (. output active-bytes) 0))
  (call (. output release))
)

(fn-expr + ones (of ImmutableArray Int) (over: Char)
  (~ (seq (+ (: n \one)) "!")
    n
  )
)

(fn-expr + one Int (over: Char)
  (~ "a"
    1
  )
)

(type + Helper
  (implements (of PatternCollectorStorage ArenaBacking))
  (prop + arena Arena)
  (prop + reads Size)
  (fn-expr + pattern-collector-context Arena :get
    (before
      (do
        (+= reads 1)
      )
    )
    arena
  )
  (fn-do + This _
    (param source Arena)
    (= arena source)
  )
)
```

```output
3
c
1
true
3
2
true
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (let helper (call Helper arena))
  (let words
    (match "a,b,c!"
      (with helper)
      (~ \letters)
      (throw (call Panic "failed"))
      (blank)
      (lazy letters (of ImmutableArray String)
        (pattern
          (~ (seq (+ (seq (: letter (chars "a-z")) (? ","))) "!")
            letter
          )
        )
      )
    )
  )
  (print ([] words 2))
  (print (. helper reads))
  (print (> (. arena active-bytes) 0))
  (call (. arena release))
)

(type + Helper
  (implements (of PatternCollectorStorage ArenaBacking))
  (prop + arena Arena)
  (prop + reads Size)
  (fn-expr + pattern-collector-context Arena :get
    (before
      (do
        (+= reads 1)
      )
    )
    arena
  )
  (fn-do + This _
    (param source Arena)
    (= arena source)
  )
)
```

```output
c
1
true
```

```brix
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let arena (call Arena))
  (let helper (call Helper arena))
  (let words
    (match "ab,c,de!"
      (with helper)
      (~ (seq (: items (call \delimited "," (~ (call \word 1)))) "!")
        items
      )
      (throw (call Panic "failed"))
    )
  )
  (print (. words length))
  (print ([] words 2))
  (print (> (. arena active-bytes) 0))
  (call (. arena release))
)

(fn-expr + word String (over: Char)
  (param n Int)
  (~ (+ (chars "a-z"))
    it
  )
)

(type + Helper
  (implements (of PatternCollectorStorage ArenaBacking))
  (prop + arena Arena)
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + This _
    (param source Arena)
    (= arena source)
  )
)
```

```output
3
de
true
```

```brix
(main
  (let total
    (match "ab"
      (~ (seq (: x \first) (: y \second))
        (+ x y)
      )
      (default
        (inline 9)
      )
      (blank)
      (lazy first Int
        (pattern
          (~ "a"
            (+ twice 1)
            (blank)
            (lazy twice (* 2 2))
          )
        )
      )
      (blank)
      (lazy second Int
        (pattern
          (~ "b"
            (+ twice 2)
            (blank)
            (lazy twice (* 3 2))
          )
        )
      )
    )
  )
  (print total)
)
```

```output
13
```

### A rule that states a CaptureSpan gives the span each arm matched, whatever its action gives.

```brix
(main
  (let acted
    (match (. "ab!" codepoints)
      (~ (seq (: span \word) "!")
        span
      )
      (throw (call Panic "no"))
      (blank)
      (lazy word (of CaptureSpan Char)
        (pattern
          (~ (+ (chars "a-z"))
            (call (of CaptureSpan Char) (. "kept" codepoints) 0 4)
          )
          (~ (+ (chars "0-9")))
        )
      )
    )
  )
  (print acted)
  (blank)
  (let bare
    (match (. "12!" codepoints)
      (~ (seq (: span \word) "!")
        span
      )
      (throw (call Panic "no"))
      (blank)
      (lazy word (of CaptureSpan Char)
        (pattern
          (~ (+ (chars "a-z"))
            (call (of CaptureSpan Char) (. "kept" codepoints) 0 4)
          )
          (~ (+ (chars "0-9")))
        )
      )
    )
  )
  (print bare)
)
```

```output
ab
12
```

### A name captured over a rule that states a CaptureSpan reads as text in an action.

```brix
(main
  (let shown
    (match (. "xalpha!" codepoints)
      (~ (seq "x" (: span \word) "!")
        (str (. span codepoints length) "/" (. span length) "/" span)
      )
      (throw (call Panic "no"))
      (blank)
      (lazy word (of CaptureSpan Char)
        (pattern
          (~ (+ (chars "a-z")))
        )
      )
    )
  )
  (print shown)
)
```

```output
5/5/alpha
```

### A name an action gives only to functions that take its CaptureSpan, or reads for its length, reaches them as the span, and no text is made for it.

```brix
(main
  (print
    (match (. "x42!" codepoints)
      (~ (seq "x" (: digits (+ (chars "0-9"))) "!")
        (call offsets digits)
      )
      (default
        (inline "none")
      )
    )
  )
  (print
    (match (. "x42!" codepoints)
      (~ (seq "x" (+ (chars "0-9")) "!")
        (call offsets it)
      )
      (default
        (inline "none")
      )
    )
  )
  (print
    (match (. "x42!" codepoints)
      (~ (seq "x" (: digits \run) "!")
        (call offsets digits)
      )
      (default
        (inline "none")
      )
      (blank)
      (lazy run (of CaptureSpan Char)
        (pattern
          (~ (+ (chars "0-9")))
        )
      )
    )
  )
  (print
    (match (. "x42!" codepoints)
      (~ (seq "x" (: digits (+ (chars "0-9"))) "!")
        (call either digits)
      )
      (default
        (inline "none")
      )
    )
  )
  (print
    (match (. "x42!" codepoints)
      (~ (seq "x" (: digits (+ (chars "0-9"))) "!")
        (str (. digits length) ":" (call offsets digits))
      )
      (default
        (inline "none")
      )
    )
  )
  (print
    (match (. "x42!" codepoints)
      (~ (seq "x" (: digits (+ (chars "0-9"))) "!")
        (str digits "/" (call either digits))
      )
      (default
        (inline "none")
      )
    )
  )
  (blank)
  (let helper (call CountingMaterializer))
  (print
    (match (. "x42!" codepoints)
      (with helper)
      (~ (seq "x" (: digits (+ (chars "0-9"))) "!")
        (call offsets digits)
      )
      (default
        (inline "none")
      )
    )
  )
  (print (. helper materializations))
)

(fn-expr + offsets String
  (param span (of CaptureSpan Char))
  (str (. span start) "-" (. span end))
)

(fn-expr + either String
  (param text String)
  (str "text " text)
)

(fn-expr + either String
  (param span (of CaptureSpan Char))
  (str "span " (call offsets span))
)

(type + CountingMaterializer
  (implements (of PatternMaterializationRoot String))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternRollback Size))
  (prop + materializations Size)
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= materializations 1)
    (return (call (. source slice-string) (..< start end)))
  )
  (fn-expr + mark Size
    (+ materializations 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (= materializations (- mark 1))
  )
)
```

```output
1-3
0-4
1-3
span 1-3
2:1-3
42/text 42
1-3
0
```

### A pattern given to a rule gives its values though nothing in it can panic.

```brix
(import /core/patterns)
(blank)

(main
  (let made
    (match "a,b!"
      (~ (seq (: numbers (call \delimited "," (~ \number-word))) "!")
        numbers
      )
      (default
        (inline (list (as 0 I32)))
      )
    )
  )
  (print made)
)

(fn-expr + number-word _ (over: Char)
  (~ (+ (chars "a-z"))
    (as 7 I32)
  )
)
```

```output
7, 7
```

### A pattern given to a rule reads an argument of the function it is written in.

```brix
(main
  (print (call ends-with "alpha!" "!"))
  (print (call ends-with "alpha?" "!"))
  (print (call ends-with "beta?" "?"))
)

(fn-do + ends-with String
  (param text String)
  (param suffix String)
  (return
    (match text
      (~ (call \forward (~ (seq (+ (chars "a-z")) $suffix))))
      (default
        (inline "none")
      )
    )
  )
)

(fn-expr + forward String (over: Char)
  (param pattern _)
  (~ \pattern)
)
```

```output
alpha!
none
beta?
```

### A pattern given to a rule calls a local rule of the rule it is written in, under the rule's own helper.

```brix
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let notes (call NoteHelper (call Arena)))
  (print
    (match "a,(b,c)"
      (with notes)
      (~ (: parsed (call \noted notes))
        ([] (. notes texts) (- parsed 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print
    (match "a,"
      (with notes)
      (~ (: parsed (call \noted notes))
        ([] (. notes texts) (- parsed 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. notes texts))
  (print (. notes marks))
)

(fn-expr + noted _ (over: Char)
  (param tree NoteHelper)
  (~ (: v \list)
    v
  )
  (where
    (lazy list U32
      (pattern
        (~ (: values (call \delimited "," (~ \element)))
          (call (. tree note) (str (. values length)))
        )
      )
    )
    (blank)
    (lazy element U32
      (pattern
        (~ (chars "a-z")
          (call (. tree note) it)
        )
        (~ (seq "(" (: inner \list) ")")
          inner
        )
      )
    )
  )
)

(type + NoteHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + texts (list String))
  (prop + marks Size)
  (fn-do + This _
    (param source Arena)
    (= arena source)
  )
  (fn-do + note U32
    (param text String)
    (+= texts text)
    (return (as (. texts length) U32))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (return (call (. source slice-string) (..< start end)))
  )
  (fn-expr + mark Size
    (before
      (do
        (+= marks 1)
      )
    )
    (+ (. texts length) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (while (> (. texts length) (- mark 1))
      (call (. texts pop))
    )
  )
)
```

```output
2
no match
a, b, c, 2, 2, a, 1
0
```

### A pattern given to a rule calls a local rule of the rule it is written in, which calls itself.

```brix
(import /core/patterns)
(blank)

(main
  (print
    (match "(a,(b,c))"
      (~ (: total \nested)
        total
      )
      (default
        (inline 0)
      )
    )
  )
  (print
    (match "(a,b"
      (~ (: total \nested)
        total
      )
      (default
        (inline 0)
      )
    )
  )
)

(fn-expr + nested _ (over: Char)
  (~ (: v \group)
    v
  )
  (where
    (lazy group Size
      (pattern
        (~ (seq "(" (: values (call \delimited "," (~ \element))) ")")
          (+ ([] values 0) (. values length))
        )
      )
    )
    (blank)
    (lazy element Size
      (pattern
        (~ (chars "a-z")
          1
        )
        (~ (: inner \group)
          (+ inner 10)
        )
      )
    )
  )
)
```

```output
3
0
```

### A helper makes the text of an optional part a name captures where the part passes, read or not, and again where an action reads the name.

```brix
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call CountingHelper (call Arena)))
  (print
    (match "12ab"
      (with helper)
      (~ (: v (call \unread helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
  (print
    (match "12ab"
      (with helper)
      (~ (: v (call \read helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
  (print
    (match "12"
      (with helper)
      (~ (: v (call \read helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
  (print
    (match "12ab"
      (with helper)
      (~ (: v (call \ended helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
)

(fn-expr + unread _ (over: Char)
  (param tree CountingHelper)
  (~ (: v \unread-number)
    v
  )
  (where
    (lazy unread-number U32
      (pattern
        (~ (seq (+ (chars "0-9")) (: u (? (+ (chars "a-z")))))
          (call (. tree note) "number")
        )
      )
    )
  )
)

(fn-expr + read _ (over: Char)
  (param tree CountingHelper)
  (~ (: v \read-number)
    v
  )
  (where
    (lazy read-number U32
      (pattern
        (~ (seq (+ (chars "0-9")) (: u (? (+ (chars "a-z")))))
          (call (. tree note) (?? u "none"))
        )
      )
    )
  )
)

(fn-expr + ended _ (over: Char)
  (param tree CountingHelper)
  (~ (: v \ended-number)
    v
  )
  (where
    (lazy ended-number U32
      (pattern
        (~ (seq (+ (chars "0-9")) (: u (? (+ (chars "a-z")))) "!")
          (call (. tree note) "ended")
        )
        (~ (seq (+ (chars "0-9")) (+ (chars "a-z")))
          (call (. tree note) "open")
        )
      )
    )
  )
)

(type + CountingHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + made Size)
  (prop + notes (list String))
  (fn-do + This _
    (param source Arena)
    (= arena source)
  )
  (fn-do + note U32
    (param text String)
    (+= notes text)
    (return (as (. notes length) U32))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= made 1)
    (return (call (. source slice-string) (..< start end)))
  )
  (fn-expr + mark Size
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)
```

```output
number
1
ab
3
none
3
open
4
```

### A helper makes the text of a captured call of a rule where the call passes when no action of the pattern makes a value through it, but for a name captured once outside optional parts and repeats.

```brix
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call CountingHelper (call Arena)))
  (print
    (match "12-ab-cd"
      (with helper)
      (~ (: v (call \repeated helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
  (print
    (match "12-ab"
      (with helper)
      (~ (: v (call \optional helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
  (print
    (match "12ab"
      (with helper)
      (~ (: v (call \single helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
  (print
    (match "12-ab-cd"
      (with helper)
      (~ (: v (call \joined helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
)

(fn-expr + repeated _ (over: Char)
  (param tree CountingHelper)
  (~ (: v \repeated-number)
    v
  )
  (where
    (lazy repeated-number U32
      (pattern
        (~ (seq (+ (chars "0-9")) (* (seq "-" (: u \repeated-word))))
          (call (. tree note) "repeated")
        )
      )
    )
    (blank)
    (lazy repeated-word
      (pattern
        (~ (+ (chars "a-z")))
      )
    )
  )
)

(fn-expr + optional _ (over: Char)
  (param tree CountingHelper)
  (~ (: v \optional-number)
    v
  )
  (where
    (lazy optional-number U32
      (pattern
        (~ (seq (+ (chars "0-9")) (? (seq "-" (: u \optional-word))))
          (call (. tree note) "optional")
        )
      )
    )
    (blank)
    (lazy optional-word
      (pattern
        (~ (+ (chars "a-z")))
      )
    )
  )
)

(fn-expr + single _ (over: Char)
  (param tree CountingHelper)
  (~ (: v \single-number)
    v
  )
  (where
    (lazy single-number U32
      (pattern
        (~ (seq (+ (chars "0-9")) (: u \single-word))
          (call (. tree note) "single")
        )
      )
    )
    (blank)
    (lazy single-word
      (pattern
        (~ (+ (chars "a-z")))
      )
    )
  )
)

(fn-expr + joined _ (over: Char)
  (param tree CountingHelper)
  (~ (: v \joined-number)
    v
  )
  (where
    (lazy joined-number U32
      (pattern
        (~ (seq (+ (chars "0-9")) (* (seq "-" (: u \joined-word))))
          (call (. tree note) (call (. ", " join) u))
        )
      )
    )
    (blank)
    (lazy joined-word
      (pattern
        (~ (+ (chars "a-z")))
      )
    )
  )
)

(type + CountingHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + made Size)
  (prop + notes (list String))
  (fn-do + This _
    (param source Arena)
    (= arena source)
  )
  (fn-do + note U32
    (param text String)
    (+= notes text)
    (return (as (. notes length) U32))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= made 1)
    (return (call (. source slice-string) (..< start end)))
  )
  (fn-expr + mark Size
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)
```

```output
repeated
2
optional
3
single
3
ab, cd
5
```

### A helper makes a name an argument of a rule call reads before the call, whether or not the call then passes, and again where an action reads the name.

```brix
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call CountingHelper (call Arena)))
  (print
    (match "12ab"
      (with helper)
      (~ (: v (call \tagging helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
  (print
    (match "12ab"
      (with helper)
      (~ (: v (call \retried helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
)

(fn-expr + tagging _ (over: Char)
  (param tree CountingHelper)
  (~ (: v \tagging-number)
    v
  )
  (where
    (lazy tagging-number U32
      (pattern
        (~ (seq (: n (+ (chars "0-9"))) (: v (call \tagged n)))
          (call (. tree note) (str v "/" n))
        )
      )
    )
  )
)

(fn-expr + retried _ (over: Char)
  (param tree CountingHelper)
  (~ (: v \retried-number)
    v
  )
  (where
    (lazy retried-number U32
      (pattern
        (~ (seq (: n (+ (chars "0-9"))) (call \tagged n) "!")
          (call (. tree note) "ended")
        )
        (~ (seq (: n (+ (chars "0-9"))) (+ (chars "a-z")))
          (call (. tree note) (str "open/" n))
        )
      )
    )
  )
)

(fn-expr + tagged String (over: Char)
  (param tag String)
  (~ (+ (chars "a-z"))
    (str tag ":" it)
  )
)

(type + CountingHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + made Size)
  (prop + notes (list String))
  (fn-do + This _
    (param source Arena)
    (= arena source)
  )
  (fn-do + note U32
    (param text String)
    (+= notes text)
    (return (as (. notes length) U32))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= made 1)
    (return (call (. source slice-string) (..< start end)))
  )
  (fn-expr + mark Size
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)
```

```output
12:ab/12
3
open/12
6
```

### A name captured over a call of a rule that states another type than text, in a repeat, in an optional part or over an optional call, is made by the helper where an action reads it; the optional call makes it where it passes too, and a call no action reads makes it where nothing in the pattern makes a value.

```brix
(import /core/atom-pool)
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call AtomHelper (call Arena)))
  (print
    (match "ab/c/def"
      (with helper)
      (~ (: v (call \path helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (str (. helper texts) " " (. helper atoms)))
  (print
    (match "ab"
      (with helper)
      (~ (: v (call \path helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (str (. helper texts) " " (. helper atoms)))
  (print
    (match "12ab"
      (with helper)
      (~ (: v (call \measure helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (str (. helper texts) " " (. helper atoms)))
  (print
    (match "12"
      (with helper)
      (~ (: v (call \measure helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (str (. helper texts) " " (. helper atoms)))
  (print
    (match "12-ab"
      (with helper)
      (~ (: v (call \suffixed helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (str (. helper texts) " " (. helper atoms)))
  (print
    (match "12ab"
      (with helper)
      (~ (: v (call \unread helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (str (. helper texts) " " (. helper atoms)))
)

(fn-expr + path _ (over: Char)
  (param tree AtomHelper)
  (~ (: v \path-names)
    v
  )
  (where
    (lazy path-names U32
      (pattern
        (~ (seq (: head \path-name) (* (seq "/" (: tail \path-name))))
          (call (. tree note) (str (. head text) " " (. tail length) " " (if-expr (. tail length) (. ([] tail (- (. tail length) 1)) text) "-")))
        )
      )
    )
    (blank)
    (lazy path-name Atom
      (pattern
        (~ (+ (chars "a-z")))
      )
    )
  )
)

(fn-expr + measure _ (over: Char)
  (param tree AtomHelper)
  (~ (: v \measure-number)
    v
  )
  (where
    (lazy measure-number U32
      (pattern
        (~ (seq (: n (+ (chars "0-9"))) (: u (? \measure-unit)))
          (case? u
            (call (. tree note) (str (. u text) "/" n))
          )
          (default
            (call (. tree note) (str "none/" n))
          )
        )
      )
    )
    (blank)
    (lazy measure-unit Atom
      (pattern
        (~ (+ (chars "a-z")))
      )
    )
  )
)

(fn-expr + suffixed _ (over: Char)
  (param tree AtomHelper)
  (~ (: v \suffixed-number)
    v
  )
  (where
    (lazy suffixed-number U32
      (pattern
        (~ (seq (: n (+ (chars "0-9"))) (? (seq "-" (: u \suffixed-unit))))
          (case? u
            (call (. tree note) (str (. u text) "/" n))
          )
          (default
            (call (. tree note) (str "none/" n))
          )
        )
      )
    )
    (blank)
    (lazy suffixed-unit Atom
      (pattern
        (~ (+ (chars "a-z")))
      )
    )
  )
)

(fn-expr + unread _ (over: Char)
  (param tree AtomHelper)
  (~ (: v \unread-number)
    v
  )
  (where
    (lazy unread-number U32
      (pattern
        (~ (seq (+ (chars "0-9")) (: u \unread-unit))
          (call (. tree note) "unread")
        )
      )
    )
    (blank)
    (lazy unread-unit Atom
      (pattern
        (~ (+ (chars "a-z")))
      )
    )
  )
)

(type + AtomHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + pool AtomPool)
  (prop + texts Size)
  (prop + atoms Size)
  (prop + notes (list String))
  (blank)
  (delegate (of PatternCaptureMaterializer Char Atom) (of PatternCaptureMaterializer Byte Atom))
  (prop _ atom-maker _ (:= (call AtomMaker this)))
  (fn-do + This _
    (param source Arena)
    (= arena source)
    (= pool (call AtomPool))
  )
  (fn-do + note U32
    (param text String)
    (+= notes text)
    (return (as (. notes length) U32))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= texts 1)
    (return (call (. arena copy-span) source start end))
  )
  (fn-expr + mark Size
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)

(type _ AtomMaker (is Tuple)
  (implements (of PatternCaptureMaterializer Char Atom))
  (implements (of PatternCaptureMaterializer Byte Atom))
  (prop _ helper AtomHelper)
  (fn-do + This _
    (param source AtomHelper)
    (= helper source)
  )
  (fn-do + (of materialize-capture Token) Atom
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= (. helper atoms) 1)
    (return (call (. helper pool store) source start end))
  )
)
```

```output
ab 2 def
0 3
ab 0 -
0 4
ab/12
1 6
none/12
2 6
ab/12
3 7
unread
3 8
```

### An action whose pattern is an optional part reads `it` as the text the part matched, or nothing where its operand did not pass.

```brix
(main
  (print
    (match "!"
      (~ (? "!")
        (?? it "none")
      )
      (default
        (inline "no match")
      )
    )
  )
  (print
    (match ""
      (~ (? "!")
        (?? it "none")
      )
      (default
        (inline "no match")
      )
    )
  )
  (print
    (match "abc"
      (~ (? (* (chars "a-z")))
        (str "<" (?? it "none") ">")
      )
      (default
        (inline "no match")
      )
    )
  )
  (print
    (match ""
      (~ (? (* (chars "a-z")))
        (str "<" (?? it "none") ">")
      )
      (default
        (inline "no match")
      )
    )
  )
  (print
    (match "abc"
      (~ (? (+ (chars "a-z")))
        (str "<" (?? it "none") ">")
      )
      (default
        (inline "no match")
      )
    )
  )
  (print
    (match ""
      (~ (? (+ (chars "a-z")))
        (str "<" (?? it "none") ">")
      )
      (default
        (inline "no match")
      )
    )
  )
  (print
    (match "!"
      (~ (: flag \bang)
        (if-expr flag "yes" "no")
      )
      (default
        (inline "no match")
      )
      (lazy bang Bool
        (pattern
          (~ (? "!")
            (not (not it))
          )
        )
      )
    )
  )
  (print
    (match ""
      (~ (: flag \bang)
        (if-expr flag "yes" "no")
      )
      (default
        (inline "no match")
      )
      (lazy bang Bool
        (pattern
          (~ (? "!")
            (not (not it))
          )
        )
      )
    )
  )
)
```

```output
!
none
<abc>
<>
<abc>
<none>
yes
no
```

### A helper makes the text an optional part gives as `it` where the part passes, but not the text of one literal, which it makes only where an action reads a name that captured it.

```brix
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call CountingHelper (call Arena)))
  (print
    (match "ab"
      (with helper)
      (~ (: v (call \letters helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
  (print
    (match ""
      (with helper)
      (~ (: v (call \letters helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
  (print
    (match "!"
      (with helper)
      (~ (: v (call \bang helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
  (print
    (match ""
      (with helper)
      (~ (: v (call \bang helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
  (print
    (match "12!"
      (with helper)
      (~ (: v (call \unread helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
  (print
    (match "12!"
      (with helper)
      (~ (: v (call \read helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
  (print
    (match "12"
      (with helper)
      (~ (: v (call \read helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper made))
)

(fn-expr + letters _ (over: Char)
  (param tree CountingHelper)
  (~ (: v \letters-inner)
    v
  )
  (where
    (lazy letters-inner U32
      (pattern
        (~ (? (+ (chars "a-z")))
          (call (. tree note) (?? it "none"))
        )
      )
    )
  )
)

(fn-expr + bang _ (over: Char)
  (param tree CountingHelper)
  (~ (: v \bang-inner)
    v
  )
  (where
    (lazy bang-inner U32
      (pattern
        (~ (? "!")
          (call (. tree note) (?? it "none"))
        )
      )
    )
  )
)

(fn-expr + unread _ (over: Char)
  (param tree CountingHelper)
  (~ (: v \unread-inner)
    v
  )
  (where
    (lazy unread-inner U32
      (pattern
        (~ (seq (+ (chars "0-9")) (: u (? "!")))
          (call (. tree note) "unread")
        )
      )
    )
  )
)

(fn-expr + read _ (over: Char)
  (param tree CountingHelper)
  (~ (: v \read-inner)
    v
  )
  (where
    (lazy read-inner U32
      (pattern
        (~ (seq (+ (chars "0-9")) (: u (? "!")))
          (call (. tree note) (?? u "none"))
        )
      )
    )
  )
)

(type + CountingHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + made Size)
  (prop + notes (list String))
  (fn-do + This _
    (param source Arena)
    (= arena source)
  )
  (fn-do + note U32
    (param text String)
    (+= notes text)
    (return (as (. notes length) U32))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= made 1)
    (return (call (. source slice-string) (..< start end)))
  )
  (fn-expr + mark Size
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)
```

```output
ab
1
none
1
!
1
none
1
unread
1
!
2
none
2
```

### A rule that states a type only the helper makes of what its arm gives is typed for the helper a pattern calls it under, and not as it is without one.

```brix
(import /core/atom-pool)
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call NamingHelper (call Arena)))
  (print
    (match "12ab"
      (with helper)
      (~ (: v (call \labelled helper))
        ([] (. helper notes) (- v 1))
      )
      (default
        (inline "no match")
      )
    )
  )
  (print (. helper named))
)

(fn-expr + labelled _ (over: Char)
  (param tree NamingHelper)
  (~ (: v \labelled-number)
    v
  )
  (where
    (lazy labelled-number U32
      (pattern
        (~ (seq (+ (chars "0-9")) (+ (chars "a-z")) (: name (call \label true)))
          (call (. tree note) (. name text))
        )
      )
    )
  )
)

(fn-expr + label Atom (over: Char)
  (param flag Bool)
  (~ ""
    (if-expr flag "yes" "no")
  )
)

(type + NamingHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternValueMaterializer String Atom))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + pool AtomPool)
  (prop + named Size)
  (prop + notes (list String))
  (fn-do + This _
    (param source Arena)
    (= arena source)
    (= pool (call AtomPool))
  )
  (fn-do + note U32
    (param text String)
    (+= notes text)
    (return (as (. notes length) U32))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + materialize-value Atom
    (param input String)
    (+= named 1)
    (return (call (. pool store) input))
  )
  (fn-expr + mark Size
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)
```

```output
yes
1
```

### A name the arms of a rule capture holds in each arm the type its part there gives it.

```brix
(main
  (print (call describe "#123"))
  (print (call describe "@abc"))
  (print (call describe "?"))
  (print (call describe "?x"))
  (print (call describe "%"))
  (print
    (match "@abc"
      (~ (: v \entry)
        v
      )
      (default
        (inline "no match")
      )
    )
  )
  (print
    (match "?"
      (~ (: v \entry)
        v
      )
      (default
        (inline "no match")
      )
    )
  )
)

(fn-do + describe String
  (param text String)
  (return
    (match text
      (~ (: v \local-entry)
        v
      )
      (default
        (inline "no match")
      )
      (blank)
      (lazy local-entry String
        (pattern
          (~ (seq "#" (: value \local-number))
            (str "number " (+ value 1))
          )
          (~ (seq "@" (: value (+ (chars "a-z"))))
            (str "word " value)
          )
          (~ (seq "?" (: value (? (+ (chars "a-z")))))
            (str "maybe " (?? value "nothing"))
          )
        )
      )
      (blank)
      (lazy local-number I32
        (pattern
          (~ (: digits (+ (chars "0-9")))
            (as (. digits length) I32)
          )
        )
      )
    )
  )
)

(fn-expr + entry String (over: Char)
  (~ (seq "#" (: value \number))
    (str "number " (+ value 1))
  )
  (~ (seq "@" (: value (+ (chars "a-z"))))
    (str "word " value)
  )
  (~ (seq "?" (: value (? (+ (chars "a-z")))))
    (str "maybe " (?? value "nothing"))
  )
  (where
    (lazy number I32
      (pattern
        (~ (: digits (+ (chars "0-9")))
          (as (. digits length) I32)
        )
      )
    )
  )
)
```

```output
number 4
word abc
maybe nothing
maybe x
no match
word abc
maybe nothing
```

### A rule or a match whose arms give characters, some read from the text their names captured, gives characters, and its helper makes the names the actions read but not the text they give again.

```brix
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call TextHelper (call Arena)))
  (for (in text (list "\\n" "\\0" "\\q" "z"))
    (print (call decode helper text))
    (print (call decode-ruled helper text))
    (print (call decode-plainly helper text))
    (print (str (. helper made) " " (. helper valued)))
  )
)

(fn-do + decode String
  (param helper TextHelper)
  (param text String)
  (return
    (match text
      (with helper)
      (~ (: v \escape)
        (str "got " (as v U32))
      )
      (default
        (inline "no match")
      )
      (blank)
      (lazy escape Char
        (pattern
          (~ "\\0"
            (as 0 Char)
          )
          (~ (seq "\\" (: c \single))
            ([] (call unescape c) 0)
          )
          (~ (seq "\\" (: c \other))
            ([] c 0)
          )
        )
      )
      (blank)
      (lazy single
        (pattern
          (~ (chars "nt"))
        )
      )
      (blank)
      (lazy other
        (pattern
          (~ (seq (! \single) .))
        )
      )
    )
  )
)

(fn-do + decode-ruled String
  (param helper TextHelper)
  (param text String)
  (return
    (match text
      (with helper)
      (~ (: v \ruled-escape)
        (str "got " (as v U32))
      )
      (default
        (inline "no match")
      )
    )
  )
)

(fn-do + decode-plainly String
  (param helper TextHelper)
  (param text String)
  (let found
    (match text
      (with helper)
      (~ "\\0"
        (as 0 Char)
      )
      (~ (seq "\\" (: c \plain-single))
        ([] (call unescape c) 0)
      )
      (~ (seq "\\" (: c \plain-other))
        ([] c 0)
      )
      (blank)
      (lazy plain-single
        (pattern
          (~ (chars "nt"))
        )
      )
      (blank)
      (lazy plain-other
        (pattern
          (~ (seq (! \plain-single) .))
        )
      )
    )
  )
  (return (if-expr found (str "got " (as found U32)) "no match"))
)

(fn-expr + ruled-escape Char (over: Char)
  (~ "\\0"
    (as 0 Char)
  )
  (~ (seq "\\" (: c \ruled-single))
    ([] (call unescape c) 0)
  )
  (~ (seq "\\" (: c \ruled-other))
    ([] c 0)
  )
  (where
    (lazy ruled-single
      (pattern
        (~ (chars "nt"))
      )
    )
    (blank)
    (lazy ruled-other
      (pattern
        (~ (seq (! \ruled-single) .))
      )
    )
  )
)

(fn-expr + unescape _
  (param c _)
  (match c
    (case= "t"
      "\t"
    )
    (case= "n"
      "\n"
    )
    (default
      ""
    )
  )
)

(type + TextHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + made Size)
  (prop + valued Size)
  (fn-do + This _
    (param source Arena)
    (= arena source)
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= made 1)
    (return (call (. source slice-string) (..< start end)))
  )
  (fn-do + materialize-value String
    (param input String)
    (+= valued 1)
    (return input)
  )
  (fn-expr + mark Size
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)
```

```output
got 10
got 10
got 10
3 2
got 0
got 0
got 0
3 4
got 113
got 113
got 113
6 6
no match
no match
no match
6 8
```

### An optional value an action gives under a helper, where the arms give an optional too, is made again by the helper only where it holds a value.

```brix
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call TextHelper (call Arena)))
  (for (in text (list "ab" "7" "?"))
    (print (call called helper text))
    (print (call read helper text))
    (print (call local helper text))
    (print (call joined helper text))
    (print (str (. helper made) " " (. helper valued)))
  )
)

(fn-do + called Int
  (param helper TextHelper)
  (param text String)
  (let found
    (match text
      (with helper)
      (~ (seq (+ (chars "a-z")) (call \maybe-name true))
        1
      )
      (~ (seq (+ (chars "0-9")) (call \maybe-name false))
        3
      )
    )
  )
  (return (?? found 0))
)

(fn-do + read Int
  (param helper TextHelper)
  (param text String)
  (let found
    (match text
      (with helper)
      (~ (seq (+ (chars "a-z")) (: v (call \maybe-name true)))
        (if-expr (== v null) 1 2)
      )
      (~ (seq (+ (chars "0-9")) (: v (call \maybe-name false)))
        (if-expr (== v null) 3 4)
      )
    )
  )
  (return (?? found 0))
)

(fn-do + local Int
  (param helper TextHelper)
  (param text String)
  (let found
    (match text
      (with helper)
      (~ (: v \stated)
        (if-expr (== v null) 1 2)
      )
      (blank)
      (lazy stated (? String)
        (pattern
          (~ (+ (chars "a-z"))
            (call pick true)
          )
          (~ (+ (chars "0-9"))
            (call pick false)
          )
        )
      )
    )
  )
  (return (?? found 0))
)

(fn-do + joined Int
  (param helper TextHelper)
  (param text String)
  (let found
    (match text
      (with helper)
      (~ (: v \unstated)
        (if-expr (== v null) 1 2)
      )
      (blank)
      (lazy unstated
        (pattern
          (~ (+ (chars "a-z"))
            (call pick true)
          )
          (~ (+ (chars "0-9"))
            (call pick false)
          )
        )
      )
    )
  )
  (return (?? found 0))
)

(fn-expr + maybe-name (? String) (over: Char)
  (param named Bool)
  (~ ""
    (call pick named)
  )
)

(fn-do + pick (? String)
  (param named Bool)
  (return (only-if "name" (if named)))
)

(type + TextHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + made Size)
  (prop + valued Size)
  (fn-do + This _
    (param source Arena)
    (= arena source)
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= made 1)
    (return (call (. source slice-string) (..< start end)))
  )
  (fn-do + materialize-value String
    (param input String)
    (+= valued 1)
    (return input)
  )
  (fn-expr + mark Size
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)
```

```output
1
2
2
2
0 7
3
3
1
1
0 7
0
0
0
0
0 7
```

### A name that holds what an optional call of a rule gave is read as what it holds where a test narrowed it.

```brix
(main
  (print (call decode "ab"))
  (print (call decode "7"))
  (print (call decode "7b"))
  (print (call decode "?"))
)

(fn-do + decode String
  (param text String)
  (let found
    (match text
      (~ (seq (chars "a-z") (: v (? \maybe)))
        (if-expr (== v null) "null" (str (+ v 1)))
      )
      (~ (seq (chars "0-9") (: v (? \maybe)))
        (if-expr (== v null) "null" (str (+ v 2)))
      )
      (blank)
      (lazy maybe U32
        (pattern
          (~ (| "b" "c")
            5
          )
        )
      )
    )
  )
  (return (?? found "none"))
)
```

```output
6
null
7
none
```

### A rule that states an optional of a type only the helper makes of what its arm gives has it made only where the arm gives a value.

```brix
(import /core/atom-pool)
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call NamingHelper (call Arena)))
  (for (in text (list "ab" "7" "?"))
    (print (call decode helper text))
    (print (. helper named))
  )
)

(fn-do + decode String
  (param helper NamingHelper)
  (param text String)
  (let found
    (match text
      (with helper)
      (~ (seq (+ (chars "a-z")) (: v (call \maybe-name true)))
        (if-expr (== v null) "null" (. v text))
      )
      (~ (seq (+ (chars "0-9")) (: v (call \maybe-name false)))
        (if-expr (== v null) "null" (. v text))
      )
    )
  )
  (return (?? found "none"))
)

(fn-expr + maybe-name (? Atom) (over: Char)
  (param named Bool)
  (~ ""
    (only-if "name" (if named))
  )
)

(type + NamingHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot U32))
  (implements (of PatternValueMaterializer String Atom))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + pool AtomPool)
  (prop + named Size)
  (prop + notes (list String))
  (fn-do + This _
    (param source Arena)
    (= arena source)
    (= pool (call AtomPool))
  )
  (fn-do + note U32
    (param text String)
    (+= notes text)
    (return (as (. notes length) U32))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + materialize-value Atom
    (param input String)
    (+= named 1)
    (return (call (. pool store) input))
  )
  (fn-expr + mark Size
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)
```

```output
name
1
null
1
none
1
```

### Alternatives that each call a rule that gives a value of one type give the name that captures them the value of the one that passed: alone, in a repeat and in an optional part.

```brix
(main
  (for (in text (list "'a" "'b" "'c" "'d" "x"))
    (print (call one text))
  )
  (for (in text (list "ab" "cab" "" "abd"))
    (print (call many text))
  )
  (for (in text (list "a." "." "c." "d."))
    (print (call maybe text))
  )
)

(fn-do + one String
  (param text String)
  (return
    (match text
      (~ (seq "'" (: tt (| \first \second \third)))
        (str "got " (+ tt 1))
      )
      (default
        (inline "no match")
      )
      (blank)
      (lazy first U32
        (pattern
          (~ "a"
            10
          )
        )
      )
      (blank)
      (lazy second U32
        (pattern
          (~ "b"
            20
          )
        )
      )
      (blank)
      (lazy third U32
        (pattern
          (~ "c"
            30
          )
        )
      )
    )
  )
)

(fn-do + many String
  (param text String)
  (return
    (match text
      (~ (: items (* (| \m-first \m-second \m-third)))
        (str "got " (. items length) " " items)
      )
      (default
        (inline "no match")
      )
      (blank)
      (lazy m-first U32
        (pattern
          (~ "a"
            10
          )
        )
      )
      (blank)
      (lazy m-second U32
        (pattern
          (~ "b"
            20
          )
        )
      )
      (blank)
      (lazy m-third U32
        (pattern
          (~ "c"
            30
          )
        )
      )
    )
  )
)

(fn-do + maybe String
  (param text String)
  (return
    (match text
      (~ (seq (: tt (? (| \o-first \o-second))) ".")
        (str "got " (?? tt 0))
      )
      (default
        (inline "no match")
      )
      (blank)
      (lazy o-first U32
        (pattern
          (~ "a"
            10
          )
        )
      )
      (blank)
      (lazy o-second U32
        (pattern
          (~ "c"
            30
          )
        )
      )
    )
  )
)
```

```output
got 11
got 21
got 31
no match
no match
got 2 10, 20
got 3 30, 10, 20
got 0 
no match
got 10
got 0
got 30
no match
```

### A repeat or a count of a call of a rule that gives an optional value gives a list of what each pass gave, nothing where a pass gave none.

```brix
(main
  (for (in text (list "a1b" "11" "" "ab" "a-"))
    (print (call lines text))
    (print (call starred text))
    (print (call counted text))
    (print (call texts text))
  )
)

(fn-do + lines String
  (param text String)
  (return
    (match text
      (~ (: items (+ \mline))
        (str "got " (. items length) ": " (call show items))
      )
      (default
        (inline "no match")
      )
    )
  )
)

(fn-do + starred String
  (param text String)
  (return
    (match text
      (~ (seq (: items (* \s-line)) (: rest (* .)))
        (str "got " (. items length) ": " (call show items) " then " rest)
      )
      (default
        (inline "no match")
      )
      (blank)
      (lazy s-line (? U32)
        (pattern
          (~ (chars "0-9")
            null
          )
          (~ "a"
            10
          )
        )
      )
    )
  )
)

(fn-do + counted String
  (param text String)
  (return
    (match text
      (~ (seq (: items ({} \c-line 2)) (: rest (* .)))
        (str "got " (. items length) ": " (call show items) " then " rest)
      )
      (default
        (inline "no match")
      )
      (blank)
      (lazy c-line (? U32)
        (pattern
          (~ (chars "0-9")
            null
          )
          (~ "a"
            10
          )
        )
      )
    )
  )
)

(fn-do + texts String
  (param text String)
  (return
    (match text
      (~ (: items (+ \t-line))
        (str "got " (. items length) ": " (call show items))
      )
      (default
        (inline "no match")
      )
      (blank)
      (lazy t-line (? String)
        (pattern
          (~ (chars "0-9")
            null
          )
          (~ "a"
            "ten"
          )
          (~ "b"
            "twenty"
          )
        )
      )
    )
  )
)

(fn-expr + mline (? U32) (over: Char)
  (~ (chars "0-9")
    null
  )
  (~ "a"
    10
  )
  (~ "b"
    20
  )
)

(fn-do + show String
  (param items _)
  (let parts (list String))
  (for (in item items)
    (+= parts (if-expr (== item null) "none" (str item)))
  )
  (return (call (. ", " join) parts))
)
```

```output
got 3: 10, none, 20
got 2: 10, none then b
got 2: 10, none then b
got 3: ten, none, twenty
got 2: none, none
got 2: none, none then 
got 2: none, none then 
got 2: none, none
no match
got 0:  then 
no match
no match
got 2: 10, 20
got 1: 10 then b
no match
got 2: ten, twenty
no match
got 1: 10 then -
no match
no match
```

### A list an action gives under a helper, where its rule states a list of a type the helper makes of the action's items, is made again item by item.

```brix
(import /core/atom-pool)
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call AtomPatternHelper (call Arena)))
  (for (in text (list "This" "a.b" "?"))
    (print
      (match text
        (with helper)
        (~ (: parsed (call \grammar helper))
          ([] (. helper notes) (- parsed 1))
        )
        (default
          (inline "no match")
        )
      )
    )
    (print (str "counts " (. helper string-capture-count) " " (. helper string-value-count) " " (. helper atom-capture-count) " " (. helper atom-value-count)))
  )
)

(fn-expr + grammar _ (over: Char)
  (param tree AtomPatternHelper)
  (~ (: names \left)
    (call (. tree note) (call show names))
  )
  (where
    (lazy left (of ImmutableArray Atom)
      (pattern
        (~ (: value "This")
          (call single value)
        )
        (~ (seq (: first (chars "a-z")) "." (: second (chars "a-z")))
          (call pair first second)
        )
      )
    )
  )
)

(fn-do + (of single Item) (of ImmutableArray Item)
  (param item Item)
  (let items (call (of Array Item) 1))
  (= ([] items 0) item)
  (return (. items immutable))
)

(fn-do + (of pair Item) (of ImmutableArray Item)
  (param first Item)
  (param second Item)
  (let items (call (of Array Item) 2))
  (= ([] items 0) first)
  (= ([] items 1) second)
  (return (. items immutable))
)

(fn-do + show String
  (param names (of ImmutableArray Atom))
  (let parts (list String))
  (for (in name names)
    (+= parts (. name text))
  )
  (return (call (. ", " join) parts))
)

(type + NodeId (is U32))

(type + AtomPatternHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot NodeId))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + pool AtomPool)
  (prop + string-capture-count Size)
  (prop + string-value-count Size)
  (prop + atom-capture-count Size)
  (prop + atom-value-count Size)
  (prop + mark-count Size)
  (prop + notes (list String))
  (blank)
  (delegate (of PatternCaptureMaterializer Char Atom) (of PatternCaptureMaterializer Byte Atom))
  (delegate (of PatternValueMaterializer String Atom))
  (prop _ atom-materializer _ (:= (call AtomAtomPatternMaterializer this)))
  (fn-do + note NodeId
    (param text String)
    (+= notes text)
    (return (as (. notes length) NodeId))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + This _
    (param source Arena)
    (= arena source)
    (= pool (call AtomPool))
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= string-capture-count 1)
    (return (call (. arena copy-span) source start end))
  )
  (fn-do + materialize-value String
    (param input String)
    (+= string-value-count 1)
    (return (call (. arena copy-string) input))
  )
  (fn-expr + mark Size
    (before
      (do
        (+= mark-count 1)
      )
    )
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)

(type _ AtomAtomPatternMaterializer (is Tuple)
  (implements (of PatternCaptureMaterializer Char Atom))
  (implements (of PatternCaptureMaterializer Byte Atom))
  (implements (of PatternValueMaterializer String Atom))
  (prop _ helper AtomPatternHelper)
  (fn-do + This _
    (param source AtomPatternHelper)
    (= helper source)
  )
  (fn-do + (of materialize-capture Token) Atom
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= (. helper atom-capture-count) 1)
    (return (call (. helper pool store) source start end))
  )
  (fn-do + materialize-value Atom
    (param input String)
    (+= (. helper atom-value-count) 1)
    (return (call (. helper pool store) input))
  )
)
```

```output
This
counts 1 1 0 1
a, b
counts 3 2 0 3
no match
counts 3 3 0 3
```

### A name the arms of a rule capture may hold a list a rule gave in one arm and an optional one in another.

```brix
(main
  (for (in text (list "#ab.x" "#.x" "#ab." "@ab.x" "@.x" "@ab." "?"))
    (print (call describe text))
    (print
      (match text
        (~ (: v \entry)
          v
        )
        (default
          (inline "no match")
        )
      )
    )
  )
)

(fn-do + describe String
  (param text String)
  (return
    (match text
      (~ (: v \local-entry)
        v
      )
      (default
        (inline "no match")
      )
      (blank)
      (lazy local-entry String
        (pattern
          (~ (seq "#" (: rules (? \local-letters)) "." (: body \local-word))
            (str "first " (if-expr rules (. rules length) 0) " " body)
          )
          (~ (seq "@" (: rules \local-letters) "." (: body (? \local-word)))
            (str "second " (. rules length) " " (?? body "nothing"))
          )
        )
      )
      (blank)
      (lazy local-letters (of ImmutableArray String)
        (pattern
          (~ (+ (: letter (chars "a-z")))
            letter
          )
        )
      )
      (blank)
      (lazy local-word String
        (pattern
          (~ (+ (chars "a-z")))
        )
      )
    )
  )
)

(fn-expr + entry String (over: Char)
  (~ (seq "#" (: rules (? \letters)) "." (: body \word))
    (str "first " (if-expr rules (. rules length) 0) " " body)
  )
  (~ (seq "@" (: rules \letters) "." (: body (? \word)))
    (str "second " (. rules length) " " (?? body "nothing"))
  )
  (where
    (lazy letters (of ImmutableArray String)
      (pattern
        (~ (+ (: found (chars "a-z")))
          found
        )
      )
    )
    (blank)
    (lazy word String
      (pattern
        (~ (+ (chars "a-z")))
      )
    )
  )
)
```

```output
first 2 x
first 2 x
first 0 x
first 0 x
no match
no match
second 2 x
second 2 x
no match
no match
second 2 nothing
second 2 nothing
no match
no match
```

### A rule given a pattern that gives text, under a helper that makes text again, has the list it accumulates made again item by item where its action reads it, the list it gives made again, and so the list where the rule calling it reads it.

```brix
(import /core/atom-pool)
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call AtomPatternHelper (call Arena)))
  (for (in text (list "ab.c.d" "x" "?"))
    (print
      (match text
        (with helper)
        (~ (: parsed (call \grammar helper))
          ([] (. helper notes) (- parsed 1))
        )
        (default
          (inline "no match")
        )
      )
    )
    (print (str "counts " (. helper string-capture-count) " " (. helper string-value-count) " " (. helper atom-capture-count) " " (. helper atom-value-count)))
  )
)

(fn-expr + grammar _ (over: Char)
  (param tree AtomPatternHelper)
  (~ (: parts (call \delimited "." (~ \raw-name)))
    (call (. tree note) (call (. "." join) parts))
  )
  (where
    (lazy raw-name
      (pattern
        (~ (seq (chars "a-z") (* (chars "-a-z0-9"))))
      )
    )
  )
)

(type + NodeId (is U32))

(type + AtomPatternHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot NodeId))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + pool AtomPool)
  (prop + string-capture-count Size)
  (prop + string-value-count Size)
  (prop + atom-capture-count Size)
  (prop + atom-value-count Size)
  (prop + mark-count Size)
  (prop + notes (list String))
  (blank)
  (delegate (of PatternCaptureMaterializer Char Atom) (of PatternCaptureMaterializer Byte Atom))
  (delegate (of PatternValueMaterializer String Atom))
  (prop _ atom-materializer _ (:= (call AtomAtomPatternMaterializer this)))
  (fn-do + note NodeId
    (param text String)
    (+= notes text)
    (return (as (. notes length) NodeId))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + This _
    (param source Arena)
    (= arena source)
    (= pool (call AtomPool))
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= string-capture-count 1)
    (return (call (. arena copy-span) source start end))
  )
  (fn-do + materialize-value String
    (param input String)
    (+= string-value-count 1)
    (return (call (. arena copy-string) input))
  )
  (fn-expr + mark Size
    (before
      (do
        (+= mark-count 1)
      )
    )
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)

(type _ AtomAtomPatternMaterializer (is Tuple)
  (implements (of PatternCaptureMaterializer Char Atom))
  (implements (of PatternCaptureMaterializer Byte Atom))
  (implements (of PatternValueMaterializer String Atom))
  (prop _ helper AtomPatternHelper)
  (fn-do + This _
    (param source AtomPatternHelper)
    (= helper source)
  )
  (fn-do + (of materialize-capture Token) Atom
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= (. helper atom-capture-count) 1)
    (return (call (. helper pool store) source start end))
  )
  (fn-do + materialize-value Atom
    (param input String)
    (+= (. helper atom-value-count) 1)
    (return (call (. helper pool store) input))
  )
)
```

```output
ab.c.d
counts 3 10 0 0
x
counts 4 14 0 0
no match
counts 4 15 0 0
```

### A pattern given to a rule that is a call of a local rule alone, where the local rule states a type the helper makes of its text, gives what the helper makes.

```brix
(import /core/atom-pool)
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call AtomPatternHelper (call Arena)))
  (for (in text (list "ab.c.d" "x" "?"))
    (print
      (match text
        (with helper)
        (~ (: parsed (call \grammar helper))
          ([] (. helper notes) (- parsed 1))
        )
        (default
          (inline "no match")
        )
      )
    )
    (print (str "counts " (. helper string-capture-count) " " (. helper string-value-count) " " (. helper atom-capture-count) " " (. helper atom-value-count)))
  )
)

(fn-expr + grammar _ (over: Char)
  (param tree AtomPatternHelper)
  (~ (: names \left)
    (call (. tree note) (call show names))
  )
  (where
    (lazy left (of ImmutableArray Atom)
      (pattern
        (~ (: names (call \delimited "." (~ \name)))
          names
        )
      )
    )
    (blank)
    (lazy name Atom
      (pattern
        (~ (seq (chars "a-z") (* (chars "-a-z0-9"))))
      )
    )
  )
)

(fn-do + show String
  (param names (of ImmutableArray Atom))
  (let parts (list String))
  (for (in name names)
    (+= parts (. name text))
  )
  (return (call (. "+" join) parts))
)

(type + NodeId (is U32))

(type + AtomPatternHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot NodeId))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + pool AtomPool)
  (prop + string-capture-count Size)
  (prop + string-value-count Size)
  (prop + atom-capture-count Size)
  (prop + atom-value-count Size)
  (prop + mark-count Size)
  (prop + notes (list String))
  (blank)
  (delegate (of PatternCaptureMaterializer Char Atom) (of PatternCaptureMaterializer Byte Atom))
  (delegate (of PatternValueMaterializer String Atom))
  (prop _ atom-materializer _ (:= (call AtomAtomPatternMaterializer this)))
  (fn-do + note NodeId
    (param text String)
    (+= notes text)
    (return (as (. notes length) NodeId))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + This _
    (param source Arena)
    (= arena source)
    (= pool (call AtomPool))
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= string-capture-count 1)
    (return (call (. arena copy-span) source start end))
  )
  (fn-do + materialize-value String
    (param input String)
    (+= string-value-count 1)
    (return (call (. arena copy-string) input))
  )
  (fn-expr + mark Size
    (before
      (do
        (+= mark-count 1)
      )
    )
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)

(type _ AtomAtomPatternMaterializer (is Tuple)
  (implements (of PatternCaptureMaterializer Char Atom))
  (implements (of PatternCaptureMaterializer Byte Atom))
  (implements (of PatternValueMaterializer String Atom))
  (prop _ helper AtomPatternHelper)
  (fn-do + This _
    (param source AtomPatternHelper)
    (= helper source)
  )
  (fn-do + (of materialize-capture Token) Atom
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= (. helper atom-capture-count) 1)
    (return (call (. helper pool store) source start end))
  )
  (fn-do + materialize-value Atom
    (param input String)
    (+= (. helper atom-value-count) 1)
    (return (call (. helper pool store) input))
  )
)
```

```output
ab+c+d
counts 0 1 3 0
x
counts 0 2 4 0
no match
counts 0 3 4 0
```

### An arm that is a call alone of a rule that states a type the helper makes of its text gives what the helper makes, beside an arm that makes the type itself.

```brix
(import /core/atom-pool)
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call AtomPatternHelper (call Arena)))
  (for (in text (list "ab" "x" "42" "?"))
    (print
      (match text
        (with helper)
        (~ (: parsed (call \grammar helper))
          ([] (. helper notes) (- parsed 1))
        )
        (default
          (inline "no match")
        )
      )
    )
    (print (str "counts " (. helper string-capture-count) " " (. helper string-value-count) " " (. helper atom-capture-count) " " (. helper atom-value-count)))
  )
)

(fn-expr + grammar NodeId (over: Char)
  (param tree AtomPatternHelper)
  (~ (: n \item)
    (call (. tree note) (. n text))
  )
  (where
    (lazy item Atom
      (pattern
        (~ \name)
        (~ (: v (+ (chars "0-9")))
          (call (. tree pool store) v)
        )
      )
    )
    (blank)
    (lazy name Atom
      (pattern
        (~ (+ (chars "a-z")))
      )
    )
  )
)

(type + NodeId (is U32))

(type + AtomPatternHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot NodeId))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + pool AtomPool)
  (prop + string-capture-count Size)
  (prop + string-value-count Size)
  (prop + atom-capture-count Size)
  (prop + atom-value-count Size)
  (prop + mark-count Size)
  (prop + notes (list String))
  (blank)
  (delegate (of PatternCaptureMaterializer Char Atom) (of PatternCaptureMaterializer Byte Atom))
  (delegate (of PatternValueMaterializer String Atom))
  (prop _ atom-materializer _ (:= (call AtomAtomPatternMaterializer this)))
  (fn-do + note NodeId
    (param text String)
    (+= notes text)
    (return (as (. notes length) NodeId))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + This _
    (param source Arena)
    (= arena source)
    (= pool (call AtomPool))
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= string-capture-count 1)
    (return (call (. arena copy-span) source start end))
  )
  (fn-do + materialize-value String
    (param input String)
    (+= string-value-count 1)
    (return (call (. arena copy-string) input))
  )
  (fn-expr + mark Size
    (before
      (do
        (+= mark-count 1)
      )
    )
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)

(type _ AtomAtomPatternMaterializer (is Tuple)
  (implements (of PatternCaptureMaterializer Char Atom))
  (implements (of PatternCaptureMaterializer Byte Atom))
  (implements (of PatternValueMaterializer String Atom))
  (prop _ helper AtomPatternHelper)
  (fn-do + This _
    (param source AtomPatternHelper)
    (= helper source)
  )
  (fn-do + (of materialize-capture Token) Atom
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= (. helper atom-capture-count) 1)
    (return (call (. helper pool store) source start end))
  )
  (fn-do + materialize-value Atom
    (param input String)
    (+= (. helper atom-value-count) 1)
    (return (call (. helper pool store) input))
  )
)
```

```output
ab
counts 0 1 1 0
x
counts 0 2 2 0
42
counts 1 3 2 0
no match
counts 1 4 2 0
```

### A rule's value of a declared union type is read as it is under a helper whose value materializers take text: a union is made again only by a materializer that takes that union.

```brix
(import /core/atom-pool)
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call AtomPatternHelper (call Arena)))
  (for (in text (list "ab" "42" "ab42x" "?"))
    (print
      (match text
        (with helper)
        (~ (: parsed (call \grammar helper))
          ([] (. helper notes) (- parsed 1))
        )
        (default
          (inline "no match")
        )
      )
    )
    (print (str "counts " (. helper string-capture-count) " " (. helper string-value-count) " " (. helper atom-capture-count) " " (. helper atom-value-count)))
  )
)

(fn-expr + grammar NodeId (over: Char)
  (param tree AtomPatternHelper)
  (~ (: pieces (+ \piece))
    (call (. tree note) (call describe pieces))
  )
  (where
    (lazy piece Piece
      (pattern
        (~ (: digits (+ (chars "0-9")))
          (call (. Piece number) (. digits length))
        )
        (~ (: word (+ (chars "a-z")))
          (call (. Piece word) word)
        )
      )
    )
  )
)

(type + Piece (is Union)
  (variant number (tuple (: count Size)))
  (variant word (tuple (: text String)))
)

(fn-do + describe String
  (param pieces (of ImmutableArray Piece))
  (let shown "")
  (for (in piece pieces)
    (match piece
      (case (call .number count)
        (= shown (str shown "#" count))
      )
      (case (call .word text)
        (= shown (str shown "<" text ">"))
      )
    )
  )
  (return shown)
)

(type + NodeId (is U32))

(type + AtomPatternHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot NodeId))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + pool AtomPool)
  (prop + string-capture-count Size)
  (prop + string-value-count Size)
  (prop + atom-capture-count Size)
  (prop + atom-value-count Size)
  (prop + mark-count Size)
  (prop + notes (list String))
  (blank)
  (delegate (of PatternCaptureMaterializer Char Atom) (of PatternCaptureMaterializer Byte Atom))
  (delegate (of PatternValueMaterializer String Atom))
  (prop _ atom-materializer _ (:= (call AtomAtomPatternMaterializer this)))
  (fn-do + note NodeId
    (param text String)
    (+= notes text)
    (return (as (. notes length) NodeId))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + This _
    (param source Arena)
    (= arena source)
    (= pool (call AtomPool))
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= string-capture-count 1)
    (return (call (. arena copy-span) source start end))
  )
  (fn-do + materialize-value String
    (param input String)
    (+= string-value-count 1)
    (return (call (. arena copy-string) input))
  )
  (fn-expr + mark Size
    (before
      (do
        (+= mark-count 1)
      )
    )
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)

(type _ AtomAtomPatternMaterializer (is Tuple)
  (implements (of PatternCaptureMaterializer Char Atom))
  (implements (of PatternCaptureMaterializer Byte Atom))
  (implements (of PatternValueMaterializer String Atom))
  (prop _ helper AtomPatternHelper)
  (fn-do + This _
    (param source AtomPatternHelper)
    (= helper source)
  )
  (fn-do + (of materialize-capture Token) Atom
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= (. helper atom-capture-count) 1)
    (return (call (. helper pool store) source start end))
  )
  (fn-do + materialize-value Atom
    (param input String)
    (+= (. helper atom-value-count) 1)
    (return (call (. helper pool store) input))
  )
)
```

```output
<ab>
counts 1 1 0 0
#2
counts 1 2 0 0
<ab>#2<x>
counts 3 3 0 0
no match
counts 3 4 0 0
```

### A lazy local declared under an action's value may share its name with a name another arm captures.

```brix
(main
  (for (in text (list "•a" "(b" "?"))
    (print
      (match text
        (~ (seq "•" (: expr (chars "a-z")))
          (str "got " expr)
        )
        (~ (seq "(" (: key (chars "a-z")))
          (scope (str "made " expr)
            (lazy expr (str "<" key ">"))
          )
        )
        (default
          (inline "none")
        )
      )
    )
  )
)
```

```output
got a
made <b>
none
```

### In a rule that states text, an arm with no expression that is a sequence of parts gives the text it matched, whatever its last part gives.

```brix
(main
  (for (in text (list "•12" "12!" "#7" "x" "12"))
    (print
      (match text
        (~ (: parsed \grammar)
          (str "got " parsed)
        )
        (default
          (inline "no match")
        )
      )
    )
  )
)

(fn-expr + grammar String (over: Char)
  (~ (: s \single)
    (str "single <" s ">")
  )
  (~ (: s \several)
    (str "several <" s ">")
  )
  (where
    (lazy single String
      (pattern
        (~ (seq "•" \number))
      )
    )
    (blank)
    (lazy several String
      (pattern
        (~ (seq \number "!"))
        (~ (seq "#" (: n \number)))
        (~ "x")
      )
    )
    (blank)
    (lazy number U32
      (pattern
        (~ (: digits (+ (chars "0-9")))
          (as (. digits length) U32)
        )
      )
    )
  )
)
```

```output
got single <•12>
got several <12!>
got several <#7>
got several <x>
no match
```

### Under a helper, an arm with no expression that is a sequence of parts gives the text it matched, made as the type its rule gives, though its last part calls a rule that gives a value of that type.

```brix
(import /core/atom-pool)
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call AtomPatternHelper (call Arena)))
  (for (in text (list "•ab" "•xab" "?ab" "! xab" "ab"))
    (print
      (match text
        (with helper)
        (~ (: parsed (call \grammar helper))
          ([] (. helper notes) (- parsed 1))
        )
        (default
          (inline "no match")
        )
      )
    )
    (print (str "counts " (. helper string-capture-count) " " (. helper string-value-count) " " (. helper atom-capture-count) " " (. helper atom-value-count)))
  )
)

(fn-expr + grammar NodeId (over: Char)
  (param tree AtomPatternHelper)
  (~ (: s \stated)
    (call (. tree note) (str "stated " (. s text)))
  )
  (~ (: s \unstated)
    (call (. tree note) (str "unstated " (. s text)))
  )
  (~ (: s \texted)
    (call (. tree note) (str "texted " s))
  )
  (where
    (lazy stated Atom
      (pattern
        (~ (seq "•" \name))
      )
    )
    (blank)
    (lazy unstated
      (pattern
        (~ (seq "?" \name))
      )
    )
    (blank)
    (lazy texted String
      (pattern
        (~ (seq "!" // \name))
      )
    )
    (blank)
    (lazy name Atom
      (pattern
        (~ (seq "x" (: rest (+ (chars "a-z"))))
          (call (. tree pool store) rest)
        )
        (~ (+ (chars "a-z")))
      )
    )
  )
)

(type + NodeId (is U32))

(type + AtomPatternHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot NodeId))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + pool AtomPool)
  (prop + string-capture-count Size)
  (prop + string-value-count Size)
  (prop + atom-capture-count Size)
  (prop + atom-value-count Size)
  (prop + mark-count Size)
  (prop + notes (list String))
  (blank)
  (delegate (of PatternCaptureMaterializer Char Atom) (of PatternCaptureMaterializer Byte Atom))
  (delegate (of PatternValueMaterializer String Atom))
  (prop _ atom-materializer _ (:= (call AtomAtomPatternMaterializer this)))
  (fn-do + note NodeId
    (param text String)
    (+= notes text)
    (return (as (. notes length) NodeId))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + This _
    (param source Arena)
    (= arena source)
    (= pool (call AtomPool))
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= string-capture-count 1)
    (return (call (. arena copy-span) source start end))
  )
  (fn-do + materialize-value String
    (param input String)
    (+= string-value-count 1)
    (return (call (. arena copy-string) input))
  )
  (fn-expr + mark Size
    (before
      (do
        (+= mark-count 1)
      )
    )
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)

(type _ AtomAtomPatternMaterializer (is Tuple)
  (implements (of PatternCaptureMaterializer Char Atom))
  (implements (of PatternCaptureMaterializer Byte Atom))
  (implements (of PatternValueMaterializer String Atom))
  (prop _ helper AtomPatternHelper)
  (fn-do + This _
    (param source AtomPatternHelper)
    (= helper source)
  )
  (fn-do + (of materialize-capture Token) Atom
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= (. helper atom-capture-count) 1)
    (return (call (. helper pool store) source start end))
  )
  (fn-do + materialize-value Atom
    (param input String)
    (+= (. helper atom-value-count) 1)
    (return (call (. helper pool store) input))
  )
)
```

```output
stated •ab
counts 0 1 2 0
stated •xab
counts 1 2 3 0
unstated ?ab
counts 1 3 5 0
texted ! xab
counts 3 5 5 0
no match
counts 3 6 5 0
```

### A match's arm with no expression that is a sequence of parts gives the text it matched under a helper that makes the type its last part gives, as a rule's does.

```brix
(import /core/atom-pool)
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call AtomPatternHelper (call Arena)))
  (for (in text (list "•ab" "•xab"))
    (let found
      (match text
        (with helper)
        (~ (seq "•" (call \name helper)))
        (throw (call Panic "none"))
      )
    )
    (print (. found text))
    (print (str "counts " (. helper string-capture-count) " " (. helper string-value-count) " " (. helper atom-capture-count) " " (. helper atom-value-count)))
  )
)

(fn-expr + name Atom (over: Char)
  (param tree AtomPatternHelper)
  (~ (seq "x" (: rest (+ (chars "a-z"))))
    (call (. tree pool store) rest)
  )
  (~ (+ (chars "a-z")))
)

(type + NodeId (is U32))

(type + AtomPatternHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot NodeId))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + pool AtomPool)
  (prop + string-capture-count Size)
  (prop + string-value-count Size)
  (prop + atom-capture-count Size)
  (prop + atom-value-count Size)
  (prop + mark-count Size)
  (prop + notes (list String))
  (blank)
  (delegate (of PatternCaptureMaterializer Char Atom) (of PatternCaptureMaterializer Byte Atom))
  (delegate (of PatternValueMaterializer String Atom))
  (prop _ atom-materializer _ (:= (call AtomAtomPatternMaterializer this)))
  (fn-do + note NodeId
    (param text String)
    (+= notes text)
    (return (as (. notes length) NodeId))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + This _
    (param source Arena)
    (= arena source)
    (= pool (call AtomPool))
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= string-capture-count 1)
    (return (call (. arena copy-span) source start end))
  )
  (fn-do + materialize-value String
    (param input String)
    (+= string-value-count 1)
    (return (call (. arena copy-string) input))
  )
  (fn-expr + mark Size
    (before
      (do
        (+= mark-count 1)
      )
    )
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)

(type _ AtomAtomPatternMaterializer (is Tuple)
  (implements (of PatternCaptureMaterializer Char Atom))
  (implements (of PatternCaptureMaterializer Byte Atom))
  (implements (of PatternValueMaterializer String Atom))
  (prop _ helper AtomPatternHelper)
  (fn-do + This _
    (param source AtomPatternHelper)
    (= helper source)
  )
  (fn-do + (of materialize-capture Token) Atom
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= (. helper atom-capture-count) 1)
    (return (call (. helper pool store) source start end))
  )
  (fn-do + materialize-value Atom
    (param input String)
    (+= (. helper atom-value-count) 1)
    (return (call (. helper pool store) input))
  )
)
```

```output
•ab
counts 0 0 2 0
•xab
counts 1 0 3 0
```

### One name captures the text of a rule in one arm of a rule and what a rule gives in another, under a helper that makes each.

```brix
(import /core/atom-pool)
(import /core/arena)
(import /core/patterns)
(blank)

(main
  (let helper (call AtomPatternHelper (call Arena)))
  (for (in text (list "ab!" "12" "ab" "?"))
    (print
      (match text
        (with helper)
        (~ (: parsed (call \grammar helper))
          ([] (. helper notes) (- parsed 1))
        )
        (default
          (inline "no match")
        )
      )
    )
    (print (str "counts " (. helper string-capture-count) " " (. helper string-value-count) " " (. helper atom-capture-count) " " (. helper atom-value-count)))
  )
)

(fn-expr + grammar NodeId (over: Char)
  (param tree AtomPatternHelper)
  (~ (seq (: p \part) (? "!"))
    (call (. tree note) p)
  )
  (where
    (lazy part String
      (pattern
        (~ (seq (: text \raw) (& "!"))
          (str "raw " text)
        )
        (~ (: text \cooked)
          (str "cooked " text)
        )
      )
    )
    (blank)
    (lazy raw String
      (pattern
        (~ (+ (chars "a-z")))
      )
    )
    (blank)
    (lazy cooked String
      (pattern
        (~ (: chars (+ (chars "0-9")))
          (str "<" chars ">")
        )
        (~ (+ (chars "a-z"))
          "letters"
        )
      )
    )
  )
)

(type + NodeId (is U32))

(type + AtomPatternHelper
  (implements (of PatternCollectorStorage ArenaBacking))
  (implements (of PatternMaterializationRoot NodeId))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternValueMaterializer String String))
  (implements (of PatternRollback Size))
  (prop + arena Arena)
  (prop + pool AtomPool)
  (prop + string-capture-count Size)
  (prop + string-value-count Size)
  (prop + atom-capture-count Size)
  (prop + atom-value-count Size)
  (prop + mark-count Size)
  (prop + notes (list String))
  (blank)
  (delegate (of PatternCaptureMaterializer Char Atom) (of PatternCaptureMaterializer Byte Atom))
  (delegate (of PatternValueMaterializer String Atom))
  (prop _ atom-materializer _ (:= (call AtomAtomPatternMaterializer this)))
  (fn-do + note NodeId
    (param text String)
    (+= notes text)
    (return (as (. notes length) NodeId))
  )
  (fn-expr + pattern-collector-context Arena :get
    arena
  )
  (fn-do + This _
    (param source Arena)
    (= arena source)
    (= pool (call AtomPool))
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= string-capture-count 1)
    (return (call (. arena copy-span) source start end))
  )
  (fn-do + materialize-value String
    (param input String)
    (+= string-value-count 1)
    (return (call (. arena copy-string) input))
  )
  (fn-expr + mark Size
    (before
      (do
        (+= mark-count 1)
      )
    )
    (+ (. arena mark value) 1)
  )
  (fn-do + rewind _
    (param mark Size)
    (call (. arena rewind) (call ArenaMark (- mark 1)))
  )
)

(type _ AtomAtomPatternMaterializer (is Tuple)
  (implements (of PatternCaptureMaterializer Char Atom))
  (implements (of PatternCaptureMaterializer Byte Atom))
  (implements (of PatternValueMaterializer String Atom))
  (prop _ helper AtomPatternHelper)
  (fn-do + This _
    (param source AtomPatternHelper)
    (= helper source)
  )
  (fn-do + (of materialize-capture Token) Atom
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= (. helper atom-capture-count) 1)
    (return (call (. helper pool store) source start end))
  )
  (fn-do + materialize-value Atom
    (param input String)
    (+= (. helper atom-value-count) 1)
    (return (call (. helper pool store) input))
  )
)
```

```output
raw ab
counts 1 3 0 0
cooked <12>
counts 2 8 0 0
cooked letters
counts 2 13 0 0
no match
counts 2 14 0 0
```

### Alternatives of one arm capture a name as text and as what a rule gave.

```brix
(main
  (for (in input (list "alpha!" "42!" "beta42!" "!"))
    (print
      (match input
        (~ (seq (| (: text \changed) (: text (+ (chars "0-9"))) (: text "")) "!")
          (str "<" text ">")
        )
        (~ (seq (: text \changed) (: tail (+ (chars "0-9"))) "!")
          (str text "+" tail)
        )
        (default
          (inline "none")
        )
      )
    )
  )
)

(fn-expr + changed _ (over: Char)
  (~ (+ (chars "a-z"))
    (++ it "?")
  )
)
```

```output
<alpha?>
<42>
beta?+42
<>
```

### An action that may panic runs in a rule that states an error it does not raise.

```brix
(main
  (for (in input (list "12" "9" "x"))
    (print (catch (call parse input)))
  )
)

(fn-expr + parse Int
  (param text String)
  (match text
    (~ \count
      it
    )
    (default
      (inline 0)
    )
  )
)

(fn-expr + count _ (over: Char) (?! SyntaxError)
  (~ (: digits (+ (chars "0-9")))
    (+ (as ([] (. digits codepoints) 0) Int) (. digits length))
  )
)
```

```output
51
58
0
```

### An action that may panic raises an error the rule states.

```brix
(main
  (for (in input (list "12" "9" "x"))
    (print (catch (call parse input)))
  )
)

(fn-expr + parse Int
  (param text String)
  (match text
    (~ \count
      it
    )
    (default
      (inline 0)
    )
  )
)

(fn-expr + count _ (over: Char) (?! SyntaxError)
  (~ (: digits (+ (chars "0-9")))
    (+ (call check ([] (. digits codepoints) 0)) (. digits length))
  )
)

(fn-expr + check Int (?! SyntaxError)
  (param char Char)
  (if-expr (!= char '9') (as char Int) (call reject "Nine"))
)

(fn-expr + reject Int (?! SyntaxError)
  (param message String)
  (throw (call SyntaxError message))
)
```

```output
51
Nine
0
```

### An arm's action gives the arm's value, so a raise as the action is an error that names where to raise instead.

```brix compile-error
(main
  (print (catch (~> "x" \letter)))
)

(fn-expr + letter String (over: Char) (?! SyntaxError)
  (~ (chars "a-z")
    (throw (call SyntaxError "letter"))
  )
)
```

```output
Exception: An arm's action gives the arm's value, so it cannot be a raise; call a function that raises it, or write "(throw error)" as the rule's last arm, which raises where no arm before it matches
<no file:8>
        (throw (call SyntaxError "letter"))
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A rule another pattern calls may raise only where its body is `~` arms: one of a `?!` arm alone is an error that says so.

```brix compile-error
(main
  (print (catch (~> "x" \letter)))
)

(fn-expr + letter String (over: Char) (?! SyntaxError)
  (~ (seq (chars "a-z") \refused)
    "letter"
  )
)

(fn-expr + refused String (over: Char) (?! SyntaxError)
  (throw (call SyntaxError "refused"))
)
```

```output
Exception: "refused" raises, and a rule a pattern calls may raise only where its body is "(~ …)" arms, the last of which may be "(throw …)"; give it a "(~ …)" arm before its "(throw …)" arm with any "({} …)" test in its pattern, or raise from a function an action calls
<no file:7>
      (~ (seq (chars "a-z") \refused)
                            ^^^^^^^^
```

### A pattern given to a rule calls a local rule of a rule that states an error it does not raise.

```brix
(main
  (print (catch (call parse "ab,cd")))
  (print (catch (call parse "ab,?")))
  (print (catch (call parse "ab;")))
)

(fn-expr + parse _
  (param text String)
  (match text
    (~ \names
      (. it length)
    )
    (default
      (inline 0)
    )
  )
)

(fn-expr + names _ (over: Char) (?! SyntaxError)
  (~ (call \delimited "," (~ \word)))
  (where
    (lazy word String
      (pattern
        (~ (+ (chars "a-z"))
          it
        )
      )
    )
  )
)
```

```output
2
0
0
```

### An error raised under a pattern given to a rule passes through the rule to the one that states it.

```brix
(main
  (print (catch (call parse "ab,cd")))
  (print (catch (call parse "ab,(?)")))
  (print (catch (call parse "ab,?")))
  (print (catch (call parse "ab;")))
)

(fn-expr + parse _
  (param text String)
  (match text
    (~ \names
      (. it length)
    )
    (default
      (inline 0)
    )
  )
)

(fn-expr + names _ (over: Char) (?! SyntaxError)
  (~ (call \delimited "," (~ \word)))
  (where
    (lazy word String
      (pattern
        (~ (seq "(" (: inner \word) ")")
          inner
        )
        (~ \leaf)
      )
    )
    (blank)
    (lazy leaf String
      (pattern
        (~ (+ (chars "a-z"))
          it
        )
        (~ "?"
          (call reject "No word")
        )
      )
    )
  )
)

(fn-expr + reject String (?! SyntaxError)
  (param message String)
  (throw (call SyntaxError message))
)
```

```output
2
No word
No word
0
```

### An error raised under a pattern given to a rule, by a local rule that calls no rule of its own, passes through the rule to the one that states it.

```brix
(main
  (print (catch (call parse "ab,cd")))
  (print (catch (call parse "ab,?")))
  (print (catch (call parse "ab;")))
)

(fn-expr + parse _
  (param text String)
  (match text
    (~ \names
      (. it length)
    )
    (default
      (inline 0)
    )
  )
)

(fn-expr + names _ (over: Char) (?! SyntaxError)
  (~ (call \delimited "," (~ \word)))
  (where
    (lazy word String
      (pattern
        (~ (+ (chars "a-z"))
          it
        )
        (~ "?"
          (call reject "No word")
        )
      )
    )
  )
)

(fn-expr + reject String (?! SyntaxError)
  (param message String)
  (throw (call SyntaxError message))
)
```

```output
2
No word
0
```

### An action that may panic binds a name another local rule of its rule captures.

```brix
(main
  (print (call parse "ab(7)"))
  (print (call parse "ab()"))
  (print (call parse "ab"))
)

(type + Part (is Union)
  (variant many (tuple (: args (list String))))
  (variant lone)
)

(fn-expr + parse String
  (param text String)
  (match text
    (~ \call)
    (default
      (inline "none")
    )
  )
)

(fn-expr + call String (over: Char)
  (~ (seq (: head \head) (: tail \tail))
    (str head " " tail)
  )
  (where
    (lazy head String
      (pattern
        (~ (seq (: args (+ (chars "a-z"))) "(")
          args
        )
      )
    )
    (blank)
    (lazy tail U32
      (pattern
        (~ (seq (: part \part) ")")
          (match part
            (case (call .many args)
              (as (. ([] args 0) length) U32)
            )
            (default
              0
            )
          )
        )
      )
    )
    (blank)
    (lazy part Part
      (pattern
        (~ (+ (chars "0-9"))
          (call .many (list it))
        )
        (~ ""
          .lone
        )
      )
    )
  )
)
```

```output
ab 1
ab 0
none
```

### A returned match whose last arm gives null is absent where no arm before it passes.

```brix
(main
  (print (?? (call word "ab") "none"))
  (print (?? (call word "12") "none"))
)

(fn-do + word (? String)
  (param text String)
  (return
    (match text
      (~ (+ (chars "a-z")))
      (default
        (inline null)
      )
    )
  )
)
```

```output
ab
none
```

### A pattern run on an array of characters gives the locations of the lines it spans.

```brix
(main
  (print (call run (. "ab\ncd" codepoints)))
)

(fn-do + run String
  (param chars ([] Char))
  (let found
    (match chars
      (~ (: v \word)
        v
      )
      (throw (call Panic "no match"))
    )
  )
  (return found)
)

(fn-expr + word _ (over: Char)
  (~ (seq "ab" "\n" "cd")
    (str (. location begin line) ":" (. location end line) ":" (. location end column))
  )
)
```

```output
0:1:1
```

### A list pattern of named rules, one testing each item with a closure and one taking the rest, splits a list where they match.

```brix
(main
  (let l (list "x" "y" "--" "z"))
  (let parts (call split l))
  (print (str "parts " (. parts length)))
  (for (in p parts)
    (print p)
  )
)

(fn-expr _ split (list String) (?! Error)
  (param arguments (list String))
  (match arguments
    (~ (seq (: parts (* \part)) (: operands \operands))
      (spread parts)
      (spread operands)
    )
    (throw (call Error "Invalid"))
    (blank)
    (lazy part String
      (pattern
        (~ (: text (\{} (!= it "--")))
          text
        )
      )
    )
    (blank)
    (lazy operands (list String)
      (pattern
        (~ (seq (\{} (== it "--")) (: values (* String)))
          (in each values)
        )
        (~ (! .)
          (call (of List String))
        )
      )
    )
  )
)
```

```output
parts 3
x
y
z
```

### A pipe that reads a spread of a `use` uses the object in its generator, which ends the use where the generator ends.

```brix
(main
  (spread (use (call Letters "abcd")))
  (pipe ~> (seq (: pair .) .)
    pair
  )
  (pipe |> show)
  (print "after")
)

(fn-do + (of show Item) _
  (param items (stream Item))
  (for (in item items)
    (print (str "item " item))
  )
)

(type + Letters
  (implements (of Usable Letters))
  (implements (of CursorIterable Char Size))
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
  (fn-do + begin-using Letters
    (print "begin")
    (return this)
  )
  (fn-do + end-using _
    (print "end")
  )
  (fn-expr + initial-iterator-state Size :get
    0
  )
  (fn-do + next-iterator (tuple Size (? Char))
    (param state Size)
    (if (< state (. text length))
      (return (tuple (+ state 1) ([] text state)))
    )
    (return (tuple state null))
  )
)
```

```output
begin
item a
item c
end
after
```

### A test of an item over characters sees the character as `it` and gives it. A lookahead of a test that reads only what the rule was given is a guard, which tests its condition alone and needs no character, so it passes over empty text.

```brix
(main
  (print (call shape "ab" true))
  (print (call shape "ab" false))
  (print (call shape "" true))
  (print (call first "abc"))
  (print (call first "xbc"))
)

(fn-expr _ shape String
  (param text String)
  (param flag Bool)
  (match text
    (~ (: words (call \grammar text flag))
      words
    )
    (default
      (inline "none")
    )
  )
)

(fn-expr _ grammar String (over: Char)
  (param source String)
  (param flag Bool)
  (~ (seq (& (\{} flag)) (* .) (! .))
    "flagged"
  )
  (~ (seq (* .) (! .))
    "plain"
  )
)

(fn-expr _ first String
  (param text String)
  (match text
    (~ (seq (: head (\{} (== it 'a'))) (: rest (* .)))
      (str head " then " rest)
    )
    (default
      (inline "no a")
    )
  )
)
```

```output
flagged
plain
flagged
a then bc
no a
```

### A guard, a lookahead of a test that reads no item, needs no input of its own: a record that ends in a part that takes nothing is given as soon as its last item is read, a guard passes at the end of the input, a negative guard passes where its condition does not hold, and a lookahead of a test that reads `it` still needs the item.

```brix
(import /core/tokenstream)
(blank)

(main
  (for (in size (~> (call (of ChunkedTokenStream Byte) (call Trickle (call (of Array Byte) (list 0u8 0u8 2u8 7u8 8u8)))) (call \guarded 4)))
    (print (str "record " size))
  )
  (print
    (match (call (of Array Byte) (list 0u8))
      (~ (seq (: size U8) (& (\{} (<= size 4))) (: data ({} Byte size)))
        (str "kept " (. data size))
      )
      (default
        (inline "refused")
      )
    )
  )
  (print
    (match (call (of Array Byte) (list 9u8))
      (~ (seq (: size U8) (! (\{} (<= size 4))))
        "too big"
      )
      (default
        (inline "fits")
      )
    )
  )
  (print
    (match (call (of Array Byte) (list 3u8))
      (~ (seq (: size U8) (& (\{} (== it 5))))
        "next is 5"
      )
      (default
        (inline "no next")
      )
    )
  )
)

(fn-expr + guarded Size (over: Byte)
  (param limit Size)
  (~ (seq (: size U8) (& (\{} (<= size limit))) (: data ({} Byte size)))
    (. data size)
  )
)

(type + Trickle
  (implements ByteReadable)
  (prop + data ([] Byte))
  (prop + cursor Index)
  (fn-do + This _
    (param data _)
  )
  (fn-do + read Size
    (param size Size)
    (param buffer ([] Byte))
    (if (or (>= cursor (. data size)) (== size 0))
      (return 0)
    )
    (print (str "read " ([] data cursor)))
    (= ([] buffer 0) ([] data cursor))
    (+= cursor 1)
    (return 1)
  )
)
```

```output
read 0
record 0
read 0
record 0
read 2
read 7
read 8
record 2
kept 0
too big
no next
```

### A rule's own failure raises its error, not a rule it called that passed: a guard in the called rule passes at the end of the input, so the data that is missing there raises the caller's.

```brix
(main
  (print (catch (call total (call (of Array Byte) (list 1u8)))))
  (print (catch (call total (call (of Array Byte) (list 9u8 1u8)))))
)

(fn-do + total Size (?! Oops)
  (param bytes ([] Byte))
  (let sum Size 0)
  (for (in size (~> bytes (call \record 4)))
    (+= sum size)
  )
  (return sum)
)

(type + Oops (is Error)
  (prop + code Int)
  (fn-do + This _
    (param code _)
    (= message (str "oops " code))
  )
)

(fn-expr + record Size (over: Byte) (?! Oops)
  (param limit Size)
  (~ (seq (: size U8) (call \limited size limit) (: data ({} Byte size)))
    (. data size)
  )
  (throw (call Oops 1002))
)

(fn-expr + limited Bool (over: Byte) (?! Oops)
  (param size Size)
  (param limit Size)
  (~ (& (\{} (<= size limit)))
    true
  )
  (throw (call Oops 1009))
)
```

```output
oops 1002
oops 1009
```

### A rule that states no type and ends in an optional part gives the text it matched.

```brix
(main
  (print (call kind "[abc]"))
  (print (call kind "[abc"))
  (print (call kind "abc"))
  (print (call matched "[abc]"))
  (print (call matched "[abc"))
  (print (call matched "abc"))
)

(fn-expr _ kind String
  (param text String)
  (match text
    (~ (seq \klass (! .))
      "class"
    )
    (default
      (inline "other")
    )
    (blank)
    (lazy klass
      (pattern
        (~ (seq "[" (* (seq (! "]") .)) (? "]")))
      )
    )
  )
)

(fn-expr _ matched String
  (param text String)
  (match text
    (~ (seq (: k \klass) (! .))
      (?? k "none")
    )
    (default
      (inline "other")
    )
    (blank)
    (lazy klass
      (pattern
        (~ (seq "[" (* (seq (! "]") .)) (? "]")))
      )
    )
  )
)
```

```output
class
class
other
[abc]
[abc
other
```

### An arm of a returned match that gives an optional makes the function's value optional, as a value it gives directly does.

```brix
(main
  (print (call direct "ab"))
  (print (call direct "a"))
  (print (call matched "ab"))
  (print (call matched "a"))
  (print (call matched "x"))
)

(fn-expr _ direct (? String)
  (param text String)
  (call pick text)
)

(fn-expr _ matched (? String)
  (param text String)
  (match text
    (~ (seq "a" (* .))
      (call pick text)
    )
    (default
      (inline "other")
    )
  )
)

(fn-expr _ pick (? String)
  (param text String)
  (case? (> (. text length) 1)
    text
  )
  (default
    (inline null)
  )
)
```

```output
ab
unknown
ab
unknown
other
```

### Arms that state no type give their text, also where an arm ends in an optional call of such a rule.

```brix
(main
  (print (call kind ".5"))
  (print (call kind ".5e3"))
  (print (call kind "e3"))
  (print (call kind "x"))
  (print (call whole "1.5"))
  (print (call whole "1.5e3"))
  (print (call whole "1e3"))
  (print (call whole "1"))
)

(fn-expr _ kind String
  (param text String)
  (match text
    (~ (seq (: t \tail) (! .))
      (?? t "none")
    )
    (default
      (inline "other")
    )
    (blank)
    (lazy tail
      (pattern
        (~ (seq \frac (? \expo)))
        (~ \expo)
      )
    )
    (blank)
    (lazy frac
      (pattern
        (~ (seq "." (* (chars "0-9"))))
      )
    )
    (blank)
    (lazy expo
      (pattern
        (~ (seq "e" (+ (chars "0-9"))))
      )
    )
  )
)

(fn-expr _ whole String
  (param text String)
  (match text
    (~ (seq (: n \number) (! .))
      n
    )
    (default
      (inline "other")
    )
    (blank)
    (lazy number String
      (pattern
        (~ (seq (+ (chars "0-9")) (? \tail) (? \unit)))
      )
    )
    (blank)
    (lazy unit
      (pattern
        (~ "px")
      )
    )
    (blank)
    (lazy tail
      (pattern
        (~ (seq \frac (? \expo)))
        (~ \expo)
      )
    )
    (blank)
    (lazy frac
      (pattern
        (~ (seq "." (* (chars "0-9"))))
      )
    )
    (blank)
    (lazy expo
      (pattern
        (~ (seq "e" (+ (chars "0-9"))))
      )
    )
  )
)
```

```output
.5
.5e3
e3
other
1.5
1.5e3
1e3
1
```

### The value of a lone call of a rule whose arms state no type is what the arms give.

```brix
(main
  (print (call kind ".5"))
  (print (call kind ".5e3"))
  (print (call kind "e3"))
  (print (call lone "[a"))
  (print (call lone "[a]"))
  (print (call mixed ".5"))
  (print (call mixed ".5e3"))
  (print (call mixed "e3"))
)

(fn-expr _ kind String
  (param text String)
  (match text
    (~ (seq \tail (! .))
      (?? it "none")
    )
    (default
      (inline "other")
    )
    (blank)
    (lazy tail
      (pattern
        (~ (seq \frac (? \expo)))
        (~ \expo)
      )
    )
    (blank)
    (lazy frac
      (pattern
        (~ (seq "." (* (chars "0-9"))))
      )
    )
    (blank)
    (lazy expo
      (pattern
        (~ (seq "e" (+ (chars "0-9"))))
      )
    )
  )
)

(fn-expr _ lone String
  (param text String)
  (match text
    (~ (seq \klass (! .))
      (?? it "none")
    )
    (default
      (inline "other")
    )
    (blank)
    (lazy klass
      (pattern
        (~ (seq "[" (* (seq (! "]") .)) (? "]")))
      )
    )
  )
)

(fn-expr _ mixed String
  (param text String)
  (match text
    (~ (seq (: t \tail) (! .))
      (?? t "none")
    )
    (default
      (inline "other")
    )
    (blank)
    (lazy tail
      (pattern
        (~ (seq \frac (? \expo)))
        (~ \expo)
      )
    )
    (blank)
    (lazy frac
      (pattern
        (~ (seq "." (* (chars "0-9"))))
      )
    )
    (blank)
    (lazy expo String
      (pattern
        (~ (seq "e" (+ (chars "0-9")))
          "E"
        )
      )
    )
  )
)
```

```output
.5
.5e3
e3
[a
[a]
.5
.5e3
E
```

### A rule of one arm that states no type gives its text where its parts give text, and the value of its last part where that part gives a value.

```brix
(main
  (print (call spans ".5"))
  (print (call spans ".5e3"))
  (print (call valued ".5"))
  (print (call valued ".5e3"))
)

(fn-expr _ spans String
  (param text String)
  (match text
    (~ (seq (: t \tail) (! .))
      (?? t "none")
    )
    (default
      (inline "other")
    )
    (blank)
    (lazy tail
      (pattern
        (~ (seq \frac (? \expo)))
      )
    )
    (blank)
    (lazy frac
      (pattern
        (~ (seq "." (* (chars "0-9"))))
      )
    )
    (blank)
    (lazy expo
      (pattern
        (~ (seq "e" (+ (chars "0-9"))))
      )
    )
  )
)

(fn-expr _ valued String
  (param text String)
  (match text
    (~ (seq (: t \tail) (! .))
      (?? t "none")
    )
    (default
      (inline "other")
    )
    (blank)
    (lazy tail
      (pattern
        (~ (seq \frac (? \expo)))
      )
    )
    (blank)
    (lazy frac
      (pattern
        (~ (seq "." (* (chars "0-9"))))
      )
    )
    (blank)
    (lazy expo String
      (pattern
        (~ (seq "e" (+ (chars "0-9")))
          "E"
        )
      )
    )
  )
)
```

```output
.5
.5e3
none
E
```

### The text of a sequence of items that are no characters is what `slice-string` makes of them.

```brix
(import /core/tokenstream)
(blank)

(main
  (let bytes (list Byte))
  (+= bytes 104u8)
  (+= bytes 105u8)
  (+= bytes 33u8)
  (+= bytes 63u8)
  (let stream (call (of ChunkedTokenStream Byte) (as bytes (of Iterable Byte))))
  (for (in pair (in (~ Pair) stream))
    (print (. pair text))
  )
)

(type + Pair
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
  (fn-expr + This _ (over: Byte)
    (~ (seq U8 U8)
      (call This it)
    )
  )
)
```

```output
hi
!?
```

### A number type named as a pattern over byte items reads as many bytes as the number holds, the first the highest.

```brix
(import /core/tokenstream)
(blank)

(main
  (let bytes (list Byte))
  (+= bytes 2u8)
  (+= bytes 5u8)
  (+= bytes 1u8)
  (+= bytes 7u8)
  (+= bytes 9u8)
  (let stream (call (of ChunkedTokenStream Byte) (as bytes (of Iterable Byte))))
  (for (in head (in (~ Head) stream))
    (print (str (. head kind) " " (. head size)))
  )
)

(type + Head
  (prop + kind U8)
  (prop + size U64)
  (fn-do + This _
    (param kind _)
    (param size _)
  )
  (fn-expr + This _ (over: Byte)
    (~ (seq (: k U8) (: n \length))
      (call This k n)
      (blank)
      (lazy length
        (pattern
          (case? (== k 1)
            (~ U16
              (as it U64)
            )
          )
          (default
            (~ U8
              (as it U64)
            )
          )
        )
      )
    )
  )
)
```

```output
2 5
1 1801
```

### A function an action makes reads a name its pattern captured.

```brix
(import /core/tokenstream)
(blank)

(main
  (let bytes (list Byte))
  (+= bytes 3u8)
  (+= bytes 5u8)
  (+= bytes 6u8)
  (+= bytes 7u8)
  (+= bytes 1u8)
  (+= bytes 9u8)
  (let stream (call (of ChunkedTokenStream Byte) (as bytes (of Iterable Byte))))
  (for (in item (in (~ Item) stream))
    (print (. item doubled))
  )
)

(type + Item
  (prop + doubled (list Int))
  (fn-do + This _
    (param doubled _)
  )
  (fn-expr + This _ (over: Byte)
    (~ (seq (: k U8) (: data ({} Byte k)))
      (call This doubled)
      (blank)
      (lazy doubled (call (of Array Int) (as k Size) (:with (=> (index) (call twice ([] data index) k)))))
    )
  )
)

(fn-expr _ twice Int
  (param byte Byte)
  (param k U8)
  (+ (* (as byte Int) 2) (as k Int))
)
```

```output
13, 15, 17
19
```

### Over items that are not characters, a name a `.` captures holds the item, a count of `.` gives the items it took, and a `.` matches any item.

```brix
(main
  (let numbers (call (of List Int)))
  (+= numbers -5)
  (+= numbers 5000000)
  (+= numbers 7)
  (+= numbers 8)
  (for (in sum (~> (call number-source numbers) \sum-pair))
    (print sum)
  )
  (for (in pair (~> (call number-source numbers) \two))
    (print pair)
  )
  (let words (call (of List String)))
  (+= words "a")
  (+= words "b")
  (for (in joined (~> (call word-source words) \word-pair))
    (print joined)
  )
)

(fn-do + number-source (of Iterable Int)
  (param numbers (list Int))
  (for (in n numbers)
    (yield n)
  )
)

(fn-do + word-source (of Iterable String)
  (param words (list String))
  (for (in word words)
    (yield word)
  )
)

(fn-expr + sum-pair Int (over: Int)
  (~ (seq (: left .) (: right .))
    (+ left right)
  )
)

(fn-expr + two String (over: Int)
  (~ (: pair ({} . 2))
    (str (. pair length) " " ([] pair 0) " " ([] pair 1))
  )
)

(fn-expr + word-pair String (over: String)
  (~ (seq (: left .) (: right .))
    (str left right)
  )
)
```

```output
4999995
15
2 -5 5000000
2 7 8
ab
```

### `it` in a local rule's arm names that arm's match, so the enclosing rule does not read its own bytes as text.

```brix
(main
  (print
    (match (list 130u8 1u8 2u8)
      (~ (: f \frame)
        f
      )
      (default
        (inline 99u64)
      )
    )
  )
)

(fn-expr + frame U64 (over: Byte)
  (~ (seq (: a U8) (: n \size))
    n
    (blank)
    (lazy size
      (pattern
        (case? (== a 130)
          (~ U16
            (as it U64)
          )
        )
        (default
          (~ U8
            7u64
          )
        )
      )
    )
  )
)
```

```output
258
```

### A record that arrives one byte at a time continues where it stopped as each read arrives: every part of a pattern reads only the input it needs, an action runs once, and a record is given before any input after it is read.

```brix
(import /core/tokenstream)
(blank)

(main
  (print
    (match (call trickle "hello world")
      (~ (seq "hello" " " (: word (+ (chars "a-z"))))
        word
      )
      (default
        (inline "no literal")
      )
    )
  )
  (print
    (match (call trickle "12345;")
      (~ (seq (: digits ({} (chars "0-9") 3)) (: rest (* (chars "0-9"))) ";")
        (str digits " " rest)
      )
      (default
        (inline "no count")
      )
    )
  )
  (print
    (match (call trickle "abc")
      (~ (seq (& "ab") (: word (+ (chars "a-z"))))
        (str "ahead " word)
      )
      (default
        (inline "no lookahead")
      )
    )
  )
  (print
    (match (call trickle "acd")
      (~ (seq (! "ab") (: word (+ (chars "a-z"))))
        (str "not ahead " word)
      )
      (default
        (inline "no negative lookahead")
      )
    )
  )
  (print
    (match (call trickle "x")
      (~ (seq "x" (! .))
        "ended"
      )
      (default
        (inline "not ended")
      )
    )
  )
  (print
    (match (call trickle "xy")
      (~ (seq "x" (! .))
        "ended"
      )
      (default
        (inline "not ended")
      )
    )
  )
  (print
    (match (call trickle "abce")
      (~ (: word (| "abcd" "abce"))
        (str "chose " word)
      )
      (default
        (inline "no choice")
      )
    )
  )
  (print
    (match (call trickle "key=value")
      (~ (seq (: key \word) "=" (: value \word))
        (str key " " value)
      )
      (default
        (inline "no call")
      )
    )
  )
  (print
    (match (call trickle (++ (* "a" 3000) ";"))
      (~ (seq (: run (+ (chars "a"))) ";")
        (str (. run length))
      )
      (default
        (inline "no run")
      )
    )
  )
  (print
    (match (call (of ChunkedTokenStream Byte) (call Trickle (call (of Array Byte) 3000)))
      (~ (: data ({} Byte 3000))
        (str (. data size))
      )
      (default
        (inline "no bytes")
      )
    )
  )
  (print
    (match (call (of ChunkedTokenStream Byte) (call Trickle (call (of Array Byte) (list 1u8 0u8 2u8))))
      (~ (seq (: small U8) (: wide U16))
        (str small " " wide)
      )
      (default
        (inline "no numbers")
      )
    )
  )
  (let tally (call Tally))
  (for (in word (~> (call trickle "one two three ") (call \counted tally)))
    (print word)
  )
  (print (. tally count))
  (let names (call Trickle (as "ab;cd;" ([] Byte))))
  (for (in name (~> (call (of ChunkedTokenStream Char) names) \named))
    (print (str name " after " (. names cursor) " bytes"))
  )
  (let frames (call Trickle (call (of Array Byte) (list 5u8 0u8 7u8))))
  (for (in frame (in (~ Frame) (call (of ChunkedTokenStream Byte) frames)))
    (print (str "frame " (. frame size) " after " (. frames cursor) " bytes"))
  )
  (let failure (catch (call entries-of "ab;\ncd")))
  (if (is failure EntryError)
    (print (str "error at " (. (as failure EntryError) line) ":" (. (as failure EntryError) column)))
  )
)

(fn-expr + word String (over: Char)
  (~ (+ (chars "a-z")))
)

(fn-expr + counted String (over: Char)
  (param tally Tally)
  (~ (seq (: word (call \tracked tally)) " ")
    word
  )
)

(fn-expr + tracked String (over: Char)
  (param tally Tally)
  (~ (: word (+ (chars "a-z")))
    (call (. tally add) word)
  )
)

(fn-expr + named String (over: Char)
  (~ (seq (: name (+ (chars "a-z"))) ";")
    name
  )
)

(fn-expr + entry String (over: Char) (?! EntryError)
  (~ (seq (: name (+ (chars "a-z"))) ";" (? "\n"))
    name
  )
  (throw (call EntryError (+ (. location line) 1) (+ (. location column) 1)))
)

(fn-do + entries-of Size (?! EntryError)
  (param text String)
  (let count Size)
  (for (in name (~> (call trickle text) \entry))
    (+= count 1)
  )
  (return count)
)

(type + EntryError (is Error)
  (prop + line Size)
  (prop + column Size)
  (fn-do + This _
    (param line _)
    (param column _)
    (= message (str "Entry error at line " line))
  )
)

(fn-do + trickle (of ChunkedTokenStream Char)
  (param text String)
  (return (call (of ChunkedTokenStream Char) (call Trickle (as text ([] Byte)))))
)

(type + Tally
  (prop + count Size)
  (fn-do + add String
    (param word String)
    (+= count 1)
    (return word)
  )
)

(type + Frame (is Tuple)
  (prop + size U64)
  (fn-expr + This _ (over: Byte)
    (~ (seq (: head U8) (: size \measure))
      (call This size)
      (blank)
      (lazy measure
        (pattern
          (case? (!= head 0u8)
            (~ ({} U8 0)
              (as head U64)
            )
          )
          (default
            (~ ({} U8 1)
              0u64
            )
          )
        )
      )
    )
  )
)

(type + Trickle
  (implements ByteReadable)
  (prop + data ([] Byte))
  (prop + cursor Index)
  (fn-do + This _
    (param data _)
  )
  (fn-do + read Size
    (param size Size)
    (param buffer ([] Byte))
    (if (or (>= cursor (. data size)) (== size 0))
      (return 0)
    )
    (= ([] buffer 0) ([] data cursor))
    (+= cursor 1)
    (return 1)
  )
)
```

```output
world
123 45
ahead abc
not ahead acd
ended
not ended
chose abce
key value
3000
3000
1 2
one
two
three
3
ab after 3 bytes
cd after 6 bytes
frame 5 after 1 bytes
frame 0 after 3 bytes
error at 2:3
```

### A stream piped through a pattern takes a helper with `with`, evaluated once as the pipe starts to read: every record the pipe matches runs under that one helper, in either form of the pipe.

```brix
(import /core/patterns)
(import /core/tokenstream)
(blank)

(main
  (let words (~> "ab cd ef" \word (with (call made))))
  (print "piped")
  (for (in word words)
    (print word)
  )
  (blank)
  (let tagger (call Tagger))
  (let destination (call (of List String)))
  "gh ij"
  (pipe ~> (seq (: text (+ (chars "a-z"))) (? " "))
    (with tagger)
    text
  )
  (pipe *-> destination)
  (print (call (. "," join) destination))
  (print (. tagger count))
)

(fn-do + made Tagger
  (print "made")
  (return (call Tagger))
)

(fn-expr + word _ (over: Char)
  (~ (seq (: text (+ (chars "a-z"))) (? " "))
    text
  )
)

(type + Tagger
  (implements (of PatternMaterializationRoot String))
  (implements (of PatternCaptureMaterializer Char String))
  (implements (of PatternCaptureMaterializer Byte String))
  (implements (of PatternRollback Size))
  (prop + count Size)
  (fn-expr + mark Size
    count
  )
  (fn-do + rewind _
    (param mark Size)
    (= count mark)
  )
  (fn-do + (of materialize-capture Token) String
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (+= count 1)
    (return (str "<" count ">"))
  )
)
```

```output
piped
made
<1>
<2>
<3>
<1>,<2>
2
```

### A pipe whose helper is a PatternRecordBoundary asks it to begin each record where the stream has more, before it matches the record and after the record before has been delivered. A match given the same helper never asks.

```brix
(main
  (let counter (call Counter))
  (for (in word (~> "ab cd ef" \word (with counter)))
    (print word)
  )
  (print (. counter count))
  (print
    (match "gh"
      (with counter)
      (~ \word)
      (default
        (inline "none")
      )
    )
  )
  (print (. counter count))
  (for (in word (~> "" \word (with counter)))
    (print word)
  )
  (print (. counter count))
)

(fn-expr + word _ (over: Char)
  (~ (seq (: text (+ (chars "a-z"))) (? " "))
    text
  )
)

(type + Counter
  (implements PatternRecordBoundary)
  (prop + count Size)
  (fn-do + begin-pattern-record _
    (+= count 1)
    (print (str "record " count))
  )
)
```

```output
record 1
ab
record 2
cd
record 3
ef
3
gh
3
3
```

### A pipe whose helper delegates PatternRecordBoundary to a property asks what the property holds to begin each record.

```brix
(main
  (for (in word (~> "ab cd" \word (with (call Delegating))))
    (print word)
  )
)

(fn-expr + word _ (over: Char)
  (~ (seq (: text (+ (chars "a-z"))) (? " "))
    text
  )
)

(type + Delegating
  (delegate PatternRecordBoundary)
  (prop _ counter Counter)
  (fn-do + This _
    (lazy counter (call Counter))
  )
)

(type + Counter
  (implements PatternRecordBoundary)
  (prop + count Size)
  (fn-do + begin-pattern-record _
    (+= count 1)
    (print (str "record " count))
  )
)
```

```output
record 1
ab
record 2
cd
```

### A stream piped through a pattern reports what fails in its pattern where it fails.

```brix crash
(main
  (for (in word (~> "ab" (seq (: text (+ (chars "a-z"))) \nope)))
    (print word)
  )
)
```

```output
Exception: Rule "nope" not found
<no file:3>
      (for (in word (~> "ab" (seq (: text (+ (chars "a-z"))) \nope)))
                                                             ^^^^^
```

### A match over a list of values gives each item pattern the item itself: a tuple as a whole, and a union by its case.

```brix
(main
  (let points (list (call Pt 1 2) (call Pt 3 4)))
  (print
    (match points
      (~ (seq (\{} (call show it)) (\{} (== (. it y) 4)))
        "points"
      )
      (default
        (inline "fail")
      )
    )
  )
  (let tokens (list (call (. Tok word) "let") (call (. Tok num) 5)))
  (print
    (match tokens
      (~ (seq (\{} (is it .word)) (\{} (is it .num)))
        "tokens"
      )
      (default
        (inline "fail")
      )
    )
  )
)

(fn-do + show Bool
  (param p Pt)
  (print (str "item " p))
  (return (== (. p x) 1))
)

(type + Pt (is (tuple (: x Int) (: y Int))))

(type + Tok (is Union)
  (variant word (tuple String))
  (variant num (tuple Int))
)
```

```output
item (1, 2)
points
tokens
```

### Over items of a union, a case named by its symbol matches one item that holds the case and gives what the case holds, or the item where it holds nothing, and a type the union has as a case matches one item that holds it and gives what it holds.

```brix
(main
  (let events (of List Event) (list .press (call (. Event move) 40) (call (. Event move) 95) .release .press .release (call .typed "hi") (call Spot 3 4) .release))
  (for (in gesture (~> events \gesture))
    (print gesture)
  )
)

(fn-expr + gesture String (over: Event)
  (~ (seq .press (: xs (+ .move)) .release)
    (str "drag " (. xs length) ", last at " ([] xs (- (. xs length) 1)))
  )
  (~ (seq .press .release)
    "tap"
  )
  (~ (: text .typed)
    (str "typed " text)
  )
  (~ (: spot Spot)
    (str "spot " (+ (. spot x) (. spot y)))
  )
  (~ (: event (| .press .release))
    (str "lone " event)
  )
)

(type + Event (is Union)
  (variant press)
  (variant release)
  (variant move (tuple Int))
  (variant typed (tuple String))
  (variant Spot)
)

(type + Spot (is (tuple (: x Int) (: y Int))))
```

```output
drag 2, last at 95
tap
typed hi
spot 7
lone release
```

### A value pattern may name a local of the rule that is declared after the arm using it.

```brix
(main
  (print
    (match "\t"
      (~ (: p Pressed)
        (as (. p key) Int)
      )
    )
  )
)

(type + KeyCode (is Char)
  (variant tab (= 9))
  (variant return (= 13))
)

(type + Pressed (is Tuple)
  (prop + key Char)
  (fn-expr + This _ (over: Char)
    (~ (: c (| $tab-key $return-key))
      (call This c)
    )
    (where
      (lazy tab-key (. KeyCode tab))
      (lazy return-key (. KeyCode return))
    )
  )
)
```

```output
9
```

### Alternatives that each test an item give the value of the one that passed, however many there are.

```brix
(main
  (print
    (match "\r"
      (~ (: p Pressed)
        (as (. p key) Int)
      )
    )
  )
  (print
    (match "\n"
      (~ (: p Pressed)
        (as (. p key) Int)
      )
    )
  )
)

(type + KeyCode (is Char)
  (variant tab (= 9))
  (variant return (= 13))
  (variant feed (= 10))
  (variant back (= 127))
)

(type + Pressed (is Tuple)
  (prop + key Char)
  (fn-expr + This _ (over: Char)
    (~ (: c (| $tab-key $return-key $back-key))
      (call This c)
    )
    (~ (: c (| (| $tab-key $return-key) (| $feed-key $back-key)))
      (call This c)
    )
    (where
      (lazy tab-key (. KeyCode tab))
      (lazy return-key (. KeyCode return))
      (lazy feed-key (. KeyCode feed))
      (lazy back-key (. KeyCode back))
    )
  )
)
```

```output
13
10
```

### A value pattern may name a constant of the item type, which is compared with one item.

```brix
(main
  (print (call classify "\x7f"))
  (print (call classify "\t"))
  (print (call classify "a"))
)

(fn-expr _ classify String
  (param text String)
  (match text
    (~ (: c (| $tab $del))
      "control"
    )
    (~ (: c .)
      "other"
    )
    (default
      "none"
    )
  )
  (where
    (lazy tab Char (char "\t"))
    (lazy del Char (char "\x7f"))
  )
)
```

```output
control
control
other
```

### A helper that observes literal words is told of each lowercase word a literal matches, and is rewound to where a part began when the part fails, and when a lookahead passes.

```brix
(main
  (let watcher (call Watcher))
  (print
    (match "let it-be Now ab"
      (with watcher)
      (~ (seq "let" " " "go")
        "go"
      )
      (~ (seq (& "l") "let" (+ " ") \words " " (+ (| "a" "b")))
        "words"
      )
      (default
        (inline "none")
      )
    )
  )
  (print (call (. " " join) (. watcher seen)))
)

(fn-expr + words String (over: Char)
  (~ (seq "it-be Now" (! "!")))
  (~ "it")
)

(type + Watcher
  (implements PatternLiteralObserver)
  (implements (of PatternRollback Size))
  (prop + seen (list String))
  (fn-expr + mark Size
    (. seen length)
  )
  (fn-do + rewind _
    (param mark Size)
  )
  (fn-do + record-pattern-literal _
    (param start Index)
    (param end Index)
    (+= seen (str "+" start ".." end))
  )
  (fn-do + rewind-pattern-literals _
    (param cursor Index)
    (+= seen (str "<" cursor))
  )
)
```

```output
words
+0..3 <4 <0 <0 +0..1 <0 +0..3 +4..9 <13
```

### Under a helper that observes literal words, a rule that matches no literal runs without the helper, so nothing that fails within it rewinds the helper.

```brix
(main
  (let watcher (call Watcher))
  (print
    (match "ab  cd"
      (with watcher)
      (~ (seq \letters \gap \letters (? \gap) (? "x"))
        "found"
      )
      (default
        (inline "none")
      )
      (blank)
      (lazy gap
        (pattern
          (~ (seq \sp \sp))
          (~ \sp)
        )
      )
      (blank)
      (lazy sp
        (pattern
          (~ (| " " "\t"))
        )
      )
    )
  )
  (print (call (. " " join) (. watcher seen)))
)

(fn-expr + letters String (over: Char)
  (~ (seq (chars "a-z") (chars "a-z")))
)

(type + Watcher
  (implements PatternLiteralObserver)
  (implements (of PatternRollback Size))
  (prop + seen (list String))
  (fn-expr + mark Size
    (. seen length)
  )
  (fn-do + rewind _
    (param mark Size)
  )
  (fn-do + record-pattern-literal _
    (param start Index)
    (param end Index)
    (+= seen (str "+" start ".." end))
  )
  (fn-do + rewind-pattern-literals _
    (param cursor Index)
    (+= seen (str "<" cursor))
  )
)
```

```output
found
<6 <6 <6
```

### A local rule of a rule typed for a helper that observes literal words may call itself, and runs under the helper each time.

```brix
(main
  (let watcher (call Watcher))
  (print
    (match "((a)b)"
      (with watcher)
      (~ (: v \nest-all)
        v
      )
      (default
        (inline "none")
      )
    )
  )
  (print (call (. " " join) (. watcher seen)))
)

(fn-expr + nest-all String (over: Char)
  (~ (: v \inner)
    v
  )
  (where
    (lazy inner String
      (pattern
        (~ (seq "(" (: v \inner) (? "b") ")")
          (str "[" v "]")
        )
        (~ "a"
          "a"
        )
      )
    )
  )
)

(type + Watcher
  (implements PatternLiteralObserver)
  (implements (of PatternRollback Size))
  (prop + seen (list String))
  (fn-expr + mark Size
    (. seen length)
  )
  (fn-do + rewind _
    (param mark Size)
  )
  (fn-do + record-pattern-literal _
    (param start Index)
    (param end Index)
    (+= seen (str "+" start ".." end))
  )
  (fn-do + rewind-pattern-literals _
    (param cursor Index)
    (+= seen (str "<" cursor))
  )
)
```

```output
[[a]]
+2..3 <3 +4..5
```

### A helper may delegate `PatternCollectorStorage` to a property, and the repeats of a match keep their items in the storage that property gives.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (let outer (call Outer (call Inner arena)))
  (print
    (match "a,bc,d"
      (with outer)
      (~ (seq (: ...items (+ (chars "a-z"))) (* (seq "," (: ...items (+ (chars "a-z"))))))
        (call (. ";" join) items)
      )
      (default
        (inline "none")
      )
    )
  )
)

(type + Inner
  (implements (of PatternCollectorStorage ArenaBacking))
  (prop + pattern-collector-context Arena)
  (fn-do + This _
    (param pattern-collector-context _)
  )
)

(type + Outer
  (implements (of PatternCollectorStorage ArenaBacking))
  (delegate (of PatternCollectorStorage ArenaBacking))
  (prop _ inner Inner)
  (fn-do + This _
    (param inner _)
  )
)
```

```output
a;bc;d
```

### Under a helper that observes literal words, a rule matched for its text, as in a lookahead, takes the text of each rule it calls, whatever type that rule states.

```brix
(main
  (let watcher (call Watcher))
  (print
    (match "for x"
      (with watcher)
      (~ (seq (! \keyword) (+ (chars "a-z")))
        "name"
      )
      (~ (seq (: p \preposition) " " (chars "a-z"))
        (str "preposition " p)
      )
      (default
        (inline "none")
      )
      (blank)
      (lazy preposition Int
        (pattern
          (~ "for")
          (~ "with")
        )
      )
      (blank)
      (lazy keyword
        (pattern
          (~ "if")
          (~ \preposition)
        )
      )
    )
  )
  (print (call (. " " join) (. watcher seen)))
)

(type + Watcher
  (implements PatternLiteralObserver)
  (implements (of PatternRollback Size))
  (implements (of PatternCaptureMaterializer Char Int))
  (implements (of PatternCaptureMaterializer Byte Int))
  (prop + seen (list String))
  (fn-expr + mark Size
    (. seen length)
  )
  (fn-do + rewind _
    (param mark Size)
  )
  (fn-do + record-pattern-literal _
    (param start Index)
    (param end Index)
    (+= seen (str "+" start ".." end))
  )
  (fn-do + rewind-pattern-literals _
    (param cursor Index)
    (+= seen (str "<" cursor))
  )
  (fn-do + (of materialize-capture Token) Int
    (param source ([] Token))
    (param start Index)
    (param end Index)
    (return (as (- end start) Int))
  )
)
```

```output
preposition 3
+0..3 <0 <0 <0 <0 +0..3
```

```brix
(import ./lib/token)
(blank)

(main
  (for (in token (in (~ Token) "ab"))
    (print (. token name))
  )
  (for (in word (in (~ Word) "ab"))
    (print (. word text))
  )
)

(type + Word
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
  (fn-expr + This _ (over: Char)
    (~ "a"
      (call Word "word a")
    )
    (~ "b"
      (call Word "word b")
    )
  )
)
```

```brix file=lib/token.bx
(type + Token
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
  (fn-expr + This _ (over: Char)
    (~ "a"
      (call Token "token a")
    )
    (~ "b"
      (call Token "token b")
    )
  )
)
```

```output
token a
token b
word a
word b
```

### An arm with no action gives the text it matched as the type its rule states.

```brix
(main
  (for (in value (~> "2.5" \decimal))
    (print (* value 2))
  )
  (for (in value (~> "7" \digit))
    (print value)
  )
  (for (in value (~> "z" \letter))
    (print value)
  )
)

(fn-expr + decimal F64 (over: Char)
  (~ (seq (+ (chars "0-9")) "." (+ (chars "0-9"))))
)

(fn-expr + digit I32 (over: Char)
  (~ (chars "0-9"))
)

(fn-expr + letter Char (over: Char)
  (~ (chars "a-z"))
)
```

```output
5
7
z
```

### An action that gives text is not read as the number its rule states.

```brix compile-error
(main
  (for (in value (~> "7" \digit))
    (print value)
  )
)

(fn-expr + digit I32 (over: Char)
  (~ (chars "0-9")
    it
  )
)
```

```output
Exception: Expected I32 here, but this is String, and there is no conversion between them
<no file:9>
      (~ (chars "0-9")
      ^^^^^^^^^^^^^^^
```

### An action converts to the type its rule states only as a value converts unasked, not as one written with "as" does: a case of a union is not read as the integer a local rule states.

```brix compile-error
(main
  (for (in value (~> "x" \count))
    (print value)
  )
)

(type + Part (is Union)
  (variant digits (tuple (: length Int)))
  (variant other)
)

(fn-expr + count Int (over: Char)
  (~ (: n \letters)
    n
  )
  (where
    (lazy letters Int
      (pattern
        (~ (+ (chars "0-9"))
          (. it length)
        )
        (~ (+ (chars "a-z"))
          (. Part other)
        )
      )
    )
  )
)
```

```output
Exception: Expected I64 here, but this is Part, which is not always I64; match it with "(case (: name I64) …)", or write "(as … I64)"
<no file:23>
            (~ (+ (chars "a-z"))
            ^^^^^^^^^^^^^^^^^^^
```

### A stream constructor's own class constructed with values it does not take is reported by its name, with the constructors a call can make.

```brix crash
(main
  (print
    (match "a=b"
      (~ (: packet Packet)
        packet
      )
      (throw (call Panic "none"))
    )
  )
)

(type + Packet (is (tuple (: key String) (: value String)))
  (fn-expr + This _ (over: Char)
    (~ (seq (: key (+ (chars "a-z"))) "=" (: value (? \word)))
      (call This key value)
    )
    (where
      (lazy word String
        (pattern
          (~ (+ (chars "a-z")))
        )
      )
    )
  )
)
```

```output
Exception: No constructor of Packet accepts (String, String?); its declared constructors are "Packet(key String, value String)"
<no file:16>
          (call This key value)
                ^^^^
```

### An action reads the capture types its own arm binds, not another arm's capture of the same name.

```brix
(main
  (for (in event (in (~ Events) "ax12;3yzx4w"))
    (print (or (is event Pair) (is event Single)))
  )
)

(type + Events (is Union)
  (variant Pair)
  (variant Single)
  (variant Other)
  (fn-expr + This Events (over: Char)
    (~ "a"
      (call Single 1)
    )
    (~ (seq "x" (: code I32) ";" (: modifiers I32) "y")
      (call pair code modifiers)
    )
    (~ "z"
      (call Other)
    )
    (~ (seq "x" (: code (? \number)) (: modifiers (? \suffix)) "w")
      (call single (?? code 1) (?? modifiers 1))
    )
    (where
      (lazy number I32
        (pattern
          (~ (: value I32)
            value
          )
        )
      )
      (lazy suffix I32
        (pattern
          (~ (seq ";" (: value I32))
            value
          )
        )
      )
    )
  )
)

(fn-expr _ pair Events
  (param code I32)
  (param modifiers I32)
  (as (call Pair (+ code modifiers)) Events)
)

(fn-expr _ single Events
  (param code I32)
  (param modifiers I32)
  (as (call Single (+ code modifiers)) Events)
)

(type + Base)

(type + Pair (is Base)
  (prop + total I32)
  (fn-do + This _
    (param total _)
  )
)

(type + Single (is Base)
  (prop + total I32)
  (fn-do + This _
    (param total _)
  )
)

(type + Other (is Base))
```

```output
true
true
false
true
```

### A rule a pattern calls is never a name a match captures, though the call is in a match of its own, as a pattern given to a rule is, and in the body of the rule the name captures.

```brix
(main
  (print
    (match "[a,a]"
      (~ \list)
      (default
        (inline "none")
      )
    )
  )
)

(fn-expr + list String (over: Char)
  (~ (: value \value)
    value
  )
  (where
    (lazy value String
      (pattern
        (~ (seq "[" (: items (? (call \delimited (~ ",") (~ \value)))) "]")
          "list"
        )
        (~ "a")
      )
    )
  )
)
```

```output
list
```

### A name captured over a sequence holds the text it matched, though the sequence ends with an optional part.

```brix
(main
  (print
    (match "12.5"
      (~ \number)
      (default
        (inline "none")
      )
    )
  )
)

(fn-expr + number String (over: Char)
  (~ (: digits (seq (+ (chars "0-9")) (? (seq "." (+ (chars "0-9"))))))
    (call take digits)
  )
)

(fn-expr + take String
  (param text String)
  text
)
```

```output
12.5
```

### An action can't read a name that an alternative it may pass doesn't capture.

```brix compile-error
(main
  (print
    (match "bb"
      (~ (seq (| (: x "a") "b") "b")
        (str "x " x)
      )
      (default
        (inline "none")
      )
    )
  )
)
```

```output
Exception: Capture x holds no value on some ways this arm matches; capture it in every alternative
<no file:6>
            (str "x " x)
                      ^
```

### A name can't be captured twice on one way through a pattern.

```brix compile-error
(main
  (print
    (match "ab"
      (~ (seq (: x "a") (: x "b"))
        x
      )
      (default
        (inline "none")
      )
    )
  )
)
```

```output
Exception: Capture x is captured twice on one way through the pattern
<no file:5>
          (~ (seq (: x "a") (: x "b"))
                            ^^^^^^^^^
```

### A name captured in alternatives holds one type.

```brix compile-error
(main
  (print
    (match "a1"
      (~ (seq (| (: x "a") (: x \digit)) .)
        "x"
      )
      (default
        (inline "none")
      )
    )
  )
)

(fn-expr + digit Int (over: Char)
  (~ (chars "0-9")
    (call Int it)
  )
)
```

```output
Exception: Capture x holds String in one alternative and I64 in another
<no file:5>
          (~ (seq (| (: x "a") (: x \digit)) .)
                               ^^^^^^^^^^^^
```

### Alternatives under a capture give values of one type.

```brix compile-error
(main
  (print
    (match "a1"
      (~ (seq (: x (| "a" \digit)) .)
        "x"
      )
      (default
        (inline "none")
      )
    )
  )
)

(fn-expr + digit Int (over: Char)
  (~ (chars "0-9")
    (call Int it)
  )
)
```

```output
Exception: Alternatives give String and I64; give each a value of one type
<no file:5>
          (~ (seq (: x (| "a" \digit)) .)
                       ^^^^^^^^^^^^^^
```

### An arm with no pattern has no `it`.

```brix compile-error
(main
  (print
    (match "ab"
      (~ "x"
        "x"
      )
      (default
        (inline (. it length))
      )
    )
  )
)
```

```output
Exception: An arm with no pattern has no "it"; read the subject by its name
<no file:8>
          (default
          ^^^^^^^
```

### A match's arms are all values or all patterns.

```brix compile-error
(main
  (print
    (match "ab"
      (case "ab"
        "same"
      )
      (~ (seq "a" .)
        "pattern"
      )
      (default
        (inline "none")
      )
    )
  )
)
```

```output
Exception: This pattern has no stream to read; outside a rule, write it in a match whose arms are all ~ patterns
<no file:8>
          (~ (seq "a" .)
          ^^^^^^^^^^^^^
```

### A rule can't be a method.

```brix compile-error
(main
  (print (call (. (call Words) first) "ab"))
)

(type + Words
  (fn-expr + first String
    (param text String)
    (match text
      (~ \word)
      (default
        (inline "none")
      )
    )
  )
  (fn-expr + word String (over: Char)
    (~ (+ (chars "a-z")))
  )
)
```

```output
Exception: A rule can't be a method; declare it at module level
<no file:16>
      (fn-expr + word String (over: Char)
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A local rule takes no arguments.

```brix compile-error
(main
  (print
    (match "aaaaa"
      (~ (call \run 5))
      (default
        (inline "none")
      )
      (lazy run
        (pattern
          (~ (+ "a"))
        )
      )
    )
  )
)
```

```output
Exception: A local rule takes no arguments or types; declare it at module level to give it some
<no file:5>
          (~ (call \run 5))
             ^^^^^^^^^^^^^
```

### A rule over another type of items says what it was asked to match.

```brix compile-error
(main
  (let numbers (list Int) (list 1 2))
  (print
    (match numbers
      (~ \word)
      (default
        (inline "none")
      )
    )
  )
)

(fn-expr + word String (over: Char)
  (~ (+ (chars "a-z")))
)
```

```output
Exception: No rule "word" matches I64 items with these arguments
<no file:6>
          (~ \word)
             ^^^^^
```

### A rule called as a function says that it is a rule.

```brix compile-error
(main
  (print (call word "ab"))
)

(fn-expr + word String (over: Char)
  (~ (+ (chars "a-z")))
)
```

```output
Exception: "word" is a rule, which matches a stream; use it in a pattern, as in ~ \word
<no file:3>
      (print (call word "ab"))
             ^^^^^^^^^^^^^^^^
```

### A match runs a pattern only over a value it can iterate.

```brix compile-error
(main
  (print
    (match 5
      (~ .)
      (default
        (inline "none")
      )
    )
  )
)
```

```output
Exception: Can't convert to iterable
<no file:5>
          (~ .)
          ^^^^^
```

### A type a pattern names must exist.

```brix compile-error
(main
  (print
    (match "ab"
      (~ Nope)
      (default
        (inline "none")
      )
    )
  )
)
```

```output
Exception: Type "Nope" not found
<no file:5>
          (~ Nope)
             ^^^^
```
