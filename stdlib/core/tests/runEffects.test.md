# Effects

### A ban rejects a door the code under it reaches, with the path from the ban

```brix compile-error
(import /JaCore/tests.h)
(blank)

(main
  (print (call parse-document))
)

(effect + probe-world
  (claim C:ctest4a)
)

(fn-do + probe Int (door probe-world)
  (return (call C:ctest4a))
)

(fn-do + load-schema Int
  (return (call probe))
)

(fn-do + parse-document Int (without probe-world)
  (return (call load-schema))
)
```

```output
Exception: cannot reach a door of 'probe-world' under 'without probe-world': parse-document (without probe-world) → load-schema → (call probe)
<no file:18>
      (return (call probe))
              ^^^^^^^^^^^^
```

### A do block bans what its code reaches, and code outside it reaches the door

```brix compile-error
(import /JaCore/tests.h)
(blank)

(main
  (print (call run))
)

(effect + probe-world
  (claim C:ctest4a)
)

(fn-do + probe Int (door probe-world)
  (return (call C:ctest4a))
)

(fn-do + helper Int
  (return (call probe))
)

(fn-do + run Int
  (let outside (call helper))
  (do (without probe-world)
    (print (call helper))
  )
  (return outside)
)
```

```output
Exception: cannot reach a door of 'probe-world' under 'without probe-world': run (do without probe-world) → helper → (call probe)
<no file:18>
      (return (call probe))
              ^^^^^^^^^^^^
```

### A ban reaches a door that answers an interface member

```brix compile-error
(import /JaCore/tests.h)
(blank)

(main
  (let source (as (call Impl) Source))
  (print (call direct source))
)

(effect + probe-world
  (claim C:ctest4a)
)

(type + Source (is Interface)
  (fn-do + take Int)
)

(type + Impl
  (implements Source)
  (fn-do + take Int (door probe-world)
    (return (call C:ctest4a))
  )
)

(fn-do + direct Int (without probe-world)
  (param source Source)
  (return (call (. source take)))
)
```

```output
Exception: cannot reach a door of 'probe-world' under 'without probe-world': direct (without probe-world) → (call (. source take)) → Impl.take
<no file:27>
      (return (call (. source take)))
              ^^^^^^^^^^^^^^^^^^^^^^
```

### A ban holds in a task started under it, and a constructor may be a door

```brix compile-error
(import /JaCore/tests.h)
(blank)

(main
  (print (call run))
)

(effect + probe-world
  (claim C:ctest4a)
)

(fn-do + probe Int (door probe-world)
  (return (call C:ctest4a))
)

(type + Reading
  (prop + value Int (= 0))
  (fn-do + This _ (door probe-world)
    (= value (call C:ctest4a))
  )
)

(fn-do + run Int (without probe-world)
  (let task
    (do-new
      (call probe)
    )
  )
  (call (. task wait))
  (return (. (call Reading) value))
)
```

```output
Exception: cannot reach a door of 'probe-world' under 'without probe-world': run (without probe-world) → a function in run → (call probe)
<no file:27>
          (call probe)
          ^^^^^^^^^^^^

Exception: cannot reach a door of 'probe-world' under 'without probe-world': run (without probe-world) → (call Reading)
<no file:31>
      (return (. (call Reading) value))
                 ^^^^^^^^^^^^^^
```

### A door that calls another door of its effect is reported where it is reached

```brix compile-error
(import /JaCore/tests.h)
(blank)

(main
  (print (call run))
)

(effect + probe-world
  (claim C:ctest4a)
)

(fn-do + inner Int (door probe-world)
  (return (call C:ctest4a))
)

(fn-do + outer Int (door probe-world)
  (return (+ (call inner) 1))
)

(fn-do + run Int (without probe-world)
  (return (call outer))
)
```

```output
Exception: cannot reach a door of 'probe-world' under 'without probe-world': run (without probe-world) → (call outer)
<no file:22>
      (return (call outer))
              ^^^^^^^^^^^^
```

### A call through a function value is an error under a ban

```brix compile-error
(import /JaCore/tests.h)
(blank)

(main
  (print (call run (=> () 1)))
)

(effect + probe-world
  (claim C:ctest4a)
)

(fn-do + run Int (without probe-world)
  (param f (fn-type Int))
  (return (call f))
)
```

```output
Exception: Function value call requires unrestricted effects: run (without probe-world) → (call f)
<no file:15>
      (return (call f))
              ^^^^^^^^
```

### A C function an effect claims is called only by its doors and the functions written in them

```brix compile-error
(import /JaCore/tests.h)
(blank)

(main
  (print (call probe))
  (print (call sneak))
)

(effect + probe-world
  (claim C:ctest4a)
)

(fn-do + probe Int (door probe-world)
  (let read (=> () (call C:ctest4a)))
  (return (call read))
)

(fn-do + sneak Int
  (return (call C:ctest4a))
)
```

```output
Exception: 'C:ctest4a' belongs to the 'probe-world' effect, so only its doors may call it
<no file:20>
      (return (call C:ctest4a))
              ^^^^^^^^^^^^^^^^
```

### A ban lets through a C function no effect claims, and code outside the ban reaches doors

```brix
(import /JaCore/tests.h)
(blank)

(main
  (print (call run))
  (print (call probe))
)

(effect + probe-world
  (claim C:ctest4a)
)

(fn-do + probe Int (door probe-world)
  (return (call C:ctest4a))
)

(fn-do + run Int (without probe-world heap)
  (call C:ctest2a 7)
  (return 1)
)
```

```output
received 71
42
```

### A name after without names an effect the module sees

```brix compile-error
(import /JaCore/tests.h)
(blank)

(main
  (print (call run))
)

(fn-do + run Int (without nowhere)
  (return 1)
)
```

```output
Exception: Unknown name 'nowhere' after without
<no file:9>
    (fn-do + run Int (without nowhere)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A C function two effects claim is called by neither's doors

```brix compile-error
(import /JaCore/tests.h)
(blank)

(main
  (print (call probe))
)

(effect + probe-world
  (claim C:ctest4a)
)

(effect + other-world
  (claim C:ctest4a)
)

(fn-do + probe Int (door probe-world)
  (return (call C:ctest4a))
)
```

```output
Exception: 'C:ctest4a' is claimed by both the 'probe-world' and the 'other-world' effect
<no file:18>
      (return (call C:ctest4a))
              ^^^^^^^^^^^^^^^^
```

### A ban follows a getter, an operator and a conversion to the doors they are, or call

```brix compile-error
(import /JaCore/tests.h)
(blank)

(main
  (print (call run))
)

(effect + probe-world
  (claim C:ctest4a)
)

(type + Meter
  (prop + base Int (= 0))
  (fn-do + reading Int :get (door probe-world)
    (return (call C:ctest4a))
  )
  (fn-do + (infix this + (param other Meter)) Int (door probe-world)
    (return (+ (call C:ctest4a) (. other base)))
  )
  (fn-to-do + Int
    (return (call sample))
  )
)

(fn-do + sample Int (door probe-world)
  (let task
    (do-new
      (call C:ctest4a)
    )
  )
  (call (. task wait))
  (return 1)
)

(fn-do + take _
  (param value Int)
  (print value)
)

(fn-do + run Int (without probe-world)
  (let meter (call Meter))
  (let a (. meter reading))
  (let b (+ meter meter))
  (call take meter)
  (return (+ a b))
)
```

```output
Exception: cannot reach a door of 'probe-world' under 'without probe-world': run (without probe-world) → the converter of Meter → (call sample)
<no file:22>
        (return (call sample))
                ^^^^^^^^^^^^^

Exception: cannot reach a door of 'probe-world' under 'without probe-world': run (without probe-world) → (. meter reading)
<no file:43>
      (let a (. meter reading))
             ^^^^^^^^^^^^^^^^^

Exception: cannot reach a door of 'probe-world' under 'without probe-world': run (without probe-world) → (+ meter meter)
<no file:44>
      (let b (+ meter meter))
             ^^^^^^^^^^^^^^^
```

### A fake bound to an effect answers each door the code under the binding reaches, and code outside it reaches the real doors

```brix
(import /JaCore/tests.h)
(blank)

(main
  (print (call run))
  (print (call add 2 3))
)

(effect + adder-world
  (claim C:ctest5)
)

(fn-do + add I32 (door adder-world)
  (param a I32)
  (param b I32)
  (return (call C:ctest5 a b))
)

(fn-do + twice I32
  (param a I32)
  (return (call add a a))
)

(type + Multiplier
  (prop + calls _ (= 0))
  (fn-do + add I32
    (param a I32)
    (param b I32)
    (+= calls 1)
    (return (* a b))
  )
)

(fn-do + run I32
  (let fake (call Multiplier))
  (let inside (as 0 I32))
  (do-with adder-world fake
    (= inside (+ (call twice 3) (call add 2 5)))
  )
  (print (str "calls " (. fake calls)))
  (return (+ inside (call twice 3)))
)
```

```output
calls 2
25
5
```

### A fake's method runs under what was bound to the effect outside its binding: the real doors, or the fake of an outer binding

```brix
(import /JaCore/tests.h)
(blank)

(main
  (print (call run))
)

(effect + adder-world
  (claim C:ctest5)
)

(fn-do + add I32 (door adder-world)
  (param a I32)
  (param b I32)
  (return (call C:ctest5 a b))
)

(fn-do + real-add I32
  (param a I32)
  (param b I32)
  (return (call add a b))
)

(type + Outer
  (fn-do + add I32
    (param a I32)
    (param b I32)
    (print (str "outer " a " " b))
    (return (+ (call real-add a b) 100))
  )
)

(type + Inner
  (fn-do + add I32
    (param a I32)
    (param b I32)
    (print (str "inner " a " " b))
    (return (+ (call real-add a b) 100))
  )
)

(fn-do + run I32 (with adder-world (call Outer))
  (print (call real-add 1 2))
  (do-with adder-world (call Inner)
    (print (call real-add 3 4))
  )
  (return 0)
)
```

```output
outer 1 2
103
inner 3 4
outer 3 4
207
0
```

### A fake must have a method of each door's name that the code under its binding reaches, taking and giving what the door does

```brix compile-error
(import /JaCore/tests.h)
(blank)

(main
  (print (call run))
)

(effect + adder-world
  (claim C:ctest5)
)

(fn-do + add I32 (door adder-world)
  (param a I32)
  (param b I32)
  (return (call C:ctest5 a b))
)

(fn-do + twice I32
  (param a I32)
  (return (call add a a))
)

(type + Empty)

(type + Wrong
  (fn-do + add I64
    (param a I32)
    (param b I32)
    (return 0)
  )
)

(fn-do + run I32
  (do-with adder-world (call Empty)
    (print (call twice 1))
  )
  (do-with adder-world (call Wrong)
    (print (call add 1 2))
  )
  (return 0)
)
```

```output
Exception: Empty has no method 'add' to answer the door of 'adder-world': run (with adder-world) → twice → (call add a a)
<no file:21>
      (return (call add a a))
              ^^^^^^^^^^^^^^

Exception: Wrong.add does not take and give what the door of 'adder-world' does: run (with adder-world) → (call add 1 2)
<no file:39>
        (print (call add 1 2))
               ^^^^^^^^^^^^^^
```

### A fake is an object of a class, and answers only a door that is a function of its module

```brix compile-error
(import /JaCore/tests.h)
(blank)

(main
  (print (call run))
)

(effect + adder-world
  (claim C:ctest5)
)

(type + Meter
  (fn-do + reading I32 :get (door adder-world)
    (return (call C:ctest5 1 2))
  )
)

(type + Fake)

(fn-do + run I32
  (do-with adder-world 5
    (print 1)
  )
  (do-with adder-world (call Fake)
    (print (. (call Meter) reading))
  )
  (return 0)
)
```

```output
Exception: A fake bound to 'adder-world' is an object of a class, not I64
<no file:22>
      (do-with adder-world 5
                           ^

Exception: a fake answers only a door of 'adder-world' that is a function of its module, and Meter.reading is not: run (with adder-world) → (. (call Meter) reading)
<no file:26>
        (print (. (call Meter) reading))
               ^^^^^^^^^^^^^^^^^^^^^^^^
```

### Under a ban a fake's door passes, and a fake's method that reaches the real door is the error

```brix compile-error
(import /JaCore/tests.h)
(blank)

(main
  (print (call run))
)

(effect + adder-world
  (claim C:ctest5)
)

(fn-do + add I32 (door adder-world)
  (param a I32)
  (param b I32)
  (return (call C:ctest5 a b))
)

(fn-do + real-add I32
  (param a I32)
  (param b I32)
  (return (call add a b))
)

(type + Quiet
  (fn-do + add I32
    (param a I32)
    (param b I32)
    (return (- a b))
  )
)

(type + Passing
  (fn-do + add I32
    (param a I32)
    (param b I32)
    (return (call real-add a b))
  )
)

(fn-do + run I32 (without adder-world)
  (let got (as 0 I32))
  (do-with adder-world (call Quiet)
    (= got (call add 5 2))
  )
  (do-with adder-world (call Passing)
    (+= got (call add 5 2))
  )
  (return got)
)
```

```output
Exception: cannot reach a door of 'adder-world' under 'without adder-world': run (without adder-world) → add → Passing.add → real-add → (call add a b)
<no file:22>
      (return (call add a b))
              ^^^^^^^^^^^^^^
```

### A task started under a binding keeps it, and so does a lambda made under it

```brix
(import /JaCore/tests.h)
(blank)

(main
  (print (call run))
)

(effect + adder-world
  (claim C:ctest5)
)

(fn-do + add I32 (door adder-world)
  (param a I32)
  (param b I32)
  (return (call C:ctest5 a b))
)

(fn-do + twice I32
  (param a I32)
  (return (call add a a))
)

(type + Multiplier
  (fn-do + add I32
    (param a I32)
    (param b I32)
    (return (* a b))
  )
)

(fn-do + run I32
  (let later (? (of Getter I32)))
  (let done (call Semaphore))
  (do-with adder-world (call Multiplier)
    (do-new
      (print (tuple "task" (call twice 3)))
      (do-new
        (print (tuple "inner task" (call twice 4)))
        (call (. done post))
      )
    )
    (call (. done wait))
    (= later (=> () (call twice 5)))
  )
  (print (tuple "later" (call later)))
  (print (tuple "outside" (call twice 5)))
  (return 0)
)
```

```output
(task, 9)
(inner task, 16)
(later, 25)
(outside, 10)
0
```

### A door a function value reaches calls the fake bound where it is called, and panics where that fake has no method for it

```brix crash
(import /JaCore/tests.h)
(blank)

(main
  (print (call run))
)

(effect + adder-world
  (claim C:ctest5)
)

(fn-do + add I32 (door adder-world)
  (param a I32)
  (param b I32)
  (return (call C:ctest5 a b))
)

(fn-do + twice I32
  (param a I32)
  (return (call add a a))
)

(type + Multiplier
  (fn-do + add I32
    (param a I32)
    (param b I32)
    (return (* a b))
  )
)

(type + Empty)

(fn-do + call-it I32
  (param f (of Mapper I32 I32))
  (return (call f 3))
)

(fn-do + run I32
  (let through (of Mapper I32 I32) twice)
  (print (tuple "outside" (call call-it through)))
  (do-with adder-world (call Multiplier)
    (print (tuple "multiplier" (call call-it through)))
  )
  (do-with adder-world (call Empty)
    (print (tuple "empty" (call call-it through)))
  )
  (return 0)
)
```

```output
(outside, 6)
(multiplier, 9)
```

### A closure made outside a binding reaches what was bound where it was made, and a named function made a value there reads the binding where it is called

```brix
(import /JaCore/tests.h)
(blank)

(main
  (print (call run))
)

(effect + adder-world
  (claim C:ctest5)
)

(fn-do + add I32 (door adder-world)
  (param a I32)
  (param b I32)
  (return (call C:ctest5 a b))
)

(fn-do + twice I32
  (param a I32)
  (return (call add a a))
)

(type + Multiplier
  (fn-do + add I32
    (param a I32)
    (param b I32)
    (return (* a b))
  )
)

(fn-do + call-it I32
  (param f (of Getter I32))
  (return (call f))
)

(fn-do + apply I32
  (param f (of Mapper I32 I32))
  (return (call f 3))
)

(fn-do + run I32
  (let n (as 3 I32))
  (let closure (of Getter I32) (=> () (call add n n)))
  (let named (of Mapper I32 I32) twice)
  (do-with adder-world (call Multiplier)
    (print (tuple "closure" (call call-it closure)))
    (print (tuple "named" (call apply named)))
  )
  (return 0)
)
```

```output
(closure, 6)
(named, 9)
0
```

### A function that reaches a door cannot become a function value under a binding of its effect

```brix compile-error
(import /JaCore/tests.h)
(blank)

(main
  (print (call run))
)

(effect + adder-world
  (claim C:ctest5)
)

(fn-do + add I32 (door adder-world)
  (param a I32)
  (param b I32)
  (return (call C:ctest5 a b))
)

(fn-do + twice I32
  (param a I32)
  (return (call add a a))
)

(type + Multiplier
  (fn-do + add I32
    (param a I32)
    (param b I32)
    (return (* a b))
  )
)

(fn-do + apply I32
  (param f (of Mapper I32 I32))
  (return (call f 3))
)

(fn-do + run I32
  (let got (as 0 I32))
  (do-with adder-world (call Multiplier)
    (= got (call apply twice))
  )
  (return got)
)
```

```output
Exception: The fake bound to 'adder-world' cannot be read through a function value: twice → add
<no file:40>
        (= got (call apply twice))
                           ^^^^^
```

### A fake stays bound to its effect where the code under the binding only reads String values of the module, written, made lazily, or copied into a local

```brix
(import /JaCore/tests.h)
(blank)
(prop + kept _ (= "x"))
(prop + joined _ (:= (++ "x" "y")))
(blank)

(main
  (let fake (call Stub))
  (do-with probe-world fake
    (print kept)
  )
  (do-with probe-world fake
    (print joined)
  )
  (do-with probe-world fake
    (let copy kept)
    (print copy)
  )
)

(effect + probe-world
  (claim C:ctest4a)
)

(type + Stub)
```

```output
x
xy
x
```

### A do-with name that reaches an effect and an imported @stack global of the same name is an error naming both

```brix compile-error
(import ./world)
(import ./weather)
(blank)

(main
  (do-with world (call Stub)
    (print 1)
  )
)

(type + Stub)
```

```brix file=world.bx
(effect + world)
```

```brix file=weather.bx
(prop + world _ @stack (= 1))
```

```output
Exception: 'world' names more than one value a do-with can bind: the global in weather, the effect in world. Import only the one meant.
<no file:7>
      (do-with world (call Stub)
      ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A module's own global hides an imported effect of the same name from a do-with, and the error says so

```brix compile-error
(import ./world)
(blank)
(prop _ world _ @stack (= 1))

(main
  (do-with world (call Stub)
    (print 1)
  )
)

(type + Stub)
```

```brix file=world.bx
(effect + world)
```

```output
Exception: This module's global 'world' hides the effect in world. Rename the global to bind the effect.
<no file:7>
      (do-with world (call Stub)
      ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A do-with binds only a @stack global or an effect

```brix compile-error
(prop _ scale Int (= 1))

(main
  (do-with scale 2
    (print scale)
  )
)
```

```output
Exception: 'scale' is a global that is not @stack, so a do-with cannot bind it
<no file:5>
      (do-with scale 2
      ^^^^^^^^^^^^^^^
```

### A subscription loop reaches what was bound where it subscribed: the real door outside a binding, and the fake inside one

```brix
(prop + numbers (of AsyncChannel Int))
(prop + letters (of AsyncChannel Int))
(blank)

(main
  (for (on n numbers)
    (print (+ n (call roll)))
  )
  (do-with world (call Stub)
    (for (on n letters)
      (print (+ n (call roll)))
    )
  )
  (<- numbers 0)
  (<- letters 0)
  (print "done")
)

(effect + world)

(fn-do + roll Int (door world)
  (return 10)
)

(type + Stub
  (fn-do + roll Int
    (return 1)
  )
)
```

```output
done
10
1
```
