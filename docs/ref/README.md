# Yumi Language Reference

This reference describes the whole Yumi language: every form a program can contain, what each one
means, and the rules the compiler enforces. It is written to be complete rather than gentle. Read a
chapter from the top to learn a feature, or jump to an entry to look one up.

Yumi programs are written in Brix, in `.bx` files. Each construct is stored as a form: an
S-expression such as `(param name Type)`{.brix}. Brix is shown as rows, with indentation, dividers
and operators, so the same parameter is shown as `- name Type`{.rows}. This reference writes code
as forms. The website shows the forms as rows, with a toggle to show the forms themselves. Each
example is a whole program, and when it prints, its output follows it.

The core library's types, such as `String`, `List` and `Map`, are part of the language and are
covered here. The rest of the standard library is surveyed in its own chapter.

## Chapters

1. [Source text](source-text.md): files, rows, indentation, dividers, names and comments, and how
   rows and forms relate.
2. [Literals](literals.md): numbers, text, characters, lists, maps and ranges.
3. [Modules](modules.md): imports, exports, values that belong to a file, and the code a file runs.
4. [Functions](functions.md): declarations, value bodies and step bodies, lazy bindings,
   parameters, results, calls, function values and generic functions.
5. [Types](types.md): classes, tuples, structs, properties, constructors, methods, generic
   types, inheritance, interfaces, delegation and extensions.
6. [Optionals](optionals.md): absence, `null`, fallbacks, chains and narrowing.
7. [Unions and enums](unions.md): values that are one of several cases, and named values of one
   type.
8. [Conversions](conversions.md): what converts on its own, `as`, conversions a type declares,
   and joins.
9. [Operators](operators.md): every operator, its precedence, and how a type defines its own.
10. [Control flow](control-flow.md): locals and blocks, conditions, decision tables and loops.
11. [Patterns](patterns.md): `match`, every kind of pattern, and taking values apart.
12. [Text](text.md): strings, characters, building text from values, text bodies and atoms.
13. [Collections](collections.md): lists, maps, arrays, views, tables, bitmaps and ranges as
    values.
14. [Streams](streams.md): the stream family, generators, comprehensions, pipelines and targets.
15. [Stream patterns](stream-patterns.md): grammars over text, bytes and other sequences.
16. [Errors](errors.md): raising, propagation, `catch`, handlers, cleanup and panics.
17. [Concurrency](concurrency.md): tasks, workers, channels and subscriptions.
18. [Reactivity](reactivity.md): tracked reads, derived properties, rules, live statements and
    events.
19. [State machines](state-machines.md): cases, transitions, payloads and lifetimes.
20. [Units](units.md): unit literals, unit families and conversions between them.
21. [Markup](markup.md): elements, attributes, children, slots, and markup that follows state.
22. [Styles](styles.md): styles, style sheets, rules, states, themes and motion.
23. [Probes](probes.md): recording what a program does, and reading what it recorded.
24. [Native code and memory](native-code-and-memory.md): C calls, C types, and memory outside the
    collector.
25. [Built-in types](built-in-types.md): the types every module can use without an import.
26. [The standard library](standard-library.md): what each library holds and how to import it.
27. [Platforms](platforms.md): the platforms programs build for, and the native libraries that limit
    some modules to some of them.

Two indexes close the reference, each pointing to the entry that explains what it lists:

- [Symbol index](symbol-index.md): operators, the marks that begin atoms and rows, names with a
  fixed meaning, and the functions core gives every module.
- [Form index](form-index.md): every Brix form, by its head, with its shape.
