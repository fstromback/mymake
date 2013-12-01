#!/bin/bash

echo "Downloading latest version using git..."
git fetch
git checkout master -f
git merge origin/master

# Compile mymake once again.
if [ -e mymake_new ]
then
    rm mymake_new
fi

echo "Compiling mymake once more..."
g++ *.cpp -o mymake_new

if [ -e mymake_new ]
then
    echo "Compilation succeeded, mymake is updated!"
    rm mymake
    mv mymake_new mymake
else
    echo "Compilation failed!"
    exit 1
fi
