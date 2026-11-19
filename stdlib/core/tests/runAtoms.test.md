# Atoms

```brix
(import /core/atom-pool)
(blank)

(main
  (let atoms (call AtomPool))
  (finally
    (call (. atoms release))
  )
  (let empty (call (. atoms store) ""))
  (let duplicate (call (. atoms store) ""))
  (print (== empty duplicate))
  (print (call (. atoms owns) empty))
  (print (== (. empty text) ""))
  (print (. atoms length))
  (print (. Atom size))
)
```

```output
true
true
true
1
8
```

```brix
(import /core/atom-pool)
(blank)

(main
  (let atoms (call AtomPool))
  (finally
    (call (. atoms release))
  )
  (let stored (call (. atoms store) "alpha-beta"))
  (let spanned (call (. atoms store) (as "alpha-beta" ([] Byte)) 0 10))
  (print (== stored spanned))
  (let tail (call (. atoms store) (as "x-beta" ([] Byte)) 2 6))
  (print (. tail text))
  (print (== tail (call (. atoms store) "beta")))
  (print (. atoms length))
)
```

```output
true
beta
true
2
```

```brix
(import /core/arena)
(import /core/array-builder)
(blank)

(main
  (let arena (call Arena))
  (finally
    (inline (call (. arena release)))
  )
  (let first (as (call (. arena copy-string) "first") Atom))
  (let second (as (call (. arena copy-string) "second") Atom))
  (let atoms (call (of ArrayBuilder Atom) arena))
  (call (. atoms append) first)
  (= ([] atoms 0) second)
  (print (. ([] atoms 0) text))
  (print (call (. arena owns) (as (. ([] atoms 0) address) Pointer)))
)
```

```output
second
true
```

```brix
(import /core/atom-pool)
(blank)

(main
  (let atoms (call AtomPool))
  (finally
    (call (. atoms release))
  )
  (let alpha (call (. atoms store) "alpha"))
  (let duplicate (call (. atoms store) "alpha"))
  (print (== alpha duplicate))
  (print (== (. alpha address) (. duplicate address)))
  (print (call (. atoms owns) alpha))
  (print (. alpha text))
  (print (as alpha String))
  (print ([] (. alpha text) (.. 1 3)))
  (print (str "atom=" (. alpha text)))
)
```

```output
true
true
true
alpha
alpha
lph
atom=alpha
```

```brix
(import /core/atom-pool)
(blank)

(main
  (let atoms (call AtomPool))
  (let others (call AtomPool))
  (finally
    (call (. atoms release))
    (call (. others release))
  )
  (let codepoints (. "xAé漢😀z" codepoints))
  (let mixed (call (. atoms store) codepoints 1 5))
  (let duplicate (call (. atoms store) "Aé漢😀"))
  (let other (call (. others store) "Aé漢😀"))
  (print (== mixed duplicate))
  (print (!= mixed other))
  (print (. mixed text length))
  (print (. mixed text size))
  (print ([] (. mixed text) (.. 1 2)))
  (print (. atoms length))
)
```

```output
true
true
4
10
é漢
1
```

```brix
(import /core/atom-pool)
(blank)

(main
  (let atoms (call AtomPool))
  (finally
    (call (. atoms release))
  )
  (let empty (call (. atoms store) ""))
  (let alpha (call (. atoms store) "alpha"))
  (let beta (call (. atoms store) "beta"))
  (print (tuple (call (. atoms ordinal) empty) (call (. atoms ordinal) alpha) (call (. atoms ordinal) beta)))
  (print (tuple (. ([] atoms 0) text) (. ([] atoms 1) text) (. ([] atoms 2) text)))
  (let order "")
  (for (in atom atoms)
    (++= order (str "[" (. atom text) "]"))
  )
  (print order)
)
```

```output
(0, 1, 2)
(, alpha, beta)
[][alpha][beta]
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let text (call (. arena copy-string) "arena-atom"))
  (let atom (as text Atom))
  (print (call (. arena owns) (as (. atom address) Pointer)))
  (print (. atom text))
  (print (as atom String))
)
```

```output
true
arena-atom
arena-atom
```

```brix
(import /core/atom-pool)
(blank)

(main
  (let atoms (call AtomPool))
  (let others (call AtomPool))
  (finally
    (call (. atoms release))
    (call (. others release))
  )
  (let attempt
    (do-new
      (let alpha (call (. atoms store) "alpha"))
      (let other-alpha (call (. others store) "alpha"))
      (let ordinal (call (. atoms ordinal) other-alpha))
      (on-failure failure
        (print (str "error: " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
error: Atom belongs to another AtomPool or is not live
```

```brix
(import /core/atom-pool)
(blank)

(main
  (let atoms (call AtomPool))
  (finally
    (call (. atoms release))
  )
  (let attempt
    (do-new
      (let source (. "abc" codepoints))
      (let invalid (call (. atoms store) source 2 1))
      (on-failure failure
        (print (str "error: " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
error: Atom codepoint span is invalid
```

```brix
(import /core/atom-pool)
(blank)

(main
  (let atoms (call AtomPool))
  (call (. atoms release))
  (let attempt
    (do-new
      (print (. atoms length))
      (on-failure failure
        (print (str "error: " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
error: AtomPool Arena is stale or released
```

```brix
(import /core/arena)
(import /core/atom-pool)
(blank)

(main
  (let atoms (call AtomPool))
  (let first (call Arena))
  (let second (call Arena))
  (let first-name (as (call (. first copy-string) "ArenaAtomTree") Atom))
  (let second-name (as (call (. second copy-string) "ArenaAtomTree") Atom))
  (print (. first-name text))
  (print (!= first-name second-name))
  (print (call (. first owns) (as (. first-name address) Pointer)))
  (print (call (. second owns) (as (. second-name address) Pointer)))
  (let first-promoted (call (. atoms store) (. first-name text)))
  (let second-promoted (call (. atoms store) (. second-name text)))
  (print (== first-promoted second-promoted))
  (print (call (. atoms owns) first-promoted))
  (print (not (call (. first owns) (as (. first-promoted address) Pointer))))
  (print (not (call (. second owns) (as (. second-promoted address) Pointer))))
  (print (!= first-name first-promoted))
  (call (. first release))
  (call (. second release))
  (print (. first-promoted text))
  (call (. atoms release))
)
```

```output
ArenaAtomTree
true
true
true
true
true
true
true
true
ArenaAtomTree
```

```brix
(import /core/atom-pool)
(blank)

(main
  (let atoms (call AtomPool))
  (finally
    (call (. atoms release))
  )
  (let first (call (. atoms store) "abca"))
  (let second (call (. atoms store) "axya"))
  (let third (call (. atoms store) "aZZa"))
  (let source (. "xabca|axyaz" codepoints))
  (print (and (!= first second) (!= second third)))
  (print (== (call (. atoms store) source 1 5) first))
  (print (== (call (. atoms store) source 6 10) second))
  (print (. atoms length))
)
```

```output
true
true
true
3
```

```brix
(import /core/atom-pool)
(blank)

(main
  (let atoms (call AtomPool))
  (finally
    (call (. atoms release))
  )
  (let first (call (. atoms store) "a00z"))
  (for (in index (..< 1 24))
    (call (. atoms store) (str "a" (/ index 10) (% index 10) "z"))
  )
  (print (. atoms length))
  (print (== (call (. atoms store) "a00z") first))
  (print (call (. atoms ordinal) (call (. atoms store) "a23z")))
)
```

```output
24
true
23
```

```brix
(import /core/atom-pool)
(blank)

(main
  (let atoms (call AtomPool))
  (finally
    (call (. atoms release))
  )
  (let first (call (. atoms store) "éa😀"))
  (let second (call (. atoms store) "éb😀"))
  (let source (. "xéa😀|éb😀z" codepoints))
  (print (!= first second))
  (print (== (call (. atoms store) source 1 4) first))
  (print (== (call (. atoms store) source 5 8) second))
  (print (. atoms length))
)
```

```output
true
true
true
2
```

```brix
(import /core/atom-pool)
(blank)

(main
  (let atoms (call AtomPool))
  (finally
    (call (. atoms release))
  )
  (let first (call (. atoms store) "a100000z"))
  (for (in index (..< 100001 160000))
    (call (. atoms store) (str "a" index "z"))
  )
  (print (. atoms length))
  (print (== (call (. atoms store) "a100000z") first))
  (print (call (. atoms ordinal) (call (. atoms store) "a159999z")))
)
```

```output
60000
true
59999
```

```brix
(main
  (print (call selected-name (call .named (as "found" Atom))))
  (print (call selected-name .missing))
  (print (call assigned-name (call .named (as "found" Atom))))
  (print (call assigned-name .missing))
  (print (call continuing-name (call .named (as "found" Atom))))
  (print (call continuing-name .missing))
)

(type + AtomChoice (is Union)
  (variant named (tuple (: value Atom)))
  (variant missing)
)

(fn-do + selected-name String
  (param choice AtomChoice)
  (let name (? Atom) null)
  (match? choice
    (case (call .named value)
      (= name value)
      (print (. name text))
    )
  )
  (if (not name)
    (return "missing")
  )
  (return (. name text))
)

(fn-do + assigned-name String
  (param choice AtomChoice)
  (let name (? Atom) null)
  (match choice
    (case (call .named value)
      (= name value)
    )
    (default
      (= name (as "fallback" Atom))
    )
  )
  (return (. name text))
)

(fn-do + continuing-name String
  (param choice AtomChoice)
  (let name (? Atom) null)
  (match choice
    (case (call .named value)
      (= name value)
    )
    (default
      (return "missing")
    )
  )
  (return (. name text))
)
```

```output
found
found
missing
found
fallback
found
missing
```

### Atoms compared, read as text and as an address, passed, returned and held in a tuple and an object

```brix
(main
  (let first (as "one" Atom))
  (let again first)
  (let other (as "two" Atom))
  (print (== first again))
  (print (!= first again))
  (print (== first other))
  (print (!= first other))
  (print (. first text))
  (print (as other String))
  (print (== (. first address) (. again address)))
  (print (== (. first address) (. other address)))
  (print (. Atom size))
  (print (. (call pick true) text))
  (print (. (call pick false) text))
  (print (call same first again))
  (print (call same first other))
  (let pair (call Pair first 3))
  (print (. pair name text))
  (print (. pair count))
  (let box (call Box other))
  (print (. box name text))
  (= (. box name) first)
  (print (== (. box name) first))
)

(type + Pair (is Tuple)
  (prop + name Atom)
  (prop + count Int)
)

(type + Box
  (prop + name Atom)
  (fn-do + This _
    (param name _)
  )
)

(fn-do + pick Atom
  (param first Bool)
  (if first
    (return (as "picked first" Atom))
  )
  (return (as "picked second" Atom))
)

(fn-do + same Bool
  (param left Atom)
  (param right Atom)
  (return (== left right))
)
```

```output
true
false
false
true
one
two
true
false
8
picked first
picked second
true
false
one
3
two
true
```

### An optional atom tested, forced, matched, defaulted and held in an object

```brix
(main
  (print (call describe (call find 0)))
  (print (call describe (call find 1)))
  (print (call named (call find 1)))
  (print (call named (call find 0)))
  (print (call held 1))
  (print (call held 0))
  (print (call forced 1))
  (let slot Slot (call Slot))
  (print (== (. slot name) null))
  (= (. slot name) (as "kept" Atom))
  (print (!= (. slot name) null))
  (print (. (!! (. slot name)) text))
  (= (. slot name) null)
  (print (== (. slot name) null))
  (print (call chosen null))
  (print (call chosen (as "given" Atom)))
  (let attempt
    (do-new
      (print (call forced 0))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Slot
  (prop + name (? Atom))
  (prop + count _ (= 2))
)

(fn-do + find (? Atom)
  (param kind Int)
  (if (== kind 0)
    (return null)
  )
  (return (as "found" Atom))
)

(fn-do + describe String
  (param name (? Atom))
  (if name
    (return (str "has " (. name text)))
  )
  (return "none")
)

(fn-do + named String
  (param name (? Atom))
  (return-if "missing" (if (not name)))
  (return (. name text))
)

(fn-do + held String
  (param kind Int)
  (let name (call find kind))
  (match? name
    (case (: atom Atom)
      (return (. atom text))
    )
  )
  (return "nothing")
)

(fn-do + forced String
  (param kind Int)
  (return (. (!! (call find kind)) text))
)

(fn-do + chosen String
  (param name (? Atom))
  (let kept (?? name (as "fallback" Atom)))
  (return (. kept text))
)
```

```output
none
has found
found
missing
found
nothing
found
true
true
kept
true
fallback
given
Optional'core:Atom is null
```

### Optional atoms in a tuple and in the cases of a union

```brix
(main
  (let entry (call Entry (as "key" Atom) null 4))
  (print (. entry key text))
  (print (== (. entry alias) null))
  (print (. entry count))
  (let named (call Entry (as "key" Atom) (as "also" Atom) 5))
  (print (!= (. named alias) null))
  (print (. (!! (. named alias)) text))
  (print (call label entry))
  (print (call label named))
  (let copy named)
  (print (. (!! (. copy alias)) text))
  (print (call size (. Entry size)))
  (print (call text (call .named (as "n" Atom))))
  (print (call text (call .aliased (as "a" Atom) null)))
  (print (call text (call .aliased (as "a" Atom) (as "b" Atom))))
  (print (call text .empty))
  (print (call optional))
  (print (call optional (call find)))
)

(type + Entry (is Tuple)
  (prop + key Atom)
  (prop + alias (? Atom))
  (prop + count Int)
)

(type + Choice (is Union)
  (variant named (tuple (: name Atom)))
  (variant aliased (tuple (: name Atom) (: alias (? Atom))))
  (variant empty)
)

(fn-do + label String
  (param entry Entry)
  (let alias (. entry alias))
  (if alias
    (return (str (. entry key text) " as " (. alias text)))
  )
  (return (. entry key text))
)

(fn-do + size Size
  (param bytes Size)
  (return bytes)
)

(fn-do + text String
  (param choice Choice)
  (return
    (match choice
      (case (call .named name)
        (. name text)
      )
      (case (call .aliased name alias)
        (str (. name text) "/" (. (?? alias name) text))
      )
      (case .empty
        "empty"
      )
    )
  )
)

(fn-do + find (? Atom)
  (return (as "found" Atom))
)

(fn-do + optional String
  (param name (? Atom) (= null))
  (let kept (? Atom) name)
  (if (== kept null)
    (return "none")
  )
  (return (. kept text))
)
```

```output
key
true
4
true
also
key
key as also
also
24
n
a/a
a/b
empty
none
found
```

### An optional atom left out of a call and filling a property

```brix
(main
  (print (call left))
  (print (call left (as "given" Atom)))
  (let slot (call Slot))
  (print (== (. slot name) null))
  (let named (call Slot (as "made" Atom)))
  (print (. (!! (. named name)) text))
)

(type + Slot
  (prop + name (? Atom))
  (fn-do + This _
    (param name (? Atom) (= null))
  )
)

(fn-do + left String
  (param name (? Atom) (= null))
  (if name
    (return (. name text))
  )
  (return "left out")
)
```

```output
left out
given
true
made
```

```brix
(import /core/arena)
(import /core/atom-pool)
(blank)

(main
  (let source (. "xAé漢😀z" codepoints))
  (print (call atom-cycle source))
)

(fn-do + atom-cycle Size
  (param source ([] Char))
  (let arena (call Arena 4096))
  (let atoms (call AtomPool))
  (finally
    (call (. atoms release))
    (call (. arena release))
  )
  (return (call atom-cycle atoms arena source))
)

(fn-do + atom-cycle Size (without gc)
  (param atoms AtomPool)
  (param arena Arena)
  (param source ([] Char))
  (let first (call (. atoms store) "alpha"))
  (let duplicate (call (. atoms store) "alpha"))
  (let mixed (call (. atoms store) source 1 5))
  (let temporary (call (. atoms store) "temporary"))
  (let stored (call (of (. arena allocate) Atom) 2))
  (= ([] stored 0) first)
  (= ([] stored 1) mixed)
  (return (+ (call (. atoms ordinal) duplicate) (call (. atoms ordinal) temporary) (. atoms length) (. ([] stored 1) text length)))
)
```

```output
9
```

```brix crash
(main
  (let atom (as "name" Atom))
  (print (str atom))
)
```

```output
Exception: Operator "++" is not defined for String and Atom
<no file:4>
      (print (str atom))
             ^^^^^^^^^^
```

```brix crash
(main
  (let atom (as "name" Atom))
  (print (== atom "name"))
)
```

```output
Exception: Operator "==" is not defined for Atom and String
<no file:4>
      (print (== atom "name"))
             ^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let atom (as "name" Atom))
  (print (== "name" atom))
)
```

```output
Exception: Operator "==" is not defined for String and Atom
<no file:4>
      (print (== "name" atom))
             ^^^^^^^^^^^^^^^^
```

### An atom and an optional atom chosen by `if` with no default

```brix
(main
  (print (call named (call given true)))
  (print (call named (call given false)))
  (print (call named (call passed true)))
  (print (call named (call passed false)))
)

(fn-do + given (? Atom)
  (param wanted Bool)
  (let atom (as "given" Atom))
  (let found (only-if atom (if wanted)))
  (return found)
)

(fn-do + passed (? Atom)
  (param wanted Bool)
  (let found (only-if (call given true) (if wanted)))
  (return found)
)

(fn-expr + named String
  (param name (? Atom))
  (?? (?. name text) "none")
)
```

```output
given
none
given
none
```
