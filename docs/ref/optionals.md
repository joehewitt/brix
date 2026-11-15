# Optionals

A value that may be absent has an optional type. `(? Int)`{.brix} holds an `Int` or nothing, and
`null` is the nothing. A value of an ordinary type always holds a value, so a program says where
absence is possible by writing `?`, and the compiler checks that absence is dealt with before the
value is used.

This chapter covers optional types, how values become optional, the ways to use one, optionals as
conditions, narrowing, and the steps that skip a null. Optionals in patterns are in Patterns, and
errors as values in Errors.

## Optional types

```brix
(main
  (let names (list "ana" "bo"))
  (let found (call find names "bo"))
  (let missing (call find names "cy"))
  (print found)
  (print missing)
  (print (?? missing -1))
  (print (!! found))
  (if found
    (print (str "found at " found))
  )
  (if (not missing)
    (print "cy is missing")
  )
)

(fn-do + find (? Int)
  (param items (list String))
  (param target String)
  (let index 0)
  (for (in item items)
    (return-if index (if (== item target)))
    (+= index 1)
  )
  (return null)
)
```

```output
1
unknown
-1
1
found at 1
cy is missing
```

`(? T)`{.brix} is the optional type of `T`. Any type can be made optional: numbers, `Bool`, text,
classes, interfaces, tuples, structs, unions, enums, lists, function types and type parameters. An
optional of an optional, `T??`{.rows}, cannot be written.

- **`null`** is the absent value. It converts to every optional type.
- **Text.** An absent value becomes the text `unknown`, alone, in interpolation, and as a field of
  a tuple.
- **No value.** A local, property or global of an optional type declared without a value starts as
  `null`.
- **Its type.** `type-of` an optional `Int` gives `[TypeInfo Optional'I64]`.
  `(call (of Optional Int) 4)`{.brix} makes one that holds `4`, and
  `(call (of Optional Int))`{.brix} one that holds nothing.

## Making an optional

A value converts on its own where an optional of its type is wanted: in a declaration, as an
argument, as a returned value, or as an item of a list. `T` and `(? T)`{.brix} share their values,
so nothing is copied or boxed.

```brix
(main
  (let size 4)
  (print (only-if size (if (> size 3))))
  (print (only-if size (if (> size 9))))
  (let picked (if-expr (> size 3) "big" null))
  (print picked)
  (print (type-of picked))
)
```

```output
4
unknown
big
[TypeInfo Optional'String]
```

- **With a condition.** `(only-if value (if condition))`{.brix} gives the value when the condition
  holds, and `null` when it does not.
- **Joins.** An `if` with `null` on one branch and a value on the other, such as
  `(if-expr (> size 3) "big" null)`{.brix}, gives an optional of the value's type. So does a `match`
  with an arm that gives `null`, and an `if` with an optional on either branch.
- **Results.** A function whose result type is inferred, and which returns a value on some paths
  and `null` or nothing on others, gives an optional. A function declared with an optional result
  type that ends without returning gives `null`. `match?`, `catch?` and a decision table with no
  `(default value)`{.brix} arm give optionals too.

### Optional parameters

```brix
(main
  (print (call greet))
  (print (call greet "bo"))
  (let nobody (? String) null)
  (print (call welcome nobody))
  (print (call welcome "cy"))
)

(fn-expr + greet String
  (param name (? String) (= null))
  (str "hi " (?? name "you"))
)

(fn-expr + welcome String
  (param name String (= "friend"))
  (str "welcome, " name)
)
```

```output
hi you
hi bo
welcome, friend
welcome, cy
```

- **Optional is not left out.** A parameter declared `(param name (? String))`{.brix} may be given
  `null`, but must be given: `No function "greet" accepts (); it is declared as "greet(name
  String?)"; "name" may be null but has no default, so a call must give it: pass null, or declare
  "name String? = null"`. Declared `(param name (? String) (= null))`{.brix}, it may be left out.
- **Defaults.** An optional given for a parameter that is not optional but has a default gives
  its value when it holds one, and the default when it holds nothing, as
  `(call welcome nobody)`{.brix} does. For a parameter without a default, an optional is an error:
  `No function "strict" accepts (I32?); it is declared as "strict(a I32)"`.

## Using an optional

An optional cannot be used as its value until the compiler knows it holds one. These forms get
the value out, or carry the absence along.

| Code | Value |
| --- | --- |
| `(!! value)`{.brix} | The value. The program panics when it is `null`. |
| `(?? value fallback)`{.brix} | The value, or the fallback when it is `null`. |
| `(?. object name)`{.brix} | The member, or `null` when the object is `null`. |
| `(?[] list index)`{.brix} | The item, or `null` when the collection is `null`. |
| `(as value Type)`{.brix} | The value, converted to the type. The program panics when it is `null`. |

```brix
(main
  (let user (? User) (call User "ana" null))
  (print (?. user name))
  (print (?. (?. user manager) name))
  (print (. (?. user tags) length))
  (let nobody (? User) null)
  (print (?. nobody name))
  (print (?? (?. nobody name) "anonymous"))
  (print (?[] (?. user tags) 0))
  (= (?. nobody name) "cy")
  (print (?? (call (?. nobody greet)) "silent"))
  (print (call (?. user greet)))
)

(type + User
  (prop + name String)
  (prop + manager (? User))
  (prop + tags (list String) (= (list "admin")))
  (fn-do + This _
    (param name String)
    (param manager (? User))
  )
  (fn-expr + greet String
    (str "hi " name)
  )
)
```

```output
ana
unknown
1
unknown
anonymous
admin
silent
hi ana
```

### Forcing

`(!! value)`{.brix} gives the value of an optional, and panics when it holds nothing:
`Optional'core/number:I64 is null`. The panic is not an error a function raises; see Errors. It
works within a chain, as in `(. (!! (. (!! (call (. holder find) 5)) next)) value)`{.brix}. Forcing
a value the compiler knows is not `null` is an error: `"count" is I64, which is never null here, so
it needs no "!"`.

### Fallbacks

`(?? value fallback)`{.brix} gives the value when it is not `null`, and the fallback otherwise.

- **Only when needed.** The fallback is evaluated only when the value is `null`.
- **Chains.** `(?? a b c)`{.brix} gives the first that is not `null`.
- **Type.** When the fallback is not optional, the result is not optional either. When it is, the
  result is optional.
- **Values that are not optional.** On a value that is not optional, `??` gives the fallback when
  the value is false as a condition: zero, empty text, an empty collection or `false`. So
  `(?? count 1)`{.brix} gives 1 when `count` is 0. See Control flow for what is false as a
  condition.
- **After `if`.** `(?? (only-if value (if condition)) other)`{.brix} is an error, because it reads
  as one value or another: write `(if-expr condition value other)`{.brix}.

### Chains

`(?. object name)`{.brix} reads a member of an object that may be `null`. When the object is
`null`, the rest of the chain is skipped and the whole chain gives `null`, so
`(. (?. user tags) length)`{.brix} gives `null` when `user` is `null`, and reads
`(. user tags length)`{.brix} otherwise.

- **Type.** The result is an optional of the last member's type.
- **Once.** The object is evaluated once, and the arguments of a call in the chain only when the
  object is not `null`.
- **Calls.** `(call (?. object method))`{.brix} calls the method when the object is not `null`. A
  method that gives nothing can be called this way as a step.
- **Assignment.** `(= (?. object name) value)`{.brix} assigns when the object is not `null`, and
  does nothing otherwise. Assigning operators such as `+=` work the same way. The value on the right
  is evaluated either way.
- **A further optional.** A member that is itself optional needs its own `?.`, as in
  `(?. (?. user manager) name)`{.brix}. Reading through it with `.` is an error: `Property "name" is
  read from User?, which may be null; unwrap it first with ! or ?.`.
- **Values that are never null.** `(?. object name)`{.brix} and `(?[] list index)`{.brix} are an
  error when the object or the list cannot be `null`:
  `"user" is User, which is never null, so it needs no "?"; write "user.name"`.

## Optionals as conditions

Where a `Bool` is wanted, an optional is true when it holds a value, so `(if found body)`{.brix}
asks whether `found` holds one, and `(not missing)`{.brix} whether it does not. An optional holding
`0`, or a `(? Bool)`{.brix} holding `false`, holds a value, and is true. To test the value of a
`(? Bool)`{.brix}, compare it, as in `(== flag true)`{.brix}.

```brix
(main
  (let flag (? Bool) false)
  (print (if-expr flag "present" "absent"))
  (print (== flag true))
  (let a (? Int) 3)
  (let b (? Int) null)
  (print (== a 3))
  (print (== b null))
  (print (== a b))
)
```

```output
present
false
true
true
false
```

- **Equality.** Two optionals are equal when both are `null`, or both hold equal values. An
  optional and a value are equal when the optional holds an equal value. `(== value null)`{.brix}
  and `(!= value null)`{.brix} test for absence.
- **Ordering.** An optional cannot be compared with `<` and the other ordering operators:
  `Operator "<" is not defined for I64 and I64?`.

## Narrowing

After a test that shows an optional holds a value, the compiler treats it as its value: it can be
used where the value's type is wanted, and its members read with `.`.

```brix
(main
  (let count (? Int) 3)
  (if count
    (print (+ count 1))
  )
  (print (call double count))
  (print (call double null))
)

(fn-do + double Int
  (param value (? Int))
  (return-if 0 (if (not value)))
  (return (* value 2))
)
```

```output
4
6
0
```

Using an optional as its value without a test is an error. For an operator it is `Operator "+" is
not defined for I64? and I64`, and where a value of the type is wanted, `Expected I64 here, but
this is I64?, which may hold nothing; write "!" after it to unwrap it, or "?? fallback" to give a
value for nothing`.

```brix compile-error
(main
  (let count (? Int) 3)
  (print (+ count 1))
)
```

```output
Exception: Operator "+" is not defined for I64? and I64
<no file:4>
      (print (+ count 1))
             ^^^^^^^^^^^
```

### What narrows

- **Conditions.** `(if value body)`{.brix} and `(if (!= value null) body)`{.brix} narrow `value` in
  the body, and `(if (not value) body (else other))`{.brix} in the `else`. A `while` condition
  narrows its body, a `when` clause on a loop narrows the item, and the arms of a decision table see
  what the arms above them ruled out.
- **Early returns.** A branch that leaves the function, such as
  `(return-if 0 (if (not value)))`{.brix}, narrows the rest of the body. So does a check that
  panics, as in `(throw (call Panic "missing") (if (not value)))`{.brix}.
- **Joined conditions.** In `(and value (> value 3))`{.brix}, the right side sees `value` narrowed,
  and in `(or (not value) (> value 3))`{.brix} too.
- **Values.** In `(if-expr value (+ value 1) 0)`{.brix} and
  `(only-if (+ value 1) (if value))`{.brix}, the value is narrowed.
- **Assignment.** Assigning a value that is not `null` narrows a local until it is assigned
  again. A local declared with no value and assigned on every path before a use is narrowed at
  the use. A declaration with a value, as in `(let count (? Int) 3)`{.brix}, does not narrow.
- **Paths.** A test of a path such as `(. box child)`{.brix}, where `box` is a local, narrows the
  path.

### What ends narrowing

- **Assigning `null`**, on any path, ends it after the paths meet.
- **Loops.** A narrowed local stays narrowed in a loop only when every path back to the loop's
  start keeps it: `Narrowing of 'value' is used in the loop but does not hold when the loop
  repeats`.
- **A function value that assigns the local.** Calling one ends it. Other calls do not.
- **Properties.** A property read by name, or through `this`, is narrowed by the same tests, but
  any call, print or loop ends it, since it could change the property. So do assigning it and
  reading a lazy property. A getter is never narrowed.

```brix compile-error
(main
  (let box (call Box))
  (call (. box show))
)

(type + Box
  (prop + label (? String))
  (fn-do + show _
    (if label
      (call touch)
      (print (. label length))
    )
  )
  (fn-do + touch _)
)
```

```output
Exception: Property "length" is read from String?, which may be null; unwrap it first with ! or ?.
<no file:12>
          (print (. label length))
                 ^^^^^^^^^^^^^^^^
```

Test again after the call, or copy the property to a local before testing it.

Narrowing also applies to unions and to the classes a value may hold; see Unions and enums, and
Types. An `if` that tests `(is value Dog)`{.brix} on a value of a base class does not yet narrow
it.

## Steps that skip null

Several steps take an optional and do nothing when it holds nothing:

| Code | Step |
| --- | --- |
| `(print? value)`{.brix} | Print the value, unless it is `null`. |
| `(=? target value)`{.brix} | Assign the value, unless it is `null`. |
| `(+=? target value)`{.brix} | Add the value, unless it is `null`. The other assigning operators have steps like it, such as `(-=? target value)`{.brix}. |
| `(return? value)`{.brix} | Return the value, unless it is `null`. |
| `(yield? value)`{.brix} | Give the value to the stream, unless it is `null`. |
| `(use? resource)`{.brix} | Use the resource, unless it is `null`. |
| `(bullet? value)`{.brix} | Give the item, unless it is `null`. |

```brix
(main
  (let total 10)
  (let none (? Int) null)
  (let some (? Int) 5)
  (+=? total none)
  (+=? total some)
  (print total)
  (print? none)
  (print? some)
  (let label "start")
  (=? label (as null (? String)))
  (print label)
)
```

```output
15
5
start
```

The value of an assignment that skips null is what the target then holds.
