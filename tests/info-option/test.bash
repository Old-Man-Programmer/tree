#!/usr/bin/env bash
# -*- coding: utf-8 -*-

home=$(cd $(dirname $0) && pwd)
source ${home}/../initialize.bash

mkdir -p b/.d .e/g h/j
touch a b/c .e/f h/{i,j/k,j/.l}
echo -e "a
\tsample file a
\tThis is a sample file
b
\tsample directory b
\tThis is a sample directory
b/c
\tsample file b/c
h
\tsample directory h" > .info
${tree} --info > ${home}/actual/stdout

assert ${home}/{expected,actual}/stdout

fin
