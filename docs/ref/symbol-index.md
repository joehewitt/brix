# Symbol index

Every symbol of Brix, with what it means and the entry that explains it: the operators, the marks
that begin an atom, the marks that begin a row, the names with a fixed meaning, and the functions
the core library gives every module. Forms are listed by their heads in the
[form index](form-index.md). Where a symbol is spelled one way in forms and another in rows, both
are given.

## Operators

| Symbol | Form | Meaning | See |
| --- | --- | --- | --- |
| `+` | `(+ a b)` | Addition, and a list with one more item. | [Arithmetic](operators.md#arithmetic) |
| `-` | `(- a b)`, `(- a)` | Subtraction, and negation. | [Arithmetic](operators.md#arithmetic) |
| `*` | `(* a b)` | Multiplication. | [Arithmetic](operators.md#arithmetic) |
| `/` | `(/ a b)` | Division. | [Arithmetic](operators.md#arithmetic) |
| `%` | `(% a b)` | Remainder. | [Arithmetic](operators.md#arithmetic) |
| `%%` | `(%% a b)` | Power. | [Arithmetic](operators.md#arithmetic) |
| `++` | `(++ a b)` | Joins texts or collections. | [Joining and membership](operators.md#joining-and-membership), [Operators on text](text.md#operators-on-text) |
| `==`, `!=` | `(== a b)`, `(!= a b)` | Equality. | [Comparison](operators.md#comparison) |
| `<`, `>`, `<=`, `>=` | `(< a b)` | Order; chains, as `(< a b c)`. | [Comparison](operators.md#comparison) |
| `&&`, `\|\|`, `!` | `(and a b)`, `(or a b)`, `(not a)` | Logic, in rows. | [Logic](operators.md#logic) |
| `/name/`, `//name//`, `///name///` | `(/name/ a b)` | A function of two parameters used as an operator. | [Named operators](operators.md#named-operators) |
| `/and/`, `/or/`, `/xor/`, `/not/`, `/lshift/`, `/rshift/`, `/zrshift/` | `(/and/ a b)` | Bitwise operations. | [Bits](operators.md#bits) |
| `<<`, `>>`, `<\|` | `(<< a b)` | Read as operators; no built-in type defines them. | [Reserved operators](operators.md#reserved-operators) |
| `?<<`, `?>>` | `(?<< a b)` | Reserved. | [Reserved operators](operators.md#reserved-operators) |
| `=` | `(= target value)` | Assignment. | [Locals](control-flow.md#locals) |
| `+=`, `-=`, `*=`, `/=`, `%=`, `%%=`, `++=` | `(+= target value)` | Update in place; on a list, `+=` adds an item and `-=` removes one. | [Precedence](operators.md#precedence), [Changing a list](collections.md#changing-a-list) |
| `=?`, `+=?` and each update with `?` | `(=? target value)` | An assignment skipped when the value is `null`. | [Steps that skip null](optionals.md#steps-that-skip-null) |
| `<-` | `(<- target value)`, `(<- source)` | Sends a value to a stream or channel, and reads one. | [Targets](streams.md#targets), [Channels](concurrency.md#channels) |
| `?<-` | `(?<- target value)` | Sends to a channel that may be `null`. | [Channels](concurrency.md#channels) |
| `<-*` | `(<-* target values)` | Sends every value of a stream. | [Targets](streams.md#targets) |
| `<->` | `(<-> property place)` | Binds a property of a markup element to a place both ways. | [Bindings](markup.md#bindings) |
| `..`, `..<` | `(.. a b)`, `(..< a b)` | Ranges, through and up to the end. | [Ranges](control-flow.md#ranges), [Ranges as values](collections.md#ranges-as-values) |
| `...` | `(... a)` | A range with no end. | [Ranges](literals.md#ranges) |
| `.` | `(. object name)` | A member. | [Using members](types.md#using-members) |
| `[]` | `([] xs index)` | An item, or a slice. | [Reading items](collections.md#reading-items), [Slices](collections.md#slices) |
| `?.`, `?[]` | `(?. object name)` | A member or item of a value that may be `null`. | [Chains](optionals.md#chains) |
| `??` | `(?? a b)` | A fallback for `null`. | [Fallbacks](optionals.md#fallbacks) |
| `!` (suffix in rows), `!!` | `(!! a)` | Forces an optional. | [Forcing](optionals.md#forcing) |
| `?` (suffix of a type) | `(? Type)` | An optional type. | [Optional types](optionals.md#optional-types) |
| `*.` | `(*. xs name)` | The member of every item. Not built yet. | [Pipelines](streams.md#pipelines) |
| `=>` | `(=> (a) value)` | A function value. | [Function values](functions.md#function-values) |
| `\|>`, `\|=>` | `(\|> a f)` | Passes a value to a function, or a stream whole. | [Pipelines](streams.md#pipelines) |
| `*>`, `?>`, `?*>`, `*\|>` | `(*> xs value)` | Pipeline stages: map, filter, map and drop `null`, and tap. | [Pipelines](streams.md#pipelines) |
| `->`, `*->` | `(-> value target)` | Sends a value, or each value, on to a target. | [Targets](streams.md#targets) |
| `<*` | `(<* pattern)` | Every match of a pattern, in a comprehension. | [Comprehensions](streams.md#comprehensions) |
| `\|` | `(\| a b)` | Alternatives in a pattern, and a pattern in a comprehension. | [Kinds of pattern](patterns.md#kinds-of-pattern), [Comprehensions](streams.md#comprehensions) |
| `~` | `(~ subject pattern)` | Matches a stream pattern. | [Matching text](stream-patterns.md#matching-text) |
| `~>` | `(~> source \rule)` | Reads records with a rule. | [Reading records](stream-patterns.md#reading-records) |
| `!`, `&`, `{}` in a stream pattern | `(! p)`, `(& p)`, `({} p 2 4)` | Not followed by, followed by, and a count. | [Pattern pieces](stream-patterns.md#pattern-pieces) |
| `\{ test }` | `(\{} test)` | A test on the next item. | [In rows](stream-patterns.md#in-rows) |
| `/`, `*/`, `\`, `*\`, `++`, `--` in a pattern | `(/ a b)` | Tree relations. Reserved. | [Kinds of pattern](patterns.md#kinds-of-pattern) |
| `?!` | `(throw error (if condition))` | Raises an error, in rows. | [Raising](errors.md#raising) |
| `#` | `#name` | A tracked read. | [Tracked reads](reactivity.md#tracked-reads) |
| `##` | `(## stream)` | The latest value of a stream, followed. | [Streams of a value](reactivity.md#streams-of-a-value) |

## Marks in atoms

| Symbol | Example | Meaning | See |
| --- | --- | --- | --- |
| `+`, `_` | `(fn-expr + name …)` | Public and private. | [Atoms](source-text.md#atoms), [Visibility](modules.md#visibility) |
| `_` | `(param name _)` | The hole: a type to infer, or a value with no name. | [Atoms](source-text.md#atoms) |
| `'` | `'A'` | A character. | [Characters](literals.md#characters) |
| `''` | `''T` | A type parameter. | [Generic functions](functions.md#generic-functions) |
| `'` in rows | `List'Int` | Type arguments, `(of List Int)` in forms. | [Names](source-text.md#names), [Generic types](types.md#generic-types) |
| `.` | `.red` | A symbol or an enum case. | [Enums](unions.md#enums) |
| `..` | `..open` | An anonymous case. | [Anonymous cases](unions.md#anonymous-cases) |
| `#` | `#count` | A tracked read. | [Tracked reads](reactivity.md#tracked-reads) |
| `&` | `&next` | A pointer to a local. | [Pointers](native-code-and-memory.md#pointers) |
| `$` | `$font` | A style read. | [Reading style values](styles.md#reading-style-values) |
| `%` | `%parent` | A markup read: an element's neighbor. | [Links between elements](markup.md#links-between-elements) |
| `C:` | `C:getenv` | A C name. | [Calling C](native-code-and-memory.md#calling-c) |
| `:` | `:get`, `:static` | A flag in a declaration's header. | [Members](types.md#members) |
| `:` before a value | `(:name value)` | A named argument. | [Calls](functions.md#calls) |
| `name:` | `(text: "Hi")` | A labeled part of a form, such as an attribute. | [Attributes](markup.md#attributes) |
| `@` | `@inline` | An annotation. | [Annotations](functions.md#annotations) |
| `\` | `\num` | A stream-pattern rule used as a value. | [Rules](stream-patterns.md#rules) |
| `0x`, `0b`, `0o`, `'` | `0xff`, `1'000` | Integers in other bases, and a digit separator. | [Integers](literals.md#integers) |
| A suffix | `200u8`, `3km` | A literal's type, or its unit. | [Suffixes](literals.md#suffixes), [Unit literals](units.md#unit-literals) |
| `"` | `"text"` | Text. | [Text](literals.md#text) |
| `\` in text | `"\n"` | An escape. | [Text](literals.md#text) |
| `$` in rows text | `"Hello, $name"` | A value written into text. | [Building text from values](text.md#building-text-from-values) |
| `;` | `; note` | A comment in forms. | [Comments](source-text.md#comments) |

## Marks that begin a row

| Row | Form | Meaning | See |
| --- | --- | --- | --- |
| `+ name …`, `_ name …`, `^ name …` | `(fn-expr + name …)` | A public, private or shared declaration. | [Declaring a function](functions.md#declaring-a-function) |
| `- name Type` | `(param name Type)` | A parameter. | [Parameters](functions.md#parameters) |
| `... name Type` | `(param ... name Type)` | The rest parameter. | [Parameters](functions.md#parameters) |
| `. name = value` | `(let name value)` | A local. | [Locals](control-flow.md#locals) |
| `.. name = value` | `(lazy name value)` | A lazy binding. | [Lazy bindings](functions.md#lazy-bindings) |
| `= value` | `(return value)` | A return. | [Returns](functions.md#returns) |
| `-- value` | `(print value)` | A print. | [Printing](text.md#printing) |
| `\| pattern` | `(case pattern …)`, `(variant …)` | A match arm, or a union case. | [`match`](patterns.md#match), [Declaring a union](unions.md#declaring-a-union) |
| `\|= value`, `\|? condition`, `??` | `(case= …)`, `(case? …)`, `(default …)` | Arms taken on equality, on a condition, and otherwise. | [Decision tables](control-flow.md#decision-tables) |
| `\|\| pattern`, `\|\|= value`, `\|\|? condition` | `(case-union …)` | Arms whose values keep their own types. | [Arms that keep their type](patterns.md#arms-that-keep-their-type) |
| `?! error if condition` | `(throw error (if condition))` | A check. | [Raising](errors.md#raising) |
| `* value`, `** values` | `(bullet value)`, `(bullet* values)` | Items of a list built below a value. | [Pipelines in rows of a block](streams.md#pipelines-in-rows-of-a-block) |
| `?> condition`, `*> value` | `(pipe ?> condition)` | Pipe stages below a value. | [Pipelines in rows of a block](streams.md#pipelines-in-rows-of-a-block) |
| `• name value` | `(:name value)` | A named argument. | [Calls](functions.md#calls) |
| `- value` under a call | `(arg value)` | One argument. | [Calls](functions.md#calls) |
| `/path`, `./path` | `(import /path)` | An import. | [Imports](modules.md#imports) |
| `<Name` | `(<> Name …)` | An element of markup. | [Elements](markup.md#elements) |
| `• Name` | `(style Name …)` | A style sheet. | [Style sheets](styles.md#style-sheets) |
| `• name` on an element | `(• name)` | A style rule the element uses. | [Which rules apply](styles.md#which-rules-apply) |
| `//`, `//key//`, `//key ->`, `//key =`, `//key ++`, `//+key`, `//-key`, `//+-key`, `//!key` | `(probe-log …)` and the other probe forms | A probe. | [Kinds of probe](probes.md#kinds-of-probe) |
| `` ` text `` | `; text` | A comment. | [Comments](source-text.md#comments) |
| `━━━━`, `────` | | Dividers between declarations. | [Dividers](source-text.md#dividers) |
| A blank line | `(blank)` | An empty row. | [Comments](source-text.md#comments) |
| `:` after a header | `(inline …)` | A body of one step on the same line. | [Lines](source-text.md#lines) |

## Names with a meaning

| Name | Meaning | See |
| --- | --- | --- |
| `this` | The object a method was called on. | [Names with a meaning](source-text.md#names-with-a-meaning) |
| `This` | The type being declared, and its constructor. | [Constructors](types.md#constructors) |
| `super` | The base class's constructor and methods. | [Inheritance](types.md#inheritance) |
| `each`, `it` | The value in a pipe stage, a callback or a predicate arm. | [Names with a meaning](source-text.md#names-with-a-meaning) |
| `true`, `false`, `null` | The two truths, and the absent value. | [Truths and null](literals.md#truths-and-null) |
| `main` | The code that runs when the file runs. | [The code a file runs](modules.md#the-code-a-file-runs) |
| `inherit` | In a style rule, the parent's value. | [What a rule sets](styles.md#what-a-rule-sets) |
| `in`, `on`, `and`, `or`, `do`, `for`, `while`, `if`, `else` | Reserved: they cannot name a local or a parameter. | [Names](source-text.md#names) |

## Functions core gives

These are visible in every module without an import.

| Function | Does | See |
| --- | --- | --- |
| `print`, `print-all` | Prints a value, or each item of a stream. | [Printing](text.md#printing) |
| `request-process-termination` | Ends the program with a status. | [How a program ends](modules.md#how-a-program-ends) |
| `when-main-ends` | Runs an action when `main` ends. | [When a program ends](concurrency.md#when-a-program-ends) |
| `usable-core-count` | The number of cores a program keeps busy. | [Workers](concurrency.md#workers) |
| `get-current-task-id` | The running task's id. | [Functions core gives](built-in-types.md#functions-core-gives) |
| `await` | Runs a function in the thread pool and waits for it. | [Waiting for a result](concurrency.md#waiting-for-a-result) |
| `unhandled-error`, `print-unhandled-error` | What is done with an error that has no caller. | [Errors with no caller](errors.md#errors-with-no-caller) |
| `join` | Joins items into text. | [String members](text.md#string-members) |
| `decode-base64` | Base64 text to bytes. | [Functions core gives](built-in-types.md#functions-core-gives) |
| `hash-for` | The hash of a key. | [Keys](collections.md#keys) |
| `map`, `filter`, `map-filter`, `tap` | The functions behind `*>`, `?>`, `?*>` and `*\|>`. | [Pipelines](streams.md#pipelines) |
| `map-do`, `flat-map`, `publisher` | A step for each item, the items of a stream of streams, and a stream as a broadcaster. | [Functions core gives](built-in-types.md#functions-core-gives) |
| `observe-collection`, `unobserve-collection` | Watch a collection's changes. | [Watching a collection](built-in-types.md#watching-a-collection) |
| `identity`, `type-identity` | Gives its argument; tests whether a value is of a type. | [Type information](built-in-types.md#type-information) |
| `live-state`, `keep-live-state` | State a live function keeps between runs. | [Live functions](reactivity.md#live-functions) |
