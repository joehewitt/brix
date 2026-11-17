# Platforms

Brix builds programs for macOS, iOS and Linux. Most of the standard library is written in Brix and
builds for all three. Some modules call native libraries that come with the toolchain, and those
libraries are built only for some platforms. A program that imports such a module builds only for
the platforms its native library is built for.

| Native library | Used by | macOS | iOS | Linux |
| --- | --- | --- | --- | --- |
| Drawing: CoreGraphics, CoreText, ImageIO, Metal | `graphics`, and `gui` and everything that draws | yes | yes | no |
| Apple UI: AppKit, UIKit | `gui` | yes | yes | no |
| Audio: CoreAudio, AudioToolbox | `media` | yes | yes | no |
| OpenSSL | `net/socket` and the network modules above it, `crypto` | yes | no | yes |
| SQLite | `data/sqlite`, and the chat history in `ai` | yes | yes | yes |
| PCRE2 | `regex`, and find and replace in `code` | yes | yes | no |
| Tree-sitter | `code/syntax` | yes | no | no |
| Terminal emulation | `os/terminal-emulator` | yes | no | no |
| WebKit | `gui/web-view`, `gui/web-sign-in` | yes | no | no |
| AVFoundation | `graphics/movie` | yes | no | no |

- Most `graphics` modules draw through the drawing library. `/graphics/primitives`,
  `/graphics/point`, `/graphics/color` and `/graphics/matrix` do not, and build everywhere.
- `ai` uses `graphics`, so it builds where `graphics` does.
- `/os/mach-port` uses Mach ports, which only Apple's systems have.
