# Choosing How To Catch Brix Issues

Use the earliest, narrowest mechanism that can identify an issue accurately. Prefer automated
enforcement when it can distinguish violations from valid code. Give each issue one primary home
rather than repeating the same rule across diagnostics and agent guides.

## Enforcement Mechanisms

| Mechanism | Use when |
| --- | --- |
| Compiler error | The compiler can prove that the program is invalid. |
| Compiler warning | The compiler can prove a noteworthy risk, but the program remains valid. |
| Lint rule | The issue is mechanically detectable but concerns maintainability, consistency, or a preferred construction. |
| Correctness rule | The compiler will not catch the mistake, it can silently produce incorrect code, and preventing it during everyday authoring justifies permanent context cost. |
| Style rule | Detection requires judgment or tradeoffs, justified exceptions are possible, and review-time guidance is sufficient. |

## Placement Rules

- Do not add a correctness rule merely to explain or preempt a compiler diagnostic.
- Prefer a lint rule over prose when the issue can be detected accurately. Put remediation and
  false-positive guidance in `agent-brix-lint.md` rather than duplicating the rule elsewhere.
- Keep correctness rules few, short, consequential, and relevant during initial authoring. They are
  loaded for everyday Brix work, so every rule has a permanent context cost.
- Put contextual preferences in the general or domain-specific style guide. Style rules must allow
  justified exceptions when their tradeoff does not fit the code.
- Promote an issue to the compiler only when it belongs to the language or compilation contract,
  not merely because compiler implementation state makes it convenient to detect there.

