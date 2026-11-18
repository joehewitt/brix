# Form index

Every Brix form, by its head, with its shape, what it means, and the entry that explains it. The
forms are grouped as the compiler's form catalog groups them, and a head with several shapes has a
row for each.

A shape names each slot of the form with the kind of thing it takes, as `name:Kind`. A slot
followed by `?` may be left out, `*` repeats it any number of times, and `+` at least once. `/`
joins the kinds a slot accepts, as in `Expr/Block`. A word in the shape that has no kind, such as
`This` or `:static`, is written as it stands. Operators and other symbols, and how they are written
in rows, are in the [symbol index](symbol-index.md).

A form marked "Not built yet" is in the catalog, but the compiler does not build what it means yet:
a program that uses it is refused or fails to build, except `*.`, which reads only one member. A
form marked "Not supported yet" is refused with a message. A form marked "Reserved" is read, and
kept for a meaning to come.


## Files

| Head | Shape | Meaning | See |
| --- | --- | --- | --- |
| `import` | `(import path:Name)` | Use another module's public names in this file. | [Modules: Imports](modules.md#imports) |
| `export` | `(export path:Name)` | Pass another module's public names on to whoever imports this file. | [Modules: Exports](modules.md#exports) |
| `prop` | `(prop access:Access name:Name type:Type/Hole deco:Deco* value:Init?)` | A value that belongs to the file. | [Modules: Values that belong to a file](modules.md#values-that-belong-to-a-file) |
| `main` | `(main row:Step*)` | The code that runs when the file runs. Rows run in order. | [Modules: The code a file runs](modules.md#the-code-a-file-runs) |
| `main` | `(main row:Value/Lazy*)` | The same, when the file's code is one pipeline. | [Modules: The code a file runs](modules.md#the-code-a-file-runs) |
| `unit` | `(unit access:Access name:Atom value:Expr)` | States that an amount of one unit equals an amount of another, so values convert between them. | [Units: Equalities](units.md#equalities) |
| `effect` | `(effect access:Access name:Name claim:Claim*)` | An effect: a kind of contact with the world outside the program. Its claims are the C functions that only its doors may call, such as `C:connect`, and `without` keeps code from reaching its doors. | [Functions: Effects and doors](functions.md#effects-and-doors) |
| `blank` | `(blank)` | An empty row, kept for the reader. | [Source text: Comments](source-text.md#comments) |

## Functions

| Head | Shape | Meaning | See |
| --- | --- | --- | --- |
| `fn-do` | `(fn-do access:Access name:Key result:Type/Hole spread:Over? :static? raises:Raises* deco:Deco* scope:Scope* :throws? header:Header* row:Step*)` | A function that runs its rows in order and returns with `return`. | [Functions: Step bodies](functions.md#step-bodies) |
| `fn-expr` | `(fn-expr access:Access name:Key result:Type/Hole spread:Over? :static? raises:Raises* deco:Deco* scope:Scope* :throws? header:Header* row:Lead* body:Value? stage:Stage*)` | A function whose body is the value it produces. | [Functions: Value bodies](functions.md#value-bodies) |
| `fn-expr` | `(fn-expr access:Access name:Key result:Type/Hole spread:Over? :static? raises:Raises* deco:Deco* scope:Scope* :throws? header:Header* row:Lazy+)` | A function whose body only names values, such as a state machine it builds. | [Functions: Lazy bindings](functions.md#lazy-bindings) |
| `fn-expr` | `(fn-expr access:Access name:Key result:Type/Hole spread:Over? :static? raises:Raises* deco:Deco* scope:Scope* :throws? header:Header* (header value:Expr) row:Value/Lazy*)` | A function whose body is sections, each a `header` and the values handed to it. | [Functions: Value bodies](functions.md#value-bodies) |
| `fn-expr` | `(fn-expr access:Access name:Key result:Type/Hole spread:Over? :static? raises:Raises* deco:Deco* scope:Scope* :throws? header:Header* (before row:Before+) ? row:Lead* body:Value? stage:Stage* (where row:Binding+) ?)` | A function whose body is the value it produces, after the checks in `before` and with the bindings in `where`. | [Functions: Rows before the value](functions.md#rows-before-the-value) |
| `fn-do` | `(fn-do access:Access This result:Type/Hole raises:Raises* deco:Deco* scope:Scope* header:Header* row:Step/Value*)` | The constructor. Its rows set the properties of a new instance. | [Types: Constructors](types.md#constructors) |
| `fn-do` | `(fn-do access:Access (of This parameter:Type+) result:Type/Hole raises:Raises* deco:Deco* scope:Scope* header:Header* row:Step/Value*)` | A constructor for one choice of type parameters. | [Types: Constructors](types.md#constructors) |
| `fn-do` | `(fn-do access:Access name:Key result:Type/Hole :get raises:Raises* deco:Deco* scope:Scope* row:Step*)` | A property computed by running rows. | [Types: Getters and setters](types.md#getters-and-setters) |
| `fn-expr` | `(fn-expr access:Access name:Key result:Type/Hole :get raises:Raises* deco:Deco* scope:Scope* row:Lead* body:Value? stage:Stage*)` | A property computed from a value. | [Types: Getters and setters](types.md#getters-and-setters) |
| `fn-expr` | `(fn-expr access:Access name:Key result:Type/Hole :get raises:Raises* deco:Deco* scope:Scope* (before row:Before+) ? row:Lead* body:Value? stage:Stage* (where row:Binding+) ?)` | A property computed from a value, after the checks in `before` and with the bindings in `where`. | [Types: Getters and setters](types.md#getters-and-setters) |
| `fn-do` | `(fn-do access:Access name:Key result:Hole :set value:Param row:Step*)` | What happens when the property is assigned. | [Types: Getters and setters](types.md#getters-and-setters) |
| `fn-expr` | `(fn-expr access:Access name:Key result:Hole :set value:Param row:Lead* body:Value? stage:Stage* bind:Lazy*)` | What happens when the property is assigned, as one value. | [Types: Getters and setters](types.md#getters-and-setters) |
| `fn-from-do` | `(fn-from-do access:Access source:Binder deco:Deco* header:Header* row:Step*)` | A conversion from another type, run as rows. | [Conversions: Declared conversions](conversions.md#declared-conversions) |
| `fn-from-expr` | `(fn-from-expr access:Access source:Binder deco:Deco* header:Header* row:Lead* body:Value? stage:Stage*)` | A conversion from another type, as a value. | [Conversions: Declared conversions](conversions.md#declared-conversions) |
| `fn-from-expr` | `(fn-from-expr access:Access source:Binder deco:Deco* header:Header* (before row:Before+) ? row:Lead* body:Value? stage:Stage* (where row:Binding+) ?)` | A conversion from another type, as a value, after the checks in `before` and with the bindings in `where`. | [Conversions: Declared conversions](conversions.md#declared-conversions) |
| `fn-to-do` | `(fn-to-do access:Access type:Type deco:Deco* scope:Scope* header:Header* row:Step*)` | A conversion to another type, run as rows. | [Conversions: Declared conversions](conversions.md#declared-conversions) |
| `fn-to-expr` | `(fn-to-expr access:Access type:Type deco:Deco* scope:Scope* header:Header* row:Lead* body:Value? stage:Stage*)` | A conversion to another type, as a value. | [Conversions: Declared conversions](conversions.md#declared-conversions) |
| `fn-to-expr` | `(fn-to-expr access:Access type:Type deco:Deco* scope:Scope* header:Header* (before row:Before+) ? row:Lead* body:Value? stage:Stage* (where row:Binding+) ?)` | A conversion to another type, as a value, after the checks in `before` and with the bindings in `where`. | [Conversions: Declared conversions](conversions.md#declared-conversions) |
| `param` | `(param mark:Mark? name:Name/Hole type:Type/Hole default:Default?)` | A parameter: its name, its type, and its default when it has one. | [Functions: Parameters](functions.md#parameters) |
| `constraint` | `(constraint op:Atom left:Type right:Any)` | A rule that a type parameter must meet. | [Functions: Constraints](functions.md#constraints) |
| `iterates` | `(iterates name:Atom source:Expr)` | Makes the body of the function a loop over the source. The body runs for each item, and the function gives the stream of results. | [Streams: Functions whose body is a comprehension](streams.md#functions-whose-body-is-a-comprehension) |
| `?!` | `(?! error:Type)` | An error the function can raise. | [Errors: Declaring errors](errors.md#declaring-errors) |
| `over:` | `(over: type:Type)` | The item type of the stream the function matches. | [Grammars: Rules](grammars.md#rules) |
| `without` | `(without name:Name+)` | Keep the code from what is named, checked as the program compiles: `heap`, `suspend`, `gc`, `safepoints`, `probe-safepoints`, or an effect a library declares, whose doors it may not reach. On a C function, `gc` and `blocking` are promises the compiler trusts. In a `do`, `cancel` holds off cancellation. | [Functions: Without and with](functions.md#without-and-with) |
| `claim` | `(claim function:Name)` | Claim the C function, such as `C:connect`, for the effect: only its doors may call it. | [Functions: Effects and doors](functions.md#effects-and-doors) |
| `door` | `(door effect:Name)` | Make the function a door of the effect: one through which the program makes that kind of contact with the world, and the only kind of function that may call the C functions the effect claims. | [Functions: Effects and doors](functions.md#effects-and-doors) |
| `with` | `(with name:Name value:Expr)` | Set the stack value `name` to `value` for the code and what it calls, as `do-with` does, or bind a fake to the effect `name`. | [Concurrency: Values for each thread, task or stack](concurrency.md#values-for-each-thread-task-or-stack) |
| `=` | `(= value:Expr/Block)` | The value. | [Types: Stored properties](types.md#stored-properties) |
| `:=` | `(:= value:Expr/Block)` | A value computed once, the first time it is read. | [Types: Lazy properties](types.md#lazy-properties) |
| `=>` | `(=> value:Expr/Body)` | A value computed each time it is read. | [Types: Computed properties](types.md#computed-properties) |
| `=>` | `(=> :do body:Step)` | A value computed each time it is read, by running one step. Not supported yet. | [Types: Computed properties](types.md#computed-properties) |
| `=>` | `(=> :do (block row:Step*))` | A value computed each time it is read, by running steps. | [Types: Computed properties](types.md#computed-properties) |
| `machine` | `(machine state:State*)` | A state machine. | [State machines: Where a machine lives](state-machines.md#where-a-machine-lives) |
| `block` | `(block row:Row/Value*)` | Rows that together make one value or one list of arguments. | [Functions: Calls](functions.md#calls) |
| `block` | `(block row:Before* row:Lead* body:Answer? stage:Stage* bind:Lazy*)` | The value a function value gives, after its checks, prints, probes and `do` forms and before the lazies it uses. | [Functions: Function values](functions.md#function-values) |
| `infix` | `(infix left:Name op:Atom right:Param)` | Defines a binary operator. | [Operators: Operators a type defines](operators.md#operators-a-type-defines) |
| `prefix` | `(prefix op:Atom operand:Name)` | Defines a prefix operator. | [Operators: Operators a type defines](operators.md#operators-a-type-defines) |
| `index-get` | `(index-get target:Name index:Param)` | Defines reading `target[index]`. | [Operators: Operators a type defines](operators.md#operators-a-type-defines) |
| `index-set` | `(index-set target:Name index:Param value:Param)` | Defines assigning `target[index]`. | [Operators: Operators a type defines](operators.md#operators-a-type-defines) |
| `index-delete` | `(index-delete target:Name index:Param)` | Defines removing `target[index]`. | [Operators: Operators a type defines](operators.md#operators-a-type-defines) |

## Types

| Head | Shape | Meaning | See |
| --- | --- | --- | --- |
| `type` | `(type access:Access name:Type clause:TypeClause* member:Member*)` | A type and its members. | [Types: Declaring a type](types.md#declaring-a-type) |
| `type-line` | `(type-line access:Access name:Type clause:TypeClause*)` | A type with no body, such as an alias. | [Types: Declaring a type](types.md#declaring-a-type) |
| `extend` | `(extend access:Access type:Type clause:TypeClause* member:Member*)` | Members added to a type declared elsewhere. | [Types: Extensions](types.md#extensions) |
| `extend-line` | `(extend-line access:Access type:Type clause:TypeClause*)` | An extension with no body. Not built yet. | [Types: Extensions](types.md#extensions) |
| `is` | `(is base:Type/Expr+)` | What the type is built on. Several types make the type stand for any one of them. | [Types: Kinds of type](types.md#kinds-of-type) |
| `facet-of` | `(facet-of owner:Type)` | Makes the type a facet of a class, or of another facet of it: a part of the class declared in a module of the class's file family. | [Types: Facets](types.md#facets) |
| `alias` | `(alias name:Atom+)` | Unit suffixes that make a number a value of this type, and other names for the type. | [Types: Declaring a type](types.md#declaring-a-type) |
| `as` | `(as type:Type+)` | Types the extension makes the type usable as. Not built yet. | [Types: Extensions](types.md#extensions) |
| `where` | `(where rule:Header+)` | Rules the extended type's parameters must meet. | [Types: Extensions](types.md#extensions) |
| `where` | `(where row:Binding+)` | The bindings an expression body uses, after its value. | [Functions: Lazy bindings](functions.md#lazy-bindings) |
| `before` | `(before row:Before+)` | The checks, prints, probes and `do` forms that run before the value of an expression body. Any other step goes in a `(do …)`. | [Functions: Rows before the value](functions.md#rows-before-the-value) |
| `prop` | `(prop access:Access name:Keyed/Hole type:Type/Hole deco:Deco* value:Init?)` | A value each instance of the type holds. | [Types: Stored properties](types.md#stored-properties) |
| `variant` | `(variant name:Name/Type payload:Type? value:Init?)` | One case of a union. | [Unions and enums: Declaring a union](unions.md#declaring-a-union) |
| `event` | `(event access:Access name:Name type:Type key:Type?)` | An event the type sends. | [Reactivity: Events](reactivity.md#events) |
| `content` | `(content row:Step/Member/State*)` | The rows that build an instance's contents. | [Markup: Components](markup.md#components) |
| `when-rule` | `(when-rule condition:Expr row:Step*)` | Rows that run each time what the condition reads changes, while the condition holds. | [Reactivity: Rules](reactivity.md#rules) |
| `whenever-rule` | `(whenever-rule source:Expr/Trigger header:Header* row:Step*)` | Rows that run each time the source changes. | [Reactivity: Rules](reactivity.md#rules) |
| `subscribe-for` | `(subscribe-for source:Clause arm:SourceArm* row:Step*)` | Rows that run for each item the source sends. | [Reactivity: Events](reactivity.md#events) |
| `and-for` | `(and-for source:Clause)` | A second source walked together with the first. | [Streams: Comprehensions](streams.md#comprehensions) |
| `subscribe` | `(subscribe source:Clause row:Step*)` | Rows that run each time the source sends. | [Reactivity: Events](reactivity.md#events) |
| `implements` | `(implements type:Type)` | An interface the type provides. | [Types: Implementing an interface](types.md#implementing-an-interface) |
| `using` | `(using name:Type)` | A style sheet or helper the type uses. | [Styles: How styles reach elements](styles.md#how-styles-reach-elements) |
| `delegate` | `(delegate type:Type+)` | Interfaces the type provides by passing them to the property on the next row. | [Types: Delegation](types.md#delegation) |
| `sees` | `(sees facet:Type)` | A sibling facet whose public members the facet reaches. | [Types: What a facet reaches](types.md#what-a-facet-reaches) |
| `style-domains` | `(style-domains name:Name domain:Domain+)` | The work that is redone when the property changes, such as layout or paint. | [Styles: Domains](styles.md#domains) |
| `domain` | `(domain name:Name target:Name?)` | One kind of work, and the related node it is redone on when one is named. | [Styles: Domains](styles.md#domains) |
| `depends` | `(depends name:Name on:Name+)` | A method of the type that is called whenever one of the named properties changes. | [Reactivity: Rules](reactivity.md#rules) |
| `unit` | `(unit access:Access name:Atom value:Expr)` | States that an amount of one unit equals an amount of another, so values convert between them. | [Units: Equalities](units.md#equalities) |

## Statements

| Head | Shape | Meaning | See |
| --- | --- | --- | --- |
| `let` | `(let target:Binder/Pattern value:Expr/Block?)` | Introduce a local and set it now. A pattern as the target takes the value apart into the names it captures. | [Control flow: Locals](control-flow.md#locals) |
| `let` | `(let name:Local type:Type/Hole value:Expr/Block?)` | Introduce a local of the type, and set it now when a value follows. A `_` type leaves the type to the value, as a value that is itself a type needs: `(let kind _ I32)`. | [Control flow: Locals](control-flow.md#locals) |
| `lazy` | `(lazy target:Binder/Pattern value:Expr/Block/Rule?)` | Name a value that is computed where it is first used. | [Functions: Lazy bindings](functions.md#lazy-bindings) |
| `lazy` | `(lazy name:Local type:Type value:Expr/Block/Rule?)` | Name a value of the type that is computed where it is first used. | [Functions: Lazy bindings](functions.md#lazy-bindings) |
| `pattern` | `(pattern mark:Atom* alternative:Stream*)` | A grammar rule. | [Grammars: Rules](grammars.md#rules) |
| `return` | `(return value:Expr/Block?)` | Leave the function, with a value when it returns one. | [Functions: Returns](functions.md#returns) |
| `return?` | `(return? value:Expr/Block)` | Leave the function with the value, unless the value is null. | [Functions: Returns](functions.md#returns) |
| `return-if` | `(return-if value:Expr? guard:Guard)` | Leave the function when the condition holds. | [Functions: Returns](functions.md#returns) |
| `if` | `(if condition:Test)` | The condition. | [Control flow: Values chosen by a condition](control-flow.md#values-chosen-by-a-condition) |
| `yield` | `(yield value:Expr/Block)` | Send the next value of a stream. | [Streams: Yields](streams.md#yields) |
| `yield?` | `(yield? value:Expr/Block)` | Send the value unless it is null. | [Streams: Yields](streams.md#yields) |
| `yield-all` | `(yield-all value:Expr/Block)` | Send every value of another stream. | [Streams: Yields](streams.md#yields) |
| `yield-all?` | `(yield-all? value:Expr/Block)` | Send every value of another stream unless it is null. Not built yet. | — |
| `break` | `(break)` | Leave the nearest loop. | [Control flow: break and continue](control-flow.md#break-and-continue) |
| `continue` | `(continue)` | Go to the next turn of the nearest loop. | [Control flow: break and continue](control-flow.md#break-and-continue) |
| `if` | `(if condition:Test row:Step+ arm:Arm*)` | Run the rows when the condition holds. The arms are tried otherwise. | [Control flow: if and else](control-flow.md#if-and-else) |
| `if` | `(if condition:Test body:Inline arm:Arm*)` | The same, with a one-step body. | [Control flow: if and else](control-flow.md#if-and-else) |
| `else-if` | `(else-if condition:Test row:Step*)` | Tried when no condition before it held. | [Control flow: if and else](control-flow.md#if-and-else) |
| `else-if` | `(else-if condition:Test body:Inline)` | The same, with a one-step body. | [Control flow: if and else](control-flow.md#if-and-else) |
| `else` | `(else row:Step*)` | Run when no condition held. | [Control flow: if and else](control-flow.md#if-and-else) |
| `else` | `(else body:Inline)` | The same, with a one-step body. | [Control flow: if and else](control-flow.md#if-and-else) |
| `inline` | `(inline body:Step/Expr)` | A one-step body. | [Functions: Calls](functions.md#calls) |
| `if` | `(if condition:Test row:Child* arm:ChildArm*)` | Inside markup: the rows are children added when the condition holds. | [Markup: Control flow in markup](markup.md#control-flow-in-markup) |
| `else-if` | `(else-if condition:Test row:Child*)` | Inside markup: children added when no condition before it held. | [Markup: Control flow in markup](markup.md#control-flow-in-markup) |
| `else` | `(else row:Child*)` | Inside markup: children added when no condition held. | [Markup: Control flow in markup](markup.md#control-flow-in-markup) |
| `while` | `(while condition:Test row:Step+)` | Run the rows as long as the condition holds. | [Control flow: while](control-flow.md#while) |
| `while` | `(while condition:Test body:Inline)` | The same, with a one-step body. | [Control flow: while](control-flow.md#while) |
| `for` | `(for source:Clause row:Step* arm:LoopArm*)` | Run the rows once for each item of the source. Rows may instead be in its last `and-for` or in its `or-for` arms, and a loop with none only walks the source. | [Control flow: for](control-flow.md#for) |
| `for` | `(for source:Clause body:Inline arm:LoopArm*)` | The same, with a one-step body. | [Control flow: for](control-flow.md#for) |
| `for` | `(for source:Clause row:Child* arm:LoopArm*)` | Inside markup: the rows are children added for each item. | [Markup: Control flow in markup](markup.md#control-flow-in-markup) |
| `and-for` | `(and-for source:Clause row:Step*)` | A second source walked together with the first. The rows of the loop go in the last one, and the loop ends when either source ends. | [Control flow: Several headers](control-flow.md#several-headers) |
| `or-for` | `(or-for source:Clause row:Step*)` | A further loop over another source, joined to the first as an alternative. Its rows run for the items of its own source. Not built yet. | [Control flow: Several headers](control-flow.md#several-headers) |
| `in` | `(in item:Binder/Expr source:Expr option:Option*)` | Each item of a collection. | [Control flow: for](control-flow.md#for) |
| `on` | `(on item:Binder/Expr source:Expr option:Option*)` | Each event a source sends. | [Concurrency: Subscriptions](concurrency.md#subscriptions) |
| `in` | `(in source:Expr option:Option*)` | Each item of a collection, unnamed. The rows read the item as `each`. | [Control flow: for](control-flow.md#for) |
| `on` | `(on source:Expr option:Option*)` | Each event a source sends, unnamed. The rows read the event as `each`. | [Reactivity: Events](reactivity.md#events) |
| `each` | `(each item:Any option:Option*)` | Each item, with no source named. | — |
| `and-for` | `(and-for source:Clause+)` | Sources walked together. | [Control flow: Several headers](control-flow.md#several-headers) |
| `when` | `(when condition:Expr)` | Keep only the items for which the condition holds. | [Control flow: Clauses](control-flow.md#clauses) |
| `whenever` | `(whenever condition:Expr)` | Listen to the source only while the condition holds. The condition reads tracked state and not the item. | [Reactivity: Streams of a value](reactivity.md#streams-of-a-value) |
| `while` | `(while condition:Expr)` | Stop at the first item for which the condition fails. | [Control flow: Clauses](control-flow.md#clauses) |
| `until` | `(until condition:Expr)` | Stop at the first item for which the condition holds. | [Control flow: Clauses](control-flow.md#clauses) |
| `keyed-by` | `(keyed-by key:Expr)` | The identity of each item, in a markup loop. | [Markup: Markup that follows state](markup.md#markup-that-follows-state) |
| `do` | `(do deco:Deco* scope:Scope* row:Step*)` | Run the rows as one step. | [Control flow: Blocks and scopes](control-flow.md#blocks-and-scopes) |
| `do` | `(do deco:Deco* body:Inline)` | Run one step. | [Control flow: Blocks and scopes](control-flow.md#blocks-and-scopes) |
| `finally` | `(finally row:Step*)` | Rows that run when the enclosing body ends, however it ends. | [Errors: Cleanup](errors.md#cleanup) |
| `finally` | `(finally body:Inline)` | One step that runs when the enclosing body ends. | [Errors: Cleanup](errors.md#cleanup) |
| `do-new` | `(do-new worker:Worker? deco:Deco* row:Step* failure:Failure*)` | Run the rows as a new task. | [Concurrency: Tasks](concurrency.md#tasks) |
| `do-new` | `(do-new worker:Worker? deco:Deco* body:Inline failure:Failure*)` | Run one step as a new task. | [Concurrency: Tasks](concurrency.md#tasks) |
| `do-new-thread` | `(do-new-thread worker:Worker? deco:Deco* row:Step* failure:Failure*)` | Run the rows as a task in the shared thread pool. | [Concurrency: Where a task runs](concurrency.md#where-a-task-runs) |
| `do-new-thread` | `(do-new-thread worker:Worker? deco:Deco* body:Inline failure:Failure*)` | Run one step as a task in the shared thread pool. | [Concurrency: Where a task runs](concurrency.md#where-a-task-runs) |
| `worker:` | `(worker: value:Expr)` | The worker that runs the task. | [Concurrency: Workers](concurrency.md#workers) |
| `on-failure` | `(on-failure error:Binder row:Step*)` | Rows the task runs when a panic of the named type ends it, after its cleanups, with that panic. | [Errors: Handling a failed task](errors.md#handling-a-failed-task) |
| `do-with` | `(do-with name:Name value:Expr row:Step*)` | Run the rows with a stack value set for them and for what they call, or a fake bound to the effect `name`. The value is restored after. | [Concurrency: Values for each thread, task or stack](concurrency.md#values-for-each-thread-task-or-stack) |
| `do-use` | `(do-use resource:Expr row:Step*)` | Run the rows with a resource that is released at the end. | [Errors: use](errors.md#use) |
| `do-use?` | `(do-use? resource:Expr row:Step*)` | The same, skipped when the resource is null. | [Errors: use](errors.md#use) |
| `throw` | `(throw error:Expr guard:Guard? handler:Handler*)` | Raise an error, only when the condition holds if one is given. With handlers below it, it raises nothing now: an error the handlers name is replaced by this one when it leaves the function. | [Errors: Raising](errors.md#raising) |
| `on-error` | `(on-error error:Binder/Caught row:Step/Value*)` | Rows that run when the form above raises the error. | [Errors: Handlers](errors.md#handlers) |
| `caught` | `(caught error:Binder earlier:Binder+)` | An error together with the errors it replaced. | [Errors: Transforming errors](errors.md#transforming-errors) |
| `print` | `(print value:Expr/Block?)` | Print a value. With no value it does nothing and stands for an empty body. | [Text: Printing](text.md#printing) |
| `print?` | `(print? value:Expr)` | Print a value unless it is null. | [Optionals: Steps that skip null](optionals.md#steps-that-skip-null) |
| `with` | `(with value:Expr)` | The first row of a `match`: the helper object its grammars call. | [Grammars: Helpers](grammars.md#helpers) |
| `header` | `(header value:Expr)` | Starts a section with a value. Each value row after it, up to the next header, is handed to that value. | [Functions: Value bodies](functions.md#value-bodies) |
| `paragraph` | `(paragraph line:Line*)` | A text body, one item for each line. | [Text: Text bodies](text.md#text-bodies) |

## Assignment

| Head | Shape | Meaning | See |
| --- | --- | --- | --- |
| `=` | `(= target:Target value:Expr/Block)` | Store a value. | [Control flow: Locals](control-flow.md#locals) |
| `=` | `(= (\| pattern:Pattern+) value:Expr)` | True when the value fits the pattern. The pattern may name parts of the value. | [Patterns: Patterns outside match](patterns.md#patterns-outside-match) |
| `=` | `(= (~ pattern:Stream+) value:Expr)` | True when the value's items fit the grammar. The grammar may name parts of what it matched. | [Grammars: Running grammars](grammars.md#running-grammars) |
| `=?` | `(=? target:Target value:Expr/Block)` | Store a value unless the value is null. | [Optionals: Steps that skip null](optionals.md#steps-that-skip-null) |
| `+=` | `(+= target:Target value:Expr/Block)` | Add to what is stored. | [Operators: Operators a type defines](operators.md#operators-a-type-defines) |
| `+=?` | `(+=? target:Target value:Expr/Block)` | Add to what is stored, unless the value is null. | [Optionals: Steps that skip null](optionals.md#steps-that-skip-null) |
| `-=` | `(-= target:Target value:Expr/Block?)` | Subtract from what is stored. With no value, remove the item the target names and give what the removal gives. | [Collections: Changing a list](collections.md#changing-a-list) |
| `-=?` | `(-=? target:Target value:Expr/Block)` | Subtract from what is stored, unless the value is null. | [Optionals: Steps that skip null](optionals.md#steps-that-skip-null) |
| `*=` | `(*= target:Target value:Expr/Block)` | Multiply what is stored. | [Operators: Arithmetic](operators.md#arithmetic) |
| `/=` | `(/= target:Target value:Expr/Block)` | Divide what is stored. | [Operators: Arithmetic](operators.md#arithmetic) |
| `%=` | `(%= target:Target value:Expr/Block)` | Store the remainder. | [Operators: Arithmetic](operators.md#arithmetic) |
| `%%=` | `(%%= target:Target value:Expr/Block)` | Raise what is stored to a power. | [Operators: Arithmetic](operators.md#arithmetic) |
| `++=` | `(++= target:Target value:Expr/Block)` | Append every item of a collection. | [Collections: Changing a list](collections.md#changing-a-list) |
| `*=?` | `(*=? target:Target value:Expr/Block)` | Multiply what is stored, unless the value is null. | [Optionals: Steps that skip null](optionals.md#steps-that-skip-null) |
| `/=?` | `(/=? target:Target value:Expr/Block)` | Divide what is stored, unless the value is null. | [Optionals: Steps that skip null](optionals.md#steps-that-skip-null) |
| `%=?` | `(%=? target:Target value:Expr/Block)` | Store the remainder, unless the value is null. | [Optionals: Steps that skip null](optionals.md#steps-that-skip-null) |
| `%%=?` | `(%%=? target:Target value:Expr/Block)` | Raise what is stored to a power, unless the value is null. | [Optionals: Steps that skip null](optionals.md#steps-that-skip-null) |
| `++=?` | `(++=? target:Target value:Expr/Block)` | Append every item of a collection, unless it is null. | [Optionals: Steps that skip null](optionals.md#steps-that-skip-null) |
| `<-` | `(<- target:Expr value:Expr*)` | Send values to a channel in order, or read one from it. | [Concurrency: Channels](concurrency.md#channels) |
| `<-*` | `(<-* target:Target value:Expr/Block)` | Send every value of a stream. | [Streams: Targets](streams.md#targets) |
| `<-*?` | `(<-*? target:Target value:Expr/Block)` | Send every value of a stream, unless it is null. Not built yet. | [Streams: Targets](streams.md#targets) |

## Calls

| Head | Shape | Meaning | See |
| --- | --- | --- | --- |
| `call` | `(call callee:Expr arg:Arg*)` | Call a function with arguments. A method is called through `(. object name)`. | [Functions: Calls](functions.md#calls) |
| `:label` | `(:label value:Expr/Block?)` | A named argument. The head is a colon and the argument's name. | [Functions: Calls](functions.md#calls) |
| `block` | `(block row:Row* value:Given? bind:Lazy*)` | Arguments written one to a row, or steps whose last row is the argument. | [Functions: Calls](functions.md#calls) |
| `inline` | `(inline body:Step/Expr)` | A one-step body given as the last argument. | [Source text: Lines](source-text.md#lines) |
| `arg` | `(arg value:Expr/Block)` | One positional argument. | [Functions: Calls](functions.md#calls) |
| `entry` | `(entry key:Expr value:Expr/Block?)` | One key and its value. | [Literals: Maps](literals.md#maps) |
| `entry?` | `(entry? key:Expr value:Expr/Block)` | One key and its value, left out when the value is null. | [Literals: Maps](literals.md#maps) |
| `item` | `(item value:Expr/Block)` | One item of a list being built. | [Streams: Pipelines in rows of a block](streams.md#pipelines-in-rows-of-a-block) |
| `item?` | `(item? value:Expr/Block)` | One item, left out when it is null. | [Streams: Pipelines in rows of a block](streams.md#pipelines-in-rows-of-a-block) |
| `spread` | `(spread value:Expr/Block)` | Every item of a collection. | [Streams: Pipelines in rows of a block](streams.md#pipelines-in-rows-of-a-block) |
| `spread?` | `(spread? value:Expr/Block)` | Every item of a collection, unless it is null. | [Streams: Pipelines in rows of a block](streams.md#pipelines-in-rows-of-a-block) |
| `pipe` | `(pipe stage:Atom value:Expr/Stream/Block? row:Row/Value*)` | One stage of a pipeline over the row above. | [Streams: Pipelines in rows of a block](streams.md#pipelines-in-rows-of-a-block) |
| `scope` | `(scope value:Expr row:Row*)` | A value with the rows that complete it, such as the `lazy` names it uses. | [Functions: Lazy bindings](functions.md#lazy-bindings) |

## Values

| Head | Shape | Meaning | See |
| --- | --- | --- | --- |
| `.` | `(. object:Expr name:Name/Type+)` | A member of a value, or a member of that member. | [Types: Using members](types.md#using-members) |
| `?.` | `(?. object:Expr name:Name+)` | A member, or null when the value is null. | [Optionals: Chains](optionals.md#chains) |
| `*.` | `(*. object:Expr name:Name+)` | The member of every item. Not built yet. | — |
| `[]` | `([] object:Expr index:Expr?)` | The item at an index or in a range. | [Collections: Reading items](collections.md#reading-items) |
| `[]` | `([] item:Type)` | The array type of the item type. | [Collections: Arrays](collections.md#arrays) |
| `?[]` | `(?[] object:Expr index:Expr)` | The item at an index, or null when the collection is null. | [Optionals: Using an optional](optionals.md#using-an-optional) |
| `!!` | `(!! value:Expr)` | The value, raising an error when it is null. | [Optionals: Forcing](optionals.md#forcing) |
| `as` | `(as value:Expr type:Type+)` | The value converted to a type. | [Conversions: as](conversions.md#as) |
| `??` | `(?? value:Expr fallback:Expr+)` | The first value that is not null. | [Optionals: Fallbacks](optionals.md#fallbacks) |
| `only-if` | `(only-if value:Expr guard:Guard)` | The value when the condition holds, otherwise null. | [Control flow: Values chosen by a condition](control-flow.md#values-chosen-by-a-condition) |
| `if-expr` | `(if-expr condition:Test value:Expr otherwise:Expr)` | The value when the condition holds, otherwise the other value. | [Control flow: Values chosen by a condition](control-flow.md#values-chosen-by-a-condition) |
| `str` | `(str part:Expr+)` | A text built from texts and values. | [Text: Building text from values](text.md#building-text-from-values) |
| `char` | `(char text:Text)` | One character. | [Literals: Characters](literals.md#characters) |
| `list` | `(list item:Expr*)` | A list of values. | [Collections: Lists](collections.md#lists) |
| `list` | `(list item:Type)` | The list type of the item type. | [Collections: Lists](collections.md#lists) |
| `map` | `(map key:Type value:Type)` | The type of a map from keys to values. | [Collections: Maps](collections.md#maps) |
| `map` | `(map key:Pattern value:Pattern)` | The same, in a pattern. | [Patterns: Maps](patterns.md#maps) |
| `tuple` | `(tuple item:Expr/Binder+)` | Values grouped in order. | [Types: Tuples](types.md#tuples) |
| `tuple` | `(tuple item:Type+)` | The tuple type of the item types. | [Types: Tuples](types.md#tuples) |
| `stream` | `(stream item:Expr)` | A stream that sends the value again each time tracked state it reads changes. | [Reactivity: Streams of a value](reactivity.md#streams-of-a-value) |
| `stream` | `(stream item:Type)` | The stream type of the item type. | [Streams: The stream family](streams.md#the-stream-family) |
| `..` | `(.. first:Expr last:Expr step:Stride?)` | The values from first through last. | [Literals: Ranges](literals.md#ranges) |
| `..<` | `(..< first:Expr last:Expr step:Stride?)` | The values from first up to, not including, last. | [Literals: Ranges](literals.md#ranges) |
| `...` | `(... first:Expr/Hole last:Expr? step:Stride?)` | An open range. | [Collections: Ranges as values](collections.md#ranges-as-values) |
| `by:` | `(by: step:Expr)` | The step between values. | [Literals: Ranges](literals.md#ranges) |
| `=>` | `(=> params:Params body:Expr/Body)` | A function value: its parameter list, then the value it produces. | [Functions: Function values](functions.md#function-values) |
| `=>` | `(=> params:Params :do body:Step?)` | A function value that runs one step. | [Functions: Function values](functions.md#function-values) |
| `=>` | `(=> params:Params :do (block row:Step*))` | A function value that runs steps in order. | [Functions: Function values](functions.md#function-values) |
| `match` | `(match subject:Expr case:Case+)` | The rows of the first arm the value fits. On a row of its own in a body of steps, it runs that arm's rows as steps. | [Patterns: match](patterns.md#match) |
| `match?` | `(match? subject:Expr case:Case+)` | The same, or null when no arm fits. | [Patterns: match?](patterns.md#match-1) |
| `case` | `(case pattern:Pattern row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` | An arm taken when the value has this shape. | [Patterns: match](patterns.md#match) |
| `case` | `(case pattern:Pattern body:Inline)` | The same, with a one-step body. | [Patterns: match](patterns.md#match) |
| `case?` | `(case? condition:Test row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` | An arm taken when the condition holds. | [Control flow: Decision tables](control-flow.md#decision-tables) |
| `case?` | `(case? condition:Test body:Inline)` | The same, with a one-step body. | [Control flow: Decision tables](control-flow.md#decision-tables) |
| `case=` | `(case= value:Expr row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` | An arm taken when the value equals this one. | [Patterns: match](patterns.md#match) |
| `case` | `(case (= pattern:Pattern subject:Expr) row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` | Rows taken when the subject fits the pattern. Not built yet. | [Patterns: Patterns outside match](patterns.md#patterns-outside-match) |
| `case` | `(case (= pattern:Pattern subject:Expr))` | The subject narrowed to the pattern, or null when it does not fit. | [Patterns: Patterns outside match](patterns.md#patterns-outside-match) |
| `case-union` | `(case-union (= pattern:Pattern subject:Expr) row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` | The same, with a result that keeps its own type. | [Patterns: Arms that keep their type](patterns.md#arms-that-keep-their-type) |
| `case-union` | `(case-union pattern:Pattern row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` | An arm like `case` whose result keeps its own type, so the match gives a union of the types of its arms. | [Patterns: Arms that keep their type](patterns.md#arms-that-keep-their-type) |
| `case-union?` | `(case-union? condition:Expr row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` | The same, taken when the condition holds. | [Patterns: Arms that keep their type](patterns.md#arms-that-keep-their-type) |
| `case-union=` | `(case-union= value:Expr row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` | The same, taken when the value equals this one. | [Patterns: Arms that keep their type](patterns.md#arms-that-keep-their-type) |
| `default` | `(default row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` | The arm taken when no other is. | [Patterns: match](patterns.md#match) |
| `default` | `(default body:Inline)` | The same, with a one-step body. | [Patterns: match](patterns.md#match) |
| `case-each` | `(case-each source:Clause row:Step/Value*)` | A stream built from a source. For each item that fits the pattern it gives the value of the rows, or the item when there are no rows. Other items are left out. | [Streams: Comprehensions](streams.md#comprehensions) |
| `case-each` | `(case-each source:Clause body:Inline)` | The same, with a one-step body. | [Streams: Comprehensions](streams.md#comprehensions) |
| `catch` | `(catch call:Expr)` | The result, or the error it raised, as one value. | [Errors: catch and catch?](errors.md#catch-and-catch) |
| `catch?` | `(catch? call:Expr)` | The result, or null when it raised an error. | [Errors: catch and catch?](errors.md#catch-and-catch) |
| `use` | `(use resource:Expr)` | A resource released when the body ends. | [Errors: use](errors.md#use) |
| `use?` | `(use? resource:Expr)` | The same, when the resource is not null. | [Errors: use](errors.md#use) |
| `select` | `(select source:Expr choice:Any)` | A value, then the pattern it is produced for. The value is produced where the pattern fits. | [Patterns: match](patterns.md#match) |
| `iterate` | `(iterate part:Any+)` | A loop written as a value: a stream of the results of its rows for each item. | [Streams: Comprehensions](streams.md#comprehensions) |
| `or-for` | `(or-for source:Any row:Any*)` | A further loop of an `iterate`, with its rows. Not built yet. | [Control flow: Several headers](control-flow.md#several-headers) |
| `type-of` | `(type-of value:Expr)` | The type of a value. | [Types: Type information](types.md#type-information) |
| `facts-of` | `(facts-of value:Expr)` | What the compiler knows about a value. Not built yet. | — |
| `##` | `(## value:Expr)` | The latest value a stream has sent. An expression that reads it is worked out again when the stream sends a new value. | [Reactivity: Streams of a value](reactivity.md#streams-of-a-value) |
| `preview` | `(preview value:Expr)` | A value recorded in a probe without turning it into text. | [Probes: Text and values](probes.md#text-and-values) |
| `snapshot` | `(snapshot value:Expr)` | A value copied into a probe. | [Probes: Text and values](probes.md#text-and-values) |

## Operators

| Head | Shape | Meaning | See |
| --- | --- | --- | --- |
| `not` | `(not value:Test)` | Not. | [Operators: Logic](operators.md#logic) |
| `-` | `(- left:Expr right:Expr*)` | Subtract, or negate one value. | [Operators: Arithmetic](operators.md#arithmetic) |
| `+` | `(+ left:Expr right:Expr*)` | Add. | [Operators: Arithmetic](operators.md#arithmetic) |
| `*` | `(* left:Expr right:Expr*)` | Multiply. | [Operators: Arithmetic](operators.md#arithmetic) |
| `/` | `(/ left:Expr right:Expr+)` | Divide, or join a path. | [Operators: Arithmetic](operators.md#arithmetic) |
| `%` | `(% left:Expr right:Expr+)` | Remainder. | [Operators: Arithmetic](operators.md#arithmetic) |
| `%%` | `(%% left:Expr right:Expr+)` | Raise to a power. | [Operators: Arithmetic](operators.md#arithmetic) |
| `++` | `(++ left:Expr right:Expr+)` | Join two collections or texts. | [Operators: Joining and membership](operators.md#joining-and-membership) |
| `and` | `(and left:Test right:Test+)` | True when every value is. | [Operators: Logic](operators.md#logic) |
| `or` | `(or left:Test right:Test+)` | True when any value is. | [Operators: Logic](operators.md#logic) |
| `==` | `(== left:Expr right:Expr+)` | Equal. | [Operators: Comparison](operators.md#comparison) |
| `!=` | `(!= left:Expr right:Expr+)` | Not equal. | [Operators: Comparison](operators.md#comparison) |
| `<` | `(< left:Expr right:Expr+)` | Less than. | [Operators: Comparison](operators.md#comparison) |
| `>` | `(> left:Expr right:Expr+)` | Greater than. | [Operators: Comparison](operators.md#comparison) |
| `<=` | `(<= left:Expr right:Expr+)` | At most. | [Operators: Comparison](operators.md#comparison) |
| `>=` | `(>= left:Expr right:Expr+)` | At least. | [Operators: Comparison](operators.md#comparison) |
| `is` | `(is value:Expr shape:Pattern+)` | True when the value is this type, never by conversion: its own type, a class it derives from, an interface it implements, or the case its union holds. Or when it fits this shape. | [Types: Values of a base class](types.md#values-of-a-base-class) |
| `is-not` | `(is-not value:Expr shape:Pattern+)` | True when the value is not this type and does not fit this shape. | [Types: Values of a base class](types.md#values-of-a-base-class) |
| `is!` | `(is! value:Expr shape:Pattern+)` | True when the value's type is exactly this one, not a class derived from it. | [Types: Values of a base class](types.md#values-of-a-base-class) |
| `is!-not` | `(is!-not value:Expr shape:Pattern+)` | True when the value's type is not exactly this one. | [Types: Values of a base class](types.md#values-of-a-base-class) |
| `is~` | `(is~ value:Expr shape:Pattern+)` | True when the value can be converted to this type. | [Conversions: Parameters that take what converts](conversions.md#parameters-that-take-what-converts) |
| `is~-not` | `(is~-not value:Expr shape:Pattern+)` | True when the value cannot be converted to this type. | [Conversions: Parameters that take what converts](conversions.md#parameters-that-take-what-converts) |
| `is-in` | `(is-in value:Expr collection:Expr)` | True when the collection holds the value. | [Operators: Joining and membership](operators.md#joining-and-membership) |
| `not-in` | `(not-in value:Expr collection:Expr)` | True when the collection does not hold the value. | [Operators: Joining and membership](operators.md#joining-and-membership) |
| `has` | `(has collection:Expr value:Expr)` | True when the collection holds the value. | [Operators: Joining and membership](operators.md#joining-and-membership) |
| `has-not` | `(has-not collection:Expr value:Expr)` | True when the collection does not hold the value. | [Operators: Joining and membership](operators.md#joining-and-membership) |
| `>>` | `(>> left:Expr right:Expr+)` | Reserved for composing two functions, left first. The compiler does not build it yet. | [Operators: Reserved operators](operators.md#reserved-operators) |
| `<<` | `(<< left:Expr right:Expr+)` | Reserved for composing two functions, right first. The compiler does not build it yet. | [Operators: Reserved operators](operators.md#reserved-operators) |
| `?>>` | `(?>> left:Expr right:Expr+)` | Reserved, like `>>`, for a value that may be null. | [Operators: Reserved operators](operators.md#reserved-operators) |
| `?<<` | `(?<< left:Expr right:Expr+)` | Reserved, like `<<`, for a value that may be null. | [Operators: Reserved operators](operators.md#reserved-operators) |
| `?<-` | `(?<- left:Expr right:Expr+)` | Send a value to a channel, unless the channel is null. | [Streams: Targets](streams.md#targets) |
| `\|>` | `(\|> left:Expr right:Expr+)` | Pass the value to the next stage. | [Streams: Pipelines](streams.md#pipelines) |
| `\|=>` | `(\|=> left:Expr right:Expr+)` | Pass the value to an expression that reads it as `it`. | [Streams: Pipelines](streams.md#pipelines) |
| `<\|` | `(<\| left:Expr right:Expr+)` | Reserved for passing a value backward. The compiler does not build it yet. | [Operators: Reserved operators](operators.md#reserved-operators) |
| `*>` | `(*> left:Expr right:Expr+)` | Map each item. | [Streams: Pipelines](streams.md#pipelines) |
| `?>` | `(?> left:Expr right:Expr+)` | Keep the items that pass. | [Streams: Pipelines](streams.md#pipelines) |
| `?*>` | `(?*> left:Expr right:Expr+)` | Map each item and drop nulls. | [Streams: Pipelines](streams.md#pipelines) |
| `*\|>` | `(*\|> left:Expr right:Expr+)` | Run a step for each item and pass the item on unchanged. | [Streams: Pipelines](streams.md#pipelines) |
| `->` | `(-> left:Expr right:Expr+)` | Write the whole value to a target in one write. The result is the target. | [Streams: Targets](streams.md#targets) |
| `*->` | `(*-> left:Expr right:Expr+)` | Write each item to a target, one write for each. | [Streams: Targets](streams.md#targets) |
| `~>` | `(~> left:Expr right:Any+)` | Match a grammar again and again along a stream. The result is a stream of what each match produces. | [Grammars: Reading records](grammars.md#reading-records) |
| `/infix/` | `(/infix/ left:Expr right:Expr+)` | A named operator; the head is its name between slashes. | [Operators: Named operators](operators.md#named-operators) |

## Types as forms

| Head | Shape | Meaning | See |
| --- | --- | --- | --- |
| `of` | `(of base:Expr argument:Type+)` | A generic name with its type arguments. | [Types: Generic types](types.md#generic-types) |
| `?` | `(? type:Any)` | An optional type, or an optional part of a grammar. | [Optionals: Optional types](optionals.md#optional-types) |
| `channel` | `(channel item:Type)` | A channel of values. | [Concurrency: Channels](concurrency.md#channels) |
| `fn-type` | `(fn-type result:Type/Hole part:Raises/Header*)` | The type of a function. | [Functions: Function types](functions.md#function-types) |
| `c-fn-type` | `(c-fn-type result:Type/Hole part:Raises/Header*)` | The type of a C function. | [Native code and memory: Callbacks](native-code-and-memory.md#callbacks) |
| `c-struct` | `(c-struct field:Any*)` | A C structure. | [Native code and memory: Structs](native-code-and-memory.md#structs) |
| `exact` | `(exact type:Type)` | Exactly this type and no subtype. | [Patterns: Types](patterns.md#types) |
| `similar` | `(similar type:Type)` | This type, or any type that converts to it. | [Conversions: Parameters that take what converts](conversions.md#parameters-that-take-what-converts) |
| `:` | `(: name:Atom type:Any)` | A name with its type, or a name for part of a pattern. | [Patterns: Names](patterns.md#names) |

## Patterns

| Head | Shape | Meaning | See |
| --- | --- | --- | --- |
| `call` | `(call rule:Expr part:Stream*)` | A stream rule applied to arguments. | [Grammars: Rules](grammars.md#rules) |
| `call` | `(call callee:Expr part:Pattern*)` | A case of a union and patterns for the values it holds. | [Patterns: Union cases](patterns.md#union-cases) |
| `list` | `(list item:Pattern*)` | A list whose items fit these patterns. `...` stands for the rest. | [Patterns: Lists](patterns.md#lists) |
| `tuple` | `(tuple item:Pattern+)` | A tuple whose items fit these patterns. | [Patterns: Tuples](patterns.md#tuples) |
| `=` | `(= key:Expr value:Pattern)` | An entry of a map whose value fits the pattern. | [Patterns: Maps](patterns.md#maps) |
| `*` | `(* part:Stream+)` | The part, any number of times. | [Grammars: Grammar pieces](grammars.md#grammar-pieces) |
| `+` | `(+ part:Stream+)` | The part, one or more times. | [Grammars: Grammar pieces](grammars.md#grammar-pieces) |
| `!` | `(! part:Stream)` | Not followed by the part. | [Grammars: Grammar pieces](grammars.md#grammar-pieces) |
| `&` | `(& part:Stream)` | Followed by the part, which is not consumed. | [Grammars: Grammar pieces](grammars.md#grammar-pieces) |
| `all` | `(all part:Any+)` | Every part must fit. | [Patterns: Values and conditions](patterns.md#values-and-conditions) |
| `\|` | `(\| part:Any+)` | Any one part must fit. | [Patterns: Values and conditions](patterns.md#values-and-conditions) |
| `{}` | `({} part:Any+)` | A test on the matched value, or a repeat count in a grammar. | [Patterns: Values and conditions](patterns.md#values-and-conditions) |
| `~` | `(~ pattern:Any part:Any*)` | A grammar and what its match produces. | [Grammars: Matching text](grammars.md#matching-text) |
| `seq` | `(seq part:Any+)` | Parts that must follow one another. | [Grammars: Grammar pieces](grammars.md#grammar-pieces) |
| `chars` | `(chars set:Text)` | Any one character of the set. | [Grammars: Character sets](grammars.md#character-sets) |
| `\{}` | `(\{} test:Any)` | A test on the next item. | [Grammars: Grammar pieces](grammars.md#grammar-pieces) |
| `into` | `(into name:Atom+)` | Where a rule's captures go. | [Grammars: Rules](grammars.md#rules) |
| `of:` | `(of: source:Any)` | The value the grammar is matched against, once. | [Grammars: Running grammars](grammars.md#running-grammars) |
| `in:` | `(in: source:Any+)` | The source the grammar is matched along, again and again. The result is a stream of the matches. | [Grammars: Running grammars](grammars.md#running-grammars) |
| `on:` | `(on: source:Any+)` | The source of events the grammar is matched along, again and again. The result is a stream of the matches. Not supported yet. | [Grammars: Running grammars](grammars.md#running-grammars) |
| `<*` | `(<* pattern:Any)` | Every match of a pattern. | [Streams: Comprehensions](streams.md#comprehensions) |
| `/` | `(/ left:Pattern right:Pattern+)` | Reserved for the child relation between tree patterns. The compiler does not match it yet. | [Patterns: Kinds of pattern](patterns.md#kinds-of-pattern) |
| `*/` | `(*/ left:Pattern right:Pattern+)` | Reserved for the descendant relation between tree patterns. The compiler does not match it yet. | [Patterns: Kinds of pattern](patterns.md#kinds-of-pattern) |
| `\` | `(\ left:Pattern right:Pattern+)` | Reserved for the parent relation between tree patterns. The compiler does not match it yet. | [Patterns: Kinds of pattern](patterns.md#kinds-of-pattern) |
| `*\` | `(*\ left:Pattern right:Pattern+)` | Reserved for the ancestor relation between tree patterns. The compiler does not match it yet. | [Patterns: Kinds of pattern](patterns.md#kinds-of-pattern) |
| `--` | `(-- left:Pattern right:Pattern+)` | Reserved for the previous-sibling relation between tree patterns. The compiler does not match it yet. | [Patterns: Kinds of pattern](patterns.md#kinds-of-pattern) |
| `++` | `(++ left:Pattern right:Pattern+)` | Reserved for the next-sibling relation between tree patterns. The compiler does not match it yet. | [Patterns: Kinds of pattern](patterns.md#kinds-of-pattern) |

## Markup

| Head | Shape | Meaning | See |
| --- | --- | --- | --- |
| `<>` | `(<> type:Type attr:Attr* use:Use* tag:Tag* row:Child*)` | An element: its type, its attributes, the style rules it uses, its theme tags, then its properties and children. | [Markup: Elements](markup.md#elements) |
| `+<>` | `(+<> type:Type attr:Attr* use:Use* row:Child*)` | Declares a slot: a named place in a component that a caller fills with an element of this type. Its rows give the default content and the properties applied to what fills it. | [Markup: Slots](markup.md#slots) |
| `<+` | `(<+ name:Name)` | Passes the content the caller gave for the named slot on to the element this row is inside. | [Markup: Repeated slots and passing slots on](markup.md#repeated-slots-and-passing-slots-on) |
| `<->` | `(<-> property:Name place:Target)` | Binds a property of the element to a place both ways: the element takes the place's value and follows it, and stores its own back as it changes. | [Markup: Bindings](markup.md#bindings) |
| `id:` | `(id: name:Name)` | The name the element is known by. | [Markup: Names](markup.md#names) |
| `slot:` | `(slot: name:Name)` | The slot the element fills. | [Markup: Slots](markup.md#slots) |
| `param:` | `(param: name:Name type:Type?)` | A value the slot passes in. | [Markup: Repeated slots and passing slots on](markup.md#repeated-slots-and-passing-slots-on) |
| `value:` | `(value: value:Expr)` | The element's main value. | [Markup: Elements](markup.md#elements) |
| `text:` | `(text: part:Any+)` | The element's text. | [Markup: Elements](markup.md#elements) |
| `for:` | `(for: tag:Name value:Expr?)` | A theme tag the element sets for itself and what is beneath it: on, or as the value says, which may read tracked state. A null value inherits the tag. | [Styles: Themes](styles.md#themes) |
| `•` | `(• name:Name argument:Expr* guard:UseIf?)` | Use a style rule. | [Styles: Which rules apply](styles.md#which-rules-apply) |
| `if:` | `(if: condition:Expr)` | Use the rule only when the condition holds. | [Styles: Conditions](styles.md#conditions) |
| `:label` | `(:label value:Expr/Block?)` | A property set by name. | [Markup: Attributes](markup.md#attributes) |

## Styles

| Head | Shape | Meaning | See |
| --- | --- | --- | --- |
| `style` | `(style name:Name clause:SheetClause* row:Style/Member*)` | A style sheet. | [Styles: Style sheets](styles.md#style-sheets) |
| `is` | `(is base:Type)` | The style sheet this one derives from. It inherits the base's slots and rules, and overrides a slot with `(prop + name _ (= value))`. | [Styles: Themes](styles.md#themes) |
| `for` | `(for tag:Name+)` | The tags under which a derived style sheet applies, such as `light` or `dark`. It follows `(is ...)`. | [Styles: Themes](styles.md#themes) |
| `style-extend` | `(style-extend name:Type row:Style/Member*)` | Rules added to a style sheet declared elsewhere. | [Styles: Extending a sheet](styles.md#extending-a-sheet) |
| `rule` | `(rule name:Name parameter:Param? use:Use* row:Style*)` | A named style rule. | [Styles: Style sheets](styles.md#style-sheets) |
| `own-rule` | `(own-rule name:Name parameter:Param? use:Use* row:Style*)` | A style rule written in another that reaches only the markup of the class of the element the other is applied to, not what is written in the element's body or given its slots. | [Styles: Nested rules, own rules and generated content](styles.md#nested-rules-own-rules-and-generated-content) |
| `when-style` | `(when-style state:Expr row:Style*)` | Rows that apply while the element is in a state. | [Styles: States](styles.md#states) |
| `when-style` | `(when-style state:Expr body:Inline)` | The same, with one row. | [Styles: States](styles.md#states) |
| `style-set` | `(style-set state:Name name:Name value:Expr)` | One style value that applies while the element is in the named state. | [Styles: States](styles.md#states) |
| `keyframes` | `(keyframes name:Name parameter:Param? kind:Atom duration:Expr row:Keyframe*)` | A keyframe `animation` or `motion`: a slot whose value calls the builder its kind names. A parameter makes it a shape, which `(shape name value)` binds. | [Styles: Keyframes](styles.md#keyframes) |
| `=` | `(= name:Name value:Expr)` | A setting the builder takes, such as `repeat`. | [Styles: Keyframes](styles.md#keyframes) |
| `range` | `(range property:Keyed from:Expr/Hole to:Expr/Hole option:Named*)` | A property that moves from one value to another. `_` leaves an end out. | [Styles: Keyframes](styles.md#keyframes) |
| `track` | `(track property:Keyed option:Named* stop:Stop*)` | A property that passes through its stops. | [Styles: Keyframes](styles.md#keyframes) |
| `at` | `(at at:Expr value:Expr option:Named*)` | The value at a point of the animation. | [Styles: Keyframes](styles.md#keyframes) |
| `shape` | `(shape shape:Expr value:Expr)` | A shape bound to a value, as `shake:3` binds `shake` to 3. | [Styles: Keyframes](styles.md#keyframes) |

## State machines

| Head | Shape | Meaning | See |
| --- | --- | --- | --- |
| `state` | `(state target:Expr trigger:Trigger? row:State/Step*)` | A state, what enters it, and what happens in it. | [State machines: Cases](state-machines.md#cases) |
| `then` | `(then target:Expr trigger:Trigger? row:Any*)` | A move to another state. | [State machines: Transitions](state-machines.md#transitions) |
| `else-state` | `(else-state target:Expr)` | When no move before it applies, leave the named state. It does not enter it. | [State machines: Fallbacks](state-machines.md#fallbacks) |
| `trigger` | `(trigger kind:Atom part:Any+)` | What causes a move, or what a `whenever-rule` watches. | [State machines: Transitions](state-machines.md#transitions) |

## Probes

| Head | Shape | Meaning | See |
| --- | --- | --- | --- |
| `probe` | `(probe target:Any part:Any*)` | Run a step only when the probe is on. | [Probes: Statement probes](probes.md#statement-probes) |
| `probe-count` | `(probe-count target:Any amount:Expr? deps:Block?)` | Count an event. The block names what the count reads. | [Probes: Counters](probes.md#counters) |
| `probe-group` | `(probe-group mark:Atom target:Any part:Any*)` | Open or close a group of probe output. | [Probes: Groups](probes.md#groups) |
| `probe-assign` | `(probe-assign target:Any value:Expr deps:Block?)` | Record a value. The block holds rows that belong to the probe. | [Probes: Events and property sets](probes.md#events-and-property-sets) |
| `probe-log` | `(probe-log target:Any part:Any*)` | Record a line of output. | [Probes: Logs](probes.md#logs) |
| `items:` | `(items: value:Any+)` | The values a probe logs. | [Probes: Events and property sets](probes.md#events-and-property-sets) |
