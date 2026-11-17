# Brix Review Procedure For Agents

Use this procedure only when asked to review Brix style. Review working code after correctness has
been established; do not burden the initial authoring pass with this guide.

## 1. Run The Core Lint Pass

Follow `docs/style/agent-brix-lint.md`. It defines the core rule set, finding statuses, and how to
fix or justify each finding. Record unresolved findings by lint rule and source location.

## 2. Review The Style Rules

Read `docs/style/agent-brix-style.md` and inspect the changed code against every applicable rule.
Cite each finding by stable rule ID and source location, for example:

```text
[YS-04] This pure list construction uses a mutable accumulator; replace it with a bullet expression.
```

Consult the domain guides linked at the end of the style guide only when their domain matches the
changed code.

For each manual finding, choose one action:

- Fix a clear violation and preserve behavior.
- Leave a justified exception and state the concrete tradeoff that makes the rule inapplicable.
- Report an uncertain finding without rewriting when resolving it requires product intent,
  performance evidence, or authority outside the review.

Do not report personal preference without a rule. If an important recurring issue has no rule, call
out the gap separately.

## 3. Verify The Review Changes

Rerun the same lint target and rule set after edits. Compile the affected Brix targets, and run
the tests the project's own instructions call for. Report fixed findings, justified exceptions,
remaining uncertainty, lint coverage, and what was and was not run.
