# Stream pattern matching

Stream patterns recognize **sequences of items**. This differs from a structure pattern that
inspects the fields or case of one value. A stream pattern can consume a word, a line, a
packet, or a sequence of typed events and produce a value describing what it recognized.

The pattern machinery works with the stream's input type. Character patterns are convenient
for text, but streams of bytes and objects are also meaningful inputs.

## At a glance

| Capability | What it gives you |
| --- | --- |
| Sequence recognition | Match successive characters, bytes, or objects with `~`. |
| Captures and transformations | Turn recognized input into text, tuples, or domain values. |
| Named rules | Build reusable grammars from pattern functions and local rules. |
| Whole input / record stream | Validate a complete input or recognize successive records. |
| Stored / arriving input | Use one grammar; reads can wait without a separate async parser. |
| Cost | Lookahead and backtracking may buffer input; effects do not automatically roll back. |

## Matching text and extracting a result

:::example-pair

- [Sequence pattern](#source-record--prefix): `~` begins recognition with a literal prefix.
- [Capture](#source-record--capture): Bind one or more lowercase characters as `value`.
- [Boundary and result](#source-record--terminator): Require a newline, then return the captured name. This prints `ada`.

```yumi #record
. name = match "name:ada\n"
    ⟦prefix⟧~ "name:"⟦/prefix⟧ ⟦capture⟧value [a-z]+⟦/capture⟧ ⟦terminator⟧"\n"
        value⟦/terminator⟧
    ?? "invalid"
-- name
```

:::

The transformation can construct a tuple, a domain object, or another value; it need not
return the original text. This is how recognition and parsing fit into ordinary expressions.

## Pattern vocabulary

The following are pattern fragments, rather than standalone Yumi programs:

| Pattern | Meaning |
| --- | --- |
| `"name:"` | Match literal text |
| `[a-z]` | Match a character in the indicated range |
| `.` | Match one input item |
| `a b` | Match `a` followed by `b` |
| `a \| b` | Try alternatives in order |
| `(pattern)` | Group a pattern |
| `pattern?` | Match an optional occurrence |
| `pattern*` | Match zero or more occurrences |
| `pattern+` | Match one or more occurrences |
| `pattern{3}` | Match a specified number of occurrences |
| `name pattern` | Capture the pattern's result |
| `\word` | Invoke a named rule |
| `&pattern` | Positive lookahead: require a match without consuming it |
| `!pattern` | Negative lookahead: require a failure without consuming input |

Choice is ordered. Place the alternatives deliberately, especially when one can match a
prefix of another. Lookahead lets a rule check context before committing its consuming part.
A repetition must be able to make progress; a rule that succeeds without consuming input is
not a useful body for an unbounded repetition.

## Named rules and structured results

A pattern function declares its input item type with `~ Char`. Another pattern calls it
with a backslash:

:::example-pair

- [Invoke a rule](#source-rules--invoke): A backslash calls a named pattern.
- [Pattern function](#source-rules--signature): Declare character input and a string result; no `do` is used.
- [Local rule](#source-rules--local): Name a reusable piece of the grammar. The outer rule exposes the name and prints `ada`.

```yumi #rules
-- match "name:ada\n"
    ~ ⟦invoke⟧\name-record⟦/invoke⟧
    ?? "invalid"

━━━━━━━━━━━━━━━━━━━━
⟦signature⟧+ name-record ~ Char String⟦/signature⟧

~ "name:" name \word "\n"
    name

⟦local⟧.. word String ~
    ~ [a-z]+⟦/local⟧
```

:::

Rules can take arguments, refer to other rules, and recurse. A grammar is therefore written
as Yumi declarations, with ordinary values available to its transformations. Pattern functions
do not use `do` on their declarations.

Optional and repeated captures carry the corresponding absence or repetition in their
result shape. Repetition over a character pattern can capture text; repetition over a rule
that produces a domain value can collect those transformed values. Do not assume that every
capture is a string just because the input is text.

An action reads a capture over text, or `it`, as a `String` made from the characters it matched.
To convert those characters without making that text, as a number parser does, give the name to a
function whose parameter is a `CaptureSpan'Char`. When an action only passes a name to such
parameters, or reads its `length`, the function receives the span of the matched characters. Where
a function takes either, the span is chosen. Any other read of the name makes it text throughout
that action.

## Failure is a choice at the boundary

A failed match can produce absence, select a `??` fallback, or return an explicit error.
This parser treats malformed input as a recoverable `InvalidRecord`:

```yumi #parse-error
-- parse-name("name:ada\n")
-- catch parse-name("invalid\n")

━━━━━━━━━━━━━━━━━━━━
+ InvalidRecord is Error

━━━━━━━━━━━━━━━━━━━━
+ parse-name String ?! InvalidRecord
    ⟦input⟧- source <Char>⟦/input⟧

match source
    ⟦success⟧~ "name:" name [a-z]+ "\n"
        name⟦/success⟧
    ⟦failure⟧?! InvalidRecord("Expected a name record")⟦/failure⟧
```

- [Source family](#source-parse-error--input): Accept any supported character stream.
- [Successful recognition](#source-parse-error--success): Produce the name.
- [Boundary policy](#source-parse-error--failure): Treat malformed input as a recoverable error. The calls print `ada` and `Expected a name record`.

## Whole input versus successive records

A root stream match checks the whole input. For example, matching `"foo."` against just
`"foo"` does not accept the trailing period. This is useful for validating a complete
document, but a connection that stays open after a record has not reached end of input.

For successive records, a pattern comprehension can recognize one record at a time:

:::example-pair

- [Successive records](#source-records--iteration): Recognize a record at a time and produce a stream of results.
- [Consume results](#source-records--consumer): Join the names into `ada, bob`.

```yumi #records
. source = "name:ada\nname:bob\n"
-- ⟦consumer⟧", ".join(⟦iteration⟧~ \name-record in source⟦/iteration⟧)⟦/consumer⟧

━━━━━━━━━━━━━━━━━━━━
+ name-record ~ Char String

~ "name:" name [a-z]+ "\n"
    name
```

:::

This prints `ada, bob`. The result is itself a stream and can feed a pipeline or another
consumer. A pipe can run every record under one helper, evaluated once as the pipe starts to
read: `source ~> \name-record with helper`. A helper that implements `PatternRecordBoundary` is
asked to `begin-pattern-record` before each record is matched, once the record before has been
delivered, so it can reclaim scratch storage between records. Pattern iteration is not a promise
to skip arbitrary invalid text: decide how unrecognized input should terminate or report failure,
and express that policy in the parser.

## The same parser with arriving input

The function below accepts `<Char>`. It first receives a stored string, then a channel
whose characters are sent by another task:

```yumi #arriving-input
-- parse-name("name:ada\n")

. incoming Char<>
do new
    for char in "name:bob\n"
        ⟦send⟧incoming <- char⟦/send⟧
    ⟦close⟧incoming.close()⟦/close⟧

⟦call⟧-- parse-name(incoming)⟦/call⟧

━━━━━━━━━━━━━━━━━━━━
+ InvalidRecord is Error

━━━━━━━━━━━━━━━━━━━━
+ parse-name String ?! InvalidRecord
    - source <Char>

match source
    ~ "name:" name [a-z]+ "\n"
        name
    ?! InvalidRecord("Expected a name record")
```

- [Arriving characters](#source-arriving-input--send): Another task supplies input one character at a time.
- [Whole-input boundary](#source-arriving-input--close): Close the channel so the root match can establish end of input.
- [Same parser](#source-arriving-input--call): The grammar, result type, and call spelling stay the same. The calls print `ada` and `bob`.

Networking uses the same principle. Given `/net/socket` and a peer that sends a sequence of
these records, the following fragment adapts its character input and consumes parsed names:

:::example-pair

- [Owned lifetime](#source-connection--cleanup): Register closure for the socket acquired here.
- [Readable characters](#source-connection--adapter): Adapt transport input to the grammar's stream model.
- [Record boundary](#source-connection--records): Process names without requiring the peer to close after each record.

```yumi #connection
. socket = TCP'Char(host, port)
⟦cleanup⟧do finally
    socket.close()⟦/cleanup⟧
. characters = ⟦adapter⟧socket as Iterable'Char⟦/adapter⟧
⟦records⟧for name in ~ \name-record in characters⟦/records⟧
    -- name
```

:::

`host`, `port`, and `name-record` are supplied by the surrounding program. The fragment has
no `async` declaration or `await`, and it does not require the peer to close after every
record. Connection setup, transport errors, and the lifetime of the socket still need the
application's policy.

## Matching non-text items

A pattern can recognize a sequence of typed objects too:

```yumi #typed-sequence
. events Event<>
do new
    events <- Click()
    events <- Click()
    events <- Key()
    events.close()

-- match events
    ⟦pattern⟧~ clicks Click* key Key⟦/pattern⟧
        ⟦result⟧clicks.length⟦/result⟧
    ?? 0

━━━━━━━━━━━━━━━━━━━━
+ Event

━━━━━━━━━━━━━━━━━━━━
+ Click is Event

━━━━━━━━━━━━━━━━━━━━
+ Key is Event
```

- [Object pattern](#source-typed-sequence--pattern): Recognize zero or more clicks followed by a key event.
- [Transformed result](#source-typed-sequence--result): Count the captured clicks. The example prints `2`.

Byte-oriented patterns can also decode typed fields and use a preceding length to control
a later repetition. This is useful for packet formats, but byte order, decoding, and framing
belong in the format's explicit contract.

## Buffering, lookahead, and backtracking

The matcher needs a logical cursor that it can restore while trying alternatives. An
already-stored source and a source that must be read from a socket provide that ability in
different ways. Token-stream adapters can retain input so a failed alternative can rewind
without asking an external producer to send the data again.

| Mechanism | Consequence |
| --- | --- |
| Backtracking | Revisit buffered input; do not reverse network IO |
| Long speculation | Potentially retain substantial input |
| Record pipeline (`~>`) | Release input once its record is delivered; retain only the unfinished record |
| Input that arrives in pieces | Continue from the item where the input ran out; never match the record again from its start, so each action runs once |
| Iterable or generator source | Pull one item at a time, so a complete record is delivered without waiting for more |
| Whole-input match | Wait for completion even after recognizing a plausible prefix |
| Lookahead | Possibly request more input to establish the condition |

Avoid treating arbitrary side effects inside speculative parsing as automatically reversible.
Yumi has explicit facilities for helper state and rollback, but cursor rewind alone is not
a transaction over the application. Prefer constructing a result and applying effects after
accepting it. Detailed custom materialization and rollback protocols warrant a separate
grammar reference.

## Where this fits

Stream patterns describe recognition over successive input items. [Structural patterns](structural-pattern-matching.md) inspect the shape of an existing value; [stream pipelines](stream-pipelines.md) describe how to produce or transform a sequence. These features compose: a grammar can yield records that a pipeline filters and a structural match classifies. See [Streams](streams.md) for the source types that make the same parser useful over stored and arriving input.
