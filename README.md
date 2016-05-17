Date: 2014-04-10 08:30:11

whisperlib
==========

Multi platform general use library (base logging / networking / http / rpc etc)

Prerequisites
=============

This library depends on the following additional libraries:

- glog (https://code.google.com/p/google-glog/) - for logging
- gflags (https://code.google.com/p/gflags/) - command line flags

Optional libraries:

- ICU (http://site.icu-project.org/) - to handle UTF8
- Protobuf (https://code.google.com/p/protobuf/) - used by the RPC mechanism

You might find useful the gperftools (https://code.google.com/p/gperftools/).

You also need a C++ compiler, bison, flex, make, zlib.

Building
========

Building using the autoconf utilities
-------------------------------------

Run

    $ ./configure
    $ make
    $ make check

If you want to keep the source directory clean, without any .o files,
you can do the following:

    $ mkdir build
    $ cd build
    $ ../configure
    $ make
    $ make check

If your build machine has a multi-core CPU with lots of RAM, try
passing the -j option to make, to speed up the build:

    $ make -j24

To run a single test:

    $ make -j24 check TESTS=whisperlib/base/test/strutil_test

To install the library, run:

    $ sudo make install

By default this installs the header files and statically built library
in /usr/local. To change the installation location, run configure with
the --prefix argument.

The autoconf build does not support cross compilation for now, so you
cannot yet build for iOS or Android.

If you need to generate the configure script, you need at least
automake-14.1 and autoconf-2.69 installed on your machine. Older
versions of these programs will not work. To regenerate the configure
and Makefile.in files:

    $ ./autogen.sh

Enjoy
