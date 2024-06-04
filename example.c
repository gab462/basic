#define BASIC_IMPLEMENTATION
#include "basic.h"

int main(void) {
    Arena* arena = make_arena(2048);

    i32* n = allocate(arena, i32);

    f64* d = allocate(arena, f64);

    *n = 4;

    *d = 12.0;

    println(s("Hello, World! %d %.2f"), *n, *d);

    i32* vec = make_vector(arena, i32, 4);

    for (i32 i = 0; i < 4; ++i) {
        vec[i] = i;
    }

    foreach(vec) {
        println(s("%d"), *it);
    }

    i32* vec2 = make_vector(arena, i32, 4);

    append(vec2, 5, 6, 7, 8);

    foreach(vec2) {
        println(s("%d"), *it);
    }

    println(read_file(arena, s("LICENSE")));

    println(append_string(arena, s("Hello,"), s("World!")));

    String* strings = make_vector(arena, String, 3);
    append(strings, s("Hello"), s("World!"), s("Test!"));

    println(join_strings(arena, s(", "), strings));

    String* split = split_string(arena, s("Hello,World,Test"), ',');

    foreach(split) {
        println(*it);
    }

    println(s("Contains: %d"), substring(s("Hello World"), s("Hello")));
    println(s("Contains: %d"), substring(s("Hello World"), s("World")));
    println(s("Contains: %d"), substring(s("Hello World"), s("Word")));

    String hello = s("Hello");

    println(string_left(hello, 2));
    println(string_right(hello, 2));
    println(string_chop_left(hello, 2));
    println(string_chop_right(hello, 2));

    free(arena);

    return 0;
}
