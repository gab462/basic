auto test_arena(ref<Arena> arena) -> void {
    auto n = arena.make<i32>(4);

    assert(*n == 4);
    assert(arena.position == 4);

    auto d = arena.make<f64>(12.0);
    assert(*d == 12.0);
    assert(arena.position == 16);

    auto ds = arena.allocate<i32>(4, 1, 2);

    assert(ds[1] == 2);
    assert(arena.position == 32);

    arena.allocate<i32>(0);
    assert(arena.position == 32);
}

auto test_vector(ref<Arena> arena) -> void {
    auto vec = Vector<i32>{arena, 1, 2, 3, 4};
    assert(vec[0] == 1);
    assert(vec[3] == 4);
    assert(vec.length == 4);
    assert(vec.position == 4);

    Vector<i32> vec1;

    vec1.reserve(arena, 4);
    assert(vec1.length == 4);
    assert(vec1.position == 0);

    vec1.append(1, 2);
    assert(vec1[0] == 1);
    assert(vec1[1] == 2);
    assert(vec1.length == 4);
    assert(vec1.position == 2);

    auto vec3 =
        Vector_Builder<i32>{arena}
        .append(1, 2)
        .append(3, 4)
        .result;

    assert(vec3[0] == 1);
    assert(vec3[3] == 4);
    assert(vec3.length == 4);
    assert(vec3.position == 4);
}

auto test_array() -> void {
    auto vec2 = make_array<i32>(5, 6, 7, 8);

    assert(vec2[0] == 5);
    assert(vec2[3] == 8);
    assert(vec2.length == 4);
    assert(vec2.position == 4);
}

auto test_string(ref<Arena> arena) -> void {
    assert(String{"Hello,"}.append(arena, "World!") == "Hello,World!");

    auto split = String{"Hello,World,Test"}.split(arena, ',');

    assert(split[0] == "Hello");
    assert(split[2] == "Test");

    assert(String{", "}.join(arena, make_array<String>("Hello", "World!", "Test!"))
            == "Hello, World!, Test!");

    assert(String{"Hello World"}.substring("Hello"));
    assert(String{"Hello World"}.substring("World"));
    assert(!String{"Hello World"}.substring("Word"));

    String hello = "Hello";

    assert(hello.left(2) == "He");
    assert(hello.right(2) == "lo");
    assert(hello.chop_left(2) == "llo");
    assert(hello.chop_right(2) == "Hel");

    assert(hello.cstr(arena)[6] == '\0');

    auto builder = String_Builder{arena};
    builder.append("Hello, ", "World");
    builder.put('!');
    auto built = builder.result;

    assert(built == "Hello, World!");
}

auto test_defer_aux(ref<Array<i32, 2>> arr) -> void {
    defer second = [&arr](){ arr.append(2); };
    defer first = [&arr](){ arr.append(1); };
}

auto test_defer() -> void {
    Array<i32, 2> arr;

    test_defer_aux(arr);

    assert(arr.position == 2);
    assert(arr[0] == 1);
    assert(arr[1] == 2);
}

auto test_all() -> void {
    Arena arena{2048};

    test_arena(arena);
    test_vector(arena);
    test_array();
    test_string(arena);
    test_defer();
}
