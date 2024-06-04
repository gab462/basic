#include <unistd.h>
#include <sys/wait.h>

#define BASIC_IMPLEMENTATION
#include "basic.h"

String pwd(Arena* arena) {
    char path[256];
    assert(getcwd(path, 256) != NULL);

    String string = {
        .length = strlen(path)
    };
    string.data = allocate_n(arena, char, string.length);
    memcpy(string.data, path, string.length);

    return string;
}

void run_command(String* cmd) {
    Arena* arena = make_arena(1024);

    println(join_strings(arena, s(" "), cmd));

    String bin = string_equals(string_left(cmd[0], 2), s("./"))
        ? append_string(arena, pwd(arena), string_chop_left(cmd[0], 1))
        : cmd[0];

    char** cmd_cstrs = make_vector(arena, char*, vector_length(cmd) + 1);
    foreach(cmd) {
        append(cmd_cstrs, cstr(arena, *it));
    }
    append(cmd_cstrs, 0);

    pid_t pid = fork();

    assert(pid >= 0);

    if (pid == 0) {
        execvp(cstr(arena, bin), cmd_cstrs);
        assert(false);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }

    free(arena);
}

void cc(String* cmd) {
    append(cmd, s("clang"));
    append(cmd, s("-std=c23"), s("-pedantic"));
    append(cmd, s("-Wall"), s("-Wextra"), s("-Wshadow"));
}

void build_self(void) {
    Arena* arena = make_arena(1024);
    String* cmd = make_vector(arena, String, 10);

    cc(cmd);
    append(cmd, s("-o"), s("build"));
    append(cmd, s("build.c"));
    append(cmd, s("-ggdb"));

    run_command(cmd);

    String* self = make_vector(arena, String, 2);
    append(self, s("./build"), s("example"));

    run_command(self);

    free(arena);
}

void build_example(void) {
    Arena* arena = make_arena(1024);
    String* cmd = make_vector(arena, String, 10);

    cc(cmd);
    append(cmd, s("-o"), s("example"));
    append(cmd, s("example.c"));
    append(cmd, s("-ggdb"));

    run_command(cmd);

    free(arena);
}

int main(int argc, char* argv[]) {
    String type = argc > 1 ? s(argv[1]) : s("self");

    if (string_equals(type, s("self")))
        build_self();
    else
        build_example();

    return 0;
}
