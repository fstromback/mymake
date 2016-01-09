#!/bin/bash

echo "Downloading latest version using git..."
git fetch
git checkout master -f
git merge origin/master

if [ ! -e mymake_v2 ]
then
    echo "Warning: Updating to version 2 of mymake, which is incompatible with the previous release."
    echo "Please see README.md for details before upgrading. Run 'update.sh' once more to continue."
    touch mymake_v2
    exit 1
fi

# Compile mymake once again.
if [ -e mymake_new ]
then
    options=$1
fi

echo "Compiling mymake once more..."
./compile.sh mymake_new

if [ -e mymake_new ]
then
    rm mymake
    mv mymake_new mymake
    echo "Compilation succeeded, mymake is updated!"
else
    echo "Compilation failed!"
    exit 1
fi
