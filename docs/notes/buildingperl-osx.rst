
Building Perl on OSX
====================

This document covers how Perl was built for Cmgui on the
OSX 10.6 build slave.  During the development of the Perl
interpreter being built with CMake there was a period of
time where that fallback Perl interpreter was not available
.  While this was the case it became necessary to have a 
version of Cmgui that would run on OSX 10.9.  To resolve
this issue it was determined to add a matching Perl 
interpreter for OSX 10.9 into the Perl interpreter.  The
section on `A Dymanic Perl For OSX 10.9` covers the process
of creating a matching Perl interpreter of OSX 10.9 that
enabled Cmgui to be successfully executed.

A Dynamic Perl For OSX 10.9
---------------------------

- due to restrictions can only build Cmgui on OSX 10.6
- version of perl on OSX 10.9
- the dynamic version location
- perl library is not on the library search path
- manipulated this with a symlink from the desired path
  to the local path

A Fallback Perl
---------------

The fallback perl for Cmgui on OSX was chosen to be Perl version
5.20.0. 

- extracted to ~/perl-5.20.0
- build dir set to ~/perl-5.20.0-build
- installed to ~/perls/5.20


References
----------

[1] perl website

