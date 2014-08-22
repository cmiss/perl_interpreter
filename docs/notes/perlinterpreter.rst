The Perl Interpreter
====================

This document covers information on the Perl interpreter that is
used by Cmgui to provide access to the Perl scripting language.

The Perl interpreter is designed to be built against dynamic Perl
libraries.  This is done so the user or script writer can use any
Perl module from the system that Cmgui is currently run on.  For 
this to work the system that Cmgui is executed must have a matching
Perl to one that is built into the Perl interpreter.  If this is
not the case then the built-in Perl interpreter will be used.  The 
built-in Perl interpreter has limited functionality allowing for 
only standard Perl modules and constructs.
