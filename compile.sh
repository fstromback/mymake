#!/bin/sh

echo "Compiling mymake..."
if [ -e $1 ]
then
    rm $1
fi

mkdir -p bin
g++ textinc/*.cpp -o bin/textinc
bin/textinc bin/templates.h templates/*.txt
g++ -std=c++11 -O3 -g -iquotesrc/ src/*.cpp src/setup/*.cpp -lpthread -o $1
