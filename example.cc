#include "basic.cc"

auto main(void) -> int {
    Arena arena{2048};

    auto n = arena.make<i32>(4);

    auto d = arena.make<f64>(12.0);

    println("Hello, World! %d %.2f", *n, *d);

    Vector<i32> vec;

    vec.reserve(arena, 4);

    vec.append(1, 2, 3, 4);

    for (auto it: vec) {
        println("%d", it);
    }

    println();

    //Array<i32, 4> vec2{5, 6, 7, 8};
    auto vec2 = make_array<i32>(5, 6, 7, 8);

    for (auto it: vec2) {
        println("%d", it);
    }

    println();

    auto sum = make_array<i32>(1, 2, 3, 4, 5, 6)
        .map([](i32 x){ return x + x; })
        .filter([](i32 x){ return x % 3 == 0; })
        .reduce([](i32 x, i32 y){ return x + y; });

    println("%d", sum);

    println();

    println(String::from_file(arena, "LICENSE"));

    println(String{"Hello,"}.append(arena, "World!"));

    println(String{", "}.join(arena, make_array<String>("Hello", "World!", "Test!")));

    auto split = String{"Hello,World,Test"}.split(arena, ',');

    for (auto it: split) {
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
