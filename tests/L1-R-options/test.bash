#!/usr/bin/env bash
# -*- coding: utf-8 -*-

home=$(cd $(dirname $0) && pwd)
source ${home}/../initialize.bash

mkdir -p b/.d .e/g h/j
touch a b/c .e/f h/{i,j/k,j/.l}
${tree} -L 1 -R > ${home}/actual/stdout

assert ${home}/{expected,actual}/stdout
assert ${home}/expected/b/00Tree.html ${target_directory}/b/00Tree.html
assert ${home}/expected/h/00Tree.html ${target_directory}/h/00Tree.html
assert ${home}/expected/h/j/00Tree.html ${target_directory}/h/j/00Tree.html

fin
