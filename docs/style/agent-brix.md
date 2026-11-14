# Brix

Brix is Yumi source written as S-expressions. This is the reference for writing it. `yumi lint`
checks a file against it and names the fix for each mistake.

## Text

- A file is a sequence of forms. A form is a list in parentheses, and its first item is its head.
- Every head comes from the vocabulary below. A function is called with `call`:
  `(call name argument …)`. A method is called through its owner: `(call (. object name) argument …)`.
- Two heads are open sets. A named argument is `(:name value)`: a colon and the argument's name.
  A named operator is `(/name/ left right)`: its name between slashes.
- One list has no head: the parameter list of a function value, as in `(=> (a b) value)`.
- An atom is a name, a number or a symbol. A name is lowercase words joined by hyphens. A type
  name starts with a capital.
- A text stands in double quotes. Write `\"` for a quote, `\\` for a backslash and `\$` for a dollar
  sign. An unescaped `$` is an error. To put a value into a text, build the text from its parts:
  `(str "Hello, " name "!")`.
- A comment runs from `;` to the end of its line and has a row to itself. `(blank)` is an empty row.
- Write one row to a line, two spaces deeper than the form that holds it, and close a form that
  spans lines on a line of its own. `yumi lint --fix` rewrites a file in this layout.

## Signatures

Each form below is listed as its signature, the kinds it is, and what it means.

- `role:Kind` is one item. The role names the item, and the kind says what may be written there.
- `Kind/Kind` takes either kind.
- `?` marks an item that may be left out. `*` marks one that repeats. `+` marks one that repeats
  and is written at least once.
- A word without a colon, such as `:get`, is written as shown. So is a nested form.
- A form may stand wherever one of its kinds is taken. A form listed twice has two shapes.

## Bodies

- A function's head says what its body is. `fn-expr`, `fn-from-expr` and `fn-to-expr` have a
  body that is a value. `fn-do`, `fn-from-do` and `fn-to-do` have a body of steps. There is no
  plain `fn`: choose one of the two before writing the body.
- A function in a type is written as one at the top level. A constructor is
  `(fn-do access This _ …)`.
- `:get` makes a function a property, read without a call. It takes no parameters. `:set` makes
  it what runs when the property is assigned. Its result is `_`, and it takes one parameter.
- A value body holds rows of kind Value, and its last row is the result. It holds no statement:
  no `let`, `return`, `if`, loop or assignment.
- Before its value, a value body may hold `(before …)`: the checks, prints, probes and `do`
  steps that run first. After its value, `(where …)` holds the lazy bindings the value uses. A
  section is written only when it has rows.
- A body of steps holds rows of kind Step. A value does not stand alone in it. Return a result
  with `(return value)`.
- A constructor holds both.
- A function value, `(=> …)`, has a value body unless `:do` marks it as steps.
- Code that runs when the file runs goes inside `(main …)`.

```
(fn-expr + ratio Float (?! ZeroHeight)
  (param shape Shape)
  (before
    (throw (call ZeroHeight) (if (== height 0)))
  )
  (/ width height)
  (where
    (lazy width (. shape width))
    (lazy height (. shape height))
  )
)
```

## Kinds

- **Top**: A form that stands at the top level of a file.
- **Member**: A form that stands in the body of a type.
- **Step**: A row of a body that runs: a statement form, a `call`, or markup. Includes Call and Lazy. Also written as a value that `pipe` stages after it work on.
- **Value**: A row of a body that produces a value. An assignment is not one. Includes Expr. Excludes Assign.
- **Expr**: A value: a name, a number, a text, or a form that produces one. Includes Call and Assign. Also written as an atom, a text.
- **Call**: A `call` form.
- **Assign**: An assignment used as a value: it stores, then gives what it stored.
- **Test**: A condition. Any value but an assignment. Includes Expr. Excludes Assign.
- **Type**: A type: a type name, a type parameter such as `''T`, or a type form. A type name is capitalized. Also written as a capitalized atom or a type parameter such as `''T`.
- **Name**: A name. Written as an atom.
- **Hole**: A place left empty. Written as `_`.
- **Access**: Who may use a declaration: `+` is public and `_` is private. Written as `+`, `_`.
- **Text**: A literal text. Written as a text.
- **Atom**: Any single atom. Written as an atom, `_`.
- **Deco**: An annotation such as `@inline`. Written as an atom that starts with `@`.
- **Mark**: The `...` that makes a parameter take the rest of the arguments. Written as `...`.
- **Binder**: A name being introduced, alone or with its type. Also written as an atom, `_`.
- **Local**: The name a `let` or `lazy` introduces. Written as an atom.
- **Target**: A place a value can be stored. Includes Expr. Also written as an atom.
- **Param**: A parameter of a function.
- **Header**: A header row of a function or type.
- **Key**: The name of a function, or the operator it defines. Also written as an atom.
- **Keyed**: The name of a member, which in a style may be keyed with a dot, as `motion.fade` is. Written as an atom.
- **Init**: The value part of a `prop` or `variant`.
- **Default**: The default value of a parameter.
- **Guard**: A condition that follows what it guards.
- **Guarded**: A value written with `only-if`, which is null where its condition fails.
- **Arg**: An argument of a call. Includes Expr.
- **Row**: A row inside the `block` of a call: a step, or an argument such as `(arg value)` or `(:label value)`. Includes Step.
- **Lead**: A row of an expression body before its value: an arm, a bullet or a pipe stage. Includes Case. Excludes Lazy and Let.
- **Result**: The value an arm gives: its last row. Includes Value.
- **Given**: The value a call's block gives as an argument: its last row. Includes Value.
- **Lazy**: A `lazy` binding.
- **Let**: A `let` declaration.
- **Stage**: A `pipe` stage, which works on the value above it.
- **Block**: A `block` form.
- **Arm**: An `else-if` or `else` inside an `if`.
- **LoopArm**: An `and-for` or `or-for` inside a `for`.
- **Clause**: The source of a loop.
- **Option**: A filter or key on a loop source.
- **Case**: An arm of a `match`.
- **Pattern**: A shape that a value is tested against. Includes Expr. Also written as an atom, `_`, a text.
- **Stream**: A part of a stream pattern. Also written as anything.
- **Child**: A row inside markup. Includes Step and Row and Value.
- **Attr**: An attribute of a markup element.
- **Use**: A style rule applied to an element or inside a rule.
- **Style**: A row of a style sheet or style rule.
- **State**: A row of a state machine.
- **Trigger**: What moves a state machine or a rule.
- **TypeClause**: A clause of a type or extension header.
- **SheetClause**: A clause of a style sheet header.
- **Line**: A line of a text body. Also written as a text.
- **Any**: Anything. Also written as anything.
- **Params**: The parameter list of a function value. Written as an atom.
- **Domain**: A style domain.
- **Over**: The item type of a stream function.
- **Raises**: An error a function can raise.
- **Rule**: A stream pattern rule.
- **Inline**: A one-step body written on the row of its form.
- **Worker**: The worker a task runs on.
- **Handler**: Rows that run when an error is raised.
- **Caught**: An error with the errors it replaced.
- **ChildArm**: An `else-if` or `else` inside a markup `if`.
- **Stride**: The step of a range.
- **UseIf**: A condition on a style rule.
- **Joined**: A further loop inside an `iterate`.
- **SourceArm**: An `and-for` after the source of a `subscribe-for`.
- **Before**: A row that runs before the value of an expression body or a function value: a check, a print, a probe, or a `do` form such as `do`, `finally` or `do-new`.
- **Binding**: A row of `(where …)`: a lazy binding.
- **Section**: The `before` or `where` part of an expression body.
- **Keyframe**: A row of a keyframe animation: a setting, a range, a track or a stop.
- **Stop**: A stop of a keyframe track.
- **Named**: A named argument, such as `(:motion spring)`.
- **Body**: The `block` of a function value without `:do`.
- **Answer**: The value a function value without `:do` gives. Includes Value.

## Forms

### Files

- `(import path:Name)` is Top. Use another module's public names in this file.
- `(export path:Name)` is Top. Pass another module's public names on to whoever imports this file.
- `(prop access:Access name:Name type:Type/Hole deco:Deco* value:Init?)` is Top. A value that belongs to the file.
- `(main row:Step*)` is Top. The code that runs when the file runs. Rows run in order.
- `(main row:Value/Lazy*)` is Top. The same, when the file's code is one pipeline.
- `(unit access:Access name:Atom value:Expr)` is Top. States that an amount of one unit equals an amount of another, so values convert between them.
- `(blank)` is Top, Member, Step, Value, Row, Case, Child, Style, State, Line, Arg, Before, Binding, Keyframe, Stop. An empty row, kept for the reader.

### Functions

- `(fn-do access:Access name:Key result:Type/Hole spread:Over? :static? raises:Raises* deco:Deco* :throws? header:Header* row:Step*)` is Top, Member. A function that runs its rows in order and returns with `return`.
- `(fn-expr access:Access name:Key result:Type/Hole spread:Over? :static? raises:Raises* deco:Deco* :throws? header:Header* row:Lead* body:Value? stage:Stage*)` is Top, Member. A function whose body is the value it produces.
- `(fn-expr access:Access name:Key result:Type/Hole spread:Over? :static? raises:Raises* deco:Deco* :throws? header:Header* row:Lazy+)` is Top, Member. A function whose body only names values, such as a state machine it builds.
- `(fn-expr access:Access name:Key result:Type/Hole spread:Over? :static? raises:Raises* deco:Deco* :throws? header:Header* (header value:Expr) row:Value/Lazy*)` is Top, Member. A function whose body is sections, each a `header` and the values handed to it.
- `(fn-expr access:Access name:Key result:Type/Hole spread:Over? :static? raises:Raises* deco:Deco* :throws? header:Header* (before row:Before+) ? row:Lead* body:Value? stage:Stage* (where row:Binding+) ?)` is Top, Member. A function whose body is the value it produces, after the checks in `before` and with the bindings in `where`.
- `(fn-do access:Access This result:Type/Hole raises:Raises* deco:Deco* header:Header* row:Step/Value*)` is Member. The constructor. Its rows set the properties of a new instance.
- `(fn-do access:Access (of This parameter:Type+) result:Type/Hole raises:Raises* deco:Deco* header:Header* row:Step/Value*)` is Member. A constructor for one choice of type parameters.
- `(fn-do access:Access name:Key result:Type/Hole :get raises:Raises* deco:Deco* row:Step*)` is Member, Top. A property computed by running rows.
- `(fn-expr access:Access name:Key result:Type/Hole :get raises:Raises* deco:Deco* row:Lead* body:Value? stage:Stage*)` is Member, Top. A property computed from a value.
- `(fn-expr access:Access name:Key result:Type/Hole :get raises:Raises* deco:Deco* (before row:Before+) ? row:Lead* body:Value? stage:Stage* (where row:Binding+) ?)` is Member, Top. A property computed from a value, after the checks in `before` and with the bindings in `where`.
- `(fn-do access:Access name:Key result:Hole :set value:Param row:Step*)` is Member, Top. What happens when the property is assigned.
- `(fn-expr access:Access name:Key result:Hole :set value:Param row:Lead* body:Value? stage:Stage* bind:Lazy*)` is Member, Top. What happens when the property is assigned, as one value.
- `(fn-from-do access:Access source:Binder deco:Deco* header:Header* row:Step*)` is Member. A conversion from another type, run as rows.
- `(fn-from-expr access:Access source:Binder deco:Deco* header:Header* row:Lead* body:Value? stage:Stage*)` is Member. A conversion from another type, as a value.
- `(fn-from-expr access:Access source:Binder deco:Deco* header:Header* (before row:Before+) ? row:Lead* body:Value? stage:Stage* (where row:Binding+) ?)` is Member. A conversion from another type, as a value, after the checks in `before` and with the bindings in `where`.
- `(fn-to-do access:Access type:Type deco:Deco* header:Header* row:Step*)` is Member. A conversion to another type, run as rows.
- `(fn-to-expr access:Access type:Type deco:Deco* header:Header* row:Lead* body:Value? stage:Stage*)` is Member. A conversion to another type, as a value.
- `(fn-to-expr access:Access type:Type deco:Deco* header:Header* (before row:Before+) ? row:Lead* body:Value? stage:Stage* (where row:Binding+) ?)` is Member. A conversion to another type, as a value, after the checks in `before` and with the bindings in `where`.
- `(param mark:Mark? name:Name/Hole type:Type/Hole default:Default?)` is Header, Param. A parameter: its name, its type, and its default when it has one.
- `(constraint op:Atom left:Type right:Any)` is Header, Member. A rule that a type parameter must meet.
- `(iterates name:Atom source:Expr)` is Header. Makes the body of the function a loop over the source. The body runs for each item, and the function gives the stream of results.
- `(?! error:Type)` is Raises. An error the function can raise.
- `(over: type:Type)` is Over. The item type of the stream the function matches.
- `(= value:Expr/Block)` is Init, Default. The value.
- `(:= value:Expr/Block)` is Init. A value computed once, the first time it is read.
- `(=> value:Expr/Body)` is Init. A value computed each time it is read.
- `(=> :do body:Step)` is Init. A value computed each time it is read, by running one step.
- `(=> :do (block row:Step*))` is Init. A value computed each time it is read, by running steps.
- `(machine state:State*)` is Init, Expr. A state machine.
- `(block row:Row/Value*)` is Init, Block. Rows that together make one value or one list of arguments.
- `(block row:Before* row:Lead* body:Answer? stage:Stage* bind:Lazy*)` is Body. The value a function value gives, after its checks, prints, probes and `do` forms and before the lazies it uses.
- `(infix left:Name op:Atom right:Param)` is Key. Defines a binary operator.
- `(prefix op:Atom operand:Name)` is Key. Defines a prefix operator.
- `(index-get target:Name index:Param)` is Key. Defines reading `target[index]`.
- `(index-set target:Name index:Param value:Param)` is Key. Defines assigning `target[index]`.
- `(index-delete target:Name index:Param)` is Key. Defines removing `target[index]`.

### Types

- `(type access:Access name:Type clause:TypeClause* member:Member*)` is Top. A type and its members.
- `(type-line access:Access name:Type clause:TypeClause*)` is Top. A type with no body, such as an alias.
- `(extend access:Access type:Type clause:TypeClause* member:Member*)` is Top. Members added to a type declared elsewhere.
- `(extend-line access:Access type:Type clause:TypeClause*)` is Top. An extension with no body.
- `(is base:Type/Expr+)` is TypeClause. What the type is built on. Several types make the type stand for any one of them.
- `(alias name:Atom+)` is TypeClause. Unit suffixes that make a number a value of this type, and other names for the type.
- `(as type:Type+)` is TypeClause. Types the extension makes the type usable as.
- `(where rule:Header+)` is TypeClause. Rules the extended type's parameters must meet.
- `(where row:Binding+)` is Section. The bindings an expression body uses, after its value.
- `(before row:Before+)` is Section. The checks, prints, probes and `do` forms that run before the value of an expression body. Any other step goes in a `(do …)`.
- `(prop access:Access name:Keyed/Hole type:Type/Hole deco:Deco* value:Init?)` is Member, State. A value each instance of the type holds.
- `(variant name:Name/Type payload:Type? value:Init?)` is Member. One case of a union.
- `(event access:Access name:Name type:Type key:Type?)` is Member. An event the type sends.
- `(content row:Step/Member/State*)` is Member. The rows that build an instance's contents.
- `(when-rule condition:Expr row:Step*)` is Member. Rows that run when the condition becomes true.
- `(whenever-rule source:Expr/Trigger header:Header* row:Step*)` is Member. Rows that run each time the source changes.
- `(subscribe-for source:Clause arm:SourceArm* row:Step*)` is Top, Member. Rows that run for each item the source sends.
- `(and-for source:Clause)` is SourceArm. A second source walked together with the first.
- `(subscribe source:Clause row:Step*)` is Top, Member. Rows that run each time the source sends.
- `(implements type:Type)` is Member, Header. An interface the type provides.
- `(using name:Type)` is Member, Header, Style. A style sheet or helper the type uses.
- `(delegate type:Type+)` is Member. Interfaces the type provides by passing them to the property on the next row.
- `(style-domains name:Name domain:Domain+)` is Member. The work that is redone when the property changes, such as layout or paint.
- `(domain name:Name target:Name?)` is Domain. One kind of work, and the related node it is redone on when one is named.
- `(depends name:Name on:Name+)` is Member. A method of the type that is called whenever one of the named properties changes.
- `(unit access:Access name:Atom value:Expr)` is Member, Step, Value. States that an amount of one unit equals an amount of another, so values convert between them.

### Statements

- `(let target:Binder/Pattern value:Expr/Block?)` is Step, Member, Case, Let. Introduce a local and set it now. A pattern as the target takes the value apart into the names it captures.
- `(let name:Local type:Type/Hole value:Expr/Block?)` is Step, Member, Case, Let. Introduce a local of the type, and set it now when a value follows. A `_` type leaves the type to the value, as a value that is itself a type needs: `(let kind _ I32)`.
- `(lazy target:Binder/Pattern value:Expr/Block/Rule?)` is Lazy, Case, Member, State, Binding. Name a value that is computed where it is first used.
- `(lazy name:Local type:Type value:Expr/Block/Rule?)` is Lazy, Case, Member, State, Binding. Name a value of the type that is computed where it is first used.
- `(pattern mark:Atom* alternative:Stream*)` is Rule. A stream pattern rule.
- `(return value:Expr/Block?)` is Step. Leave the function, with a value when it returns one.
- `(return? value:Expr/Block)` is Step. Leave the function with the value, unless the value is null.
- `(return-if value:Expr? guard:Guard)` is Step. Leave the function when the condition holds.
- `(if condition:Test)` is Guard. The condition.
- `(yield value:Expr/Block)` is Step. Send the next value of a stream.
- `(yield? value:Expr/Block)` is Step. Send the value unless it is null.
- `(yield-all value:Expr/Block)` is Step. Send every value of another stream.
- `(yield-all? value:Expr/Block)` is Step. Send every value of another stream unless it is null.
- `(break)` is Step. Leave the nearest loop.
- `(continue)` is Step. Go to the next turn of the nearest loop.
- `(if condition:Test row:Step+ arm:Arm*)` is Step, Child. Run the rows when the condition holds. The arms are tried otherwise.
- `(if condition:Test body:Inline arm:Arm*)` is Step, Child. The same, with a one-step body.
- `(else-if condition:Test row:Step*)` is Arm. Tried when no condition before it held.
- `(else-if condition:Test body:Inline)` is Arm. The same, with a one-step body.
- `(else row:Step*)` is Arm. Run when no condition held.
- `(else body:Inline)` is Arm. The same, with a one-step body.
- `(inline body:Step/Expr)` is Inline. A one-step body.
- `(if condition:Test row:Child* arm:ChildArm*)` is Child. Inside markup: the rows are children added when the condition holds.
- `(else-if condition:Test row:Child*)` is ChildArm. Inside markup: children added when no condition before it held.
- `(else row:Child*)` is ChildArm. Inside markup: children added when no condition held.
- `(while condition:Test row:Step+)` is Step. Run the rows as long as the condition holds.
- `(while condition:Test body:Inline)` is Step. The same, with a one-step body.
- `(for source:Clause row:Step* arm:LoopArm*)` is Step, Child. Run the rows once for each item of the source. Rows may instead be in its last `and-for` or in its `or-for` arms, and a loop with none only walks the source.
- `(for source:Clause body:Inline arm:LoopArm*)` is Step, Child. The same, with a one-step body.
- `(for source:Clause row:Child* arm:LoopArm*)` is Child. Inside markup: the rows are children added for each item.
- `(and-for source:Clause row:Step*)` is LoopArm. A second source walked together with the first. The rows of the loop go in the last one, and the loop ends when either source ends.
- `(or-for source:Clause row:Step*)` is LoopArm. A further loop over another source, joined to the first as an alternative. Its rows run for the items of its own source.
- `(in item:Binder/Expr source:Expr option:Option*)` is Clause, Expr, Pattern. Each item of a collection.
- `(on item:Binder/Expr source:Expr option:Option*)` is Clause, Expr. Each event a source sends.
- `(in source:Expr option:Option*)` is Clause. Each item of a collection, unnamed. The rows read the item as `each`.
- `(on source:Expr option:Option*)` is Clause. Each event a source sends, unnamed. The rows read the event as `each`.
- `(each item:Any option:Option*)` is Clause, Expr. Each item, with no source named.
- `(and-for source:Clause+)` is Clause. Sources walked together.
- `(when condition:Expr)` is Option. Keep only the items for which the condition holds.
- `(whenever condition:Expr)` is Option. Listen to the source only while the condition holds. The condition reads tracked state and not the item.
- `(while condition:Expr)` is Option. Stop at the first item for which the condition fails.
- `(until condition:Expr)` is Option. Stop at the first item for which the condition holds.
- `(keyed-by key:Expr)` is Option. The identity of each item, in a markup loop.
- `(do deco:Deco* row:Step*)` is Step, State, Before. Run the rows as one step.
- `(do deco:Deco* body:Inline)` is Step, State, Before. Run one step.
- `(finally row:Step*)` is Step, State, Before. Rows that run when the enclosing body ends, however it ends.
- `(finally body:Inline)` is Step, State, Before. One step that runs when the enclosing body ends.
- `(do-new worker:Worker? deco:Deco* row:Step*)` is Step, Before. Run the rows as a new task.
- `(do-new worker:Worker? deco:Deco* body:Inline)` is Step, Before. Run one step as a new task.
- `(do-new-thread worker:Worker? deco:Deco* row:Step*)` is Step, Before. Run the rows as a task in the shared thread pool.
- `(do-new-thread worker:Worker? deco:Deco* body:Inline)` is Step, Before. Run one step as a task in the shared thread pool.
- `(worker: value:Expr)` is Worker. The worker that runs the task.
- `(do-with name:Name value:Expr row:Step*)` is Step, Before. Run the rows with a stack value set for them and for what they call. The value is restored after.
- `(do-use resource:Expr row:Step*)` is Step, Before. Run the rows with a resource that is released at the end.
- `(do-use? resource:Expr row:Step*)` is Step, Before. The same, skipped when the resource is null.
- `(throw error:Expr guard:Guard? handler:Handler*)` is Step, Value, Row, Case, Member, Before. Raise an error, only when the condition holds if one is given. With handlers below it, it raises nothing now: an error the handlers name is replaced by this one when it leaves the function.
- `(on-error error:Binder/Caught row:Step/Value*)` is Step, Value, Row, Handler, Case. Rows that run when the form above raises the error.
- `(caught error:Binder earlier:Binder+)` is Caught. An error together with the errors it replaced.
- `(print value:Expr/Block?)` is Step, Value, Before. Print a value. With no value it does nothing and stands for an empty body.
- `(print? value:Expr)` is Step, Value, Before. Print a value unless it is null.
- `(with value:Expr)` is Step, Value, Row, Case. The first row of a `match`: the helper object its patterns call.
- `(header value:Expr)` is Step, Value, Child. Starts a section with a value. Each value row after it, up to the next header, is handed to that value.
- `(paragraph line:Line*)` is Step, Value. A text body, one item for each line.

### Assignment

- `(= target:Target value:Expr/Block)` is Step, Child, Style, Assign. Store a value.
- `(= (| pattern:Pattern+) value:Expr)` is Expr. True when the value fits the pattern. The pattern may name parts of the value.
- `(= (~ pattern:Stream+) value:Expr)` is Test. True when the value's items fit the stream pattern. The pattern may name parts of what it matched.
- `(=? target:Target value:Expr/Block)` is Step, Assign. Store a value unless the value is null.
- `(+= target:Target value:Expr/Block)` is Step, Assign. Add to what is stored.
- `(+=? target:Target value:Expr/Block)` is Step, Assign. Add to what is stored, unless the value is null.
- `(-= target:Target value:Expr/Block?)` is Step, Assign, Expr. Subtract from what is stored. With no value, remove the item the target names and give what the removal gives.
- `(-=? target:Target value:Expr/Block)` is Step. Subtract from what is stored, unless the value is null.
- `(*= target:Target value:Expr/Block)` is Step. Multiply what is stored.
- `(/= target:Target value:Expr/Block)` is Step. Divide what is stored.
- `(%= target:Target value:Expr/Block)` is Step. Store the remainder.
- `(%%= target:Target value:Expr/Block)` is Step. Raise what is stored to a power.
- `(++= target:Target value:Expr/Block)` is Step. Append every item of a collection.
- `(*=? target:Target value:Expr/Block)` is Step. Multiply what is stored, unless the value is null.
- `(/=? target:Target value:Expr/Block)` is Step. Divide what is stored, unless the value is null.
- `(%=? target:Target value:Expr/Block)` is Step. Store the remainder, unless the value is null.
- `(%%=? target:Target value:Expr/Block)` is Step. Raise what is stored to a power, unless the value is null.
- `(++=? target:Target value:Expr/Block)` is Step. Append every item of a collection, unless it is null.
- `(<- target:Expr value:Expr*)` is Step, Expr. Send values to a channel in order, or read one from it.
- `(<-* target:Target value:Expr/Block)` is Step. Send every value of a stream.
- `(<-*? target:Target value:Expr/Block)` is Step. Send every value of a stream, unless it is null.

### Calls

- `(call callee:Expr arg:Arg*)` is Call, Expr, Step, Value, Pattern, Stream, Target. Call a function with arguments. A method is called through `(. object name)`.
- `(:label value:Expr/Block?)` is Arg, Row, Named. A named argument. The head is a colon and the argument's name.
- `(block row:Row* value:Given? bind:Lazy*)` is Arg. Arguments written one to a row, or steps whose last row is the argument.
- `(inline body:Step/Expr)` is Arg. A one-step body given as the last argument.
- `(arg value:Expr/Block)` is Row. One positional argument.
- `(entry key:Expr value:Expr/Block?)` is Row, Arg. One key and its value.
- `(entry? key:Expr value:Expr/Block)` is Row, Arg. One key and its value, left out when the value is null.
- `(bullet value:Expr/Block)` is Row, Value, Case. One item of a list being built.
- `(bullet? value:Expr/Block)` is Row, Value, Case. One item, left out when it is null.
- `(bullet* value:Expr/Block)` is Row, Value, Case. Every item of a collection.
- `(bullet*? value:Expr/Block)` is Row, Value, Case. Every item of a collection, unless it is null.
- `(pipe stage:Atom value:Expr/Stream/Block? row:Row/Value*)` is Row, Value, Case, Step, Stage. One stage of a pipeline over the row above.
- `(scope value:Expr row:Row*)` is Expr, Value, Step, Target. A value with the rows that complete it, such as the `lazy` names it uses.

### Values

- `(. object:Expr name:Name/Type+)` is Expr, Type, Target, Pattern. A member of a value, or a member of that member.
- `(?. object:Expr name:Name+)` is Expr, Target. A member, or null when the value is null.
- `(*. object:Expr name:Name+)` is Expr. The member of every item.
- `([] object:Expr index:Expr?)` is Expr, Target. The item at an index or in a range.
- `([] item:Type)` is Type. The array type of the item type.
- `(?[] object:Expr index:Expr)` is Expr, Target. The item at an index, or null when the collection is null.
- `(!! value:Expr)` is Expr, Target. The value, raising an error when it is null.
- `(as value:Expr type:Type+)` is Expr. The value converted to a type.
- `(?? value:Expr fallback:Expr+)` is Expr. The first value that is not null.
- `(only-if value:Expr guard:Guard)` is Expr, Guarded. The value when the condition holds, otherwise null.
- `(if-expr condition:Test value:Expr otherwise:Expr)` is Expr, Step. The value when the condition holds, otherwise the other value.
- `(str part:Expr+)` is Expr, Line. A text built from texts and values.
- `(char text:Text)` is Expr. One character.
- `(list item:Expr*)` is Expr, Pattern. A list of values.
- `(list item:Type)` is Type. The list type of the item type.
- `(map key:Type value:Type)` is Type. The type of a map from keys to values.
- `(map key:Pattern value:Pattern)` is Pattern. The same, in a pattern.
- `(tuple item:Expr/Binder+)` is Expr, Pattern, Binder. Values grouped in order.
- `(tuple item:Type+)` is Type. The tuple type of the item types.
- `(stream item:Expr)` is Expr, Pattern. A stream that sends the value again each time tracked state it reads changes.
- `(stream item:Type)` is Type. The stream type of the item type.
- `(.. first:Expr last:Expr step:Stride?)` is Expr. The values from first through last.
- `(..< first:Expr last:Expr step:Stride?)` is Expr. The values from first up to, not including, last.
- `(... first:Expr/Hole last:Expr? step:Stride?)` is Expr. An open range.
- `(by: step:Expr)` is Stride. The step between values.
- `(=> params:Params body:Expr/Body)` is Expr. A function value: its parameter list, then the value it produces.
- `(=> params:Params :do body:Step?)` is Expr. A function value that runs one step.
- `(=> params:Params :do (block row:Step*))` is Expr. A function value that runs steps in order.
- `(match subject:Expr case:Case+)` is Expr, Step, Value. The rows of the first arm the value fits.
- `(match? subject:Expr case:Case+)` is Expr, Step, Value. The same, or null when no arm fits.
- `(case pattern:Pattern row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` is Case, Row. An arm taken when the value has this shape.
- `(case pattern:Pattern body:Inline)` is Case, Row. The same, with a one-step body.
- `(case? condition:Test row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` is Case, Value, Row, Step. An arm taken when the condition holds.
- `(case? condition:Test body:Inline)` is Case, Value, Row, Step. The same, with a one-step body.
- `(case= value:Expr row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` is Case, Row. An arm taken when the value equals this one.
- `(case (= pattern:Pattern subject:Expr) row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` is Case, Step, Value, Row. Rows taken when the subject fits the pattern.
- `(case (= pattern:Pattern subject:Expr))` is Expr. The subject narrowed to the pattern, or null when it does not fit.
- `(case-union (= pattern:Pattern subject:Expr) row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` is Case, Step, Value, Row. The same, with a result that keeps its own type.
- `(case-union pattern:Pattern row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` is Case, Row. An arm like `case` whose result keeps its own type, so the match gives a union of the types of its arms.
- `(case-union? condition:Expr row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` is Case, Row. The same, taken when the condition holds.
- `(case-union= value:Expr row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` is Case, Row. The same, taken when the value equals this one.
- `(default row:Step/Row* result:Result? stage:Stage* bind:Lazy*)` is Case, Value, Row, Step. The arm taken when no other is.
- `(default body:Inline)` is Case, Value, Row, Step. The same, with a one-step body.
- `(case-each source:Clause row:Step/Value*)` is Expr, Value. A stream built from a source. For each item that fits the pattern it gives the value of the rows, or the item when there are no rows. Other items are left out.
- `(case-each source:Clause body:Inline)` is Expr, Value. The same, with a one-step body.
- `(catch call:Expr)` is Expr, Step. The result, or the error it raised, as one value.
- `(catch? call:Expr)` is Expr, Step. The result, or null when it raised an error.
- `(use resource:Expr)` is Expr, Step. A resource released when the body ends.
- `(use? resource:Expr)` is Expr, Step. The same, when the resource is not null.
- `(select source:Expr choice:Any)` is Expr, Case. A value, then the pattern it is produced for. The value is produced where the pattern fits.
- `(iterate part:Any+)` is Expr, Value, Step. A loop written as a value: a stream of the results of its rows for each item.
- `(or-for source:Any row:Any*)` is Joined. A further loop of an `iterate`, with its rows.
- `(type-of value:Expr)` is Expr. The type of a value.
- `(facts-of value:Expr)` is Expr. What the compiler knows about a value.
- `(## value:Expr)` is Expr. The latest value a stream has sent. An expression that reads it is worked out again when the stream sends a new value.
- `(preview value:Expr)` is Expr. A value recorded in a probe without turning it into text.
- `(snapshot value:Expr)` is Expr. A value copied into a probe.

### Operators

- `(not value:Test)` is Expr. Not.
- `(- left:Expr right:Expr*)` is Expr, Pattern. Subtract, or negate one value.
- `(+ left:Expr right:Expr*)` is Expr, Pattern. Add.
- `(* left:Expr right:Expr*)` is Expr, Pattern. Multiply.
- `(/ left:Expr right:Expr+)` is Expr. Divide, or join a path.
- `(% left:Expr right:Expr+)` is Expr. Remainder.
- `(%% left:Expr right:Expr+)` is Expr. Raise to a power.
- `(++ left:Expr right:Expr+)` is Expr. Join two collections or texts.
- `(and left:Test right:Test+)` is Expr. True when every value is.
- `(or left:Test right:Test+)` is Expr. True when any value is.
- `(== left:Expr right:Expr+)` is Expr. Equal.
- `(!= left:Expr right:Expr+)` is Expr. Not equal.
- `(< left:Expr right:Expr+)` is Expr. Less than.
- `(> left:Expr right:Expr+)` is Expr. Greater than.
- `(<= left:Expr right:Expr+)` is Expr. At most.
- `(>= left:Expr right:Expr+)` is Expr. At least.
- `(is value:Expr shape:Pattern+)` is Expr. True when the value is this type, never by conversion: its own type, a class it derives from, an interface it implements, or the case its union holds. Or when it fits this shape.
- `(is-not value:Expr shape:Pattern+)` is Expr. True when the value is not this type and does not fit this shape.
- `(is! value:Expr shape:Pattern+)` is Expr. True when the value's type is exactly this one, not a class derived from it.
- `(is!-not value:Expr shape:Pattern+)` is Expr. True when the value's type is not exactly this one.
- `(is~ value:Expr shape:Pattern+)` is Expr. True when the value can be converted to this type.
- `(is~-not value:Expr shape:Pattern+)` is Expr. True when the value cannot be converted to this type.
- `(is-in value:Expr collection:Expr)` is Expr. True when the collection holds the value.
- `(not-in value:Expr collection:Expr)` is Expr. True when the collection does not hold the value.
- `(has collection:Expr value:Expr)` is Expr. True when the collection holds the value.
- `(has-not collection:Expr value:Expr)` is Expr. True when the collection does not hold the value.
- `(>> left:Expr right:Expr+)` is Expr. Reserved for composing two functions, left first. The compiler does not build it yet.
- `(<< left:Expr right:Expr+)` is Expr. Reserved for composing two functions, right first. The compiler does not build it yet.
- `(?>> left:Expr right:Expr+)` is Expr. Reserved, like `>>`, for a value that may be null.
- `(?<< left:Expr right:Expr+)` is Expr. Reserved, like `<<`, for a value that may be null.
- `(?<- left:Expr right:Expr+)` is Expr. Send a value to a channel unless it is null.
- `(|> left:Expr right:Expr+)` is Expr. Pass the value to the next stage.
- `(|=> left:Expr right:Expr+)` is Expr. Pass the value to an expression that reads it as `it`.
- `(<| left:Expr right:Expr+)` is Expr. Reserved for passing a value backward. The compiler does not build it yet.
- `(*> left:Expr right:Expr+)` is Expr. Map each item.
- `(?> left:Expr right:Expr+)` is Expr. Keep the items that pass.
- `(?*> left:Expr right:Expr+)` is Expr. Map each item and drop nulls.
- `(*|> left:Expr right:Expr+)` is Expr. Run a step for each item and pass the item on unchanged.
- `(-> left:Expr right:Expr+)` is Expr, Step. Write the whole value to a target in one write. The result is the target.
- `(*-> left:Expr right:Expr+)` is Expr, Step. Write each item to a target, one write for each.
- `(~> left:Expr right:Any+)` is Expr. Match a pattern again and again along a stream. The result is a stream of what each match produces.
- `(/infix/ left:Expr right:Expr+)` is Expr. A named operator; the head is its name between slashes.

### Types As Forms

- `(of base:Expr argument:Type+)` is Type, Expr, Key, Pattern, Stream. A generic name with its type arguments.
- `(? type:Any)` is Type, Expr, Pattern, Stream. An optional type, or an optional part of a stream pattern.
- `(channel item:Type)` is Type. A channel of values.
- `(fn-type result:Type/Hole part:Raises/Header*)` is Type. The type of a function.
- `(c-fn-type result:Type/Hole part:Raises/Header*)` is Type. The type of a C function.
- `(c-struct field:Any*)` is Type. A C structure.
- `(exact type:Type)` is Type, Pattern. Exactly this type and no subtype.
- `(similar type:Type)` is Type, Pattern. This type, or any type that converts to it.
- `(: name:Atom type:Any)` is Binder, Pattern, Type, Expr, Stream. A name with its type, or a name for part of a pattern.

### Patterns

- `(call rule:Expr part:Stream*)` is Stream. A stream rule applied to arguments.
- `(call callee:Expr part:Pattern*)` is Pattern. A case of a union and patterns for the values it holds.
- `(list item:Pattern*)` is Pattern. A list whose items fit these patterns. `...` stands for the rest.
- `(tuple item:Pattern+)` is Pattern. A tuple whose items fit these patterns.
- `(= key:Expr value:Pattern)` is Pattern. An entry of a map whose value fits the pattern.
- `(* part:Stream+)` is Stream. The part, any number of times.
- `(+ part:Stream+)` is Stream. The part, one or more times.
- `(! part:Stream)` is Stream. Not followed by the part.
- `(& part:Stream)` is Stream. Followed by the part, which is not consumed.
- `(all part:Any+)` is Pattern. Every part must fit.
- `(| part:Any+)` is Pattern, Stream, Expr. Any one part must fit.
- `({} part:Any+)` is Pattern, Stream, Expr. A test on the matched value, or a repeat count in a stream pattern.
- `(~ pattern:Any part:Any*)` is Stream, Case, Value, Row, Step, Expr, Pattern. A stream pattern and what its match produces.
- `(seq part:Any+)` is Stream. Parts that must follow one another.
- `(chars set:Text)` is Stream. Any one character of the set.
- `(\{} test:Any)` is Stream. A test on the next item.
- `(into name:Atom+)` is Stream. Where a rule's captures go.
- `(of: source:Any)` is Stream. The value the pattern is matched against, once.
- `(in: source:Any+)` is Stream. The source the pattern is matched along, again and again. The result is a stream of the matches.
- `(on: source:Any+)` is Stream. The source of events the pattern is matched along, again and again. The result is a stream of the matches.
- `(<* pattern:Any)` is Expr, Stream, Pattern. Every match of a pattern.
- `(/ left:Pattern right:Pattern+)` is Pattern. Reserved for the child relation between tree patterns. The compiler does not match it yet.
- `(*/ left:Pattern right:Pattern+)` is Pattern. Reserved for the descendant relation between tree patterns. The compiler does not match it yet.
- `(\ left:Pattern right:Pattern+)` is Pattern. Reserved for the parent relation between tree patterns. The compiler does not match it yet.
- `(*\ left:Pattern right:Pattern+)` is Pattern. Reserved for the ancestor relation between tree patterns. The compiler does not match it yet.
- `(-- left:Pattern right:Pattern+)` is Pattern. Reserved for the previous-sibling relation between tree patterns. The compiler does not match it yet.
- `(++ left:Pattern right:Pattern+)` is Pattern. Reserved for the next-sibling relation between tree patterns. The compiler does not match it yet.

### Markup

- `(<> type:Type attr:Attr* use:Use* row:Child*)` is Step, Value, Expr, Child, Style. An element: its type, its attributes, the style rules it uses, then its properties and children.
- `(+<> type:Type attr:Attr* use:Use* row:Child*)` is Step, Child. Declares a slot: a named place in a component that a caller fills with an element of this type. Its rows give the default content and the properties applied to what fills it.
- `(<+ name:Name)` is Step, Child. Passes the content the caller gave for the named slot on to the element this row is inside.
- `(id: name:Name)` is Attr. The name the element is known by.
- `(slot: name:Name)` is Attr. The slot the element fills.
- `(param: name:Name type:Type?)` is Attr. A value the slot passes in.
- `(value: value:Expr)` is Attr. The element's main value.
- `(text: part:Any+)` is Attr, Arg. The element's text.
- `(• name:Name argument:Expr* guard:UseIf?)` is Use, Style, Child. Use a style rule.
- `(if: condition:Expr)` is UseIf. Use the rule only when the condition holds.
- `(:label value:Expr/Block?)` is Child. A property set by name.

### Styles

- `(style name:Name clause:SheetClause* row:Style/Member*)` is Top. A style sheet.
- `(is base:Type)` is SheetClause. The style sheet this one derives from. It inherits the base's slots and rules, and overrides a slot with `(prop + name _ (= value))`.
- `(for tag:Name+)` is SheetClause. The tags under which a derived style sheet applies, such as `light` or `dark`. It follows `(is ...)`.
- `(style-extend name:Type row:Style/Member*)` is Top. Rules added to a style sheet declared elsewhere.
- `(rule name:Name parameter:Param? use:Use* row:Style*)` is Style, Member. A named style rule.
- `(own-rule name:Name parameter:Param? use:Use* row:Style*)` is Style. A style rule written in another that reaches only the markup of the class of the element the other is applied to, not what is written in the element's body or given its slots.
- `(when-style state:Expr row:Style*)` is Style. Rows that apply while the element is in a state.
- `(when-style state:Expr body:Inline)` is Style. The same, with one row.
- `(style-set state:Name name:Name value:Expr)` is Style. One style value that applies while the element is in the named state.
- `(keyframes name:Name parameter:Param? kind:Atom duration:Expr row:Keyframe*)` is Style, Member. A keyframe `animation` or `motion`: a slot whose value calls the builder its kind names. A parameter makes it a shape, which `(shape name value)` binds.
- `(= name:Name value:Expr)` is Keyframe. A setting the builder takes, such as `repeat`.
- `(range property:Keyed from:Expr/Hole to:Expr/Hole option:Named*)` is Keyframe. A property that moves from one value to another. `_` leaves an end out.
- `(track property:Keyed option:Named* stop:Stop*)` is Keyframe. A property that passes through its stops.
- `(at at:Expr value:Expr option:Named*)` is Keyframe, Stop. The value at a point of the animation.
- `(shape shape:Expr value:Expr)` is Expr. A shape bound to a value, as `shake:3` binds `shake` to 3.

### State Machines

- `(state target:Expr trigger:Trigger? row:State/Step*)` is State, Member, Step. A state, what enters it, and what happens in it.
- `(then target:Expr trigger:Trigger? row:Any*)` is State. A move to another state.
- `(else-state target:Expr)` is State. When no move before it applies, leave the named state. It does not enter it.
- `(trigger kind:Atom part:Any+)` is Trigger. What causes a move, or what a `whenever-rule` watches.

### Probes

- `(probe target:Any part:Any*)` is Step, Value, Child, Before. Run a step only when the probe is on.
- `(probe-count target:Any amount:Expr? deps:Block?)` is Step, Value, Child, Before. Count an event. The block names what the count reads.
- `(probe-group mark:Atom target:Any part:Any*)` is Step, Value, Child, Before. Open or close a group of probe output.
- `(probe-assign target:Any value:Expr deps:Block?)` is Step, Value, Child, Before. Record a value. The block holds rows that belong to the probe.
- `(probe-log target:Any part:Any*)` is Step, Value, Child, Before. Record a line of output.
- `(items: value:Any+)` is Any. The values a probe logs.

## What The Checker Does Not See

The checker knows the shape of every form. It does not know these things.

- `(if condition a b)` is not a choice between `a` and `b`. Every row after the condition runs
  when the condition holds. The other branch is `(else …)`, written as the last item of the `if`.
- `=` always stores. The checker rejects `(= a b)` where a condition is taken, and nowhere else.
  To compare, write `(== a b)`.
- The constants are `null`, `true` and `false`. To the checker `nil`, `t` and `#t` are names.
- Whether a name exists, what a function takes and whether types agree are the compiler's work.

## Checking

Run `yumi lint file.bx`. Each mistake is reported as `file:line:column rule [status] message`:
`syntax/parse-error` for a form the catalog rejects, and the project's lint rules, such as
`layout/line-length` and `layout/depth` for forms that make a row too wide or nest too deep.

- `--fix` rewrites the file with the safe fixes: canonical layout, declarations sorted by kind,
  `(main …)` around top-level code, `(if …)` moved to the end of a guarded form, `fn-do` for a
  `fn-expr` whose rows are all steps, `:do` added to a function value whose rows are all steps,
  and each rule's own fix.
- `--fix-rule syntax/parse-error --fix-rule layout/format` limits it to those first fixes.
