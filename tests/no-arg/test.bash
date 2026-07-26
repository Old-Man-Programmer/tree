#!/usr/bin/env bash
# -*- coding: utf-8 -*-

# Set home, the load initialize.bash.
# initialize.bash initializes the environment exectuing
# this test case.
# initialize.bash defines helper functions: assert() and fin().
home=$(cd $(dirname $0) && pwd)
source ${home}/../initialize.bash

mkdir -p b/.d .e/g h/j
touch a b/c .e/f h/{i,j/k,j/.l}

# Record the output of tree to files under ${home}/actual.
# Prepare expected output to ${home}/expected.
${tree} > ${home}/actual/stdout

# Use assert() to compares files under ${home}/expected ${home}/actual.
assert ${home}/{expected,actual}/stdout

# Call fin() at the enf of test case.
fin
