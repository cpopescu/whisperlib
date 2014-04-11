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

You can build in two ways.

Building using waf
-----------------

To build you need waf-1.6.8:
http://code.google.com/p/waf/

To build on your local machine run:

    $ ./waf-1.6.8 configure build

The build output will go to build/wout/.... You may want to run:
$ ./waf-1.6.8 install --install_dir=<your install dir>
to place the headers and libs where you want them to end up.

To build for ios run something like:

    $ ./waf-1.6.8 configure build --arch=ios-armv7s

(this will build for ios6, and armv7s set). Other values for arch are:
ios-armv6, ios-armv7, ios-i386  (last one is for the simulator)

The build output will go to build/wout.ios-armv7s/...

To build for android run:

    $ ./waf-1.6.8 configure build --arch=android-14-arm

(This builds for ICS in ARM - look in n WafUtil.py for other possible values).

Important:
Check the options in WafUtil.py or by running in this directory waf --help.
You may need to specify your android sdk / ndk paths.


You may want to install in your project directory if you want to incorporate
this in an IOS app. To do this run after build:

    $ ./waf-1.6.8 install -v --arch=ios-armv7s --install_root=$MY_IOS_PROJECT_DIR/libs/whisperlib

You may also want to build for all ios architectures then run a lipo to combine
all libraries into a combined one and copy it manually (with another script).

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
