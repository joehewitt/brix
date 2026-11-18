# Imports

```yumi crash
/foo
```

```output
Exception: Module "foo" not found
<no file:2>
    /foo
    ^^^^
```

```yumi
./lib/foo

-- foo()
```

```yumi file=lib/foo.yumi

-- "fooey"

━━━━━━━━━━━━━━━━━━━━
+ foo

24

```

```output
fooey
24
```

```yumi crash
./lib/foo

-- foo()
```

```yumi file=lib/foo.yumi

━━━━━━━━━━━━━━━━━━━━
_ foo

24

```

```output
Exception: Function "foo" is private, declared with _, so only its own module can call it
<no file:4>
    -- foo()
       ^^^^^
```

```yumi
./lib/foo

-- foo()
```

```yumi file=lib/foo.yumi

━━━━━━━━━━━━━━━━━━━━
_ bar

24

━━━━━━━━━━━━━━━━━━━━
+ foo

bar()

```

```output
24
```

```yumi
./lib/foo
./lib/foo/bar

-- foo()
-- bar()
```

```yumi file=lib/foo/foo.yumi

━━━━━━━━━━━━━━━━━━━━
+ foo

24

```

```yumi file=lib/foo/bar.yumi

━━━━━━━━━━━━━━━━━━━━
+ bar

42

```

```output
24
42
```

### A folder without a module of its own name leaves the module beside it importable

```yumi
./lib/foo

-- foo()
```

```yumi file=lib/foo.yumi

━━━━━━━━━━━━━━━━━━━━
+ foo

24

```

```yumi file=lib/foo/bar.yumi

━━━━━━━━━━━━━━━━━━━━
+ bar

42

```

```output
24
```

### A module whose name starts with `../` is found beside the folder of the module that imports it, and is the module another names by its path from there.

```yumi
./lib
./sub/uses

bump()
-- seen()
```

```yumi file=lib.yumi

+ count = 0

━━━━━━━━━━━━━━━━━━━━
+ do bump

count += 1

```

```yumi file=sub/uses.yumi

../lib

━━━━━━━━━━━━━━━━━━━━
+ do seen Int

= count

```

```output
1
```

### A folder's own module wins over the module beside the folder

```yumi
./lib/foo

-- foo()
```

```yumi file=lib/foo.yumi

━━━━━━━━━━━━━━━━━━━━
+ foo

1

```

```yumi file=lib/foo/foo.yumi

━━━━━━━━━━━━━━━━━━━━
+ foo

2

```

```output
2
```

```yumi
./lib/foo

-- foo()

━━━━━━━━━━━━━━━━━━━━
+ bar

1
```

```yumi file=lib/foo.yumi

━━━━━━━━━━━━━━━━━━━━
+ bar

18

━━━━━━━━━━━━━━━━━━━━
+ foo

bar()

```

```output
18
```

### Global variables in another module

```yumi
./lib/foo

foo()
```

```yumi file=lib/foo.yumi

_ a = 10

━━━━━━━━━━━━━━━━━━━━
+ do foo

-- a

```

```output
10
```

### Imported module main after core globals initialize

```yumi
./lib/foo
```

```yumi file=lib/foo.yumi

-- true

```

```output
true
```

### Public global variables from another module

```yumi
./lib/foo

-- pookie
```

```yumi file=lib/foo.yumi

+ pookie = 10

```

```output
10
```

### Private global variables from another module

```yumi crash
./lib/foo

-- dookie
```

```yumi file=lib/foo.yumi

_ dookie = 5

```

```output
Exception: Name "dookie" not found
<no file:4>
    -- dookie
       ^^^^^^
```

### Assign public global variables from another module

```yumi
./lib/foo

pookie = 10
foo()
```

```yumi file=lib/foo.yumi

+ pookie = 5

━━━━━━━━━━━━━━━━━━━━
+ do foo

-- pookie

```

```output
10
```

### Assign private global variables from another module

```yumi
./lib/foo

. pookie = 10
foo()
-- pookie
```

```yumi file=lib/foo.yumi

_ pookie = 5

━━━━━━━━━━━━━━━━━━━━
+ do foo

-- pookie

```

```output
5
10
```

### Circular import

```yumi
./lib/foo

-- "init"
-- foo()
```

```yumi file=lib/foo.yumi

./bar

-- "init foo"
-- bar()

━━━━━━━━━━━━━━━━━━━━
+ foo

"foo"

```

```yumi file=lib/bar.yumi

./foo

-- "init bar"
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ bar

"bar"

```

```output
init bar
foo
init foo
bar
init
foo
```

```yumi
./first
./second

. value = Choice("latest")
-- value.text
```

```yumi file=first.yumi

━━━━━━━━━━━━━━━━━━━━
+ Choice

+ number = 7

```

```yumi file=second.yumi

━━━━━━━━━━━━━━━━━━━━
+ Choice

+ text String

────────────────────
+ This
    - text String

```

```output
latest
```

### Private type members stay usable inside their declaring module

```yumi
./lib/box

. box = Box()
-- box.label
-- box.reveal()
box.label = "renamed"
-- box.reveal()
-- make-box("made").reveal()
. named = <Box
    label = "attribute"
-- named.label
```

```yumi file=lib/box.yumi

━━━━━━━━━━━━━━━━━━━━
+ Box

_ secret String
+ label String
_ kind => "private kind"

────────────────────
+ This

this.secret = "hidden"
this.label = "shown"

────────────────────
_ This
    - value String

this.secret = value
this.label = value

────────────────────
_ hidden

"$secret method"

────────────────────
+ reveal

"$label/${hidden()}/$kind"

━━━━━━━━━━━━━━━━━━━━
+ do make-box Box
    - value String

. box = Box(value)
box.secret = "$value!"
= box

```

```output
shown
shown/hidden method/private kind
renamed/hidden method/private kind
made/made! method/private kind
attribute
```

### Private fields cannot be read from another module

```yumi crash
./lib/box

. box = Box()
-- box.secret
```

```yumi file=lib/box.yumi

━━━━━━━━━━━━━━━━━━━━
+ Box

_ secret String
+ label String
_ kind => "private kind"

────────────────────
+ This

this.secret = "hidden"
this.label = "shown"

────────────────────
_ This
    - value String

this.secret = value
this.label = value

────────────────────
_ hidden

"$secret method"

────────────────────
+ reveal

"$label/${hidden()}/$kind"

━━━━━━━━━━━━━━━━━━━━
+ do make-box Box
    - value String

. box = Box(value)
box.secret = "$value!"
= box

```

```output
Exception: Property "secret" is private
<no file:5>
    -- box.secret
       ^^^^^^^^^^
```

### Private fields cannot be written from another module

```yumi crash
./lib/box

. box = Box()
box.secret = "leak"
```

```yumi file=lib/box.yumi

━━━━━━━━━━━━━━━━━━━━
+ Box

_ secret String
+ label String
_ kind => "private kind"

────────────────────
+ This

this.secret = "hidden"
this.label = "shown"

────────────────────
_ This
    - value String

this.secret = value
this.label = value

────────────────────
_ hidden

"$secret method"

────────────────────
+ reveal

"$label/${hidden()}/$kind"

━━━━━━━━━━━━━━━━━━━━
+ do make-box Box
    - value String

. box = Box(value)
box.secret = "$value!"
= box

```

```output
Exception: Property "secret" is private
<no file:5>
    box.secret = "leak"
    ^^^^^^^^^^
```

### Private lazy properties cannot be read from another module

```yumi crash
./lib/box

. box = Box()
-- box.kind
```

```yumi file=lib/box.yumi

━━━━━━━━━━━━━━━━━━━━
+ Box

_ secret String
+ label String
_ kind => "private kind"

────────────────────
+ This

this.secret = "hidden"
this.label = "shown"

────────────────────
_ This
    - value String

this.secret = value
this.label = value

────────────────────
_ hidden

"$secret method"

────────────────────
+ reveal

"$label/${hidden()}/$kind"

━━━━━━━━━━━━━━━━━━━━
+ do make-box Box
    - value String

. box = Box(value)
box.secret = "$value!"
= box

```

```output
Exception: Property "kind" is private
<no file:5>
    -- box.kind
       ^^^^^^^^
```

### Private methods cannot be called from another module

```yumi crash
./lib/box

. box = Box()
-- box.hidden()
```

```yumi file=lib/box.yumi

━━━━━━━━━━━━━━━━━━━━
+ Box

_ secret String
+ label String
_ kind => "private kind"

────────────────────
+ This

this.secret = "hidden"
this.label = "shown"

────────────────────
_ This
    - value String

this.secret = value
this.label = value

────────────────────
_ hidden

"$secret method"

────────────────────
+ reveal

"$label/${hidden()}/$kind"

━━━━━━━━━━━━━━━━━━━━
+ do make-box Box
    - value String

. box = Box(value)
box.secret = "$value!"
= box

```

```output
Exception: Method "hidden" of Box is private, declared with _, so only its own module can call it
<no file:5>
    -- box.hidden()
       ^^^^^^^^^^^^
```

### Private constructors cannot be called from another module

```yumi crash
./lib/box

-- Box("direct").label
```

```yumi file=lib/box.yumi

━━━━━━━━━━━━━━━━━━━━
+ Box

_ secret String
+ label String
_ kind => "private kind"

────────────────────
+ This

this.secret = "hidden"
this.label = "shown"

────────────────────
_ This
    - value String

this.secret = value
this.label = value

────────────────────
_ hidden

"$secret method"

────────────────────
+ reveal

"$label/${hidden()}/$kind"

━━━━━━━━━━━━━━━━━━━━
+ do make-box Box
    - value String

. box = Box(value)
box.secret = "$value!"
= box

```

```output
Exception: No constructor of Box accepts (String); its declared constructors are "Box()"; "Box(value String)" is private to its module
<no file:4>
    -- Box("direct").label
       ^^^
```

### Private fields cannot be set as constructor properties from another module

```yumi crash
./lib/box

. box = <Box
    secret = "leak"
-- box.label
```

```yumi file=lib/box.yumi

━━━━━━━━━━━━━━━━━━━━
+ Box

_ secret String
+ label String
_ kind => "private kind"

────────────────────
+ This

this.secret = "hidden"
this.label = "shown"

────────────────────
_ This
    - value String

this.secret = value
this.label = value

────────────────────
_ hidden

"$secret method"

────────────────────
+ reveal

"$label/${hidden()}/$kind"

━━━━━━━━━━━━━━━━━━━━
+ do make-box Box
    - value String

. box = Box(value)
box.secret = "$value!"
= box

```

```output
Exception: Property "secret" not found
<no file:5>
        secret = "leak"
        ^^^^^^
```

### Subclasses in another module cannot see private base members

```yumi crash
./lib/box

. crate = Crate()
-- crate.peek()

━━━━━━━━━━━━━━━━━━━━
+ Crate is Box

────────────────────
+ peek

secret
```

```yumi file=lib/box.yumi

━━━━━━━━━━━━━━━━━━━━
+ Box

_ secret String
+ label String
_ kind => "private kind"

────────────────────
+ This

this.secret = "hidden"
this.label = "shown"

────────────────────
_ This
    - value String

this.secret = value
this.label = value

────────────────────
_ hidden

"$secret method"

────────────────────
+ reveal

"$label/${hidden()}/$kind"

━━━━━━━━━━━━━━━━━━━━
+ do make-box Box
    - value String

. box = Box(value)
box.secret = "$value!"
= box

```

```output
Exception: Name "secret" not found
<no file:13>
    secret
    ^^^^^^
```

### Extensions in another module cannot see private members

```yumi crash
./lib/box

. box = Box()
-- box.peek()

━━━━━━━━━━━━━━━━━━━━
+ extend Box

────────────────────
+ peek

hidden()
```

```yumi file=lib/box.yumi

━━━━━━━━━━━━━━━━━━━━
+ Box

_ secret String
+ label String
_ kind => "private kind"

────────────────────
+ This

this.secret = "hidden"
this.label = "shown"

────────────────────
_ This
    - value String

this.secret = value
this.label = value

────────────────────
_ hidden

"$secret method"

────────────────────
+ reveal

"$label/${hidden()}/$kind"

━━━━━━━━━━━━━━━━━━━━
+ do make-box Box
    - value String

. box = Box(value)
box.secret = "$value!"
= box

```

```output
Exception: Function "hidden" not found
<no file:13>
    hidden()
    ^^^^^^^^
```

### Private overloads are skipped from another module

```yumi
./lib/scale

. scale = Scale()
-- scale.inside(2)
-- scale.apply(2)
-- scale.inside(2)
-- scale.assisted()
```

```yumi file=lib/scale.yumi

━━━━━━━━━━━━━━━━━━━━
+ Scale

────────────────────
+ apply
    - value Float

"public float"

────────────────────
_ apply
    - value Int

"private int"

────────────────────
+ inside
    - value Int

apply(value)

━━━━━━━━━━━━━━━━━━━━
+ extend Scale

────────────────────
_ helper

"private extension helper"

────────────────────
+ assisted

helper()

```

```output
private int
public float
private int
private extension helper
```

### Private extension members cannot be called from another module

```yumi crash
./lib/scale

. scale = Scale()
-- scale.helper()
```

```yumi file=lib/scale.yumi

━━━━━━━━━━━━━━━━━━━━
+ Scale

────────────────────
+ apply
    - value Float

"public float"

────────────────────
_ apply
    - value Int

"private int"

────────────────────
+ inside
    - value Int

apply(value)

━━━━━━━━━━━━━━━━━━━━
+ extend Scale

────────────────────
_ helper

"private extension helper"

────────────────────
+ assisted

helper()

```

```output
Exception: Method "helper" of Scale is private, declared with _, so only its own module can call it
<no file:5>
    -- scale.helper()
       ^^^^^^^^^^^^^^
```

### Private fields stay usable through table rows inside their declaring module

```yumi
/core/table
./lib/particle

. table = Table'Particle(1)
-- table[0].id
-- total-mass(table)
```

```yumi file=lib/particle.yumi

/core/table

━━━━━━━━━━━━━━━━━━━━
+ Particle is Struct

+ id I32
_ mass Float

━━━━━━━━━━━━━━━━━━━━
+ do total-mass Float
    - table Table'Particle

table[0].mass = 2.5
= table[0].mass

```

```output
0
2.5
```

### Private fields cannot be read through table rows from another module

```yumi crash
/core/table
./lib/particle

. table = Table'Particle(1)
-- table[0].mass
```

```yumi file=lib/particle.yumi

/core/table

━━━━━━━━━━━━━━━━━━━━
+ Particle is Struct

+ id I32
_ mass Float

━━━━━━━━━━━━━━━━━━━━
+ do total-mass Float
    - table Table'Particle

table[0].mass = 2.5
= table[0].mass

```

```output
Exception: Property "mass" is private
<no file:6>
    -- table[0].mass
       ^^^^^^^^^^^^^
```

### Private setters stay usable inside their declaring module

```yumi
./lib/dial

. dial = Dial()
dial.bump()
-- dial.level
```

```yumi file=lib/dial.yumi

━━━━━━━━━━━━━━━━━━━━
+ Dial

+ log = ""

────────────────────
+ get level

log

────────────────────
_ do level = value String

log = value

────────────────────
+ do bump

level = "bumped"

```

```output
bumped
```

### Private setters cannot be called from another module

```yumi crash
./lib/dial

. dial = Dial()
dial.level = "leak"
```

```yumi file=lib/dial.yumi

━━━━━━━━━━━━━━━━━━━━
+ Dial

+ log = ""

────────────────────
+ get level

log

────────────────────
_ do level = value String

log = value

────────────────────
+ do bump

level = "bumped"

```

```output
Exception: Property "level" is private
<no file:5>
    dial.level = "leak"
    ^^^^^^^^^^
```

### Private base methods cannot be called through super from another module

```yumi crash
./lib/base

-- Derived().describe()

━━━━━━━━━━━━━━━━━━━━
+ Derived is Base

────────────────────
+ describe

"derived ${super.describe()}"
```

```yumi file=lib/base.yumi

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
_ describe

"base"

```

```output
Exception: Method "describe" of Base is private, declared with _, so only its own module can call it
<no file:12>
    "derived ${super.describe()}"
               ^^^^^^^^^^^^^^^^
```

### Private tuple fields cannot be read from another module

```yumi crash
./lib/pair

. pair = Pair(1, 2)
-- pair.shown
-- pair.total
-- pair.hidden
```

```yumi file=lib/pair.yumi

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ shown Int
_ hidden Int

────────────────────
+ get total

shown + hidden

```

```output
Exception: Property "hidden" is private
<no file:7>
    -- pair.hidden
       ^^^^^^^^^^^
```

### The main of an imported module runs before the module that imports it, and its functions are called by name.

```yumi
./lib/util

-- "main"
-- util-add(1, 2)
shout("x")
```

```yumi file=lib/util.yumi

-- "util main"

━━━━━━━━━━━━━━━━━━━━
+ util-add Int
    - a Int
    - b Int

a + b

━━━━━━━━━━━━━━━━━━━━
+ do shout
    - s String

-- s ++ "!"

```

```output
util main
main
3
x!
```

```yumi
./lib/a
./lib/b

-- "entry"
```

```yumi file=lib/a.yumi

./c

-- "a"

```

```yumi file=lib/b.yumi

-- "b"

```

```yumi file=lib/c.yumi

-- "c"

```

```output
c
a
b
entry
```

```yumi
./lib/b
./lib/a

-- "entry"
```

```yumi file=lib/a.yumi

./c

-- "a"

```

```yumi file=lib/b.yumi

-- "b"

```

```yumi file=lib/c.yumi

-- "c"

```

```output
b
c
a
entry
```

### A name two imported modules declare is found through the import written last, and the modules it exports, before the one written first.

```yumi
./lib/near
./lib/far

-- shade
```

```yumi file=lib/near.yumi

+ shade = "near"

```

```yumi file=lib/far.yumi

+ ./tone

```

```yumi file=lib/tone.yumi

+ shade = "tone"

```

```output
tone
```

### A type written by name that two imported modules declare is the one of the import written first, and so is one a function's argument is written with.

```yumi
./lib/narrow
./lib/wide

. top = Id.max
. given Id = 200
-- "${top as U32} ${given as U32} ${pick(3 as Id)}"

━━━━━━━━━━━━━━━━━━━━
_ do pick String
    - id Id

= "narrow ${id as U32}"
```

```yumi file=lib/narrow.yumi

━━━━━━━━━━━━━━━━━━━━
+ Id is U8

```

```yumi file=lib/wide.yumi

━━━━━━━━━━━━━━━━━━━━
+ Id is U16

━━━━━━━━━━━━━━━━━━━━
+ do pick String
    - id Id

= "wide ${id as U32}"

```

```output
255 200 narrow 3
```

### A class derives only from a base its own module can name: a base declared in a module that only an imported module imports is not found, though a property of the imported module holds that base.

```yumi compile-error
./lib/shelf

Shelf().item = Book()

━━━━━━━━━━━━━━━━━━━━
+ Book is Item
```

```yumi file=lib/shelf.yumi

./item

━━━━━━━━━━━━━━━━━━━━
+ Shelf

+ item Item? = null

```

```yumi file=lib/item.yumi

━━━━━━━━━━━━━━━━━━━━
+ Item

```

```output
Exception: Type "Item" not found; it is declared in ./lib/item, which this module does not import. Add the import ./lib/item
<no file:7>
    + Book is Item
              ^^^^
```

### A module imported by two paths that name one file, one through `..`, is one module, whose types are the same type.

```yumi
/core/arena
/core/../core/arena

. arena = use Arena()
-- "ok ${make(arena)}"

━━━━━━━━━━━━━━━━━━━━
_ make Int
    - arena Arena

3
```

```output
ok 3
```

### A folder's own module imported by its file's path is the module the folder's name imports, so the functions the compiler supplies for `/math` are found through either name.

```yumi
/math/math

. x = -0.5
-- abs(x)
-- sqrt(9.0)
```

```output
0.5
3
```

### A C header under the `c` root is found among the system headers

```yumi
/c/unistd.h

-- C:getpid() > 0
```

```output
true
```

### A C header that is not there is a module not found at its import

```yumi crash
/no-such-header.h

-- 1
```

```output
Exception: Module "no-such-header.h" not found
<no file:2>
    /no-such-header.h
    ^^^^^^^^^^^^^^^^^
```

### A module named for a platform is part of that platform's builds. Importing one named for another platform imports nothing, and its file is not read.

```yumi
./lib/input.macos
./lib/input.ios
./lib/look.apple

install-input()
-- look()
```

```yumi file=lib/input.macos.yumi

━━━━━━━━━━━━━━━━━━━━
+ do install-input

-- "macos input"
```

```yumi file=lib/input.ios.yumi

/no-such-module

━━━━━━━━━━━━━━━━━━━━
+ do install-input

-- "ios input"
```

```yumi file=lib/look.apple.yumi

━━━━━━━━━━━━━━━━━━━━
+ look String

"apple look"
```

```output
macos input
apple look
```

### A function that only a module for another platform declares is not found, and the error names that module

```yumi crash
./lib/input.ios

install-input()
```

```yumi file=lib/input.ios.yumi

━━━━━━━━━━━━━━━━━━━━
+ do install-input

-- "ios input"
```

```output
Exception: Function "install-input" not found; it is only defined in input.ios.yumi, which isn't part of macos builds
<no file:4>
    install-input()
    ^^^^^^^^^^^^^^^
```

### A module name that ends in something other than a platform is an error

```yumi crash
./lib/input.windows

-- 1
```

```yumi file=lib/input.windows.yumi

-- 2
```

```output
Exception: Module "./lib/input.windows" ends in ".windows", which isn't a platform
<no file:2>
    ./lib/input.windows
    ^^^^^^^^^^^^^^^^^^^
```

### A module for one platform can't import a module for a platform it is never built with

```yumi crash
./lib/keys.macos

-- 1
```

```yumi file=lib/keys.macos.yumi

./touch.ios
```

```yumi file=lib/touch.ios.yumi

-- 2
```

```output
Exception: Module "./touch.ios" is never part of the same build as keys.macos.yumi
<lib/keys.macos.yumi:2>
    ./touch.ios
    ^^^^^^^^^^^
```

### A file whose name starts with _ is private to its folder: modules in that folder and in the folders inside it import it

```yumi
./lib/shelf
./lib/deep/stand

-- "${shelf-tool()} ${stand-tool()}"
```

```yumi file=lib/_tool.yumi

━━━━━━━━━━━━━━━━━━━━
+ tool-name String

"tool"

```

```yumi file=lib/shelf.yumi

./tool

━━━━━━━━━━━━━━━━━━━━
+ shelf-tool String

tool-name()

```

```yumi file=lib/deep/stand.yumi

./../tool

━━━━━━━━━━━━━━━━━━━━
+ stand-tool String

tool-name()

```

```output
tool tool
```

### A module outside the folder of a file whose name starts with _ can't import it, even from the folder around it

```yumi compile-error
./top

-- 1
```

```yumi file=top.yumi

./lib/tool

```

```yumi file=lib/_tool.yumi

-- 2
```

```output
Exception: Module "./lib/tool" is private to lib/, so only modules there can import it
<top.yumi:2>
    ./lib/tool
    ^^^^^^^^^^
```
