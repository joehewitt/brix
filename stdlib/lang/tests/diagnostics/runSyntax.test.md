# Syntax

Syntax errors at a known trap say what Brix expects instead

### reserved word as a name

```yumi crash
. on = 1
-- on
```

```output
Exception: "on" is a reserved word and cannot be a name; choose another name
<no file:2>
    . on = 1
         ^
```

### preposition after an expression

```yumi crash
. items = [3, 1]
. sorted = sort of items
```

```output
Exception: "of" cannot follow an expression; a call takes its arguments in parentheses, as in "sort(keys)", and a callback after a bullet, as in "some(items) • where each > 2"
<no file:3>
    . sorted = sort of items
                    ^
```

### where after a loop source

```yumi crash
for x in [3, 1] where x > 2
    -- x
```

```output
Exception: A loop has no "where" clause; filter its items with "when", as in "for x in items when x > 2"
<no file:2>
    for x in [3, 1] where x > 2
                    ^
```

### bullet that ends a header line

```yumi crash
if some([3, 1]) • where
    each > 2
    -- "big"
```

```output
Exception: "if" owns the block below it, so "• where" at the end of its line cannot take that block; finish the argument on this line, or compute it in a local above
<no file:2>
    if some([3, 1]) • where
                           ^
```

### label given twice on one line

```yumi crash
-- some([3, 1]) • where each > 1 • where each > 2
```

```output
Exception: "where" is given twice in one call; the bullets on a line all belong to one call, so give a nested call its bullets inside parentheses, as in "• where (some(each.kids) • where each > 2)"
<no file:2>
    -- some([3, 1]) • where each > 1 • where each > 2
                                    ^^^^^^^^^^^^^^^^
```

### camel case name

```yumi crash
. itemCount = 1
-- itemCount
```

```output
Exception: Names are kebab-case; write "item-count", not "itemCount"
<no file:2>
    . itemCount = 1
          ^
```

### cast followed by an operator

```yumi crash
. x = 3
. n = x as I64 + 1
```

```output
Exception: A cast cannot be followed by an operator; wrap it in parentheses, as in "(x as I64) + 1"
<no file:3>
    . n = x as I64 + 1
                   ^
```

### operator without spaces

```yumi crash
. x = 3
. y = x+1
```

```output
Exception: An operator needs a space on each side; write "x + 1"
<no file:3>
    . y = x+1
           ^
```

### compound operator without spaces

```yumi crash
. x = 3
x+=1
```

```output
Exception: An operator needs a space on each side; write "x += 1"
<no file:3>
    x+=1
     ^
```

### power written as two stars

```yumi crash
. x = 2 ** 3
-- x
```

```output
Exception: Raise to a power with "a %% b"
<no file:2>
    . x = 2 ** 3
             ^
```

### power in place written as two stars

```yumi crash
. x = 2
x **= 3
-- x
```

```output
Exception: Raise in place with "x %%= b"
<no file:3>
    x **= 3
       ^
```

### power written as a caret

```yumi crash
. x = 2 ^ 3
-- x
```

```output
Exception: Raise to a power with "a %% b"; for the bits set in one but not the other, write "a /xor/ b"
<no file:2>
    . x = 2 ^ 3
            ^
```

### list continued across lines

```yumi crash
. xs = [
    1, 2]
```

```output
Exception: An expression ends at the end of its line, so this "[" is never closed; write it on one line, or give a call its arguments on indented "- value" lines below it
<no file:2>
    . xs = [
            ^
```

### call continued across lines

```yumi crash
-- add(
    1, 2)

━━━━━━━━━━━━━━━━━━━━
+ add I32
    - a I32
    - b I32

a + b
```

```output
Exception: An expression ends at the end of its line, so this "(" is never closed; write it on one line, or give a call its arguments on indented "- value" lines below it
<no file:2>
    -- add(
           ^
```

### operator at the end of a line

```yumi crash
. x = 3 +
    4
```

```output
Exception: An expression ends at the end of its line, so nothing follows this "+"; write the whole expression on one line
<no file:2>
    . x = 3 +
             ^
```

### braces

```yumi crash
if true {
    -- 1
}
```

```output
Exception: Blocks are written by indentation, not braces; remove "{" and "}" and indent the body below the line
<no file:2>
    if true {
            ^
```

### semicolon

```yumi crash
. x = 1;
```

```output
Exception: A line needs no ";"; remove it
<no file:2>
    . x = 1;
           ^
```

### return keyword

```yumi crash
-- f()

━━━━━━━━━━━━━━━━━━━━
+ f I32

return 3
```

```output
Exception: Brix has no "return"; in a do function, return a value with "= value"
<no file:7>
    return 3
           ^
```

### let keyword

```yumi crash
let x = 3
```

```output
Exception: Declare a local with ". name = value", not "let"
<no file:2>
    let x = 3
        ^
```

### def keyword

```yumi crash
def f(x):
    -- 1
```

```output
Exception: Declare a function after a thick divider (━) as "+ name ReturnType", with each parameter on an indented "- name Type" line
<no file:2>
    def f(x):
        ^
```

### hash comment

```yumi crash
#note
-- 1
```

```output
Exception: Name "note" not found
<no file:2>
    #note
    ^^^^^
```

### escaped quote in an embed

```yumi crash
. s = "a"
-- "v ${s.find(\"a\")}"
```

```output
Exception: Inside "${...}", write quotes without a backslash, as in "${name.find("a")}"
<no file:3>
    -- "v ${s.find(\"a\")}"
                   ^
```

### statement after a divider

```yumi crash
-- 1

━━━━━━━━━━━━━━━━━━━━
+ f I32

3

━━━━━━━━━━━━━━━━━━━━
-- f()
```

```output
Exception: A divider starts a declaration, so a statement cannot follow it; put module-level statements above the first divider
<no file:10>
    -- f()
    ^
```

### declaration without a divider

```yumi crash
-- 1

━━━━━━━━━━━━━━━━━━━━
+ do a

-- 1

` second
+ do b

-- 2
```

```output
Exception: A declaration with a body needs a divider line above it: a thick divider (━) at module level, or a thin divider (─) for a member of a type
<no file:10>
    + do b
         ^
```

### a function with parameters written after another function body with no divider

```yumi crash
-- first(1)
━━━━━━━━━━━━━━━━━━━━
+ first I32
    - n I32

n + 1

+ second I32
    - n I32

n * 2
```

```output
Exception: "+ second I32" needs a divider line above it: a thick divider (━) at module level, or a thin divider (─) for a member of a type
<no file:10>
        - n I32
            ^
```

### a function written after a property with no divider

```yumi crash
-- Box().a
━━━━━━━━━━━━━━━━━━━━
+ Box

+ a = 1
+ size I32

2
```

```output
Exception: "+ size I32" needs a divider line above it: a thick divider (━) at module level, or a thin divider (─) for a member of a type
<no file:9>
    2
    ^
```

### a type at the top of a module with no divider above its member

```yumi crash
+ Box
+ a I32

────────────────────
+ This
a = 1
```

```output
Exception: Syntax error
<no file:8>
```

### an event declared with no type

```yumi crash
. d = Door()

━━━━━━━━━━━━━━━━━━━━
+ Door

+ event rang
+ count = 0
```

```output
Exception: An event names the type of what it carries, as in "+ event rang String"
<no file:7>
    + event rang
                ^
```

### A unit declared from a signed amount of another is pointed to an equality.

```yumi crash
-- 2bar as Foo

━━━━━━━━━━━━━━━━━━━━

+ Foo (foo) is F64
+ Bar (bar) is -1foo
+ Baz (baz) is 1foo
```

```output
Exception: A unit is declared from an amount of another unit with no sign, as in "+ Bar (bar) is 1foo"; to relate two units by a negative amount, declare Bar from a number type and write an equality, as in "+ 1bar = -1foo"
<no file:7>
    + Bar (bar) is -1foo
                   ^
```
