
Building Perl on Linux
======================

This document covers how Perl was built for Cmgui on the
Ubuntu 12.04 build slave.  Building a static or dynamic
Perl library on a Linux based OS is a relatively easy 
matter.  The differences are minimal and the build progresses
with very little fuss.  The section `Dynamic Perl Library` covers building
a dymainc Perl library and the section `Static Perl Library` covers building
a static Perl library.

Dynamic Perl Library
--------------------

- Use version 5.16.3
::
  export X=5
  export Y=16
  export Z=3
  cd ~/
  mkdir perls
  cd perls
  wget http://www.cpan.org/src/${X}.0/perl-${X}.${Y}.${Z}.tar.gz
  tar -xzf perl-${X}.${Y}.${Z}.tar.gz
  mkdir perl-${X}.${Y}.${Z}-build
  cd perl-${X}.${Y}.${Z}-build
  ../perl-${X}.${Y}.${Z}/Configure -des -Dusemultiplicity -Duseithreads -Duseshrplib -Dmksymlinks -Dprefix=$HOME/perls/${X}.${Y}
  make -j
  make install


Static Perl Library
-------------------

- Use version 5.20.0
- very similar to Dynamic Perl libary just omit the 'useshrplib'
here is the full description 
::
  export X=5
  export Y=20
  export Z=0
  cd ~/
  mkdir perls
  cd perls
  wget http://www.cpan.org/src/${X}.0/perl-${X}.${Y}.${Z}.tar.gz
  tar -xzf perl-${X}.${Y}.${Z}.tar.gz
  mkdir perl-${X}.${Y}.${Z}-build
  cd perl-${X}.${Y}.${Z}-build
  ../perl-${X}.${Y}.${Z}/Configure -des -Dusemultiplicity -Duseithreads -Dmksymlinks -Dprefix=$HOME/perls/${X}.${Y}
  make -j
  make install

References
----------

[1] perl website


