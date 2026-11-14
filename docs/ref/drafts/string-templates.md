# String templates

A string template function lets its body look like the text it produces. The declaration
begins with `¶`, followed by the function's name and ordinary parameters. After the signature,
the body is text with embedded Yumi expressions.

This is useful for messages, documents, reports, and generated source. The surrounding text
remains readable as a whole, while the changing parts come from the same functions, types,
and expressions used elsewhere in the program.

## At a glance

| Capability | What it gives you |
| --- | --- |
| `¶` function | Write a multiline document as the body of a function returning text. |
| `$name`, `${expression}` | Insert values and calculations from ordinary Yumi code. |
| `════════════════════` | End literal text and define supporting calculations below it. |
| Conditions and repetition | Use conditional expressions, matches, comprehensions, and `join`. |
| Composition | Call templates from templates, or define a type's string converter. |
| Output contract | Whitespace and escaping belong to the document format you are producing. |

## Text as a function body

:::example-pair

- [Text function](#source-greeting--declaration): `¶` appears before the function name; no outer quotes or explicit return are needed.
- [Ordinary input](#source-greeting--input): The template accepts parameters like any other function.
- [Two paragraphs](#source-greeting--body): The call inserts `Ada` and returns the text. The caller chooses whether to print, save, or send it.

```yumi #greeting
-- greeting("Ada")

━━━━━━━━━━━━━━━━━━━━
⟦declaration⟧+ ¶ greeting⟦/declaration⟧
    ⟦input⟧- name String⟦/input⟧

⟦body⟧Hello, $name.

Welcome to Yumi.⟦/body⟧
```

:::

A short message may fit comfortably in an ordinary quoted string. The template form becomes
more useful as the text acquires lines, indentation, punctuation, and repeated structure.

## Interpolation uses Yumi expressions

`$name` inserts a named value. `${expression}` inserts the result of an expression:

```yumi #interpolation
-- line-item("Apples", 3, 2)

━━━━━━━━━━━━━━━━━━━━
+ ¶ line-item
    - name String
    - quantity Int
    - price Int

⟦name⟧$name⟦/name⟧: $quantity at $price each, total ⟦calculation⟧${quantity * price}⟦/calculation⟧
```

- [Named value](#source-interpolation--name): Insert an input directly.
- [Expression](#source-interpolation--calculation): Compute the total using ordinary arithmetic. Numbers participate in string construction without a separate template evaluator.

Simple property access can use forms such as `$person.name`. Braces are useful for more
substantial expressions and for making the boundary between a name and adjacent text explicit.
For example, `${name}-report` clearly places the hyphenated suffix outside the identifier.

Quotation marks can appear directly in the text body. Interpolation and escape sequences
still have meaning: `\$name` produces the literal text `$name`, and `\\` represents a literal
backslash. The template frees the document from outer quotes without turning it into an
entirely uninterpreted block of bytes.

## Put the document first, then its dependencies

A larger template can name its supporting calculations below the text. The double-line
divider ends the text portion while keeping the following code inside the function:

:::example-pair

- [Document](#source-document-first--text): The main expression shows the receipt before its calculations.
- [Back to code](#source-document-first--boundary): The double-line divider ends text within this function.
- [Dependency](#source-document-first--dependency): The lazy binding supplies the value used by `$total`.

```yumi #document-first
-- receipt("Apples", 3, 2)

━━━━━━━━━━━━━━━━━━━━
+ ¶ receipt
    - name String
    - quantity Int
    - price Int

⟦text⟧Receipt
=======
Item: $name
Quantity: $quantity
Total: $total⟦/text⟧

⟦boundary⟧════════════════════⟦/boundary⟧
⟦dependency⟧.. total = quantity * price⟦/dependency⟧
```

:::

This is the same result-first organization available to
[expression functions](syntax-and-expressions.md#lazy-bindings-put-dependencies-below-the-result).
It also gives calculations a place to grow without making every interpolation harder to read.
A lookup, a match, or a sequence operation can live in a named dependency while the template
continues to show the document's structure.

The dividers have different jobs. `════════════════════` returns from text to code within
the current function. `━━━━━━━━━━━━━━━━━━━━` introduces the next top-level declaration.
Without the text-ending boundary, a line that looks like a calculation is still part of
the template's text.

## Conditional content

Conditional text can use an ordinary conditional expression. There is no need to introduce
a second set of template-specific decision rules:

```yumi #conditional-text
-- heading("Release notes", true)

━━━━━━━━━━━━━━━━━━━━
+ ¶ heading
    - title String
    - is-draft Bool

⟦placement⟧# $title$suffix⟦/placement⟧

════════════════════
.. suffix = ⟦decision⟧if is-draft then " (draft)" else ""⟦/decision⟧
```

- [Placement](#source-conditional-text--placement): The literal body describes where the optional wording belongs.
- [Decision](#source-conditional-text--decision): The choice gives the wording or an empty string.

For a larger decision, the dependency can be a `match` whose branches return text, including
calls to other template functions. A whole section can therefore vary while the surrounding
document remains in one place.

This keeps the responsibility clear: the expression chooses the content, and the template
places that content in the document. A line containing the word `if` in the literal body
does not begin a conditional statement.

## Repetition and composition

A template can call another template just as it calls any other function. Repetition can
come from a comprehension or stream pipeline, with `join` choosing the separator:

:::example-pair

- [Insert the rows](#source-composition--insert): The outer template owns the document.
- [Separate the results](#source-composition--join): The join puts a newline between items.
- [Repeat an ordinary call](#source-composition--repeat): The comprehension calls the item template once per input.
- [Reusable piece](#source-composition--item): The inner template owns one item's presentation and remains independently callable.

```yumi #composition
-- shopping-list(["Apples", "Pears", "Bread"])

━━━━━━━━━━━━━━━━━━━━
+ ¶ shopping-list
    - items [String]

Shopping list
=============
⟦insert⟧$rows⟦/insert⟧

════════════════════
.. rows = ⟦join⟧"\n".join(lines)⟦/join⟧
.. lines = ⟦repeat⟧shopping-item(each) in items⟦/repeat⟧

━━━━━━━━━━━━━━━━━━━━
⟦item⟧+ ¶ shopping-item⟦/item⟧
    - item String

- $item
```

:::

The same arrangement scales to a report containing sections, a message containing optional
details, or generated source containing repeated declarations. Each piece remains callable
on its own and can receive the data it needs through ordinary parameters.

A collection or stream supplies the sequence; the template does not need its own collection
model. [Stream pipelines](stream-pipelines.md) can filter or transform the input before it
becomes text. Joining a sequence into one string does need the sequence to finish, so an
ongoing source needs a meaningful boundary before it can become a complete document.

## Whitespace belongs to the result

Within the text body, indentation is text indentation rather than nested Yumi code:

```yumi #whitespace
━━━━━━━━━━━━━━━━━━━━
+ ¶ outline

⟦indentation⟧Project
    Documentation
        Language guide
    Examples⟦/indentation⟧
```

- [Text indentation](#source-whitespace--indentation): These spaces and internal newlines belong to the result, rather than creating nested Yumi blocks.

Internal newlines, blank lines, and indentation let the source express the layout of the
output. The blank line after the signature introduces the body; it is not an extra opening
paragraph. Trailing literal whitespace at the end of the template is trimmed, which keeps
spacing before a declaration boundary out of the result.

When a final newline is part of the intended output, express it as content rather than relying
on blank source lines at the boundary. For example, a final `${"\n"}` inserts a newline as an
expression result.

Composition also makes whitespace a deliberate choice. A child template returns its text;
interpolating it does not automatically reindent all of its lines to match the insertion
point. A join's separator and any indentation inside the child are part of the resulting
document. This is especially useful to keep in mind when producing nested source code or
other indentation-sensitive formats.

## Templates can define a type's text form

The template form also works for a string converter inside a type:

```yumi #converter
. entry = Entry("Apples", 3)
-- ⟦conversion⟧entry as String⟦/conversion⟧

━━━━━━━━━━━━━━━━━━━━
+ Entry is Tuple

+ name String
+ quantity Int

────────────────────
⟦template⟧+ ¶ to String⟦/template⟧

⟦fields⟧$name × $quantity⟦/fields⟧
```

- [Request text](#source-converter--conversion): The cast invokes the type's string conversion.
- [Template converter](#source-converter--template): A type member can define text with the same syntax as a free function.
- [Read fields](#source-converter--fields): The converter uses the value's fields; this produces `Apples × 3`.

A value may need several representations, however. A short label, a detailed report, and a
serialized record can have different purposes. Named template functions let those choices
remain explicit rather than making one string conversion serve every caller.

## Text generation and structured output

Templates can produce Markdown, HTML, configuration, or source code because their output
is text. They do not infer the escaping rules of the format from what that text resembles.
An interpolation into HTML, JSON, or another format needs the conversion appropriate to
that position. The JSON library, for example, performs escaping explicitly when constructing
a quoted JSON string.

For data serialization, a format library can own that contract. For a document whose wording
and arrangement are the main concern, a template keeps the authored text visible. These
approaches can compose: an embedded value can come from a serializer or a formatting helper.

## String templates and markup

A template containing `<Box>` produces text containing those characters.
[Markup](markup-and-stylesheets.md) constructs a tree of objects with properties, children,
events, and lifetimes. That difference determines which feature fits the result.

| Desired result | Choose |
| --- | --- |
| Email, report, prompt, generated file | A string template |
| Data encoded for a protocol | A format library that owns serialization and escaping |
| Live object tree with events and properties | Markup |
| A component containing longer text | Markup whose property calls a template |

String template functions bring text into the same compositional model as other Yumi code.
Parameters describe the inputs, expressions compute the changing parts, and function calls
assemble larger results. The body remains a document that a reader can recognize.
