#!/usr/bin/env bash
# -*- coding: utf-8 -*-

home=$(cd $(dirname $0) && pwd)
source ${home}/../initialize.bash

mkdir -p b/.d .e/g h/j
touch a b/c .e/f h/{i,j/k,j/.l}
${tree} -L 2 -R > ${home}/actual/stdout

assert ${home}/{expected,actual}/stdout
assert ${home}/expected/h/j/00Tree.html ${target_directory}/h/j/00Tree.html

fin
