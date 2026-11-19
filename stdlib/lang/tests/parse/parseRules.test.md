# Rules

### Parses a no-parameter rule as a class member.

```brix
(type + Box
  (rule flex-col
    (= layout flex-col)
  )
)
```

```output
<Class name="Box" access="public-access">
    <StyleRule name="flex-col">
        <Block>
            <AssignmentExpression op="=">
                <Id name="layout"/>
                <Id name="flex-col"/>
            </AssignmentExpression>
        </Block>
    </StyleRule>
</Class>
```

### Parses an uppercase type-name selector as a style rule.

```brix
(type + Button
  (rule Button
    (= role "button")
  )
)
```

```output
<Class name="Button" access="public-access">
    <StyleRule name="Button">
        <Block>
            <AssignmentExpression op="=">
                <Id name="role"/>
                <String text="button"/>
            </AssignmentExpression>
        </Block>
    </StyleRule>
</Class>
```

### Parses an inline single parameter on a class-member rule.

```brix
(type + Box
  (rule flex
    (param amount Float)
    (= flex amount)
  )
)
```

```output
<Class name="Box" access="public-access">
    <StyleRule name="flex">
        <Param name="amount">
            <TypeId name="Float"/>
        </Param>
        <Block>
            <AssignmentExpression op="=">
                <Id name="flex"/>
                <Id name="amount"/>
            </AssignmentExpression>
        </Block>
    </StyleRule>
</Class>
```

### Parses a root "This" rule with indented content.

```brix
(type + Panel
  (rule This
    (<> Label (text: "child"))
    (= padding 1)
  )
)
```

```output
<Class name="Panel" access="public-access">
    <StyleRule name="This">
        <Block>
            <Markup>
                <TypeId name="Label"/>
                <args>
                    <String text="child"/>
                </args>
            </Markup>
            <AssignmentExpression op="=">
                <Id name="padding"/>
                <Integer value="1"/>
            </AssignmentExpression>
        </Block>
    </StyleRule>
</Class>
```

### Parses nested style rules inside a rule body.

```brix
(type + ProgressBar
  (rule blue-bar
    (= background-color sky)
    (blank)
    (rule progress-fill
      (= color white)
    )
  )
)
```

```output
<Class name="ProgressBar" access="public-access">
    <StyleRule name="blue-bar">
        <Block>
            <AssignmentExpression op="=">
                <Id name="background-color"/>
                <Id name="sky"/>
            </AssignmentExpression>
            <StyleRule name="progress-fill">
                <Block>
                    <AssignmentExpression op="=">
                        <Id name="color"/>
                        <Id name="white"/>
                    </AssignmentExpression>
                </Block>
            </StyleRule>
        </Block>
    </StyleRule>
</Class>
```

### Parses generated content inside a style-rule state branch.

```brix
(type + ListItem
  (rule chevron
    (when-style #expanded
      (<> Label (slot: after) (text: ">"))
    )
  )
)
```

```output
<Class name="ListItem" access="public-access">
    <StyleRule name="chevron">
        <Block>
            <StyleWhen state="expanded">
                <Block>
                    <Markup>
                        <TypeId name="Label"/>
                        <SlotBinding>
                            <Id name="after"/>
                        </SlotBinding>
                        <args>
                            <String text=">"/>
                        </args>
                    </Markup>
                </Block>
            </StyleWhen>
        </Block>
    </StyleRule>
</Class>
```

### Parses a negated style-rule state branch.

```brix
(type + ListItem
  (rule chevron
    (when-style (not #expanded)
      (= left 1)
    )
  )
)
```

```output
<Class name="ListItem" access="public-access">
    <StyleRule name="chevron">
        <Block>
            <StyleWhen state="expanded" negated="true">
                <Block>
                    <AssignmentExpression op="=">
                        <Id name="left"/>
                        <Integer value="1"/>
                    </AssignmentExpression>
                </Block>
            </StyleWhen>
        </Block>
    </StyleRule>
</Class>
```

### Parses nested rules and rule applications inside a style-rule state branch.

```brix
(type + Section
  (rule panel
    (when-style #expanded
      (• highlight)
      (rule header
        (= left 1)
      )
      (rule footer (• flex-row))
    )
  )
)
```

```output
<Class name="Section" access="public-access">
    <StyleRule name="panel">
        <Block>
            <StyleWhen state="expanded">
                <Block>
                    <StyleRuleApplication name="highlight"/>
                    <StyleRule name="header">
                        <Block>
                            <AssignmentExpression op="=">
                                <Id name="left"/>
                                <Integer value="1"/>
                            </AssignmentExpression>
                        </Block>
                    </StyleRule>
                    <StyleRule name="footer">
                        <StyleRuleApplications>
                            <StyleRuleApplication name="flex-row"/>
                        </StyleRuleApplications>
                    </StyleRule>
                </Block>
            </StyleWhen>
        </Block>
    </StyleRule>
</Class>
```

### Parses own style rules inside a rule body and inside a style-rule state branch.

```brix
(type + IconSlot
  (rule icon-x
    (own-rule icon-host
      (= color white)
    )
    (blank)
    (when-style #pressed
      (own-rule icon-host (• flex-row))
      (own-rule icon-mark
        (= color black)
      )
    )
  )
)
```

```output
<Class name="IconSlot" access="public-access">
    <StyleRule name="icon-x">
        <Block>
            <StyleRule name="icon-host" own="true">
                <Block>
                    <AssignmentExpression op="=">
                        <Id name="color"/>
                        <Id name="white"/>
                    </AssignmentExpression>
                </Block>
            </StyleRule>
            <StyleWhen state="pressed">
                <Block>
                    <StyleRule name="icon-host" own="true">
                        <StyleRuleApplications>
                            <StyleRuleApplication name="flex-row"/>
                        </StyleRuleApplications>
                    </StyleRule>
                    <StyleRule name="icon-mark" own="true">
                        <Block>
                            <AssignmentExpression op="=">
                                <Id name="color"/>
                                <Id name="black"/>
                            </AssignmentExpression>
                        </Block>
                    </StyleRule>
                </Block>
            </StyleWhen>
        </Block>
    </StyleRule>
</Class>
```

### Parses a nested style rule named `own` as a plain rule.

```brix
(type + Panel
  (rule chrome
    (rule own (• flex-row))
    (rule owner
      (= color white)
    )
  )
)
```

```output
<Class name="Panel" access="public-access">
    <StyleRule name="chrome">
        <Block>
            <StyleRule name="own">
                <StyleRuleApplications>
                    <StyleRuleApplication name="flex-row"/>
                </StyleRuleApplications>
            </StyleRule>
            <StyleRule name="owner">
                <Block>
                    <AssignmentExpression op="=">
                        <Id name="color"/>
                        <Id name="white"/>
                    </AssignmentExpression>
                </Block>
            </StyleRule>
        </Block>
    </StyleRule>
</Class>
```

### Parses inline shorthand applications on a style-rule header.

```brix
(type + Button
  (rule button (• bg 1) (• color blue) (• absolute)
    (= left 0)
  )
)
```

```output
<Class name="Button" access="public-access">
    <StyleRule name="button">
        <StyleRuleApplications>
            <StyleRuleApplication name="bg">
                <args>
                    <Integer value="1"/>
                </args>
            </StyleRuleApplication>
            <StyleRuleApplication name="color">
                <args>
                    <Id name="blue"/>
                </args>
            </StyleRuleApplication>
            <StyleRuleApplication name="absolute"/>
        </StyleRuleApplications>
        <Block>
            <AssignmentExpression op="=">
                <Id name="left"/>
                <Integer value="0"/>
            </AssignmentExpression>
        </Block>
    </StyleRule>
</Class>
```

### Parses bare and parameterized style-rule applications on a markup header.

```brix
(main
  (<> Box (id: box) (• flex-col) (• flex 1) (• bg blue))
)
```

```output
<Markup>
    <TypeId name="Box"/>
    <Id name="box"/>
    <StyleRuleApplications>
        <StyleRuleApplication name="flex-col"/>
        <StyleRuleApplication name="flex">
            <args>
                <Integer value="1"/>
            </args>
        </StyleRuleApplication>
        <StyleRuleApplication name="bg">
            <args>
                <Id name="blue"/>
            </args>
        </StyleRuleApplication>
    </StyleRuleApplications>
</Markup>
```

### Parses colon shorthand style-rule arguments.

```brix
(main
  (<> Box (id: box) (• flex 1) (• bg red) (• rgb (call RGB 1 2 3)))
)
```

```output
<Markup>
    <TypeId name="Box"/>
    <Id name="box"/>
    <StyleRuleApplications>
        <StyleRuleApplication name="flex">
            <args>
                <Integer value="1"/>
            </args>
        </StyleRuleApplication>
        <StyleRuleApplication name="bg">
            <args>
                <Id name="red"/>
            </args>
        </StyleRuleApplication>
        <StyleRuleApplication name="rgb">
            <args>
                <CallExpression>
                    <TypeId name="RGB"/>
                    <Argument>
                        <Integer value="1"/>
                    </Argument>
                    <Argument>
                        <Integer value="2"/>
                    </Argument>
                    <Argument>
                        <Integer value="3"/>
                    </Argument>
                </CallExpression>
            </args>
        </StyleRuleApplication>
    </StyleRuleApplications>
</Markup>
```

### Parses colon shorthand with postfix-style argument boundaries.

```brix
(main
  (<> Box (id: box) (• bg red) (• flex 1) (• tuple (tuple 1 2)) (• item ([] list 0)) (• member (. foo bar)))
)
```

```output
<Markup>
    <TypeId name="Box"/>
    <Id name="box"/>
    <StyleRuleApplications>
        <StyleRuleApplication name="bg">
            <args>
                <Id name="red"/>
            </args>
        </StyleRuleApplication>
        <StyleRuleApplication name="flex">
            <args>
                <Integer value="1"/>
            </args>
        </StyleRuleApplication>
        <StyleRuleApplication name="tuple">
            <args>
                <Tuple>
                    <Integer value="1"/>
                    <Integer value="2"/>
                </Tuple>
            </args>
        </StyleRuleApplication>
        <StyleRuleApplication name="item">
            <args>
                <Index>
                    <Id name="list"/>
                    <Integer value="0"/>
                </Index>
            </args>
        </StyleRuleApplication>
        <StyleRuleApplication name="member">
            <args>
                <Get name="bar">
                    <Id name="foo"/>
                </Get>
            </args>
        </StyleRuleApplication>
    </StyleRuleApplications>
</Markup>
```

```brix
(main
  (<> Box (• ml -260) (• offset -2.5) (• x -50%) (• y -1.25px) (• scale -1e-3) (• flex-col))
)
```

```output
<Markup>
    <TypeId name="Box"/>
    <StyleRuleApplications>
        <StyleRuleApplication name="ml">
            <args>
                <Integer value="-260"/>
            </args>
        </StyleRuleApplication>
        <StyleRuleApplication name="offset">
            <args>
                <Float value="-2.5"/>
            </args>
        </StyleRuleApplication>
        <StyleRuleApplication name="x">
            <args>
                <Integer value="-50" unit="%"/>
            </args>
        </StyleRuleApplication>
        <StyleRuleApplication name="y">
            <args>
                <Float value="-1.25" unit="px"/>
            </args>
        </StyleRuleApplication>
        <StyleRuleApplication name="scale">
            <args>
                <Float value="-1e-3"/>
            </args>
        </StyleRuleApplication>
        <StyleRuleApplication name="flex-col"/>
    </StyleRuleApplications>
</Markup>
```

### Parses styles after constructor arguments.

```brix
(main
  (<> Button (id: button) (text: "Login") (• primary) (• disabled-while is-authenticating))
)
```

```output
<Markup>
    <TypeId name="Button"/>
    <Id name="button"/>
    <args>
        <String text="Login"/>
    </args>
    <StyleRuleApplications>
        <StyleRuleApplication name="primary"/>
        <StyleRuleApplication name="disabled-while">
            <args>
                <Id name="is-authenticating"/>
            </args>
        </StyleRuleApplication>
    </StyleRuleApplications>
</Markup>
```

### Parses colon shorthand styles after constructor arguments.

```brix
(main
  (<> Button (id: button) (text: "Login") (• disabled-while is-authenticating) (• bg red))
)
```

```output
<Markup>
    <TypeId name="Button"/>
    <Id name="button"/>
    <args>
        <String text="Login"/>
    </args>
    <StyleRuleApplications>
        <StyleRuleApplication name="disabled-while">
            <args>
                <Id name="is-authenticating"/>
            </args>
        </StyleRuleApplication>
        <StyleRuleApplication name="bg">
            <args>
                <Id name="red"/>
            </args>
        </StyleRuleApplication>
    </StyleRuleApplications>
</Markup>
```

### Parses a style-rule application while preserving the normal markup body.

```brix
(main
  (<> Box (id: box) (• flex-col)
    (= background-color red)
    (<> Label (text: "child"))
  )
)
```

```output
<Markup>
    <TypeId name="Box"/>
    <Id name="box"/>
    <StyleRuleApplications>
        <StyleRuleApplication name="flex-col"/>
    </StyleRuleApplications>
    <Block>
        <AssignmentExpression op="=">
            <Id name="background-color"/>
            <Id name="red"/>
        </AssignmentExpression>
        <Markup>
            <TypeId name="Label"/>
            <args>
                <String text="child"/>
            </args>
        </Markup>
    </Block>
</Markup>
```

### Parses a top-level style sheet declaration as a class inheriting StyleSheet.

```brix
(style Theme
  (rule Button
    (= role "button")
  )
  (blank)
  (prop + accent _ (= 3))
)
```

```output
<Class name="Theme" access="public-access">
    <Implements inherited="true">
        <TypeId name="StyleSheet"/>
    </Implements>
    <StyleRule name="Button">
        <Block>
            <AssignmentExpression op="=">
                <Id name="role"/>
                <String text="button"/>
            </AssignmentExpression>
        </Block>
    </StyleRule>
    <Property access="public-access">
        <TypeCapture>
            <Id name="accent"/>
        </TypeCapture>
        <Integer value="3"/>
    </Property>
</Class>
```

### Parses a style sheet derived from another for tags, with a slot of its base it overrides.

```brix
(style DarkTheme
  (is Theme)
  (for dark high-contrast)
  (prop + accent _ (= 4))
)
```

```output
<Class name="DarkTheme" access="public-access" tags="dark, high-contrast">
    <Implements inherited="true">
        <TypeId name="Theme"/>
    </Implements>
    <Property access="public-access">
        <TypeCapture>
            <Id name="accent"/>
        </TypeCapture>
        <Integer value="4"/>
    </Property>
</Class>
```

### Parses whenever members with a markup query trigger and a value trigger.

```brix
(type + Box
  (prop + count Int)
  (whenever-rule (trigger children ListItem)
    (param item ListItem)
    (+= count 1)
  )
  (whenever-rule count
    (= count 0)
  )
)
```

```output
<Class name="Box" access="public-access">
    <Property access="public-access">
        <TypeCapture>
            <Id name="count"/>
            <TypeId name="Int"/>
        </TypeCapture>
    </Property>
    <Whenever kind="children">
        <TypeId name="ListItem"/>
        <ArgumentDecl>
            <TypeCapture>
                <Id name="item"/>
                <TypeId name="ListItem"/>
            </TypeCapture>
        </ArgumentDecl>
        <Block>
            <AssignmentExpression op="+=">
                <Id name="count"/>
                <Integer value="1"/>
            </AssignmentExpression>
        </Block>
    </Whenever>
    <Whenever kind="value">
        <Id name="count"/>
        <Block>
            <AssignmentExpression op="=">
                <Id name="count"/>
                <Integer value="0"/>
            </AssignmentExpression>
        </Block>
    </Whenever>
</Class>
```

### Parses a bullet argument whose inline block follows the colon without a space.

```brix
(type + Box
  (rule cell (• border "#07101d")
    (when-style #is-hovered
      (• border "#ffffff")
    )
  )
)
```

```output
<Class name="Box" access="public-access">
    <StyleRule name="cell">
        <StyleRuleApplications>
            <StyleRuleApplication name="border">
                <args>
                    <String text="#07101d"/>
                </args>
            </StyleRuleApplication>
        </StyleRuleApplications>
        <Block>
            <StyleWhen state="is-hovered">
                <Block>
                    <StyleRuleApplication name="border">
                        <args>
                            <String text="#ffffff"/>
                        </args>
                    </StyleRuleApplication>
                </Block>
            </StyleWhen>
        </Block>
    </StyleRule>
</Class>
```

```brix
(type + Thing
  (rule button
    (param amount Int)
    (• base (if: #selected))
    (= x amount)
  )
)
```

```output
<Class name="Thing" access="public-access">
    <StyleRule name="button">
        <Param name="amount">
            <TypeId name="Int"/>
        </Param>
        <StyleRuleApplications>
            <StyleRuleApplication name="base">
                <condition>
                    <Id name="selected" tracked="true"/>
                </condition>
            </StyleRuleApplication>
        </StyleRuleApplications>
        <Block>
            <AssignmentExpression op="=">
                <Id name="x"/>
                <Id name="amount"/>
            </AssignmentExpression>
        </Block>
    </StyleRule>
</Class>
```

```brix
(main
  (<> Box (• hidden (if: #selected)) (• flex 1 (if: #enabled)))
)
```

```output
<Markup>
    <TypeId name="Box"/>
    <StyleRuleApplications>
        <StyleRuleApplication name="hidden">
            <condition>
                <Id name="selected" tracked="true"/>
            </condition>
        </StyleRuleApplication>
        <StyleRuleApplication name="flex">
            <args>
                <Integer value="1"/>
            </args>
            <condition>
                <Id name="enabled" tracked="true"/>
            </condition>
        </StyleRuleApplication>
    </StyleRuleApplications>
</Markup>
```

### A markup element binds theme tags after `for`: each tag follows its value, and one written without a value is on. Each binding is the element's `theme-tag` row, ahead of its own rows.

```brix
(main
  (<> Box (id: stage) (• page) (for: night #is-night) (for: round)
    (= flex 1)
  )
)
```

```output
<Markup>
    <TypeId name="Box"/>
    <Id name="stage"/>
    <StyleRuleApplications>
        <StyleRuleApplication name="page"/>
    </StyleRuleApplications>
    <Block>
        <AssignmentExpression op="=">
            <Id name="theme-tag"/>
            <CallExpression>
                <TypeId name="ThemeTag"/>
                <Argument>
                    <String text="night"/>
                </Argument>
                <Argument>
                    <Id name="is-night" tracked="true"/>
                </Argument>
            </CallExpression>
        </AssignmentExpression>
        <AssignmentExpression op="=">
            <Id name="theme-tag"/>
            <CallExpression>
                <TypeId name="ThemeTag"/>
                <Argument>
                    <String text="round"/>
                </Argument>
                <Argument>
                    <Id name="true"/>
                </Argument>
            </CallExpression>
        </AssignmentExpression>
        <AssignmentExpression op="=">
            <Id name="flex"/>
            <Integer value="1"/>
        </AssignmentExpression>
    </Block>
</Markup>
```

### Rejects theme tags bound on a root `<This`, whose lines give properties their first values.

```brix
(type + Tag
  (rule badge
    (style-set is-active ink 11)
  )
)
```

```output
<Class name="Tag" access="public-access">
    <StyleRule name="badge">
        <Block>
            <StyleWhen state="is-active">
                <AssignmentExpression op="=">
                    <Id name="ink"/>
                    <Integer value="11"/>
                </AssignmentExpression>
            </StyleWhen>
        </Block>
    </StyleRule>
</Class>
```

### Parses a keyframe animation or motion as a slot whose value calls the builder its head names, and a parameter as a shape, which a name followed by a colon binds.

```brix
(style Theme
  (keyframes pop-in animation 220ms
    (= repeat 2)
    (range opacity 0 _ (:motion fade))
    (range tint _ 1 (:after 10ms))
    (track offset.x
      (at 0% 0)
      (at 50% 4 (:easing ease))
      (at 100% 0)
    )
  )
  (blank)
  (keyframes overshoot (param peak F64) motion 280ms
    (at 0% 0)
    (at 65% peak)
    (at 100% 1 (:motion spring))
  )
  (blank)
  (rule Field
    (= animation (shape shake 3))
  )
)
```

```output
<Class name="Theme" access="public-access">
    <Implements inherited="true">
        <TypeId name="StyleSheet"/>
    </Implements>
    <Property access="public-access">
        <TypeCapture>
            <Id name="pop-in"/>
        </TypeCapture>
        <CallExpression>
            <Get name="at">
                <CallExpression>
                    <Get name="at">
                        <CallExpression>
                            <Get name="at">
                                <CallExpression>
                                    <Get name="track">
                                        <CallExpression>
                                            <Get name="range">
                                                <CallExpression>
                                                    <Get name="range">
                                                        <CallExpression>
                                                            <Id name="animation"/>
                                                            <Argument>
                                                                <Integer value="220" unit="ms"/>
                                                            </Argument>
                                                            <Argument name="repeat">
                                                                <Integer value="2"/>
                                                            </Argument>
                                                        </CallExpression>
                                                    </Get>
                                                    <Argument>
                                                        <StyleProperty name="opacity"/>
                                                    </Argument>
                                                    <Argument>
                                                        <Integer value="0"/>
                                                    </Argument>
                                                    <Argument>
                                                        <Id name="null"/>
                                                    </Argument>
                                                    <Argument name="motion">
                                                        <Id name="fade"/>
                                                    </Argument>
                                                </CallExpression>
                                            </Get>
                                            <Argument>
                                                <StyleProperty name="tint"/>
                                            </Argument>
                                            <Argument>
                                                <Id name="null"/>
                                            </Argument>
                                            <Argument>
                                                <Integer value="1"/>
                                            </Argument>
                                            <Argument name="after">
                                                <Integer value="10" unit="ms"/>
                                            </Argument>
                                        </CallExpression>
                                    </Get>
                                    <Argument>
                                        <StyleProperty name="offset" component="x"/>
                                    </Argument>
                                </CallExpression>
                            </Get>
                            <Argument>
                                <StyleProperty name="offset" component="x"/>
                            </Argument>
                            <Argument>
                                <Integer value="0" unit="%"/>
                            </Argument>
                            <Argument>
                                <Integer value="0"/>
                            </Argument>
                        </CallExpression>
                    </Get>
                    <Argument>
                        <StyleProperty name="offset" component="x"/>
                    </Argument>
                    <Argument>
                        <Integer value="50" unit="%"/>
                    </Argument>
                    <Argument>
                        <Integer value="4"/>
                    </Argument>
                    <Argument name="easing">
                        <Id name="ease"/>
                    </Argument>
                </CallExpression>
            </Get>
            <Argument>
                <StyleProperty name="offset" component="x"/>
            </Argument>
            <Argument>
                <Integer value="100" unit="%"/>
            </Argument>
            <Argument>
                <Integer value="0"/>
            </Argument>
        </CallExpression>
    </Property>
    <Property access="public-access">
        <TypeCapture>
            <Id name="overshoot"/>
        </TypeCapture>
        <CallExpression>
            <Id name="style-shape"/>
            <Argument>
                <Function names="(closure)" access="private-access">
                    <ArgumentDecl>
                        <TypeCapture>
                            <Id name="peak"/>
                            <TypeId name="F64"/>
                        </TypeCapture>
                    </ArgumentDecl>
                    <CallExpression>
                        <Get name="at">
                            <CallExpression>
                                <Get name="at">
                                    <CallExpression>
                                        <Get name="at">
                                            <CallExpression>
                                                <Id name="motion"/>
                                                <Argument>
                                                    <Integer value="280" unit="ms"/>
                                                </Argument>
                                            </CallExpression>
                                        </Get>
                                        <Argument>
                                            <Integer value="0" unit="%"/>
                                        </Argument>
                                        <Argument>
                                            <Integer value="0"/>
                                        </Argument>
                                    </CallExpression>
                                </Get>
                                <Argument>
                                    <Integer value="65" unit="%"/>
                                </Argument>
                                <Argument>
                                    <Id name="peak"/>
                                </Argument>
                            </CallExpression>
                        </Get>
                        <Argument>
                            <Integer value="100" unit="%"/>
                        </Argument>
                        <Argument>
                            <Integer value="1"/>
                        </Argument>
                        <Argument name="motion">
                            <Id name="spring"/>
                        </Argument>
                    </CallExpression>
                </Function>
            </Argument>
        </CallExpression>
    </Property>
    <StyleRule name="Field">
        <Block>
            <AssignmentExpression op="=">
                <Id name="animation"/>
                <CallExpression>
                    <Get name="bind">
                        <Id name="shake"/>
                    </Get>
                    <Argument>
                        <Integer value="3"/>
                    </Argument>
                </CallExpression>
            </AssignmentExpression>
        </Block>
    </StyleRule>
</Class>
```

### A shape's argument may be a negated number, which is a negative literal, or a number that starts with its decimal point.

```brix
(main
  (let offsets (list (shape $shake -4) (shape shake .5)))
)
```

```output
<Declaration>
    <TypeCapture>
        <Id name="offsets"/>
    </TypeCapture>
    <List>
        <CallExpression>
            <Get name="bind">
                <StyleRead name="shake"/>
            </Get>
            <Argument>
                <Integer value="-4"/>
            </Argument>
        </CallExpression>
        <CallExpression>
            <Get name="bind">
                <Id name="shake"/>
            </Get>
            <Argument>
                <Float value="0.5"/>
            </Argument>
        </CallExpression>
    </List>
</Declaration>
```
