#!/usr/bin/env python3
"""
Tool to generate and compare man page formatting
"""
import subprocess

def generate_groff_output(man_file):
    """Generate formatted man page using groff"""
    result = subprocess.run(
        ['groff', '-man', '-Tutf8', man_file],
        capture_output=True,
        check=True
    )
    return result.stdout

def get_current_tree_man():
    """Get output from 'tree man' command"""
    result = subprocess.run(
        ['zig', 'build', 'run', '--', 'man'],
        capture_output=True,
        check=True
    )
    return result.stdout

def save_outputs():
    """Generate and save both outputs"""
    print("Generating original formatted man page...")
    original = generate_groff_output('/tmp/original_tree.1')
    with open('/tmp/original_formatted.txt', 'wb') as f:
        f.write(original)
    print(f"Saved original: {len(original)} bytes")

    print("\nGetting current 'tree man' output...")
    current = get_current_tree_man()
    with open('/tmp/current_formatted.txt', 'wb') as f:
        f.write(current)
    print(f"Saved current: {len(current)} bytes")

    print("\nComparison:")
    print(f"  Original lines: {original.count(b'\\n')}")
    print(f"  Current lines:  {current.count(b'\\n')}")
    print(f"  Difference:     {original.count(b'\\n') - current.count(b'\\n')} lines missing")

    return original, current

def show_line_diff(original, current):
    """Show line-by-line differences"""
    orig_lines = original.decode('utf-8', errors='replace').split('\n')
    curr_lines = current.decode('utf-8', errors='replace').split('\n')

    print("\n" + "="*80)
    print("LINE-BY-LINE COMPARISON")
    print("="*80)

    max_lines = max(len(orig_lines), len(curr_lines))

    for i in range(min(10, max_lines)):  # Show first 10 lines
        print(f"\nLine {i+1}:")
        if i < len(orig_lines):
            print(f"  ORIG: {repr(orig_lines[i][:100])}")
        else:
            print(f"  ORIG: [MISSING]")

        if i < len(curr_lines):
            print(f"  CURR: {repr(curr_lines[i][:100])}")
        else:
            print(f"  CURR: [MISSING]")

    # Show where content diverges
    print("\n" + "="*80)
    print("FINDING WHERE CONTENT ENDS...")
    print("="*80)
    print(f"\nCurrent output ends at line {len(curr_lines)}")
    if len(curr_lines) < len(orig_lines):
        print(f"Original has {len(orig_lines) - len(curr_lines)} more lines")
        print(f"\nNext lines in original (after current ends):")
        for i in range(len(curr_lines), min(len(curr_lines) + 10, len(orig_lines))):
            print(f"  Line {i+1}: {repr(orig_lines[i][:80])}")

if __name__ == '__main__':
    original, current = save_outputs()
    show_line_diff(original, current)

    print("\n" + "="*80)
    print("FILES SAVED:")
    print("  /tmp/original_formatted.txt - Full formatted man page")
    print("  /tmp/current_formatted.txt  - Current 'tree man' output")
    print("="*80)
