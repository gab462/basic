#include "../basic.cc"
#include "../test.cc"

extern "C" auto init() -> void {
    test_all();

    println("ok");
}
