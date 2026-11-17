# The Compiler

How the compiler in `stdlib/lang` is built, and the rules its design keeps. The program is
`apps/compiler/brix-cli.yumi`; `bin/brix` builds it into `.tmp/build/tools/brix` from the seed
that `.release.json` pins (`toolchain/compiler.py`).

| Directory | Holds |
|---|---|
| `stdlib/lang/ast` | parser, AST tables, declarations, modules, the program and its name index |
| `stdlib/lang/check` | binding, the type store and relations, typing, facts, diagnostics |
| `stdlib/lang/report` | the text of facts, diagnostics and declarations, for reports and inspection |
| `stdlib/lang/emit` | program tables, the emitter, the inliner, IR to MIR, the emission round, linking |
| `stdlib/lang/ir` | the IR the emitter writes, its text and its digest |
| `stdlib/lang/backend` | MIR passes, register allocation, AArch64 encoding, Mach-O |
| `stdlib/lang/product` | the product interfaces an app declares, and the app bundles, signing and device installs made from them |
| `stdlib/lang/lint` | the linter, whose semantic rules read facts |
| `stdlib/lang/analysis` | the analysis service behind `brix --serve` |
| `compiler` | the command line, the build daemon, and the test, lint, probe and bench tools |

## Pipeline

1. **Parse.** `SyntaxProgram` parses modules on workers, a large file in pieces between its
   dividers (`piece-parse.yumi`). `ProgramBuilder` makes each module's `AST` and declaration
   rows (`ModuleBuilder`) and links them into a `Program`.
2. **Bind.** `Binder` binds each declaration once into `Bindings`, shared by all its instances.
3. **Type.** `type-program` (`parallel-typing.yumi`) types the program on parallel passes
   (`type-in-parallel`). `merge-sessions` takes in what they typed. `check-pass` types what is
   left, then settles class tables, witness tables, panics and stack locals on one thread.
   `Session.seal` then seals the type store and the instance table.
4. **Diagnostics.** `FactsText` renders them; the build stops if there are any.
5. **Program tables.** `ProgramTables.build` numbers everything whole-program on one thread. A
   construct emission has no shape for is a `Refusal`, and refuses the whole program.
6. **Emission round.** `EmissionWorker`s take functions heaviest first (`WorkQueue`). Each
   emits IR, inlines (`IRInliner`), lowers to MIR (`IRToMIR`) and runs the backend's
   `FunctionPipeline`, all in its own arenas. A build daemon's round has two passes so it can
   reuse functions.
7. **Merge.** Literals are named in function order, through prefix sums over each function's
   count. GC descriptors are sorted into one table.
8. **Object and link.** `MachOWriter` writes the object. Then `Program.release-trees` and
   `Session.release-typing` free the trees and the typing state, and `link-executable` links.

| Owned by | Contents | Released |
|---|---|---|
| Module trees | syntax rows, name index | `Program.release-trees`, after emission |
| Session | type store, instances, signatures | with the session |
| Typing state | facts, bindings, scratch | `Session.release-typing`, after emission |
| Program tables | layouts, functions, class and witness tables, storage | after the object is written |
| Function arenas | IR, then MIR | reset when a worker takes its next function |

## Architecture Rules

**Passes**
1. The pass order is parse, declare, link, bind, type, program tables, emit, IR to MIR, machine
   code. Binding runs once per declaration. Typing and emission run per instance.
2. One walker per pass enumerates the language: `Binder`, `Checker`, `Emitter`. Lowering is
   never spread across type, operator and builder objects. The checker and the emitter are each
   a file family: a core module (`checker.yumi`, `emitter.yumi`) and one module per feature, such
   as `emitter.loops.yumi`, each a facet of the walker that `sees` the sibling features it calls.
   A helper or state two features use stays in the core.
3. There are two lowering modes, statement and expression. A subsystem (markup, styles, state
   machines, stream patterns, reactivity) desugars, uses typed intrinsics, or uses a
   whole-construct emitter shape. It never adds a third mode.
4. Dependencies point one way. Modules in `emit` import no checker, dispatch, relations or
   binder. The backend imports nothing from `/lang/ast`, `/lang/check`, `/lang/ir` or
   `/lang/emit`. This holds today, but no tool checks it.

**Decisions before emission**
5. Each pass asks only its own questions. Binding never reads types. Typing never resolves
   names. Emission never resolves, dispatches, infers, compares types or reports a
   diagnostic. When emission lacks something, fix the type pass or program tables.
6. Emission reads syntax plus facts. No second representation of a body exists.
7. Program tables seal everything whole-program before emission: class-table and witness slots,
   layouts, GC descriptors, symbols, task-local and thread-local layouts, tracked properties,
   event ids and style indexes. The type pass schedules every instance those tables need, so
   interning an instance after the seal panics. A `@stack` global that no `do with` in the
   program binds, and whose value is a literal, is a `global-constant` there, read with no slot
   and no check.
8. Emitted IR is never reopened. Result, panic and cleanup shapes are chosen at entry from the
   signature, the facts, the body scans and program tables. Inlining reads finished IR and
   writes a new function.
9. Generated declarations are rows. If dispatch can choose it, it has a `DeclId`.
10. An instance key carries every dimension that changes code: declaration, type arguments,
    effect mask, markup property fills and slot sets, style context, stream-helper ABI variant.

**Identity and data**
11. Every type roots in a declaration. What the compiler knows about a core declaration is an
    `IntrinsicKind` tag, bound once by `bind-types`. Generic code names no stdlib class.
12. Identity is interned once and never mutated. Later knowledge lives in side tables keyed by
    id, and facts are written once.
13. No strings as keys. Keys are U32 word sequences through `IdMap`.
14. High-volume data is `Table` rows with U32 ids in arenas, released at phase boundaries.
15. Syntax ids are post-order: a child's id is below its parent's, except the late-linked `next`
    fields of if, for and loop blocks. A body is one contiguous range
    (`post-order-violation` checks this).

**Demand, errors, determinism**
16. Typing is demand-driven, with a state per instance. A cycle is a diagnostic. Panic capability
    (`Panics`) and error sets (`ErrorSets`) are settled over the call graph after the bodies.
17. Type relations never instantiate anything, so ranking overloads cannot recurse into typing
    a body.
18. Diagnostics are data. An instance with a diagnostic is never emitted.
19. Build mutable, seal, then share read-only. IR rows copy their locations (line and column
    inline), so nothing after emission reads a tree.
20. Work runs on workers wherever it splits: parsing per module and per piece, typing on
    parallel passes, emission, IR to MIR and the backend per function. Program tables and the
    merge are serial today. Parallel work writes rows indexed by stable ids, never into shared
    lists in finishing order.
21. Output is deterministic. The object is the same at any worker count and in any typing order.
    Program tables renumber what typing registered in source or stable-key order
    (`keep-reached-styles`, `declare-projection-remaps`, `number-keys`, `gate-order`,
    `application-order`).
22. Every emitted function records its dependencies (`IRDependency`: callees with their
    may-panic and generator flags).
23. IR is append-only tables. A block is a contiguous range ending in a terminator, and one
    emitter owns block creation through a closed set of shapes.
24. The runtime contracts are exact: object and table layouts; x27 as the task context; a function
    returning a panic calls `brixPanicReturn`, which sends the return into the caller's landing pad
    found in `__brix_pads`, so a call that may panic is not followed by a test; a place that raises
    a panic calls the `RaiseMessage` or `RaisePanic` function the compiler emits, which sets it
    through `brixRaisePanic`, whose stack trace starts at that place; the collector's control word
    carries the termination bit beside the handshake bit, so a poll reads one byte; GC maps, roots
    and write barriers; the calling convention; the `__brix_gcmap`, `__brix_gcroots`, `__brix_trace`
    and `__brix_pads` sections.
25. Facts are the tooling contract. The linter's semantic rules and the analysis service read
    them in process; nothing is emitted for tooling.

## Layer Invariants

**Binding**
- The `with` and `without` sections of a block or a header never reach binding. Both readers
  lower them (`section-annotations` and `stack-bound` in `ast/_grammar-helpers.yumi`): a
  `without` name becomes the annotation it stands for, and each binding an
  `ASTStackLocalBindingBlock`.
- Binding reads syntax and declaration rows, never types. No type id appears in a binding table.
- Resolution order is fixed: slots innermost first, then the implicit names, members of the
  owner chain, module globals, then functions and types through the name index, then
  unresolved. A local shadows a member, a member a global, a global a function.
- The identifier map is total over a body. Type identifiers are another node kind, evaluated by
  the type store.
- A lambda's identifiers live in its owner's map, but its body scans are its own. A yield inside
  a lambda makes the lambda a generator, not its owner.
- An unresolved name binds to the unresolved kind with a diagnostic row, and the walk goes on.
  Binding records use summaries and captures; it never decides how a slot is stored.

**Type store and relations**
- Every type row points at a declaration. Anonymous tuples, unions, functions, closures and
  results root at core's `Tuple`, `Union`, `Function` and `Closure`.
- Representation is derived by walking the base chain to a tagged declaration.
- Completion fills a type's pools lazily (`Types.complete`), gathering into scratch and
  committing one contiguous range per pool. It is cycle-safe: entering a type in progress is a
  diagnostic. Accessors such as `completion-of`, `properties-of` and `cases-of` complete first.
- A relation is a `Relation` record (rank, match kind, unified type, payload), never a Bool.
- Units are ordinary classes whose base chain reaches a numeric root; ratios are equalities.

**Typing**
- Every identifier reads its `Resolution` from the bindings. Every question about a declaration
  is a column read, and every question about a type a fact-pool read.
- A demand on an instance in progress is a cycle and is diagnosed at the demanding site.
- Facts are written once per instance. A join writes the join type on the join node and a
  conversion fact on each arm; it never rewrites an arm.
- The invalid type is absorbing: an expression with an invalid operand is invalid without a
  second diagnostic, and a call with an invalid argument is not dispatched.
- Instances are interned once. A return type found late is written into the signature, never
  into the key. A lambda instance is keyed by its closure row plus its enclosing instance.

**Facets**
- The loader finds file families (`family-base` in `module-resolver.yumi`), and refuses an import
  of a family's module from outside the family (`syntax-program.yumi`).
- A facet is an `ExtensionDecl` with a `facet` name. `ProgramBuilder` binds facets into
  `program.facets` (`FacetTable`, `ast/facets.yumi`) before it indexes extensions: each facet's
  parent, owner class and state, the siblings its `sees` names, and each class's facets in the
  order they are laid out, by the name of the declaring module and then by declaration.
- A facet joins the extension index under its owner class. `Program.extension-after` shows it
  wherever the class is, except that a module other than the class's own that declares facets of
  the class sees only those it reaches. `Program.reaches-private` gives such a module the private
  declarations of the class's module.
- `ProgramBuilder.note-families` records each module's file family in `Program.families`.
  `Program.reaches-access` says whether code reaches a declaration of an access: a public one, a
  shared one (`^`, `.family-access`) of its own family (`same-family`), and a private one as
  `reaches-private` says. The binder, `hiding-access` (behind `is-hidden`), `visible-from`,
  constructor gathering, the type store and the report ask it. The checker refuses `^` on a
  module's own declarations (`check-shared-access`).
- `complete-class` lays out the facets' stored and lazy properties after the class's own
  (`gather-facets`). Each `TypeProperty` keeps its declaring facet, so a name two facets keep
  private is found by the module that reads it (`property-seen`), and a lazy slot by its name and
  facet (`property-declared`). Facet getters and setters stay out of the roster and dispatch as an
  extension's do.
- The checker reports a facet's binding, shape and name errors (`check-facet`), and a derived
  class's names that a facet of a class above declares (`check-below-facets`), but for a method,
  getter or setter of the same kind as the facet's, which overrides it (`overrides-facet`).
- A facet's methods, getters and setters are class-table methods (`is-table-method`). Dispatch
  ranks a facet member as an extension's, at its owner's level, and a member a derived class
  declares again replaces it rather than conflicting with it (`is-facet-member`), so
  `redispatch` records the override and `super` reaches the facet's.
- A facet's events are its class's: `declared-event` finds an event in a class's body or in a
  facet the module sees, for `event-of`, `hides-event` and `declares-event`, and its payload and
  key types are read in the declaring module (`declared-payload`). Program tables key events by
  class and name, so an event's name excludes every facet's members at any access
  (`check-facet-names`), and `has-events` counts facets' events for `close`.
- Program tables give each facet property the value its facet writes (`facet-default`), after the
  class's own, and find the class that writes a facet's lazy value through its owner
  (`writer-of`).
- A class's styles and style sheets come from its own `using` and then its facets' in their order:
  `sheet-parts` lists a class's bound facets after it, `part-members` reads a facet's members and
  `holder-env` evaluates a facet's `using` under its owner's types, so the walks that gather a
  class's styles (`find-visible-styles`, `chain-sheets`) take the facets' as the class's own.
- A facet may write rules (`RuleDecl`, `add-rule`): sections, `when` and `whenever` rules and
  dependencies. The checker's walks of a class's rules (`demand-rules`, `plan-event-handlers`,
  `writes-sections`, `lowers-for`, `find-query-targets`) take `sheet-parts`, and program tables'
  (`declare-class-rules`, `declare-markup-rules`, `reach-rules`) take `class-parts`, so each class's
  facets' rules follow its own, typed for the class as its own are.
- A class below a facet's class that gives a facet's stored property a new value keeps the facet's
  slot and becomes the property's owner (`add-property`). `is-given-below` then sends
  `declaring-decl`, `declaring-holder` and `default-of` to that class, so its value is typed and
  stored as a base's property's is. A constructor's argument fills a facet's property by name
  (`fill-inherited`), but for a private one, whose name two facets may share.
- A facet's name is a type: `nearest-class` finds it where the caller sees the facet, as a
  `.type-class` row of the facet's declaration, which `Types.facet-of-type` recognises.
  `complete-facet` completes it as a class with no base whose property range is its owner's own
  rows, so layouts, lazy slots and memo places are the owner's. `facet-owner` gives the owner's
  type, and once the store is sealed only one made before.
- Relations (`relate-facets`, ahead of the base-chain checks in `relate`): a class at or below the
  owner and a facet within another narrow by an up-cast; a facet widens to its owner or above by an
  up-cast marked `explicit`, which `implicit` refuses, and casts to a class below its owner by a
  down-cast. All are the owner's pointer, so emission does nothing new.
- No instance has a facet's type as its own. `property-seen` finds a facet value's properties
  among its facet's and its ancestors' that the module sees (`property-of-facet`), `rank-methods`
  ranks only those members, on the owner (`rank-facet-members`), and the receiver's conversion to
  the owner is allowed (`lowers-facet`). `demand-member` types a facet value's properties for the
  owner. `refuses-facet-test` refuses `is` and an arm naming a facet, and
  `rejects-facet-default` a stored property of a facet's type with no value.
- `Program.entry-rank` and `facet-listed-last` give a facet's name the rank and place of its
  owner's module, so importing the owner brings it.
- A facet's `is` lines are its `ExtensionDecl.interfaces`. `complete-class` adds them to its
  owner's interfaces after the owner's own (`gather-facet-interfaces`), read in the facet's module,
  and `check-facet-interfaces` reports one that names nothing or no interface. Dispatch lets a
  facet member answer an interface's function without the extension conflict (`accept`,
  `is-facet-member`), so the witness for the owner is the facet's member.

## Vocabulary

| Term | Meaning |
|---|---|
| Syntax id | `SyntaxId`, a row of a module's AST node table; zero is the no-op node |
| Decl id | `DeclId`, a row of a module's declaration table; zero is no declaration |
| Module id | `ModuleId`, a module's index in the program |
| Type id | `TypeId`, an interned row of `Types`; equal ids are equal types |
| Instance | `Instance`, a declaration bound to type arguments under an effect mask: the unit of typing and emission |
| Session | `Session`, one compile's typing state: program, bindings, binder, types, relations, scratch, instances, facts, error sets, panics |
| Role | One object a session holds. A leaf algorithm takes the narrowest role it needs, not the session |
| Facts | `Facts`, per-instance product tables: node types, calls, conversions, pairs, forces, diagnostics |
| Completion | `TypeCompletion`, a type's lazily filled row, with `TypeProperty`, `TypeCase`, `TypeAssociated` and `TypeEquality` pools |
| Scratch, span | A `core/scratch` stack (`TypingScratch` keeps one per kind), and a read-only slice over a finished run on it |
| Row range | `RowRange`, a start and count over one table |
| Mask | A bit set over one flag enum: `SlotMask`, `CaptureMask`, `LazyUseMask`, `ResolutionMask`, `EffectMask` |
| Argument shape, call argument | `ArgumentShape` is a declared argument without types; `CallArgument` a supplied one. `Viability` lays one over the other |
| Candidate, hit | Name lookup yields candidates; ranking with types yields a `Hit`, or a partial hit or an ambiguity |
| Narrowing key | `NarrowingKey`, what a narrowing is recorded against, made by `slot-key`, `member-key` and `path-key-at` |
| Schedule, demand | `schedule` interns an instance and queues it; `demand` types something now, re-entrantly |
| Emitter shape | A control-flow template the emitter owns: branch, loop, switch, result check, panic check, cleanup |

## Naming Conventions

- A store is a plural noun, its row the singular, and the row id the singular plus `Id`:
  `Instances` holds `Instance` rows addressed by `InstanceId`. No `Row` or `Store` suffixes.
- Suffixes say what a name is: `Kind` an enum, `Id` a row id, `Mask` a bit set, `Fact` a
  per-instance product row, `Hit` a ranked dispatch result.
- A bare `U32` or `Size` field is a count or a depth. Every id, index, mask, rank, mark and range
  is an alias, so a signature says what a number means.
- Sentinels follow the id family: an id over a table with a reserved zero row is zero when
  absent (`DeclId`, `SyntaxId`, `ScopeId`); any other id has a `no-` constant
  (`no-type`, `no-instance`, `no-slot`, `no-binding`).
- Verbs have fixed meanings: `intern` maps a key to an id idempotently; `schedule` queues;
  `demand` types now; `complete` fills a completion; `record` appends a product row; `note`
  changes walker state only; `mark` sets a flag on an existing row.
- Walkers are agent nouns (`Checker`, `Binder`, `Ranking`). An algorithm that belongs to a walker
  lives with it.

## What The Design Rejects

These were the TypeScript compiler's structural costs, and each rule above answers one:
- typing and lowering in one walk, so a type decision could depend on what was already emitted,
  and return types were found late and patched in by whole-program fixpoints;
- finished IR rewritten for generators, environments, results and late table offsets;
- every diagnostic an exception, which needed rollback machinery;
- tooling fed by side-channel trace arrays and dead functions lowered only to fill them;
- eight lowering modes per node kind, spread over AST, type and operator classes.

## Why Earlier Attempts Failed

Two attempts to replace the TypeScript compiler failed before the port that worked:
- An in-place refactor kept old paths as fallbacks. Keeping one stayed green and removing one went
  red, so passing tests proved nothing about the new code, and progress was counted in additions.
- A first port judged success by IR text identical to TypeScript's, which forced it to copy
  TypeScript's accidents.
- Ledgers written by the implementing agent claimed "done, oracle-verified" while every oracle had
  drifted and 44% of core cases did not type clean.

What worked: progress was claimed only from tools the implementing agent did not control (whole
spec cases compiled and run), each claim checked structure as well as behavior, and old code was
deleted rather than kept beside the new.

## Language Decisions Without A Spec Case

The port's decisions log recorded each language decision the owner made. Every one has a case in
`stdlib/core/tests` except these:
- **core-type-wins** (2026-10-03): core's type wins over a type of the same name in an imported
  module, with no diagnostic.
- **core-capture-closure** (2026-10-03): core's `Capture` and `Closure` are types a program may
  name (`. x Capture? = null`).
- **source-order-numbering** (2026-10-13): functions the compiler makes under one name in a module
  are numbered top to bottom, as a stack trace shows. Traces show it only in passing.
- **c-type-map** (2026-10-20): only `unsigned long long` as `U64` is pinned (`runCCalls`);
  `unsigned char` as `U8` and `signed char` as `I8` are not.
- **keep-transitive** (2026-10-22): a C header import exposes every declaration of its
  translation unit, including those of the headers it includes.
- **uid-order** (2026-10-23): an object's `uid` is unique, but the order uids are given in is not
  defined, and no code may depend on it.
