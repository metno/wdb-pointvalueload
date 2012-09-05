#!/bin/sh
autoreconf -if

if test -f "${PWD}/configure"; then
    echo "======================================"
    echo "You are ready to run './configure'"
    echo "======================================"
else
    echo "  Failed to generate ./configure script!"
fi
