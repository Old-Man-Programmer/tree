#!/usr/bin/env bash
# -*- coding: utf-8 -*-

home=$(cd $(dirname $0) && pwd)
source ${home}/../initialize.bash

mkdir -p b/.d .e/g h/j
touch a b/c .e/b h/{b,j/b,j/.l}
${tree} -P b --matchdirs > ${home}/actual/stdout

assert ${home}/{expected,actual}/stdout

fin
