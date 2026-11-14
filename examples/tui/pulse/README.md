# Pulse

A live macOS workstation dashboard written entirely in Brix. Cyan and violet area charts, a
braille memory ring, gradient meters, and a process inspector sit in a responsive terminal layout.
Charts glide continuously between samples, with fractional scrolling and eased network scales.

Run from the repository root:

```sh
bin/yumi examples/tui/pulse/pulse-app.bx -o .tmp/build/pulse
```

For the populated, animated showcase:

```sh
bin/yumi examples/tui/pulse/pulse-app.bx -o .tmp/build/pulse -- --demo
```

The full layout appears at 108 columns × 34 rows. A compact layout fits 64 × 22; 132 × 42 gives
the charts and process list more room. Use a terminal with RGB backgrounds and Unicode support.

| Key | Action |
| --- | --- |
| `↑` / `↓`, `j` / `k` | Select a process; selection follows its PID across updates |
| `Page Up` / `Page Down` | Move a page through the process list |
| `Home` / `End` | Select the first / last process |
| `/` | Focus the process filter; typing narrows the list immediately |
| `Enter` | Leave the filter, or toggle the selected process's inspector |
| `Esc` | Leave the filter, close help or the inspector, then clear the filter |
| `c` / `m` | Sort descending by CPU or resident memory |
| `Space` | Freeze or resume displayed measurements |
| `d` | Switch between live and simulated data, clearing history |
| `a` | Toggle smooth chart motion and the animated heartbeat |
| `?` | Open or close help |
| `q` | Quit |

Click a process to select it. Click a column heading to cycle through ascending, descending, and
unsorted order. The header stays visible while the process list scrolls. The app only reads system
state; selecting a process sends no signal.

Live collection uses macOS's `top`, `ps`, `sysctl`, `netstat`, `hostname`, and `df`. It samples CPU
over an interval and refreshes approximately every two to three seconds. Process CPU is the
average reported by `ps` and can exceed 100% for a process using multiple cores. Memory is the
rounded physical usage reported by `top`, with the installed total from `sysctl`; it is not a
memory-pressure score. Network rates use measured counter deltas from physical `en*` interfaces,
excluding loopback and VPN interfaces. Each network graph scales independently. Storage shows
used and available space on the current working directory's volume.

Live history starts empty and retains 120 measurements. Missing rates are marked as measuring;
collector failures are shown while the last valid sample remains visible. Pausing freezes the
charts and measurements while the sampler continues. Animation uses the TUI frame scheduler,
interpolating known samples with about one sample of visual delay; numeric readings show the
latest measurement immediately. `MotionChannel` with `follow` smooths the network scales. The clock
and demo sampling use a separate timer, so pausing animation does not stop timekeeping. Motion stops at the newest known value if collection stalls.
Turning motion off shows each measurement immediately, with no interpolation over time.
Demo mode is explicitly labeled and uses deterministic
simulated workloads, with a populated history on entry. Quitting stops the sampler and its active
child process.

The workspace uses `GroupPanel` and `StatusBar`; footer hints come from the active `Shortcut`
nodes. `Spinner` supplies the heartbeat, and the gradient meters customize the standard
`ProgressBar` track. `Table'PulseProcess` owns columns, numeric sorting, selection, keyboard navigation, paging,
and scrolling. Its `cell` slot exposes `TableCell` state: `item`, `column`, `is-selected`,
`column-width`, and aligned `text`. Pulse supplies colored labels and a `PulseMeter` through that
slot; `TableHeaderRow` supplies the pinned, sortable header. Keyed rows preserve the selected process
across samples and sorting. Charts and the braille ring remain custom painters.

- `pulse-app.bx`: declarative layout, reactive state, shortcuts, and lifecycle.
- `pulse-processes.bx`: table columns, rich cell content, and initial selection policy.
- `pulse-data.bx`: live collector, parsers, deterministic demo, and formatting.
- `pulse-visuals.bx`: palette, theme, chart, meter, and ring painters.
