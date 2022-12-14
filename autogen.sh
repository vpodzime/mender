#!/bin/sh
set -e

if [ ! -f "$(dirname "$0")/vendor/googletest/README.md" ] ; then
    echo "Please run the following to clone submodules first:"
    echo "  git submodule update --init"
    exit 1
fi

cd "$(dirname "$0")"
aclocal --install
autoreconf --install
