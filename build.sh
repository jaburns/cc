#!/usr/bin/env bash
cd "$(dirname "$0")"
set -e

BASE='
    -DDEBUG=1
    -DTEST=0
    -Wall
    -Werror
    -Wswitch-enum
    -Wno-unused-function
    -Wno-logical-op-parentheses
    -fno-exceptions
    -fno-rtti
'

if grep -q '#define DEBUG \+1' src/main.cc; then
    EXTRA='
        -g
        -O0
        -fsanitize=undefined,unsigned-integer-overflow
        -fno-omit-frame-pointer
        -Wno-unused-variable
        -Wno-unused-but-set-variable
    '
else
    EXTRA='
        -g
        -O3
    '
fi

mkdir -p bin

build_deps() {
    clang -std=c11 -c src/vendor/vendor_impl.c -o bin/vendor.o
    clang -std=c++17 -x c++-header src/main_dep.hh -o src/main_dep.pch $BASE $EXTRA
}

build_main() {
    clang -std=c++17 -include-pch src/main_dep.pch bin/vendor.o src/main.cc -o bin/hello $BASE $EXTRA
}

build() {
    if ! build_main; then
        echo 'Rebuilding deps and trying again...'
        build_deps
        build_main
    fi
}

build