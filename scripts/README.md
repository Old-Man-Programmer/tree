# Development & Maintenance Scripts

## Verify Man Page

Verify that the embedded man page matches the original groff-formatted output:

```bash
python3 scripts/verify_man_page.py
```

This script:
- Extracts the original `doc/tree.1` from git history
- Generates the expected groff-formatted output
- Compares it with the current `tree man` output
- Reports whether they match byte-for-byte

## Regenerate Man Page

If you need to regenerate the embedded man page from the original source:

```bash
# First, extract the original man page from git
git show d501b58ff9cbfd64272c8cbcad0bda36a3fada06:doc/tree.1 > /tmp/original_tree.1

# Generate formatted output and compare
python3 scripts/generate_full_man.py

# This will show differences and save comparison files to /tmp/
```

The script creates:
- `/tmp/original_formatted.txt` - Expected formatted man page
- `/tmp/current_formatted.txt` - Current `tree man` output
- Line-by-line comparison showing where content differs

## Sync Man Page with Original

To update `src/man.zig` with the complete formatted man page:

```bash
# Extract original man page
git show d501b58ff9cbfd64272c8cbcad0bda36a3fada06:doc/tree.1 > /tmp/original_tree.1

# Generate formatted output
groff -man -Tutf8 /tmp/original_tree.1 > /tmp/original_formatted.txt

# Convert to hex and update src/man.zig
python3 << 'EOF'
with open('/tmp/original_formatted.txt', 'rb') as f:
    content = f.read()

hex_bytes = []
for i in range(0, len(content), 12):
    chunk = content[i:i+12]
    hex_chunk = ', '.join(f'0x{b:02x}' for b in chunk)
    hex_bytes.append('    ' + hex_chunk)

hex_content = ',\n'.join(hex_bytes)

man_zig = f'''const std = @import("std");

pub fn printManPage() !void {{
    var stdout_buffer: [4096]u8 = undefined;
    var stdout_writer = std.fs.File.stdout().writer(&stdout_buffer);
    const stdout = &stdout_writer.interface;

    try stdout.writeAll(man_content);
    try stdout.flush();
}}

const man_content: []const u8 = &[_]u8{{
{hex_content}
}};
'''

with open('src/man.zig', 'w') as f:
    f.write(man_zig)

print(f"Updated src/man.zig with {len(content)} bytes")
EOF

# Verify it worked
python3 scripts/verify_man_page.py
```
