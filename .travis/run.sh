#!/bin/bash

FLAGS="-Werror"

if [ $CC = "clang" ]; then
	FLAGS="$FLAGS -Wno-error=unused-command-line-argument"
fi

./autogen.sh && ./configure && make CFLAGS="$FLAGS" && make distcheck
