humextra
========

C++ programs and library for processing Humdrum data files.


Downloading
===========

To download from the terminal if you have git installed on your system:
   ```git clone https://github.com/craigsapp/humextra```
Otherwise you can download from the "zip" link at the top of this page
(https://github.com/craigsapp/humextra/archive/master.zip).

If you want to use the "git" command to download the humextra software, there are
several ways to install it:

(1) In linux/unix, use some automated installation system which is installed
on your computer.  Some possible methods: `yum install git`, `apt-get
install git` (Ubuntu), or `emerge git` (Gentoo).

(2) in OS X, use a package management system such as MacPorts
(http://macports.org) or Homebrew (http://mxcl.github.io/homebrew).  These
are the OS X equivalents of apt-get/yum/emerge found on linux systems.
Typically you will also need to first install XCode from the Apple App
Store (free).  After installing XCode, go into its menu system and install
the command-line development tools: (a) click on the Downloads tab in
XCode (b) select "command line tools" (c) click on "install" button.
If MacPorts is installed, then type `sudo port install git`; for Homebrew
the command would be `sudo brew install git`.

IDEs such as Eclipse (http://www.eclipse.org/downloads) have a git
interface built into them.  Other GUI-based git programs can be found
at this link: http://git-scm.com/downloads/guis . These might be more
appropriate for MS Windows, although you will have to install a unix
terminal system, such as cygwin (http://www.cygwin.com) to use the
humextras program in MS Windows.


Compiling
=========

To compile everything, type within the humextra directory:
    ```make```
If using OS X, see the above notes on XCode in the Download section if you get a 
complaint about the make command not being found.

To compile only the humextra library:
    ```make library```

To compile all humextra programs (after making the library):
    ```make programs```

To compile a particular humextra program (after making the library):
    ```make humcat```

To test that the programs have been compiled successfully, try typing:
    ```bin/keycor h://wtc/wtc1f01.krn```
which should reply with an estimate of the key for J.S. Bach's Well-Tempered Clavier, 
Book I fugue 1:
    ```The best key is: C Major```


Installing
==========

Compiled programs will be stored in the humextra/bin sub-directory.
To use these programs from any location in the file system, you must tell
the computer where to search for them.  This is done in the terminal by
adding the bin directory to the $PATH variable.  Type ```echo $PATH```
to see what directories are currently being searched for commands.
A lazy way of installing would be to copy the programs in humextra/bin
into one of those locations, such as /usr/bin.

A more proper way would be to update the $PATH environmental variable.
To do this you will need to know which shell you are using.  Type ```echo
$SHELL``` in the terminal to see which one: bash or tcsh are the two main
ones.  If you are using bash, then you can temporarily add humextra/bin
to the $PATH with this command:
    ```PATH=$PATH:/location/of/humextra/bin```
To do the same in tcsh (without the outer set of quotes):
    ```setenv PATH $PATH":/location/of/humextra/bin"```

For a more permanent installation the $PATH must be updated in the login
scripts for bash/tcsh, either for a single user or for the entire system.
For an individual user in bash, add this line:
    ```export PATH=$PATH:/location/of/humextra/bin```
to the file ~/.bashrc .  For tcsh, add the line
    ```setenv PATH $PATH":/location/of/humextra/bin```
to the file ~/.csrhc .  If you want to install for all users on a computer
system, then you would add such lines to the system login script (which
will vary on the version of unix you are using).


Updating
========

To update if you downloaded with git:
   ```git pull```

Then either type `make` to recompile the external libraries, the humextra 
library and the programs, or type `make update` to compile the humextra
library and programs.

When using git to download and update the humextra repository, you can add these
lines to ~/.gitconfig:
```
    [alias]
        hist = log --pretty=format:\"%h %ad | %s%d [%an]\" --graph --date=short
```
Then the command `git hist` will give a short listing of changes (one commit per line)
sorted in reverse chronological order.


Documentation
=============

All humextra programs include an option called `--options` which will list
all of the defined options for the program along with their aliases and default values. 
For example here is the options list for the barnum program:
```
barnum --options
   r|remove=b
   s|start=i:1
   a|all=b
   debug=b
   author=b
   version=b
   example=b
   h|help=b
```

The line `r|remove=b` means that the barnum program accepts an option
called `-r` or equivalently `--remove` which is a Boolean type of option
(it does not take arguments, but is rather a switch to turn on a feature
in the program).  The line `s|start=i:1` means that the `-s` or `--start`
option requires an integer argument, and the default value if not given
is the value `1`.  Options can also have types `d` for double arguments 
(floating-point numbers) and `s` for string arguments.

For more detailed information about each humextra program, go to the webpage
    http://extras.humdrum.org/man
or click on the command name in the list below:

[autostem](http://extras.humdrum.org/man/autostem),
[mvspine](http://extras.humdrum.org/man/mvspine),
[myank](http://extras.humdrum.org/man/myank),
[notearray](http://extras.humdrum.org/man/notearray),
[transpose](http://extras.humdrum.org/man/transpose),
[zscores](http://extras.humdrum.org/man/zscores),
[barnum](http://extras.humdrum.org/man/barnum),
[scaletype](http://extras.humdrum.org/man/scaletype),
[theloc](http://extras.humdrum.org/man/theloc),
[keycor](http://extras.humdrum.org/man/keycor),
[hum2muse](http://extras.humdrum.org/man/hum2muse),
[humplay](http://extras.humdrum.org/man/humplay),
[satb2gs](http://extras.humdrum.org/man/satb2gs),
[kern2skini](http://extras.humdrum.org/man/kern2skini),
[gettime](http://extras.humdrum.org/man/gettime),
[hum2xml](http://extras.humdrum.org/man/hum2xml),
[humcat](http://extras.humdrum.org/man/humcat),
[pitchmix](http://extras.humdrum.org/man/pitchmix),
[sonority](http://extras.humdrum.org/man/sonority),
[tindex](http://extras.humdrum.org/man/tindex),
[mkeyscape](http://extras.humdrum.org/man/mkeyscape),
[humsplit](http://extras.humdrum.org/man/humsplit),
[hum2abc](http://extras.humdrum.org/man/hum2abc),
[thrux](http://extras.humdrum.org/man/thrux),
[kern2cmn](http://extras.humdrum.org/man/kern2cmn),
[pae2kern](http://extras.humdrum.org/man/pae2kern),
[addref](http://extras.humdrum.org/man/addref),
[thememakerx](http://extras.humdrum.org/man/thememakerx),
[rscale](http://extras.humdrum.org/man/rscale),
[hum2mid](http://extras.humdrum.org/man/hum2mid),
[sample](http://extras.humdrum.org/man/sample),
[simil](http://extras.humdrum.org/man/simil),
[xml2hum](http://extras.humdrum.org/man/xml2hum),
[spinetrace](http://extras.humdrum.org/man/spinetrace),
[proll](http://extras.humdrum.org/man/proll),
[tiefix](http://extras.humdrum.org/man/tiefix),
[kern2melisma](http://extras.humdrum.org/man/kern2melisma),
[swing](http://extras.humdrum.org/man/swing),
[tsroot](http://extras.humdrum.org/man/tsroot),
[humpdf](http://extras.humdrum.org/man/humpdf),
[rcheck](http://extras.humdrum.org/man/rcheck),
[prange](http://extras.humdrum.org/man/prange),
[autodynam](http://extras.humdrum.org/man/autodynam),
[ottava](http://extras.humdrum.org/man/ottava),
[time2tempo](http://extras.humdrum.org/man/time2tempo),
[chorck](http://extras.humdrum.org/man/chorck),
[prettystar](http://extras.humdrum.org/man/prettystar),
[ridx](http://extras.humdrum.org/man/ridx),
[hum2gmn](http://extras.humdrum.org/man/hum2gmn),
[beat](http://extras.humdrum.org/man/beat),
[serialize](http://extras.humdrum.org/man/serialize),
[extractx](http://extras.humdrum.org/man/extractx),
[tntype](http://extras.humdrum.org/man/tntype),
[humtable](http://extras.humdrum.org/man/humtable),
[scordur](http://extras.humdrum.org/man/scordur),
[motive](http://extras.humdrum.org/man/motive),
[lofcog](http://extras.humdrum.org/man/lofcog),
[kern2dm](http://extras.humdrum.org/man/kern2dm),
[similx](http://extras.humdrum.org/man/similx),
[harm2kern](http://extras.humdrum.org/man/harm2kern),
[themax](http://extras.humdrum.org/man/themax),
[minrhy](http://extras.humdrum.org/man/minrhy),
[hgrep](http://extras.humdrum.org/man/hgrep),
[time2matlab](http://extras.humdrum.org/man/time2matlab),
[mid2hum](http://extras.humdrum.org/man/mid2hum)

For a basic tutorial on programming with the humextra library, see the webpage
    http://wiki.ccarh.org/wiki/Humdrum_Extras

