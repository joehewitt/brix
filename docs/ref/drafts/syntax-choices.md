# Why Yumi looks the way it does

I made several choices in Yumi that are likely to look strange on first contact. Some remove
familiar punctuation. Others add punctuation where most languages use words. A few take things
normally left to a style guide and make them part of the grammar.

The thread connecting these choices is how I want source code to read. I want declarations to
have an obvious outline, names to look like words, and expressions to put their main idea where
the eye arrives first. I am willing to spend more vertical space to make those relationships
visible.

That is a preference, not a claim that unfamiliar syntax is inherently better. Familiarity is
a substantial advantage. Yumi asks readers to give some of it up, and each choice has to earn
back that cost.

## At a glance

| Capability | What it gives you |
| --- | --- |
| Visible outline | Significant dividers, flush-left bodies, one parameter per line. |
| Reliable name shapes | Kebab-case values, capitalized types, meaningful operator spaces. |
| Compact grammar | Symbols, apostrophe type arguments, bare-path imports. |
| Readable relationships | Expression functions, named arguments, named infix calls. |

## 1. Significant dividers

Horizontal rules are grammar in Yumi. They establish declaration ownership, not just visual separation.

:::example-pair

- [Top-level boundary](#source-dividers--top): A thick divider begins the type declaration.
- [Member boundary](#source-dividers--member): A thin divider keeps `label` inside `Token`.
- [Next declaration](#source-dividers--next): The next thick divider ends the type's section; `double` is a module function.

```yumi #dividers
-- Token("hello").label()

⟦top⟧━━━━━━━━━━━━━━━━━━━━
+ Token is Tuple⟦/top⟧

+ text String

⟦member⟧────────────────────
+ label String⟦/member⟧

"[$text]"

⟦next⟧━━━━━━━━━━━━━━━━━━━━
+ double Int⟦/next⟧
    - value Int

value * 2
```

:::

**Compare the notation:**

```text
class Token {
    label() {
        return "[" + this.text + "]";
    }
}

function double(value) {
    return value * 2;
}
```

**Why I chose it**

- I want a file's larger divisions visible before I read the words.
- A conspicuous boundary lets a body sit at the left margin while keeping its owner clear.

**Tradeoffs**

- A divider costs a whole line and is less convenient to type than a brace; editor support matters.
- Changing a thin divider to a thick one changes structure. Fonts, diffs, and copied snippets must preserve the distinction.

## 2. Kebab case identifiers

Ordinary names use hyphens between words.

:::example-pair

- [Word boundaries](#source-kebab-case--name): Each hyphen belongs to a name; the spaced `*` is an operator.

```yumi #kebab-case
. item-count = 3
. unit-price = 20
. ⟦name⟧total-price = item-count * unit-price⟦/name⟧
-- total-price
```

:::

**Why I chose it**

- I find `total-price` easier to read as a compound phrase than `totalPrice` or `total_price`.
- The boundaries are explicit without changing letter height at every word.

**Tradeoffs**

- `total-price` is one identifier; `total - price` is subtraction. This choice depends on significant spacing.
- Generic editor word navigation may split at hyphens. Selection, search, and rename need language-aware support.

## 3. Mandatory identifier case

Capitalization reliably distinguishes everyday Latin type names from value names.

:::example-pair

- [Type name](#source-identifier-case--type): Begin with a capital and use internal capitals as needed.
- [Value name](#source-identifier-case--value): Use lowercase words separated by hyphens.

```yumi #identifier-case
. reading = TemperatureReading(21)
-- reading.degrees

━━━━━━━━━━━━━━━━━━━━
⟦type⟧+ TemperatureReading is Tuple⟦/type⟧

⟦value⟧+ degrees Int⟦/value⟧
```

:::

**Why I chose it**

- I want a reader to recognize a name's basic category without finding its declaration.
- Many languages recommend this distinction. I want both readers and the parser to be able to rely on it.

**Tradeoffs**

- Naming freedom becomes a language constraint: `temperatureReading` is not an ordinary value identifier, and `Temperature-Reading` is not the type spelling.
- Acronyms and foreign naming conventions need accommodation. C identifiers use forms such as `C:someFunction`; supported Greek identifiers have their own rules.

## 4. Mandatory whitespace around operators

Spaces around ordinary binary operators do grammatical work.

:::example-pair

- [One name](#source-operator-spacing--identifier): Read the hyphenated identifier: `100`.
- [Subtraction](#source-operator-spacing--subtract): Spaces distinguish two operands and an operator: `7`.
- [Precedence remains](#source-operator-spacing--precedence): Multiplication still comes first: `16`.

```yumi #operator-spacing
. total = 10
. price = 3
. total-price = 100
⟦identifier⟧-- total-price⟦/identifier⟧
⟦subtract⟧-- total - price⟦/subtract⟧
⟦precedence⟧-- total + price * 2⟦/precedence⟧
```

:::

In a C-like language, `total-price` normally subtracts regardless of spacing. That freedom conflicts with the names I want.

**Why I chose it**

- Kebab-case names require a predictable distinction between a word and subtraction.
- I prefer a consistent visual rhythm: solid names, with space around the operations connecting them.

**Tradeoffs**

- An edit that looks like formatting can change meaning or invalidate an expression.
- This is not a universal spacing rule for symbols. `-3`, `!ready`, member access, calls, and type punctuation follow their own grammar.

## 5. Function bodies always flush to the left

Parameters sit under the declaration header; the function body returns to the left margin.

:::example-pair

- [Signature indentation](#source-left-margin--parameters): Group the inputs under their function.
- [Body at the margin](#source-left-margin--body): The divider and declaration establish ownership.

```yumi #left-margin
-- price-label(3, 20)

━━━━━━━━━━━━━━━━━━━━
+ price-label String
⟦parameters⟧    - count Int
    - unit-price Int⟦/parameters⟧

⟦body⟧"Total: ${count * unit-price}"⟦/body⟧
```

:::

**Compare the notation:**

```text
def price_label(count, unit_price):
    return f"Total: {count * unit_price}"
```

**Why I chose it**

- I want indentation to express structure inside the body: branches, nested expressions, and block arguments.
- Methods do not lose another level of horizontal space merely because they belong to a type.
- Moving work between a module function and a method can preserve its visual shape.

**Tradeoffs**

- Readers accustomed to indented bodies may initially mistake this for module-level code.
- Dividers must make ownership clear. Control-flow blocks still rely on indentation.

## 6. One argument per line

Each ordinary declared parameter gets a line. Longer calls can use a matching block form.

:::example-pair

- [Block call](#source-argument-lines--call): Give each supplied argument a line.
- [Parameter outline](#source-argument-lines--signature): Keep each name and type together in a consistent column.

```yumi #argument-lines
⟦call⟧-- address-label
    - "Ada"
    - "10 Main Street"
    - "Honolulu"⟦/call⟧

━━━━━━━━━━━━━━━━━━━━
+ address-label String
⟦signature⟧    - recipient String
    - street String
    - city String⟦/signature⟧

"$recipient, $street, $city"
```

:::

**Compare the notation:**

```text
function addressLabel(recipient: String, street: String, city: String): String
```

Compact calls such as `address-label("Ada", "10 Main Street", "Honolulu")` remain available. The line-per-argument form is an option at call sites.

**Why I chose it**

- I want signatures to scan downward, with room for each name, type, and default.
- A parameter change tends to produce a small, localized diff.

**Tradeoffs**

- Tiny functions can spend more lines on their signatures than their bodies.
- Vertical layout does not explain the roles of three positional strings. Named arguments solve that separate problem.

## 7. Prefer symbols over keywords

Small, recurring grammatical roles often use symbols. Pipelines make that preference especially visible.

:::example-pair

- [Expand input](#source-symbols--input): Contribute a range to the stream.
- [Filter](#source-symbols--filter): Keep values above two.
- [Transform](#source-symbols--map): Produce `30, 40, 50`.

```yumi #symbols
. values = List
    ⟦input⟧** 1 .. 5⟦/input⟧
    ⟦filter⟧?> each > 2⟦/filter⟧
    ⟦map⟧*> each * 10⟦/map⟧
-- values
```

:::

**Compare the notation:**

```text
let values = range(1, 6)
    .filter(item => item > 2)
    .map(item => item * 10);
```

| Yumi form | Conventional role |
| --- | --- |
| `. value = ...` | Local binding |
| `.. value = ...` | Lazy binding |
| `+ name` / `- parameter` | Public declaration / ordinary parameter |
| `= value` in a procedure | `return value` |
| `<= value` in a generator | `yield value` |
| `<<= values` | `yield from values` |

This is a preference, not a ban on keywords: `match`, `if`, `for`, and `do` still have jobs.

**Why I chose it**

- Once learned, the marks let me scan the shape of the work from its left edge.
- I want repeated grammar to occupy less attention than the expressions it organizes.

**Tradeoffs**

- Words such as `filter` give newcomers clues that `?>` does not.
- Symbols are harder to search for or say aloud, and assistive tools may read them inconsistently.
- Familiar marks gain contextual meanings: statement-leading `<= value` yields, while `left <= right` compares.

## 8. Expression functions

An expression function states its answer directly, with supporting calculations below it.

:::example-pair

- [Answer first](#source-expression-first--answer): The body shows the output's shape without `return`.
- [Explain below](#source-expression-first--dependency): Evaluate the named dependency when it is needed.

```yumi #expression-first
-- receipt(3, 20)

━━━━━━━━━━━━━━━━━━━━
+ receipt String
    - count Int
    - unit-price Int

⟦answer⟧"Total: $total"⟦/answer⟧

⟦dependency⟧.. total = count * unit-price⟦/dependency⟧
```

:::

**Compare the notation:**

```text
function receipt(count, unitPrice) {
    const total = count * unitPrice;
    return `Total: ${total}`;
}
```

**Why I chose it**

- I want to see the returned record, text, or decision before its supporting calculations.
- Lazy bindings let source order follow the explanation: read the answer, then look up its ingredients.
- Expression functions should be a normal way to write code, not just a shorthand for tiny helpers.

**Tradeoffs**

- Laziness changes evaluation timing. Reads of mutable state and effects can make that difference observable.
- An expression function is not a guarantee of purity.
- `do` functions remain useful for mutation, cleanup, and early exits with `=`. Readers learn two body shapes.

## 9. Named arguments

A bullet passes an argument by the name of its parameter, so a call can state the role of each input.

:::example-pair

- [Visible relationships](#source-named-arguments--call): The call states which input is the origin and which is the destination.
- [Ordinary parameters](#source-named-arguments--signature): `from` and `to` are plain parameter names, used by the call and by the body.

```yumi #named-arguments
-- ⟦call⟧travel • from "home" • to "office"⟦/call⟧

━━━━━━━━━━━━━━━━━━━━
+ travel String
    ⟦signature⟧- from String
    - to String⟦/signature⟧

"$from -> $to"
```

:::

**Compare the notation:**

```text
travel("home", "office")
travel(origin="home", destination="office")
```

**Why I chose it**

- Named arguments make roles explicit, and relationship words such as `from` and `to` read well as parameter names.
- They help when two inputs share a type and swapping them would still type-check.
- Every named argument has one shape, `• name value`, whether it holds a value or a callback.

**Tradeoffs**

- A parameter has one name, used both at the call site and in the body.
- Vague names such as `with` can still make a poor API.
- The bullets on a line all belong to one call, so a nested call takes its bullets inside parentheses.

## 10. Named infix operators

A two-input function can appear between its operands when its name is surrounded by slashes.

:::example-pair

- [Infix call](#source-named-infix--infix): Place a named relationship between its inputs.
- [Ordinary call](#source-named-infix--ordinary): Use the same function with conventional call syntax; both print `7`.

```yumi #named-infix
-- ⟦infix⟧3 /plus/ 4⟦/infix⟧
-- ⟦ordinary⟧plus(3, 4)⟦/ordinary⟧

━━━━━━━━━━━━━━━━━━━━
+ plus Int
    - left Int
    - right Int

left + right
```

:::

**Compare the notation:**

```yumi
left ///combine/// middle /adjust/ right
left ///combine/// (middle /adjust/ right)
```

| Form | Relative precedence |
| --- | --- |
| `/name/` | Highest of the named forms |
| `//name//` | Middle |
| `///name///` | Lowest |

All bind less tightly than ordinary addition and multiplication. Slash count belongs to the call, not the function name. Alternatives include `combine(left, right)` and `left.combine(right)`.

**Why I chose it**

- I want left-to-right relationships without making an unmarked word ambiguous between operand and operator.
- A domain operation can use an existing name without inventing a punctuation character. Bitwise operations use forms such as `flags /and/ mask`.

**Tradeoffs**

- Readers must notice slash count to understand precedence. Domain vocabulary does not automatically make a long expression clear.
- I would use parentheses whenever grouping deserves attention, or keep the ordinary call when it is clearer.

## 11. Parameterized types with apostrophes

Apostrophes attach type arguments to the type name.

:::example-pair

- [Apply a type](#source-type-arguments--application): Supply `Int` for the type parameter.
- [Declare a parameter](#source-type-arguments--parameter): `Item` names the type used by the field.

```yumi #type-arguments
. box = ⟦application⟧Box'Int(42)⟦/application⟧
-- box.value

━━━━━━━━━━━━━━━━━━━━
⟦parameter⟧+ Box'Item is Tuple⟦/parameter⟧

+ value Item
```

:::

| Yumi | Common angle-bracket spelling |
| --- | --- |
| `Box'Int` | `Box<Int>` |
| `Map'String'Int` | `Map<String, Int>` |
| `List'(List'Int)` | `List<List<Int>>` |

Square-bracket application, such as `Box[Int]`, is another common alternative.

**Why I chose it**

- A common short application needs no closing delimiter.
- Flat argument lists avoid accumulating commas and angle brackets.
- Apostrophes leave angle brackets available for streams and comparisons.

**Tradeoffs**

- Nested applications need grouping, and a long chain of apostrophes can become hard to scan.
- Copying must preserve the ASCII apostrophe rather than turn it into a typographic quote.
- This simplifies a spelling, not generic inference or constraints.

## 12. Imports are bare paths

Dependencies appear as bare paths at the top of a file.

:::example-pair

- [Library lookup](#source-imports--library): An absolute module path, not an operating-system root directory.
- [Relative lookup](#source-imports--relative): A module near the current file.

```yumi #imports
⟦library⟧/time/duration⟦/library⟧
⟦relative⟧./helpers⟦/relative⟧
```

:::

Conventional alternatives include `import time.duration` or `import "./helpers"`.

**Why I chose it**

- I want the dependency list to look like a list of dependencies, without a repeated keyword.
- Slash notation keeps hierarchy and relative references visible.

**Tradeoffs**

- The spelling borrows filesystem expectations, even though library lookup gives an absolute module path its meaning.
- Removing `import` removes an obvious newcomer clue and a token that generic search tools can recognize.

## The choices as a whole

These choices are easier to explain together than in isolation. Dividers support flush-left
bodies. Hyphenated names depend on meaningful spaces around operators. Parameter lines and
named arguments give calls an outline. Expression functions put the result first, while symbols
keep the recurring scaffolding compact.

There is a tension in that design: I spend lines generously and characters selectively. I
want a file to have enough visible structure to navigate, while keeping each expression's
main content close together.

I also know that every departure from convention spends some of the reader's patience.
Consistency does not erase that initial cost, and taste is not a substitute for observing
where people struggle. My hope is that, once the vocabulary is familiar, the choices work
together well enough that reading Yumi feels simpler than the list of rules makes it sound.
