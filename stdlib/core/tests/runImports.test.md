# Imports

```brix crash
(import /foo)
```

```output
Exception: Module "foo" not found
<no file:2>
    (import /foo)
    ^^^^^^^^^^^^^
```

```brix
(import ./lib/foo)
(blank)

(main
  (print (call foo))
)
```

```brix file=lib/foo.bx
(main
  (print "fooey")
)

(fn-expr + foo _
  24
)
```

```output
fooey
24
```

```brix crash
(import ./lib/foo)
(blank)

(main
  (print (call foo))
)
```

```brix file=lib/foo.bx
(fn-expr _ foo _
  24
)
```

```output
Exception: Function "foo" is private, declared with _, so only its own module can call it
<no file:6>
      (print (call foo))
             ^^^^^^^^^^
```

```brix
(import ./lib/foo)
(blank)

(main
  (print (call foo))
)
```

```brix file=lib/foo.bx
(fn-expr _ bar _
  24
)

(fn-expr + foo _
  (call bar)
)
```

```output
24
```

```brix
(import ./lib/foo)
(import ./lib/foo/bar)
(blank)

(main
  (print (call foo))
  (print (call bar))
)
```

```brix file=lib/foo/foo.bx
(fn-expr + foo _
  24
)
```

```brix file=lib/foo/bar.bx
(fn-expr + bar _
  42
)
```

```output
24
42
```

### A folder without a module of its own name leaves the module beside it importable

```brix
(import ./lib/foo)
(blank)

(main
  (print (call foo))
)
```

```brix file=lib/foo.bx
(fn-expr + foo _
  24
)
```

```brix file=lib/foo/bar.bx
(fn-expr + bar _
  42
)
```

```output
24
```

### A module whose name starts with `../` is found beside the folder of the module that imports it, and is the module another names by its path from there.

```brix
(import ./lib)
(import ./sub/uses)
(blank)

(main
  (call bump)
  (print (call seen))
)
```

```brix file=lib.bx
(prop + count _ (= 0))

(fn-do + bump _
  (+= count 1)
)
```

```brix file=sub/uses.bx
(import ../lib)

(fn-do + seen Int
  (return count)
)
```

```output
1
```

### A folder's own module wins over the module beside the folder

```brix
(import ./lib/foo)
(blank)

(main
  (print (call foo))
)
```

```brix file=lib/foo.bx
(fn-expr + foo _
  1
)
```

```brix file=lib/foo/foo.bx
(fn-expr + foo _
  2
)
```

```output
2
```

```brix
(import ./lib/foo)
(blank)

(main
  (print (call foo))
)

(fn-expr + bar _
  1
)
```

```brix file=lib/foo.bx
(fn-expr + bar _
  18
)

(fn-expr + foo _
  (call bar)
)
```

```output
18
```

### Global variables in another module

```brix
(import ./lib/foo)
(blank)

(main
  (call foo)
)
```

```brix file=lib/foo.bx
(prop _ a _ (= 10))

(fn-do + foo _
  (print a)
)
```

```output
10
```

### Imported module main after core globals initialize

```brix
(import ./lib/foo)
```

```brix file=lib/foo.bx
(main
  (print true)
)
```

```output
true
```

### Public global variables from another module

```brix
(import ./lib/foo)
(blank)

(main
  (print pookie)
)
```

```brix file=lib/foo.bx
(prop + pookie _ (= 10))
```

```output
10
```

### Private global variables from another module

```brix crash
(import ./lib/foo)
(blank)

(main
  (print dookie)
)
```

```brix file=lib/foo.bx
(prop _ dookie _ (= 5))
```

```output
Exception: Name "dookie" not found
<no file:6>
      (print dookie)
             ^^^^^^
```

### Assign public global variables from another module

```brix
(import ./lib/foo)
(blank)

(main
  (= pookie 10)
  (call foo)
)
```

```brix file=lib/foo.bx
(prop + pookie _ (= 5))

(fn-do + foo _
  (print pookie)
)
```

```output
10
```

### Assign private global variables from another module

```brix
(import ./lib/foo)
(blank)

(main
  (let pookie 10)
  (call foo)
  (print pookie)
)
```

```brix file=lib/foo.bx
(prop _ pookie _ (= 5))

(fn-do + foo _
  (print pookie)
)
```

```output
5
10
```

### Circular import

```brix
(import ./lib/foo)
(blank)

(main
  (print "init")
  (print (call foo))
)
```

```brix file=lib/foo.bx
(import ./bar)
(blank)

(main
  (print "init foo")
  (print (call bar))
)

(fn-expr + foo _
  "foo"
)
```

```brix file=lib/bar.bx
(import ./foo)
(blank)

(main
  (print "init bar")
  (print (call foo))
)

(fn-expr + bar _
  "bar"
)
```

```output
init bar
foo
init foo
bar
init
foo
```

```brix
(import ./first)
(import ./second)
(blank)

(main
  (let value (call Choice "latest"))
  (print (. value text))
)
```

```brix file=first.bx
(type + Choice
  (prop + number _ (= 7))
)
```

```brix file=second.bx
(type + Choice
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)
```

```output
latest
```

### Private type members stay usable inside their declaring module

```brix
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (print (. box label))
  (print (call (. box reveal)))
  (= (. box label) "renamed")
  (print (call (. box reveal)))
  (print (call (. (call make-box "made") reveal)))
  (let named
    (<> Box
      (= label "attribute")
    )
  )
  (print (. named label))
)
```

```brix file=lib/box.bx
(type + Box
  (prop _ secret String)
  (prop + label String)
  (prop _ kind _ (=> "private kind"))
  (fn-do + This _
    (= (. this secret) "hidden")
    (= (. this label) "shown")
  )
  (fn-do _ This _
    (param value String)
    (= (. this secret) value)
    (= (. this label) value)
  )
  (fn-expr _ hidden _
    (str secret " method")
  )
  (fn-expr + reveal _
    (str label "/" (call hidden) "/" kind)
  )
)

(fn-do + make-box Box
  (param value String)
  (let box (call Box value))
  (= (. box secret) (str value "!"))
  (return box)
)
```

```output
shown
shown/hidden method/private kind
renamed/hidden method/private kind
made/made! method/private kind
attribute
```

### Private fields cannot be read from another module

```brix crash
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (print (. box secret))
)
```

```brix file=lib/box.bx
(type + Box
  (prop _ secret String)
  (prop + label String)
  (prop _ kind _ (=> "private kind"))
  (fn-do + This _
    (= (. this secret) "hidden")
    (= (. this label) "shown")
  )
  (fn-do _ This _
    (param value String)
    (= (. this secret) value)
    (= (. this label) value)
  )
  (fn-expr _ hidden _
    (str secret " method")
  )
  (fn-expr + reveal _
    (str label "/" (call hidden) "/" kind)
  )
)

(fn-do + make-box Box
  (param value String)
  (let box (call Box value))
  (= (. box secret) (str value "!"))
  (return box)
)
```

```output
Exception: Property "secret" is private
<no file:7>
      (print (. box secret))
             ^^^^^^^^^^^^^^
```

### Private fields cannot be written from another module

```brix crash
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (= (. box secret) "leak")
)
```

```brix file=lib/box.bx
(type + Box
  (prop _ secret String)
  (prop + label String)
  (prop _ kind _ (=> "private kind"))
  (fn-do + This _
    (= (. this secret) "hidden")
    (= (. this label) "shown")
  )
  (fn-do _ This _
    (param value String)
    (= (. this secret) value)
    (= (. this label) value)
  )
  (fn-expr _ hidden _
    (str secret " method")
  )
  (fn-expr + reveal _
    (str label "/" (call hidden) "/" kind)
  )
)

(fn-do + make-box Box
  (param value String)
  (let box (call Box value))
  (= (. box secret) (str value "!"))
  (return box)
)
```

```output
Exception: Property "secret" is private
<no file:7>
      (= (. box secret) "leak")
         ^^^^^^^^^^^^^^
```

### Private lazy properties cannot be read from another module

```brix crash
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (print (. box kind))
)
```

```brix file=lib/box.bx
(type + Box
  (prop _ secret String)
  (prop + label String)
  (prop _ kind _ (=> "private kind"))
  (fn-do + This _
    (= (. this secret) "hidden")
    (= (. this label) "shown")
  )
  (fn-do _ This _
    (param value String)
    (= (. this secret) value)
    (= (. this label) value)
  )
  (fn-expr _ hidden _
    (str secret " method")
  )
  (fn-expr + reveal _
    (str label "/" (call hidden) "/" kind)
  )
)

(fn-do + make-box Box
  (param value String)
  (let box (call Box value))
  (= (. box secret) (str value "!"))
  (return box)
)
```

```output
Exception: Property "kind" is private
<no file:7>
      (print (. box kind))
             ^^^^^^^^^^^^
```

### Private methods cannot be called from another module

```brix crash
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (print (call (. box hidden)))
)
```

```brix file=lib/box.bx
(type + Box
  (prop _ secret String)
  (prop + label String)
  (prop _ kind _ (=> "private kind"))
  (fn-do + This _
    (= (. this secret) "hidden")
    (= (. this label) "shown")
  )
  (fn-do _ This _
    (param value String)
    (= (. this secret) value)
    (= (. this label) value)
  )
  (fn-expr _ hidden _
    (str secret " method")
  )
  (fn-expr + reveal _
    (str label "/" (call hidden) "/" kind)
  )
)

(fn-do + make-box Box
  (param value String)
  (let box (call Box value))
  (= (. box secret) (str value "!"))
  (return box)
)
```

```output
Exception: Method "hidden" of Box is private, declared with _, so only its own module can call it
<no file:7>
      (print (call (. box hidden)))
             ^^^^^^^^^^^^^^^^^^^^^
```

### Private constructors cannot be called from another module

```brix crash
(import ./lib/box)
(blank)

(main
  (print (. (call Box "direct") label))
)
```

```brix file=lib/box.bx
(type + Box
  (prop _ secret String)
  (prop + label String)
  (prop _ kind _ (=> "private kind"))
  (fn-do + This _
    (= (. this secret) "hidden")
    (= (. this label) "shown")
  )
  (fn-do _ This _
    (param value String)
    (= (. this secret) value)
    (= (. this label) value)
  )
  (fn-expr _ hidden _
    (str secret " method")
  )
  (fn-expr + reveal _
    (str label "/" (call hidden) "/" kind)
  )
)

(fn-do + make-box Box
  (param value String)
  (let box (call Box value))
  (= (. box secret) (str value "!"))
  (return box)
)
```

```output
Exception: No constructor of Box accepts (String); its declared constructors are "Box()"; "Box(value String)" is private to its module
<no file:6>
      (print (. (call Box "direct") label))
                      ^^^
```

### Private fields cannot be set as constructor properties from another module

```brix crash
(import ./lib/box)
(blank)

(main
  (let box
    (<> Box
      (= secret "leak")
    )
  )
  (print (. box label))
)
```

```brix file=lib/box.bx
(type + Box
  (prop _ secret String)
  (prop + label String)
  (prop _ kind _ (=> "private kind"))
  (fn-do + This _
    (= (. this secret) "hidden")
    (= (. this label) "shown")
  )
  (fn-do _ This _
    (param value String)
    (= (. this secret) value)
    (= (. this label) value)
  )
  (fn-expr _ hidden _
    (str secret " method")
  )
  (fn-expr + reveal _
    (str label "/" (call hidden) "/" kind)
  )
)

(fn-do + make-box Box
  (param value String)
  (let box (call Box value))
  (= (. box secret) (str value "!"))
  (return box)
)
```

```output
Exception: Property "secret" not found
<no file:8>
          (= secret "leak")
             ^^^^^^
```

### Subclasses in another module cannot see private base members

```brix crash
(import ./lib/box)
(blank)

(main
  (let crate (call Crate))
  (print (call (. crate peek)))
)

(type + Crate (is Box)
  (fn-expr + peek _
    secret
  )
)
```

```brix file=lib/box.bx
(type + Box
  (prop _ secret String)
  (prop + label String)
  (prop _ kind _ (=> "private kind"))
  (fn-do + This _
    (= (. this secret) "hidden")
    (= (. this label) "shown")
  )
  (fn-do _ This _
    (param value String)
    (= (. this secret) value)
    (= (. this label) value)
  )
  (fn-expr _ hidden _
    (str secret " method")
  )
  (fn-expr + reveal _
    (str label "/" (call hidden) "/" kind)
  )
)

(fn-do + make-box Box
  (param value String)
  (let box (call Box value))
  (= (. box secret) (str value "!"))
  (return box)
)
```

```output
Exception: Name "secret" not found
<no file:12>
        secret
        ^^^^^^
```

### Extensions in another module cannot see private members

```brix crash
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (print (call (. box peek)))
)

(extend + Box
  (fn-expr + peek _
    (call hidden)
  )
)
```

```brix file=lib/box.bx
(type + Box
  (prop _ secret String)
  (prop + label String)
  (prop _ kind _ (=> "private kind"))
  (fn-do + This _
    (= (. this secret) "hidden")
    (= (. this label) "shown")
  )
  (fn-do _ This _
    (param value String)
    (= (. this secret) value)
    (= (. this label) value)
  )
  (fn-expr _ hidden _
    (str secret " method")
  )
  (fn-expr + reveal _
    (str label "/" (call hidden) "/" kind)
  )
)

(fn-do + make-box Box
  (param value String)
  (let box (call Box value))
  (= (. box secret) (str value "!"))
  (return box)
)
```

```output
Exception: Function "hidden" not found
<no file:12>
        (call hidden)
        ^^^^^^^^^^^^^
```

### Private overloads are skipped from another module

```brix
(import ./lib/scale)
(blank)

(main
  (let scale (call Scale))
  (print (call (. scale inside) 2))
  (print (call (. scale apply) 2))
  (print (call (. scale inside) 2))
  (print (call (. scale assisted)))
)
```

```brix file=lib/scale.bx
(type + Scale
  (fn-expr + apply _
    (param value Float)
    "public float"
  )
  (fn-expr _ apply _
    (param value Int)
    "private int"
  )
  (fn-expr + inside _
    (param value Int)
    (call apply value)
  )
)

(extend + Scale
  (fn-expr _ helper _
    "private extension helper"
  )
  (fn-expr + assisted _
    (call helper)
  )
)
```

```output
private int
public float
private int
private extension helper
```

### Private extension members cannot be called from another module

```brix crash
(import ./lib/scale)
(blank)

(main
  (let scale (call Scale))
  (print (call (. scale helper)))
)
```

```brix file=lib/scale.bx
(type + Scale
  (fn-expr + apply _
    (param value Float)
    "public float"
  )
  (fn-expr _ apply _
    (param value Int)
    "private int"
  )
  (fn-expr + inside _
    (param value Int)
    (call apply value)
  )
)

(extend + Scale
  (fn-expr _ helper _
    "private extension helper"
  )
  (fn-expr + assisted _
    (call helper)
  )
)
```

```output
Exception: Method "helper" of Scale is private, declared with _, so only its own module can call it
<no file:7>
      (print (call (. scale helper)))
             ^^^^^^^^^^^^^^^^^^^^^^^
```

### Private fields stay usable through table rows inside their declaring module

```brix
(import /core/table)
(import ./lib/particle)
(blank)

(main
  (let table (call (of Table Particle) 1))
  (print (. ([] table 0) id))
  (print (call total-mass table))
)
```

```brix file=lib/particle.bx
(import /core/table)

(type + Particle (is Struct)
  (prop + id I32)
  (prop _ mass Float)
)

(fn-do + total-mass Float
  (param table (of Table Particle))
  (= (. ([] table 0) mass) 2.5)
  (return (. ([] table 0) mass))
)
```

```output
0
2.5
```

### Private fields cannot be read through table rows from another module

```brix crash
(import /core/table)
(import ./lib/particle)
(blank)

(main
  (let table (call (of Table Particle) 1))
  (print (. ([] table 0) mass))
)
```

```brix file=lib/particle.bx
(import /core/table)

(type + Particle (is Struct)
  (prop + id I32)
  (prop _ mass Float)
)

(fn-do + total-mass Float
  (param table (of Table Particle))
  (= (. ([] table 0) mass) 2.5)
  (return (. ([] table 0) mass))
)
```

```output
Exception: Property "mass" is private
<no file:8>
      (print (. ([] table 0) mass))
             ^^^^^^^^^^^^^^^^^^^^^
```

### Private setters stay usable inside their declaring module

```brix
(import ./lib/dial)
(blank)

(main
  (let dial (call Dial))
  (call (. dial bump))
  (print (. dial level))
)
```

```brix file=lib/dial.bx
(type + Dial
  (prop + log _ (= ""))
  (fn-expr + level _ :get
    log
  )
  (fn-do _ level _ :set (param value String)
    (= log value)
  )
  (fn-do + bump _
    (= level "bumped")
  )
)
```

```output
bumped
```

### Private setters cannot be called from another module

```brix crash
(import ./lib/dial)
(blank)

(main
  (let dial (call Dial))
  (= (. dial level) "leak")
)
```

```brix file=lib/dial.bx
(type + Dial
  (prop + log _ (= ""))
  (fn-expr + level _ :get
    log
  )
  (fn-do _ level _ :set (param value String)
    (= log value)
  )
  (fn-do + bump _
    (= level "bumped")
  )
)
```

```output
Exception: Property "level" is private
<no file:7>
      (= (. dial level) "leak")
         ^^^^^^^^^^^^^^
```

### Private base methods cannot be called through super from another module

```brix crash
(import ./lib/base)
(blank)

(main
  (print (call (. (call Derived) describe)))
)

(type + Derived (is Base)
  (fn-expr + describe _
    (str "derived " (call (. super describe)))
  )
)
```

```brix file=lib/base.bx
(type + Base
  (fn-expr _ describe _
    "base"
  )
)
```

```output
Exception: Method "describe" of Base is private, declared with _, so only its own module can call it
<no file:11>
        (str "derived " (call (. super describe)))
                        ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Private tuple fields cannot be read from another module

```brix crash
(import ./lib/pair)
(blank)

(main
  (let pair (call Pair 1 2))
  (print (. pair shown))
  (print (. pair total))
  (print (. pair hidden))
)
```

```brix file=lib/pair.bx
(type + Pair (is Tuple)
  (prop + shown Int)
  (prop _ hidden Int)
  (fn-expr + total _ :get
    (+ shown hidden)
  )
)
```

```output
Exception: Property "hidden" is private
<no file:9>
      (print (. pair hidden))
             ^^^^^^^^^^^^^^^
```

### The main of an imported module runs before the module that imports it, and its functions are called by name.

```brix
(import ./lib/util)
(blank)

(main
  (print "main")
  (print (call util-add 1 2))
  (call shout "x")
)
```

```brix file=lib/util.bx
(main
  (print "util main")
)

(fn-expr + util-add Int
  (param a Int)
  (param b Int)
  (+ a b)
)

(fn-do + shout _
  (param s String)
  (print (++ s "!"))
)
```

```output
util main
main
3
x!
```

```brix
(import ./lib/a)
(import ./lib/b)
(blank)

(main
  (print "entry")
)
```

```brix file=lib/a.bx
(import ./c)
(blank)

(main
  (print "a")
)
```

```brix file=lib/b.bx
(main
  (print "b")
)
```

```brix file=lib/c.bx
(main
  (print "c")
)
```

```output
c
a
b
entry
```

```brix
(import ./lib/b)
(import ./lib/a)
(blank)

(main
  (print "entry")
)
```

```brix file=lib/a.bx
(import ./c)
(blank)

(main
  (print "a")
)
```

```brix file=lib/b.bx
(main
  (print "b")
)
```

```brix file=lib/c.bx
(main
  (print "c")
)
```

```output
b
c
a
entry
```

### A name two imported modules declare is found through the import written last, and the modules it exports, before the one written first.

```brix
(import ./lib/near)
(import ./lib/far)
(blank)

(main
  (print shade)
)
```

```brix file=lib/near.bx
(prop + shade _ (= "near"))
```

```brix file=lib/far.bx
(export ./tone)
```

```brix file=lib/tone.bx
(prop + shade _ (= "tone"))
```

```output
tone
```

### A type written by name that two imported modules declare is the one of the import written first, and so is one a function's argument is written with.

```brix
(import ./lib/narrow)
(import ./lib/wide)
(blank)

(main
  (let top (. Id max))
  (let given Id 200)
  (print (str (as top U32) " " (as given U32) " " (call pick (as 3 Id))))
)

(fn-do _ pick String
  (param id Id)
  (return (str "narrow " (as id U32)))
)
```

```brix file=lib/narrow.bx
(type + Id (is U8))
```

```brix file=lib/wide.bx
(type + Id (is U16))

(fn-do + pick String
  (param id Id)
  (return (str "wide " (as id U32)))
)
```

```output
255 200 narrow 3
```

### A class derives only from a base its own module can name: a base declared in a module that only an imported module imports is not found, though a property of the imported module holds that base.

```brix compile-error
(import ./lib/shelf)
(blank)

(main
  (= (. (call Shelf) item) (call Book))
)

(type + Book (is Item))
```

```brix file=lib/shelf.bx
(import ./item)

(type + Shelf
  (prop + item (? Item) (= null))
)
```

```brix file=lib/item.bx
(type + Item)
```

```output
Exception: Type "Item" not found; it is declared in ./lib/item, which this module does not import. Add the import ./lib/item
<no file:9>
    (type + Book (is Item))
                     ^^^^
```

### A module imported by two paths that name one file, one through `..`, is one module, whose types are the same type.

```brix
(import /core/arena)
(import /core/../core/arena)
(blank)

(main
  (let arena (use (call Arena)))
  (print (str "ok " (call make arena)))
)

(fn-expr _ make Int
  (param arena Arena)
  3
)
```

```output
ok 3
```

### A folder's own module imported by its file's path is the module the folder's name imports, so the functions the compiler supplies for `/math` are found through either name.

```brix
(import /math/math)
(blank)

(main
  (let x -0.5)
  (print (call abs x))
  (print (call sqrt 9.0))
)
```

```output
0.5
3
```

### A C header under the `c` root is found among the system headers

```brix
(import /c/unistd.h)
(blank)

(main
  (print (> (call C:getpid) 0))
)
```

```output
true
```

### A C header that is not there is a module not found at its import

```brix crash
(import /no-such-header.h)
(blank)

(main
  (print 1)
)
```

```output
Exception: Module "no-such-header.h" not found
<no file:2>
    (import /no-such-header.h)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A module named for a platform is part of that platform's builds. Importing one named for another platform imports nothing, and its file is not read.

```brix
(import ./lib/input.macos)
(import ./lib/input.ios)
(import ./lib/look.apple)
(blank)

(main
  (call install-input)
  (print (call look))
)
```

```brix file=lib/input.macos.bx
(fn-do + install-input _
  (print "macos input")
)
```

```brix file=lib/input.ios.bx
(import /no-such-module)

(fn-do + install-input _
  (print "ios input")
)
```

```brix file=lib/look.apple.bx
(fn-expr + look String
  "apple look"
)
```

```output
macos input
apple look
```

### A function that only a module for another platform declares is not found, and the error names that module

```brix crash
(import ./lib/input.ios)
(blank)

(main
  (call install-input)
)
```

```brix file=lib/input.ios.bx
(fn-do + install-input _
  (print "ios input")
)
```

```output
Exception: Function "install-input" not found; it is only defined in input.ios.bx, which isn't part of macos builds
<no file:6>
      (call install-input)
      ^^^^^^^^^^^^^^^^^^^^
```

### A module name that ends in something other than a platform is an error

```brix crash
(import ./lib/input.windows)
(blank)

(main
  (print 1)
)
```

```brix file=lib/input.windows.bx
(main
  (print 2)
)
```

```output
Exception: Module "./lib/input.windows" ends in ".windows", which isn't a platform
<no file:2>
    (import ./lib/input.windows)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A module for one platform can't import a module for a platform it is never built with

```brix crash
(import ./lib/keys.macos)
(blank)

(main
  (print 1)
)
```

```brix file=lib/keys.macos.bx
(import ./touch.ios)
```

```brix file=lib/touch.ios.bx
(main
  (print 2)
)
```

```output
Exception: Module "./touch.ios" is never part of the same build as keys.macos.bx
<lib/keys.macos.bx:1>
    (import ./touch.ios)
    ^^^^^^^^^^^^^^^^^^^^
```

### A file whose name starts with _ is private to its folder: modules in that folder and in the folders inside it import it

```brix
(import ./lib/shelf)
(import ./lib/deep/stand)
(blank)

(main
  (print (str (call shelf-tool) " " (call stand-tool)))
)
```

```brix file=lib/_tool.bx
(fn-expr + tool-name String
  "tool"
)
```

```brix file=lib/shelf.bx
(import ./tool)

(fn-expr + shelf-tool String
  (call tool-name)
)
```

```brix file=lib/deep/stand.bx
(import ./../tool)

(fn-expr + stand-tool String
  (call tool-name)
)
```

```output
tool tool
```

### A module outside the folder of a file whose name starts with _ can't import it, even from the folder around it

```brix compile-error
(import ./top)
(blank)

(main
  (print 1)
)
```

```brix file=top.bx
(import ./lib/tool)
```

```brix file=lib/_tool.bx
(main
  (print 2)
)
```

```output
Exception: Module "./lib/tool" is private to lib/, so only modules there can import it
<top.bx:1>
    (import ./lib/tool)
    ^^^^^^^^^^^^^^^^^^^
```
