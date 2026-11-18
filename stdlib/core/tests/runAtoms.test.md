# Atoms

```yumi
/core/atom-pool

. atoms = AtomPool()
do finally
    atoms.release()
. empty = atoms.store("")
. duplicate = atoms.store("")
-- empty == duplicate
-- atoms.owns(empty)
-- empty.text == ""
-- atoms.length
-- Atom.size
```

```output
true
true
true
1
8
```

```yumi
/core/atom-pool

. atoms = AtomPool()
do finally
    atoms.release()
. stored = atoms.store("alpha-beta")
. spanned = atoms.store("alpha-beta" as Byte[], 0, 10)
-- stored == spanned
. tail = atoms.store("x-beta" as Byte[], 2, 6)
-- tail.text
-- tail == atoms.store("beta")
-- atoms.length
```

```output
true
beta
true
2
```

```yumi
/core/arena
/core/array-builder

. arena = Arena()
do finally arena.release()
. first = arena.copy-string("first") as Atom
. second = arena.copy-string("second") as Atom
. atoms = ArrayBuilder'Atom(arena)
atoms.append(first)
atoms[0] = second
-- atoms[0].text
-- arena.owns(atoms[0].address as Pointer)
```

```output
second
true
```

```yumi
/core/atom-pool

. atoms = AtomPool()
do finally
    atoms.release()
. alpha = atoms.store("alpha")
. duplicate = atoms.store("alpha")
-- alpha == duplicate
-- alpha.address == duplicate.address
-- atoms.owns(alpha)
-- alpha.text
-- alpha as String
-- alpha.text[1 .. 3]
-- "atom=${alpha.text}"
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

```yumi
/core/atom-pool

. atoms = AtomPool()
. others = AtomPool()
do finally
    atoms.release()
    others.release()
. codepoints = "xAé漢😀z".codepoints
. mixed = atoms.store(codepoints, 1, 5)
. duplicate = atoms.store("Aé漢😀")
. other = others.store("Aé漢😀")
-- mixed == duplicate
-- mixed != other
-- mixed.text.length
-- mixed.text.size
-- mixed.text[1 .. 2]
-- atoms.length
```

```output
true
true
4
10
é漢
1
```

```yumi
/core/atom-pool

. atoms = AtomPool()
do finally
    atoms.release()
. empty = atoms.store("")
. alpha = atoms.store("alpha")
. beta = atoms.store("beta")
-- atoms.ordinal(empty), atoms.ordinal(alpha), atoms.ordinal(beta)
-- atoms[0].text, atoms[1].text, atoms[2].text
. order = ""
for atom in atoms
    order ++= "[${atom.text}]"
-- order
```

```output
(0, 1, 2)
(, alpha, beta)
[][alpha][beta]
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()
. text = arena.copy-string("arena-atom")
. atom = text as Atom
-- arena.owns(atom.address as Pointer)
-- atom.text
-- atom as String
```

```output
true
arena-atom
arena-atom
```

```yumi
/core/atom-pool

. atoms = AtomPool()
. others = AtomPool()
do finally
    atoms.release()
    others.release()
. attempt = do new
    . alpha = atoms.store("alpha")
    . other-alpha = others.store("alpha")
    . ordinal = atoms.ordinal(other-alpha)
catch failure
    -- "error: $failure"
attempt.wait()
```

```output
error: Atom belongs to another AtomPool or is not live
```

```yumi
/core/atom-pool

. atoms = AtomPool()
do finally
    atoms.release()
. attempt = do new
    . source = "abc".codepoints
    . invalid = atoms.store(source, 2, 1)
catch failure
    -- "error: $failure"
attempt.wait()
```

```output
error: Atom codepoint span is invalid
```

```yumi
/core/atom-pool

. atoms = AtomPool()
atoms.release()
. attempt = do new
    -- atoms.length
catch failure
    -- "error: $failure"
attempt.wait()
```

```output
error: AtomPool Arena is stale or released
```

```yumi
/core/arena
/core/atom-pool

. atoms = AtomPool()
. first = Arena()
. second = Arena()
. first-name = first.copy-string("ArenaAtomTree") as Atom
. second-name = second.copy-string("ArenaAtomTree") as Atom
-- first-name.text
-- first-name != second-name
-- first.owns(first-name.address as Pointer)
-- second.owns(second-name.address as Pointer)
. first-promoted = atoms.store(first-name.text)
. second-promoted = atoms.store(second-name.text)
-- first-promoted == second-promoted
-- atoms.owns(first-promoted)
-- !first.owns(first-promoted.address as Pointer)
-- !second.owns(second-promoted.address as Pointer)
-- first-name != first-promoted
first.release()
second.release()
-- first-promoted.text
atoms.release()
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

```yumi
/core/atom-pool

. atoms = AtomPool()
do finally
    atoms.release()
. first = atoms.store("abca")
. second = atoms.store("axya")
. third = atoms.store("aZZa")
. source = "xabca|axyaz".codepoints
-- first != second && second != third
-- atoms.store(source, 1, 5) == first
-- atoms.store(source, 6, 10) == second
-- atoms.length
```

```output
true
true
true
3
```

```yumi
/core/atom-pool

. atoms = AtomPool()
do finally
    atoms.release()
. first = atoms.store("a00z")
for index in 1 .. < 24
    atoms.store("a${index / 10}${index % 10}z")
-- atoms.length
-- atoms.store("a00z") == first
-- atoms.ordinal(atoms.store("a23z"))
```

```output
24
true
23
```

```yumi
/core/atom-pool

. atoms = AtomPool()
do finally
    atoms.release()
. first = atoms.store("éa😀")
. second = atoms.store("éb😀")
. source = "xéa😀|éb😀z".codepoints
-- first != second
-- atoms.store(source, 1, 4) == first
-- atoms.store(source, 5, 8) == second
-- atoms.length
```

```output
true
true
true
2
```

```yumi
/core/atom-pool

. atoms = AtomPool()
do finally
    atoms.release()
. first = atoms.store("a100000z")
for index in 100001 .. < 160000
    atoms.store("a${index}z")
-- atoms.length
-- atoms.store("a100000z") == first
-- atoms.ordinal(atoms.store("a159999z"))
```

```output
60000
true
59999
```

```yumi
-- selected-name(.named("found" as Atom))
-- selected-name(.missing)
-- assigned-name(.named("found" as Atom))
-- assigned-name(.missing)
-- continuing-name(.named("found" as Atom))
-- continuing-name(.missing)

━━━━━━━━━━━━━━━━━━━━
+ AtomChoice is Union

| named(value Atom)
| missing

━━━━━━━━━━━━━━━━━━━━
+ do selected-name String
    - choice AtomChoice

. name Atom? = null
match? choice
    | .named(value)
        name = value
        -- name.text
if !name
    = "missing"
= name.text

━━━━━━━━━━━━━━━━━━━━
+ do assigned-name String
    - choice AtomChoice

. name Atom? = null
match choice
    | .named(value)
        name = value
    ??
        name = "fallback" as Atom
= name.text

━━━━━━━━━━━━━━━━━━━━
+ do continuing-name String
    - choice AtomChoice

. name Atom? = null
match choice
    | .named(value)
        name = value
    ??
        = "missing"
= name.text
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

```yumi
. first = "one" as Atom
. again = first
. other = "two" as Atom
-- first == again
-- first != again
-- first == other
-- first != other
-- first.text
-- other as String
-- first.address == again.address
-- first.address == other.address
-- Atom.size
-- pick(true).text
-- pick(false).text
-- same(first, again)
-- same(first, other)
. pair = Pair(first, 3)
-- pair.name.text
-- pair.count
. box = Box(other)
-- box.name.text
box.name = first
-- box.name == first

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ name Atom
+ count Int

━━━━━━━━━━━━━━━━━━━━
+ Box

+ name Atom

────────────────────
+ This
    - name

━━━━━━━━━━━━━━━━━━━━
+ do pick Atom
    - first Bool

if first
    = "picked first" as Atom
= "picked second" as Atom

━━━━━━━━━━━━━━━━━━━━
+ do same Bool
    - left Atom
    - right Atom

= left == right
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

```yumi
-- describe(find(0))
-- describe(find(1))
-- named(find(1))
-- named(find(0))
-- held(1)
-- held(0)
-- forced(1)
. slot Slot = Slot()
-- slot.name == null
slot.name = "kept" as Atom
-- slot.name != null
-- slot.name!.text
slot.name = null
-- slot.name == null
-- chosen(null)
-- chosen("given" as Atom)
. attempt = do new
    -- forced(0)
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Slot

+ name Atom?
+ count = 2

━━━━━━━━━━━━━━━━━━━━
+ do find Atom?
    - kind Int

if kind == 0
    = null
= "found" as Atom

━━━━━━━━━━━━━━━━━━━━
+ do describe String
    - name Atom?

if name
    = "has ${name.text}"
= "none"

━━━━━━━━━━━━━━━━━━━━
+ do named String
    - name Atom?

=? "missing" if !name
= name.text

━━━━━━━━━━━━━━━━━━━━
+ do held String
    - kind Int

. name = find(kind)
match? name
    | atom Atom
        = atom.text
= "nothing"

━━━━━━━━━━━━━━━━━━━━
+ do forced String
    - kind Int

= find(kind)!.text

━━━━━━━━━━━━━━━━━━━━
+ do chosen String
    - name Atom?

. kept = name ?? ("fallback" as Atom)
= kept.text
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

```yumi
. entry = Entry("key" as Atom, null, 4)
-- entry.key.text
-- entry.alias == null
-- entry.count
. named = Entry("key" as Atom, "also" as Atom, 5)
-- named.alias != null
-- named.alias!.text
-- label(entry)
-- label(named)
. copy = named
-- copy.alias!.text
-- size(Entry.size)
-- text(.named("n" as Atom))
-- text(.aliased("a" as Atom, null))
-- text(.aliased("a" as Atom, "b" as Atom))
-- text(.empty)
-- optional()
-- optional(find())

━━━━━━━━━━━━━━━━━━━━
+ Entry is Tuple

+ key Atom
+ alias Atom?
+ count Int

━━━━━━━━━━━━━━━━━━━━
+ Choice is Union

| named(name Atom)
| aliased(name Atom, alias Atom?)
| empty

━━━━━━━━━━━━━━━━━━━━
+ do label String
    - entry Entry

. alias = entry.alias
if alias
    = "${entry.key.text} as ${alias.text}"
= entry.key.text

━━━━━━━━━━━━━━━━━━━━
+ do size Size
    - bytes Size

= bytes

━━━━━━━━━━━━━━━━━━━━
+ do text String
    - choice Choice

= match choice
    | .named(name)
        name.text
    | .aliased(name, alias)
        "${name.text}/${(alias ?? name).text}"
    | .empty
        "empty"

━━━━━━━━━━━━━━━━━━━━
+ do find Atom?

= "found" as Atom

━━━━━━━━━━━━━━━━━━━━
+ do optional String
    - name Atom? = null

. kept Atom? = name
if kept == null
    = "none"
= kept.text
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

```yumi
-- left()
-- left("given" as Atom)
. slot = Slot()
-- slot.name == null
. named = Slot("made" as Atom)
-- named.name!.text

━━━━━━━━━━━━━━━━━━━━
+ Slot

+ name Atom?

────────────────────
+ This
    - name Atom? = null

━━━━━━━━━━━━━━━━━━━━
+ do left String
    - name Atom? = null

if name
    = name.text
= "left out"
```

```output
left out
given
true
made
```

```yumi
/core/arena
/core/atom-pool

. source = "xAé漢😀z".codepoints
-- atom-cycle(source)

━━━━━━━━━━━━━━━━━━━━
+ do atom-cycle Size
    - source Char[]

. arena = Arena(4096)
. atoms = AtomPool()
do finally
    atoms.release()
    arena.release()
= atom-cycle(atoms, arena, source)

━━━━━━━━━━━━━━━━━━━━
+ do atom-cycle Size without gc
    - atoms AtomPool
    - arena Arena
    - source Char[]

. first = atoms.store("alpha")
. duplicate = atoms.store("alpha")
. mixed = atoms.store(source, 1, 5)
. temporary = atoms.store("temporary")
. stored = arena.allocate'Atom(2)
stored[0] = first
stored[1] = mixed
= atoms.ordinal(duplicate) + atoms.ordinal(temporary) + atoms.length + stored[1].text.length
```

```output
9
```

```yumi crash
. atom = "name" as Atom
-- "$atom"
```

```output
Exception: Operator "++" is not defined for String and Atom
<no file:3>
    -- "$atom"
       ^^^^^^^
```

```yumi crash
. atom = "name" as Atom
-- atom == "name"
```

```output
Exception: Operator "==" is not defined for Atom and String
<no file:3>
    -- atom == "name"
       ^^^^^^^^^^^^^^
```

```yumi crash
. atom = "name" as Atom
-- "name" == atom
```

```output
Exception: Operator "==" is not defined for String and Atom
<no file:3>
    -- "name" == atom
       ^^^^^^^^^^^^^^
```

### An atom and an optional atom chosen by `if` with no default

```yumi
-- named(given(true))
-- named(given(false))
-- named(passed(true))
-- named(passed(false))

━━━━━━━━━━━━━━━━━━━━
+ do given Atom?
    - wanted Bool

. atom = "given" as Atom
. found = atom if wanted
= found

━━━━━━━━━━━━━━━━━━━━
+ do passed Atom?
    - wanted Bool

. found = given(true) if wanted
= found

━━━━━━━━━━━━━━━━━━━━
+ named String
    - name Atom?

name?.text ?? "none"
```

```output
given
none
given
none
```
