#include <unistd.h>
#include <sys/wait.h>

#include "basic.cc"

String pwd(Arena& arena) {
    char path[256];
    assert(getcwd(path, 256) != NULL);

    String string;
    string.length = strlen(path);
    string.data = arena.allocate<const char>(string.length);

    memcpy(const_cast<char*>(string.data), path, string.length);

    return string;
}

void run_command(Vector<String> cmd) {
    Arena arena{1024};

    println(String{" "}.join(arena, cmd));

    String bin = (cmd[0].left(2) == "./")
        ? pwd(arena).append(arena, cmd[0].chop_left(1))
        : cmd[0];

    Vector<const char*> cmd_cstrs{arena, cmd.length + 1};
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

void cc(Vector<String>& cmd) {
    cmd.append("clang");
    cmd.append("-std=c++17", "-pedantic");
    cmd.append("-Wall", "-Wextra", "-Wshadow");
    cmd.append("-fno-exceptions", "-fno-rtti");
}

void build_self(void) {
    Arena arena{1024};
    Vector<String> cmd{arena, 12};

    cc(cmd);
    cmd.append("-o", "build");
    cmd.append("build.cc");
    cmd.append("-ggdb");

    run_command(cmd);

    Vector<String> self{arena, 2};
    self.append("./build", "example");

    run_command(self);
}

void build_example(void) {
    Arena arena{1024};
    Vector<String> cmd{arena, 12};

    cc(cmd);
    cmd.append("-o", "example");
    cmd.append("example.cc");
    cmd.append("-ggdb");

    run_command(cmd);
}

int main(int argc, char* argv[]) {
    String type = argc > 1 ? argv[1] : "self";

    if (type == "self")
        build_self();
    else
        build_example();

    return 0;
}
