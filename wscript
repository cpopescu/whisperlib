# -*- Mode:python; indent-tabs-mode:nil; coding:utf-8 -*-
# python-indent-offset:4

import os
import WafUtil, waflib

# Configuration variables - go directly up on top
top = '.'
out = None

subdirs = [
           'whisperlib',
           'google',
           ]

def options(opt):
    WafUtil.setup_options(opt)

def init(ctx):
    WafUtil.setup_init(ctx)
    # Only now we know when to output - set out accordingly.
    global out
    out = WafUtil.Arch.GetOutputDir(ctx)

def configure(ctx):
    WafUtil.setup_configure(ctx)

def all(ctx):
    waflib.Options.commands = ['clean', 'build']
    ctx.options.gen_cpp = True

def build(ctx):
    WafUtil.setup_build(ctx)
    WafUtil.setup_context_includes(ctx, '.')

    # Recursively add build targets in sub-directories
    ctx.recurse(subdirs)
    ctx.set_toplib_options(ctx, 'whisperlib', install_path = '${INSTALL_LIBDIR}')
    ctx.build_top_lib(ctx, 'whisperlib')

    ctx.install_files(ctx.env.INSTALL_PYTHONDIR, 'WafUtil.py')
    ctx.output_config_file(ctx, 'whisperlib/base/core_config.h')

