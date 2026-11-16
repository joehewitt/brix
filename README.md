Yumi
=======

Yumi is a statically typed, natively compiled language with garbage collection. It brings
together features usually found in different kinds of language: type inference, classes and
pattern matching, lazy expressions and imperative procedures, stream pipelines and grammars,
tasks and channels, declarative markup, and a direct bridge to C.

```yumi
-- summary([2, 3, 4, 5])

━━━━━━━━━━━━━━━━━━━━
+ summary
    - values [Int]

"Squares of odd values: $text"

.. text = ", ".join(squares)
.. squares = each * each in values when each % 2 != 0
```

Yumi often puts the answer first and the work below it. The body of `summary` is the text it
returns, the lazy bindings under it say how that text is made, and the program prints
`Squares of odd values: 9, 25`.

Yumi is not production software yet. The language, the compiler and the standard library still
change from release to release.

Installing
----------

Yumi runs on Apple silicon Macs with macOS 15 or later, and on aarch64 Linux with glibc 2.34 or
later on an Armv8.2 processor or newer, such as AWS Graviton2 and later. To install it, run:

```bash
curl -fsSL https://brix-lang.com/install.py | python3 -
```

The installer checks the machine, downloads the compiler, the standard library and the tools
they need into `~/.yumi`, compiles a small program to check the installation, and offers to put
`~/.yumi/bin` on your `PATH`.

- On a Mac, install Apple's command line tools first, with `xcode-select --install`. Yumi links
  programs with them, and macOS's `python3` needs them too.
- On Linux, nothing else is needed: the installer brings its own linker, C headers and libclang.
- Add options after `python3 -`: `--version 2026.11.03-1` installs a particular release,
  `--yes` answers every question with its default, and `--no-modify-path` leaves your shell's
  startup files alone. `--help` lists the rest.

While the release bucket is private, the installer can download only on a machine set up with
the `yumi` AWS profile.

A first program
---------------

Save this as `hello.yumi`:

```yumi
-- "Hello from Yumi"
```

Then build it and run it:

```bash
yumi hello.yumi -o hello
```

`yumi hello.yumi` alone checks the program without building it, `--compile` builds it without
running it, and `yumi --help` lists every option.

Building from source
--------------------

The installer can also clone this repository at the release it installs and build the compiler
from source:

```bash
curl -fsSL https://brix-lang.com/install.py | python3 - --source
```

The compiler is written in Yumi, so it is built by an earlier release of itself, the seed that
`.release.json` pins. A release's tagged commit pins that release, so a source build needs
only the release and the few prebuilt libraries the installer fetches with it, and it gives a
compiler identical to the released one. In a clone, `bin/yumi` builds the compiler and the C
runtime when they are out of date and then runs it, taking the same arguments as `yumi`, and
`yumi` itself runs the clone's `bin/yumi` once you trust the folder. A repository that keeps a
clone in a folder of its own can make its `.release.json` a link to the clone's, so that `yumi`
runs the clone's `bin/yumi` anywhere in the repository. A Mac needs Apple's command line tools for
this, and Linux needs the distribution's `clang`, version 16 or later.

Releases are cut from `main` with `toolchain/release/yumi-release`. A release builds the compiler
three times on a Mac and checks that the last two builds agree, builds it for Linux, runs the test
suites, and checks the packages in a sandbox on the Mac and in a new Linux VM. In the VM, the Linux
compiler must build itself, and the macOS compiler from the C headers the Mac read, into the same
objects the Mac built.

The downloads the website offers are packaged separately, when wanted, with
`toolchain/release/brix-downloads`. `brix-downloads editor` builds Brix Editor, puts a toolchain
release inside it so that it runs programs without an installed `yumi`, signs it, and makes a
disk image that opens to the app beside a link to Applications. With a Developer ID identity and
`--notary-profile`, it notarizes and staples the image too.

The repository
--------------

| Directory | Holds |
| --- | --- |
| `stdlib` | the standard library, including the compiler itself in `stdlib/lang` |
| `compiler` | the compiler's command line, its build daemon, and the test, lint, probe, bench and Brix tools |
| `libs` | the C and C++ runtime: the garbage collector, the event loop, tasks, and system calls |
| `toolchain` | the scripts that build the runtime, the compiler and releases, and in `toolchain/deps` the recipes for the third-party libraries and tools that builds use |
| `examples` | small programs, from servers to terminal and graphical apps |
| `editor` | Brix Editor, a small editor that reads, writes and runs Brix programs and other source |
| `docs` | the guides for writing Yumi and notes on the libraries; the language reference is in `docs/ref` |

Learning the language
---------------------

[The language reference](docs/ref/README.md) describes the whole language, a chapter for each part,
starting with [source text](docs/ref/source-text.md).

License
-------

Copyright 2025 Joe Hewitt

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
