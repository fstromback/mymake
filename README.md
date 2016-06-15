# mymake

## Overview

Mymake is a tool to build C/C++-programs that requires close to no configuration.

For simple programs, it is enough to do `mm main.cpp`, where `main.cpp` is the file containing the
`main` function of the program. Mymake will then find all other source files that need to be
compiled and will compile these. Like `make`, mymake also only re-compiles the files that have been
changed (keeping track of included headers) automatically, without configuration.

To be able to do this, mymake makes two assumptions about your code. Firstly: if you include a
header, say `foo.h`, the file `foo.cpp`, `foo.cxx`, `foo.c`... contains the implementation for what
is declared in the header. So if you include the header, the source file needs to be linked into
your program as well. Secondly: mymake assumes that all source files that are not reachable this way
are not needed by your program. This is true as long as your are not declaring things in another
file than the header file that has the same name as an implementation file. If you break this
assumption, mymake fails to detect your dependencies automatically and needs some help figuring this
out.

Generally, these assumptions are met as long as you follow the structure usually found in
C/C++-programs, so usually you do not have to worry. If your codebase does not follow these
criteria, it is usually easy to make the codebase meed the criteria by adding a few includes and/or
a few mostly empty header files.

## Terminology


Here, the terminology used in mymake is explained.

- **target**: a directory containing a set of source files that together will provide one final output file.
- **project**: a directory containing a set of related *targets*. These *targets* can depend on each other.
- **option**: a string (either specified on the command line or in a project) that modifies the behaviour of the build.

Note: I am using the command `mm` to execute mymake through this document.

## Installation

### Linux/Unix (or MinGW)

The first time you install mymake, clone the repository somewhere and then run `install.sh`. This
will compile mymake using `g++`, and then ask you if you want to set up an alias in your shell. If
you set up an alias, you need to type `source ~/.bashrc` in your shell, or restart your shell, to be
able to use mymake.

If you do not want an alias, you can copy the binary `mymake` to somewhere in your path.

After this, run `mm --config` to generate a global `.mymake`-file which contains your system
specific compilation settings.

### Windows (using Visual Studio 2008 or later)

The first time you install mymake, clone the repository somewhere. Open the "Visual Studio Command
Prompt", from the start menu (or any command prompt you can run cl.exe from) and cd into the
directory you cloned mymake into and run `compile.bat`. Now, copy `mymake.exe` somewhere in your
path.

After this, run `mm --config` to generate a global `.mymake`-file which contains your system
specific compilation settings.

From here, you have several options:
- You always run mymake from the "Visual Studio Command Prompt".
- You add the environment variables from the "Visual Studio Command Prompt" to your system. Run `set` to seem them.
- You add the environment variables from the "Visual Studio Command Prompt" to your global `.mymake`-file.

The two first ones are not described any further here.

To add environment variables to your `.mymake`-file, open `C:\Users\<user>\.mymake` in your
favourite editor and find the lines in the generated configuration starting with `env+=` (they are
commented out). Find the paths for your installation by running `set` in the "Visual Studio Command
Prompt", and uncommenting and changing the lines you need. Note you only need the `Path` variables
that points to `cl.exe`, not the regular ones that are also present in the regular command prompt.

For example, if your output from `set` is:

```
INCLUDE=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\ATLMFC\INCLUDE;C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\INCLUDE;C:\Program Files\Microsoft SDKs\Windows\v6.0A\include;

LIB=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\ATLMFC\LIB;C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\LIB;C:\Program Files\Microsoft SDKs\Windows\v6.0A\lib;

Path=C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE;C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\BIN;C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\Tools;C:\windows\Microsoft.NET\Framework\v3.5;C:\windows\Microsoft.NET\Framework\v2.0.50727;C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\VCPackages;C:\Program Files\Microsoft SDKs\Windows\v6.0A\bin;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\WINDOWS\System32\WindowsPowerShell\v1.0\;
```

you want to add:

```
env+=INCLUDE=>C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\ATLMFC\INCLUDE
env+=INCLUDE=>C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\INCLUDE
env+=INCLUDE=>C:\Program Files\Microsoft SDKs\Windows\v6.0A\include

env+=LIB=>C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\ATLMFC\LIB
env+=LIB=>C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\LIB
env+=LIB=>C:\Program Files\Microsoft SDKs\Windows\v6.0A\lib

env+=Path<=C:\Program Files\Microsoft SDKs\Windows\v6.0A\bin
env+=Path<=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\BIN
```

In general, it does not hurt to add too many environment variables.

## Update

### Linux/Unix

Run the `update.sh` script, and it will download the latest version from git and re-compile it. If
you have set up an alias when you installed mymake, you do not need to do anything more. Otherwise
you need to copy mymake to your path once more.

### Windows

Check out the latest version of mymake using the git client you are using. Then re-compile mymake,
either using `compile.bat`, or by using mymake: `mm release -f`. If you are using mymake, remember that the
resulting binary is in the release-directory.

## Usage

To compile things with mymake, run `mm <file>` where `<file>` is the source file containing the
main function of your program. In most simple programs, this is enough, but sometimes some
configuration is required. Mymake will then compile your program and finally run your program if the
compilation succeeded.

If you need to configure your build process, run `mm --target` to generate a sample `.mymake`-file
that shows all available options. At startup, mymake tries to find a `.mymake` or `.myproject`-file
in the current directory or any parent directories (except for your home directory). If one is
found, mymake uses that directory as the root directory when trying to compile.

To pass parameters to the compiled program, use the `-a` parameter.


## Parallel builds

Mymake supports parallel builds by default. When you run `mm --config` for the first time, mymake
asks for the number of threads you want to use when compiling by default. This is then written into
your global `.mymake`-file. As it resides in the global `.mymake`-file, it can be overwritten in
local configurations and even from the command line using `-j 3` or `-j 1` to disable parallel builds
temporarily.

When building projects in parallel, mymake analyzes the dependency graph of your targets and finds
projects which can be compiled in parallel without breaking any dependencies between them. This can
be disabled on a project-by-project basis by setting `parallel` to `no`.

When building single targets, mymake assumes that all source files (except any precompiled headers)
can be compiled in parallel. If this is not the case, disable parallel builds for those targets by
setting `parallel` to `no` in that `.mymake`-file, or in a global section of your `.myproject`-file.
Pre- and post build steps are never run in parallel.

To limit how many compilation processes are spawned during compilation, mymake examines
`maxThreads`. Mymake spawns maximum that many processes globally, even if two or more targets are
compiled in parallel.

When building in parallel, mymake automatically adds a string like `1>` or `p1: ` in front of all
output done in parallel. Each target gets a unique number, so that it is easy to see which target
each error message originates from. The output `1>` is similar to what is used in Visual Studio,
which works in Emacs when using the Visual Studio compiler. On GCC, mymake adds a string like `p1:`,
which Emacs correctly recognizes. However, if it causes trouble, set `usePrefix=no` either in your
project file, or in your global `.mymake`-file.

## Configuration files

Configuration in mymake is done by assigning values to variables. Each variable is an array of
strings.

Mymake knows two different configuration files: `.mymake` and `.myproject`. Both follows the same
basic syntax. A configuration file contains a number of sections, each of which contains a number of
assignments.

In this context, a name is a string only containing characters and numbers.

Lines starting with `#` are comments.

A section starts with zero or more comma separated names enclosed in square brackets (`[]`). The
names indicate which options need to be specified on the command line for the section to be
evaluated. For example: `[windows,lib]` indicates that the options `windows` and `lib` needs to be
active for that section to be evaluated. You can also negate the options required by prepending a
`!`, eg. `[lib,!windows]`. A special case: `[]` is always evaluated. If nothing is specified before
an assignment, it is assumed to be contained inside an empty section (ie. `[]`).

Each section contains zero or more assignments. An assignment has the form `name=value` or
`name+=value`. The first form, means that the variable on the left hand side is replaced with
the value on the right hand side. The second form, however, means that the value is appended to the
variable.

Each of these assignments are evaluated from top to bottom, so a variable can be overwritten later
in a configuration file. The global `.mymake`-file is evaluated before any local files. Any
variables originating from the command-line are applied last.

By default, mymake defines either the option `windows` or `unix` so that files can detect what kind
of platform are being used easily.

## Pre-defined options

The following options are pre-defined by mymake or the default configuration:
- `release`: When this option is present, mymake generates a release version of your program. See the
  default .mymake-file for how this is done for your compiler.
- `lib`: build a static library.
- `sharedlib`: build a shared library.
- `project`: defined when a project file is evaluated in the project-context.
- `windows`: Defined when running on a windows system, expected to be using `cl.exe` as the compiler.
- `unix`: Defined when running on an unix system, or when running in MinGW.

## Wildcard patterns in mymake

Wildcard patterns (containing * and ?) work as expected in mymake, with one exception: the character
`/` matches both `/` and `\`, to ease compatibility between systems. Hence, always write your
patterns as if `/` is the path delimiter.

## Variables used by mymake

These variables are used by mymake to understand what should be done:
- `ext`: array of the file extensions you want to compile. Whenever mymake realizes you have included `x.h`,
  looks for all extensions in `ext` and tries them to find the corresponding implementation file.
- `execExt`: file extension of executable files.
- `intermediateExt`: file extension of intermediate files.
- `buildDir`: string containing the directory used to store all temporary files when building your program.
  Relative to the root directory of the target (ie. where the `.mymake` file is).
- `execDir`: string containing the directory used to store the final output (the executable) of all targets.
  Relative to the root directory of the target.
- `ignore`: array of patterns (like in the shell) that determines if a certain file should be ignored. Useful
  when working with templates sometimes.
- `noIncludes`: array of patterns (like in the shell) that determines if a certain path should not be scanned for
  headers. Useful when you want to parts of the code that is not C/C++, where it is not meaningful to look for
  `#include`.
- `input`: array of file names to use as roots when looking for files that needs to be compiled. Anything that
  is not an option that is specified on the command line is appended to this variable. The special value `*` can
  be used to indicate that all files with an extension in the `ext` variable should be compiled. This is usually
  what you want when you are compiling a library of some kind.
- `output`: string specifying the name of the output file. If not specified, the name of the first input file
  is used instead.
- `appendExt`: append the original extension of the original source file to the intermediate file when compiling.
  This allows mymake to compile projects where there are multiple files with the same name, eg. `foo.cpp` and `foo.c`
  without both trying to create `foo.o` and thereby causing compilation to fail. Mymake warns you if you might need to
  add use this option.
- `include`: array of paths that should be added to the include path of the compilation.
- `includeCl`: flag to prepend all elements in `include`.
- `library`: array of system libraries that should be linked to your executable.
- `libraryCl`: flag to prepend all elements in `library`.
- `localLibrary`: array of local libraries that should be linked to your executable (usually used in a project).
- `localLibraryCl`: flag to prepend all elements in `localLibrary`.
- `define`: preprocessor defines.
- `defineCl`: preprocessor define flag.
- `exceute`: yes or no, telling if mymake should execute the program after a successful compilation. This can be
  overridden on the command line using `-e` or `-ne`.
- `showTime`: yes or no, telling if mymake should show the total compilation time when done (not implemented).
- `pch`: the precompiled header file name that should be used. If you are using the default configuration, you only
  need to set this variable to use precompiled headers. If you are using `#pragma once` in gcc, you will sadly get a
  warning that seems impossible to disable (it is not a problem when precompiling headers).
- `pchFile`: the name of the compiled version of the file in `pch`.
- `pchCompile`: command line for compiling the precompiled header file.
- `pchCompileCombined`: if set to yes, `pchCompile` is expected to generate both the pch-file and compile a .cpp-file.
- `preBuild`: array of command-lines that should be executed before the build is started. Expands variables.
- `postBuild`: array of command-lines that should be executed after the build is completed. Expands variables.
- `compile`: array of command lines to use when compiling files. Each command line starts with a pattern
  (ending in `:`) that is matched against the file name to be compiled. The command line added last is checked
  first, and the first matching command-line is used. Therefore it is useful to first add the general command-line
  (starting with `*:`), and then add more specific ones. Here, you can use `<file>` for the input file and `<output>`.
- `link`: command line used when linking the intermediate files. Use `<files>` for all input files and `<output>` for
  the output file-name.
- `linkOutput`: link the output of one target to any target that are dependent on that target. See projects for more information.
- `forwardDeps`: forward any of this target's dependencies to any target that is dependent on this target.
- `env`: set environment variables. Each of the elements in `env` are expected to be of the form:
  `variable=value` or `variable<=value` or `variable=>value`. The first form replaces the environment variable `variable`
  with `value`, the second form prepends `value` to `variable` using the system's separator (`:` on unix and `;` on windows),
  the third form appends `value` to `variable`. The second and third forms are convenient when working with `PATH` for example.
- `explicitTargets`: In projects: ignore any potential targets that do not have their own `.mymake`-file.
- `parallel`: In projects, this indicates if projects that have all dependencies satisfied may be built in parallel. The default
  value is `yes`, so projects not tolerating parallel builds may set it to `no`.
  In targets, this indicates if files in targets may be built in parallel. If so, all input files, except precompiled headers,
  are built in parallel using up to `maxThreads` threads globally. If specific targets do not tolerate this, set `parallel` to
  `no`, and mymake will build those targets in serial.
- `maxThreads`: Limits the global number of threads (actually processes) used to build the project/target globally.
- `usePrefix`: When building in parallel, add a prefix to the output corresponding to different targets. Defaults to either
  `vc` or `gnu` (depending on your system). If you set it to `no`, no prefix is added. `vc` adds `n>` before output,
  `gnu` adds `pn: ` before output. This is so that Emacs recognizes the error messages from the vc and the gnu compiler,
  respectively.
- `absolutePath`: send absolute paths to the compiler, this helps emacs find proper source files in projects with multiple
  targets.
- `implicitDeps`: (defaults to `yes`), if set, mymake tries to figure out dependencies between targets by looking at includes.
  Sometimes, this results in unneeded circular dependencies, causing compilation to fail, so sometimes it is neccessary to set
  this to `no`.

## Variables in strings

In indicated variables, you can embed any variable (even ones not known by mymake) that will be
expanded into that string. The syntax for this is `<variable>`. This means that the value of
`variable` is inserted into the string at that location. If `variable` is an array with multiple
values, these values are concatenated into one space separated string.

It is also possible to append a string before each element in another variable by using
`<prefix*variable>`. This means that the string in the variable `prefix` is appended to each element
in `variable` and then concatenated into a space separated string.

It is also possible to apply an operation to each element in an array by using `<op|variable>`,
where `op` is the operation to apply. Currently, it is not possible to specify operations in
configuration files, you can only use the ones built into mymake. Operations currently supported
are:
- `title`: treat the element as a path and extract the file or directory name (eg. `src/foo.txt` gives `foo.txt`).
- `titleNoExt`: same as title, but the file extension is removed as well.
- `noExt`: remove the file type from a path.
- `path`: format the element as a path for the current operating system. For example: `src/foo.txt` evaluates to `src\foo.txt` on Windows.
- `buildpath`: make the element into a path inside the build path.
- `exexpath`: make the element int a path inside the bin path.
- `parent`: evaluates to the parent directory of the path. If no parent is given (eg. only a file-name), the element is removed from the array.
- `if`: make all elements empty. This can be used to test if a variable contains a value and then include something.

It is also possible to combine these two operations, like: `<prefix*path|files>`. Stars have
priority over pipes, and expressions are evaluated left-to-right.

There are two variables that mymake generate automatically:
- `includes`: equivalent to `<includeCl*path|include>`, ie. all include paths required by the project prefixed by any flags needed.
- `libs`: equivalent to `<libraryCl*path|library> <localLibraryCl*path|library>`, ie. all libraries required by the project.

## Targets

The goal of a target is to produce the file `<execDir>/<output>.<outputExt>`. To do this, mymake
starts by looking at all files in `input` (which, among others are the files specified on the
command line) and adding them to the set of files to compile. For each file in the set, mymake then
finds all included files. Both directly included files and indirectly included files are
found. After that, mymake finds corresponding implementation files (of any type in `<ext>`) and adds
them to the set of files to compile as well. When this yields no more files, mymake compiles them
one by one if necessary (just like make would do). Finally, it link all files together to the
output file.

To do this, mymake considers the global `~/.mymake`-file as well as any local `.mymake`-file, in
that order. Therefore, any assignments in the local file can overwrite global settings.

## Projects

A project is a collection of targets that depend on each other in some way. In mymake, a project is
represented by a `.myproject`-file in one directory and optional `.mymake`-files in each
subdirectory that is representing another project. The project configuration follows the same syntax
as the target configurations, but the project configuration has two special sections: `build` and `deps`.

The `build` section contains information about which options should be passed to each project. When
building a project, any options passed on the command line are not automatically passed on to the
targets. Instead you have to specify which options are needed by assigning a variable with the same
name as the target any options needed like so:

```
[build]
main+=lib
main+=debug
```

There is also a special target named `all`. All options in this target are used for all targets.

The `deps` section contains information about target dependencies. By default, mymake tries to figure out
which targets depend on each other by looking at the includes. Sometimes this is not enough, since a
project may need the result of another project as a pre-processing step or similar. Therefore,
mymake allows you to specify any dependencies explicitly.

If one target results in a library, it is convenient to set the variable `linkOutput` to `yes` for
that target. Mymake will then add the output of the library target to the `library` variable of any
targets that depend on it. See the `testproj` directory for an example.

Of course, the `build` and `deps` sections are nothing but regular options, and can therefore be
combined with other options as well:

```
[build,windows]
main+=foo
```
