struct FILE;

auto fopen(const char*, const char*) -> FILE* {
    return nullptr;
}

auto fseek(FILE*, int, int) -> int {
    return -1;
}

auto ftell(FILE*) -> int {
    return 0;
}

auto ferror(FILE*) -> int {
    return -1;
}

auto fclose(FILE*) -> void {}

auto fread(char*, int, int, FILE*) -> void {}

#define SEEK_SET -1
#define SEEK_END -1
#define STDOUT_FILENO -1

template <typename ...A>
auto snprintf(char* dst, size_t n, const char* fmt, A...) -> size_t {
    if (dst != NULL) {
        memcpy(dst, fmt, n);
    }

    return strlen(fmt);
}
