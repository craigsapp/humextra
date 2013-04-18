humextra
========

C++ programs and library for processing Humdrum data files.


Downloading
===========

To download from the terminal if you have git installed on your system:
   ```git clone https://github.com/craigsapp/humextra```
Otherwise you can download from the "zip" link at the top of this page
(https://github.com/craigsapp/humextra/archive/master.zip).


Compiling
=========

To compile everything, type within the humextra directory:
    ```make```

To compile only the library:
    ```make library```

To compile all programs (after making the library):
    ```make programs```

To compile a particular program (after making the library):
    ```make humcat```

To test that the programs have been compiled successfully, try typing:
    ```bin/keycor h://wtc/wtc1f01.krn```
which should reply with an estimate of the key for J.S. Bach's Well-Tempered Clavier, 
Book I fugue 1:
    ```The best key is: C Major```


Installing
==========

Compiled programs will be stored in the humextra/bin subdirectory.
To use these programs from any location in the filesystem, you must tell
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
   ```git update```

Then either type `make` to recompile the external libraries, the humextra 
library and the programs, or type `make updatex` to compile the humextra
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
all of the options along with their aliases and default values. 

For more detailed information about each humextra program, go to the webpage:
    ```http://extras.humdrum.org/man```

