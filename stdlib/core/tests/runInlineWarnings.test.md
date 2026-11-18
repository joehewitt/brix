# Inline warnings

```yumi warn
/core

-- recursive-warning(2)

━━━━━━━━━━━━━━━━━━━━
+ recursive-warning Int @inline
    - value Int

match value
    | 0
        0
    ??
        recursive-warning(value - 1) + 1
```

```stderr
Warning: Cannot inline __source__:recursive-warning: recursive function; @inline declaration at no file:7
<no file:14>
            recursive-warning(value - 1) + 1
            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Warning: Cannot inline __source__:recursive-warning: recursive function; @inline declaration at no file:7
<no file:4>
    -- recursive-warning(2)
       ^^^^^^^^^^^^^^^^^^^^
```

```output
2
```

```yumi warn
/core

-- even-warning(2)

━━━━━━━━━━━━━━━━━━━━
+ do even-warning Bool @inline
    - value Int

if value == 0
    = true
= odd-warning(value - 1)

━━━━━━━━━━━━━━━━━━━━
+ do odd-warning Bool @inline
    - value Int

if value == 0
    = false
= even-warning(value - 1)
```

```stderr
Warning: Cannot inline __source__:even-warning: recursive function; @inline declaration at no file:7
<no file:20>
    = even-warning(value - 1)
      ^^^^^^^^^^^^^^^^^^^^^^^
Warning: Cannot inline __source__:odd-warning: recursive function; @inline declaration at no file:15
<no file:12>
    = odd-warning(value - 1)
      ^^^^^^^^^^^^^^^^^^^^^^
Warning: Cannot inline __source__:even-warning: recursive function; @inline declaration at no file:7
<no file:4>
    -- even-warning(2)
       ^^^^^^^^^^^^^^^
```

```output
true
```

```yumi warn
/core

for value in generator-warning()
    -- value

━━━━━━━━━━━━━━━━━━━━
+ do generator-warning @inline

<= 7
```

```stderr
```

```output
7
```

```yumi warn
/core
-- dispatch(Child())
━━━━━━━━━━━━━━━━━━━━
+ Base
────────────────────
+ value Int @inline

1
━━━━━━━━━━━━━━━━━━━━
+ Child is Base
────────────────────
+ value Int

2
━━━━━━━━━━━━━━━━━━━━
+ dispatch Int
    - object Base

object.value()
```

```stderr
Warning: Cannot inline __source__:Base:value: unresolved target: virtual dispatch; @inline declaration at no file:7
<no file:20>
    object.value()
    ^^^^^^^^^^^^^^
```

```output
2
```

```yumi warn
/core

-- successful-warning(2)

━━━━━━━━━━━━━━━━━━━━
+ successful-warning Int @inline
    - value Int

value + 1

━━━━━━━━━━━━━━━━━━━━
+ unused-warning Int @inline
    - value Int

unused-warning(value)
```

```stderr
```

```output
3
```
