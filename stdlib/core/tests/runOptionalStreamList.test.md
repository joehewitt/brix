# Optional stream list

```yumi
. a = Entry("a")
. b = Entry("b")
. missing Entry?
. source [Entry?] = [a, missing, b]
for item in source
    -- if item then "present" else "absent"
. entries = collect(source)
-- entries.length
for entry in entries
    -- entry.text

━━━━━━━━━━━━━━━━━━━━
+ Entry

+ text String

────────────────────
+ This
    - text String

━━━━━━━━━━━━━━━━━━━━
+ collect
    - source <Entry?>

** source
?*> each
|> List
```

```output
present
absent
present
2
a
b
```

```yumi
. mapped =
    ** values()
    *> each ?? -1
    |> List
. filtered =
    ** values()
    ?> each != null
    *> each ?? -1
    |> List
-- ",".join(mapped)
-- ",".join(filtered)

━━━━━━━━━━━━━━━━━━━━
+ do values @inline

. missing I32?
<= 2
<= missing
<= 4
```

```output
2,-1,4
2,4
```

### A stream of optional objects passes the ones that hold nothing to a loop

```yumi
. boxes [Box?] = [Box(1), null, Box(3)]
. count = 0
for box in boxes
    count += 1
    -- if box then box.value else 0
-- count
-- total(boxes)
-- total(some(boxes))

━━━━━━━━━━━━━━━━━━━━
+ Box

+ value Int

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ do some <Box?>
    - boxes [Box?]

for box in boxes
    <= box
<= null

━━━━━━━━━━━━━━━━━━━━
+ do total Int
    - boxes <Box?>

. sum = 0
for box in boxes
    if box
        sum += box.value
    else
        sum += 100
= sum
```

```output
1
0
3
3
104
204
```

### A map-filter whose body is never null drops no item, and asks for a map.

```yumi crash
-- List([1, 2, 3] ?*> (if each > 1 then each else null))
-- List([1, 2, 3] ?*> each * 2)
```

```output
Exception: The body of this "?*>" gives I64, which is never null, so it drops no item; write "*>"
<no file:3>
    -- List([1, 2, 3] ?*> each * 2)
            ^^^^^^^^^^^^^^^^^^^^^^
```
