#!/bin/bash

TEST_DIR="test-cases"
MUST_FAIL="$TEST_DIR/mf"
MUST_COMP="$TEST_DIR/mc"

RESET="\033[0m"
GREEN="\033[38;2;0;255;0m"
RED="\033[38;2;255;0;0m"

for file in $(ls $MUST_FAIL); do
    printf "Running $MUST_FAIL/$file..."

    ./build/l-compiler < $MUST_FAIL/$file &> /dev/null

    # Make sure the compiler exit with non zero status.
    if [ "$?" -ne 0 ]; then
        printf "$GREEN Ok"
    else
        printf "$RED Error"
    fi

    printf "$RESET.\n"
done

for file in $(ls $MUST_COMP); do
    printf "Running $MUST_COMP/$file..."

    ./build/l-compiler < $MUST_COMP/$file &> /dev/null

    # Make sure the compiler exit with non zero status.
    if [ "$?" -ne 0 ]; then
        printf "$GREEN Ok"
    else
        printf "$RED Error"
    fi

    printf "$RESET.\n"
done
