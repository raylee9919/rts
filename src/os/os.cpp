// Copyright Seong Woo Lee. All Rights Reserved.

#include "os/os.h"
#include "basic/string.h"
#include "third_party/xxhash3/xxhash.h"

Guid guid_from_bytes(void *bytes, u64 size) {
    XXH128_hash_t hash = XXH3_128bits_withSeed(bytes, size, 0);
    Guid result;
    result._64[0] = hash.low64;
    result._64[1] = hash.high64;
    return result;
}

Guid guid_from_string(String str) {
    return guid_from_bytes(str.str, str.len * sizeof(str.str[0]));
}
