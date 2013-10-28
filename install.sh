#!/bin/sh

echo "Compiling mymake..."
if [ -e mymake ]
then
    rm mymake
fi

g++ *.cpp -o mymake

if [ -e mymake ]
then
    echo "Done compiling!"
else
    echo "Compilation error!"
    exit 1
fi


# set up aliases in shells?
while true
do
    echo "Do you wish to add alias for mymake to your shell? [Y/n]"
    read add_alias

    if [ "$add_alias" = "n" ]
    then
	exit 0
    elif [ "$add_alias" = "y" ]
    then
	break
    elif [ "$add_alias" = "" ]
    then
	break
    fi
done

echo "What would you like your alias to be? [default=mm]"
read alias_name

if [ "$alias_name" = "" ]
then
    alias_name="mm"
fi

file=`pwd`"/mymake"

go_on="yes"
while [ -n "$go_on" ]
do
    echo "Which shell should I add to?"
    echo "1: Bash"
    echo "2: csh"
    read shell_type

    go_on=""

    if [ $shell_type = 1 ]
    then
	echo "alias ${alias_name}='$file'" >> ~/.bashrc
	echo "Done! Now do source ~/.bashrc"
    elif [ $shell_type = 2 ]
    then
	echo "alias ${alias_name} \"$file\"" >> ~/.cshrc.private
	echo "Done! Now do source ~/.cshrc.private"
    else
	echo "I do not know about that shell..."
	go_on="yes"
    fi
done


