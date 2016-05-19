/*
 * Copyright (c) 2013, Urban Engines inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Urban Engines inc nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Catalin Popescu
 */

#include "whisperlib/base/lru_cache.h"
#include "whisperlib/base/system.h"
void TestCache() {
    SimpleLruCachePolicy<int, int> policy;
    LruCache<int, int> cache(2, policy);
    ref_counted<int>* val;

    CHECK(!cache.Get(0, &val));
    val->DecRef();
    CHECK(!cache.Put(0, new int(1000)));
    CHECK(!cache.Put(1, new int(1001)));
    CHECK(cache.Get(0, &val));
    CHECK_EQ(*val->get(), 1000);
    val->DecRef();
    CHECK(cache.Get(1, &val));
    CHECK_EQ(*val->get(), 1001);
    val->DecRef();

    CHECK(!cache.Put(2, new int(1002)));
    CHECK(cache.Get(2, &val));
    CHECK_EQ(*val->get(), 1002);
    val->DecRef();
    CHECK(!cache.Get(0, &val));

    for (int i = 3; i < 1000; ++i) {
        CHECK(cache.HasKey(i - 2));
        CHECK(cache.HasKey(i - 1));
        CHECK(!cache.Put(i, new int(1000 + i)));
        CHECK(!cache.HasKey(i - 2));
        CHECK(!cache.Get(i - 2, &val));
    }
    cache.EvictAll();
    CHECK_EQ(cache.size(), 0);

    CHECK(!cache.Put(0, new int(2000)));
    CHECK(!cache.Put(1, new int(2001)));
    for (int i = 2; i < 1000; i += 2) {
        CHECK(cache.Get(i - 2, &val));
        CHECK_EQ(*val->get(), 2000 + i - 2);
        val->DecRef();

        CHECK(!cache.Put(i, new int(1000 + i)));
        CHECK(!cache.HasKey(i - 1));
        CHECK(!cache.Put(i + 1, new int(1000 + i + 1)));
        CHECK(!cache.HasKey(i - 2));
        CHECK(cache.Put(i, new int(2000 + i), &val, NULL));
        CHECK_EQ(*val->get(), 1000 + i);
        val->DecRef();
        CHECK(cache.Get(i, &val));
        CHECK_EQ(*val->get(), 2000 + i);
        val->DecRef();
        CHECK(cache.Remove(i + 1, &val));
        CHECK_EQ(*val->get(), 1000 + i + 1);
        val->DecRef();
        CHECK(!cache.Put(i + 1, new int(2000 + i + 1)));
    }
    CHECK_EQ(cache.size(), 2);
    cache.EvictAll();
    CHECK_EQ(cache.size(), 0);
    LOG_INFO << "PASS";
}

int main(int argc, char* argv[]) {
    whisper::common::Init(argc, argv);
    // TestCache();
    return 0;
}
