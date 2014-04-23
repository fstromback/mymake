#!/bin/bash

echo "Downloading latest version using git..."
git fetch
git checkout master -f
git merge origin/master

options="-"
if [ $# -eq 1 ]
then
    options=$1
fi

echo "Compiling mymake once more..."
if ./compile.sh mymake_new $options
then
    :
else
    exit 1
fi

if [ -e mymake_new ]
then
    rm mymake
    mv mymake_new mymake
    echo "Compilation succeeded, mymake is updated!"
else
    echo "Compilation failed!"
    exit 1
fi
