// We need this library stub for proper waf linking

// Also: this is the place to take care of the
//  -compatibility_version and -current_version flags.

#ifdef __cplusplus
extern "C" {
#endif

const char id[] = "libiwhisperlib-2.0.1";
const char* GetLibcoreId() {
    return id;
}

#ifdef __cplusplus
}
#endif
