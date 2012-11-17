
#include <whisperlib/base/lru_cache.h>
#include <whisperlib/base/system.h>
void TestCache() {
    SimpleLruCachePolicy<int, int> policy;
    synch::MutexPool pool(7);
    LruCache<int, int> cache(2, policy, &pool);
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
        CHECK(cache.Put(i, new int(2000 + i), &val));
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
    common::Init(argc, argv);
    // TestCache();
    return 0;
}
