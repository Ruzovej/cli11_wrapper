#!/usr/bin/env bash

# assumes PWD being parent directory ... TODO polish later

set -e

scripts/build.bash \
    --target cli11_wrapper_unit_tests

input=(
    build/tests/unit/cli11_wrapper_unit_tests
    --no-intro=true
    --no-version=true
)

if [[ "$1" == "--gdb" ]]; then
    shift
    gdb --args "${input[@]}" "$@"
else
    time "${input[@]}" "$@"
fi
