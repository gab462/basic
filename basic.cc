#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

// https://groups.google.com/a/isocpp.org/g/std-proposals/c/xDQR3y5uTZ0/m/VKmOiLRzHqkJ
template <typename T, typename ...A> using func = auto (*) (A...) -> T;

auto operator new(size_t, void* ptr) -> void* {
    return ptr;
}

auto operator new[](size_t, void* ptr) -> void* {
    return ptr;
}

struct Arena {
    size_t capacity;
    u32 position;
    void* memory;

    explicit Arena(size_t n): capacity{n}, position{0}, memory{malloc(n)} {
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
struct Container {
    size_t length;
    u32 position;
    T* data;

    auto push(T value) -> void {
        assert(position < length);

        data[position++] = value;
    }

    template <typename ...A>
    auto append(A... args) -> Container<T>& {
        (push(args), ...);

        return *this;
    }

    auto operator[] (u32 n) -> T& {
        return data[n];
    }

    auto begin() -> T* {
        return &data[0];
    }

    auto end() -> T* {
        return &data[position];
    }

    auto clone(Arena& arena) -> Container<T> {
        Container<T> container{length, position, nullptr};

        container.data = arena.allocate<T>(length);

        memcpy(container.data, data, sizeof(T) * length);

        return container;
    }

    auto drop(u32 n) -> Container<T> {
        assert(n <= length);

        return Container{length - n, position < n ? 0 : position - n, data + n};
    }

    auto take(u32 n) -> Container<T> {
        assert(n <= length);

        return Container{n, n < position ? n : position, data};
    }

    auto head() -> T& {
        return data[0];
    }

    auto tail() -> Container<T> {
        return drop(1);
    }

    auto map(func<T, T> op) -> Container<T>& {
        for (auto& it: *this) {
            it = op(it);
        }

        return *this;
    }

    auto filter(func<bool, T> op) -> Container<T>& {
        u32 i = 0;

        for (auto& it: *this) {
            if (op(it))
                data[i++] = it;
        }

        position = i;

        return *this;
    }

    auto reduce(func<T, T, T> op) -> T {
        T acc{}; // FIXME: may not be well defined

        for (auto& it: *this) {
            acc = op(acc, it);
        }

        return acc;
    }
};

template <typename T, size_t N>
struct Array: Container<T> {
    T buffer[N];

    Array(): Container<T>{N, 0, buffer} {}

    template <typename ...A>
    Array(A... args): Container<T>{N, sizeof...(args), buffer}, buffer{args...} {}
};

// FIXME: constructor
template <typename T, typename ...A>
auto make_array(A... args) -> Array<T, sizeof...(args)> {
    return Array<T, sizeof...(args)>{args...};
}

template <typename T>
struct Vector: Container<T> {
    Vector(): Container<T>{} {}

    template <typename ...A>
    Vector(Arena& arena, A... args): Container<T>{sizeof...(args), sizeof...(args), nullptr} {
        Container<T>::data = arena.allocate<T>(Container<T>::length, args...);
        Container<T>::position = Container<T>::length;
    }

    auto reserve(Arena& arena, size_t n) -> Vector<T>& {
        assert(Container<T>::data == nullptr);

        Container<T>::data = arena.allocate<T>(n);
        Container<T>::length = n;

        return *this;
    }
};

struct String {
    size_t length;
    const char* data;

    String(const char* s): length{strlen(s)}, data{s} {}
    String(const char* s, size_t n): length{n}, data{s} {}
    String(): length{0}, data{nullptr} {}

    auto operator==(String other) -> bool {
        if (length != other.length)
            return false;

        return memcmp(data, other.data, length) == 0;
    }

    auto append(Arena& arena, String other) -> String {
        String string;

        string.length = length + other.length;

        char* buffer = arena.allocate<char>(string.length);

        memcpy(buffer, data, length);
        memcpy(buffer + length, other.data, other.length);

        string.data = buffer;

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
        Vector<String> strings;
        strings.reserve(arena, separator_count + 1);

        size_t position = 0;

        for (size_t i = 0; i < length; ++i) {
            if (data[i] == separator) {
                strings.append(String{data + position, i - position});
                position = i + 1;
            }

            if (i + 1 == length)
                strings.append(String{data + position, length - position});
        }

        return strings;
    }

    auto join(Arena& arena, Container<String> strings) -> String {
        String string;

        for (auto& it: strings) {
            string.length += it.length + length;
        }

        string.length -= length;

        char* buffer = arena.allocate<char>(string.length);

        u32 position = 0;

        for (auto& it: strings) {
            memcpy(buffer + position, it.data, it.length);
            position += it.length;

            if (position < string.length) {
                // Also copy separator
                memcpy(buffer + position, data, length);
                position += length;
            }
        }

        string.data = buffer;

        return string;
    }

    template <typename ...A>
    auto format(Arena& arena, A... args) -> String {
        Arena cstr_arena{length + 1};
        const char* format_cstr = cstr(cstr_arena);

        String string;

        string.length = snprintf(NULL, 0, format_cstr, args...);

        char* buffer = arena.allocate<char>(string.length + 1);

        snprintf(buffer, string.length + 1, format_cstr, args...);

        string.data = buffer;

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

    char* buffer = arena.allocate<char>(string.length);

    fread(buffer, 1, string.length, file);

    assert(ferror(file) == 0);

    fclose(file);

    string.data = buffer;

    return string;
}

auto println() -> void {
    assert(write(STDOUT_FILENO, "\n", 1) >= 0);
}

auto println(String string) -> void {
    assert(write(STDOUT_FILENO, string.data, string.length) >= 0);
    assert(write(STDOUT_FILENO, "\n", 1) >= 0);
}

auto println(const char* s) -> void {
    println(String{s});
}

template <typename ...A>
auto println(const char* format, A... args) -> void {
    Arena arena{static_cast<size_t>(snprintf(NULL, 0, format, args...)) + 1};

    println(String{format}.format(arena, args...));
}
