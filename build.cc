#include <unistd.h>
#include <sys/wait.h>

#include "basic.cc"

#define FLAGS "-std=c++17", "-pedantic", \
    "-Wall", "-Wextra", "-Wshadow", \
    "-fno-exceptions", "-fno-rtti"

#define WASM_FLAGS "--target=wasm32", \
    "-fno-builtin", "--no-standard-libraries", \
    "-Wl,--no-entry,--export-all,--allow-undefined", \
    "-Iwasm/stub"

auto absolute_path(ptr<Arena> arena, String file) -> String {
    assert(file.left(2) == "./"); // TODO: other cases

    buf<char, 256> path;
    assert(getcwd(path, 256) != NULL);

    return String::create("")
        .join(arena, make_array<String>(String::create(path), file.chop_left(1)).data);
}

template <typename ...A>
auto run_command(A... args) -> void {
    auto arena = Arena::create(1024);
    defer cleanup = [&arena](){ arena.destroy(); };

    auto cmd = make_array<ptr<imm<char>>>(args..., static_cast<ptr<char>>(0));
    auto strings = make_array<String>(String::create(args)...);

    println(String::create(" ").join(&arena, strings.data));

    auto bin = (strings[0].left(2) == "./")
        ? absolute_path(&arena, strings[0]).cstr(&arena)
        : cmd[0];

    pid_t pid = fork();

    assert(pid >= 0);

    if (pid == 0) {
        execvp(const_cast<ptr<char>>(bin), const_cast<ptr<imm<ptr<char>>>>(cmd.data.data));
        assert(false);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

auto build_self() -> void {
    run_command("cc", FLAGS, "-o", "build", "build.cc");

    run_command("./build", "run_tests");
}

auto build_tests() -> void {
    run_command("cc", FLAGS, "-o", "run_tests", "run_tests.cc");
}

auto build_wasm() -> void {
    auto src = "wasm/main.cc";
    auto bin = "wasm/index.wasm";

    run_command("clang", FLAGS, WASM_FLAGS, "-o", bin, src);
}

auto clean() -> void {
    run_command("rm", "wasm/index.wasm");
    run_command("rm", "run_tests");
    run_command("rm", "build");
}

auto main(int argc, ptr<ptr<char>> argv) -> int {
    String type = String::create(argc > 1 ? argv[1] : "self");

    if (type == "self")
        build_self();
    else if (type == "wasm")
        build_wasm();
    else if (type == "clean")
        clean();
    else {
        build_tests();
    }

    return 0;
}
