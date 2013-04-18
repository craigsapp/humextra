humextra
========

C++ programs for processing Humdrum data files.


Downloading
===========

To download from the terminal if you have git installed on your system:
   git clone https://github.com/craigsapp/humextra
Otherwise you can download from the "zip" link at the top of this page.

To compile everything, type:
   make

To compile only the library:
   make library

To compile all programs (after making the library):
   make programs

To compile a particular program:
   make humcat


Updating
========

To update if you downloaded with git:
   git update

Then either type "make" to recompile the external libraries, the humextra 
library and the programs, or type "make update" to compile the humextra
library and programs.


Documentation
=============

All humextra programs include an option called --options which will list
all of the options along with their aliases and default values.

For more information about programs, go to the webpage:
    http://extras.humdrum.org/man


