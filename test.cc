auto test_types() -> void {
    static_assert(sizeof(u8) == 1);
    static_assert(sizeof(u16) == 2);
    static_assert(sizeof(u32) == 4);
    static_assert(sizeof(u64) == 8);

    static_assert(sizeof(i8) == 1);
    static_assert(sizeof(i16) == 2);
    static_assert(sizeof(i32) == 4);
    static_assert(sizeof(i64) == 8);

    static_assert(sizeof(f32) == 4);
    static_assert(sizeof(f64) == 8);

    // static_assert(typeof(usize) == typeof(sizeof(u8)));
}

auto test_arena(ptr<Arena> arena) -> void {
    auto n = arena->make<i32>(4);

    assert(*n == 4);
    assert(arena->position == 4);

    auto d = arena->make<f64>(12.0);
    assert(*d == 12.0);
    assert(arena->position == 16);

    auto ds = arena->allocate<i32>(4, 1, 2);

    assert(ds[1] == 2);
    assert(arena->position == 32);

    arena->allocate<i32>(0);
    assert(arena->position == 32);
}

auto test_vector(ptr<Arena> arena) -> void {
    auto vec = Vector<i32>::create(arena, 4);
    vec.append(1, 2, 3, 4);
    assert(vec[0] == 1);
    assert(vec[3] == 4);
    assert(vec.length == 4);
    assert(vec.tail == 4);

    auto vec1 = Vector<i32>::create();

    vec1.reserve(arena, 4);
    assert(vec1.length == 4);
    assert(vec1.tail == 0);

    vec1.append(1, 2);
    assert(vec1[0] == 1);
    assert(vec1[1] == 2);
    assert(vec1.length == 4);
    assert(vec1.tail == 2);

    auto vec3 = Vector_Builder<i32>::create(arena);
    vec3.append(1, 2);
    vec3.append(3, 4);
    auto res = vec3.result;

    assert(res[0] == 1);
    assert(res[3] == 4);
    assert(res.length == 4);
    assert(res.tail == 4);
}

auto test_array() -> void {
    auto vec2 = make_array<i32>(5, 6, 7, 8);

    assert(vec2[0] == 5);
    assert(vec2[3] == 8);
    assert(vec2.size() == 4);
    assert(vec2.tail == 4);
}

auto test_stack() -> void {
    auto s = Stack<i32, 3>::create();

    s.push(1);
    assert(s.tail == 1);
    assert(s.data[0] == 1);
    s.push(2);
    assert(s.tail == 2);
    assert(s.data[1] == 2);
    assert(s.pop() == 2);
    assert(s.tail == 1);
}

auto test_queue() -> void {
    auto s = Queue<i32, 3>::create();

    s.put(1);
    assert(s.head == 0);
    assert(s.tail == 1);
    assert(s.data[0] == 1);
    s.put(2, 3);
    assert(s.tail == 0);
    assert(s.data[1] == 2);
    assert(s.data[2] == 3);
    assert(s.get() == 1);
    assert(s.tail == 0);
    assert(s.head == 1);
    s.put(4);
    assert(s.data[0] == 4);
    assert(s.tail == 1);
    assert(s.head == 1);
}

auto test_string(ptr<Arena> arena) -> void {
    assert(String::create("Hello,").append(arena, String::create("World!")) == "Hello,World!");

    auto split = String::create("Hello,World,Test").split(arena, ',');

    assert(split[0] == "Hello");
    assert(split[2] == "Test");

    assert(String::create(", ").join(arena, make_array<String>(String::create("Hello"), String::create("World!"), String::create("Test!")).view())
            == "Hello, World!, Test!");

    assert(String::create("Hello World").substring(String::create("Hello")));
    assert(String::create("Hello World").substring(String::create("World")));
    assert(!String::create("Hello World").substring(String::create("Word")));

    auto hello = String::create("Hello");

    assert(hello.left(2) == "He");
    assert(hello.right(2) == "lo");
    assert(hello.chop_left(2) == "llo");
    assert(hello.chop_right(2) == "Hel");

    assert(hello.cstr(arena)[5] == '\0');

    auto builder = String_Builder::create(arena);
    builder.append(String::create("Hello, "), String::create("World"));
    builder.put('!');
    auto res = builder.result;

    assert(res == "Hello, World!");
}

auto test_defer_aux(ptr<Array<i32, 2>> arr) -> void {
    defer second = [&arr](){ arr->append(2); };
    defer first = [&arr](){ arr->append(1); };
}

auto test_defer() -> void {
    auto arr = Array<i32, 2>::create();

    test_defer_aux(&arr);

    assert(arr.tail == 2);
    assert(arr[0] == 1);
    assert(arr[1] == 2);
}

auto test_all() -> void {
    auto arena = Arena::create(2048);
    defer cleanup = [&arena](){ arena.destroy(); };

    test_arena(&arena);
    test_vector(&arena);
    test_array();
    test_stack();
    test_queue();
    test_string(&arena);
    test_defer();
}
