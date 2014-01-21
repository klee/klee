#!/bin/sh
# Deprecated, use 'llvm-lit'.

echo "warning: '$0' is deprecated, use 'llvm-lit' instead."
# FIXME: Make test suite work in parallel.
exec llvm-lit --threads=1 "$@"
