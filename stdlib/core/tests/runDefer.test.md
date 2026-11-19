# Defer

```brix
(main
  (call foo)
)

(fn-do + foo _
  (finally
    (print "goodbye")
  )
  (blank)
  (finally
    (print "ok")
  )
  (blank)
  (print "hello")
)
```

```output
hello
ok
goodbye
```

```brix
(main
  (call foo)
)

(fn-do + foo _
  (if true
    (finally
      (print "goodbye")
    )
  )
  (print "hello")
)
```

```output
goodbye
hello
```

```brix
(main
  (call foo)
)

(type + Thing
  (prop + num _ (= 0))
  (fn-do + This _
    (param n _)
    (= num n)
    (print (str "hello " num))
  )
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print (str "goodbye " num))
  )
)

(fn-do + show _
  (param b _)
  (param c _)
  (print "show")
  (return 1)
)

(fn-do + foo _
  (call show b c
    (block
      (lazy b (use (call Thing 1)))
      (lazy c (use (call Thing 2)))
    )
  )
)
```

```output
hello 1
hello 2
show
goodbye 2
goodbye 1
```

```brix
(main
  (print "hi")
  (do-use (call Thing)
    (print "using")
  )
  (print "used")
)

(type + Thing
  (fn-do + begin-using _
    (print "begin")
    (return this)
  )
  (fn-do + end-using _
    (print "end")
  )
)
```

```output
hi
begin
using
end
used
```

```brix crash
(main
  (call foo)
)

(type + Thing)

(fn-expr + foo _
  (scope b
    (lazy b (use (call Thing)))
  )
)
```

```output
Exception: Thing cannot be used, as it has no "begin-using" and is not Closable; declare "begin-using" and "end-using", or implement Closable
<no file:10>
        (lazy b (use (call Thing)))
                ^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (let attempt
    (do-new
      (finally
        (print "ok")
      )
      (throw (call Panic "bad"))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
ok
bad
```

```brix
(main
  (let attempt
    (do-new
      (finally
        (print "main")
      )
      (call foo)
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
  (print "done")
)

(fn-do + foo _
  (finally
    (print "foo")
  )
  (blank)
  (call bar)
)

(fn-do + bar _
  (finally
    (print "bar")
  )
  (throw (call Panic "bad"))
)
```

```output
bar
foo
main
bad
done
```

### A value with no begin-using that is not Closable cannot be used.

```brix crash
(main
  (call foo)
)

(type + Thing
  (fn-do + end-using _
    (throw (call Panic 42))
  )
)

(fn-expr + foo _
  (scope b
    (lazy b (use (call Thing)))
  )
)
```

```output
Exception: Thing cannot be used, as it has no "begin-using" and is not Closable; declare "begin-using" and "end-using", or implement Closable
<no file:14>
        (lazy b (use (call Thing)))
                ^^^^^^^^^^^^^^^^^^
```

### A panic from a call in do finally is seen by earlier cleanup

```brix
(main
  (let attempt
    (do-new
      (finally
        (print (str "first cleanup sees " (. (call Task) failure)))
      )
      (blank)
      (finally
        (print "before cleanup call")
        (call foo)
        (print "after cleanup call")
      )
      (on-failure _
        (print)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + foo _
  (throw (call Panic "cleanup call"))
)
```

```output
before cleanup call
first cleanup sees cleanup call
```

### Direct panic in do finally is seen by earlier cleanup

```brix
(main
  (let attempt
    (do-new
      (finally
        (print (str "first cleanup sees " (. (call Task) failure)))
      )
      (blank)
      (finally
        (print "before direct cleanup panic")
        (throw (call Panic "direct cleanup"))
        (print "after direct cleanup panic")
      )
      (on-failure _
        (print)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
before direct cleanup panic
first cleanup sees direct cleanup
```

### Nested panic in do finally is seen by earlier cleanup

```brix
(main
  (let attempt
    (do-new
      (finally
        (print (str "first cleanup sees " (. (call Task) failure)))
      )
      (blank)
      (finally
        (if true
          (print "inside cleanup branch")
          (throw (call Panic "nested cleanup"))
        )
        (print "after nested cleanup")
      )
      (on-failure _
        (print)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
inside cleanup branch
first cleanup sees nested cleanup
```

### Cleanup panic replaces the active panic

```brix
(main
  (let attempt
    (do-new
      (finally
        (print (str "first cleanup sees " (. (call Task) failure)))
      )
      (blank)
      (finally
        (print "cleanup panic")
        (throw (call Panic "cleanup"))
        (print "after cleanup panic")
      )
      (blank)
      (throw (call Panic "original"))
      (on-failure _
        (print)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
cleanup panic
first cleanup sees cleanup
```

### Remaining finalizers run after a cleanup panic

```brix
(main
  (let attempt
    (do-new
      (finally
        (print (str "first cleanup sees " (. (call Task) failure)))
      )
      (blank)
      (finally
        (print "older cleanup")
      )
      (blank)
      (finally
        (print "newer cleanup")
        (throw (call Panic "newer"))
        (print "after newer cleanup")
      )
      (on-failure _
        (print)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
newer cleanup
older cleanup
first cleanup sees newer
```

### Illegal to return in do finally

```brix crash
(main
  (finally
    (return 1)
  )
)
```

```output
Exception: Illegal deferred statement
<no file:4>
        (return 1)
        ^^^^^^^^^^
```

### A cleanup cannot break or continue the loop around it, though a loop inside it can.

```brix compile-error
(main
  (for (in i (..< 0 2))
    (finally
      (for (in j (..< 0 3))
        (if (== j 1)
          (break)
        )
      )
      (continue)
    )
  )
)
```

```output
Exception: A cleanup cannot "continue" the loop around it, as it runs however its block is left; write "continue" in the loop's body
<no file:10>
          (continue)
          ^^^^^^^^^^
```

### Defer with early return

```brix
(main
  (print (call foo))
)

(fn-do + foo _
  (finally
    (print "bye")
  )
  (print "hi")
  (return 1)
  (print "again")
)
```

```output
hi
bye
1
```

### Defer with early return in nested scope

```brix
(main
  (print (call foo 2))
  (print (call foo 0))
)

(fn-do + foo _
  (param a _)
  (finally
    (print "bye")
  )
  (blank)
  (print "hi")
  (if (> a 1)
    (return 1)
  )
  (print "again")
)
```

```output
hi
bye
1
hi
again
bye
unknown
```

```brix
(main
  (let foo (use (call Foo)))
  (let ticks (call TickSource))
  (blank)
  (for (on t ticks)
    (break)
    ; This break is causing do finally cleanup here, which leads to a crash
  )
  (blank)
  (print "bye")
)

(type + Foo
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print "end")
  )
)

(type + TickSource
  (implements (of Broadcastable Int))
  (fn-expr + is-closed Bool :get
    false
  )
  (fn-do + subscribe _ (?! Error)
    (param subscriber (of Subscriber Int))
    (call subscriber 1 this)
  )
  (fn-do + unsubscribe _ (?! Error)
    (param subscriber (of Subscriber Int))
  )
  (fn-do + subscribe _ (?! Error)
    (param subscriber (of Subscribable Int))
    (call subscriber 1 this)
  )
  (fn-do + unsubscribe _ (?! Error)
    (param subscriber (of Subscribable Int))
  )
)
```

```output
bye
end
```

### Can't refer to names declared after the do finally, even though it may be executed after the name is declared

```brix crash
(main
  (finally
    (print y)
  )
  (call foo 0)
  (let y 9)
)

(fn-do + foo _
  (param n _)
  (if n
    (throw (call Panic))
  )
)
```

```output
Exception: Name "y" not found
<no file:4>
        (print y)
               ^
```

### A later do finally sees the names declared before it, though a statement before it may leave

```brix
(main
  (finally
    (print "first")
  )
  (let x 1)
  (call foo 0)
  (let y 9)
  (finally
    (print (str "second " y))
  )
  (call show)
  (print (str "end " x))
)

(fn-do + foo _
  (param n _)
  (if n
    (throw (call Panic))
  )
)

(fn-do + show _
  (finally
    (print "inner first")
  )
  (call foo 0)
  (let z 3)
  (finally
    (print (str "inner second " z))
  )
  (call foo 0)
)
```

```output
inner second 3
inner first
end 1
second 9
first
```

### use bounded by lazy function in nested expression context

```brix
(main
  (print "before")
  (let a
    (block
      (. foo a)
      (lazy foo (use (call Foo)))
    )
  )
  (print "after")
)

(type + Foo
  (prop + a _ (= 1))
  (fn-expr + begin-using _
    (before
      (print "begin")
    )
    this
  )
  (fn-do + end-using _
    (print "end")
  )
)
```

```output
before
begin
end
after
```

```brix
(import /core/arena)
(import /BxCore/BxCore.h)
(blank)

(main
  (let reserved-before (call C:brixArenaRuntimeActiveBytes))
  (let alias (call owned-arena))
  (print (== (. alias state) 2))
  (print (== (call C:brixArenaRuntimeActiveBytes) reserved-before))
)

(fn-do + owned-arena _
  (let arena (use (call Arena)))
  (call (of (. arena allocate) Byte) 8192)
  (return arena)
)
```

```output
true
true
```

```brix
(import /core/arena)
(import /BxCore/BxCore.h)
(blank)

(main
  (let reserved-before (call C:brixArenaRuntimeActiveBytes))
  (let arena (call Arena))
  (let error (catch (call fail-with-arena arena)))
  (print (is error Error))
  (print (== (. arena state) 2))
  (print (== (call C:brixArenaRuntimeActiveBytes) reserved-before))
)

(fn-do + fail-with-arena _
  (param arena Arena)
  (use arena)
  (call (of (. arena allocate) Byte) 8192)
  (throw (call Error "expected"))
)
```

```output
true
true
true
```

```brix
(import /core/arena)
(import /BxCore/BxCore.h)
(blank)

(main
  (let reserved-before (call C:brixArenaRuntimeActiveBytes))
  (let arena (call Arena))
  (let attempt
    (do-new
      (use arena)
      (call (of (. arena allocate) Byte) 8192)
      (throw (call Panic "arena panic"))
      (on-failure failure
        (print failure)
        (print (== (. arena state) 2))
        (print (== (call C:brixArenaRuntimeActiveBytes) reserved-before))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
arena panic
true
true
```

```brix
(import /core/arena)
(import /BxCore/BxCore.h)
(blank)

(main
  (let reserved-before (call C:brixArenaRuntimeReservedBytes))
  (do
    (use (call Arena 8192))
    (print (> (call C:brixArenaRuntimeReservedBytes) reserved-before))
  )
  (print (== (call C:brixArenaRuntimeReservedBytes) reserved-before))
)
```

```output
true
true
```

### Each instantiation of a generic function gets its own use cleanup.

```brix
(main
  (call show 1)
  (call show 2.5)
)

(type + Thing
  (prop + num _ (= 0))
  (fn-do + This _
    (param n _)
    (= num n)
    (print (str "hello " num))
  )
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print (str "goodbye " num))
  )
)

(fn-do + (of show T) _
  (param value T)
  (let thing (use (call Thing 1)))
  (print (str "show " value))
)
```

```output
hello 1
show 1
goodbye 1
hello 1
show 2.5
goodbye 1
```

### A later iteration that skips a use does not end the earlier iteration's value again.

```brix
(main
  (for (in i (..< 0 3))
    (print (and (== i 0) (== (. foo a) 1)))
    (lazy foo (use (call Foo)))
  )
)

(type + Foo
  (prop + a _ (= 1))
  (fn-expr + begin-using _
    (before
      (print "begin")
    )
    this
  )
  (fn-do + end-using _
    (print "end")
  )
)
```

```output
begin
true
end
false
false
```

### A panic in cleanup on a normal exit unwinds to the caller after the remaining cleanup runs.

```brix
(main
  (call guard (=> () :do (call finish false)))
  (call guard (=> () :do (call finish true)))
  (call guard (=> () :do (call nested)))
)

(fn-do + guard _
  (param body _)
  (let attempt
    (do-new
      (call body)
      (print "returned")
      (on-failure failure
        (print (str "caught " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + finish _
  (param fail Bool)
  (finally
    (print "older")
  )
  (blank)
  (finally
    (print "newer")
    (if fail
      (throw (call Panic "newer"))
    )
  )
  (blank)
  (print "body")
)

(fn-do + nested _
  (finally
    (print "outer cleanup")
  )
  (do
    (finally
      (throw (call Panic "block"))
    )
    (print "in block")
  )
  (print "after block")
)
```

```output
body
newer
older
returned
body
newer
older
caught newer
in block
outer cleanup
caught block
```

### A use in a method that a subclass also reaches through super compiles in each copy of the method.

```brix
(main
  (let base (call Opener))
  (call (. base open))
  (let child (call ChildOpener))
  (= (. child label) "child")
  (call (. child open))
)

(type + Frame
  (prop + owner Opener)
  (fn-do + This _
    (param owner _)
  )
  (fn-do + begin-using _
    (print (str "begin " (. owner label)))
    (return this)
  )
  (fn-do + end-using _
    (print (str "end " (. owner label)))
  )
)

(type + Opener
  (prop + label _ (= "base"))
  (fn-do + open _
    (let frame (use (call Frame this)))
    (print (str "open " (. frame owner label)))
  )
)

(type + ChildOpener (is Opener)
  (fn-do + open _
    (print "child first")
    (call (. super open))
  )
)
```

```output
begin base
open base
end base
child first
begin child
open child
end child
```

### A use in a loop's body ends at each turn, and where continue or break leaves it.

```brix
(main
  (for (in i (..< 0 4))
    (let t (use (call Thing i)))
    (if (== i 1)
      (continue)
    )
    (if (== i 3)
      (break)
    )
    (print (str "body " i))
  )
  (print "done")
)

(type + Thing
  (prop + n _ (= 0))
  (fn-do + This _
    (param n _)
    (print (str "hello " n))
  )
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print (str "bye " n))
  )
)
```

```output
hello 0
body 0
bye 0
hello 1
bye 1
hello 2
body 2
bye 2
hello 3
bye 3
done
```

### A use in a nested block ends where the block ends.

```brix
(main
  (let a 1)
  (if (== a 1)
    (let t (use (call Thing 1)))
    (print "in if")
  )
  (print "after if")
  (let t2 (use (call Thing 2)))
  (print "end")
)

(type + Thing
  (prop + n _ (= 0))
  (fn-do + This _
    (param n _)
    (print (str "hello " n))
  )
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print (str "bye " n))
  )
)
```

```output
hello 1
in if
bye 1
after if
hello 2
end
bye 2
```

### A use that gives a block's value lasts as long as the local the block gives it to, and one a local of the block holds ends with the block.

```brix
(main
  (let a
    (block
      (use (call Thing 1))
    )
  )
  (print (str "after a " (. a n)))
  (let b
    (block
      (let t (use (call Thing 2)))
      (. t n)
    )
  )
  (print (str "after b " b))
  (print "end")
)

(type + Thing
  (prop + n _ (= 0))
  (fn-do + This _
    (param n _)
    (print (str "hello " n))
  )
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print (str "bye " n))
  )
)
```

```output
hello 1
after a 1
hello 2
bye 2
after b 2
end
bye 1
```

### A return ends the uses open, newest first.

```brix
(main
  (print (call f true))
  (print (call f false))
)

(fn-do + f Int
  (param early Bool)
  (let t (use (call Thing 5)))
  (if early
    (return 1)
  )
  (let u (use (call Thing 6)))
  (return 2)
)

(type + Thing
  (prop + n _ (= 0))
  (fn-do + This _
    (param n _)
    (print (str "hello " n))
  )
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print (str "bye " n))
  )
)
```

```output
hello 5
bye 5
1
hello 5
hello 6
bye 6
bye 5
2
```

### A lazy local whose value is a use ends with the block of the read that evaluates it.

```brix
(main
  (print "start")
  (print (. t n))
  (if true
    (print (str "inner " (. t n)))
  )
  (print "end")
  (lazy t (use (call Thing 8)))
)

(type + Thing
  (prop + n _ (= 0))
  (fn-do + This _
    (param n _)
    (print (str "hello " n))
  )
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print (str "bye " n))
  )
)
```

```output
start
hello 8
8
inner 8
end
bye 8
```

### A panic raised by end-using is seen by a do finally opened before the use.

```brix
(main
  (let attempt
    (do-new
      (finally
        (print (str "cleanup sees " (. (call Task) failure)))
      )
      (let t (use (call Bad)))
      (on-failure _
        (print)
      )
    )
  )
  (call (. attempt wait))
)

(type + Bad
  (fn-expr + begin-using _
    (before
      (print "begin")
    )
    this
  )
  (fn-do + end-using _
    (print "end")
    (throw (call Panic "end failed"))
  )
)
```

```output
begin
end
cleanup sees end failed
```

### A do use in a loop's body ends at each turn, and where continue leaves it.

```brix
(main
  (let items (list 1 2 3))
  (for (in i items)
    (do-use (call Thing i)
      (if (== i 2)
        (continue)
      )
      (print (str "item " i))
    )
  )
  (print "done")
)

(type + Thing
  (prop + n _ (= 0))
  (fn-do + This _
    (param n _)
    (print (str "hello " n))
  )
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print (str "bye " n))
  )
)
```

```output
hello 1
item 1
bye 1
hello 2
bye 2
hello 3
item 3
bye 3
done
```

### An error that leaves a function ends the uses open.

```brix
(main
  (let r (catch (call work)))
  (print r)
)

(fn-do + work Int (?! Error)
  (let t (use (call Thing 2)))
  (let n (call fail))
  (return n)
)

(fn-do + fail Int (?! Error)
  (throw (call Error "failed"))
)

(type + Thing
  (prop + n _ (= 0))
  (fn-do + This _
    (param n _)
    (print (str "hello " n))
  )
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print (str "bye " n))
  )
)
```

```output
hello 2
bye 2
failed
```

### An arm of a match given as a value is a block of its own: the uses and `do finally` it opens end where it does, newest first.

```brix
(main
  (let k 1)
  (let v
    (match k
      (case= 1
        (let t (use (call Thing 1)))
        (finally
          (print "arm done")
        )
        (+ (. t n) (. u n))
        (lazy u (use (call Thing 2)))
      )
      (default
        0
      )
    )
  )
  (print (str "v " v))
)

(type + Thing
  (prop + n _ (= 0))
  (fn-do + This _
    (param n _)
    (print (str "hello " n))
  )
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print (str "bye " n))
  )
)
```

```output
hello 1
hello 2
bye 2
arm done
bye 1
v 3
```

### An arm given as a value ends the use it opens each time it runs.

```brix
(main
  (for (in k (..< 0 3))
    (let v
      (match k
        (case= 1
          (let t (use (call Thing k)))
          (* (. t n) 10)
        )
        (default
          (. u n)
          (lazy u (use (call Thing (+ k 5))))
        )
      )
    )
    (print (str "v " v))
  )
  (print "done")
)

(type + Thing
  (prop + n _ (= 0))
  (fn-do + This _
    (param n _)
    (print (str "hello " n))
  )
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print (str "bye " n))
  )
)
```

```output
hello 5
bye 5
v 5
hello 1
bye 1
v 10
hello 7
bye 7
v 7
done
```

### A panic leaving main where a `do finally` is open is headed by the first check made since it opened.

```brix crash
(main
  (let a (as (call Nested (as (call Leaf 1) View) 1) View))
  (let b (as (call Nested (as (call Other) View) 1) View))
  (print (call maybe 1))
  (finally
    (print "fin")
  )
  (print "go")
  (print (== a b))
)

(fn-do + maybe Int
  (param k Int)
  (if (== k 0)
    (throw (call Panic "zero"))
  )
  (return k)
)

(type + View (is Interface))

(type + Leaf (is (tuple (: value I32)))
  (implements View)
)

(type + Other
  (implements View)
)

(type + Nested (is (tuple (: inner View) (: marker I32)))
  (implements View)
)
```

```stderr
Unhandled panic: Unsupported interface equality: concrete types differ
<no file:10>

Task 1

Stack trace:
  0  __source__:InterfaceEquality:Class:__source__:Nested:Class:__source__:View no file
  1  __source__:main no file:10:10
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A panic leaving main after a block whose cleanups have run is headed by the first check made after the block.

```brix crash
(main
  (let a (as (call Nested (as (call Leaf 1) View) 1) View))
  (let b (as (call Nested (as (call Other) View) 1) View))
  (do
    (finally
      (print "inner fin")
    )
    (print (call maybe 2))
  )
  (print "after")
  (print (== a b))
)

(fn-do + maybe Int
  (param k Int)
  (if (== k 0)
    (throw (call Panic "zero"))
  )
  (return k)
)

(type + View (is Interface))

(type + Leaf (is (tuple (: value I32)))
  (implements View)
)

(type + Other
  (implements View)
)

(type + Nested (is (tuple (: inner View) (: marker I32)))
  (implements View)
)
```

```stderr
Unhandled panic: Unsupported interface equality: concrete types differ
<no file:12>

Task 1

Stack trace:
  0  __source__:InterfaceEquality:Class:__source__:Nested:Class:__source__:View no file
  1  __source__:main no file:12:10
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A panic leaving main where a `use` is open is headed by the `use`, whose `begin-using` is the first check made with it open.

```brix crash
(main
  (let a (as (call Nested (as (call Leaf 1) View) 1) View))
  (let b (as (call Nested (as (call Other) View) 1) View))
  (let t (use (call Res 1)))
  (print "go")
  (print (== a b))
)

(type + View (is Interface))

(type + Leaf (is (tuple (: value I32)))
  (implements View)
)

(type + Other
  (implements View)
)

(type + Nested (is (tuple (: inner View) (: marker I32)))
  (implements View)
)

(type + Res
  (prop + n _ (= 0))
  (fn-do + This _
    (param n _)
  )
  (fn-expr + begin-using _
    (before
      (print (str "begin " n))
    )
    this
  )
  (fn-do + end-using _
    (print (str "end " n))
    (if (>= n 100)
      (throw (call Panic (str "end panic " n)))
    )
  )
)
```

```stderr
Unhandled panic: Unsupported interface equality: concrete types differ
<no file:5>

Task 1

Stack trace:
  0  __source__:InterfaceEquality:Class:__source__:Nested:Class:__source__:View no file
  1  __source__:main no file:7:10
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A side of a conditional or of `??` is no block of its own: a use read there ends where the block around it does.

```brix
(main
  (let o (? Int) null)
  (let v (if-expr (== o null) (. t n) 0))
  (print (str "v " v))
  (let w (?? o (. u n)))
  (print (str "w " w))
  (print "end")
  (lazy t (use (call Thing 5)))
  (lazy u (use (call Thing 6)))
)

(type + Thing
  (prop + n _ (= 0))
  (fn-do + This _
    (param n _)
    (print (str "hello " n))
  )
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print (str "bye " n))
  )
)
```

```output
hello 5
v 5
hello 6
w 6
end
bye 6
bye 5
```

### A panic a cleanup raises where a block of main ends, with no check made before, is reported with no source.

```brix crash
(main
  (let a (as (call Nested (as (call Leaf 1) View) 1) View))
  (let b (as (call Nested (as (call Other) View) 1) View))
  (do
    (finally
      (print (== a b))
    )
    (print "in")
  )
  (print "after")
)

(fn-do + maybe Int
  (param k Int)
  (if (== k 0)
    (throw (call Panic "zero"))
  )
  (return k)
)

(type + View (is Interface))

(type + Leaf (is (tuple (: value I32)))
  (implements View)
)

(type + Other
  (implements View)
)

(type + Nested (is (tuple (: inner View) (: marker I32)))
  (implements View)
)
```

```stderr
Unhandled panic: Unsupported interface equality: concrete types differ
<no file>

Task 1

Stack trace:
  0  __source__:InterfaceEquality:Class:__source__:Nested:Class:__source__:View no file
  1  __source__:main no file:7:14
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### The landing pad a cleanup's panic makes where its block ends keeps no source a later check would give it.

```brix crash
(main
  (let a (as (call Nested (as (call Leaf 1) View) 1) View))
  (let b (as (call Nested (as (call Other) View) 1) View))
  (do
    (finally
      (print (== a b))
    )
    (print "in")
  )
  (print (call maybe 1))
)

(fn-do + maybe Int
  (param k Int)
  (if (== k 0)
    (throw (call Panic "zero"))
  )
  (return k)
)

(type + View (is Interface))

(type + Leaf (is (tuple (: value I32)))
  (implements View)
)

(type + Other
  (implements View)
)

(type + Nested (is (tuple (: inner View) (: marker I32)))
  (implements View)
)
```

```stderr
Unhandled panic: Unsupported interface equality: concrete types differ
<no file>

Task 1

Stack trace:
  0  __source__:InterfaceEquality:Class:__source__:Nested:Class:__source__:View no file
  1  __source__:main no file:7:14
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### Cleanup that allocates runs to completion while a panic unwinds

```brix
(main
  (let attempt
    (do-new
      (finally
        (let parts (call (of List String)))
        (for (in index (..< 0 3))
          (+= parts (str "part " index))
        )
        (print (str "cleanup " (. parts length) " " ([] parts 2)))
      )
      (blank)
      (throw (call Panic "original"))
      (on-failure failure
        (print (str "caught " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
cleanup 3 part 2
caught original
```

### Cleanup in a caller still runs to completion after a callee's cleanup allocated

```brix
(main
  (let attempt
    (do-new
      (finally
        (print (str "outer " (call label 2)))
      )
      (blank)
      (call work)
      (on-failure failure
        (print (str "caught " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + work _
  (finally
    (print (str "inner " (call label 1)))
  )
  (blank)
  (throw (call Panic "original"))
)

(fn-do + label String
  (param count Int)
  (let names (call (of List String)))
  (for (in index (..< 0 count))
    (+= names (str "step " index))
  )
  (return (str (. names length) ":" ([] names (- count 1))))
)
```

```output
inner 1:step 0
outer 2:step 1
caught original
```

### A loop over a stream in a cleanup runs the stream to its end: where the loop ends, where it breaks, where the body panicked, and where the function returns early.

```brix
(main
  (call plain)
  (call broken)
  (call inbody)
  (call outside)
  (call twice)
  (print "done")
)

(fn-do + numbers _
  (param tag String)
  (finally
    (print (str "end " tag))
  )
  (yield 1)
  (yield 2)
  (yield 3)
)

(fn-do + plain _
  (finally
    (print "cleanup")
    (for (in each (call numbers "a"))
      (print (str "each " each))
    )
    (print "after")
  )
  (print "body")
)

(fn-do + broken _
  (finally
    (for (in each (call numbers "b"))
      (if (== each 2)
        (break)
      )
      (print (str "each " each))
    )
    (print "after")
  )
  (print "body")
)

(fn-do + inbody _
  (let attempt
    (do-new
      (call thrower)
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + thrower _
  (finally
    (for (in each (call numbers "d"))
      (print (str "each " each))
    )
    (print "after")
  )
  (throw (call Panic "first"))
)

(fn-do + outside _
  (let attempt
    (do-new
      (for (in each (call numbers "e"))
        (throw (call Panic "boom") (if (== each 2)))
        (print (str "each " each))
      )
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + twice _
  (param n _ (= 2))
  (finally
    (for (in a (call numbers "f"))
      (for (in b (call numbers "g"))
        (print (str "pair " a " " b))
      )
    )
    (print "after")
  )
  (if (== n 2)
    (print "early")
    (return)
  )
  (print "late")
)
```

```output
body
cleanup
each 1
each 2
each 3
end a
after
body
each 1
end b
after
each 1
each 2
each 3
end d
after
first
each 1
end e
boom
early
pair 1 1
pair 1 2
pair 1 3
end g
pair 2 1
pair 2 2
pair 2 3
end g
pair 3 1
pair 3 2
pair 3 3
end g
end f
after
done
```

### A value returned through a use or a do finally is converted to the type the function returns before the cleanups run.

```brix
(main
  (print (str "use " (call with-use)))
  (print (str "early " (call with-early true)))
  (print (str "expression " (call with-expression)))
  (print (str "finally " (call with-finally)))
)

(type + Resource
  (prop + open _ (= false))
  (fn-do + begin-using _
    (= open true)
    (return this)
  )
  (fn-do + end-using _
    (= open false)
  )
  (fn-to-expr + I64
    (if-expr open 42 0)
  )
)

(fn-do + with-use I64
  (let input (use (call Resource)))
  (return input)
)

(fn-do + with-early I64
  (param early Bool)
  (let input (use (call Resource)))
  (return-if input (if early))
  (return 0)
)

(fn-expr + with-expression I64
  (use (call Resource))
)

(fn-do + with-finally I64
  (let input (call Resource))
  (= (. input open) true)
  (finally
    (= (. input open) false)
  )
  (return input)
)
```

```output
use 42
early 42
expression 42
finally 42
```

### A returned call that may raise is converted before the cleanups run, and an error it raises is returned once they have run.

```brix
(main
  (print (str "value " (catch (call f false))))
  (print (str "error " (catch (call f true))))
)

(type + Resource
  (prop + open _ (= false))
  (fn-to-expr + I64
    (if-expr open 42 0)
  )
)

(fn-do + make Resource (?! Error)
  (param r Resource)
  (param fails Bool)
  (if fails
    (throw (call Error "failed"))
  )
  (return r)
)

(fn-do + f I64 (?! Error)
  (param fails Bool)
  (let r (call Resource))
  (= (. r open) true)
  (finally
    (= (. r open) false)
    (print "cleanup")
  )
  (return (call make r fails))
)
```

```output
cleanup
value 42
cleanup
error failed
```

### A panic raised converting a returned value runs the cleanups once, after the conversion.

```brix
(main
  (let attempt
    (do-new
      (print (call f))
      (on-failure failure
        (print (str "caught " failure))
      )
    )
  )
  (call (. attempt wait))
)

(type + Broken
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print "end")
  )
  (fn-to-do + I64
    (print "converting")
    (throw (call Panic "conversion failed"))
    (return 0)
  )
)

(fn-do + f I64
  (let t (use (call Broken)))
  (return t)
)
```

```output
converting
end
caught conversion failed
```

### A closure written as one expression ends the use it opens as it returns.

```brix
(main
  (let make (=> () (use (call Resource))))
  (print (. (call make) open))
)

(type + Resource
  (prop + open _ (= false))
  (fn-do + begin-using _
    (= open true)
    (return this)
  )
  (fn-do + end-using _
    (= open false)
    (print "end-using")
  )
)
```

```output
end-using
false
```

### A constructor's do finally runs where its body ends, where an error leaves it, and in a constructor that super runs.

```brix
(main
  (let holder (call Holder false))
  (print (. holder value))
  (let failed (catch? (call Holder true)))
  (print (if-expr failed "made" "not made"))
  (let child (call Child))
  (print (. child value))
)

(type + Holder
  (prop + value _ (= 0))
  (fn-do + This _ (?! IOError)
    (param fails Bool)
    (finally
      (print (str "cleanup " fails))
    )
    (throw (call IOError "failed") (if fails))
    (= value 1)
  )
)

(type + Base
  (prop + value _ (= 0))
  (fn-do + This _
    (finally
      (print "base cleanup")
    )
    (= value 2)
  )
)

(type + Child (is Base)
  (fn-do + This _
    (call super)
    (finally
      (print "child cleanup")
    )
    (+= value 1)
  )
)
```

```output
cleanup false
1
cleanup true
not made
base cleanup
child cleanup
3
```

### A value that is only Closable is closed where a use of it ends.

```brix
(main
  (let thing (use (call Thing)))
  (print "body")
)

(type + Thing
  (implements Closable)
  (fn-do + close _
    (print "closed")
  )
)
```

```output
body
closed
```

### A block that uses a value that is only Closable closes it where the block is left.

```brix
(main
  (do-use (call Thing)
    (print "body")
  )
  (print "after")
)

(type + Thing
  (implements Closable)
  (fn-do + close _
    (print "closed")
  )
)
```

```output
body
closed
after
```

### An error raised where a use ends, by end-using or by close, leaves the function as one a cleanup raises does.

```brix
(main
  (print (catch (call closed)))
  (print (catch (call ended)))
)

(type + Shut
  (implements Closable)
  (fn-do + close _
    (throw (call IOError "close failed"))
  )
)

(type + Ended
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (throw (call IOError "end failed"))
  )
)

(fn-do + closed Int
  (let shut (use (call Shut)))
  (return 1)
)

(fn-do + ended Int
  (do-use (call Ended)
    (print "body")
  )
  (return 2)
)
```

```output
close failed
body
end failed
```

### A use of a value that has begin-using as well as close ends it with end-using.

```brix
(main
  (let thing (use (call Thing)))
  (print (. thing name))
)

(type + Thing
  (implements Closable)
  (prop + name _ (= "thing"))
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print "ended")
  )
  (fn-do + close _
    (print "closed")
  )
)
```

```output
thing
ended
```

### A use of an optional value that is only Closable closes it when it holds one.

```brix
(main
  (print "start")
  (do
    (let thing (use? (call make true)))
    (print (?? (?. thing name) "none"))
  )
  (do
    (let other (use? (call make false)))
    (print (?? (?. other name) "none"))
  )
)

(fn-expr _ make (? Thing)
  (param present Bool)
  (if-expr present (call Thing) null)
)

(type + Thing
  (implements Closable)
  (prop + name _ (= "thing"))
  (fn-do + close _
    (print "closed")
  )
)
```

```output
start
thing
closed
none
```
