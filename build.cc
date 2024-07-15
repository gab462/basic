#include <unistd.h>
#include <sys/wait.h>

#include "basic.cc"

#define CC "cc", "-std=c++17", "-pedantic", \
    "-Wall", "-Wextra", "-Wshadow", \
    "-fno-exceptions", "-fno-rtti"

auto pwd(Arena& arena) -> String {
    char path[256];
    assert(getcwd(path, 256) != NULL);

    String string;
    string.length = strlen(path);

    char* buffer = arena.allocate<char>(string.length);

    memcpy(buffer, path, string.length);

    string.data = buffer;

    return string;
}

template <typename ...A>
auto run_command(A... args) -> void {
    Arena arena{1024};

    auto cmd = make_array<String>(args...);

    println(String{" "}.join(arena, cmd));

    String bin = (cmd[0].left(2) == "./")
        ? pwd(arena).append(arena, cmd[0].chop_left(1))
        : cmd[0];

    Vector<const char*> cmd_cstrs;
    cmd_cstrs.reserve(arena, cmd.length + 1);
    for (auto& it: cmd) {
        cmd_cstrs.append(it.cstr(arena));
    }
    cmd_cstrs.append(static_cast<char*>(0));

    pid_t pid = fork();

    assert(pid >= 0);

    if (pid == 0) {
        execvp(const_cast<char*>(bin.cstr(arena)), const_cast<char* const*>(cmd_cstrs.data));
        assert(false);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

auto build_self(void) -> void {
    run_command(CC, "-o", "build", "build.cc", "-ggdb");

    run_command("./build", "example");
}

auto build_example(void) -> void {
    run_command(CC, "-o", "example", "example.cc", "-ggdb");
}

auto main(int argc, char* argv[]) -> int {
    String type = argc > 1 ? argv[1] : "self";

    if (type == "self")
        build_self();
    else
        build_example();

    return 0;
}
