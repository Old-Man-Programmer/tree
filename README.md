# Bo (Tree)

This is a UNIX utility to display a tree view of directories. The original tree (written in C) can be found [here](http://oldmanprogrammer.net/source.php?dir=projects/tree)

Bo (Tree) is Zig version of this UNIX utility where C implementation is moved to Zig implementation gradually to make cross-platform dead easy and improve the safety without performance loss. It is also an educational project to test drive Zig. Everyone is welcome here via PRs.

## Requirements

- Zig 0.15.2

## Build Your Own Binary

```bash
zig build # Debug build (default)
zig build -Doptimize=ReleaseSafe
zig build -Doptimize=ReleaseSmall
zig build -Doptimize=ReleaseFast
```

The compiled binary will be located at `zig-out/bin/tree` (or `zig-out/bin/colortree` on OpenBSD, `zig-out/bin/tree.exe` on Windows).

## Cross-Compilation

**Fully Supported:**

- Linux (x86_64, ARM64)
- macOS (x86_64, Apple Silicon)
- FreeBSD (x86_64, ARM64)
- Android (via NDK)

**Native Build Only:**

- OpenBSD (requires building on native machine due to Zig cross-compilation limitation)
- Solaris/Illumos (requires building on native machine due to Zig cross-compilation limitation)

**Requires POSIX Environment:**

- Windows (requires Cygwin or similar POSIX layer, not native Windows, working on it to make non-POSIX dependent with full Zig re-write)

**Not Supported Anymore:**

- HP/UX: Zig/LLVM does not target PA-RISC/IA64 with HP/UX ABI
- OS/2: Zig/LLVM does not target i386-os2
- HP NonStop: Zig/LLVM does not target NonStop OS

Zig makes it super easy to build for any platform from any platform:

```bash
zig build -Dtarget=x86_64-windows   # Build Windows binary (requires Cygwin)
zig build -Dtarget=aarch64-linux    # Build ARM64 Linux binary
zig build -Dtarget=x86_64-macos     # Build macOS binary
zig build -Dtarget=aarch64-macos    # Build Apple Silicon binary
zig build -Dtarget=x86_64-freebsd   # Build FreeBSD binary
```

## Running Without Installing

```bash
zig build run -- --version
zig build run -- -L 2
```

## Viewing Documentation

```bash
bo man # Display the manual page
```

## Old Todos

Should do:

- [ ] Use stdint.h and inttypes.h to standardize the int sizes and format strings.
  Not sure how cross-platform this would be.

- [ ] Add --DU option to fully report disk usage, taking into account files that
  are not displayed in the output.

- [ ] Make wide character support less of a hack.

- [ ] Fully support HTML colorization properly and allow for an external stylesheet.

- [ ] Might be nice to prune files by things like type, mode, access/modify/change
  time, uid/gid, etc. ala find.

- [ ] Just incorporate the stat structure into _info, since we now need most of
  the structure anyway.

- [ ] Move as many globals into a struct or structs to reduce namespace pollution.

Maybe do:

- [ ] With the addition of TREE_COLORS, add some custom options to perhaps colorize
  metadata like the permissions, date, username, etc, and change the color of
  the tree lines and so on.

- [ ] Refactor color.c.

- [ ] Output BSON on stddata instead of JSON.
