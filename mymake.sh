#!/bin/bash
#if ~/cpp/mymake/mymake -s ./${1}.cpp -o build-${1}/ -e ${1} -c c++
if ~/cpp/mymake/mymake -s ./${1}.cpp -o build/ -e ${1} -c c++
then
    ./$@
fi