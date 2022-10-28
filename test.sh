#!/bin/bash

TEST_DIR="test-cases"

RESET="\033[0m"
GREEN="\033[38;2;0;255;0m"
RED="\033[38;2;255;0;0m"

for file in $(ls $TEST_DIR); do
    printf "Running $file..."

    ./build/l-compiler < $TEST_DIR/$file &> /dev/null

    # Make sure the compiler exit with non zero status.
    if [ "$?" -ne 0 ]; then
        printf "$GREEN Ok"
    else
        printf "$RED Error"
    fi

    printf "$RESET.\n"
done
