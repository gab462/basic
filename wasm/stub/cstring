auto memcpy(char* dst, const char* src, size_t n) -> void {
    for (size_t i = 0; i < n; ++i) {
        dst[i] = src[i];
    }
}

auto memcmp(const char* a, const char* b, size_t n) -> int {
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) {
            return -1;
        }
    }

    return 0;
}

extern "C" auto memset(char* m, int c, size_t n) -> void {
    for (size_t i = 0; i < n; ++i) {
        m[i] = c;
    }
}

auto strlen(const char* s) -> size_t {
    size_t n = 0;

    while (*s++) ++n;

    return n;
}
