# Introduce a test framework

*Thd document is derived from [the original merge request]
(https://gitlab.com/OldManProgrammer/unix-tree/-/merge_requests/19)
submitted by Kenta Arai. Masatake YAMATO converted to the Markdown format.*

## Background

Additional bugs might be added when new features or patches are
merged. In my opinion, the most fatal cause is no easy methods to
compare the behavior of old and new tree. Some OSS projects contain
the methods (e.g. coreutils).

## How to test tree

Run `make check`.
The command runs test cases under `tests` directory.

```terminal
$ make check
cc -O3 -std=c11 -Wpedantic -Wall -Wextra -Wstrict-prototypes -Wshadow -Wconversion -DLARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c -o tree.o tree.c
cc -O3 -std=c11 -Wpedantic -Wall -Wextra -Wstrict-prototypes -Wshadow -Wconversion -DLARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c -o list.o list.c
cc -O3 -std=c11 -Wpedantic -Wall -Wextra -Wstrict-prototypes -Wshadow -Wconversion -DLARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c -o hash.o hash.c
cc -O3 -std=c11 -Wpedantic -Wall -Wextra -Wstrict-prototypes -Wshadow -Wconversion -DLARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c -o color.o color.c
cc -O3 -std=c11 -Wpedantic -Wall -Wextra -Wstrict-prototypes -Wshadow -Wconversion -DLARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c -o file.o file.c
cc -O3 -std=c11 -Wpedantic -Wall -Wextra -Wstrict-prototypes -Wshadow -Wconversion -DLARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c -o filter.o filter.c
cc -O3 -std=c11 -Wpedantic -Wall -Wextra -Wstrict-prototypes -Wshadow -Wconversion -DLARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c -o info.o info.c
cc -O3 -std=c11 -Wpedantic -Wall -Wextra -Wstrict-prototypes -Wshadow -Wconversion -DLARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c -o unix.o unix.c
cc -O3 -std=c11 -Wpedantic -Wall -Wextra -Wstrict-prototypes -Wshadow -Wconversion -DLARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c -o xml.o xml.c
cc -O3 -std=c11 -Wpedantic -Wall -Wextra -Wstrict-prototypes -Wshadow -Wconversion -DLARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c -o json.o json.c
cc -O3 -std=c11 -Wpedantic -Wall -Wextra -Wstrict-prototypes -Wshadow -Wconversion -DLARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c -o html.o html.c
cc -O3 -std=c11 -Wpedantic -Wall -Wextra -Wstrict-prototypes -Wshadow -Wconversion -DLARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c -o strverscmp.o strverscmp.c
cc -s -o tree tree.o list.o hash.o color.o file.o filter.o info.o unix.o xml.o json.o html.o strverscmp.o
make -C tests
make[1]: Entering directory '/home/kenta/Git/Kenta111/unix-tree/tests'
***** tests are launched ******
if [ -d /tmp/tree ]; then rm -rf /tmp/tree; fi
mkdir -p /tmp/tree
Test: no-arg
***** tests were finished: 0 ******
make[1]: Leaving directory '/home/kenta/Git/Kenta111/unix-tree/tests'
```

If a test case fail, you will see:
```terminal
$ make check
make -C tests
make[1]: Entering directory '/home/yamato/var/tree/tests'
***** tests are launched ******
if [ -d /tmp/tree ]; then rm -rf /tmp/tree; fi
mkdir -p /tmp/tree
Test: no-arg
/home/yamato/var/tree/tests/no-arg/expected/stdout /home/yamato/var/tree/tests/no-arg/actual/stdout differ: byte 45, line 4
FAILURE: /home/yamato/var/tree/tests/no-arg/expected/stdout /home/yamato/var/tree/tests/no-arg/actual/stdout
--- /home/yamato/var/tree/tests/no-arg/expected/stdout  2026-07-26 05:10:55.593200288 +0900
+++ /home/yamato/var/tree/tests/no-arg/actual/stdout    2026-07-26 05:10:57.735247380 +0900
@@ -1,7 +1,7 @@
 .
 ├── a
 ├── b
-│   └── X
+│   └── c
 └── h
     ├── i
     └── j
***** tests were finished: 1 ******
make[1]: *** [Makefile:13: all] Error 1
make[1]: Leaving directory '/home/yamato/var/tree/tests'
make: *** [Makefile:139: check] Error 2
```

## How to add a test

First of all, add test entry in `tests/Makefile`.

```Makefile
entries = no-arg new-test
```

Second, You must create a new test directory. `test.bash` under the
directory is run as a test case. As the file name shown, you must
use bash for implementing a test case.

It is helpful to reuse the existing directory.

```terminal
$ cp -r tests/{no-arg,new-test} # reuse the no-arg test
```

Finally, overwrite `tests/new-test/test.bash` in order to test tree behavior.
Then, run make check and check results!

## Note
The commit does not have enough entries to test all tree functions.
Although I will add some entries, I am not familiar with some options.
Significant efforts from other contributors are required.
