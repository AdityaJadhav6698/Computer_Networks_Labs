#!/bin/bash

FILE="$1"

if [ -z "$FILE" ]; then
    echo "Please provide a C file."
    exit 1
fi

DIR="$(dirname "$FILE")"
BASE="$(basename "$FILE" .c)"

cd "$DIR" || exit 1

gcc "$BASE.c" -o "$BASE" || exit 1

echo "Running $BASE..."
"./$BASE"