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

auto absolute_path(ref<Arena> arena, String file) -> String {
    assert(file.left(2) == "./"); // TODO: other cases

    buf<char, 256> path;
    assert(getcwd(path, 256) != NULL);

    return String_Builder{arena}
        .append(path, file.chop_left(1))
        .result;
}

template <typename ...A>
auto run_command(A... args) -> void {
    Arena arena{1024};

    auto cmd = make_array<String>(args...);

    println(String{" "}.join(arena, cmd));

    String bin = (cmd[0].left(2) == "./")
        ? absolute_path(arena, cmd[0])
        : cmd[0];

    Vector<ptr<imm<char>>> cmd_cstrs;
    cmd_cstrs.reserve(arena, cmd.length + 1);
    for (auto it: cmd) {
        cmd_cstrs.append(it.cstr(arena));
    }
    cmd_cstrs.append(static_cast<ptr<char>>(0));

    pid_t pid = fork();

    assert(pid >= 0);

    if (pid == 0) {
        execvp(const_cast<ptr<char>>(bin.cstr(arena)), const_cast<ptr<imm<ptr<char>>>>(cmd_cstrs.data));
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
    String type = argc > 1 ? argv[1] : "self";

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
