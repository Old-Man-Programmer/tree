#!/usr/bin/env bash
# -*- coding: utf-8 -*-

home=$(cd $(dirname $0) && pwd)
source ${home}/../initialize.bash

mkdir -p b/.d .e/g h/j
touch a b/c .e/f h/{i,j/k,j/.l}
if [ ! -h m ]; then ln -s b m; fi
if [ ! -h n ]; then ln -s .e n; fi
if [ ! -h o ]; then ln -s h o; fi
${tree} -l > ${home}/actual/stdout

assert ${home}/{expected,actual}/stdout

fin
