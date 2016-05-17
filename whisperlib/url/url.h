// -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
// (c) Copyright 2011, Urban Engines
// All rights reserved.

#include "whisperlib/base/types.h"

#if defined(USE_ICU) && defined(HAVE_ICU)
  #include "whisperlib/url/google-url/gurl.h"
#else
  #include "whisperlib/url/simple_url.h"
#endif

