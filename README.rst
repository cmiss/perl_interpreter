
Perl Interpreter
================

This software is the perl_interpreter for CMISS.
It is released under the Mozilla Public License v1.1

The perl interpreter enables legacy CMISS commands to be interspersed with perl code.

Requirements
------------

To build the perl interpreter the following packages are required.

 - Build toolchain
 - CMake
 - Perl

MacOS
-----

For MacOS the system perl is not suitable for use with the perl interpreter.
You will need to install a suitable perl before configuring the perl interpreter.
A suitable perl can be installed from https://perlbrew.pl/.

Building on MacOS
"""""""""""""""""

Commands for buildng a release build of the perl interpreter using the Terminal appilcation::

  git clone https://github.com/cmiss/perl_interpreter.git
  mkdir build-perl_interpreter
  cd build-perl_interpreter
  cmake -DINSTALL_PREFIX=../usr/local ../perl_interpreter
  make
  make install

If all of these commnds successfully complete the perl interpreter will be available for using with CmGui from '../usr/local/' directory.

Further Reading
---------------

Further information, help pages, a tracker and a wiki is available at 
http://www.cmiss.org/cmgui/

This software was originally developed at The University of Auckland and 
the Copyright held by Auckland UniServices Ltd.
This development has been for over 15 years and this release is under ongoing
development.
