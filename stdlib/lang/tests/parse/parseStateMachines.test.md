# State machines

State machines

### Parses a declaration-bound state machine with an initial case and an event transition.

```yumi
+ auth ~>
    ~> idle
    ~> authenticating on submitted
```

```brix
(prop + auth _
  (machine
    (state idle)
    (state authenticating (trigger on submitted))
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="auth"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="idle"/>
        <StateCase name="authenticating">
            <StateTrigger kind="on">
                <Id name="submitted"/>
            </StateTrigger>
        </StateCase>
    </StateMachine>
</Property>
```

### Parses target tuple payloads as state-case target expressions, not dotted union references.

```yumi
+ auth ~>
    ~> idle
    ~> authenticating(email, password)
    ~> authenticated(token)
```

```brix
(prop + auth _
  (machine
    (state idle)
    (state (call authenticating email password))
    (state (call authenticated token))
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="auth"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="idle"/>
        <StateCase name="authenticating">
            <target>
                <Id name="email"/>
                <Id name="password"/>
            </target>
        </StateCase>
        <StateCase name="authenticated">
            <target>
                <Id name="token"/>
            </target>
        </StateCase>
    </StateMachine>
</Property>
```

### Parses a top-level anonymous state machine as its own state machine node.

```yumi
~> idle
~> authenticating(email, password)
~> authenticated(token)
```

```output
<StateMachine>
    <StateCase name="idle"/>
    <StateCase name="authenticating">
        <target>
            <Id name="email"/>
            <Id name="password"/>
        </target>
    </StateCase>
    <StateCase name="authenticated">
        <target>
            <Id name="token"/>
        </target>
    </StateCase>
</StateMachine>
```

### Parses nested state cases as children of the owning state case.

```yumi
+ checkout ~>
    ~> editing
    ~> submitted
        ~> validating
        ~> charging
```

```brix
(prop + checkout _
  (machine
    (state editing)
    (state submitted
      (state validating)
      (state charging)
    )
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="checkout"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="editing"/>
        <StateCase name="submitted">
            <StateCase name="validating"/>
            <StateCase name="charging"/>
        </StateCase>
    </StateMachine>
</Property>
```

### Parses then continuations as lexical continuations, not child cases.

```yumi
+ auth ~>
    ~> authenticating for .valid(e, p) on login-submitted
        then authenticated(token) for .authentic(token) on auth-result
        then rejected for .incorrect-password on auth-result

        .. auth-result = authenticate(e, p)
```

```brix
(prop + auth _
  (machine
    (state authenticating (trigger for (call .valid e p) login-submitted)
      (then (call authenticated token) (trigger for (call .authentic token) auth-result))
      (then rejected (trigger for .incorrect-password auth-result))
      (blank)
      (lazy auth-result (call authenticate e p))
    )
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="auth"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="authenticating">
            <StateTrigger kind="for-on">
                <StructureMatch>
                    <TypeCase name="valid">
                        <TypeTuple>
                            <TypeCapture>
                                <Id name="e"/>
                            </TypeCapture>
                            <TypeCapture>
                                <Id name="p"/>
                            </TypeCapture>
                        </TypeTuple>
                    </TypeCase>
                </StructureMatch>
                <Id name="login-submitted"/>
            </StateTrigger>
            <StateContinuation name="authenticated">
                <target>
                    <Id name="token"/>
                </target>
                <StateTrigger kind="for-on">
                    <StructureMatch>
                        <TypeCase name="authentic">
                            <TypeCapture>
                                <Id name="token"/>
                            </TypeCapture>
                        </TypeCase>
                    </StructureMatch>
                    <Id name="auth-result"/>
                </StateTrigger>
            </StateContinuation>
            <StateContinuation name="rejected">
                <StateTrigger kind="for-on">
                    <StructureMatch>
                        <TypeCase name="incorrect-password"/>
                    </StructureMatch>
                    <Id name="auth-result"/>
                </StateTrigger>
            </StateContinuation>
            <Declaration lazy="true">
                <TypeCapture>
                    <Id name="auth-result"/>
                </TypeCapture>
                <CallExpression>
                    <Id name="authenticate"/>
                    <Argument>
                        <Id name="e"/>
                    </Argument>
                    <Argument>
                        <Id name="p"/>
                    </Argument>
                </CallExpression>
            </Declaration>
        </StateCase>
    </StateMachine>
</Property>
```

### Parses fallbacks attached to the nearest preceding transition decision group.

```yumi
+ auth ~>
    ~> editing
    ~> authenticating for .valid(e, p) on login-submitted
        then authenticated for .authentic on auth-result
        ?? editing

    ?? editing
```

```brix
(prop + auth _
  (machine
    (state editing)
    (state authenticating (trigger for (call .valid e p) login-submitted)
      (then authenticated (trigger for .authentic auth-result))
      (else-state editing)
    )
    (blank)
    (else-state editing)
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="auth"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="editing"/>
        <StateCase name="authenticating">
            <StateTrigger kind="for-on">
                <StructureMatch>
                    <TypeCase name="valid">
                        <TypeTuple>
                            <TypeCapture>
                                <Id name="e"/>
                            </TypeCapture>
                            <TypeCapture>
                                <Id name="p"/>
                            </TypeCapture>
                        </TypeTuple>
                    </TypeCase>
                </StructureMatch>
                <Id name="login-submitted"/>
            </StateTrigger>
            <StateContinuation name="authenticated">
                <StateTrigger kind="for-on">
                    <StructureMatch>
                        <TypeCase name="authentic"/>
                    </StructureMatch>
                    <Id name="auth-result"/>
                </StateTrigger>
            </StateContinuation>
            <StateFallback target="editing"/>
        </StateCase>
        <StateFallback target="editing"/>
    </StateMachine>
</Property>
```

### Parses guarded event transitions in both allowed orders.

```yumi
+ auth ~>
    ~> authenticated when each is .authentic on login-authenticated
    ~> invalid-email on submitted when !email-box.text
```

```brix
(prop + auth _
  (machine
    (state authenticated (trigger when-on (is each .authentic) login-authenticated))
    (state invalid-email (trigger on submitted (not (. email-box text))))
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="auth"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="authenticated">
            <StateTrigger kind="on-when">
                <Id name="login-authenticated"/>
                <BinaryExpression operator="is">
                    <Id name="each"/>
                    <Symbol name="authentic"/>
                </BinaryExpression>
            </StateTrigger>
        </StateCase>
        <StateCase name="invalid-email">
            <StateTrigger kind="on-when">
                <Id name="submitted"/>
                <UnaryExpression operator="!">
                    <Get name="text">
                        <Id name="email-box"/>
                    </Get>
                </UnaryExpression>
            </StateTrigger>
        </StateCase>
    </StateMachine>
</Property>
```

### Parses for-pattern transitions with a separate when guard.

```yumi
+ auth ~>
    ~> authenticating(email, password) for .valid(email, password) on login-submitted when remember-me.checked
```

```brix
(prop + auth _
  (machine
    (state (call authenticating email password) (trigger for (call .valid email password) login-submitted (. remember-me checked)))
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="auth"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="authenticating">
            <target>
                <Id name="email"/>
                <Id name="password"/>
            </target>
            <StateTrigger kind="for-on-when">
                <StructureMatch>
                    <TypeCase name="valid">
                        <TypeTuple>
                            <TypeCapture>
                                <Id name="email"/>
                            </TypeCapture>
                            <TypeCapture>
                                <Id name="password"/>
                            </TypeCapture>
                        </TypeTuple>
                    </TypeCase>
                </StructureMatch>
                <Id name="login-submitted"/>
                <Get name="checked">
                    <Id name="remember-me"/>
                </Get>
            </StateTrigger>
        </StateCase>
    </StateMachine>
</Property>
```

### Parses when without on as a stream-valued trigger expression.

```yumi
+ auth ~>
    ~> authenticated when authenticated
```

```brix
(prop + auth _
  (machine
    (state authenticated (trigger when authenticated))
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="auth"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="authenticated">
            <StateTrigger kind="when-stream">
                <Id name="authenticated"/>
            </StateTrigger>
        </StateCase>
    </StateMachine>
</Property>
```

### Parses whenever as a trigger whose value the machine follows, read in a closure of its own.

```yumi
+ flow ~>
    ~> idle
    ~> watching
        then idle whenever #feed
```

```brix
(prop + flow _
  (machine
    (state idle)
    (state watching
      (then idle (trigger whenever #feed))
    )
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="flow"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="idle"/>
        <StateCase name="watching">
            <StateContinuation name="idle">
                <StateTrigger kind="whenever">
                    <Function names="(closure)" access="private-access">
                        <Id name="feed" tracked="true"/>
                    </Function>
                </StateTrigger>
            </StateContinuation>
        </StateCase>
    </StateMachine>
</Property>
```

### Parses immediate boolean if transitions.

```yumi
+ auth ~>
    ~> authenticated if session-token
    ~> editing if !session-token
```

```brix
(prop + auth _
  (machine
    (state authenticated (trigger if session-token))
    (state editing (trigger if (not session-token)))
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="auth"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="authenticated">
            <StateTrigger kind="if">
                <Id name="session-token"/>
            </StateTrigger>
        </StateCase>
        <StateCase name="editing">
            <StateTrigger kind="if">
                <UnaryExpression operator="!">
                    <Id name="session-token"/>
                </UnaryExpression>
            </StateTrigger>
        </StateCase>
    </StateMachine>
</Property>
```

### Parses immediate structure-match if transitions and exposes captures to the target tuple.

```yumi
+ auth ~>
    ~> authenticating(e, p) if | .valid(e, p) = login-state
```

```brix
(prop + auth _
  (machine
    (state (call authenticating e p) (trigger if-match (call .valid e p) login-state))
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="auth"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="authenticating">
            <target>
                <Id name="e"/>
                <Id name="p"/>
            </target>
            <StateTrigger kind="if-match">
                <MatchExpression>
                    <Id name="login-state"/>
                    <StructureMatch>
                        <TypeCase name="valid">
                            <TypeTuple>
                                <TypeCapture>
                                    <Id name="e"/>
                                </TypeCapture>
                                <TypeCapture>
                                    <Id name="p"/>
                                </TypeCapture>
                            </TypeTuple>
                        </TypeCase>
                    </StructureMatch>
                </MatchExpression>
            </StateTrigger>
        </StateCase>
    </StateMachine>
</Property>
```

### Parses stream expressions after on without inventing a separate stream-expression grammar.

```yumi
+ auth ~>
    ~> editing on email-box.key-input /or/ password-box.key-input
```

```brix
(prop + auth _
  (machine
    (state editing (trigger on (/or/ (. email-box key-input) (. password-box key-input))))
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="auth"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="editing">
            <StateTrigger kind="on">
                <BinaryExpression operator="or">
                    <Get name="key-input">
                        <Id name="email-box"/>
                    </Get>
                    <Get name="key-input">
                        <Id name="password-box"/>
                    </Get>
                </BinaryExpression>
            </StateTrigger>
        </StateCase>
    </StateMachine>
</Property>
```

### Parses scoped state declarations and actions as ordinary state-owned body items.

```yumi
+ auth ~>
    ~> authenticating on login-validated
        do
            spinner.start()

        do finally
            spinner.stop()

        .. auth-result = authenticate(email-box.text, password-box.text)
        then authenticated when auth-result
```

```brix
(prop + auth _
  (machine
    (state authenticating (trigger on login-validated)
      (do
        (call (. spinner start))
      )
      (blank)
      (finally
        (call (. spinner stop))
      )
      (blank)
      (lazy auth-result (call authenticate (. email-box text) (. password-box text)))
      (then authenticated (trigger when auth-result))
    )
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="auth"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="authenticating">
            <StateTrigger kind="on">
                <Id name="login-validated"/>
            </StateTrigger>
            <Do>
                <Block>
                    <CallExpression>
                        <Get name="start">
                            <Id name="spinner"/>
                        </Get>
                    </CallExpression>
                </Block>
            </Do>
            <Defer>
                <Block>
                    <CallExpression>
                        <Get name="stop">
                            <Id name="spinner"/>
                        </Get>
                    </CallExpression>
                </Block>
            </Defer>
            <Declaration lazy="true">
                <TypeCapture>
                    <Id name="auth-result"/>
                </TypeCapture>
                <CallExpression>
                    <Id name="authenticate"/>
                    <Argument>
                        <Get name="text">
                            <Id name="email-box"/>
                        </Get>
                    </Argument>
                    <Argument>
                        <Get name="text">
                            <Id name="password-box"/>
                        </Get>
                    </Argument>
                </CallExpression>
            </Declaration>
            <StateContinuation name="authenticated">
                <StateTrigger kind="when-stream">
                    <Id name="auth-result"/>
                </StateTrigger>
            </StateContinuation>
        </StateCase>
    </StateMachine>
</Property>
```

### Parses local multiline lazy stream declarations under a state case.

```yumi
+ auth ~>
    ~> rejected when rejected
        .. inauthentic
            login-authenticated
            ?> each is! .authentic

        then editing on email-box.key-input /or/ password-box.key-input
```

```brix
(prop + auth _
  (machine
    (state rejected (trigger when rejected)
      (lazy inauthentic
        (block
          login-authenticated
          (pipe ?> (is! each .authentic))
        )
      )
      (blank)
      (then editing (trigger on (/or/ (. email-box key-input) (. password-box key-input))))
    )
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="auth"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="rejected">
            <StateTrigger kind="when-stream">
                <Id name="rejected"/>
            </StateTrigger>
            <Declaration lazy="true">
                <TypeCapture>
                    <Id name="inauthentic"/>
                </TypeCapture>
                <Block>
                    <Pipeline op="?>">
                        <Id name="login-authenticated"/>
                        <BinaryExpression operator="is!">
                            <Id name="each"/>
                            <Symbol name="authentic"/>
                        </BinaryExpression>
                    </Pipeline>
                </Block>
            </Declaration>
            <StateContinuation name="editing">
                <StateTrigger kind="on">
                    <BinaryExpression operator="or">
                        <Get name="key-input">
                            <Id name="email-box"/>
                        </Get>
                        <Get name="key-input">
                            <Id name="password-box"/>
                        </Get>
                    </BinaryExpression>
                </StateTrigger>
            </StateContinuation>
        </StateCase>
    </StateMachine>
</Property>
```

### Parses independent nested named state machines under a state case.

```yumi
+ app ~>
    ~> ready
        + auth ~>
            ~> editing
            ~> authenticating
            ~> authenticated

        + network ~>
            ~> online
            ~> offline

    ~> closed
```

```brix
(prop + app _
  (machine
    (state ready
      (prop + auth _
        (machine
          (state editing)
          (state authenticating)
          (state authenticated)
        )
      )
      (blank)
      (prop + network _
        (machine
          (state online)
          (state offline)
        )
      )
    )
    (blank)
    (state closed)
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="app"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="ready">
            <Property access="public-access">
                <TypeCapture>
                    <Id name="auth"/>
                </TypeCapture>
                <StateMachine>
                    <StateCase name="editing"/>
                    <StateCase name="authenticating"/>
                    <StateCase name="authenticated"/>
                </StateMachine>
            </Property>
            <Property access="public-access">
                <TypeCapture>
                    <Id name="network"/>
                </TypeCapture>
                <StateMachine>
                    <StateCase name="online"/>
                    <StateCase name="offline"/>
                </StateMachine>
            </Property>
        </StateCase>
        <StateCase name="closed"/>
    </StateMachine>
</Property>
```

### Rejects then at top level because continuations must be owned by a source state.

```yumi crash
then authenticated on auth-result
```

```output
Exception: State continuation has no owning state
<no file:2>
    then authenticated on auth-result
    ^^^^
```

### Parses a standalone fallback as a structure match, not as a state fallback.

```yumi
?? editing
```

```brix
(main
  (default
    (inline editing)
  )
)
```

```output
<StructureMatch>
    <TypeTransform>
        <Id name="editing"/>
    </TypeTransform>
</StructureMatch>
```

### Rejects duplicate fallbacks in one decision group.

```yumi crash
+ auth ~>
    ~> authenticating on submitted
    ?? editing
    ?? rejected
```

```output
Exception: Multiple fallbacks in one state transition group
<no file:5>
        ?? rejected
        ^^^^^^^^^^^
```

### Parses a class state machine declared like a function after a thin divider.

```yumi
━━━━━━━━━━━━━━━━━━━━
+ Widget

────────────────────
+ phase ~>

~> idle
~> active on advance
```

```brix
(type + Widget
  (prop + phase _
    (machine
      (state idle)
      (state active (trigger on advance))
    )
  )
)
```

```output
<Class name="Widget" access="public-access">
    <Property access="public-access">
        <TypeCapture>
            <Id name="phase"/>
        </TypeCapture>
        <StateMachine>
            <StateCase name="idle"/>
            <StateCase name="active">
                <StateTrigger kind="on">
                    <Id name="advance"/>
                </StateTrigger>
            </StateCase>
        </StateMachine>
    </Property>
</Class>
```

### Parses a module state machine declared like a function after a thick divider.

```yumi
━━━━━━━━━━━━━━━━━━━━
+ light ~>

~> off
~> on when switched
```

```brix
(prop + light _
  (machine
    (state off)
    (state on (trigger when switched))
  )
)
```

```output
<Property access="public-access">
    <TypeCapture>
        <Id name="light"/>
    </TypeCapture>
    <StateMachine>
        <StateCase name="off"/>
        <StateCase name="on">
            <StateTrigger kind="when-stream">
                <Id name="switched"/>
            </StateTrigger>
        </StateCase>
    </StateMachine>
</Property>
```

### Rejects a state machine declaration without ~> after its name.

```yumi crash
+ auth
    ~> idle
```

```output
Exception: A state machine declaration needs ~> after its name
<no file:2>
    + auth
    ^^^^^
```

### Rejects a class state machine whose cases are indented under a property.

```yumi crash
━━━━━━━━━━━━━━━━━━━━
+ Widget

+ phase ~>
    ~> idle
```

```output
Exception: A class state machine follows a thin divider, with its cases at column zero
<no file:5>
    + phase ~>
    ^^^^^^^^^
```

### Parses local immediate declaration-bound state machines.

```yumi
━━━━━━━━━━━━━━━━━━━━
+ do build

. flow ~>
    ~> idle
    ~> ready if true
```

```brix
(fn-do + build _
  (let flow
    (machine
      (state idle)
      (state ready (trigger if true))
    )
  )
)
```

```output
<Function names="build" access="public-access" imperative="true">
    <Block>
        <Declaration>
            <TypeCapture>
                <Id name="flow"/>
            </TypeCapture>
            <StateMachine>
                <StateCase name="idle"/>
                <StateCase name="ready">
                    <StateTrigger kind="if">
                        <Id name="true"/>
                    </StateTrigger>
                </StateCase>
            </StateMachine>
        </Declaration>
    </Block>
</Function>
```

### Parses local lazy declaration-bound state machines.

```yumi
━━━━━━━━━━━━━━━━━━━━
+ build

.. flow ~>
    ~> idle
    ~> ready if enabled
```

```brix
(fn-expr + build _
  (lazy flow
    (machine
      (state idle)
      (state ready (trigger if enabled))
    )
  )
)
```

```output
<Function names="build" access="public-access">
    <Block>
        <Declaration lazy="true">
            <TypeCapture>
                <Id name="flow"/>
            </TypeCapture>
            <StateMachine>
                <StateCase name="idle"/>
                <StateCase name="ready">
                    <StateTrigger kind="if">
                        <Id name="enabled"/>
                    </StateTrigger>
                </StateCase>
            </StateMachine>
        </Declaration>
    </Block>
</Function>
```

### Parses anonymous state machines in class content sections.

```yumi
━━━━━━━━━━━━━━━━━━━━
+ Panel

────────────────────
~> idle
~> active on activate
```

```brix
(type + Panel
  (content
    (state idle)
    (state active (trigger on activate))
  )
)
```

```output
<Class name="Panel" access="public-access">
    <Content>
        <Block>
            <StateMachine>
                <StateCase name="idle"/>
                <StateCase name="active">
                    <StateTrigger kind="on">
                        <Id name="activate"/>
                    </StateTrigger>
                </StateCase>
            </StateMachine>
        </Block>
    </Content>
</Class>
```

### Parses nested named state machines inside class-owned state cases.

```yumi
━━━━━━━━━━━━━━━━━━━━
+ Session

────────────────────
+ app ~>

~> closed
~> ready
    + auth ~>
        ~> editing
        ~> authenticated on login
```

```brix
(type + Session
  (prop + app _
    (machine
      (state closed)
      (state ready
        (prop + auth _
          (machine
            (state editing)
            (state authenticated (trigger on login))
          )
        )
      )
    )
  )
)
```

```output
<Class name="Session" access="public-access">
    <Property access="public-access">
        <TypeCapture>
            <Id name="app"/>
        </TypeCapture>
        <StateMachine>
            <StateCase name="closed"/>
            <StateCase name="ready">
                <Property access="public-access">
                    <TypeCapture>
                        <Id name="auth"/>
                    </TypeCapture>
                    <StateMachine>
                        <StateCase name="editing"/>
                        <StateCase name="authenticated">
                            <StateTrigger kind="on">
                                <Id name="login"/>
                            </StateTrigger>
                        </StateCase>
                    </StateMachine>
                </Property>
            </StateCase>
        </StateMachine>
    </Property>
</Class>
```

### Parses a state machine written as a section of a class as a property of the class.

```yumi
━━━━━━━━━━━━━━━━━━━━
+ Panel

────────────────────
+ phase ~>

~> idle
~> active on go
```

```brix
(type + Panel
  (prop + phase _
    (machine
      (state idle)
      (state active (trigger on go))
    )
  )
)
```

```output
<Class name="Panel" access="public-access">
    <Property access="public-access">
        <TypeCapture>
            <Id name="phase"/>
        </TypeCapture>
        <StateMachine>
            <StateCase name="idle"/>
            <StateCase name="active">
                <StateTrigger kind="on">
                    <Id name="go"/>
                </StateTrigger>
            </StateCase>
        </StateMachine>
    </Property>
</Class>
```
