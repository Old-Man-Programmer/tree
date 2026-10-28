#!/bin/sh
#
# Tests for the interaction of --du (accumulate the size of a directory from
# its contents) and -L (limit the depth of the displayed listing).
#
# The size reported for a directory that is displayed must account for
# everything below it, even when -L keeps those entries from being printed,
# while the listing itself must stay limited to the requested depth.
#
# Build tree first ("make"), then run with "make check", or directly:
#   sh tests/test_du.sh
# A different binary may be given with:
#   TREE=/path/to/tree sh tests/test_du.sh

set -u

TREE=${TREE:-}
if [ -z "$TREE" ]; then
  TREE=$(dirname "$0")/../tree
fi

if [ ! -x "$TREE" ]; then
  echo "$0: '$TREE' is not an executable; run 'make' first or set TREE." >&2
  exit 1
fi

WORK=$(mktemp -d 2>/dev/null) || WORK=/tmp/tree-du-test.$$
rm -rf "$WORK"
mkdir -p "$WORK/data/a/b/c" "$WORK/data/a/empty" "$WORK/data/a/.hid" \
         "$WORK/data/x/y" || exit 1

dd if=/dev/zero of="$WORK/data/a/b/c/deep.txt" bs=1024 count=100 2>/dev/null
dd if=/dev/zero of="$WORK/data/a/b/mid.txt" bs=1024 count=10 2>/dev/null
dd if=/dev/zero of="$WORK/data/a/top.txt" bs=1024 count=50 2>/dev/null
dd if=/dev/zero of="$WORK/data/a/.hid/h.txt" bs=1024 count=7 2>/dev/null
dd if=/dev/zero of="$WORK/data/x/y/z.txt" bs=1024 count=33 2>/dev/null
dd if=/dev/zero of="$WORK/data/plain.txt" bs=1024 count=1 2>/dev/null

DATA=$WORK/data
failures=0

fail() {
  echo "FAIL: $1"
  failures=$((failures + 1))
}

pass() {
  echo "ok: $1"
}

# Turn sized output (tree -s --du -f) into "<size>TAB<path>" lines.
sizes() {
  awk '{
    i = index($0, "[")
    j = index($0, "]")
    if (i == 0 || j == 0) next
    size = substr($0, i + 1, j - i - 1) + 0
    path = substr($0, j + 1)
    sub(/^[ \t]+/, "", path)
    if (path == "") next
    printf "%d\t%s\n", size, path
  }'
}

# Pull the "N bytes used ..." total out of the report line.
total() {
  awk '/ bytes used/ { print $1 + 0; }'
}

# Compare the sizes of every directory that is displayed with -L against the
# sizes reported for the same directories by an unlimited --du run.
check_sizes() {
  label=$1
  shift

  "$TREE" --du -f --noreport "$@" "$DATA" >"$WORK/unlimited.out" 2>&1 || {
    fail "$label: unlimited run failed"
    return
  }
  sizes <"$WORK/unlimited.out" >"$WORK/unlimited.tsv"

  level=1
  while [ $level -le 4 ]; do
    "$TREE" --du -f --noreport -L $level "$@" "$DATA" >"$WORK/limited.out" 2>&1 || {
      fail "$label -L $level: limited run failed"
      return
    }
    sizes <"$WORK/limited.out" >"$WORK/limited.tsv"

    mismatch=$(awk -F '\t' '
      NR == FNR { unlimited[$2] = $1; next }
      ($2 in unlimited) && unlimited[$2] != $1 {
        printf "%s: -L prints %s, full tree is %s\n", $2, $1, unlimited[$2]
      }
    ' "$WORK/unlimited.tsv" "$WORK/limited.tsv")

    if [ -n "$mismatch" ]; then
      fail "$label -L $level: sizes of displayed directories differ from --du without -L"
      echo "$mismatch" | sed 's/^/    /'
    else
      pass "$label -L $level: sizes match the full --du run"
    fi
    level=$((level + 1))
  done
}

# The byte total of the report must not depend on the display level either.
check_total() {
  label=$1
  shift

  full=$("$TREE" --du "$@" "$DATA" 2>&1 | total)
  level=1
  while [ $level -le 4 ]; do
    limited=$("$TREE" --du -L $level "$@" "$DATA" 2>&1 | total)
    if [ "$limited" != "$full" ]; then
      fail "$label -L $level: report total is $limited, expected $full"
    else
      pass "$label -L $level: report total $limited"
    fi
    level=$((level + 1))
  done
}

# -L must still limit what is printed -- the fix may not print deeper entries.
check_depth() {
  level=$1

  "$TREE" -f --noreport -L $level "$DATA" >"$WORK/plain.out" 2>&1
  "$TREE" --du -f --noreport -L $level "$DATA" >"$WORK/du.out" 2>&1

  awk '{ i = index($0, "/"); if (i) print substr($0, i) }' <"$WORK/plain.out" >"$WORK/plain.paths"
  awk '{
    i = index($0, "]")
    if (i) { s = substr($0, i + 1); j = index(s, "/"); if (j) print substr(s, j) }
  }' <"$WORK/du.out" >"$WORK/du.paths"

  if diff -q "$WORK/plain.paths" "$WORK/du.paths" >/dev/null 2>&1; then
    pass "-L $level: --du displays the same entries as without --du"
  else
    fail "-L $level: --du changed which entries are displayed"
    diff "$WORK/plain.paths" "$WORK/du.paths" | sed 's/^/    /'
  fi

  if grep -q 'deep.txt' "$WORK/du.paths"; then
    fail "-L $level: deep.txt should not be displayed"
  else
    pass "-L $level: contents below the level limit stay hidden"
  fi
}

# 1. The reported size of a directory is independent of -L.
check_sizes "--du"
check_sizes "--du -a" -a
check_sizes "--du --prune" --prune
check_sizes "--du -a --prune" -a --prune

# 2. Same for the grand total in the report line.
check_total "--du"
check_total "--du -a" -a

# 3. The listing itself is still truncated by -L.
check_depth 1
check_depth 2
check_depth 3

# 4. The size of a directory whose contents are entirely hidden by -L must
#    equal the size reported for it by a full --du run (this is the exact
#    regression from issue #44: it used to be just the directory entry size).
"$TREE" --du -f --noreport "$DATA" >"$WORK/all.out" 2>&1
"$TREE" --du -f --noreport -L 1 "$DATA" >"$WORK/one.out" 2>&1
full_a=$(sizes <"$WORK/all.out" | awk -F '\t' -v p="$DATA/a" '$2 == p { print $1 }')
hidden_a=$(sizes <"$WORK/one.out" | awk -F '\t' -v p="$DATA/a" '$2 == p { print $1 }')
if [ -z "$full_a" ] || [ -z "$hidden_a" ]; then
  fail "could not find the size of $DATA/a in the output"
elif [ "$full_a" != "$hidden_a" ]; then
  fail "data/a with -L 1 reports $hidden_a bytes, expected $full_a"
else
  pass "data/a reports $full_a bytes with -L 1 (its contents are not displayed)"
fi

echo
if [ $failures -eq 0 ]; then
  echo "All --du/-L tests passed."
  rm -rf "$WORK"
  exit 0
fi

echo "$failures test(s) failed (output kept in $WORK)."
exit 1
