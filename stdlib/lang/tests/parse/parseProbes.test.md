# Probes

```yumi
// foo ${bar} baz
// foo $bar baz
// foo $bar.baz
// foo $bar?.baz
// foo $bar
```

```brix
(main
  (probe-log _ (text: "foo " bar " baz"))
  (probe-log _ (text: "foo " bar " baz"))
  (probe-log _ (text: "foo " (. bar baz)))
  (probe-log _ (text: "foo " (?. bar baz)))
  (probe-log _ (text: "foo " bar))
)
```

```output
<Probe>
    <String text="foo "/>
    <Id name="bar"/>
    <String text=" baz"/>
</Probe>
<Probe>
    <String text="foo "/>
    <Id name="bar"/>
    <String text=" baz"/>
</Probe>
<Probe>
    <String text="foo "/>
    <Get name="baz">
        <Id name="bar"/>
    </Get>
</Probe>
<Probe>
    <String text="foo "/>
    <Get name="baz" optional="true">
        <Id name="bar"/>
    </Get>
</Probe>
<Probe>
    <String text="foo "/>
    <Id name="bar"/>
</Probe>
```

```yumi
// dispatch $shown returns ${type-text(store, index, result)}
```

```brix
(main
  (probe-log _ (text: "dispatch " shown " returns " (call type-text store index result)))
)
```

```output
<Probe>
    <String text="dispatch "/>
    <Id name="shown"/>
    <String text=" returns "/>
    <CallExpression>
        <Id name="type-text"/>
        <Argument>
            <Id name="store"/>
        </Argument>
        <Argument>
            <Id name="index"/>
        </Argument>
        <Argument>
            <Id name="result"/>
        </Argument>
    </CallExpression>
</Probe>
```

```yumi
━━━━━━━━━━━━━━━━━━━━
_ do trace-call TypeId

if result
    // dispatch $shown returns ${type-text(store, index, result)}

= result
```

```brix
(fn-do _ trace-call TypeId
  (if result
    (probe-log _ (text: "dispatch " shown " returns " (call type-text store index result)))
  )
  (blank)
  (return result)
)
```

```output
<Function names="trace-call" access="private-access" imperative="true">
    <TypeId name="TypeId"/>
    <Block>
        <If>
            <TypePredicate>
                <Id name="result"/>
            </TypePredicate>
            <Block>
                <Probe>
                    <String text="dispatch "/>
                    <Id name="shown"/>
                    <String text=" returns "/>
                    <CallExpression>
                        <Id name="type-text"/>
                        <Argument>
                            <Id name="store"/>
                        </Argument>
                        <Argument>
                            <Id name="index"/>
                        </Argument>
                        <Argument>
                            <Id name="result"/>
                        </Argument>
                    </CallExpression>
                </Probe>
            </Block>
        </If>
        <Return>
            <Id name="result"/>
        </Return>
    </Block>
</Function>
```

```yumi
// $$foo
```

```brix
(main
  (probe-log _ (text: "\$" foo))
)
```

```output
<Probe>
    <String text="$"/>
    <Id name="foo"/>
</Probe>
```

```yumi
// $foo$
```

```brix
(main
  (probe-log _ (text: foo "\$"))
)
```

```output
<Probe>
    <Id name="foo"/>
    <String text="$"/>
</Probe>
```

```yumi
// \$foo
```

```brix
(main
  (probe-log _ (text: "\$foo"))
)
```

```output
<Probe>
    <String text="$foo"/>
</Probe>
```

```yumi
// \${foo}
```

```brix
(main
  (probe-log _ (text: "\${foo}"))
)
```

```output
<Probe>
    <String text="${foo}"/>
</Probe>
```

```yumi
// ${foo
// $foo}
// ${foo}}
// ${{foo}}
// ${{foo}}
```

```brix
(main
  (probe-log _ (text: "\${foo"))
  (probe-log _ (text: foo "}"))
  (probe-log _ (text: foo "}"))
  (probe-log _ (text: "\${{foo}}"))
  (probe-log _ (text: "\${{foo}}"))
)
```

```output
<Probe>
    <String text="${foo"/>
</Probe>
<Probe>
    <Id name="foo"/>
    <String text="}"/>
</Probe>
<Probe>
    <Id name="foo"/>
    <String text="}"/>
</Probe>
<Probe>
    <String text="${{foo}}"/>
</Probe>
<Probe>
    <String text="${{foo}}"/>
</Probe>
```

```yumi
-- // foo
```

```output
Exception: Syntax error
<no file:2>
    -- // foo
       ^
```

```yumi
//foo = bar
//object:foo = bar
```

```brix
(main
  (probe-assign foo bar)
  (probe-assign (: object foo) bar)
)
```

```output
<ProbeAssign key="foo">
    <Id name="bar"/>
</ProbeAssign>
<ProbeAssign key="foo" object="object">
    <Id name="bar"/>
</ProbeAssign>
```

```yumi
//+ foo $!name
```

```brix
(main
  (probe-group + _ (text: "foo " (snapshot name)))
)
```

```output
<ProbeGroup begin="true">
    <String text="foo "/>
    <ProbeSnapshot>
        <Id name="name"/>
    </ProbeSnapshot>
</ProbeGroup>
```

```yumi
//-probe.group
```

```brix
(main
  (probe-group - probe.group)
)
```

```output
<ProbeGroup end="true" key="probe.group" data="true"/>
```

```yumi
//- goodbye $name snapshot $!name
```

```brix
(main
  (probe-group - _ (text: "goodbye " name " snapshot " (snapshot name)))
)
```

```output
<ProbeGroup end="true">
    <String text="goodbye "/>
    <Id name="name"/>
    <String text=" snapshot "/>
    <ProbeSnapshot>
        <Id name="name"/>
    </ProbeSnapshot>
</ProbeGroup>
```

```yumi
//+- foo
```

```brix
(main
  (probe-group +- _ (text: "foo"))
)
```

```output
<ProbeGroup begin="true" end="true">
    <String text="foo"/>
</ProbeGroup>
```

```yumi
//+probe.group -> name, count
```

```brix
(main
  (probe-group + probe.group (items: name count))
)
```

```output
<ProbeGroup begin="true" key="probe.group" data="true">
    <Id name="name"/>
    <Id name="count"/>
</ProbeGroup>
```

```yumi
// foo $?bar baz
// foo $?bar.baz
// foo $?bar?.baz
// foo $?{bar.baz} baz
```

```brix
(main
  (probe-log _ (text: "foo " (preview bar) " baz"))
  (probe-log _ (text: "foo " (preview (. bar baz))))
  (probe-log _ (text: "foo " (preview (?. bar baz))))
  (probe-log _ (text: "foo " (preview (. bar baz)) " baz"))
)
```

```output
<Probe>
    <String text="foo "/>
    <ProbePreview>
        <Id name="bar"/>
    </ProbePreview>
    <String text=" baz"/>
</Probe>
<Probe>
    <String text="foo "/>
    <ProbePreview>
        <Get name="baz">
            <Id name="bar"/>
        </Get>
    </ProbePreview>
</Probe>
<Probe>
    <String text="foo "/>
    <ProbePreview>
        <Get name="baz" optional="true">
            <Id name="bar"/>
        </Get>
    </ProbePreview>
</Probe>
<Probe>
    <String text="foo "/>
    <ProbePreview>
        <Get name="baz">
            <Id name="bar"/>
        </Get>
    </ProbePreview>
    <String text=" baz"/>
</Probe>
```

```yumi
//tui.paint -> "repaint", frame
```

```brix
(main
  (probe-log tui.paint (items: "repaint" frame))
)
```

```output
<Probe key="tui.paint">
    <String text="repaint"/>
    <Id name="frame"/>
</Probe>
```

```yumi
//+tui.paint// begin $frame
//-tui.paint// end $frame
//+-tui.layout// layout $box
//-tui.paint
```

```brix
(main
  (probe-group + tui.paint (text: "begin " frame))
  (probe-group - tui.paint (text: "end " frame))
  (probe-group +- tui.layout (text: "layout " box))
  (probe-group - tui.paint)
)
```

```output
<ProbeGroup begin="true" key="tui.paint">
    <String text="begin "/>
    <Id name="frame"/>
</ProbeGroup>
<ProbeGroup end="true" key="tui.paint">
    <String text="end "/>
    <Id name="frame"/>
</ProbeGroup>
<ProbeGroup begin="true" end="true" key="tui.layout">
    <String text="layout "/>
    <Id name="box"/>
</ProbeGroup>
<ProbeGroup end="true" key="tui.paint" data="true"/>
```

```yumi
//channel:write -> $?message, task
//channel:read -> $?{reader.message!}, task
```

```brix
(main
  (probe-log (: channel write) (items: (preview message) task))
  (probe-log (: channel read) (items: (preview (!! (. reader message))) task))
)
```

```output
<Probe object="channel" key="write">
    <ProbePreview>
        <Id name="message"/>
    </ProbePreview>
    <Id name="task"/>
</Probe>
<Probe object="channel" key="read">
    <ProbePreview>
        <UnaryExpression operator="!">
            <Get name="message">
                <Id name="reader"/>
            </Get>
        </UnaryExpression>
    </ProbePreview>
    <Id name="task"/>
</Probe>
```

```yumi
//tui.box.root -> window
//tui.box.root
```

```brix
(main
  (probe-log tui.box.root (items: window))
  (probe-log tui.box.root)
)
```

```output
<Probe key="tui.box.root">
    <Id name="window"/>
</Probe>
<Probe key="tui.box.root"/>
```

```yumi
//http.response.headers -> response-headers
    .. response-headers = "ok"
        .. suffix = "!"
```

```brix
(main
  (probe-log http.response.headers (items: response-headers)
    (block
      (lazy response-headers
        (scope "ok"
          (lazy suffix "!")
        )
      )
    )
  )
)
```

```output
<Block>
    <Probe key="http.response.headers">
        <Id name="response-headers"/>
    </Probe>
    <Declaration lazy="true">
        <TypeCapture>
            <Id name="response-headers"/>
        </TypeCapture>
        <Block>
            <Declaration lazy="true">
                <TypeCapture>
                    <Id name="suffix"/>
                </TypeCapture>
                <String text="!"/>
            </Declaration>
            <Declaration lazy="true">
                <TypeCapture>
                    <Id name="it"/>
                </TypeCapture>
                <String text="ok"/>
            </Declaration>
            <Id name="it"/>
        </Block>
    </Declaration>
</Block>
```

```yumi
//channel:write -> detail, task
    .. detail = "ok"
```

```brix
(main
  (probe-log (: channel write) (items: detail task)
    (block
      (lazy detail "ok")
    )
  )
)
```

```output
<Block>
    <Probe object="channel" key="write">
        <Id name="detail"/>
        <Id name="task"/>
    </Probe>
    <Declaration lazy="true">
        <TypeCapture>
            <Id name="detail"/>
        </TypeCapture>
        <String text="ok"/>
    </Declaration>
</Block>
```

```yumi
//http.bad -> x
    . x = "nope"
```

```brix
(main
  (probe-log http.bad (items: x)
    (block
      (let x "nope")
    )
  )
)
```

```output
<Block>
    <Probe key="http.bad">
        <Id name="x"/>
    </Probe>
    <Declaration>
        <TypeCapture>
            <Id name="x"/>
        </TypeCapture>
        <String text="nope"/>
    </Declaration>
</Block>
```

```yumi
//http// bad $x
```

```brix
(main
  (probe-log http (text: "bad " x))
)
```

```output
<Probe key="http">
    <String text="bad "/>
    <Id name="x"/>
</Probe>
```

```yumi
//!stream-pattern do foo(1)
```

```brix
(main
  (probe stream-pattern
    (do
      (inline (call foo 1))
    )
  )
)
```

```output
<ProbeStatement key="stream-pattern">
    <Do>
        <CallExpression>
            <Id name="foo"/>
            <Argument>
                <Integer value="1"/>
            </Argument>
        </CallExpression>
    </Do>
</ProbeStatement>
```

```yumi
//stream-pattern.reads ++
//this:stream-pattern.object-reads ++
//stream-pattern.read-tokens += size
//this:stream-pattern.object-tokens += size
```

```brix
(main
  (probe-count stream-pattern.reads)
  (probe-count (: this stream-pattern.object-reads))
  (probe-count stream-pattern.read-tokens size)
  (probe-count (: this stream-pattern.object-tokens) size)
)
```

```output
<ProbeCounter key="stream-pattern.reads"/>
<ProbeCounter key="stream-pattern.object-reads" object="this"/>
<ProbeCounter key="stream-pattern.read-tokens">
    <Id name="size"/>
</ProbeCounter>
<ProbeCounter key="stream-pattern.object-tokens" object="this">
    <Id name="size"/>
</ProbeCounter>
```

```yumi
//!gui.frame.objects . start0 = 5
```

```brix
(main
  (probe gui.frame.objects (let start0 5))
)
```

```output
<ProbeStatement key="gui.frame.objects">
    <Declaration>
        <TypeCapture>
            <Id name="start0"/>
        </TypeCapture>
        <Integer value="5"/>
    </Declaration>
</ProbeStatement>
```
