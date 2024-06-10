#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

auto operator new(size_t n, void* ptr) -> void* {
    (void) n;
    return ptr;
}

auto operator new[](size_t n, void* ptr) -> void* {
    (void) n;
    return ptr;
}

struct Arena {
    u32 capacity;
    u32 position;
    void* memory;

    Arena(u32 n): capacity{n}, position{0}, memory{malloc(n)} {
        assert(memory != NULL);
    }

    ~Arena() {
        free(memory);
    }

    template <typename T, typename ...A>
    auto make(A... args) -> T* {
        if (position % alignof(T) != 0)
            position += (alignof(T) - (position % alignof(T)));

        assert(position + sizeof(T) <= capacity);

        T* pointer = new(static_cast<char*>(memory) + position) T{args...};

        position += sizeof(T);

        return pointer;
    }

    template <typename T, typename ...A>
    auto allocate(u32 n = 1, A... args) -> T* {
        if (position % alignof(T) != 0)
            position += (alignof(T) - (position % alignof(T)));

        assert(position + sizeof(T) * n <= capacity);

        T* pointer = new(static_cast<char*>(memory) + position) T[n]{args...};

        position += sizeof(T) * n;

        return pointer;
    }
};

template <typename T>
struct Vector {
    u32 length;
    u32 position;
    T* data;

    Vector(Arena& arena, u32 n): length{n}, position{0} {
        data = arena.allocate<T>(length);
    }

    template <typename ...A>
    Vector(Arena& arena, T a, T b, A... args): length{sizeof...(args) + 2}, position{0} {
        data = arena.allocate<T>(length, a, b, args...);
    }

    auto push(T value) -> void {
        assert(position < length);

        data[position++] = value;
    }

    template <typename ...A>
    auto append(A... args) -> void {
        (push(args), ...);
    }

    auto operator[] (u32 n) -> T& {
        return data[n];
    }

    auto begin () -> T* {
        return &data[0];
    }

    auto end () -> T* {
        return &data[length]; // position?
    }
};

struct String {
    const char* data;
    u32 length;

    String(const char* s): data(s), length(strlen(s)) {}
    String(const char* s, u32 n): data(s), length(n) {}
    String(): data(nullptr), length(0) {}

    auto operator==(String other) -> bool {
        if (length != other.length)
            return false;

        return memcmp(data, other.data, length) == 0;
    }

    auto append(Arena& arena, String other) -> String {
        String string;

        string.length = length + other.length;

        string.data = arena.allocate<const char>(string.length);

        memcpy(const_cast<char*>(string.data), data, length);
        memcpy(const_cast<char*>(string.data + length), other.data, other.length);

        return string;
    }

    auto count_characters(char c) -> u32 {
        u32 count = 0;

        for (u32 i = 0; i < length; ++i) {
            if (data[i] == c)
                ++count;
        }

        return count;
    }

    auto split(Arena& arena, char separator) -> Vector<String> {
        u32 separator_count = count_characters(separator);
        Vector<String> strings{arena, separator_count + 1};

        u32 position = 0;

        for (u32 i = 0; i < length; ++i) {
            if (data[i] == separator) {
                strings.append(String{ data + position, i - position });
                position = i + 1;
            }

            if (i + 1 == length)
                strings.append(String{ data + position, length - position });
        }

        return strings;
    }

    auto join(Arena& arena, Vector<String> strings) -> String {
        String string;

        for (auto& it: strings) {
            string.length += it.length + length;
        }

        string.length -= length;

        string.data = arena.allocate<const char>(string.length);

        u32 position = 0;

        for (auto& it: strings) {
            memcpy(const_cast<char*>(string.data) + position, it.data, it.length);
            position += it.length;

            if (position < string.length) {
                // Also copy separator
                memcpy(const_cast<char*>(string.data) + position, data, length);
                position += length;
            }
        }

        return string;
    }

    auto substring(String other) -> bool {
        if (other.length > length)
            return false;

        u32 match = 0;

        for (u32 i = 0; i < length; ++i) {
            if (data[i] == other.data[match])
                ++match;
            else
                match = data[i] == other.data[0] ? 1 : 0;

            if (match == other.length)
                return true;
        }

        return false;
    }

    auto right(u32 n) -> String {
        return { data + length - n, n };
    }

    auto left(u32 n) -> String {
        return { data, n };
    }

    auto chop_right(u32 n) -> String {
        return { data, length - n };
    }

    auto chop_left(u32 n) -> String {
        return { data + n, length - n };
    }
    
    auto cstr(Arena& arena) -> const char* {
        char* s = arena.allocate<char>(length + 1);

        memcpy(s, data, length);

        s[length] = '\0';

        return s;
    }
};

auto read_file(Arena& arena, String path) -> String {
    Arena scratch{path.length + 1};

    const char* path_cstr = path.cstr(scratch);

    FILE* file = fopen(path_cstr, "rb");

    assert(file != NULL);

    assert(fseek(file, 0, SEEK_END) >= 0);

    String string;

    string.length = ftell(file);

    assert(fseek(file, 0, SEEK_SET) >= 0);

    string.data = arena.allocate<char>(string.length);

    fread(const_cast<char*>(string.data), 1, string.length, file);

    assert(ferror(file) == 0);

    fclose(file);

    return string;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"

template <typename ...A>
auto println(String string, A... args) -> void {
    if (!string.substring("%")) { // FIXME: plain % impossible
        assert(write(STDOUT_FILENO, string.data, string.length) >= 0);
        assert(write(STDOUT_FILENO, "\n", 1) >= 0);
    } else {
        Arena cstr_arena{string.length + 1};
        const char* format_cstr = string.cstr(cstr_arena);

        u32 length = snprintf(NULL, 0, format_cstr, args...);

        Arena format_arena{length + 1};

        String format{ format_arena.allocate<const char>(length + 1), length };

        snprintf(const_cast<char*>(format.data), format.length + 1, format_cstr, args...);

        assert(write(STDOUT_FILENO, format.data, format.length) >= 0);
        assert(write(STDOUT_FILENO, "\n", 1) >= 0);
    }
}

template <typename ...A>
auto fmt(Arena& arena, String format, A... args) -> String {
    Arena cstr_arena{format.length + 1};
    const char* format_cstr = format.cstr(cstr_arena);

    String string;

    string.length = snprintf(NULL, 0, format_cstr, args...);

    string.data = arena.allocate<const char>(string.length + 1);

    snprintf(const_cast<char*>(string.data), string.length + 1, format_cstr, args...);

    return string;
}

#pragma clang diagnostic pop
