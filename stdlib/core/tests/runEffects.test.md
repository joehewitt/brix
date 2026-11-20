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

### A door that answers an interface member under a ban panics where it runs

```brix crash
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
  (print "before")
  (return (call (. source take)))
)
```

```output
before
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

### A call through a function value under a ban passes, and a door it reaches panics where it runs

```brix crash
(import /JaCore/tests.h)
(blank)

(main
  (print (call run (=> () 1)))
  (print (call probe))
  (print (call run (=> () (call probe))))
)

(effect + probe-world
  (claim C:ctest4a)
)

(fn-do + probe Int (door probe-world)
  (return (call C:ctest4a))
)

(fn-do + run Int (without probe-world)
  (param f (fn-type Int))
  (return (call f))
)
```

```output
1
42
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

### A named function made a value under a binding reads the fake bound where it is called

```brix
(main
  (let kept (as twice Mapper))
  (do-with adder-world (call Multiplier)
    (print (call apply twice))
    (= kept twice)
  )
  (print (call kept 3))
  (print (call apply kept))
)

(type + Mapper (is (fn-type I32 (param a I32))))

(effect + adder-world)

(fn-do + add I32 (door adder-world)
  (param a I32)
  (param b I32)
  (return (+ a b))
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
  (param f Mapper)
  (return (call f 3))
)
```

```output
9
6
6
```

### A function that reaches a door becomes a C callback under a binding of its effect

```brix
(main
  (do-with world (call Stub)
    (call install)
  )
  (call fire)
)

(type + Callback (is (c-fn-type Void (param n I32))))

(prop _ native Pointer)

(fn-do + install _
  (= native (as (as callback Callback) Pointer))
  (call callback 0)
)

(fn-do + fire _
  (call callback 1)
)

(fn-do + callback _
  (param n I32)
  (print (call roll))
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
1
10
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

### A generator a task reads reaches the fake bound where the task was started

```brix
(main
  (call run)
)

(fn-do + run _ (with world (call Stub 5))
  (let s (call Session (call Provider)))
  (call (. s send))
  (call (. (!! (. s task)) wait))
)

(type + Provider
  (fn-do + stream (stream Int)
    (yield (call roll))
    (yield (call roll))
  )
)

(type + Session
  (prop + provider Provider)
  (prop + task (? Task))
  (fn-do + This _
    (param provider Provider)
  )
  (fn-do + send _
    (= task
      (do-new
        (for (in each (call (. provider stream)))
          (print each)
        )
      )
    )
  )
)

(effect + world)

(fn-do + roll Int (door world)
  (return 10)
)

(type + Stub
  (prop + base Int)
  (fn-do + This _
    (param base Int)
  )
  (fn-do + roll Int
    (return base)
  )
)
```

```output
5
5
```

### A generator a task reads through an overriding method reaches the fake bound where the task was started

```brix
(main
  (call run)
)

(fn-do + run _ (with world (call Stub 5))
  (let s (call Session (as (call Dice) Provider)))
  (call (. s send))
  (call (. (!! (. s task)) wait))
)

(type + Provider
  (fn-do + stream (stream Int)
    (yield 0)
  )
)

(type + Dice (is Provider)
  (fn-do + stream (stream Int)
    (yield (call roll))
    (yield (call roll))
  )
)

(type + Session
  (prop + provider Provider)
  (prop + task (? Task))
  (fn-do + This _
    (param provider Provider)
  )
  (fn-do + send _
    (= task
      (do-new
        (for (in each (call (. provider stream)))
          (print each)
        )
      )
    )
  )
)

(effect + world)

(fn-do + roll Int (door world)
  (return 10)
)

(type + Stub
  (prop + base Int)
  (fn-do + This _
    (param base Int)
  )
  (fn-do + roll Int
    (return base)
  )
)
```

```output
5
5
```

### A fake's method that calls its door through a function value reaches the real door

```brix
(main
  (do-with world (call Stub)
    (print (call roll))
  )
)

(type + Roller (is (fn-type Int)))

(effect + world)

(fn-do + roll Int (door world)
  (return 10)
)

(fn-do + real-roll Int
  (return (call roll))
)

(fn-do + through Int
  (param f Roller)
  (return (call f))
)

(type + Stub
  (fn-do + roll Int
    (return (+ (call through real-roll) 1))
  )
)
```

```output
11
```

### A print calls the print door of console, so a ban on console refuses it

```brix compile-error
(main
  (call report 3)
)

(fn-do + label String
  (param count Int)
  (return (str "count " count))
)

(fn-do + report _ (without console)
  (param count Int)
  (print (call label count))
)
```

```output
Exception: cannot reach a door of 'console' under 'without console': report (without console) → (print (call label count))
<no file:13>
      (print (call label count))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A fake bound to console answers each print under the binding, whatever the function that prints is kept from

```brix
(main
  (let fake (call Recorder))
  (do-with console fake
    (print "inside")
    (call quiet)
    (call apply shout)
  )
  (print "outside")
  (print (str "recorded " (. fake count)))
)

(fn-do + quiet _ (without heap)
  (print "quiet")
)

(fn-do + shout _
  (print (tuple 1 2))
)

(fn-do + apply _
  (param f (fn-type Void))
  (call f)
)

(type + Recorder
  (prop + count _ (= 0))
  (fn-do + print _
    (param text String)
    (+= count 1)
  )
)
```

```output
outside
recorded 3
```

### A ban holds on code that appends to a list and parses JSON, whose observers are function values

```brix
(import /JaCore/tests.h)
(import /data/json)
(blank)

(main
  (print (call tally "[1, 2, 3]"))
)

(effect + probe-world
  (claim C:ctest4a)
)

(fn-do + tally Int (?! Error) (without probe-world)
  (param text String)
  (let items (list 1 2))
  (+= items 3)
  (let parsed (call parse-json text))
  (return (+ (. items size) (. (as parsed (list JSON)) size)))
)
```

```output
6
```

### A lambda made outside a ban reaches no door under it, though the program binds a fake elsewhere

```brix crash
(import /JaCore/tests.h)
(blank)

(main
  (let lam (=> () (call probe)))
  (print (call faked))
  (print (call lam))
  (print (call run lam))
)

(effect + probe-world
  (claim C:ctest4a)
)

(fn-do + probe Int (door probe-world)
  (return (call C:ctest4a))
)

(type + Fixed
  (fn-do + probe Int
    (return 7)
  )
)

(fn-do + faked Int
  (let got 0)
  (do-with probe-world (call Fixed)
    (= got (call probe))
  )
  (return got)
)

(fn-do + run Int (without probe-world)
  (param f (fn-type Int))
  (print "inside")
  (return (call f))
)
```

```output
7
42
inside
```

### A do block's ban refuses a door a function value reaches in it, and lets it through after

```brix crash
(import /JaCore/tests.h)
(blank)

(main
  (call run probe)
)

(effect + probe-world
  (claim C:ctest4a)
)

(fn-do + probe Int (door probe-world)
  (return (call C:ctest4a))
)

(fn-do + run _
  (param f (fn-type Int))
  (do (without probe-world)
    (print "inside")
  )
  (print (call f))
  (do (without probe-world)
    (print (call f))
  )
)
```

```output
inside
42
```

### A task started under a ban refuses a door a function value reaches in it

```brix crash
(import /JaCore/tests.h)
(blank)

(main
  (call run probe)
)

(effect + probe-world
  (claim C:ctest4a)
)

(fn-do + probe Int (door probe-world)
  (return (call C:ctest4a))
)

(fn-do + run _ (without probe-world)
  (param f (fn-type Int))
  (let task (do-new
    (print "in task")
    (print (call f))
  ))
  (call (. task wait))
)
```

```output
in task
```

### A function that is an expression bans what a function value reaches in it

```brix crash
(import /JaCore/tests.h)
(blank)

(main
  (print (call plus-one (=> () 1)))
  (print (call plus-one probe))
)

(effect + probe-world
  (claim C:ctest4a)
)

(fn-do + probe Int (door probe-world)
  (return (call C:ctest4a))
)

(fn-expr + plus-one Int (without probe-world)
  (param f (fn-type Int))
  (+ (call f) 1)
)
```

```output
2
```

### A fake bound inside a ban answers a door a function value reaches

```brix
(import /JaCore/tests.h)
(blank)

(main
  (print (call run probe))
)

(effect + probe-world
  (claim C:ctest4a)
)

(fn-do + probe Int (door probe-world)
  (return (call C:ctest4a))
)

(type + Fixed
  (fn-do + probe Int
    (return 7)
  )
)

(fn-do + run Int (without probe-world)
  (param f (fn-type Int))
  (let got 0)
  (do-with probe-world (call Fixed)
    (= got (call f))
  )
  (return got)
)
```

```output
7
```

### A ban inside a binding refuses the door the fake would answer

```brix compile-error
(main
  (do-with world (call Stub)
    (print (call roll))
    (do (without world)
      (print (call twice))
    )
  )
)

(effect + world)

(fn-do + roll Int (door world)
  (return 10)
)

(fn-do + twice Int
  (return (* (call roll) 2))
)

(type + Stub
  (fn-do + roll Int
    (return 1)
  )
)
```

```output
Exception: cannot reach a door of 'world' under 'without world': main (do without world) → twice → (call roll)
<no file:18>
      (return (* (call roll) 2))
                 ^^^^^^^^^^^
```

### A door that is a method panics where a function value reaches it under a fake

```brix crash
(main
  (let read (of Getter I32) read-meter)
  (print (call read))
  (do-with world (call Stub)
    (print (call read))
  )
)

(type + Getter (is (fn-type I32)))

(effect + world)

(type + Meter
  (fn-do + reading I32 :get (door world)
    (return 4)
  )
)

(fn-do + read-meter I32
  (return (. (call Meter) reading))
)

(type + Stub)
```

```output
4
```

### A fake's method raises only what its door raises

```brix compile-error
(main
  (do-with world (call Stub)
    (print (call roll))
  )
)

(effect + world)

(fn-do + roll Int (door world)
  (return 10)
)

(type + Stub
  (fn-do + roll Int
    (throw (call Error "no dice"))
  )
)
```

```output
Exception: Stub.roll does not take and give what the door of 'world' does: main (with world) → (call roll)
<no file:4>
        (print (call roll))
               ^^^^^^^^^^^
```

### A ban on outside refuses a door of any effect, though the module where it is written does not import the one that declares it

```brix compile-error
(import ./schema)
(blank)

(main
  (print (call parse-document))
)

(fn-do + parse-document Int (without outside)
  (return (call load-schema))
)
```

```brix file=schema.bx
(import ./world)
(blank)

(fn-do + load-schema Int
  (return (call probe))
)
```

```brix file=world.bx
(import /JaCore/tests.h)
(blank)

(effect + probe-world
  (claim C:ctest4a)
)

(fn-do + probe Int (door probe-world)
  (return (call C:ctest4a))
)
```

```output
Exception: cannot reach a door of 'probe-world' under 'without outside': parse-document (without outside) → load-schema → (call probe)
<schema.bx:5>
      (return (call probe))
              ^^^^^^^^^^^^
```

### A do block's ban on outside passes code that reaches no door, with no effect imported

```brix
(main
  (let sum 0)
  (do (without outside)
    (for (in i (..< 0 4))
      (+= sum i)
    )
  )
  (print sum)
)
```

```output
6
```

### A ban on outside joins a restriction, and each is reported with the section that writes both

```brix compile-error
(import /BxCore/BxNumber.h)
(blank)

(main
  (print (call outer 3))
)

(effect + probe-world)

(fn-do + probe Int (door probe-world)
  (return 1)
)

(fn-do + outer Int (without outside heap)
  (param n Int)
  (let text (call C:brixIntToString n 0))
  (return (+ n (call probe)))
)
```

```output
Exception: allocation is not allowed under 'without heap': outer (without heap, outside) → (call C:brixIntToString n 0)
<no file:17>
      (let text (call C:brixIntToString n 0))
                ^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Exception: cannot reach a door of 'probe-world' under 'without outside': outer (without heap, outside) → (call probe)
<no file:18>
      (return (+ n (call probe)))
                   ^^^^^^^^^^^^
```

### No effect may be named outside

```brix compile-error
(effect + outside)

(main
  (print 1)
)
```

```output
Exception: No effect may be named 'outside', which after without stands for every effect
<no file:2>
    (effect + outside)
    ^^^^^^^^^^^^^^^^^
```

### A ban on outside holds past a narrower ban the code reaches, however many effects the program declares

```brix compile-error
(main
  (print (call run))
)

(effect + e1)
(effect + e2)
(effect + e3)
(effect + e4)
(effect + e5)
(effect + e6)
(effect + e7)
(effect + e8)
(effect + e9)
(effect + e10)
(effect + e11)
(effect + e12)

(fn-do + probe Int (door e1)
  (return 1)
)

(fn-do + middle Int (without e2 e3 e4 e5)
  (return (call probe))
)

(fn-do + run Int (without outside)
  (return (call middle))
)
```

```output
Exception: cannot reach a door of 'e1' under 'without outside': run (without outside) → middle → (call probe)
<no file:24>
      (return (call probe))
              ^^^^^^^^^^^^
```

### A list of feeds and a stage piped in it reach the fake bound where they are made

```brix
(main
  (call run)
)

(fn-do + run _ (with world (call Stub 5))
  (print (call (. ", " join) (call fed)))
  (print (call (. ", " join) (call staged)))
)

(fn-do + fed (list String)
  (return
    (call (of List String)
      (block
        (spread (call rolls))
        (pipe ?> (> (. each length) 0))
      )
    )
  )
)

(fn-do + staged (list String)
  (return
    (call (of List String)
      (block
        (spread (list 1 2))
        (pipe *> (str (+ each (call roll))))
      )
    )
  )
)

(fn-do + rolls (list String)
  (return (list (str (call roll)) (str (call roll))))
)

(effect + world)

(fn-do + roll Int (door world)
  (return 10)
)

(type + Stub
  (prop + base Int)
  (fn-do + This _
    (param base Int)
  )
  (fn-do + roll Int
    (return base)
  )
)
```

```output
5, 5
6, 7
```
