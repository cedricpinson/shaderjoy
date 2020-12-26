#!/bin/bash

SCRIPT_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CLANG_TIDY="${1}"
NUM_JOBS="${2}"
shift 2


CLANG_RUNNER=$(command -v /usr/local/opt/llvm/Toolchains/LLVM11.0.0.xctoolchain/usr/share/clang/run-clang-tidy.py || command -v /usr/bin/run-clang-tidy-11.py)
if [ $? -ne 0 ]
then
    echo "run-clang-tidy.py not found, it should be installed with clang-11" && false
fi

${CLANG_RUNNER} -clang-tidy-binary ${CLANG_TIDY} -config='' -p ./ -j ${NUM_JOBS} -quiet $@ >clang-tidy.log 2>/dev/null

if [ $? -ne 0 ]
then
    cat clang-tidy.log | grep -v -e '^$' | grep -v 'clang-tidy' && false
fi
