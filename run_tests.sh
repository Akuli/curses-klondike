#!/bin/bash
for file in tests/test_*.jou; do
    echo -n "Running $file: "
    jou "$@" $file || exit 1
done
