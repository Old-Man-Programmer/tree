#!/usr/bin/env bash
# -*- coding: utf-8 -*-

home=$(cd $(dirname $0) && pwd)
source ${home}/../initialize.bash

# See README.md of this test case about the reason of overriding
# LC_ALL here.
export LC_ALL=C

mkdir -p b/.d .e/g h/j
touch a b/c .e/f h/{i,j/k,j/.l}
${tree} -a > ${home}/actual/stdout

assert ${home}/{expected,actual}/stdout

fin
