# Modules

A program is made of modules, and each module is one file. A file imports the modules whose names
it uses, declares the values that belong to it, and may run code of its own when the program
starts. This chapter covers how a file names another, what an import makes visible, exports,
visibility, the values a file holds, the order in which files start, and how a program ends.

## What a module is

- **One file.** A module is a `.bx` file. A path names the file without its extension, so
  `(import ./lib.bx)`{.brix} is a missing module.
- **Folder modules.** A folder is not a module, but it can hold one: a file with the folder's own
  name. `/tui` is the file `tui/tui.bx` under the standard library. When a folder has such a file
  and a file of the same name sits beside the folder, the one inside the folder is the module. A
  folder with neither is a missing module, so `(import /os)`{.brix} fails, while
  `(import /os/path)`{.brix} finds `os/path.bx`.
- **Platform modules.** A file named with a platform after a dot, as in `input.macos.bx`, is part
  of that platform's builds only. Importing it for another platform imports nothing. Any other
  suffix is an error, except in a file family. Platforms lists the platforms and the families of
  them.
- **File families.** A file named after another beside it, then a dot and more name, as
  `box.scroll.bx` is beside `box.bx`, belongs to the file family of that file, its base.
  - **The name** after the base's may have several parts, and may end with a platform, as
    `box.touch.ios.bx` does, which makes the file part of that platform's builds only.
  - **Only the family imports them.** The base and the family's other files may import a
    family's files. Elsewhere it is an error: `Module "./lib/box.scroll" belongs to the file
    family of box.bx, so only that family can import it`. Code outside imports the base, which
    imports the files of its family.
  - **Ordinary modules.** A family's files may declare anything a module may. A class's facets are
    declared in its family; see Types.
  - **No base.** A dotted name with no file of its first part beside it, such as `host.macos.bx`
    beside no `host.bx`, is no family's, and any module may import it.
- **Names.** Stack traces, object text and some messages name a module.
  - The **entry file**, the one given to `brix`, is always `__source__`, whatever the file is
    called.
  - A **library module** is named by its path without the leading `/`, as in `os/path`.
  - A **program's other files** are named by their path from the entry file's folder, as in
    `models/account` or `../shared/thing`.

## Imports

```brix
(import /math)

(main
  (print (call sqrt 16.0))
  (print (call abs -2.5))
  (print (call floor 2.7))
)
```

```output
4
2.5
2
```

| Form | Rows | Finds |
| --- | --- | --- |
| `(import /os/path)`{.brix} | `/os/path`{.rows} | `os/path` in the search paths. |
| `(import ./convert)`{.brix} | `./convert`{.rows} | `convert` beside the importing file. |
| `(import ./lib/shapes)`{.brix} | `./lib/shapes`{.rows} | `shapes` in the folder `lib` beside it. |
| `(import ../shared/thing)`{.brix} | `../shared/thing`{.rows} | `shared/thing` from the importing file's parent folder. |
| `(export ./circle)`{.brix} | `+ ./circle`{.rows} | The same as an import, and the names pass on; see [Exports](#exports). |

- **Every public name, unqualified.** An import makes every public name of the module usable as
  if it were declared here: functions, types, module values, getters and setters, units, and the
  members of public extensions. There is no selective import, no renaming and no qualified
  access, so a module's own name is not a value.
- **One path each.** An import takes one path, written bare and not in quotes. A file imports as
  many modules as it needs, one form each.
- **Where.** An import stands at the top level of the file. Inside `main` it is an error.
- **Not passed on.** An import gives its names to the file that writes it and to no other. When
  `a` imports `b`, a file that imports `a` must still import `b` to use `b`'s names, unless `a`
  exports `b`.
- **Once.** Each module is loaded once, however many files import it and however many times one
  file does.
- **Loaded even if unused.** An import that nothing uses still loads its module, whose values are
  set and whose `main` runs. The compiler does not report it.
- **Private files.** `./helper` also finds `_helper.bx` beside the importing file when there is no
  `helper` file. Rooted paths name such files in full, as in `/core/_bytes`. A module whose name
  starts with `_` belongs to the modules around it, by convention; the compiler does not stop
  others importing it.

### Search paths

A rooted path, one that starts with `/`, is looked up in each search path in turn, and the first
folder that holds it wins.

- **The default** is the toolchain's standard library and nothing else.
- **`BRIXPATH`**, when it is set, replaces the default with its colon-separated folders. List the
  standard library among them, because without it no program builds:
  `Module "core" not found`.
- **A program's own files** are reached by relative paths, `./` and `../`, from the file that
  imports them. There is no project file or manifest, and a project's root folder plays no part.

### A name that is not imported

```brix compile-error
(main
  (print (call sqrt 16.0))
)
```

```output
Exception: Function "sqrt" not found; it is declared in /math, which this module does not import. Add the import /math
<no file:3>
      (print (call sqrt 16.0))
             ^^^^^^^^^^^^^^^^
```

- **The suggestion.** When a name is not found, the compiler looks for it in the modules the
  program loads and in the search paths, and names up to three modules that declare it.
- **A missing module** is an error at the import: `Module "no/such" not found` for
  `(import /no/such)`{.brix}, and `Module "./nothing" not found` for
  `(import ./nothing)`{.brix}.
- **The compiler's status.** The compiler exits with status 2 when it reports errors.

### Core

- **Always imported.** Every module imports the core modules without writing them: `core`,
  `core/error`, `core/number`, `core/string`, `core/type`, `core/backing`, `core/array`,
  `core/function`, `core/collection`, `core/list`, `core/map`, `core/range`, `core/stream`,
  `core/channel`, `core/event`, `core/tracker`, `core/parsing` and `core/task`. That is where
  `true`, `false`, `null`, `String`, `List`, `print` and `request-process-termination` come from.
  Importing one of them by name changes nothing.
- **Imported on use.** A module that writes grammars, markup or styles imports the core
  modules that support them without writing them.
- **The rest by name.** Other modules under `core`, such as `/core/table`, are imported like any
  library. The Standard library lists them.

### Overloads and clashes

- **Overloads across modules.** Functions of one name overload one another across modules: a
  module's own `describe` taking a `Float` and two imported ones taking an `Int` and a `String`
  are one set, and each call reaches the one its arguments fit.
- **A module's own names first.** A module value or type declared in the module hides an imported
  one of the same name.
- **Clashes between imports.** The compiler does not report two imports that declare the same
  name; which one a use reaches depends on the order of the imports. The linter reports such
  clashes, as `names/import-conflicts` and `names/local-shadows-import`. Rename one of them.

### Import cycles

Two modules may import each other, directly or through others, and the compiler does not report
it. Each can call the other's functions. A cycle matters only to the values the modules set as
they start: the module reached second starts first, so its values cannot read the other's eager
values. Such a read panics, `Global 'a-value' is read before it is initialized`. A lazy value or a
literal value breaks the dependency; see [Order of initialization](#order-of-initialization).

## Exports

`(export ./circle)`{.brix}, in rows `+ ./circle`{.rows}, imports the module for this file and also
passes its public names on to every file that imports this one.

- **The exporter uses them too.** The exported module's names are usable in the file that exports
  it, as with an import.
- **Exports chain.** When `a` exports `b` and `b` exports `c`, a file that imports `a` gets the
  names of all three.
- **Whole modules.** An export takes one module path. There is no way to pass on one name.
- **Libraries.** A library made of several files gathers them in its folder module. `/tui` is
  `tui/tui.bx`, which exports the modules of the terminal library, so one import gives all of
  them. `/units` exports `./distance`, so importing `/units` gives the distance units too, while
  importing `/units/distance` gives only those.

## Visibility

Every declaration states its access: `+` is public and `_` is private. That holds for module
values, functions, types, members, extensions, events and units. A member may also be written
with `^`, shared with its file family.

- **Private means the file.** A private name is usable anywhere in its own file: in functions,
  methods, setters and the rows of `main`. Two files in one folder cannot see each other's private
  names.
- **From another module**, a private name is refused:
  - a private function or method: `Function "helper" is private, declared with _, so only its own
    module can call it`;
  - a private property, setter or computed property: `Property "hidden" is private`;
  - a private constructor: the message lists the constructors and says which are private to their
    module;
  - a private event: `Event "poked" is private`;
  - a private module value or type is not found, as if it were not declared:
    `Name "secret" not found` and `Type "Hidden" not found`.
- **Shared means the file family.** A member declared with `^` is usable in every file of its
  module's file family, as a public one is, and refused elsewhere, as a private one is:
  - a shared method: `Method "keep-end" of Box is shared, declared with ^, so only its own file
    family can call it`;
  - a shared property: `Property "width" is shared, declared with ^, so only its own file family
    can use it`;
  - a shared constructor: the message lists the constructors and says which are shared with
    their file family.

  Only a member is shared, and `^` on anything else is an error: `Only a member can be shared
  with its file family; declare this + or _`. A shared event does not bubble, as a private one
  does not.
- **Facets.** A file that declares a facet of a class reaches the private declarations of the
  file that declares the class, but that file does not reach the facet's. See Types.
- **Extensions.** The members a public extension adds are visible in its own module and in the
  modules that import it or receive it through an export. A private extension's members are
  visible in its own module only. An extension must state its access, as in
  `(extend + String (fn-expr + shouted String (. self uppercased)))`{.brix}.
- **Always public.** Converters and unit equalities cannot be private: `Converters must be public`
  and `Equalities must be public`.

## Values that belong to a file

```brix
(prop + limit _ (= 3))

(prop _ names (list String) (= (list "ada" "grace")))

(prop _ count Int)

(prop _ report _ (:= (call build-report)))

(prop _ doubled _ (=> (* count 2)))

(main
  (print (str "limit " limit ", count " count))
  (+= count 5)
  (print doubled)
  (+= names "linus")
  (print names)
  (print report)
  (print report)
)

(fn-expr _ build-report String
  (before
    (print "building report")
  )
  (str (. names length) " names")
)
```

```output
limit 3, count 0
10
ada, grace, linus
building report
3 names
3 names
```

A `prop` at the top level of a file declares a module value. There is one of it for the whole
program, shared by every task.

| Form | Rows | Value |
| --- | --- | --- |
| `(prop + limit _ (= 3))`{.brix} | `+ limit = 3`{.rows} | Eager: set as the module starts. |
| `(prop _ names (list String) (= (list "ada")))`{.brix} | `_ names [String] = ["ada"]`{.rows} | Eager, with its type written. |
| `(prop _ count Int)`{.brix} | `_ count Int`{.rows} | The type's default. |
| `(prop _ report _ (:= (call build-report)))`{.brix} | `_ report := build-report()`{.rows} | Lazy: computed at the first read, once. |
| `(prop _ doubled _ (=> (* count 2)))`{.brix} | `_ doubled => count * 2`{.rows} | Computed again at every read. |
| `(prop + calls _ @task (= 0))`{.brix} | `+ @task calls = 0`{.rows} | One for each task; see Concurrency. |

- **Defaults.** A value with a type and no value takes the type's default: `0` for a number, `""`
  for a `String`, `null` for an optional, an empty list for a list, and for a class, an object made by
  its constructor with no arguments.
- **Assigning.** A module value can be assigned in its own file, and a public one in any file that
  imports it, with `=`, `+=` and the other updates. A value computed at every read cannot be
  assigned: `Setter not found`.
- **Lazy values.** A lazy value is computed at its first read, and never if nothing reads it.
  Assigning it before then skips its value. An error that leaves its value is reported at the read
  that started it.
- **What a value may read.** A value's initializer reads other module values, functions and
  imports. It cannot read the locals of `main`, and neither can a function.
- **Not tracked.** Module values cannot be tracked; see Reactivity.

### Getters and setters

```brix
(prop _ celsius _ (= 20.0))

(main
  (print fahrenheit)
  (= fahrenheit 212.0)
  (print celsius)
  (+= fahrenheit 18.0)
  (print celsius)
)

(fn-expr + fahrenheit Float :get
  (+ (* celsius 1.8) 32)
)

(fn-do + fahrenheit _ :set
  (param value Float)
  (= celsius (/ (- value 32) 1.8))
)
```

```output
68
100
110
```

- **Getters.** A function marked `:get` is called wherever its name is read, in its module and in
  the modules that import it. A getter with no setter is a read-only module value.
- **Setters.** A function marked `:set` is called wherever its name is assigned. An update such as
  `+=` reads through the getter and stores through the setter.

### Constants

A module value whose value is a literal number or text, with no decoration, is a constant. The
program lays it down before any module starts, so any value or module may read it, even one that
starts first. It can still be assigned.

## Order of initialization

```brix
(prop + first _ (= (call noisy "first")))

(prop + second _ (= (call noisy "second")))

(prop + unused _ (= (call noisy "unused")))

(prop + later _ (:= (call noisy "later")))

(main
  (print "main starts")
  (print second)
  (print first)
  (print later)
  (print later)
)

(fn-do _ noisy String
  (param name String)
  (print (str "initializing " name))
  (return name)
)
```

```output
initializing first
initializing second
initializing unused
main starts
second
first
initializing later
later
later
```

A program starts in this order:

1. **Constants**, in every module.
2. **Each module in turn**, in initialization order:
   - its eager values, in the order the file declares them, whether anything reads them or not;
   - then its `main`.
3. **The entry file last.**

The initialization order puts each module after the modules it imports, in the order its imports
are written, and visits each module once. So when the entry file imports `report` and then
`tally`, and `report` imports `tally` too, `tally` starts first, then `report`, then the entry
file. In an import cycle, the module reached second starts first.

A file's eager values all come before its `main`, even those declared after it. Lazy values come
when they are first read.

### A value read before it is set

A value's initializer may read a value declared after it only if that value is a constant. Read
directly, a later value is a compile error:

```brix compile-error
(prop + total _ (= (+ base 1)))

(prop + base _ (= (call ten)))

(main
  (print total)
)

(fn-expr _ ten Int
  10
)
```

```output
Exception: Global 'base' is read before it is initialized
<no file:2>
    (prop + total _ (= (+ base 1)))
                          ^^^^
```

Read through a function, or from a module that starts earlier, it is a panic when the program
runs:

```brix crash
(prop + first _ (= (call read-name)))

(prop + name _ (= (call noisy "world")))

(main
  (print first)
)

(fn-expr _ read-name String
  name
)

(fn-do _ noisy String
  (param text String)
  (return text)
)
```

```stderr

Unhandled panic: Global 'name' is read before it is initialized
<no file:11>

Task 1

Stack trace:
  0  __source__:read-name no file:11:3
  1  __source__:init.__source__.globals no file:2:20
  2  __source__:brix.main no file
```

To break such a dependency, declare the value that is read first, make the value that reads it
lazy with `(:= …)`, or make the value that is read a constant.

## The code a file runs

```brix
(import /math)

(main
  (print "first block")
  (let sides (list 3.0 4.0))
)

(fn-expr + hypotenuse Float
  (param a Float)
  (param b Float)
  (call sqrt (+ (* a a) (* b b)))
)

(main
  (print (call hypotenuse ([] sides 0) ([] sides 1)))
)
```

```output
first block
5
```

- **Every file's `main` runs.** `main` holds the code that runs when its file starts, and every
  module's `main` runs once, after the module's values are set. A library's `main` runs before
  the files that import it.
- **Why.** A program starts with what it does, with no entry function to declare. The cost falls
  on libraries: importing one sets its values and runs its `main`, which may read files or fail,
  so keep a library's `main` small.
- **Several `main` forms** in one file are one body that runs in file order. A local of the first
  is visible in the next.
- **No `main`.** A file without one only declares. Its values are still set as the program
  starts.
- **What `main` cannot hold.** An import, a value standing alone, or a `return`: `The top level of
  a module cannot return a value with "return"; remove the "return", or print the value with
  "print"`.
- **In rows**, the statements of a file are its rows before the first divider. Another block of
  statements after the declarations follows a thick divider and a blank row, and becomes another
  `(main …)`.

### How a program ends

When the entry file's `main` finishes, the program waits for the tasks it started, then ends.

- **`when-main-ends`.** The actions given to `(call when-main-ends action)`{.brix} run when `main`
  finishes, before its cleanups.
- **Errors.** A recoverable error that leaves a `main`, the entry file's or another module's, ends
  the program with a report on stderr; see Errors.
- **Arguments and the environment** come from libraries, not from core. `/os/cli` gives
  `cmdline-args`, a list of texts whose first item is the program's path, and `cmdline`, the
  arguments taken apart into a command, flags and options. `/os/env` gives
  `(call env "HOME")`{.brix}, an optional text, and `(call set-env "HOME" "/tmp")`{.brix}.

```brix crash
(main
  (print "checking")
  (call request-process-termination 3)
  (print "main still ends its rows")
)
```

```output
checking
main still ends its rows
```

- **Requesting the end.** `(call request-process-termination 3)`{.brix} asks the program to end
  with that status; with no argument the status is 130. The task goes on until it next waits, and
  the wait ends it. Tasks still running are not waited for.

| Status | Means |
| --- | --- |
| 0 | The program ended normally. |
| 1 | A recoverable error left a `main` or a task's body. |
| 134 | A panic was not handled. |
| 138 | The stack overflowed. |
| 128 + the signal | The program was stopped by SIGHUP or SIGTERM. |
| Any other | The program requested it. |

## Building and running

- **Checking.** `brix app.bx` checks the program: every declaration of the entry file, whether
  anything reaches it or not, and what it uses of other files.
- **Running.** `brix app.bx -o build/app` builds the executable at `build/app` and runs it, and
  exits with the program's status. Arguments for the program follow the source path; those that
  start with `-` follow `--`, as in `brix app.bx -o build/app -- --loud`.
- **Building only.** `brix app.bx --compile -o build/app` builds without running.
- **Reached code.** A build checks only the code the program reaches, so a broken function that
  nothing calls is reported by a check and not by a build.
- **A program in several files.** Put the files in one folder, or in folders below it, import them
  with `./` and `../`, and give `brix` the entry file. `brix app.bx --list-sources` lists every
  file the program loads.
