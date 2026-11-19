# Associated types

### Concrete associated types resolve without adding constructor fields.

```brix
(main
  (let arena (call AssociatedArena))
  (let holder (call (of AssociatedContextHolder AssociatedArena)))
  (print (type-of arena))
  (print (type-of (call (. AssociatedArena Context))))
  (print (type-of (call (. AssociatedArena (of Backing U32)))))
  (print (type-of (call (. DerivedAssociatedArena Context))))
  (print (type-of (. holder context)))
)

(type + AssociatedContextProvider (is Interface)
  (prop + _ Context)
)

(type + (of AssociatedBox Item))

(type + AssociatedArena
  (implements AssociatedContextProvider)
  (prop + _ Context (= I32))
  (prop + _ (of Backing Item) (= (of AssociatedBox Item)))
)

(type + DerivedAssociatedArena (is AssociatedArena))

(type + (of AssociatedContextHolder Backing)
  (constraint is Backing AssociatedContextProvider)
  (prop + context (. Backing Context) (= (call (. Backing Context))))
)
```

```output
[TypeInfo AssociatedArena]
[TypeInfo I32]
[TypeInfo AssociatedBox'U32]
[TypeInfo I32]
[TypeInfo I32]
```

### An inapplicable associated-storage constructor does not block a valid fallback candidate.

```brix
(main
  (let box (call (of AssociatedCandidateFallback U32) 7u32))
  (print (. box value))
)

(type + AssociatedCandidateProvider (is Interface)
  (prop + _ (of Backing Item))
)

(type + (of AssociatedCandidateBacking Item))

(type + (of AssociatedCandidateFallback Item Backing)
  (constraint = Backing (of AssociatedCandidateBacking Item))
  (prop + value Item)
  (fn-do + This _
    (param value Item)
    (constraint = Backing (of AssociatedCandidateBacking Item))
  )
  (fn-do + (of This Storage) _
    (param storage Storage)
    (constraint is Storage AssociatedCandidateProvider)
    (constraint = Backing (. Storage (of Backing Item)))
  )
)
```

```output
7
```

### Rejected constrained constructor candidates do not instantiate converters during matching.

```brix
(main
  (let values (call constrained-stream-list false))
  (print (. values length))
  (print ([] values 0))
  (let failure (catch (call constrained-stream-list true)))
  (print (is failure ConstrainedStreamError))
)

(type + ConstrainedStreamError (is Error))

(fn-do + constrained-stream-list (list I32) (?! ConstrainedStreamError)
  (param fail Bool)
  (return (call (of List I32) (call constrained-stream-items fail)))
)

(fn-do + constrained-stream-items (stream I32)
  (param fail Bool)
  (if fail
    (throw (call ConstrainedStreamError "failed"))
  )
  (yield 7)
)
```

```output
1
7
true
```

### Interface requirements expose each conformer's binding to constrained generic code.

```brix
(main
  (let managed (call associated-context (call (of ManagedAssociatedBacking U32))))
  (let native (call associated-context (call (of NativeAssociatedBacking U32))))
  (print (type-of managed))
  (print (type-of native))
)

(type + (of AssociatedBacking Item) (is Interface)
  (prop + _ Context)
)

(type + (of ManagedAssociatedBacking Item)
  (implements (of AssociatedBacking Item))
  (prop + _ Context (= I32))
)

(type + (of NativeAssociatedBacking Item)
  (implements (of AssociatedBacking Item))
  (prop + _ Context (= U64))
)

(fn-expr + (of associated-context Item Backing) (. Backing Context)
  (param backing Backing)
  (constraint is Backing (of AssociatedBacking Item))
  (call (. Backing Context))
)
```

```output
[TypeInfo I32]
[TypeInfo U64]
```

### Parameterized associated families participate in constructor owner-parameter assignments.

```brix
(main
  (let selected (call (of AssociatedFamilyBox U32) (call AssociatedStorage)))
  (print (type-of selected))
)

(type + AssociatedStorageProvider (is Interface)
  (prop + _ (of Backing Item))
)

(type + AssociatedStorage
  (implements AssociatedStorageProvider)
  (prop + _ (of Backing Item) (= (of SelectedAssociatedBacking Item)))
)

(type + (of DefaultAssociatedBacking Item))

(type + (of SelectedAssociatedBacking Item))

(type + (of AssociatedFamilyBox Item Backing)
  (constraint = Backing (of DefaultAssociatedBacking Item))
  (fn-do + (of This Storage) _
    (param storage Storage)
    (constraint is Storage AssociatedStorageProvider)
    (constraint = Backing (. Storage (of Backing Item)))
  )
)
```

```output
[TypeInfo AssociatedFamilyBox'U32'SelectedAssociatedBacking'U32]
```

### Associated-family arguments infer backward after the owner is resolved by another argument.

```brix
(main
  (let item (call infer-associated-item (call (of SelectedAssociatedBacking U32)) (call AssociatedStorage)))
  (print (type-of item))
)

(type + AssociatedStorageProvider (is Interface)
  (prop + _ (of Backing Item))
)

(type + AssociatedStorage
  (implements AssociatedStorageProvider)
  (prop + _ (of Backing Item) (= (of SelectedAssociatedBacking Item)))
)

(type + (of SelectedAssociatedBacking Item))

(fn-expr + (of infer-associated-item Item Storage) Item
  (param backing (. Storage (of Backing Item)))
  (param storage Storage)
  (constraint is Storage AssociatedStorageProvider)
  (call Item)
)
```

```output
[TypeInfo U32]
```

### A matching concrete member cannot replace a missing generic requirement.

```brix crash
(main
  (call undeclared-associated-member (call UndeclaredAssociatedStorage))
)

(type + EmptyAssociatedProvider (is Interface))

(type + UndeclaredAssociatedStorage
  (implements EmptyAssociatedProvider)
  (prop + _ Context (= I32))
)

(fn-expr + (of undeclared-associated-member Storage) (. Storage Context)
  (param storage Storage)
  (constraint is Storage EmptyAssociatedProvider)
  (call (. Storage Context))
)
```

```output
Exception: Associated type Context is not declared by constraints on Storage
<no file:13>
    (fn-expr + (of undeclared-associated-member Storage) (. Storage Context)
                                                         ^^^^^^^^^^^^^^^^^^
```

### Conformance requires a same-named associated-type binding.

```brix crash
(main
  (call MissingAssociatedBinding)
)

(type + MissingAssociatedProvider (is Interface)
  (prop + _ Context)
)

(type + MissingAssociatedBinding
  (implements MissingAssociatedProvider)
  (prop + context I32)
)
```

```output
Exception: Must implement associated type Context in MissingAssociatedBinding
<no file:10>
    (type + MissingAssociatedBinding
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A conformer binding must preserve the requirement's family arity.

```brix crash
(main
  (call WrongArityAssociatedBinding)
)

(type + FamilyArityProvider (is Interface)
  (prop + _ (of Backing Item))
)

(type + WrongArityAssociatedBinding
  (implements FamilyArityProvider)
  (prop + _ Backing (= I32))
)
```

```output
Exception: Associated type Backing has incompatible arity
<no file:12>
      (prop + _ Backing (= I32))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A private binding cannot satisfy a public associated-type requirement.

```brix crash
(main
  (call PrivateAssociatedBinding)
)

(type + PublicAssociatedProvider (is Interface)
  (prop + _ Context)
)

(type + PrivateAssociatedBinding
  (implements PublicAssociatedProvider)
  (prop _ _ Context (= I32))
)
```

```output
Exception: Associated type Context must be public
<no file:12>
      (prop _ _ Context (= I32))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Circular concrete bindings diagnose the dependent lookup.

```brix crash
(main
  (call (. CircularAssociation Context))
)

(type + CircularAssociation
  (prop + _ Context (= (. CircularAssociation Context)))
)
```

```output
Exception: Circular associated type Context
<no file:7>
      (prop + _ Context (= (. CircularAssociation Context)))
                           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Two inherited requirements with the same name are ambiguous in generic code.

```brix crash
(main
  (call ambiguous-associated-member (call AmbiguousAssociatedStorage))
)

(type + LeftAssociatedProvider (is Interface)
  (prop + _ Context)
)

(type + RightAssociatedProvider (is Interface)
  (prop + _ Context)
)

(type + BothAssociatedProviders (is Interface)
  (implements LeftAssociatedProvider)
  (implements RightAssociatedProvider)
)

(type + AmbiguousAssociatedStorage
  (implements BothAssociatedProviders)
  (prop + _ Context (= I32))
)

(fn-expr + (of ambiguous-associated-member Storage) (. Storage Context)
  (param storage Storage)
  (constraint is Storage BothAssociatedProviders)
  (call (. Storage Context))
)
```

```output
Exception: Associated type Context is ambiguous
<no file:24>
    (fn-expr + (of ambiguous-associated-member Storage) (. Storage Context)
                                                        ^^^^^^^^^^^^^^^^^^
```

### An associated family with no evidence for its owner stays underconstrained.

```brix crash
(main
  (call underconstrained-associated-item (call (of UnderconstrainedBacking U32)))
)

(type + UnderconstrainedProvider (is Interface)
  (prop + _ (of Backing Item))
)

(type + (of UnderconstrainedBacking Item))

(fn-expr + (of underconstrained-associated-item Item Storage) Item
  (param backing (. Storage (of Backing Item)))
  (constraint is Storage UnderconstrainedProvider)
  (call Item)
)
```

```output
Exception: No function "underconstrained-associated-item" accepts (UnderconstrainedBacking'U32); it is declared as "underconstrained-associated-item(backing Storage.Backing'Item)"
<no file:3>
      (call underconstrained-associated-item (call (of UnderconstrainedBacking U32)))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Associated-type access obeys module visibility.

```brix crash
(import ./foreign-associated)
(blank)

(main
  (call (. ForeignAssociatedOwner Context))
)
```

```brix file=foreign-associated.bx
(type + ForeignAssociatedOwner
  (prop _ _ Context (= I32))
)
```

```output
Exception: Associated type Context is private
<no file:6>
      (call (. ForeignAssociatedOwner Context))
            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An associated type declared with ^ is shared with its module's file family

```brix
(import ./lib/shelf)
(blank)

(main
  (print (call (. (call Shelf) describe)))
)
```

```brix file=lib/shelf.bx
(import ./shelf.parts)

(type + Shelf
  (prop ^ _ Context (= I32))
  (fn-expr + describe String
    (str (call shelf-zero))
  )
)
```

```brix file=lib/shelf.parts.bx
(import ./shelf)

(fn-expr + shelf-zero I32
  (call (. Shelf Context))
)
```

```output
0
```

### An associated type declared with ^ is refused outside its module's file family

```brix crash
(import ./shared-associated)
(blank)

(main
  (call (. SharedAssociatedOwner Context))
)
```

```brix file=shared-associated.bx
(type + SharedAssociatedOwner
  (prop ^ _ Context (= I32))
)
```

```output
Exception: Associated type Context is shared, declared with ^, so only its own file family can use it
<no file:6>
      (call (. SharedAssociatedOwner Context))
            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An associated-type binding must be a type expression.

```brix crash
(main
  (call InvalidAssociatedBinding)
)

(type + InvalidAssociatedBinding
  (prop + _ Context (= 1))
)
```

```output
Exception: Associated type binding must be a type
<no file:7>
      (prop + _ Context (= 1))
                           ^
```
