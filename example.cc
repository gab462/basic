#include "basic.cc"

int main(void) {
    Arena arena{2048};

    i32* n = arena.make<i32>(4);

    f64* d = arena.make<f64>(12.0);

    println("Hello, World! %d %.2f", *n, *d);

    Vector<i32> vec{arena, 4};

    for (i32 i = 0; i < 4; ++i) {
        vec[i] = i;
    }

    for (auto& it: vec) {
        println("%d", it);
    }

    Vector<i32> vec2{arena, 4};

    vec2.append(5, 6, 7, 8);

    for (auto& it: vec2) {
        println("%d", it);
    }

    println(read_file(arena, "LICENSE"));

    println(String{"Hello,"}.append(arena, "World!"));

    println(String{", "}.join(arena, Vector<String>{arena, "Hello", "World!", "Test!"}));

    Vector<String> split = String{"Hello,World,Test"}.split(arena, ',');

    for (auto& it: split) {
        println(it);
    }

    println("Contains: %d", String{"Hello World"}.substring("Hello"));
    println("Contains: %d", String{"Hello World"}.substring("World"));
    println("Contains: %d", String{"Hello World"}.substring("Word"));

    String hello = "Hello";

    println(hello.left(2));
    println(hello.right(2));
    println(hello.chop_left(2));
    println(hello.chop_right(2));

    return 0;
}
