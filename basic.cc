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

template <typename F>
struct defer {
    F callback;

    defer(F f): callback{f} {}

    ~defer() {
        callback();
    }
};

struct Arena {
    usize capacity;
    usize position;
    ptr<void> memory;

    static auto create(usize n) -> Arena {
        auto mem = malloc(n);
        assert(mem != NULL);

        return { n, 0, mem };
    }

    auto destroy() -> void {
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

        assert(position + sizeof(T) < capacity);

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
    usize tail;
    ptr<T> data;

    auto append(T value) -> void {
        assert(tail < length);

        data[tail++] = value;
    }

    template <typename ...A>
    auto append(A... args) -> void {
        (append(args), ...);
    }

    auto operator[] (usize n) -> ref<T> {
        return data[n];
    }

    auto begin() -> ptr<T> {
        return &data[0];
    }

    auto end() -> ptr<T> {
        return &data[tail];
    }
};

template <typename T, usize N>
struct Array {
    buf<T, N> data;
    usize tail;

    static auto create() -> Array<T, N> {
        return {};
    }

    template <typename ...A>
    static auto create(A... args) -> Array<T, N> {
        return { { args... }, sizeof...(args) };
    }

    auto append(T obj) -> void {
        assert(tail < N);

        data[tail++] = obj;
    }

    template <typename ...A>
    auto append(A... args) -> void {
        (append(args), ...);
    }

    auto view() -> Container<T> {
        return { N, tail, data };
    }

    auto size() -> usize {
        return N;
    }

    auto operator[] (usize n) -> ref<T> {
        return data[n];
    }

    auto begin() -> ptr<T> {
        return &data[0];
    }

    auto end() -> ptr<T> {
        return &data[tail];
    }
};

template <typename T, typename ...A>
auto make_array(A... args) -> Array<T, sizeof...(args)> {
    return Array<T, sizeof...(args)>::create(args...);
}

template <typename T, usize N>
struct Stack {
    buf<T, N> data;
    usize tail;

    static auto create() -> Stack<T, N> {
        return {};
    }

    auto push(T obj) -> void {
        assert(tail < N);

        data[tail++] = obj;
    }

    template <typename ...A>
    auto push(A... args) -> void {
        (push(args), ...);
    }

    auto pop() -> T {
        assert(tail > 0);

        return data[--tail];
    }

    auto size() -> usize {
        return tail;
    }

    auto begin() -> T* {
        return &data[0];
    }

    auto end() -> T* {
        return &data[tail];
    }
};

template <typename T, usize N>
struct Queue {
    buf<T, N> data;
    usize head;
    usize tail;
    usize count;

    static auto create() -> Queue<T, N> {
        return {};
    }

    auto put(T obj) -> void {
        assert(count < N);

        data[tail++] = obj;
        tail %= N;
        ++count;
    }

    template <typename ...A>
    auto put(A... args) -> void {
        (put(args), ...);
    }

    auto get() -> T {
        assert(count > 0);

        auto idx = head++;
        head %= N;
        --count;

        return data[idx];
    }

    auto size() -> usize {
        return count;
    }

    auto begin() -> T* {
        return &data[0];
    }

    auto end() -> T* {
        return &data[tail];
    }
};

template <typename T>
struct Vector {
    usize tail;
    usize length;
    ptr<T> data;

    static auto create() -> Vector<T> {
        return {};
    }

    static auto create(ptr<Arena> arena, usize n) -> Vector<T> {
        auto vector = Vector::create();
        vector.reserve(arena, n);
        return vector;
    }

    auto reserve(ptr<Arena> arena, usize n) -> void {
        assert(data == nullptr);

        data = arena->allocate<T>(n);
        length = n;
    }

    template <typename ...A>
    auto append(T obj) -> void {
        assert(tail < length);

        data[tail++] = obj;
    }

    template <typename ...A>
    auto append(A... args) -> void {
        (append(args), ...);
    }

    auto view() -> Container<T> {
        return { length, tail, data };
    }

    auto size() -> usize {
        return length;
    }

    auto operator[] (usize n) -> ref<T> {
        return data[n];
    }

    auto begin() -> ptr<T> {
        return &data[0];
    }

    auto end() -> ptr<T> {
        return &data[tail];
    }
};

template <typename T>
struct Vector_Builder {
    ptr<Arena> arena;
    ptr<T> end;
    Vector<T> result;

    static auto create(ptr<Arena> a) -> Vector_Builder<T> {
        a->align<T>();

        auto start = static_cast<ptr<T>>(a->end());

        return { a, start, { 0, 0, start } };
    }

    auto grow(usize n) -> void {
        result.length += n;
        result.tail += n;
        end += n;
    }

    template <typename ...A>
    auto append(A... args) -> void {
        static constexpr auto n = sizeof...(args);

        auto space = arena->allocate<T>(n, args...);

        assert(end == space);

        grow(n);
    }

    template <typename ...A>
    auto put(A... args) -> void {
        auto space = arena->make<T>(args...);

        assert(end == space);

        grow(1);
    }
};

struct String {
    ptr<imm<char>> data;
    usize length;

    static auto create() -> String {
        return {};
    }

    static auto create(ptr<imm<char>> s) -> String {
        return { s, strlen(s) };
    }

    static auto create(ptr<imm<char>> s, usize n) -> String {
        return { s, n };
    }

    auto operator==(String other) -> bool {
        if (length != other.length)
            return false;

        return memcmp(data, other.data, length) == 0;
    }

    auto operator==(ptr<imm<char>> s) -> bool {
        auto other = String::create(s);

        return *this == other;
    }

    auto append(ptr<Arena> arena, String other) -> String {
        auto string = String::create();

        string.length = length + other.length;

        auto buffer = arena->allocate<char>(string.length);

        memcpy(buffer, data, length);
        memcpy(buffer + length, other.data, other.length);

        string.data = buffer;

        return string;
    }

    auto split(ptr<Arena> arena, char separator) -> Vector<String> {
        auto strings = Vector_Builder<String>::create(arena);

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

    auto join(ptr<Arena> arena, Container<String> strings) -> String {
        auto string = String::create();

        for (auto it: strings) {
            string.length += it.length + length;
        }

        string.length -= length;

        auto buffer = arena->allocate<char>(string.length);

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
    auto format(ptr<Arena> arena, A... args) -> String {
        auto cstr_arena = Arena::create(length + 1);
        defer cleanup = [&cstr_arena](){ cstr_arena.destroy(); };

        auto format_cstr = cstr(&cstr_arena);

        auto string = String::create();

        string.length = snprintf(NULL, 0, format_cstr, args...);

        auto buffer = arena->allocate<char>(string.length + 1);

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
    
    auto cstr(ptr<Arena> arena) -> ptr<imm<char>> {
        auto s = arena->allocate<char>(length + 1);

        memcpy(s, data, length);

        s[length] = '\0';

        return s;
    }

    static auto from_file(ptr<Arena> arena, String path) -> String {
        auto scratch = Arena::create(path.length + 1);
        defer cleanup = [&scratch](){ scratch.destroy(); };

        auto path_cstr = path.cstr(&scratch);

        auto file = fopen(path_cstr, "rb");

        assert(file != NULL);

        assert(fseek(file, 0, SEEK_END) >= 0);

        auto string = String::create();

        string.length = ftell(file);

        assert(fseek(file, 0, SEEK_SET) >= 0);

        auto buffer = arena->allocate<char>(string.length);

        fread(buffer, 1, string.length, file);

        assert(ferror(file) == 0);

        fclose(file);

        string.data = buffer;

        return string;
    }
};

struct String_Builder {
    ptr<Arena> arena;
    ptr<char> end;
    String result;

    static auto create(ptr<Arena> a) -> String_Builder {
        auto start = static_cast<ptr<char>>(a->end());

        return { a, start, { start, 0 } };
    }

    auto grow(usize n) -> void {
        result.length += n;
        end += n;
    }

    auto push(String string) -> void {
        auto space = arena->allocate<char>(string.length);

        assert(end == space);

        memcpy(space, string.data, string.length);

        grow(string.length);
    }

    template <typename ...A>
    auto append(A... args) -> void {
        (push(args), ...);
    }

    auto put(char c) -> void {
        auto space = arena->make<char>(c);

        assert(end == space);

        grow(1);
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
    println(String::create(s));
}

template <typename ...A>
auto println(ptr<imm<char>> format, A... args) -> void {
    auto arena = Arena::create(static_cast<usize>(snprintf(NULL, 0, format, args...)) + 1);
    defer cleanup = [&arena](){ arena.destroy(); };

    println(String::create(format).format(&arena, args...));
}
