# State machines

State machines

### Parses a declaration-bound state machine with an initial case and an event transition.

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

### Parses nested state cases as children of the owning state case.

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

### Parses a standalone fallback as a structure match, not as a state fallback.

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

### Parses a class state machine declared like a function after a thin divider.

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

### Parses local immediate declaration-bound state machines.

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
