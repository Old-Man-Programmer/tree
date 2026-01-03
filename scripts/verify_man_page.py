#!/usr/bin/env python3
"""
Verification tool for tree man page
Compares 'tree man' output with original groff-formatted doc/tree.1
"""
import subprocess
import sys

def main():
    print("="*80)
    print("TREE MAN PAGE VERIFICATION")
    print("="*80)

    # Generate original from git
    print("\n1. Extracting original man page from git...")
    subprocess.run([
        'git', 'show', 'd501b58ff9cbfd64272c8cbcad0bda36a3fada06:doc/tree.1'
    ], stdout=open('/tmp/original_tree.1', 'wb'), check=True)
    print("   ✓ Extracted to /tmp/original_tree.1")

    # Generate formatted original
    print("\n2. Generating formatted original with groff...")
    result = subprocess.run(
        ['groff', '-man', '-Tutf8', '/tmp/original_tree.1'],
        capture_output=True,
        check=True
    )
    original = result.stdout
    with open('/tmp/expected_man.txt', 'wb') as f:
        f.write(original)
    print(f"   ✓ Generated {len(original)} bytes")

    # Get current tree man output
    print("\n3. Getting current 'tree man' output...")
    result = subprocess.run(
        ['zig', 'build', 'run', '--', 'man'],
        capture_output=True,
        check=True
    )
    current = result.stdout
    with open('/tmp/actual_man.txt', 'wb') as f:
        f.write(current)
    print(f"   ✓ Got {len(current)} bytes")

    # Compare
    print("\n4. Comparing outputs...")
    print(f"   Expected: {len(original):6} bytes, {original.count(b'\\n'):4} lines")
    print(f"   Actual:   {len(current):6} bytes, {current.count(b'\\n'):4} lines")

    if original == current:
        print("\n" + "="*80)
        print("✓ SUCCESS: Man page matches original exactly!")
        print("="*80)
        return 0
    else:
        print("\n" + "="*80)
        print("✗ FAILURE: Man page does not match!")
        print("="*80)

        # Find differences
        for i in range(min(len(original), len(current))):
            if original[i] != current[i]:
                print(f"\nFirst difference at byte {i}:")
                print(f"  Expected: {repr(original[max(0,i-30):i+30])}")
                print(f"  Actual:   {repr(current[max(0,i-30):i+30])}")
                break
        else:
            print(f"\nContent matches but length differs by {abs(len(original) - len(current))} bytes")

        print("\nSaved files:")
        print("  /tmp/expected_man.txt - Expected output")
        print("  /tmp/actual_man.txt   - Actual output")
        print("\nRun: diff /tmp/expected_man.txt /tmp/actual_man.txt")
        return 1

if __name__ == '__main__':
    sys.exit(main())
