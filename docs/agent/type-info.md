# Type names

`type-of value` retains the declared type of its expression. Its name properties answer separate questions:

| Property | Meaning |
| --- | --- |
| `.name` | The declared type used to check the source expression. |
| `.concrete-name` | The concrete type recorded by the runtime type descriptor. Interface values obtain this descriptor from their witness table. |
| `.inlined-name` | The type known at an expanded inline call site. When no more specific type is known, this is the declared name. |

For a parameter declared as an interface, `.name` continues to report that interface after inlining. `.inlined-name` can report the concrete argument type without creating an interface box. When an inline argument selects between known concrete alternatives, `.inlined-name` follows the selected alternative. An unknown interface argument retains the interface name, even when `.concrete-name` can identify its concrete value at runtime.

The printed `TypeInfo` value and existing reflection properties retain their behavior. `.inlined-name` does not change overload selection or expose concrete-only members in a body checked against an interface.

`.inlined-name` describes compiler knowledge rather than another runtime identity. Visible local forwarding and inline returns preserve that knowledge. An opaque call or storage boundary can lose it, in which case the declared name is reported.
