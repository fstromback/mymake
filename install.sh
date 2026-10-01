#!/bin/bash

./compile.sh mm

if [ -e mm ]
then
    :
else
    exit 1
fi

touch mm_v2

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

file=`pwd`"/mm"

go_on="yes"
while [ -n "$go_on" ]
do
    echo "Which shell should I add to?"
    echo "1: Bash"
    echo "2: csh"
    echo "3: fish"
    read shell_type

    go_on=""

    if [ $shell_type = 1 ]
    then
	echo "alias ${alias_name}='$file'" >> ~/.bashrc
	echo "Done! Now do source ~/.bashrc"
    elif [ $shell_type = 2 ]
    then
	echo "alias ${alias_name} \"$file\"" >> ~/.cshrc
	echo "Done! Now do source ~/.cshrc"
    elif [ $shell_type = 3 ]
    then
  echo -e "function ${alias_name}\n    $file\nend" >> ~/.config/fish/functions/${alias_name}.fish
	echo "Done! It is now added as a fish function."
    else
	echo "I do not know about that shell..."
	go_on="yes"
    fi
done


