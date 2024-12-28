# Notes on `a-option` Test Case and OS-Specific `strcoll()` Collation Behavior

**Author**: Antigravity (AI Pair Programmer)

## Overview

The `a-option` test case verifies the behavior of the `-a` option in `tree`, which includes hidden files (files starting with a dot `.`, such as `.d`, `.e`, `.l`).

When sorting file names using `alnumsort()` (which invokes standard C library `strcoll()`), the output order of hidden files differs between operating systems even under UTF-8 locales such as `en_US.UTF-8`.

## Technical Background

`tree` uses `strcoll(name1, name2)` from `<string.h>` for alphabetical sorting (`alnumsort()`).

In a UTF-8 locale (e.g., `en_US.UTF-8`), the collation weight assigned to a leading dot (`.`) depends on the underlying C library implementation:

- **Linux (glibc)**: Implements Unicode Collation Algorithm (UCA) primary weights where leading punctuation marks like `.` are weighted lower or ignored during primary comparison. Therefore, `a` and `b` come before `.e` (`a` -> `b` -> `.e` -> `h`).
- **macOS (Darwin / BSD libc)**: Uses a collation table where punctuation marks like `.` (ASCII 46) are ordered before alphabetical characters like `a` (ASCII 97). Therefore, `.e` comes before `a` (`.e` -> `a` -> `b` -> `h`).

### Comparison Matrix for `a-option` Output

| OS / C Library | Locale | Collation Order for `a` vs `.e` | Sort Result for Root Directory |
| :--- | :--- | :--- | :--- |
| **Linux (glibc)** | `en_US.UTF-8` | `a` < `.e` | `a` -> `b` -> `.e` -> `h` |
| **macOS (Darwin libc)** | `en_US.UTF-8` | `.e` < `a` | `.e` -> `a` -> `b` -> `h` |
| **All OSes (POSIX)** | `C` / `POSIX` | `.e` < `a` (ASCII 46 < 97) | `.e` -> `a` -> `b` -> `h` |

## Resolution Strategy

To achieve deterministic and reproducible test output across both Linux and macOS, the `a-option` test uses `LC_ALL=C` specifically for this test case. Under `LC_ALL=C`, ASCII byte comparison (ASCII 46 for `.` < ASCII 97 for `a`) is strictly enforced across all operating systems, producing identical output on both Linux and macOS.
