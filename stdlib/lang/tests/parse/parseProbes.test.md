# Probes

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

```brix
(main
  (probe-group - probe.group)
)
```

```output
<ProbeGroup end="true" key="probe.group" data="true"/>
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

```brix
(main
  (probe grammar
    (do
      (inline (call foo 1))
    )
  )
)
```

```output
<ProbeStatement key="grammar">
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

```brix
(main
  (probe-count grammar.reads)
  (probe-count (: this grammar.object-reads))
  (probe-count grammar.read-tokens size)
  (probe-count (: this grammar.object-tokens) size)
)
```

```output
<ProbeCounter key="grammar.reads"/>
<ProbeCounter key="grammar.object-reads" object="this"/>
<ProbeCounter key="grammar.read-tokens">
    <Id name="size"/>
</ProbeCounter>
<ProbeCounter key="grammar.object-tokens" object="this">
    <Id name="size"/>
</ProbeCounter>
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
