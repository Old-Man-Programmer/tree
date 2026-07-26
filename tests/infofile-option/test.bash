#!/usr/bin/env bash
# -*- coding: utf-8 -*-

home=$(cd $(dirname $0) && pwd)
source ${home}/../initialize.bash

mkdir -p binx bootx usrx
touch bootx/{a,b,c}

${tree} --infofile ${home}/_info > ${home}/actual/stdout

assert ${home}/{expected,actual}/stdout

fin
