humextra
========

C++ programs and library for processing Humdrum data files.


Downloading
===========

To download from a terminal if you have git installed on your system:
   ```git clone https://github.com/craigsapp/humextra```

Otherwise you can download from the "zip" link at the top of this page
(https://github.com/craigsapp/humextra/archive/master.zip).  From the
command line you can download the humextra ZIP file with this command 
in linux:
    ```wget https://github.com/craigsapp/humextra/archive/master.zip```
or use curl in OS X:
    ```curl -L https://github.com/craigsapp/humextra/archive/master.zip > master.zip```
Then unzip the file with this command:
    ```unzip master.zip```

If you want to use the "git" command to download the humextra software,
check to see if it is installed on your computer by typing this command
in the terminal:
   ```which git```
It should return with the location of the command in the file structure,
such as:
    ```/usr/bin/git```
If `which git` does not reply with any location, then you will have to
install git.  There are several ways to install it:

(1) In linux/unix, use some automated installation system which is
installed on your computer.  Some possible methods: `yum install git`,
`apt-get install git` (Ubuntu), or `emerge git` (Gentoo).

(2) in OS X, use a package management system such as Homebrew
(http://mxcl.github.io/homebrew) or MacPorts (http://macports.org).
For example, to install Homebrew type the following command in the
terminal:
   ```ruby -e "$(curl -fsSL https://raw.github.com/mxcl/homebrew/go)"```
Homebrew and MacPorts are two OS X equivalents of apt-get/yum/emerge 
which are used to install software over the internet on linux systems.

In Mac OS X 10.9 (Mavericks) you should first install the Xcode 
command-line compiling tools using this command:
   ```xcode-select --install```
This will install xcode command-line tools over the internet after you
click on a few pop-up windows.

For older Mac OS X versions, you will need to first install XCode from
the Apple App Store (free).  After installing XCode, go into its menu
system and install the command-line development tools: (a) click on
the Downloads tab in XCode (b) select "command line tools" (c) click on
"install" button.  

To install git after homebrew/macports and XCode command line tools are
installed.  type `brew install git` if using Homebrew, or `sudo port
install git` if using MacPorts.

IDEs such as Eclipse (http://www.eclipse.org/downloads) have a git
interface built into them.  Other GUI-based git programs can be found
at this link: http://git-scm.com/downloads/guis . These might be more
appropriate for MS Windows, although you will have to install a unix
terminal system such as cygwin (http://www.cygwin.com) to use the
humextras program in MS Windows.


Compiling
=========

To compile everything, type within the main humextra directory:
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
For an individual user in bash add this line:
    ```export PATH=$PATH:/location/of/humextra/bin```
to the file ~/.bashrc .  For tcsh, add the line
    ```setenv PATH $PATH":/location/of/humextra/bin```
to the file ~/.csrhc .  Alternatively, the PATH environmental variable
may need to be set in `~/.profile`.If you want to install for all users
on a computer system then you would add such lines to the system login
script (which will vary on the version of unix you are using).


Updating
========

To update if you downloaded with git:
   ```git pull```

Then either type `make` to recompile the external libraries, the humextra 
library and the programs, or type `make update` to compile just the humextra
library and programs, or `make libupdate` to update only the library file 
without recompiling example programs.  If you installed by download a ZIP file,
then you would need to re-download another ZIP file with the updated code.

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

[addref](http://extras.humdrum.org/man/addref),
[autodynam](http://extras.humdrum.org/man/autodynam),
[autostem](http://extras.humdrum.org/man/autostem),
[barnum](http://extras.humdrum.org/man/barnum),
[beat](http://extras.humdrum.org/man/beat),
[chorck](http://extras.humdrum.org/man/chorck),
[cint](http://extras.humdrum.org/man/cint),
[dittox](http://extras.humdrum.org/man/dittox),
[extractx](http://extras.humdrum.org/man/extractx),
[gettime](http://extras.humdrum.org/man/gettime),
[harm2kern](http://extras.humdrum.org/man/harm2kern),
[hgrep](http://extras.humdrum.org/man/hgrep),
[hum2abc](http://extras.humdrum.org/man/hum2abc),
[hum2gmn](http://extras.humdrum.org/man/hum2gmn),
[hum2mid](http://extras.humdrum.org/man/hum2mid),
[hum2muse](http://extras.humdrum.org/man/hum2muse),
[hum2xml](http://extras.humdrum.org/man/hum2xml),
[humcat](http://extras.humdrum.org/man/humcat),
[humpdf](http://extras.humdrum.org/man/humpdf),
[humplay](http://extras.humdrum.org/man/humplay),
[humsplit](http://extras.humdrum.org/man/humsplit),
[humtable](http://extras.humdrum.org/man/humtable),
[kern2cmn](http://extras.humdrum.org/man/kern2cmn),
[kern2dm](http://extras.humdrum.org/man/kern2dm),
[kern2melisma](http://extras.humdrum.org/man/kern2melisma),
[kern2skini](http://extras.humdrum.org/man/kern2skini),
[keycor](http://extras.humdrum.org/man/keycor),
[location](http://extras.humdrum.org/man/location),
[lofcog](http://extras.humdrum.org/man/lofcog),
[mid2hum](http://extras.humdrum.org/man/mid2hum),
[minrhy](http://extras.humdrum.org/man/minrhy),
[mkeyscape](http://extras.humdrum.org/man/mkeyscape),
[motive](http://extras.humdrum.org/man/motive),
[mvspine](http://extras.humdrum.org/man/mvspine),
[myank](http://extras.humdrum.org/man/myank),
[notearray](http://extras.humdrum.org/man/notearray),
[ottava](http://extras.humdrum.org/man/ottava),
[pae2kern](http://extras.humdrum.org/man/pae2kern),
[pitchmix](http://extras.humdrum.org/man/pitchmix),
[prange](http://extras.humdrum.org/man/prange),
[prettystar](http://extras.humdrum.org/man/prettystar),
[proll](http://extras.humdrum.org/man/proll),
[rcheck](http://extras.humdrum.org/man/rcheck),
[ridx](http://extras.humdrum.org/man/ridx),
[rscale](http://extras.humdrum.org/man/rscale),
[sample](http://extras.humdrum.org/man/sample),
[satb2gs](http://extras.humdrum.org/man/satb2gs),
[scaletype](http://extras.humdrum.org/man/scaletype),
[scordur](http://extras.humdrum.org/man/scordur),
[serialize](http://extras.humdrum.org/man/serialize),
[simil](http://extras.humdrum.org/man/simil),
[similx](http://extras.humdrum.org/man/similx),
[sonority](http://extras.humdrum.org/man/sonority),
[spinetrace](http://extras.humdrum.org/man/spinetrace),
[swing](http://extras.humdrum.org/man/swing),
[theloc](http://extras.humdrum.org/man/theloc),
[themax](http://extras.humdrum.org/man/themax),
[thememakerx](http://extras.humdrum.org/man/thememakerx),
[thrux](http://extras.humdrum.org/man/thrux),
[tiefix](http://extras.humdrum.org/man/tiefix),
[time2matlab](http://extras.humdrum.org/man/time2matlab),
[time2tempo](http://extras.humdrum.org/man/time2tempo),
[tindex](http://extras.humdrum.org/man/tindex),
[tntype](http://extras.humdrum.org/man/tntype),
[transpose](http://extras.humdrum.org/man/transpose),
[tsroot](http://extras.humdrum.org/man/tsroot),
[voicecount](http://extras.humdrum.org/man/voicecount),
[xml2hum](http://extras.humdrum.org/man/xml2hum),
[zscores](http://extras.humdrum.org/man/zscores).

For a basic tutorial on programming with the humextra library, see the webpage
    http://wiki.ccarh.org/wiki/Humdrum_Extras 
.

