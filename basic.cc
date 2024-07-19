#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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

using usize = size_t;

// https://groups.google.com/a/isocpp.org/g/std-proposals/c/xDQR3y5uTZ0/m/VKmOiLRzHqkJ
template <typename T> using ptr = T*;
template <typename T> using ref = T&;
template <typename T> using imm = T const;
template <typename T, usize N> using buf = T[N];
template <typename T, typename ...A> using func = auto (*) (A...) -> T;

auto operator new(usize, ptr<void> p) -> ptr<void> {
    return p;
}

auto operator new[](usize, ptr<void> p) -> ptr<void> {
    return p;
}

struct Arena {
    usize capacity;
    usize position;
    ptr<void> memory;

    explicit Arena(usize n): capacity{n}, position{0}, memory{malloc(n)} {
        assert(memory != NULL);
    }

    ~Arena() {
        free(memory);
    }

    auto end() -> ptr<void> {
        return static_cast<ptr<char>>(memory) + position;
    }

    template <typename T>
    auto align() -> void {
        if (position % alignof(T) != 0)
            position += (alignof(T) - (position % alignof(T)));
    }

    template <typename T, typename ...A>
    auto make(A... args) -> ptr<T> {
        align<T>();

        assert(position + sizeof(T) <= capacity);

        auto pointer = new(end()) T{args...};

        position += sizeof(T);

        return pointer;
    }

    template <typename T, typename ...A>
    auto allocate(usize n = 1, A... args) -> ptr<T> {
        align<T>();

        assert(sizeof...(args) <= n);

        auto pointer = end();

        (make<T>(args), ...);

        // grow without initializing if no arguments
        position += sizeof(T) * (n - sizeof...(args));

        return static_cast<ptr<T>>(pointer);
    }
};

template <typename T>
struct Container {
    usize length;
    usize position;
    ptr<T> data;

    auto push(T value) -> void {
        assert(position < length);

        data[position++] = value;
    }

    template <typename ...A>
    auto append(A... args) -> ref<Container<T>> {
        (push(args), ...);

        return *this;
    }

    auto operator[] (usize n) -> ref<T> {
        return data[n];
    }

    auto begin() -> ptr<T> {
        return &data[0];
    }

    auto end() -> ptr<T> {
        return &data[position];
    }
};

template <typename T, usize N>
struct Array: Container<T> {
    buf<T, N> buffer;

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
    Vector(ref<Arena> arena, A... args): Container<T>{sizeof...(args), sizeof...(args), nullptr} {
        Container<T>::data = arena.allocate<T>(Container<T>::length, args...);
    }

    auto reserve(ref<Arena> arena, usize n) -> ref<Vector<T>> {
        assert(Container<T>::data == nullptr);

        Container<T>::data = arena.allocate<T>(n);
        Container<T>::length = n;

        return *this;
    }
};

template <typename T>
struct Vector_Builder {
    ref<Arena> arena;
    ptr<T> end;
    Vector<T> result;

    Vector_Builder(ref<Arena> a): arena{a}, result{} {
        a.align<T>();
        result.data = static_cast<ptr<T>>(a.end());
        end = result.data;
    }

    auto grow(usize n) -> void {
        result.length += n;
        result.position += n;
        end += n;
    }

    template <typename ...A>
    auto append(A... args) -> ref<Vector_Builder<T>> {
        auto n = sizeof...(args);

        auto space = arena.allocate<T>(n, args...);

        assert(end == space);

        grow(n);

        return *this;
    }

    template <typename ...A>
    auto put(A... args) -> ref<Vector_Builder<T>> {
        auto space = arena.make<T>(args...);

        assert(end == space);

        grow(1);

        return *this;
    }
};

struct String {
    usize length;
    ptr<imm<char>> data;

    String(ptr<imm<char>> s): length{strlen(s)}, data{s} {}
    String(ptr<imm<char>> s, usize n): length{n}, data{s} {}
    String(): length{0}, data{nullptr} {}

    auto operator==(String other) -> bool {
        if (length != other.length)
            return false;

        return memcmp(data, other.data, length) == 0;
    }

    auto append(ref<Arena> arena, String other) -> String {
        String string;

        string.length = length + other.length;

        auto buffer = arena.allocate<char>(string.length);

        memcpy(buffer, data, length);
        memcpy(buffer + length, other.data, other.length);

        string.data = buffer;

        return string;
    }

    auto split(ref<Arena> arena, char separator) -> Vector<String> {
        Vector_Builder<String> strings{arena};

        usize position = 0;

        for (usize i = 0; i < length; ++i) {
            if (data[i] == separator) {
                strings.put(data + position, i - position);
                position = i + 1;
            }

            if (i + 1 == length)
                strings.put(data + position, length - position);
        }

        return strings.result;
    }

    auto join(ref<Arena> arena, Container<String> strings) -> String {
        String string;

        for (auto it: strings) {
            string.length += it.length + length;
        }

        string.length -= length;

        auto buffer = arena.allocate<char>(string.length);

        usize position = 0;

        for (auto it: strings) {
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
    auto format(ref<Arena> arena, A... args) -> String {
        Arena cstr_arena{length + 1};
        auto format_cstr = cstr(cstr_arena);

        String string;

        string.length = snprintf(NULL, 0, format_cstr, args...);

        auto buffer = arena.allocate<char>(string.length + 1);

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

    auto right(usize n) -> String {
        return { data + length - n, n };
    }

    auto left(usize n) -> String {
        return { data, n };
    }

    auto chop_right(usize n) -> String {
        return { data, length - n };
    }

    auto chop_left(usize n) -> String {
        return { data + n, length - n };
    }
    
    auto cstr(ref<Arena> arena) -> ptr<imm<char>> {
        auto s = arena.allocate<char>(length + 1);

        memcpy(s, data, length);

        s[length] = '\0';

        return s;
    }

    static auto from_file(ref<Arena> arena, String path) -> String {
        Arena scratch{path.length + 1};

        auto path_cstr = path.cstr(scratch);

        auto file = fopen(path_cstr, "rb");

        assert(file != NULL);

        assert(fseek(file, 0, SEEK_END) >= 0);

        String string;

        string.length = ftell(file);

        assert(fseek(file, 0, SEEK_SET) >= 0);

        auto buffer = arena.allocate<char>(string.length);

        fread(buffer, 1, string.length, file);

        assert(ferror(file) == 0);

        fclose(file);

        string.data = buffer;

        return string;
    }
};

struct String_Builder {
    ref<Arena> arena;
    ptr<char> end;
    String result;

    String_Builder(ref<Arena> a): arena{a}, result{} {
        result.data = static_cast<ptr<char>>(a.end());
        end = static_cast<ptr<char>>(a.end());
    }

    auto grow(usize n) -> void {
        result.length += n;
        end += n;
    }

    auto push(String string) -> void {
        auto space = arena.allocate<char>(string.length);

        assert(end == space);

        memcpy(space, string.data, string.length);

        grow(string.length);
    }

    template <typename ...A>
    auto append(A... args) -> ref<String_Builder> {
        (push(args), ...);

        return *this;
    }

    auto put(char c) -> void {
        auto space = arena.make<char>(c);

        assert(end == space);

        grow(1);
    }
};

template <typename F>
struct defer {
    F callback;

    defer(F f): callback{f} {}

    ~defer() {
        callback();
    }
};

auto println() -> void {
    assert(write(STDOUT_FILENO, "\n", 1) >= 0);
}

auto println(String string) -> void {
    assert(write(STDOUT_FILENO, string.data, string.length) >= 0);
    assert(write(STDOUT_FILENO, "\n", 1) >= 0);
}

auto println(ptr<imm<char>> s) -> void {
    println(String{s});
}

template <typename ...A>
auto println(ptr<imm<char>> format, A... args) -> void {
    Arena arena{static_cast<usize>(snprintf(NULL, 0, format, args...)) + 1};

    println(String{format}.format(arena, args...));
}
