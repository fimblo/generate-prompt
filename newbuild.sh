#!/bin/bash
rm build/*o
echo Building...
gcc -Wall -Wextra -c src/git-status.c -o build/git-status.o
gcc -Wall -Wextra -c src/test-functions.c -o build/test-functions.o
gcc build/git-status.o build/test-functions.o -o build/test-functions -lgit2
echo run: build/test-functions
echo -------------------------
build/test-functions

