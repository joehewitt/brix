# Source text

A Brix program is text in files. A `.bx` file holds Brix forms, which are S-expressions. A `.yumi`
file holds rows, which use indentation, dividers and operators. The two are views of one language:
every form has a rows spelling, and every row a form. This chapter covers files, how forms are
read, how rows are laid out, names, comments, and how the two views relate.

## Files

- **Two kinds.** The extension decides how a file is read: `.bx` as forms, and anything else as
  rows. Each file is read on its own, so one program can mix the two, and when `x.bx` and `x.yumi`
  sit side by side, an import of `x` reads `x.bx`. Modules covers how files find each other.
- **One kind in a file.** A file holds forms or rows, never both.
- **UTF-8.** Source text is UTF-8, with no byte-order mark. Write lines ending in LF; forms also
  accept CRLF.
- **Names.** A file may have any name. The repository's files use lowercase words joined by
  hyphens. A dot in a file name marks a platform, as in `input.macos.bx`; see Platforms.
- **Empty files.** An empty file is a module that declares nothing, and the last line needs no
  line ending.

## Forms

```brix
(import /math)

(prop _ greeting String (= "Hello"))

(main
  (print (call greet "Ada"))
  (print (call hypotenuse 3.0 4.0))
)

(fn-expr + greet String
  (param name String)
  (str greeting ", " name "!")
)

(fn-expr + hypotenuse Float
  (param a Float)
  (param b Float)
  (call sqrt (+ (* a a) (* b b)))
)
```

```output
Hello, Ada!
5
```

A file is a sequence of forms. A form is a list in parentheses, and its first item is its head,
which says what the form is. The other items are its rows: atoms, texts and other forms.

- **Items.** An item is an atom, such as `greet`, `+` or `3.0`; a text in double quotes, which may
  span lines; or a list in parentheses.
- **Where an atom ends.** An atom ends at whitespace, a parenthesis, a double quote or a `;`. So
  `(print(str"a""b"))` is read like `(print (str "a" "b"))`.
- **Heads.** A head is a word from the language's vocabulary, such as `fn-expr`, `let`, `if` or
  `print`. A function is not a head: a call is written `(call greet "Ada")`{.brix}.
- **Named arguments and operators.** Two kinds of head are open. A head that starts with a colon
  names an argument, as in `(call f (:limit 3))`{.brix}. A head between slashes is a named
  operator, as in `(/max/ 3 4)`{.brix}.
- **One list with no head.** The parameters of a function value are a plain list, as in
  `(=> (a b) (+ a b))`{.brix}.
- **The top level** holds declarations, imports and `(main …)` forms, in any order. A function may
  come after the `main` that calls it.

### Layout

```brix
(main (print (call area 2 3)) (print "on one line"))
(fn-expr + area Int (param width Int) (param height Int) (* width height))
(main
  (print
    (call area
      4
      5
    )
  )
)
```

```output
6
on one line
20
```

- **Whitespace separates items and means nothing else.** Spaces, tabs and line breaks may go
  anywhere between items, and a form may sit on one line or spread over many.
- **The usual layout** puts one row to a line, two spaces deeper than the form that holds it, and
  closes a form that spans lines on a line of its own. Functions, types and other declarations
  with bodies are set apart by a blank line. `brix lint --fix` rewrites a file in this layout.
- **Hints.** When a `)` is missing or extra, the compiler uses the indentation to guess where it
  belongs.

### Atoms

The first characters of an atom say what it is:

| Atom | Read as |
| --- | --- |
| A digit, or `-`, `.` or `-.` and a digit | A number, as in `-5`, `.5`, `1'000`, `0xff` or `3km`; see Literals. |
| `'A'` | A character. |
| `''T` | A type parameter. |
| `.red` | A symbol or an enum case. |
| `..name` | An anonymous case. |
| `#name` | A tracked read; see Reactivity. |
| `&name` | A reference. |
| `$name`, `%name` | A style read and a markup read. |
| `C:name` | A C name; see Native code and memory. |
| A capital letter | A type name. |
| Anything else | A name. |

Some atoms fill fixed places in a form:

- **Access.** `+` is public, `_` is private, and `^` shares a member with its file family, as in
  `(fn-expr + greet String …)`.
- **The hole.** `_` in a type's place lets the compiler infer it, and in a pattern or a loop it
  stands for a value with no name.
- **Flags** start with a colon, such as `:get` and `:static`, and **annotations** start with `@`,
  such as `@inline`. Both go in the header of a declaration, flags first.
- **Constants.** `true`, `false` and `null`. `nil`, `#t` and `#f` are refused with a message that
  gives the Yumi spelling.

### What is not an atom

- **No quoting.** A `'` starts a character or a type parameter and nothing else, and `quote` is
  not a form.
- **Generic types are forms.** A list of integers is `(of List Int)`, a map is
  `(of Map String Int)`, and an optional is `(? Int)`. `List'Int` is not a type name in forms.
- **Members are forms.** A member read is `(. corner x)`, and a method call is
  `(call (. line split) ",")`. `corner.x` is not a name: the compiler says to write `(. corner x)`.

### What the reader refuses

```brix compile-error
(main
  (println "hello")
)
```

```output
Exception: `println` is not a form. Write `(print value)`.
<no file:3>
      (println "hello")
      ^
```

- **Unknown heads.** A word that is not a form is refused. For a word some other language uses,
  such as `lambda`, `defun`, `cond` or `try`, the message names the Yumi form. Otherwise it shows
  how to call a function of that name.
- **A form in the wrong place.** The message names the place and what it takes, as with
  `(print …) cannot be a form at the top level of a file, which takes Top`.
- **A form with too much.** The message gives the form's shape, as with `(let …) takes nothing
  more, and (list …) follows. Its shape is (let target:Binder/Pattern value:Expr/Block?).`
- **Unbalanced parentheses and quotes** are reported where the form or text began:
  `Missing closing quote for the text opened at line 2, column 10`.

## Rows

The same program in rows:

```rows
/math

_ greeting String = "Hello"
-- greet("Ada")
-- hypotenuse(3.0, 4.0)

━━━━━━━━━━━━━━━━━━━━
+ greet String
    - name String

"$greeting, $name!"

━━━━━━━━━━━━━━━━━━━━
+ hypotenuse Float
    - a Float
    - b Float

sqrt(a * a + b * b)
```

### Indentation

- **Four spaces make a level**, and only spaces count. A row indented deeper than the row above
  belongs to it, as a body, a block or a parameter list.
- **Write whole levels.** The level is the number of spaces divided by four, rounded down, and a
  tab counts for nothing. So a body indented with two spaces or a tab is not a body. A row may go
  one level deeper than the row above, not two.
- **Comment lines** may be indented any way.
- **Blank lines** do not end a block; only a row less indented does.

### Dividers

A divider is a line of one glyph repeated at least three times, with nothing else on it, not even
spaces. It begins a declaration that has a body, so the body can start at the left margin, and a
method has the same shape as a function of the module.

| Divider | Begins |
| --- | --- |
| `━━━━` | A declaration at the top level with a body: a function, a type with members, an extension or a style sheet. |
| `────` | A function inside a type: a method, getter, setter, constructor or converter. |
| `════` | A section of a value body. |

- **Before the first divider** come the file's imports, its module values, and the statements it
  runs.
- **After a divider** comes the declaration's header, then its parameters, indented one level,
  then its body, flush with the header. A blank line between the header and the body is usual,
  but not required.
- **A body runs to the next divider.** Anything below a body belongs to it, so the next
  declaration with a body needs its own divider: `"+ g Int" is read as part of the body of "f",
  because no divider separates them; put a thick divider (━) on the line above it`.
- **Back to the top level.** A thick divider followed by a blank line ends the declaration above
  and returns to the top level, where module values and statements may follow.
- **Inside a type**, a thick divider ends the type, and a thin divider begins a member function.

### Row marks

A row begins with a mark that says what it is, followed by a space.

| Row | Meaning | Form |
| --- | --- | --- |
| `+ name …`, `_ name …`, `^ name …` | A public, private or shared declaration. | `(fn-expr + name …)`, `(prop _ name …)`, `(fn-do ^ name …)` |
| `- name Type` | A parameter. | `(param name Type)`{.brix} |
| `... name Type` | The rest parameter. | `(param ... name Type)` |
| `. name = value` | A local. | `(let name value)` |
| `.. name = value` | A lazy binding. | `(lazy name value)` |
| `= value` | A return. | `(return value)`{.brix} |
| `<= value`, `<<= values`, `<=? value` | A yield of one item, of each item of a source, and of an item where it is not `null`. | `(yield value)`, `(yield-all values)`, `(yield? value)` |
| `-- value` | A print. | `(print value)`{.brix} |
| `\| pattern` | A match arm, or a union case in a type. | `(case pattern …)`, `(variant …)` |
| `\|= value`, `\|? condition`, `??` | Arms taken on equality, on a condition, and otherwise. | `(case= …)`, `(case? …)`, `(default …)` |
| `\|\| pattern`, `\|\|= value`, `\|\|? condition` | Arms whose values keep their own types. | `(case-union …)`, `(case-union= …)`, `(case-union? …)` |
| `?! error if condition` | A check. | `(throw error (if condition))` |
| `* value`, `** values` | Items of a list built below it. | `(item value)`, `(spread values)` |
| `?> condition`, `*> value` | Pipe stages, flush below a value. | `(pipe ?> condition)`, `(pipe *> value)` |
| `• name value` | A named argument. | `(:name value)` |
| `- value` under a call | One argument. | `(arg value)` |
| `/path`, `./path` | An import. | `(import /path)` |
| `// text` | A probe. | `(probe-log _ (text: "text"))` |
| `` ` text `` | A comment. | `; text` |
| A blank line | An empty row. | `(blank)` |

### Lines

- **Operators need spaces.** A binary operator has a space on each side, because `-` can be part
  of a name: `a-1` is one name, and `a+b` gives `An operator needs a space on each side; write
  "a + b"`.
- **An expression ends at the end of its line.** A line cannot end in an operator, a comma or an
  open bracket.
- **Longer constructs continue on indented rows.**
  - A call with its arguments on `- value` rows below it.
  - A bullet at the end of a line takes the indented block below as its value, except on an `if`,
    `else`, `while`, `match` or `for` line, whose block is its body.
  - A list built from `*` rows below a builder.
  - Pipe stages, flush below the value they take.
  - A text continues on the next line after a `\` at the end of a line.
- **One-line bodies.** A colon puts a body of one step on the same line, as in `if n > 9: -- "big"`
  and `for x in xs: -- x`. In forms, such a step is wrapped in `(inline …)`.
- **Braces.** Blocks are written by indentation. `if c {` gives `Blocks are written by
  indentation, not braces`.

## Names

```brix
(main
  (let item-count 3)
  (let unit-price-2 20)
  (let π 3.5)
  (print (* item-count unit-price-2))
  (print (* π 2))
  (let corner (call Vec2 1 2))
  (print (. corner x))
  (print (call (of List Int) 4 5))
)

(type + Vec2 (is Tuple)
  (prop + x Int)
  (prop + y Int)
)
```

```output
60
7
1
4, 5
```

- **Values** are named in lowercase words joined by hyphens, with digits allowed after the first
  letter: `item-count`, `unit-price-2`. A single Greek letter, such as `π` or `Δ`, is a name too,
  and so is `°`.
- **Types** are named by a capital letter followed by letters and digits: `Vec2`, `HttpRequest`.
- **The first letter decides.** A name that starts with a capital letter is read as a type, so a
  local cannot be capitalized and a type cannot start lowercase. A reader can tell a type from a
  value without finding its declaration.
- **Messages.** A name in another case is refused with its kebab-case spelling: ``itemCount` is
  not a Brix name. A name is lowercase words joined by hyphens: write `item-count`.``
- **No `?` or `!`.** A name cannot hold `?` or `!`. In rows, `value!` forces an optional and
  `?.` chains one.
- **Reserved words.** `in`, `on`, `and`, `or`, `do`, `for`, `while`, `if` and `else` cannot name a
  local or a parameter. Every other word can, including `match`, `from`, `to`, `list` and
  `print`.
- **C names** are written `C:name`, in C's own spelling, as in `C:getenv`.
- **Generic names in rows.** Rows write a type's arguments after an apostrophe, which needs no
  closing bracket and leaves `<` and `>` to comparison: `List'Int` is `(of List Int)`, and
  `Map'String'Int` is `(of Map String Int)`. The commonest types have
  shorter spellings: `[Int]` is `(list Int)`, `[String = Int]` is `(map String Int)`, `<Int>` is
  `(stream Int)` and `Int?` is `(? Int)`.

### Names with a meaning

- **`This`** is the type being declared. A constructor is a function named `This`, and
  `(call This count)` makes another of the type.
- **`this`** is the object a method was called on, as in `(+= (. this count) 1)`{.brix}. A
  property of the type can also be named bare inside the type.
- **`each` and `it`** name the value in a pipe stage, a callback or a predicate arm.
- **`_`** is the hole (see [Atoms](#atoms)), and at the start of a declaration it means private.

### Shadowing

- **One name to a scope.** Declaring a local twice in one scope is an error: `"x" is already
  declared on line 1 in this scope`.
- **Inner scopes may shadow.** A local in a nested block, a loop item or a `do` body may reuse the
  name of an outer local, a parameter or an imported function. The inner one is used until its
  block ends.

## Comments

```brix
; A comment runs from a semicolon to the end of its line.
(main
  ; It has a row to itself, like any other row.
  (print "one")
  (blank)
  (print "two")
)
```

```output
one
two
```

- **In forms**, a comment runs from `;` to the end of the line. It may come between any two items,
  but the usual layout gives it a row of its own. A `;` inside a text is part of the text.
- **In rows**, a comment is a line whose first character, after any spaces, is a backtick, as in
  `` ` A comment``. It has a line to itself; there is no comment after code on the same line.
- **Not comments.** In rows, `#` is refused with `Comments start with a backtick`, and `//` starts
  a probe; see Probes.
- **Doc comments.** By convention, comment rows right under a declaration's header describe it.
  The compiler gives them no meaning.
- **Blank rows.** `(blank)` is an empty row, kept so that the rows view can show it. It
  does nothing.

## How rows and forms relate

| | Rows | Forms |
| --- | --- | --- |
| Function | `+ double Int`, a parameter `- n Int`, and the body `n * 2` | `(fn-expr + double Int (param n Int) (* n 2))`{.brix} |
| Step function | `+ do total Int` | `(fn-do + total Int …)` |
| Module value | `+ limit Int = 3` | `(prop + limit Int (= 3))`{.brix} |
| Local | `. shapes = [Square(2), Square(3)]` | `(let shapes (list (call Square 2) (call Square 3)))`{.brix} |
| Call | `describe(2)` | `(call describe 2)`{.brix} |
| Member | `shape.area` | `(. shape area)`{.brix} |
| Text with values | `"$a and $b"` | `(str a " and " b)`{.brix} |
| Choice | `if flag then "yes" else null` | `(if-expr flag "yes" null)`{.brix} |
| Named operator | `3 /max/ 4` | `(/max/ 3 4)`{.brix} |
| Function value | `:: x Int => x * 2` | `(=> ((: x Int)) (* x 2))`{.brix} |

- **Statements become `(main …)`.** The statements at a file's top level, before the first divider
  or after a thick divider and a blank line, are a `(main …)` form. Each run of them is one form,
  in file order.
- **Nesting.** Forms nest by parentheses. Rows use operator precedence, and add parentheses where
  the forms need them: `(+ (* a a) (* b b))` is `a * a + b * b`, and `(* (+ a b) 2)` is
  `(a + b) * 2`.
- **Layout is not carried over.** Number spellings such as `1'000` and `0xff` survive a trip from
  one view to the other and back. Several blank rows become one, several `(main …)` forms in a
  row become one, and a comment written after code moves to a row of its own.
- **Converting.** `brix brix file.yumi` writes `file.bx` beside the rows file, replacing any that
  is there.
