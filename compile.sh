#!/bin/sh

echo "Compiling mymake..."
if [ -e mymake ]
then
    rm mymake
fi

mkdir -p bin
g++ textinc/*.cpp -o bin/textinc
bin/textinc bin/templates.h templates/*.txt
g++ -O3 src/*.cpp -o $1
