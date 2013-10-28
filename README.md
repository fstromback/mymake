mymake
======

mymake is an alternative to make, but requires close to no configuration in most cases.

mymake was created when I got tired of waiting for the simplest compilation to finish each time I made a change to my source (no make, nothing but the compiler). Manually keeping track of which files to re-compile was completely out of the question, so I tried make. However, the approach I knew at that time required me to remember to update my make file each time I included a new file from any other source file. I realized that this would be tiresome and very error-prone, so I decided against it. Lazy as I was (or maybe dumb...) I did not feel like I wanted to take the effort to learn how to write proper make-files, neither evaluate other tools. I wanted something almost as easy to use as to just compile files on the command line, but which can also do a "minimal rebuild".

This was when mymake was born. It is, as previously mentionend, designed to work almost like the native approach on the command line, but it also reduces build-times by doing "minimal rebuilds" as well as minimizes what you have to type in the terminal to make it do its job. One thing I also incorporated is the "auto-run" feature. When you are doing a lot of testing, you don't want to first rebuild and then run your program if it was successfully built. Especially if you have command-lines more than a few (one?) character. It turns out that this is more or less what you want when you are coding smaller projects, where you do not want to start by writing makefiles or configuring other tools, but you just want to code.

How it works
------------

How does mymake accomplish its task without almost any configuration?
It makes the following assumption about your code: If you include a header-file, you want to compile the corresponding .cpp file as well. This means that cpp and h files are treated as a pair (if a cpp-file does not exist, it does not matter). This is usually no problem, as this is how you usually structure your code to keep track of it yourself.

Making this assumption, we can then look for all files needed in the project as long as we are given at least one file to start from. This is the file given to mymake for compilation.

So what happens when you tell mymake to compile your code file `main.cpp` is that mymake first scans it for `#include` statements, and keeps track of these files. If we assume we find that `main.cpp` includes `class.h`, then mymake remembers that `main.cpp` depends on `class.h`, as well as adds `class.cpp` to the list of files to examine (if it exists). Mymake will in this manner recursively scan all included files from all `.cpp` files found. To speed up the compilation process even more, mymake checks the file dates of all included files against the latest sucessfully compiled object file, to see if a re-compilation is neccessary. Mymake also keeps a cache of includes in the build-folder to avoid scanning uncanged files for includes.

This incremental rebuild will work fine in most cases, however it might currently fail to detect some changes if you keep editing files while the rebuild is in progress. I have not personally seen this issue myself, but it might happen with the current implementation. If something looks strange, you can always rebuild with `mm <file> -f` to force a rebuild, or you can simply remove the build directory.

Worth to notice is that mymake does not take any other preprocessor macros into consideration, which means that mymake will treat all `#include` as active ones, even if they are surrounded by `#if 0` or similar. This is usually not a problem, since it will only result in mymake re-compiling some files too often and compiles a few files mor than what might have been neccessary. Mymake does not either take any notice of `#include <>`, as these are considered library files, which should be specified on the link command-line instead (in `.mymake`).

One thing worth to think about here is that one can have multiple files containing an `int main()`, as long as these files do not include each others. If you have one file `main.cpp`, which is the regular entry-point for your application, you can also have a file `run_tests.cpp`, which runs your tests instead. Then you can simply run them with `mm main` and `mm run_tests` respectively. The two main files will even share intermediate object files, which also reduces compilation times. Another use is if you have a networked application, and want to have a server and a client with a lot of shared code, then you can have your `server.cpp` and `client.cpp` side-by-side without any problems.

Updating
---------

Updating mymake is from now on easy, assuming you used the install script. Just run the update.sh in the repo, and mymake will get the latest changes and re-install mymake.

Installation
-------------

Sadly, even mymake requires some configuration. The first thing you need to do is to download the source code somewhere. Then you can simply run the "install.sh" to build mymake from source and, if you want, add an alias to your favorite shell. I prefer the short name `mm`, which I am going to use throughout this document.

When that is done, you just need to set up your global configuration file.


Configuration
--------------

To create your global configuration file `~/.mymake`, you just need to run `mm -config` to create the global configuration file. This will later serve as a base file to your specific projects, so it might be worth spending a few moments to custumize it, even though everything will work perfectly fine without editing it.

The configuration file now supports platform specific properties. If a flag is preceeded by XXX., it means that mymake will ignore that line unless we are currently compiling for the platform XXX. For example, in the default generated compilation file, you will see `win.includeCl=...`. This means that this line will override the default `includeCl` used on linux, since it comes after that line in the file. There are currently two platforms supported, `win` and `unix` for windows and unix-like systems respectively.

These are also extended to generic command line parameters. If you enter the command line `mymake foo bar`, then all entries in the configuration file containing `foo` and `bar` will be applied. A setting can contain more than one dot, for example `win.foo.input=bar.cpp`. In that case it will only be added if all the configurations are specified.

Now we have a global configuration file. This is the file used for all settings not present in the local configuration file. Settings in the local file will override the global one as well. In new projects, if you need to specialize the global file, you can easily copy it to the current working directory using `mm -cp`. This copies it into the working directory and adding comments to all lines, so that relevant lines can easily be uncommented.

Below is a description of the most important settings in the `.mymake` file:

`ext=cpp`
Tells mymake which file types it should consider implementation files. One row for each file type. The default is to accept cpp, cc, cxx, c++ and c. This should be fine in most cases.

`input=`
This row is equivalent to specifying files on the command line. This should not be used in the global configuration, and does not need to be included in the specific files either, as mymake will in most cases discover relevant files itself. As discussed above, mymake will use all files listed as inputs (either here or on the command line) to traverse the include-hierarcy to find the needed files for the project.

`ignore=`
Add one ignore-line for each wildcard pattern you wish to ignore. This is useful when dealing with c++-templates when you are keeping the template-implementation in a separate file named `XXX.template.cpp` for example. Since that file should not be compiled as a regular file, you can simply ignore it with the wildcard pattern `*.template.*`.

`include=`
Additional include paths, automatically tells the compiler about these as well. As before, use one include-line per include path you need.

`includeCl=`
The flag used to tell the compiler about additional include paths. `-iquote ` for gcc (pay attention to trailing spaces here!)

`libraryCl=`
The flag used to tell the compiler about additional libraries (static).

`libs=`
Tells mymake that the project needs additional libraries using the libraryCl flag.

`out=`
If you want a specific name of the executable file, specify it here. Otherwise mymake will generate a name from the first source file specified.

`compile=g++ <file> -c <includes> -o <output>`
The command line executed to compile a compilation unit to an object file.
* `<file>` will be replaced by the current file to compile
* `<includes>` will be replaced by whatever is in `includeCl` followed by include paths. Each path will get its own `includeCl` first.
* `<output>` will be replaced by the relative path to the output file the compiler is expected to produce.

`link=g++ <files> -o <output>`
The command line executed to link object-files together to an executable file.
* `<files>` will be replaced by a list of input files, separated by spaces.
* `<output>` will be replaced by the output executable the compiler should produce.

Usage
-----

Now when we have got through the heavy configuration (`mm -config`, really time consuming, right?), we can go through how I thought it to be used.

Generally you use one of the three top-most in your shell to run and test your program. Since you only have one command, you can quickly compile and run by hitting up-arrow and return in your shell (or `M-p RET` if you have it in Emacs...).

* `mm main` compile code with the root file named `main.cpp`, and execute it if compilation succeeded.
* `mm main -a foo bar` To pass parameters `foo bar` to your program when it starts.
* `mm main -ne` Do not execute the compiled program.
* `mm main -f` If you want to force recompilation of all files.

If you see that you need to customize the `.mymake` file (adding libraries for example), simply do:
`mm -cp` and it will be copied for you. More or less a shorthand for `cp ~/.mymake ./`

Quick start
-----------

This is a list of the bare minimum you need to do to get mymake up and running. Great if you want (just like me) get coding without understanding why it works, or are (just like me) too lazy to read the wall of text above.

* Download the code
* `install.sh` - will compile code and add (if you want) aliases to your shell
* `mm -config` - create the global configuration file
* `cd ~/my/project/`
* `mm root_file`

That is about it. The last line is where your project is compiled and run. If you need to specialize the configuration for your project, just do:

* `mm -cp`

and edit `.mymake` in your text editor.

Windows
-------

Mymake can also be used on Windows with `cl.exe`, the Visual Studio compiler. Sadly, I have not made any install scripts for Windows yet, but you should be fine with compiling all the source files with something like `cl *.cpp /nologo /EHsc /Fomymake.exe`. Then store the executable somewhere in your `PATH` and then you should be able to follow the rest of the steps above. This of course assumes you have the visual studio environment variable set up. It is possible to work around that, but it gets kind of messy.
