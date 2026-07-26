#!/usr/bin/env bash
# -*- coding: utf-8 -*-

# The sort logic used in tree is affected by the current locale.
export LC_ALL=en_US.UTF-8
export TREE_CHARSET=UTF-8

if [[ -z ${home} ]]; then
    echo 'INTERNAL ERROR: ${home} is not set'
    caller
    exit 2
fi

tree="${home}"/../../tree
test_name=$(basename "${home}")
target_directory=/tmp/tree-"$(id -nu)"/"${test_name}"
failed=0

diff=:
if command -v diff >/dev/null 2>&1; then
    diff=diff
fi

if [[ -d ${home}/actual ]]; then
    rm -rf "${home}"/actual
fi
mkdir "${home}"/actual
if [[ -d ${target_directory} ]]; then
    rm -rf "${target_directory}"
fi
mkdir -p "${target_directory}"
cd "${target_directory}"

echo "Test: ${test_name}"

# @fn assert
# @brief Validate actual output with expected output
# @param ${1} The path of expected output
# @param ${2} The path of actual output
function assert {
    if ! cmp "${1}" "${2}"; then
        failed=1
        echo "FAILURE:" "${1}" "${2}"
        ${diff} -uN "${1}" "${2}"
    fi
}

# @fn fin
# @brief Report the test result as exit status
function fin {
    exit "${failed}"
}
