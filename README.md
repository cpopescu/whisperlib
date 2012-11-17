whisperlib
==========

Multi platform general use library (base logging / networking / http / rpc etc)

To build you need waf:
http://code.google.com/p/waf/

You may also better have glog / gflags / google perftools for your 
desktop/server build. You also need a c++ compiler, bison, flex, make, zlib.

To build on your local machine run:

$ waf configure build

The build output will go to build/wout/.... You may want to run:
$ waf install --install_dir=<your install dir>
to place the headers and libs where you want them to end up.

To build for ios run something like:

$ waf configure build --arch=ios-armv7s
(this will build for ios6, and armv7s set). Other values for arch are:
ios-armv6, ios-armv7, ios-i386  (last one is for the simulator)

The build output will go to build/wout.ios-armv7s/...

To build for android run:
$ waf configure build --arch=android-14-arm
(This builds for ICS in ARM - look in n WafUtil.py for other possible values).

Important:
Check the options in WafUtil.py or by running in this directory waf --help.
You may need to specify your android sdk / ndk paths.


You may want to install in your project directory if you want to incorporate
this in an IOS app. To do this run after build:

waf install -v --arch=ios-armv7s --install_root=$MY_IOS_PROJECT_DIR/libs/whisperlib

You may also want to build for all ios architectures then run a lipo to combine
all libraries into a combined one and copy it manually (with another script).

Enjoy


