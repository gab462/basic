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

    template <typename T, typename ...A>
    auto make(A... args) -> ptr<T> {
        if (position % alignof(T) != 0)
            position += (alignof(T) - (position % alignof(T)));

        assert(position + sizeof(T) <= capacity);

        auto pointer = new(static_cast<ptr<char>>(memory) + position) T{args...};

        position += sizeof(T);

        return pointer;
    }

    template <typename T, typename ...A>
    auto allocate(usize n = 1, A... args) -> ptr<T> {
        if (position % alignof(T) != 0)
            position += (alignof(T) - (position % alignof(T)));

        assert(position + sizeof(T) * n <= capacity);

        auto pointer = new(static_cast<ptr<char>>(memory) + position) T[n]{args...};

        position += sizeof(T) * n;

        return pointer;
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

    auto clone(ref<Arena> arena) -> Container<T> {
        Container<T> container{length, position, nullptr};

        container.data = arena.allocate<T>(length);

        memcpy(container.data, data, sizeof(T) * length);

        return container;
    }

    auto drop(usize n) -> Container<T> {
        assert(n <= length);

        return Container{length - n, position < n ? 0 : position - n, data + n};
    }

    auto take(usize n) -> Container<T> {
        assert(n <= length);

        return Container{n, n < position ? n : position, data};
    }

    auto head() -> ref<T> {
        return data[0];
    }

    auto tail() -> Container<T> {
        return drop(1);
    }

    auto map(func<T, T> op) -> ref<Container<T>> {
        for (ref<T> it: *this) {
            it = op(it);
        }

        return *this;
    }

    // TODO:
    // template <typename U>
    // auto map(ref<Arena> arena, func<U, T> op) -> ref<Vector<U>>

    auto filter(func<bool, T> op) -> ref<Container<T>> {
        usize i = 0;

        for (ref<T> it: *this) {
            if (op(it))
                data[i++] = it;
        }

        position = i;

        return *this;
    }

    auto reduce(func<T, T, T> op) -> T {
        T acc{}; // FIXME: may not be well defined

        for (ref<T> it: *this) {
            acc = op(acc, it);
        }

        return acc;
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

    auto count_characters(char c) -> u32 {
        u32 count = 0;

        for (u32 i = 0; i < length; ++i) {
            if (data[i] == c)
                ++count;
        }

        return count;
    }

    auto split(ref<Arena> arena, char separator) -> Vector<String> {
        auto separator_count = count_characters(separator);
        Vector<String> strings;
        strings.reserve(arena, separator_count + 1);

        usize position = 0;

        for (usize i = 0; i < length; ++i) {
            if (data[i] == separator) {
                strings.append(String{data + position, i - position});
                position = i + 1;
            }

            if (i + 1 == length)
                strings.append(String{data + position, length - position});
        }

        return strings;
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
