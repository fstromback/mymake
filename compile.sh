#!/bin/bash

if [ -e $1 ]
then
    rm $1
fi

if [ $2 = "ida" ]
then
    g++ *.cpp -o $1 -std=c++11 -L/sw/gcc-${GCC4_V}/lib -static-libstdc++
else
    g++ *.cpp -o $1
fi

if [ -e $1 ]
then
    echo "Done compiling!"
    exit 0
else
    echo ""
    echo "Compilation error!"
    echo "Try one of the special options:"
    echo "ida - when on the computer systems at IDA, LiU"
    exit 1
fi
