#define BASIC_IMPLEMENTATION
#include "basic.h"

int main(void) {
    Arena* arena = make_arena(2048);

    int* n = allocate(arena, int);

    double* d = allocate(arena, double);

    *n = 4;

    *d = 12.0;

    println(s("Hello, World! %d %.2f"), *n, *d);

    int* vec = make_vector(arena, int, 4);

    for (int i = 0; i < 4; ++i) {
        vec[i] = i;
    }

    foreach(int, vec) {
        println(s("%d"), *it);
    }

    int* vec2 = make_vector(arena, int, 4);

    append(vec2, 5);
    append(vec2, 6);
    append(vec2, 7);
    append(vec2, 8);

    foreach(int, vec2) {
        println(s("%d"), *it);
    }

    println(read_file(arena, s("LICENSE")));

    println(append_string(arena, s("Hello,"), s("World!")));

    String* strings = make_vector(arena, String, 3);
    append(strings, s("Hello"));
    append(strings, s("World!"));
    append(strings, s("Test!"));

    println(join_strings(arena, s(", "), strings));

    String* split = split_string(arena, s("Hello,World,Test"), ',');

    foreach(String, split) {
        println(*it);
    }

    println(s("Contains: %d"), substring(s("Hello World"), s("Hello")));
    println(s("Contains: %d"), substring(s("Hello World"), s("World")));
    println(s("Contains: %d"), substring(s("Hello World"), s("Word")));

    free(arena);

    return 0;
}
