# -*- python-indent: 4; indent-tabs-mode: nil; coding: utf-8 -*-
# Copyright, 1618labs, Inc. 2011 onwards, All rights reserved
# Author: giao, cp
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
# * Neither the name of 1618labs nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


import commands
import glob
import os
import sys

from waflib import Build, Logs, Node, Options, TaskGen, Utils
#from waflib.Tools import c_preproc
#c_preproc.go_absolute = True

JNI_INCLUDE_DIRS = [
    '/System/Library/Frameworks/JavaVM.framework/Headers',
    '/Library/Java/JavaVirtualMachines/jdk1.7.0_09.jdk/Contents/Home/include',
    '/usr/lib/jvm/java-6-openjdk/include',
    '/usr/lib/jvm/java-7-oracle/include',
    '/usr/local/java/jdk1.7.0_45/include'
]

## NOTE: to build NACL - one needs to pre-build regal w/ proper NACL sdk path
# please read README.nacl.txt

def setup_options(opt):
    opt.load('compiler_c compiler_cxx gas flex bison')
    opt.add_option('--arch', action='store', default='',
                   help='Configure for build. Empty for machine default.')
    opt.add_option('--android_ndk_path', action='store',
                   default='%s/android-ndk' % os.environ['HOME'],
                   help='Root of the android ndk to use.')
    opt.add_option('--android_sdk_path', action='store',
                   default='%s/android-sdk' % os.environ['HOME'],
                   help='Root of the android sdk to use.')
    opt.add_option('--tizen_sdk_path', action='store',
                   default='%s/tizen-sdk' % os.environ['HOME'],
                   help='Root of the tizen sdk to use.')
    opt.add_option('--nacl_sdk_path', action='store',
                   default='%s/nacl_sdk' % os.environ['HOME'],
                   help='Root of the nacl sdk to use.')
    opt.add_option('--nacl_lib', action='store',
                   default='glibc',
                   help='Nacl library to use (glibc or newlib) - regal has to be compiled w/ it.')
    opt.add_option('--regal_sdk_path', action='store',
                   default='%s/regal' % os.environ['HOME'],
                   help='Root of the regal sdk to use.')
    opt.add_option('--cxx', action='store',
                   default='g++' if Options.platform == 'linux' else 'clang++',
                   help='Specify C++ compiler')
    opt.add_option('--c', action='store',
                   default='gcc' if Options.platform == 'linux' else 'clang++',
                   help='Specify C++ compiler')
    opt.add_option('--asm', action='store',
                   default='nasm' if Options.platform == 'linux' else 'as',
                   help='Specify assembler to use')
    opt.add_option('--disable_ssl', action='store_true', default=False,
                   help='If is true we use open SSL')
    opt.add_option('--release', action='store_true', default=False,
                   help='Configure for release (non-debug) build')
    opt.add_option('--install_root', action='store', default=None,
                   help='Where to install the output - defaults to build '
                   'wout root dir/install')
    opt.add_option('--use_icu', action='store_true', default=False,
                   help='Use ICU library if found')
    opt.add_option('--icu_config', action='store', default='',
                   help='If you want to use a ICU, give the icuconfig path '
                   'here if not in normal path')
    opt.add_option('--pkg_config', action='store', default='',
                   help='If you want to use pkg-config, give the binary path '
                   'here if not in normal path')
    opt.add_option('--osxver', action='store', default='',
                   help='Compile for this version of OSX')
    opt.add_option('--jni_include_dirs', action='store',
                   default=','.join(JNI_INCLUDE_DIRS),
                   help='Where the jni.h include dir is placed. Unfortunately ' +
                   'there is no easy way to determine this automatically.')
    opt.add_option('--use_harfbuzz', action='store_true', default=False,
                   help='Set this if you want to use Freetype/Harfbuzz for text rendering in OpenGL.')
    opt.add_option('--use_pango', action='store_true', default=False,
                   help='Use Pango for text rendering library if found')
    repo = os.path.basename(os.getcwd())
    opt.add_option('--repo', action='store_true',
                   default=repo,
                   help='The repository name - top directory name')


def setup_init(ctx):
    # Added for uniformity
    pass

def setup_configure(ctx, enable_cxx=True):
    verify_current_dir()

    print ('>>>>>>>> 1618labs specific configuration configure <<<<<<<')
    ctx.env.common_impl = 'win32' if Options.platform == 'win32' else 'unix'
    Arch.SetOSType(ctx)

    ctx.android_configure = android_configure
    if enable_cxx:
        Arch.PrepareCxxContextEnv(ctx)

    if ctx.options.verbose:
        print ctx.env

def setup_build(ctx):
    verify_current_dir()
    setup_install_dirs(ctx)


    ## Add function to generate thrift library from .thrift file
    ctx.proto_lib = proto_lib

    ## Add cc_binary function to ctx, but disable for ios build
    ctx.cc_binary = ignore if ctx.env.TIZEN or ctx.env.IOS or ctx.env.ANDROID else cc_binary
    ctx.ignore = ignore

    ctx.output_config_file = output_config_file
    ctx.simple_binary = simple_binary
    ctx.nacl_binary = nacl_binary
    ctx.make_iterable = make_iterable

    ## Library building
    if not 'added_libs' in ctx.__dict__:
        ctx.added_libs = {}
    ctx.add_lib = add_lib
    ctx.build_top_lib = build_top_lib
    ctx.set_toplib_options = set_toplib_options
    ctx.add_toplib_options = add_toplib_options
    ctx.add_jnilib = add_jnilib
    ctx.android_build = android_build
    ctx.prepare_standard_deps = prepare_standard_deps

    # Install helpers
    ctx.install_all_includes = install_all_includes
    ctx.install_all_files = install_all_files


######################################################################
#
# Architecture specific confuguration
#
class Arch:
    def __init__(self):
        self.__call__ = True
    IOS_ARMV6 = 'ios-armv6'
    IOS_ARMV7 = 'ios-armv7'
    IOS_ARMV7S = 'ios-armv7s'
    IOS_ARM64 = 'ios-arm64'
    IOS_SIMULATOR = 'ios-i386'
    IOS_SIMULATOR64 = 'ios-x86_64'

    ANDROID_4_ARM = 'android-4-arm'
    ANDROID_4_x86 = 'android-4-x86'
    ANDROID_5_ARM = 'android-5-arm'
    ANDROID_5_x86 = 'android-5-x86'
    ANDROID_8_ARM5 = 'android-8-arm5'
    ANDROID_8_ARM = 'android-8-arm'
    ANDROID_8_x86 = 'android-8-x86'
    ANDROID_9_ARM = 'android-9-arm'
    ANDROID_9_x86 = 'android-9-x86'
    ANDROID_14_ARM = 'android-14-arm'
    ANDROID_14_x86 = 'android-14-x86'
    ANDROID_17_ARM = 'android-17-arm'
    ANDROID_17_x86 = 'android-17-x86'
    ANDROID_18_ARM = 'android-18-arm'
    ANDROID_18_x86 = 'android-18-x86'
    ANDROID_19_ARM = 'android-19-arm'
    ANDROID_19_x86 = 'android-19-x86'

    TIZEN_2_1_EMU = 'tizen-2.1-emu'
    TIZEN_2_1_DEV = 'tizen-2.1-dev'

    NACL_32_x86_64 = 'nacl-32-x86_64'
    NACL_32_x86_32 = 'nacl-32-x86_32'
    NACL_32_ARM = 'nacl-32-arm'

    DEFAULT = ''

    __IOS_PREFIX = 'ios'
    __ANDROID_PREFIX = 'android'
    __TIZEN_PREFIX = 'tizen'
    __NACL_PREFIX = 'nacl'

    __TIZEN_PLATFORM = {
        'emu': ('emulator', 'i386'),
        'dev': ('device', 'arm'),
        }

    __IOS_ARCH = {
        IOS_ARMV6     : ('armv6', 'iPhoneOS', '__ARM_ARCH_6__', 'arm'),
        IOS_ARMV7     : ('armv7', 'iPhoneOS', '__ARM_ARCH_7__', 'arm'),
        IOS_ARMV7S    : ('armv7s', 'iPhoneOS', '__ARM_ARCH_7__', 'arm'),
        IOS_ARM64     : ('arm64', 'iPhoneOS', '__ARM_64__', 'arm64'),
        IOS_SIMULATOR : ('i386', 'iPhoneSimulator', '__I386__', 'i386'),
        IOS_SIMULATOR64 : ('x86_64', 'iPhoneSimulator', '__X86_64__', 'x86_64'),
        }

    __TIZEN_ARCH = [
        TIZEN_2_1_EMU,
        TIZEN_2_1_DEV,
        ]
    __NACL_ARCH = {
        NACL_32_x86_32 : ('x86', 'pepper_32', 'i686', 'x86_32',
                          'i686-nacl/4.4.3'),
        NACL_32_x86_64 : ('x86', 'pepper_32', 'x86_64', 'x86_64',
                          'x86_64-nacl/4.4.3'),
        NACL_32_ARM : ('arm', 'pepper_32', 'arm', 'arm',
                       'arm-nacl/4.8.1'),
        }

    __ANDROID_ARCH = [
        ## We reduce these to the interesting intersection of what is
        ## available on NDK and on SDK.
        ## We have no android-9 available on SDK (it is declared obsolete),
        ## and on NDK we have 8-arm, 9 and 14. So we intersected to these 3.
        ## We default arm build to arm7a (arm5 instruction set also
        ## but not in this build).
        # ANDROID_8_ARM5,
        # ANDROID_8_ARM,
        ANDROID_14_ARM,
        ANDROID_14_x86,

        ANDROID_17_ARM,
        ANDROID_17_x86,
        ANDROID_18_ARM,
        ANDROID_18_ARM,
        ANDROID_19_x86,
        ANDROID_19_ARM,
        ]
    __ANDROID_PLATFORM = {
        'arm' : 'arm',
        'arm5' : 'arm',
        'x86' : 'x86',
        }

    @staticmethod
    def SetOSType(ctx):
        ctx.env.IOS = Arch.GetIosEnv(ctx) is not None
        ctx.env.ANDROID = Arch.GetAndroidEnv(ctx) is not None
        ctx.env.TIZEN = Arch.GetTizenEnv(ctx) is not None
        ctx.env.NACL = Arch.GetNaclEnv(ctx) is not None
        if ctx.env.IOS or ctx.env.ANDROID or ctx.env.TIZEN or ctx.env.NACL:
            ctx.env.MACOSX = False
            ctx.env.LINUX = False
        else:
            ctx.env.MACOSX = os.path.isdir('/System/Library/Frameworks/')
            ctx.env.LINUX = os.path.isfile('/proc/cpuinfo')
    @staticmethod
    def GetOutputDir(ctx, repo=None, arch=None):
        if arch is None:
            arch = ctx.options.arch
        if repo is None:
            repo = os.path.basename(get_current_repository_dir())
        if arch != Arch.DEFAULT:
            return os.path.join('..', 'wout.%s' % arch, repo)
        path = os.path.join('..', 'wout', repo)
        return path

    @staticmethod
    def GetProtoCompiler(ctx):
        return 'protoc'

    @staticmethod
    def GetIosEnv(ctx):
        if ctx.options.arch in  Arch.__IOS_ARCH:
            return Arch.__IOS_ARCH.get(ctx.options.arch)[1]
        return None

    @staticmethod
    def GetBinDir(ctx):
        if ctx.options.arch in  Arch.__IOS_ARCH:
            return Arch.__IOS_ARCH.get(ctx.options.arch)[3]
        return None

    @staticmethod
    def GetIosArch(ctx):
        if ctx.options.arch in  Arch.__IOS_ARCH:
            return Arch.__IOS_ARCH.get(ctx.options.arch)[0]
        return None

    @staticmethod
    def GetIosArchDefine(ctx):
        if ctx.options.arch in  Arch.__IOS_ARCH:
            return Arch.__IOS_ARCH.get(ctx.options.arch)[2]
        return None

    @staticmethod
    def GetOsxBin(ctx):
        if not ctx.env.MACOSX:
            return None
        bindir = '/Applications/Xcode.app/Contents/Developer/Toolchains/'\
               'XcodeDefault.xctoolchain/usr/bin/'
        if not bindir in os.environ['PATH'].split(':'):
            os.environ['PATH'] = os.environ['PATH'] + ':' + bindir
        return bindir

    @staticmethod
    def GetOsxFrameworkDir(ctx):
        if not ctx.env.MACOSX:
            return None
        framework_dir = '/System/Library/Frameworks/'
        ctx.env.LINKFLAGS.append('-L/usr/local/lib')
        if ctx.options.osxver:
            versions = [ctx.options.osxver]
        else:
            versions = ['10.9', '10.8','10.7']

        for ver in versions:
            path = '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX%s.sdk/System/Library/Frameworks' % ver
            if os.path.isdir(path):
                framework_dir = path
                ctx.env.CXXFLAGS.append('-I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX%s.sdk/usr/include/' % ver)
                ctx.env.CXXFLAGS.append('-I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX%s.sdk/usr/include/libxml2' % ver)
                ctx.env.CXXFLAGS.append('-I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX%s.sdk/usr/include/c++/4.2.1/' % ver)
                ctx.env.EXTRA_JNI_DIRS = ['/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX%s.sdk/System/Library/Frameworks/JavaVM.framework/Versions/A/Headers/' % ver]
                # ctx.env.CXXFLAGS.append('-I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX%s.sdk/usr/include/c++/4.2.1/backward/' % ver)
                ctx.env.CXXFLAGS.append('-Wno-bool-conversion')
                if ver == '10.9':
                    ctx.env.LINKFLAGS.append('-stdlib=libstdc++')  # -lc++ not binary compatible
                break
        return framework_dir

    @staticmethod
    def GetNaclEnv(ctx):
        parts = ctx.options.arch.split('-')
        if parts[0] != Arch.__NACL_PREFIX or len(parts) != 3:
            return None
        if os.path.isdir('/System/Library/Frameworks/'):
            ctx.env.NACL_HOST = 'mac'
        elif os.path.isfile('/proc/cpuinfo'):
            ctx.env.NACL_HOST = 'linux'
        else:
            return None

        ctx.env.NACL_ENV = Arch.__NACL_ARCH[ctx.options.arch][1]
        ctx.env.NACL_ARCH = Arch.__NACL_ARCH[ctx.options.arch][0]
        ctx.env.NACL_ARCH_PREFIX = Arch.__NACL_ARCH[ctx.options.arch][2]
        ctx.env.NACL_BIN_PREFIX = '%s-nacl-' % ctx.env.NACL_ARCH_PREFIX
        ctx.env.NACL_USR_PREFIX =  Arch.__NACL_ARCH[ctx.options.arch][3]
        ctx.env.NACL_LIB_PREFIX =  Arch.__NACL_ARCH[ctx.options.arch][4]

        return '%s/' % ctx.env.NACL_ENV

    @staticmethod
    def GetNaclToolchainDir(ctx):
        if not ctx.env.NACL_ENV:
            return None
        return '%s/%s/toolchain/%s_%s_%s/' % (
            ctx.options.nacl_sdk_path, ctx.env.NACL_ENV, ctx.env.NACL_HOST,
            ctx.env.NACL_ARCH, ctx.options.nacl_lib)

    @staticmethod
    def GetNaclDevRoot(ctx, name):
        return '%s/%s/toolchain/%s_%s_%s/%s-nacl' % (
            ctx.options.nacl_sdk_path, ctx.env.NACL_ENV,
            ctx.env.NACL_HOST, ctx.env.NACL_ARCH,
            name, ctx.env.NACL_USR_PREFIX)

    @staticmethod
    def GetTizenEnv(ctx):
        parts = ctx.options.arch.split('-')
        if parts[0] != Arch.__TIZEN_PREFIX or len(parts) != 3:
            return None
        ctx.env.TIZEN_ENV = Arch.__TIZEN_PLATFORM[parts[2]][0]
        ctx.env.TIZEN_ARCH = Arch.__TIZEN_PLATFORM[parts[2]][1]
        return 'tizen%s/rootstraps/tizen-%s-%s.native' % (
            parts[1], ctx.env.TIZEN_ENV, parts[1])

    @staticmethod
    def GetTizenGnuDir(ctx):
        parts = ctx.options.arch.split('-')
        if parts[0] != Arch.__TIZEN_PREFIX or len(parts) != 3:
            return None
        return '%s/tools/%s-linux-gnueabi-gcc-4.5/%s-linux-gnueabi/' % (
            ctx.options.tizen_sdk_path,
            ctx.env.TIZEN_ARCH, ctx.env.TIZEN_ARCH)

    @staticmethod
    def GetAndroidEnv(ctx):
        parts = ctx.options.arch.split('-')
        if parts[0] != Arch.__ANDROID_PREFIX or len(parts) != 3:
            return None
        ctx.env.ANDROID_ARCH = Arch.__ANDROID_PLATFORM[parts[2]]
        return '%s-%s/arch-%s' % (parts[0], parts[1],
                                  Arch.__ANDROID_PLATFORM[parts[2]])

    @staticmethod
    def GetAndroidTarget(ctx):
        parts = ctx.options.arch.split('-')
        if parts[0] != Arch.__ANDROID_PREFIX or len(parts) != 3:
            return None
        return '%s-%s' % (parts[0], parts[1])

    @staticmethod
    def GetAndroidArch(ctx):
        parts = ctx.options.arch.split('-')
        if parts[0] != Arch.__ANDROID_PREFIX or len(parts) != 3:
            return None
        return parts[2]

    @staticmethod
    def GetAndroidVersion(ctx):
        parts = ctx.options.arch.split('-')
        if parts[0] != Arch.__ANDROID_PREFIX or len(parts) != 3:
            return None
        return parts[1]

    @staticmethod
    def IsAndroidSupported(ctx):
        return ctx.options.arch in Arch.__ANDROID_ARCH

    @staticmethod
    def IsTizenSupported(ctx):
        return ctx.options.arch in Arch.__TIZEN_ARCH

    @staticmethod
    def ArchCanExecute(ctx):
        if Arch.GetAndroidArch(ctx):
            return False
        ios_arch = Arch.GetIosArch(ctx)
        if ios_arch:
            return ios_arch == 'i386'
        if Arch.GetTizenEnv(ctx):
            return False
        if Arch.GetNaclEnv(ctx):
            return False
        return True

    @staticmethod
    def GetBuildBinary(ctx, binary):
        if Arch.GetIosEnv(ctx):
            #bindir = '%s/usr/bin' % Arch.GetDevRoot(ctx)
            #if not bindir in os.environ['PATH'].split(':'):
            #    os.environ['PATH'] = os.environ['PATH'] + ':' + bindir
            #toolsdir = '/Applications/Xcode.app//Contents/Developer/'\
	    #		'Toolchains/XcodeDefault.xctoolchain/usr/bin/'
            #if not toolsdir in os.environ['PATH'].split(':'):
            #    os.environ['PATH'] = os.environ['PATH'] + ':' + toolsdir
            #cxx = '%s/%s' % (bindir, binary)
            #if os.path.exists(cxx):
            #    return cxx
            return '/Applications/Xcode.app/Contents/Developer/'\
                   'Toolchains/XcodeDefault.xctoolchain/usr/bin/' + binary

        if Arch.GetAndroidEnv(ctx):
            (dir, prefix, _) = Arch.GetAndroidToolsPrefix(ctx)
            bindir = '%s/bin' % dir
            if not bindir in os.environ['PATH'].split(':'):
                os.environ['PATH'] = os.environ['PATH'] + ':' + bindir
            return '%s/%s-%s' % (bindir, prefix, binary)

        if Arch.GetTizenEnv(ctx):
            (cc_dir, gcc_dir, usr_dir) = Arch.GetTizenToolsPrefix(ctx)
            if not cc_dir in os.environ['PATH'].split(':'):
                os.environ['PATH'] = os.environ['PATH'] + ':' + cc_dir
            return '%s/%s' % (cc_dir, binary)

        if Arch.GetNaclEnv(ctx):
            cc_dir = Arch.GetNaclToolchainDir(ctx)
            if not cc_dir in os.environ['PATH'].split(':'):
                os.environ['PATH'] = os.environ['PATH'] + ':' + cc_dir
            return '%s/bin/%s%s' % (cc_dir, ctx.env.NACL_BIN_PREFIX, binary)

        if ctx.env.MACOSX:
            return '%s/%s' % (Arch.GetOsxBin(ctx), binary)

        return binary

    @staticmethod
    def GetCxxCompiler(ctx):
        if ctx.env.IOS or ctx.env.TIZEN or ctx.env.MACOSX:
            return Arch.GetBuildBinary(ctx, 'clang++')
        if ctx.env.ANDROID or ctx.env.NACL:
            return Arch.GetBuildBinary(ctx, 'g++')
        return Arch.GetBuildBinary(ctx, ctx.options.cxx)

    @staticmethod
    def GetCCompiler(ctx):
        if ctx.env.IOS or ctx.env.TIZEN or ctx.env.MACOSX:
            return Arch.GetBuildBinary(ctx, 'clang')
        if ctx.env.ANDROID or ctx.env.NACL:
            return Arch.GetBuildBinary(ctx, 'gcc')
        return Arch.GetBuildBinary(ctx, ctx.options.c)

    @staticmethod
    def GetAssembler(ctx):
        if ctx.env.ANDROID or ctx.env.NACL:
            return Arch.GetBuildBinary(ctx, 'as')
        return Arch.GetBuildBinary(ctx, ctx.options.asm)

    @staticmethod
    def GetAr(ctx):
        #if ctx.env.IOS:
        #    return Arch.GetBuildBinary(ctx, 'libtool')
        return Arch.GetBuildBinary(ctx, 'ar')


    @staticmethod
    def GetDevRoot(ctx, arch=None):
        if arch is None:
            arch = ctx.options.arch
        if arch in Arch.__IOS_ARCH:
            dev = Arch.__IOS_ARCH[arch][1]
            PLATFORMS_PATHS = [
                '/Applications/Xcode.app/Contents/Developer/Platforms',
                '/Developer/Platforms',
                ]
            for pdir in PLATFORMS_PATHS:
                d = '%s/%s.platform/Developer' % (pdir, dev)
                if os.path.exists(d):
                    return d
            return '/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain'
        android_env = Arch.GetAndroidEnv(ctx)
        if android_env:
            return '%s/platforms/%s' % (
                ctx.options.android_ndk_path, android_env)
        if Arch.GetTizenEnv(ctx):
            (cc_dir, gcc_dir, usr_dir) = Arch.GetTizenToolsPrefix(ctx)
            return usr_dir
        nacl_env = Arch.GetNaclEnv(ctx)
        if nacl_env:
            return Arch.GetNaclDevRoot(ctx, ctx.options.nacl_lib)

        return '/'

    @staticmethod
    def GetTizenToolsPrefix(ctx):
        return ('%s/tools/llvm-3.1/bin/' % ctx.options.tizen_sdk_path,
                Arch.GetTizenGnuDir(ctx),
                "%s/platforms/%s/" % (ctx.options.tizen_sdk_path,
                                      Arch.GetTizenEnv(ctx)))

    @staticmethod
    def GetAndroidToolsPrefix(ctx):
        arch = Arch.GetAndroidArch(ctx)
        for system in ['darwin-x86', 'darwin-x86_64', 'linux-x86', 'linux-x86_64']:
            for ver in ['4.8', '4.7', '4.6']:
                path = '%s/toolchains/arm-linux-androideabi-%s/'\
                    'prebuilt/%s/bin' % (
                    ctx.options.android_ndk_path, ver, system)
                if os.path.isdir(path):
                    if arch == 'arm' or arch == 'arm5':
                        return ('%s/toolchains/arm-linux-androideabi-%s/'\
                                'prebuilt/%s' % (
                            ctx.options.android_ndk_path, ver, system),
                                'arm-linux-androideabi', ver)
                    else:
                        prefix = '%s/toolchains/x86-%s/prebuilt/%s' % (
                            ctx.options.android_ndk_path, ver, system)
                        if  os.path.isfile(os.path.join(
                                prefix, 'bin/i686-android-linux-g++')):
                            return (prefix, 'i686-android-linux', ver)
                        if  os.path.isfile(os.path.join(
                                prefix, 'bin/i686-linux-android-g++')):
                            return (prefix, 'i686-linux-android', ver)
                        ctx.fatal('Error - cannot find tools [%s]' % prefix)

        return (None, None, None)

    @staticmethod
    def GetInstallLibsSubDir(ctx):
        android_target = Arch.GetAndroidTarget(ctx)
        if android_target:
            return os.path.join(android_target, Arch.GetAndroidLibsSubDir(ctx))
        if Arch.GetIosEnv(ctx) is not None:
            ios_libs = Arch.GetIosLibsSubDir(ctx)
            if ios_libs:
                return '%s-%s' % (Arch.__IOS_PREFIX, ios_libs)

        if Arch.GetTizenEnv(ctx):
            return '%s-%s' % (Arch.__TIZEN_PREFIX,
                              ctx.env.TIZEN_ENV)

        if Arch.GetNaclEnv(ctx):
            return '%s-%s' % (Arch.__NACL_PREFIX,
                              ctx.env.NACL_ENV)
        return ''

    @staticmethod
    def GetIosLibsSubDir(ctx):
        return '%s' %  Arch.GetIosArch(ctx)

    @staticmethod
    def GetAndroidLibsSubDir(ctx):
        arch = Arch.GetAndroidArch(ctx)
        if arch == 'arm5':
            return 'armeabi'
        elif arch == 'arm':
            return 'armeabi-v7a'
        elif arch == 'x86':
            return 'x86'
        else:
            return 'local'

    @staticmethod
    def GetAndroidStlDirs(ctx):
        arch_libs = Arch.GetAndroidLibsSubDir(ctx)
        if not arch_libs:
            return (None, None)
        #return ('%s/sources/cxx-stl/stlport/stlport/' %  ctx.options.android_ndk_path,
        #        '%s/sources/cxx-stl/stlport/libs/%s' %
        #            (ctx.options.android_ndk_path, arch_libs))

        for version in ['4.8/', '4.7/', '4.6/']:
            dirs = ('%s/sources/cxx-stl/gnu-libstdc++/%sinclude'
                % (ctx.options.android_ndk_path, version),
                '%s/sources/cxx-stl/gnu-libstdc++/%slibs/%s'
                % (ctx.options.android_ndk_path, version, arch_libs))
            if os.path.isdir(dirs[0]) and os.path.isdir(dirs[1]):
                return dirs
        ctx.fatal('Cannot find NDK include / lib directories')

    @staticmethod
    def PrepareCxxCompiler(ctx):
        # Setup IOS basic path
        cxx = Arch.GetCxxCompiler(ctx)

        # Check for the CXX compiler
        try:
            ctx.find_program(cxx)
        except ctx.errors.ConfigurationError:
            ctx.fatal('cxx compiler "%s" not found' % cxx)

        c = Arch.GetCCompiler(ctx)
        # Check for the C compiler
        try:
            ctx.find_program(c)
        except ctx.errors.ConfigurationError:
            ctx.fatal('c compiler "%s" not found' % c)

        asm = Arch.GetAssembler(ctx)
        # Check for the ASM compiler
        try:
            ctx.find_program(asm)
        except ctx.errors.ConfigurationError:
            ctx.fatal('as assembler  "%s" not found' % asm)

        ar = Arch.GetAr(ctx)
        # Check for the AR archiver
        try:
            ctx.find_program(ar)
        except ctx.errors.ConfigurationError:
            ctx.fatal('as assembler  "%s" not found' % ar)

        ctx.env.CXX = cxx
        ctx.env.CC = c
        ctx.env.AS = asm
        ctx.env.AR = ar

        ctx.load('compiler_cxx')
        ctx.load('compiler_c')
        ctx.load('gas')
        ctx.load('bison')
        ctx.load('flex')

    @staticmethod
    def PrepareCxxContextEnv(ctx):
        # Detect the defines in this environment

        Arch.PrepareCxxCompiler(ctx)
        arch = ctx.options.arch
        if arch in [Arch.IOS_ARMV6, Arch.IOS_ARMV7, Arch.IOS_ARMV7S, Arch.IOS_ARM64]:
            Arch.PrepareIosArmEnv(ctx)
        elif arch in [Arch.IOS_SIMULATOR, Arch.IOS_SIMULATOR64]:
            Arch.PrepareIosSimEnv(ctx)
        elif (Arch.GetAndroidEnv(ctx) and
              (Arch.GetAndroidArch(ctx) == 'arm' or
               Arch.GetAndroidArch(ctx) == 'arm5')):
            Arch.PrepareAndroidArmEnv(ctx)
        elif Arch.GetAndroidEnv(ctx) and Arch.GetAndroidArch(ctx) == 'x86':
            Arch.PrepareAndroidx86Env(ctx)
        elif Arch.GetTizenEnv(ctx) and ctx.env.TIZEN_ARCH == 'arm':
            Arch.PrepareTizenArmEnv(ctx)
        elif Arch.GetTizenEnv(ctx) and ctx.env.TIZEN_ARCH == 'i386':
            Arch.PrepareTizenI386Env(ctx)
        elif Arch.GetNaclEnv(ctx):
            Arch.PrepareNaclEnv(ctx)
        else:
            if arch.startswith(Arch.__ANDROID_PREFIX):
                ctx.fatal('You probably misspelled the android architectures. '
                          'Choose one of: %s' % Arch.__ANDROID_ARCH)
            if arch.startswith(Arch.__IOS_PREFIX):
                ctx.fatal('You probably misspelled the iOS architecture '
                          'choose one of: %s' % Arch.__IOS_ARCH.keys())

            Arch.PrepareDefaultEnv(ctx)

        if ctx.env.LINUX:
	    (s, o) = commands.getstatusoutput('uname -p')
            print "----> Commands: %s / [%s]" % (s, o)
            if not s and o == 'x86_64':
                 ctx.env.LINUX_64 = True
                 ctx.env.DEFINES.append('__x86_64__')
                 # ctx.env.CXXFLAGS.append('-D__x86_64__')
            else:
                 ctx.env.LINUX_64 = False

        # Finishing touches
        ctx.env.CXXFLAGS.extend(['-Wstrict-aliasing'])
        if ctx.options.release:
            ctx.env.DEFINES.append('NDEBUG')
            copt = '-Os' if ctx.env.IOS else '-O2'
            ctx.env.CXXFLAGS.extend([copt])   # + ['-S', '-flto']
            ctx.env.LINKFLAGS.extend(['-O2'])
        else:
            ctx.env.DEFINES.append('DEBUG')
            ctx.env.CXXFLAGS.extend(['-g', '-O0'])
            ctx.env.LINKFLAGS.extend(['-g', '-O1'])

        if ctx.env.IOS:
            ctx.env.DEFINES.append('IOS')
        if ctx.env.ANDROID:
            ctx.env.DEFINES.append('ANDROID')
        if ctx.env.TIZEN:
            ctx.env.DEFINES.append('TIZEN')
        if ctx.env.NACL:
            ctx.env.DEFINES.append('NACL')
        if ctx.env.MACOSX:
            ctx.env.DEFINES.append('MACOSX')
        if ctx.env.LINUX:
            ctx.env.DEFINES.append('LINUX')
        # to enable openssl
        if not ctx.options.disable_ssl:
            ctx.env.USE_OPENSSL = True
            ctx.env.DEFINES.append('USE_OPENSSL')
            # ctx.env.CXXFLAGS.append('-DUSE_OPENSSL')
        if ctx.env.ANDROID or ctx.env.IOS or ctx.env.NACL:
            # or ctx.env.IOS:
            ctx.env.USE_GLUES = True
            # Remove some pieces from openssl when compiling for mobile
            ctx.env.DEFINES.extend(['OPENSSL_NO_ENGINE',
                                     'OPENSSL_NO_CMS',
                                     'OPENSSL_NO_EC',
                                     'OPENSSL_NO_ECDSA',
                                     'OPENSSL_NO_ECDH',
                                     'OPENSSL_NO_OCSP',
                                     'OPENSSL_NO_DH',
                                     'OPENSSL_NO_X509',
                                     ])
        if ctx.env.IOS:
            ctx.env.USE_OPENSSL = False

        if ctx.options.use_harfbuzz:
            ctx.env.USE_HARFBUZZ = True
            ctx.env.DEFINES.append('USE_HARFBUZZ')
        if ctx.env.USE_HARFBUZZ:
            ctx.env.USE_FREETYPE = True
            ctx.env.DEFINES.append('USE_FREETYPE')

        ctx.env.CXXFLAGS.append('-Wno-deprecated-declarations')
        ctx.env.CXXFLAGS.extend(['-D%s' % d for d in ctx.env.DEFINES])
        ctx.env.CFLAGS = []
        ctx.env.CFLAGS.extend(ctx.env.CXXFLAGS)
        ctx.env.CFLAGS.extend(['-x', 'c', '-Wno-unused-value'])
        for op in ['-fpermissive', '-frtti', '-fexceptions']:
            if op in ctx.env.CFLAGS:
                ctx.env.CFLAGS.remove(op)

        if not ctx.env.TIZEN and not ctx.env.NACL:
            ctx.env.CXXFLAGS.append('-std=c++0x')

    @staticmethod
    def PrepareDefaultEnv(ctx):
        ctx.env.EXTRA_JNI_DIRS = []
        ctx.env.CXXFLAGS = ['-I/usr/local/include/']
        # jni_dirs = ctx.options.jni_include_dirs.split(',')
        # ctx.env.CXXFLAGS.extend(['-I%s' % d for d in jni_dirs])
        if ctx.env.LINUX:
            ctx.env.CXXFLAGS.append('-I/usr/lib/jvm/java-6-openjdk/include/linux')
            ctx.env.CXXFLAGS.append('-I/usr/lib/jvm/java-7-oracle/include/linux')

        ctx.env.LINKFLAGS = []
        if ctx.env.LINUX:
            ctx.env.CXXFLAGS.append('-I/usr/include/GL/')
            ctx.env.CXXFLAGS.append('-I/usr/include/libxml2/')

        if ctx.env.MACOSX:
            framework_dir = Arch.GetOsxFrameworkDir(ctx)

            ctx.env.CXXFLAGS.append('-F%s' % framework_dir)
            ctx.env.CXXFLAGS.append('-I%s/OpenGL.framework/Headers/' % framework_dir)
            ctx.env.CXXFLAGS.append('-I/usr/include/GL/')
            ctx.env.LINKFLAGS.append('-F%s' % framework_dir)
            # ctx.env.CXXFLAGS.extend(['-I%s/darwin/' % d for d in jni_dirs])
        ctx.env.CXXFLAGS.append('-fPIC')
        configure_cxx_defines(ctx)

        if ctx.env.LINUX:
            ctx.env.USE_GOOGLE_PERFTOOLS = True
            ctx.env.DEFINES.append('USE_GOOGLE_PERFTOOLS')
        else:
            ctx.env.USE_GOOGLE_PERFTOOLS = False


#        if ctx.options.use_icu or ctx.options.use_harfbuzz:
        if ctx.options.use_icu:
            (icu_cppflags, icu_ldflags) = get_icu_config(ctx)
#            if icu_cppflags:
#                ctx.env.CXXFLAGS.extend(Utils.to_list(icu_cppflags))
#            if icu_ldflags:
#                ctx.env.LINKFLAGS.extend(Utils.to_list(icu_ldflags))

            if icu_cppflags and icu_ldflags:
                ctx.env.ICU_CXXFLAGS = icu_cppflags
                ctx.env.ICU_LINKFLAGS = icu_ldflags
                ctx.env.HAVE_ICU = True
                ctx.env.USE_ICU = True
                ctx.env.DEFINES.append('HAVE_ICU')
                ctx.env.DEFINES.append('USE_ICU')

        if ctx.options.use_pango:
            (pango_cppflags, pango_ldflags) = get_pkg_config(ctx, 'pango')
            (ft2_cppflags, ft2_ldflags) = get_pkg_config(ctx, 'freetype2')
            (cairo_cppflags, cairo_ldflags) = get_pkg_config(ctx, 'cairo')
            if (pango_cppflags and pango_ldflags
                and ft2_cppflags and ft2_ldflags
                and cairo_cppflags and cairo_ldflags):
                ctx.env.PANGO_CXXFLAGS = ft2_cppflags.split()
                ctx.env.PANGO_CXXFLAGS.extend(cairo_cppflags.split())
                ctx.env.PANGO_CXXFLAGS.extend(pango_cppflags.split())
                ctx.env.PANGO_LINKFLAGS = ft2_ldflags.split()
                ctx.env.PANGO_LINKFLAGS.extend(cairo_ldflags.split())
                ctx.env.PANGO_LINKFLAGS.extend(pango_ldflags.split())
                ctx.env.HAVE_PANGO = True
                ctx.env.USE_PANGO = True
                ctx.env.DEFINES.append('HAVE_PANGO')
                ctx.env.DEFINES.append('USE_PANGO')
            else:
                if (not pango_cppflags or not pango_ldflags):
                    sys.exit("Pango is not installed, please check your setup - "
                             "Pango disabled")
                if (not ft2_cppflags or not ft2_ldflags):
                    sys.exit("Pango needs freetype2, please check your setup - "
                             "Pango disabled")
                if (not cairo_cppflags or not cairo_ldflags):
                    sys.exit("Pango needs cairo, please check your setup - "
                             "Pango disabled")


        ctx.env.LINKFLAGS.append(
            '-L%s' % os.path.abspath(Arch.GetOutputDir(ctx, ctx.options.repo)))

        if ctx.env.LINUX:
            ctx.env.CXXFLAGS.append('-lrt')
            ctx.env.CXXFLAGS.append('-fpermissive')

        if ctx.env.USE_GOOGLE_PERFTOOLS and ctx.env.HAVE_GOOGLE_PERFTOOLS:
            ctx.env.CXXFLAGS.extend(['-fno-builtin-malloc',
                                     '-fno-builtin-calloc',
                                     '-fno-builtin-realloc',
                                     '-fno-builtin-free'])

        ctx.env.USE_GLOG_LOGGING = True
        ctx.env.USE_GFLAGS = True
        ctx.env.DEFINES.append('USE_GLOG_LOGGING')
        ctx.env.DEFINES.append('USE_GFLAGS')
        # From google-perftools instruction:
        # ctx.env.CXXFLAGS = Utils.to_list(ctx.env.CXXFLAGS) +
        #               ['-Wl,--eh-frame-hdr']


    @staticmethod
    def PrepareTizenArmEnv(ctx):
        Arch.PrepareTizenCommonEnv(ctx)
        ctx.env.CXXFLAGS.extend(
            ['-target',  'arm-tizen-linux-gnueabi',
             '-ccc-gcc-name', 'arm-linux-gnueabi-g++',
             '-march=armv7-a',
             ])
        ctx.env.LINKFLAGS.extend([
            '-ccc-gcc-name', 'arm-linux-gnueabi-g++',
            '-march=armv7-a',
            '-target',  'arm-tizen-linux-gnueabi',
            ])
        configure_cxx_defines(ctx)


    @staticmethod
    def PrepareTizenI386Env(ctx):
        Arch.PrepareTizenCommonEnv(ctx)
        ctx.env.CXXFLAGS.extend(
            ['-target',  'i386-tizen-linux-gnueabi',
             '-ccc-gcc-name', 'i386-linux-gnueabi-g++',
             '-march=i386',
             ])
        ctx.env.LINKFLAGS.extend([
            '-ccc-gcc-name', 'i386-linux-gnueabi-g++',
            '-march=i386',
            '-target',  'i386-tizen-linux-gnueabi'
            ])
        configure_cxx_defines(ctx)

    @staticmethod
    def PrepareTizenCommonEnv(ctx):
        """ Common Ios environment preparation """
        if not Arch.IsTizenSupported(ctx):
            ctx.fatal('Unsupported tizen arch, try one of: %s' %
                      Arch.__TIZEN_ARCH)
        ctx.env.CXXFLAGS = [
            '-c',
            '-fmessage-length=0',
            '-Wall',
            '-gcc-toolchain',
            Arch.GetTizenGnuDir(ctx) + '/../',
            '-Wno-gnu',
            '-Wno-overloaded-virtual',   # Clucene does a lot of that
            '-fPIE',
            '-Wno-c++11-extensions',
            '-D_APP_LOG', '-MMD',  '-MP',   ##  ???
            '--sysroot=%s/platforms/%s' % (
              ctx.options.tizen_sdk_path,
              Arch.GetTizenEnv(ctx)),
            '-I%s/platforms/%s/usr/include' % (
              ctx.options.tizen_sdk_path,
              Arch.GetTizenEnv(ctx)),
            '-I%s/platforms/%s/usr/include/osp' % (
              ctx.options.tizen_sdk_path,
              Arch.GetTizenEnv(ctx)),
            '-I%s/library' % (
              ctx.options.tizen_sdk_path),
            ]
        ctx.env.LD = '%s/bin/ld' % Arch.GetTizenGnuDir(ctx)

        # ctx.env.LINK_CXX = '%s/bin/g++' % Arch.GetTizenGnuDir(ctx)
        ctx.env.LINK_CXX = Arch.GetCxxCompiler(ctx)

        ctx.env.LINKFLAGS_cxxshlib = ['-shared',
                                      '-lgnustl_shared']
        ctx.env.LINKFLAGS_cxxstlib = ['-lgnustl_static']

        ctx.env.SHLIB_MARKER = '-shared'
        ctx.env.STLIB_MARKER = ''
        ctx.env.LINKFLAGS = [
            '-gcc-toolchain',
            Arch.GetTizenGnuDir(ctx) + '/../',
            '-Xlinker', '--as-needed',
            '--sysroot=%s/platforms/%s' % (
              ctx.options.tizen_sdk_path,
              Arch.GetTizenEnv(ctx)),
            # '-Wl,--as-needed',
            '-pie',
            '-L%s/platforms/%s/usr/lib' % (
              ctx.options.tizen_sdk_path,
              Arch.GetTizenEnv(ctx)),
            '-L%s/platforms/%s/usr/lib/osp' % (
              ctx.options.tizen_sdk_path,
              Arch.GetTizenEnv(ctx)),
            '-L%s/lib' % Arch.GetDevRoot(ctx),
            ]

        # -L"/Users/cpopescu/workspace/CubeTest/lib"
        # -target i386-tizen-linux-gnueabi
        # -gcc-toolchain /Users/cpopescu/tizen-sdk/tools/smart-build-interface/../i386-linux-gnueabi-gcc-4.5/
        # -ccc-gcc-name i386-linux-gnueabi-g++
        # -march=i386

        # -Xlinker --as-needed -pie -lpthread
        # -Xlinker -rpath="/opt/usr/apps/t5lBJFGcWp/lib"
        # -Xlinker -rpath="/home/developer/sdk_tools/lib"
        # --sysroot="/Users/cpopescu/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native"
        # -L"/Users/cpopescu/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/lib"
        # -L"/Users/cpopescu/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/lib/osp"
        # -lpthread
        # -losp-compat -losp-uifw -losp-appfw -losp-image -losp-json -losp-ime -losp-net
        # -losp-content -losp-locations -losp-telephony -losp-uix -losp-media -losp-messaging -losp-web
        # -losp-social -losp-wifi -losp-bluetooth -losp-nfc -losp-face -losp-speech-tts -losp-speech-stt
        # -losp-shell -losp-shell-core -losp-vision
        # -lxml2

# clang++ -o"CubeTest.exe"  ./src/GlesCube11.o ./src/GlesCube11Entry.o ./src/ImageData1.o ./src/ImageData2.o    -L"/Users/cpopescu/workspace/CubeTest/lib" -target i386-tizen-linux-gnueabi -gcc-toolchain /Users/cpopescu/tizen-sdk/tools/smart-build-interface/../i386-linux-gnueabi-gcc-4.5/ -ccc-gcc-name i386-linux-gnueabi-g++ -march=i386 -Xlinker --as-needed -pie -lpthread -Xlinker -rpath="/opt/usr/apps/t5lBJFGcWp/lib" -Xlinker -rpath="/home/developer/sdk_tools/lib" --sysroot="/Users/cpopescu/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native" -L"/Users/cpopescu/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/lib" -L"/Users/cpopescu/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/lib/osp" -losp-uifw -losp-appfw -losp-image -losp-json -losp-ime -losp-net -lpthread -losp-content -losp-locations -losp-telephony -losp-uix -losp-media -losp-messaging -losp-web -losp-social -losp-wifi -losp-bluetooth -losp-nfc -losp-face -losp-speech-tts -losp-speech-stt -losp-shell -losp-shell-core -losp-vision -lxml2 -losp-compat

        ### TODO(cp)

    @staticmethod
    def PrepareNaclEnv(ctx):
        ctx.env.CXXFLAGS = [
            '-DPPAPI=1',
            '-DSYS_PPAPI=1',
            '-D__USE_LEAN_SELECTOR__=1',

#            '-fomit-frame-pointer',
#            '-Wno-constant-logical-operand',

            '-c',
            '-pthread',
            '-fmessage-length=0',
            '-Wall',
            '-Wno-comment',
#            '-fPIE',
            '-fPIC',
#            '-MMD',
            '-I%s/include' % Arch.GetDevRoot(ctx),
#            '-I%s/%s/include/newlib' % (ctx.options.nacl_sdk_path, ctx.env.NACL_ENV),
            '-I%s/%s/ports/include' % (ctx.options.nacl_sdk_path, ctx.env.NACL_ENV),
            '-I%s/%s/include' % (ctx.options.nacl_sdk_path, ctx.env.NACL_ENV),
            '-I%s/include' % (ctx.options.regal_sdk_path),
            '-Wno-sign-compare',
        ]
        if ctx.options.nacl_lib == 'newlib':
            ctx.env.CXXFLAGS.append('-I%s/%s/include/newlib' % (
                ctx.options.nacl_sdk_path, ctx.env.NACL_ENV))

        ctx.env.LD = Arch.GetBuildBinary(ctx, 'ld')

        # ctx.env.LINK_CXX =  Arch.GetBuildBinary(ctx, 'c++')
        ctx.env.LINK_CXX = Arch.GetCxxCompiler(ctx)

        ctx.env.LINKFLAGS_cxxshlib = ['-shared',
                                      '-lgnustl_shared']
        ctx.env.LINKFLAGS_cxxstlib = ['-lgnustl_static']

        ctx.env.SHLIB_MARKER = '' # -shared'
        ctx.env.STLIB_MARKER = ''
        if ctx.options.release:
            libtype = 'Release'
        else:
            libtype = 'Debug'
        ctx.env.LINKFLAGS = [
            '-L%s/lib' % (Arch.GetDevRoot(ctx)),
            '-L%s/../lib/gcc/%s' % (Arch.GetDevRoot(ctx), ctx.env.NACL_LIB_PREFIX),
            '-L%s/lib/gcc/%s' % (Arch.GetDevRoot(ctx), ctx.env.NACL_LIB_PREFIX),
            '-L%s/%s/ports/lib/%s_%s/%s' % (ctx.options.nacl_sdk_path, ctx.env.NACL_ENV,
                                            ctx.options.nacl_lib,
                                            ctx.env.NACL_USR_PREFIX, libtype),
            '-L%s/%s/lib/%s_%s/%s' % (ctx.options.nacl_sdk_path, ctx.env.NACL_ENV,
                                      ctx.options.nacl_lib,
                                      ctx.env.NACL_USR_PREFIX, libtype),
            '-L%s/lib/nacl-%s' % (ctx.options.regal_sdk_path, ctx.env.NACL_ARCH_PREFIX),
            ]
        if ctx.env.NACL_USR_PREFIX == 'x86_32':
            ctx.env.CXXFLAGS.append('-m32')
            ctx.env.CXXFLAGS.append(
                '-I%s/x86_64-nacl/include/' % os.path.dirname(Arch.GetDevRoot(ctx)))
        elif ctx.env.NACL_USR_PREFIX == 'x86_64':
            ctx.env.CXXFLAGS.append('-m64')
            ctx.env.CXXFLAGS.append(
                '-I%s/x86_64-nacl/include/' % os.path.dirname(Arch.GetDevRoot(ctx)))
        else:
            ctx.env.CXXFLAGS.append('-marm')
            ctx.env.CXXFLAGS.append('-Wno-unused-local-typedefs')
            ctx.env.CXXFLAGS.append(
                '-I%s/arm-nacl/include/' % os.path.dirname(Arch.GetDevRoot(ctx)))

        configure_cxx_defines(ctx)
        ctx.env.CXXFLAGS.append('-MMD')
        ctx.env.CXXFLAGS.append('-fpermissive')

    @staticmethod
    def PrepareAndroidArmEnv(ctx):
        Arch.PrepareAndroidCommonEnv(ctx)
        ver = Arch.GetAndroidVersion(ctx)
        arm_v5_defs = [
            '-D__ARM_ARCH_5__',
            '-D__ARM_ARCH_5T__',
            '-D__ARM_ARCH_5E__',
            '-D__ARM_ARCH_5TE__',
            '-march=armv5te']
        arm_v7_defs = [
            '-D__ARM_ARCH_7__',
            '-D__ARM_ARCH_7A__',
            '-march=armv7-a']
        if (Arch.GetAndroidArch(ctx) == 'arm5'):
            # Theoretically this should work, practically has issues.
            # Now disabled w/ accepted versions.
            ctx.env.CXXFLAGS.extend(arm_v5_defs)
        else:
            ctx.env.CXXFLAGS.extend(arm_v7_defs)
        ctx.env.CXXFLAGS.extend([
            '-fpic',
            '-fstack-protector',
            '-fno-strict-aliasing',
            '-finline-limit=64',
            '-fpermissive',
            '-Wno-psabi',
            '-mthumb',
            '-mtune=xscale',
            '-msoft-float',
            '-O2',
            ])
        configure_cxx_defines(ctx)

        # Need to re-enable these now, after the defines check
        ctx.env.CXXFLAGS.extend([
            '-fexceptions',
            '-frtti',
            ])

    @staticmethod
    def PrepareAndroidx86Env(ctx):
        Arch.PrepareAndroidCommonEnv(ctx)
        ctx.env.CXXFLAGS.extend([
            '-O2',
            '-fstrict-aliasing',
            '-funswitch-loops',
            '-finline-limit=300',
            ])
        configure_cxx_defines(ctx)
        # Need to re-enable these now, after the defines check
        ctx.env.CXXFLAGS.extend([
            '-fexceptions',
            '-frtti',
            ])

    @staticmethod
    def PrepareAndroidCommonEnv(ctx):
        """ Common Ios environment preparation """
        if not Arch.IsAndroidSupported(ctx):
            ctx.fatal('Unsupported android arch, try one of: %s' %
                      Arch.__ANDROID_ARCH)
        ctx.env.ANDROID_ARCH = Arch.GetAndroidArch(ctx)

        ctx.env.ANDROID_NDKROOT = Arch.GetDevRoot(ctx)
        (ctx.env.ANDROID_TOOLSDIR,
         ctx.env.ANDROID_TOOLSPREFIX,
         ctx.env.ANDROID_TOOLSVER) = Arch.GetAndroidToolsPrefix(ctx)
        ctx.env.EXTRA_JNI_DIRS = [os.path.join(ctx.env.ANDROID_NDKROOT, 'usr', 'include')]

        if not ctx.env.ANDROID_TOOLSDIR:
            ctx.fatal('Android tool root "%s" not found' % cxx)

        ver = Arch.GetAndroidVersion(ctx)
        (stl_inc_dir, stl_lib_dir) = Arch.GetAndroidStlDirs(ctx)
        # Compiler and linker flags
        ctx.env.CXXFLAGS = [
            '-MMD', '-MP', # -MF ...
            '-ffunction-sections',
            '-funwind-tables',
            '-fomit-frame-pointer',
            '-fno-exceptions',
            '-fPIC',


            '-I%s/usr/include/' % ctx.env.ANDROID_NDKROOT,
            '-I%s/usr/include/GLES' % ctx.env.ANDROID_NDKROOT,
            '-I%s' % stl_inc_dir,
            '-I%s/include' % stl_lib_dir,   # Don't ask why

            # Need to define these this way. For some reason
            # ctx.env.DEFINES.append does not work.
            # '-DWHISPER_DATA_MUTEX_POOL_SIZE=25',
            # '-DANDROID',
            # '-D__STDC_INT64__',
            ]
        ctx.env.DEFINES.append('__STDC_INT64__')
        ctx.env.DEFINES.append('WHISPER_DATA_MUTEX_POOL_SIZE=25')

        ctx.env.LINKFLAGS = [
            '-Wl,--gc-sections',
            '-Wl,-z,nocopyreloc',
            '-Wl,--no-undefined',
            '-Wl,-z,noexecstack',
            '-rdynamic',

            '--sysroot=%s' % ctx.env.ANDROID_NDKROOT,
            '-L%s/usr/lib' % ctx.env.ANDROID_NDKROOT,
            '-L%s' % stl_lib_dir,
            '-llog',
            '-landroid',
            '-ljnigraphics',
            ]
        ctx.env.THREAD_LIB_TO_USE = 'c'
        ctx.env.LINKFLAGS_cxxshlib = ['-shared', '-lgnustl_shared']
        ctx.env.LINKFLAGS_cxxstlib = ['-lgnustl_static']
        # '-lgnustl_shared']   # for some reason this gets bad

        ctx.env.LINK_CXX = '%s/bin/%s-g++' % (ctx.env.ANDROID_TOOLSDIR,
                                              ctx.env.ANDROID_TOOLSPREFIX)
        ctx.env.LD = '%s/bin/%s-ld' % (ctx.env.ANDROID_TOOLSDIR,
                                       ctx.env.ANDROID_TOOLSPREFIX)

        ctx.env.SHLIB_MARKER = '-shared'
        ctx.env.STLIB_MARKER = ''

    @staticmethod
    def PrepareIosArmEnv(ctx):
        # First we prepare in simulator mode to obtain defines
        Arch.PrepareIosEnvForArchitecture(ctx, Arch.IOS_SIMULATOR)
        # Get the defines - actual ones
        configure_cxx_defines(ctx)
        # Prepare in processor native architecture
        Arch.PrepareIosEnvForArchitecture(ctx, ctx.options.arch)
        Arch.AppendPostConfigIosFlags(ctx)

    @staticmethod
    def PrepareIosSimEnv(ctx):
        # Prepare.
        Arch.PrepareIosEnvForArchitecture(ctx, ctx.options.arch)
        # Get the defines
        configure_cxx_defines(ctx)
        Arch.AppendPostConfigIosFlags(ctx)

    @staticmethod
    def AppendPostConfigIosFlags(ctx):
        ctx.env.CXXFLAGS.append('-F%s/System/Library/Frameworks/' % ctx.env.SDKROOT)
        ctx.env.LINKFLAGS.append('-F%s/System/Library/Frameworks/' % ctx.env.SDKROOT)
        ctx.env.LINK_CXX = ctx.env.DEVROOT + '/usr/bin/ld'
        ctx.env.LINKFLAGS_cxxshlib = ['-dylib']

    @staticmethod
    def PrepareIosEnvForArchitecture(ctx, arch):
        """ Common Ios environment preparation """
        ctx.env.IOS_ARCH = Arch.GetIosArch(ctx)
        ctx.env.DEVROOT = Arch.GetDevRoot(ctx, arch)

        # Find SDK directory to setup SDKROOT
        min_ver = '5.0'  # minimum ios version
        for ver in ['5.0', '5.1', '6.0', '6.1', '7.0', '7.1']:
            path = '{0}/SDKs/{1}{2}.sdk'.format(
                ctx.env.DEVROOT, Arch.__IOS_ARCH[arch][1], ver)
            if os.path.isdir(path):  break

        ctx.env.SDKROOT = path
        # Compiler and linker flags
        cpu_arch = Arch.__IOS_ARCH[arch][0]
        cflags = '-arch %s -miphoneos-version-min=%s -isysroot %s ' % (  # -pipe
            cpu_arch, min_ver, ctx.env.SDKROOT)
        ctx.env.CXXFLAGS = Utils.to_list(cflags)  # -std=c++0x -no-cpp-precomp
        ctx.env.CXXFLAGS.append('-I%s/usr/include/' % ctx.env.SDKROOT)
        ctx.env.CXXFLAGS.append(
            '-I%s/System/Library/Frameworks/OpenGLES.framework/Headers/ES1'
            % ctx.env.SDKROOT)

        ctx.env.CXXFLAGS.extend(['-x', 'c++',
                                 '-fmessage-length=0',
                                 '-stdlib=libstdc++'
                                 ])
        ctx.env.DEFINES.append('WHISPER_DATA_MUTEX_POOL_SIZE=25')
        ctx.env.DEFINES.append(Arch.GetIosArchDefine(ctx))

        ctx.env.ASFLAGS = ['-arch', Arch.__IOS_ARCH[arch][0]]
        ctx.env.LINKFLAGS = ['-arch', Arch.__IOS_ARCH[arch][0]]
        ctx.env.LINKFLAGS.append(
            '-L%s' % os.path.abspath(Arch.GetOutputDir(ctx, ctx.options.repo)))

        if arch != Arch.IOS_SIMULATOR and arch != Arch.IOS_SIMULATOR64:
            if arch != Arch.IOS_ARM64:
                ctx.env.CXXFLAGS.append('-mthumb')

            ctx.env.LINKFLAGS.extend(['-syslibroot', ctx.env.SDKROOT])

        #ctx.env.ARFLAGS = "-rcs"
        #ctx.env.ARFLAGS=['-static',
        #                 '-arch_only',
        #                 Arch.__IOS_ARCH[arch][0],
        #                 '-syslibroot',
        #                 ctx.env.SDKROOT,
        #                 '-L%s' % os.path.abspath(Arch.GetOutputDir(ctx, 'core'))]
        #ctx.env.AR_TGT_F = '-o'
        #ctx.env.AR_SRC_F = ''


def prepare_standard_deps(ctx, **kwargs):
    toplib = kwargs.get('toplib', None)
    if not toplib:
        ctx.fatal('Please specify toplib')

    deps = kwargs.get('deps', [])
    use= kwargs.get('use', [])

    framework = []
    lib = []
    lib.append('m')
    if not ctx.env.TIZEN:
        lib.append('z')

    if ctx.env.THREAD_LIB_TO_USE:
        lib.append(ctx.env.THREAD_LIB_TO_USE)
    else:
        lib.append('pthread')

    if ctx.env.USE_GLOG_LOGGING and ctx.env.HAVE_GLOG:
        lib.append('glog')
    if ctx.env.USE_GFLAGS and ctx.env.HAVE_GFLAGS:
        lib.append('gflags')
    if (ctx.env.ANDROID or ctx.env.IOS) and not ctx.options.disable_ssl:
        use.append('openssl')

    if 'gl' in deps:
        if ctx.env.ANDROID:
            lib.append('GLESv1_CM')  # EGL ??
            lib.append('GLESv2')  # EGL ??
        elif not ctx.env.IOS and not ctx.env.MACOSX:
            lib.append('GL')

        if ctx.env.LINUX:
            lib.append('GLEW')

        if ctx.env.IOS:
            framework.append('OpenGLES')

    if not ctx.env.ANDROID:
        lib.append('stdc++')
    # Else we append differently lib.append('gnustl_static')
    # else:

    if ctx.env.IOS:
        framework.extend(['ImageIO',
                          'MobileCoreServices',
                          'CoreText',
                          'CoreFoundation',
                          'CoreGraphics'])
    elif ctx.env.MACOSX:
        framework.extend(["Cocoa"])


    print "=========== USE FOR: ", toplib, " -- ", use

    ctx.add_lib(ctx,
                toplib = toplib,
                name = toplib + 'lib.root',
                target = toplib,
                use = use,
                framework = framework,
                lib = lib)


######################################################################
#
# Library building (top level / dynamic)
#
def add_lib(ctx, **kwargs):
    """
    Use this in sub-directory wscript to add files to a top level library.
    This should be used as a regular ctx.stlib(), which is to be replaced by
    this function for 1618labs purpose, with some extra options:
    - toplib - library(es) to add the specifications for.
    - toponly - default True if toplib is specified, build just the
      top level lib, skip the static, sub-directory, library build
      (highly recommended)
    """
    toplibs = kwargs.get('toplib', None)
    toponly = kwargs.get('toponly', toplibs != None)
    if ctx.env.NACL:
        target = kwargs.get('target', None)
        if not target:
            target += '_' + ctx.env.NACL_USR_PREFIX
            target = kwargs.get('target', target)
    targets = []
    if toplibs is not None:
        del kwargs['toplib']
        for toplib in make_iterable(toplibs):
            add_toplib_options(ctx, toplib, **kwargs)
            targets.extend([v for (k, v) in kwargs.iteritems()
                           if k == 'target'])

    if not toponly:
        ctx.stlib(**kwargs)
    elif targets:
        print 'Top only lib for targets: %s => toplib %s' % (
            ', '.join(targets), ', '.join(make_iterable(toplibs)))

def add_toplib_options(ctx, toplib, **kwargs):
    __change_toplib_options(ctx, toplib, False, **kwargs)

def set_toplib_options(ctx, toplib, **kwargs):
    __change_toplib_options(ctx, toplib, True, **kwargs)

def __change_toplib_options(ctx, toplib, replace, **kwargs):
    current_path = ctx.path.abspath()[len(ctx.srcnode.abspath()) + 1:]
    if toplib in ctx.added_libs:
        lib_args = ctx.added_libs[toplib]
    else:
        lib_args = ctx.added_libs[toplib] = {}
    for (k, v) in kwargs.iteritems():
        if k == 'source':
            new_options = set([os.path.join(current_path, src)
                               if isinstance(src, str) else src
                               for src in make_iterable(v)])
        else:
            new_options = set(make_iterable(v))
        if replace or k not in lib_args:
            lib_args[k] = new_options
        else:
            lib_args[k] |= new_options


def simple_binary(ctx, files, prefix='',
                  extra_use = [], extra_lib = [], extra_framework=[],
                  install_bin = False):
    if ctx.env.ANDROID or ctx.env.IOS or ctx.env.TIZEN or ctx.env.NACL:
        return
    libs = ['z', 'm', 'pthread', 'dl']
    use = []
    frameworks = []
    if ctx.env.HAVE_GFLAGS:
        libs.append('gflags')
    if ctx.env.HAVE_GLOG:
        libs.append('glog')
    # Only use tcmalloc for Linux.  tcmalloc might have recent bug for OSX build
    if ctx.env.HAVE_GOOGLE_PERFTOOLS and ctx.env.USE_GOOGLE_PERFTOOLS:
        libs.append('tcmalloc')

    if ctx.env.MACOSX:
        frameworks = ["Cocoa", "OpenGL"]
        if ctx.env.USE_OPENSSL:
            libs.extend(['ssl', 'crypto'])
    if ctx.env.LINUX:
        libs.extend(['GL', 'GLEW'])
        if ctx.env.USE_OPENSSL:
            libs.extend(['ssl', 'crypto'])

    cxxflags = []
    if ctx.env.HAVE_PANGO and ctx.env.USE_PANGO:
        cxxflags.extend(ctx.env.PANGO_CXXFLAGS)
        libs.extend(['gobject-2.0', 'glib-2.0', 'cairo',
                     'pango-1.0', 'pangocairo-1.0'])
    if install_bin:
        install_path = ctx.env.INSTALL_BINDIR
    else:
        install_path = None
    ctx.cc_binary(
             ctx,
             target = files[0].split('.')[0],
             source = ['%s%s' % (prefix, t) for t in make_iterable(files)],
             use = ['whisperlib_includes',
                    'whisperlib'
                    ] + use + extra_use,
             framework = frameworks + extra_framework,
             lib = extra_lib + libs,
             libpath = [ctx.path.get_bld().abspath()],
             install_path = install_path,
             cxxflags = cxxflags,
             )


def nacl_binary(ctx, files, prefix='', extra_use = [], extra_lib = []):
    if not ctx.env.NACL:
        return
    libs = ['z', 'm', 'pthread', 'RegalGLUlib', 'Regallib', 'freetype',
            'ppapi', 'ppapi_cpp', 'ppapi_gles2', 'nacl_io']
    if ctx.options.nacl_lib == 'glibc':
        libs.append('dl')


    use = []
    bin_name = files[0].split('.')[0]
    target = '%s_%s.nexe' % (bin_name, ctx.env.NACL_USR_PREFIX)
#    extra_map_file =  '%s/%s_%s.map' % \
#                     (ctx.path.get_bld().abspath(), bin_name, ctx.env.NACL_USR_PREFIX)
    ctx.cc_binary(
        ctx,
        target = target,
        source = ['%s%s' % (prefix, t) for t in make_iterable(files)],
        use = ['core_includes',
               'core'
               ] + use + extra_use,
        # framework = frameworks + extra_framework,
        lib = libs + extra_lib,
        libpath = [ctx.path.get_bld().abspath()],
        install_path = None,
#        extra_map_file = extra_map_file,
        )

def build_top_lib(ctx, toplibs, lang='c++'):
    """
    In top level directories, use this to build libraries that have
    sources collected from its sub-directories, for the desired toplibs.
    You must have a <toplib>.cc stub file defined.
    """
    if ctx.cmd != 'build' and ctx.cmd != 'install':
        return
    for toplib in make_iterable(toplibs):
        lib_args = ctx.added_libs.get(toplib, None)
        if lib_args is None:
            raise ('Cannot build unknown toplib %s, make sure you used \n'
                   '  ctx.add_lib(toplib = "%s", ...) in one of your wscript'
                   % toplib)
        # Remove the name from uses:
        uses = lib_args.get('use', set())
        print 'Toplib %s: Uses: %s, Libs: %s' % (toplib, uses, lib_args.get('lib', None))
        names = lib_args.get('name', set())
        uses -= names

        lib_args_copy = {}
        for (k, v) in lib_args.iteritems():
            list_value = [elem for elem in v]
            if len(list_value) == 1:
                lib_args_copy[k] = list_value[0]
            else:
                lib_args_copy[k] = list_value

        # name confuses the hell out of this - use just target
        del lib_args_copy['name']

        # Target to build the object filed for the sources.
        # Separate target to build once
        lib_args_copy['target'] = 'objects_%s' % toplib
        lib_args_copy['use'] = [elem for elem in uses if elem != toplib]

        if lang == 'c':
            lib_args_copy['features'] = 'c'
            ctx(**lib_args_copy)
        elif lang == 'c++':
            lib_args_copy['features'] = 'cxx'
            ctx.objects(**lib_args_copy)

        # Prepare for library building - dependent on the objects.
        lib_args_copy['target'] = toplib
        lib_args_copy['use'] = 'objects_%s' % toplib
        lib_args_copy['source'] = ['%s.cc' % toplib]  # need a stub
        # Build the static and dynamic libraries (to check for missing symbols)
        ctx.stlib(**lib_args_copy) # required for IOS
        if ctx.env.ANDROID or os.environ.get('BUILD_SHARED_LIBS'):
            ctx.shlib(**lib_args_copy) # required for Android


def get_verified_android_target(ctx):
    android_target = Arch.GetAndroidTarget(ctx)
    if not android_target or not Arch.IsAndroidSupported(ctx):
        print ('Not an android target, or an unsupported android arch. '
               'Skipping %s' % ctx.path.abspath())
        return None

    if (not os.access(ctx.options.android_sdk_path, os.R_OK) or
        not os.access(os.path.join(ctx.options.android_sdk_path,
                                   'tools', 'android'), os.R_OK)):
        ctx.fatal('Invalid Android SDK path (%s), please use '
                  '--android_sdk_path to specify a valid one' %
                  ctx.options.android_sdk_path)
    return android_target

def android_configure(ctx, path_prefix,
                      lib_deps=None,
                      jar_deps=None,
                      src_deps=None,
                      is_lib=False,
                      proguard_file='proguard.cfg',
                      min_target=None):
    """
    Used to configure android directory for ant build.
    """
    android_target = get_verified_android_target(ctx)
    if not android_target:
        return

    build_path = os.path.join(ctx.path.get_bld().abspath(), path_prefix)
    src_path = os.path.join(ctx.path.abspath(), path_prefix)

    ant_properties = os.path.join(src_path, 'ant.properties')
    local_properties = os.path.join(src_path, 'local.properties')
    project_properties = os.path.join(src_path, 'project.properties')

    if not os.access(build_path, os.R_OK):
        os.makedirs(build_path)

    bin_link = os.path.join(src_path, 'bin')
    libs_dir = os.path.join(src_path, 'libs')
    if not os.access(libs_dir, os.R_OK):
        os.makedirs(libs_dir)
    src_dir = os.path.join(src_path, 'src')
    if not os.access(src_dir, os.R_OK):
        os.makedirs(src_dir)

    safe_symlink(build_path, bin_link)
    if jar_deps:
        for j in jar_deps:
            safe_symlink(os.path.join(ctx.path.abspath(), 'libs', j),
                         os.path.join(libs_dir, j));

    if src_deps:
        for j in src_deps:
            dest_dir = os.path.join(src_dir, 'com', 'g1618labs', 'core')
            if not os.access(dest_dir, os.R_OK):
                os.makedirs(dest_dir)
            safe_symlink(os.path.join('..', 'wout.install', 'java', 'com', 'g1618labs', 'core', j),
                         os.path.join(dest_dir, j));

    print '... Writing : %s' % ant_properties
    open(ant_properties, 'w').write(
        '# Generated by waf\nout.dir=%s\n' % build_path)
    print '... Writing : %s' % local_properties
    open(local_properties, 'w').write(
        '# Generated by waf\nsdk.dir=%s\nndk.dir=%s\n' %
        (ctx.options.android_sdk_path, ctx.options.android_ndk_path))

    print '... Writing : %s' % project_properties
    project_lines = ['# Generated by waf',
                     'target=%s' % (min_target if min_target else android_target),
                     'arch_dir=%s' % Arch.GetAndroidLibsSubDir(ctx),
                     'debug=%d' % (not ctx.options.release)]
    if (proguard_file and
        os.access(os.path.join(src_path, proguard_file), os.R_OK)):
        project_lines.append('proguard.config=%s' % proguard_file)
    if lib_deps:
        ndx = 1
        for lib in lib_deps:
            lib_path = os.path.join(ctx.path.abspath(), lib);
            project_lines.append(
                'android.library.reference.%s=%s' % (
                ndx, os.path.relpath(lib_path, src_path)))
            ndx += 1
    if is_lib:
        project_lines.append('android.library=true')

    open(project_properties, 'w').write('\n'.join(project_lines) + '\n')

def android_setup_avd(ctx, name, sdcard_size='128M',
                      abi='armeabi-v7a', options=''):
    android_target = get_verified_android_target(ctx)
    if not android_target:
        return
    if Arch.GetAndroidArch(ctx) != 'x86':
        print 'Skipping avd creation - doing only for emulator builds.'
        return

    sdcard_path = None
    sdcard_label = None
    avd_root = Arch.GetOutputDir(ctx, 'avd')
    if not os.access(avd_root, os.R_OK):
        os.makedirs(avd_root)

    avd_path = os.path.join(avd_root, name)

    avd_image = os.path.join(avd_path, 'userdata.img')
    system_image = os.path.join(ctx.options.android_sdk_path,
                                'system-images/%s/%s/userdata.img' % (
        android_target, abi))
    if not os.access(system_image, os.R_OK):
        ctx.fatal('Cannot access android image file %s.' % system_image)

    print 'Creating avd under: %s' % avd_path
    if sdcard_size:
        sdcard_label = 'sdCard%s.%s' % (name, sdcard_size)
        sdcard_path = os.path.join(avd_root,
                                   'sdcard_%s.%s' % (name, sdcard_size))
        if not os.access(sdcard_path, os.R_OK):
            print 'Creating sdcard for avd under: %s' % sdcard_path
            sdcard_create_command = '%s/tools/mksdcard -l %s %s %s' % (
                ctx.options.android_sdk_path,
                sdcard_label, sdcard_size, sdcard_path)
            if os.system(sdcard_create_command):
                ctx.fatal('Failure: cannot create sdcard under %s.\n %s' %
                          (sdcard_path, sdcard_create_command))

    avd_list_cmd = '%s/tools/android list avd -c' % ctx.options.android_sdk_path
    (status, out) = commands.getstatusoutput(avd_list_cmd)
    if status:
        ctx.fatal('Cannot list avd-s: %s' % avd_list_cmd)
    if name in out.split('\n'):
        print ('AVD %s already exists - skipping.' % name)
        return
    avd_create_cmd = '%s/tools/android create avd --name %s '\
                     '--target %s --path %s --abi %s %s' % (
        ctx.options.android_sdk_path,
        name, android_target, avd_path, abi, options)
    if sdcard_path:
        avd_create_cmd += ' --sdcard %s' % sdcard_path
    print 'Creating avd %s under: %s' % (name, avd_path)
    if os.system(avd_create_cmd):
        ctx.fatal('Error creating avd w/ command:\n   %s' % avd_create_cmd)
    print 'Android avd created OK'


def android_build(ctx, target, use = None):
    android_target = get_verified_android_target(ctx)
    if not android_target:
        return
    if ctx.options.release:
        mode = 'release'
    else:
        mode = 'debug'
    src_path = ctx.path.abspath()
    target_file = os.path.join(ctx.path.get_bld().abspath(), target)
    cmd = 'rm %s; cd %s && ant %s && cd - && touch %s' % (
        target_file, src_path, mode, target_file)
    if use:
        ctx(rule=cmd, always=True, target=target, depends_on = use)
    else:
        ctx(rule=cmd, always=True, target=target)
    # depends_on = [lib_deps])

def android_build_start_command(ctx, path, project, avd):
    android_target = get_verified_android_target(ctx)
    if not android_target:
        return
    if Arch.GetAndroidArch(ctx) != 'x86':
        print 'Skipping emulator start command - '\
              'doing it only for emulator builds.'
        return
    build_path = os.path.abspath(
        os.path.join(ctx.path.get_bld().abspath(), path))
    script_name = '%s/emul_start_%s' % (build_path, project)
    if ctx.options.release:
        app_name = '%s/%s.apk' % (build_path, project)
    else:
        app_name = '%s/%s-debug.apk' % (build_path, project)

    print 'Creating a script to start project %s under:\n %s' % (
        project, script_name)
    script = """
#!/bin/bash
#
# Auto-generated script to run the emulator, install the
# project package and set location for %(project)s
#
echo 'Starting emulator for avd:  %(avd)s..'
%(sdk)s/tools/emulator -avd %(avd)s &
echo 'Waiting for emulator to start...'
%(sdk)s/platform-tools/adb wait-for-device
echo 'Setting the geo location:'
echo 'geo fix 37426000 -122169000' | nc localhost 5554
echo '(Re)Installing the app:'
%(sdk)s/platform-tools/adb install -r %(app_name)s
""" % { 'sdk' : ctx.options.android_sdk_path,
        'avd' : avd,
        'app_name' : app_name,
        'project' : project }
    open(script_name, 'w').write(script)
    os.system('chmod a+x %s' % script_name)

    print '\n\nDONE. To start the emulator w/ %s project:\n$   %s\n\n' % (
        project, script_name)


def copy_java_gen_files(ctx, gen_node):
    if not isinstance(ctx, Build.InstallContext):
        return
    # here I basically give up in making waf do this correctly
    # as the rule can generate as may files, so just copy the
    # stuff over.
    install_dir_java = ctx.env.INSTALL_JAVADIR
    source_node = gen_node.find_node('../com')
    if source_node:
        def install_generated_java(ctx):
            print '--> Forcing java copying from  %s to %s' % (
                source_node.abspath(), install_dir_java)
            if not os.access(install_dir_java, os.R_OK):
                os.makedirs(install_dir_java)
            os.system('cp -rvf %s %s/' % (source_node.abspath(),
                                          install_dir_java))
                # Normally:
                # install_all_files(ctx, '*.java', install_dir_java,
                #              max_depth=10, cwd_node=source_node)
        ctx.add_pre_fun(install_generated_java)

######################################################################

PROTO_EXT = '.proto'
PROTO_EXT_LEN = len('.proto')
PB_SUFFIX = ['.pb.h', '.pb.cc', '_pb2.py']

def proto_gen_targets(ctx, cmd_base, proto_list, install_dir_h, install_py):
    all_targets = []
    pb_compiler = Arch.GetProtoCompiler(ctx)

    for src in proto_list:
        file_targets = []
        file_targets.extend(['%s%s' % (src[:-PROTO_EXT_LEN], t) for t in PB_SUFFIX])
        all_targets.extend(file_targets)
        cmd = cmd_base + '/' + src
        print ' Adding pb Rule: \n%s\nTARGETS: %s' % (cmd, file_targets)

        # Need to add them all - targets, with a rule that actually generates
        # the file, else the target finding will not work
        for t in file_targets:
            ctx(rule = cmd, source = [src], target = t)

    if install_dir_h:
        ctx.install_files(
            install_dir_h,
            [t for t in all_targets if t[-2:] == '.h'])
    if install_py:
        ctx.install_files(
            os.path.join(ctx.env.INSTALL_PYTHONDIR, 'proto'),
            [t for t in all_targets if t[-3:] == '.py'])

    return ([f for f in all_targets if f[-3:] == '.cc'], all_targets)

################################################################################
#
# Protobuf building
#

def proto_lib(ctx, toplib=None, name=None, target=None,
              source=[], includes=[], install_java=False, install_dir_h=True,
              install_py=True):
    """
    Implement proto_lib() function to build a library from .thrift files.
    Usage:
    ## For top-level wscript file
    def build(ctx):
        ctx.proto_lib = proto_lib

    ## For wscript files in sub directories
    def build(ctx):
        ctx.proto_lib(ctx, name='MyService', source='MyService.proto')
    """
    if not (name or target):
        Logs.error('proto_lib needs either "name" or "target" parameter')
        return
    if not name:  name = target
    if not target:  target = name

    # Compose lists of valid proto files and their prefixes
    proto_list = [src for src in source if src.endswith(PROTO_EXT)]
    if not proto_list:
        Logs.error('proto_lib needs source=["files.proto",...] parameter')
        return

    ctx.post_mode = Build.POST_LAZY
    # Setup some directories
    bld_top = ctx.bldnode.abspath()
    gen_node = ctx.path.get_bld()
    if not os.path.exists(gen_node.abspath()):  os.makedirs(gen_node.abspath())

    cpp_out_dir = bld_top   # WRONG?  Arch.GetOutputDir(ctx)
    java_out_dir = bld_top  # WRONG:  os.path.dirname(gen_node.abspath())
    # print "\nDIRS: src_top:", ctx.top_dir, "bld_top:", bld_top, "\n"

    # Add TaskGen to generate C++ code from .proto files
    cmd = '{0} -I{1} -I{2} --cpp_out={3} --java_out={3} --python_out={3} '.format(
         Arch.GetProtoCompiler(ctx), ctx.top_dir, ctx.path.abspath(), cpp_out_dir)

    # The targets cotain the .cpp generated files + .h generated files
    (cpp_sources, cpp_targets) = proto_gen_targets(
            ctx, cmd + ctx.path.abspath(), proto_list, install_dir_h, install_py)
    if install_java:
        copy_java_gen_files(ctx, gen_node)

    # Build the specified library from proto-generated cpp files
    use = ['whisperlib_includes',  'proto_includes']
    ctx.add_lib(ctx,
        toplib = toplib,
        name = name,
        target = target,
        source = cpp_sources,
        depends_on = cpp_targets,
        use = use)

######################################################################
#
# JNI lib generation
#
def add_jnilib(ctx, **kwargs):
    """
    Generate jni C interfaces from .java files. Need to have a
    jni_classes specified in kwargs w/ full class specification.
    We expect ClassName.java to be present in the calling source
    directory.
    Whatever comes out, is installed (if desired) and this process
    is a dependency of the lib that is built w/ the rest of kwargs.
    """
    src_classes = kwargs.get('src_classes', None)
    if not src_classes:
        src_classes = []

    jni_classes = kwargs.get('jni_classes', None)
    if not jni_classes:
        ctx.fatal('Need to specify some jni classes.')

    gen_classpath = kwargs.get('gen_classpath', None)
    src_classpath = kwargs.get('src_classpath', None)

    jni_dirs = ctx.options.jni_include_dirs.split(',')
    jni_dirs.extend(ctx.env.EXTRA_JNI_DIRS)
    jni_h_readable = False
    for d in jni_dirs:
        if os.access(os.path.join(d, 'jni.h'), os.R_OK):
            jni_h_readable = True
    if not jni_h_readable:
        ctx.fatal('Cannot determine the proper jni.h include path. '
                  'Please provide it w/ --jni_include_dirs (currently: %s)' %
                  ctx.options.jni_include_dirs)

    gen_node = ctx.path.get_bld()
    gen_dir = gen_node.abspath()

    src_dir = ctx.path.abspath()

    gen_cp = []
    src_cp = []

    if gen_classpath:
        for gcp in gen_classpath:
            gen_cp.append(os.path.join(gen_dir, gcp))
    if src_classpath:
        for scp in src_classpath:
            src_cp.append(os.path.join(src_dir, scp))

    src_deps = []

    inter_targets = []
    header_files = []

    for cn in src_classes:
        cn_comp = cn.split('.')
        if len(cn_comp) < 2:
            ctx.fatal('Need to specify full java class path [%s]. '
                      'E.g. com.comp.foo.Bar.' % cn)
        name = cn_comp[-1]
        src_file = os.path.join(src_dir, '%s.java' % name)
        if not os.access(src_file, os.R_OK):
            ctx.fatal('Cannot read java source: %s' % src_file)

        waf_target = '%s.class' % os.path.join(*cn_comp)
        target_class = os.path.join(gen_dir, waf_target)

        src_deps.append(waf_target)

        android_target = Arch.GetAndroidTarget(ctx)

        android_class_path = '%s/platforms/%s/android.jar' %(
            ctx.options.android_sdk_path, android_target)

        final_classpath = ':'.join([android_class_path] + src_cp + gen_cp)

        cmd = 'javac -d %(gen_dir)s -cp "%(acp)s" %(src_file)s' % {
            'src_file' : src_file,
            'acp' : final_classpath,
            'gen_dir' : gen_dir}
        print 'Java class compilation: %s.java [%s] => %s [%s]\n=> %s' % (
            name, cn, target_class, waf_target, cmd)
        ctx(rule = cmd, source = ['%s.java' % name], target=waf_target, use = kwargs.get('use', []))
        inter_targets.append(waf_target)

    for cn in jni_classes:
        cn_comp = cn.split('.')
        if len(cn_comp) < 2:
            ctx.fatal('Need to specify full java class path [%s]. '
                      'E.g. com.comp.foo.Bar.' % cn)
        name = cn_comp[-1]
        src_file = os.path.join(src_dir, '%s.java' % name)
        if not os.access(src_file, os.R_OK):
            ctx.fatal('Cannot read jni source: %s' % src_file)

        target = os.path.join(gen_dir, '%s.h' % name)

        target_class = '%s.class' % os.path.join(gen_dir, name)
        android_target = Arch.GetAndroidTarget(ctx)

        android_class_path = '%s/platforms/%s/android.jar' %(
            ctx.options.android_sdk_path, android_target)

        final_classpath = ':'.join([android_class_path] + src_cp + gen_cp)

        cmd = 'javac -d %(gen_dir)s -cp "%(acp)s" %(src_file)s && '\
            'javah -o %(target)s -classpath "%(gen_dir)s:%(acp)s" -jni %(cn)s' % {
            'src_file' : src_file, 'target' : target,
            'acp' : final_classpath,
            'gen_dir' : gen_dir, 'cn': cn }
        waf_target = os.path.basename(target)
        after = src_deps
        print 'JNI class generation: %s.java [%s] => %s [%s] after [%s]\n=> %s' % (
            name, cn, target, waf_target, after, cmd)
        ctx(rule = cmd, source = ['%s.java' % name], target=waf_target, after=after)
        inter_targets.append(waf_target)
        header_files.append(target)


    # Setup install - we need to do them as post-rules as the source
    # files are not available during the actual build processing.
    # Important: rules are added in the root wscript dir, and install
    # uses relative path,that's why we have to get the os.path.relpath
    # of the generate targets.
    if 'install_dir_h' in kwargs:
        install_dir_h = kwargs['install_dir_h']
        def install_h_fun(ctx):
            ctx.install_files(os.path.join(ctx.env.INSTALL_INCLUDEDIR,
                                           install_dir_h),
                              inter_targets,
                              cwd=gen_node)
        if os.access(header_files[0], os.R_OK):
            install_h_fun(ctx)
        else:
            ctx.add_post_fun(install_h_fun)

    install_dir_java = ctx.env.INSTALL_JAVADIR

    for cn in jni_classes:
        java_subdir = os.sep.join(cn.split('.')[:-1])
        ctx.install_files(
            os.path.join(install_dir_java, java_subdir),
            '%s.java' % cn.split('.')[-1])

    del kwargs['jni_classes']

    for cn in src_classes:
        java_subdir = os.sep.join(cn.split('.')[:-1])
        ctx.install_files(
            os.path.join(install_dir_java, java_subdir),
            '%s.java' % cn.split('.')[-1])

    if kwargs.get('src_classes', None):
        del kwargs['src_classes']

    # Prepare params for library building
    extend_list_key(kwargs, 'use',
                    # Now, these are relative at the time of build (which means
                    # to the current path (not source root)
                    [os.path.relpath(t, ctx.path.abspath())
                     for t in header_files])
    extend_list_key(kwargs, 'depends_on', inter_targets)
    extend_list_key(kwargs, 'includes', jni_dirs)

    # Add the actual library for building
    add_lib(ctx, **kwargs)

######################################################################

# Utility for processing depends_on rule - to induce some of our dependencies.
from waflib.TaskGen import feature, before_method
@feature('*')
@before_method('process_rule')
def post_the_other(self):
#    env = getattr(self, 'env', None)
#    extra_map_file = getattr(self, 'extra_map_file', None)
#
#    if env:
#        ndx = -1
#        i = 0
#        for option in env['LINKFLAGS']:
#            if option.startswith('-Wl,-Map,'):
#                ndx = i
#            i += 1
#        if ndx >= 0:
#            del env['LINKFLAGS'][ndx]
#    if extra_map_file and env:
#         env['LINKFLAGS'].append('-Wl,-Map,' + extra_map_file)

    deps = getattr(self, 'depends_on', [])
    if type(deps) == type(''):
        deps = [deps]
    for name in deps:
        dep = self.bld.get_tgen_by_name(name)
        dep.post()


######################################################################
#
# General Utilities
#
def split_define(d):
    comp = [s.strip() for s in d.split('=')]
    if len(comp) == 0:
        sys.exit('>> Bad define to export: [%s]' %d)
    if len(comp) == 1:
        return comp[0]
    return '%s %s' % (comp[0], comp[1])

def split_define_name(d):
    comp = [s.strip() for s in d.split('=')]
    if len(comp) == 0:
        sys.exit('>> Bad define to export: [%s]' %d)
    return comp[0]

def output_config_file(ctx, filename):
    content = '\n'.join(['#ifndef %s\n#define %s\n#endif' % (split_define_name(d), split_define(d))
                         for d in ctx.env.DEFINES])
    gen_node = ctx.path.get_bld()
    if not os.path.exists(gen_node.abspath()):
        os.makedirs(gen_node.abspath())

    bld_top = ctx.bldnode.abspath()
    intermediate = os.path.join(bld_top, '%s_' % filename)
    open(intermediate, 'w').write('// Automatically generated\n%s\n' % content)
    cmd = 'mv -f %s %s/%s' % (intermediate, bld_top, filename)
    ctx(rule=cmd, target=filename, source = ['WafUtil.py'])
    ctx.install_files(ctx.env.INSTALL_INCLUDEDIR, [filename])

def get_icu_config(ctx):
    if ctx.options.icu_config:
        icu_binary = ctx.options.icu_config
    else:
        icu_binary = 'icu-config'

    (status, output_cpp) = commands.getstatusoutput('%s --cppflags' % icu_binary)
    if status:
        print 'ICU disabled (cpp): %s' % output_cpp
        return (None, None)
    (status, output_ld) = commands.getstatusoutput('%s --ldflags' % icu_binary)
    if status:
        print 'ICU disabled (ld): %s' % output_ld
        return (None, None)
    return (output_cpp.strip(), output_ld.strip())

def get_pkg_config(ctx, package):
    if ctx.options.pkg_config:
        pkg_config_binary = ctx.options.pkg_config
    else:
        pkg_config_binary = 'pkg-config'

    (status, output_cpp) = commands.getstatusoutput('pkg-config --cflags %s' % package)
    if status:
        print 'Package %s disabled (cpp): %s' % (package, output_cpp)
        return (None, None)
    (status, output_ld) = commands.getstatusoutput('pkg-config --libs %s' % package)
    if status:
        print 'Package %s disabled (cpp): %s' % (package, output_ld)
        return (None, None)
    return (output_cpp.strip(), output_ld.strip())

def setup_context_includes(ctx, core_dir):
    core_includes = [core_dir,
                     os.path.abspath(os.path.join(core_dir, 'ext', 'snappy')),
                     os.path.abspath(os.path.join(core_dir, 'ext'))]
    if ctx.env.ANDROID or ctx.env.IOS:
        core_includes.append(
            os.path.abspath(os.path.join(core_dir, 'ext', 'openssl-1.0.1e', 'include')))
        core_includes.append(
            os.path.abspath(os.path.join(core_dir, 'ext', 'openssl-1.0.1e')))
    if ctx.env.TIZEN:
        core_includes.append(
            os.path.abspath(os.path.join(core_dir, 'ext', 'zlib')))

    if ctx.env.USE_FREETYPE:
        core_includes.append(
            os.path.abspath(os.path.join(core_dir, 'ext', 'freetype-2.5.0', 'include')))
    if ctx.env.USE_HARFBUZZ:
        core_includes.append(
            os.path.abspath(os.path.join(ctx.env.INSTALL_INCLUDEDIR, 'ext', 'harfbuzz-0.9.19', 'src')))

    ctx(name = 'openssl_includes',
        export_includes = ['.', core_dir,
                           os.path.abspath(os.path.join(core_dir, 'ext', 'openssl-1.0.1e', 'include')),
                           os.path.abspath(os.path.join(core_dir, 'ext', 'openssl-1.0.1e')),
                           os.path.abspath(os.path.join(core_dir, 'ext', 'openssl-1.0.1e', 'crypto')),
                           os.path.abspath(os.path.join(core_dir, 'ext', 'openssl-1.0.1e', 'ssl'))])

    ctx(name = 'whisperlib_includes',
        export_includes = ['.',
                           core_dir, os.path.abspath(core_dir)])
    ctx(name = 'proto_includes',
        export_includes = ['.', core_dir, Arch.GetOutputDir(ctx, 'whisperlib')])
    ctx(name = 'proto_compiler_includes',
        export_includes = [core_dir])

def setup_install_dirs(ctx):
    if ctx.options.install_root:
        ctx.env.INSTALL_DIR = ctx.options.install_root
    else:
        ctx.env.INSTALL_DIR = os.path.abspath(
            os.path.join('..', 'wout.install'))
    ctx.env.INSTALL_BINDIR = os.path.join(ctx.env.INSTALL_DIR, 'bin')
    ctx.env.INSTALL_INCLUDEDIR = os.path.join(ctx.env.INSTALL_DIR, 'include')
    ctx.env.INSTALL_JAVADIR = os.path.join(ctx.env.INSTALL_DIR, 'java')
    ctx.env.INSTALL_PYTHONDIR = os.path.join(ctx.env.INSTALL_DIR, 'python')
    ctx.env.INSTALL_TOOLSDIR = os.path.join(ctx.env.INSTALL_DIR, 'tools')
    libs_dir = os.path.join(ctx.env.INSTALL_DIR, 'lib')
    libs_subdir = Arch.GetInstallLibsSubDir(ctx)
    ctx.env.INSTALL_LIBDIR = os.path.join(libs_dir, libs_subdir)

    print 'Setting up install dir to %s' % ctx.env.INSTALL_DIR
    print 'Setting up libs output to to %s' % ctx.env.INSTALL_LIBDIR

def install_all_includes(ctx, max_depth=2, pattern='*.h', cwd_node=None,
                         install_prefix=''):
    prefix = ctx.path.abspath()[len(ctx.srcnode.abspath()) + 1:]
    if not install_prefix:
        install_prefix = prefix
    install_all_files(ctx, pattern, '${INSTALL_INCLUDEDIR}',
                      prefix=prefix, install_prefix=install_prefix,
                      max_depth=max_depth, cwd_node=cwd_node)

def install_all_files(ctx, pattern, install_dir,
                      prefix='', install_prefix='',
                      max_depth=2,  cwd_node=None):
    if cwd_node:
        saved_cwd = os.getcwd()
        os.chdir(cwd_node.abspath())
    to_install = {}
    for depth in xrange(max_depth):
        if prefix:
            files = [f[len(prefix) + 1:] for f
                     in glob.iglob(os.path.join(prefix, pattern))]
        else:
            files = glob.iglob(pattern)
        for f in files:
            if install_prefix:
                subdir = os.path.join(
                    install_dir, install_prefix, os.path.dirname(f))
            else:
                subdir = os.path.join(install_dir, os.path.dirname(f))
            if os.path.isfile(os.path.join(prefix, f)):
                if subdir in to_install:
                    to_install[subdir].append(f)
                else:
                    to_install[subdir] = [f]

        pattern = os.path.join('*', pattern)

    for (d, files) in to_install.iteritems():
        if cwd_node:
            ctx.install_files(d, [cwd_node.find_node(os.path.join(prefix, f))
                                  for f in files])
        else:
            ctx.install_files(d, files)
    if cwd_node:
        os.chdir(saved_cwd)


def ignore(*args, **kwargs):
    return

def cc_binary(ctx, **kwargs):
    ctx.program(**kwargs)

def make_iterable(value):
    """Utility to make the value an always safe to iterate object."""
    if type(value) == type(''):
        return [value]
    try:
        itvalue = iter(value)
        return itvalue
    except TypeError:
        return [value]

def get_current_repository_dir():
    cmd = 'git rev-parse --show-toplevel'
    (status, output) = commands.getstatusoutput(cmd)
    if status:
        return os.getcwd()
        # sys.exit('>>>> FATAL: Cannot get git repository [%s]' % cmd)
    return output.strip()

def verify_current_dir():
    cwd = os.getcwd()
    repo = get_current_repository_dir()
    if cwd != repo:
        sys.exit('>>>> FATAL: Please run waf in your top repository dir: %s' %
                 repo)

def extend_list_key(kwargs, k, v):
    if k in kwargs:
        kwargs[k].extend(v)
    else:
        kwargs[k] = v

def safe_symlink(src, dest):
    print '... Sym linking %s into %s' % (src, dest)
    if not os.access(src, os.R_OK):
        sys.exit('Cannot access symlink source: %s' % src)
    if os.access(dest, os.R_OK):
        if not os.path.islink(dest):
            sys.exit('Node: %s exists, and is not a link. If you want to '
                      'go on with waf, please delete it yourself and re-run '
                      'waf configure' % dest)
    if os.path.islink(dest):
        os.unlink(dest)
    os.symlink(os.path.abspath(src), dest)

######################################################################
#
# Config variables extraction (the HAVE_.. defines)
#

def configure_cxx_defines(conf):
    check_HAVE_Include(conf, 'poll.h')
    check_HAVE_Include(conf, 'sys/poll.h')
    if not conf.env.ANDROID:
        # I cannot emphasize enough how screwed is epoll on
        # android (just ask Deepak :)- so, even if if epoll
        # is available on android, we choose not to use it
        # as on client we are good enough w/ poll
        check_HAVE_Include(conf, 'sys/epoll.h')
    check_HAVE_Include(conf, 'sys/eventfd.h')

    check_HAVE_GETTIMEOFDAY(conf)
    check_HAVE_CLOCK_GETTIME(conf)
    check_HAVE_MACH_TIMEBASE(conf)
    check_HAVE_SCHED_GET_PRIORITY_MIN(conf)
    check_HAVE_SCHED_GET_PRIORITY_MAX(conf)
    check_HAVE_READ_WRITE_MUTEX(conf)
    check_HAVE_SYS_STAT64(conf)
    check_HAVE_FDATASYNC(conf)
    check_HAVE_LSEEK64(conf)
    check_HAVE_DOUBLE_T(conf)
    check_HAVE_FLOAT_T(conf)
    check_HAVE_WCSTOLL(conf)
    check_HAVE_WCTOMB(conf)
    check_HAVE_WCSDUP(conf)

    check_HAVE_GLOG(conf)
    check_HAVE_GFLAGS(conf)
    check_HAVE_GOOGLE_PERFTOOLS(conf)
    #check_HAVE_Include(conf, 'glog/logging.h')
    #check_HAVE_Include(conf, 'gflags/gflags.h')

    check_HAVE_READDIR_R(conf)
    check_HAVE_INET_PTON(conf)
    check_HAVE_GETADDRINFO(conf)


    check_HAVE_Include(conf, 'sys/types.h')
    check_HAVE_Include(conf, 'pthread.h')
    check_HAVE_Include(conf, 'inttypes.h')
    check_HAVE_Include(conf, 'endian.h')
    check_HAVE_Include(conf, 'machine/endian.h')
    check_HAVE_Include(conf, 'machine/_types.h')
    check_HAVE_Include(conf, 'bits/limits.h')
    check_HAVE_Include(conf, 'stdint.h')
    check_HAVE_Include(conf, 'string.h')
    check_HAVE_Include(conf, 'strings.h')
    check_HAVE_Include(conf, 'strings.h', function='memcpy')
    check_HAVE_Include(conf, 'strings.h', function='memset')
    check_HAVE_Include(conf, 'math.h', function='pow')
    check_HAVE_Include(conf, 'stdlib.h', function='posix_memalign')
    check_HAVE_Include(conf, 'stdlib.h', function='memalign')
    check_HAVE_Include(conf, 'execinfo.h')

    check_HAVE_Include(conf, 'wchar.h')
    check_HAVE_Include(conf, 'wctype.h')
    check_HAVE_Include(conf, 'ctype.h')
    check_HAVE_Include(conf, 'wstring')

    check_HAVE_Include(conf, 'memory.h')
    check_HAVE_Include(conf, 'regex.h')
    check_HAVE_Include(conf, 'dlfcn.h')
    check_HAVE_Include(conf, 'unistd.h')
    check_HAVE_Include(conf, 'dirent.h')
    check_HAVE_Include(conf, 'errno.h')
    check_HAVE_Include(conf, 'sys/stat.h')
    check_HAVE_Include(conf, 'sys/dir.h')
    check_HAVE_Include(conf, 'openssl/ssl.h')

    check_HAVE_Include(conf, 'linux/types.h')
    check_HAVE_Include(conf, 'linux/errqueue.h')

    check_HAVE_Include(conf, 'netdb.h')
    check_HAVE_Include(conf, 'netinet/in.h')
    check_HAVE_Include(conf, 'sys/param.h')
    check_HAVE_Include(conf, 'sys/uio.h')
    check_HAVE_Include(conf, 'sys/socket.h')
    check_HAVE_Include(conf, 'sys/time.h')
    check_HAVE_Include(conf, 'sys/timeb.h')
    check_HAVE_Include(conf, 'sys/mman.h')
    check_HAVE_Include(conf, 'sys/types.h')
    check_HAVE_Include(conf, 'glib-2.0/glib/gmacros.h')

    check_HAVE_Include(conf, 'ext/hash_set')
    check_HAVE_Include(conf, 'ext/hash_map')
    check_HAVE_Include(conf, 'ext/hash_fun.h')
    check_HAVE_Include(conf, 'functional_hash.h')
    check_HAVE_Include(conf, 'functional')
    check_HAVE_Include(conf, 'tr1/functional_hash.h')
    check_HAVE_Include(conf, 'unordered_set')
    check_HAVE_Include(conf, 'tr1/unordered_set')
    check_HAVE_Include(conf, 'unordered_map')
    check_HAVE_Include(conf, 'tr1/unordered_map')
    check_HAVE_Include(conf, 'tr1/memory')
    check_HAVE_Include(conf, 'memory')
    check_HAVE_Include(conf, 'libxml/xmlreader.h')

    check_HAVE_Include(conf, 'nameser8_compat.h')

    check_HAVE_Include(conf, 'unicode/utf8.h')
    check_HAVE_Include(conf, 'unicode/utf.h')

    check_HAVE_Include(conf, 'GL/glfw.h')
    check_HAVE_Include(conf, 'GLFW/glfw3.h')

    if conf.env.HAVE_GLFW_GLFW3_H:
        conf.env.GLFW_LIB = 'glfw3'
    elif conf.env.HAVE_GL_GLFW_H:
        conf.env.GLFW_LIB = 'glfw'
    else:
        conf.env.GLFW_LIB = None

    conf.env.DEFINES.append('PATH_SEPARATOR=\'%s\'' % os.sep)


def check_HAVE_FLOAT_T(conf):
    CheckWithFragment(conf, 'HAVE_FLOAT_T', '''
    #include <math.h>
    int main() {
      float_t x = 0;
      return 0;
    }
    ''')
def check_HAVE_DOUBLE_T(conf):
    CheckWithFragment(conf, 'HAVE_DOUBLE_T', '''
    #include <math.h>
    int main() {
      double_t x = 0;
      return 0;
    }
    ''')

def check_HAVE_WCSDUP(conf):
    CheckWithFragment(conf, 'HAVE_WCSDUP', '''
    #include <wchar.h>
    int main() {
      wchar_t src[] = L"";
      wchar_t* dest = wcsdup(src);
      free(dest);
      return 0;
    }
    ''')

def check_HAVE_WCTOMB(conf):
    CheckWithFragment(conf, 'HAVE_WCTOMB', '''
    #include <wchar.h>
    #include <stdlib.h>
    int main() {
      wchar_t src = L'X';
      char dest[10];
      wctomb(dest, src);
      return 0;
    }
    ''')

def check_HAVE_WCSTOLL(conf):
    CheckWithFragment(conf, 'HAVE_WCSTOLL', '''
    #include <wchar.h>
    int main() {
      wchar_t src[] = L"0";
      long long ret = wcstoll(src, NULL, 10);
      return 0;
    }
    ''')

def check_HAVE_GETTIMEOFDAY(conf):
    CheckWithFragment(conf, 'HAVE_GETTIMEOFDAY', '''
    #include <sys/time.h>
    int main() {
      struct timeval now;
      return gettimeofday(&now, 0);
    }
    ''')

def check_HAVE_CLOCK_GETTIME(conf):
    CheckWithFragment(conf, 'HAVE_CLOCK_GETTIME', '''
    #include <time.h>
    int main() {
      struct timespec now;
      return clock_gettime(CLOCK_REALTIME, &now);
    }
    ''')

def check_HAVE_MACH_TIMEBASE(conf):
    CheckWithFragment(conf, 'HAVE_MACH_TIMEBASE', '''
    #include <time.h>
    #include <mach/mach_time.h>
    int main() {
      static mach_timebase_info_data_t timer_info = {0,0};
      return mach_timebase_info(&timer_info);
    }
    ''')

def check_HAVE_READ_WRITE_MUTEX(conf):
    CheckWithFragment(conf, 'HAVE_READ_WRITE_MUTEX', '''
    #include <pthread.h>
    int main() {
      pthread_rwlock_t mu;
      return pthread_rwlock_init(&mu, 0);
    }
    ''')

def check_HAVE_SCHED_GET_PRIORITY_MIN(conf):
    CheckWithFragment(conf, 'HAVE_SCHED_GET_PRIORITY_MIN', '''
    #include <sched.h>
    int main() {
      return -1 == sched_get_priority_min(SCHED_FIFO);
    }
    ''')

def check_HAVE_SCHED_GET_PRIORITY_MAX(conf):
    CheckWithFragment(conf, 'HAVE_SCHED_GET_PRIORITY_MAX', '''
    #include <sched.h>
    int main() {
      return -1 == sched_get_priority_max(SCHED_FIFO);
    }
    ''')

def check_HAVE_SYS_STAT64(conf):
    CheckWithFragment(conf, 'HAVE_SYS_STAT64', '''
    #include <sys/stat.h>
    int main() {
       struct stat64 st;
       stat64(".", &st);
       return 0;
    }
    ''')

def check_HAVE_FDATASYNC(conf):
    CheckWithFragment(conf, 'HAVE_FDATASYNC', '''
    #include <unistd.h>
    int main() {
        fdatasync(1);
        return 0;
    }
    ''')

def check_HAVE_LSEEK64(conf):
    CheckWithFragment(conf, 'HAVE_LSEEK64', '''
    #include <sys/types.h>
    #include <unistd.h>
    int main() {
       lseek64(0, 0, SEEK_CUR);
       return 0;
    }
    ''')

def check_HAVE_GLOG(conf):
    CheckWithFragment(conf, 'HAVE_GLOG', '''
     #include <glog/logging.h>
     int main() { return 0; }
     ''')

def check_HAVE_GOOGLE_PERFTOOLS(conf):
    CheckWithFragment(conf, 'HAVE_GOOGLE_PERFTOOLS', '''
     #include <google/heap-profiler.h>
     int main() { return 0; }
     ''')

def check_HAVE_READDIR_R(conf):
    CheckWithFragment(conf, 'HAVE_READDIR_R', '''
    #include <dirent.h>
    #include <stdlib.h>
    int main() {
       struct dirent* buf = static_cast<struct dirent *>(malloc(2048));
       struct dirent* entry = NULL;
       DIR* dirp = NULL;
       if (false) {
       ::readdir_r(dirp, buf, &entry);  // just check compilation
       }
       return 0;
    }
    ''')

def check_HAVE_INET_PTON(conf):
    CheckWithFragment(conf, 'HAVE_INET_PTON', '''
    #include <arpa/inet.h>
    int main() {
       int ipv4;
       int error = inet_pton(AF_INET, "127.0.0.1", &ipv4);
       return 0;
    }
    ''')

def check_HAVE_GETADDRINFO(conf):
    CheckWithFragment(conf, 'HAVE_GETADDRINFO', '''
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <stdlib.h>
    int main() {
       char data[16];
       struct addrinfo* result = (struct addrinfo*) data;
       int error = ::getaddrinfo("www.google.com", NULL, NULL, &result);
       return 0;
       }
    ''')


def check_HAVE_GFLAGS(conf):
    CheckWithFragment(conf, 'HAVE_GFLAGS', '''
     #include <gflags/gflags.h>
     int main() { return 0; }
     ''')


def check_HAVE_Include(conf, include, function=None, mandatory=False):
    execute = Arch.ArchCanExecute(conf)
    conf.check(header_name=include,
               function=function,
               execute=execute,
               define_ret=True,
               mandatory=mandatory,
               compile_mode='cxx')

def CheckWithFragment(conf, var, fragment, mandatory=False):
    execute = Arch.ArchCanExecute(conf)
    try:
        conf.check_cxx(fragment    = fragment,
                       define_name = var,
                       execute     = execute,
                       define_ret  = True,
                       mandatory   = mandatory,
                       msg         = 'Checking for %s' % var)
    except Exception, e:
        if mandatory:
            sys.exit('Required variable cannot be checked: %s\n Error: %s' % (
                var, e))
