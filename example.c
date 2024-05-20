#include <stdio.h>

#define BASIC_IMPLEMENTATION
#include "basic.h"

int main(void) {
  Arena* arena = make_arena(16);

  int* n = allocate(arena, int);

  double* d = allocate(arena, double);

  *n = 4;

  *d = 12.0;

  printf("Hello, World! %d %.2f\n", *n, *d);

  free(arena);

  arena = make_arena(64);

  int* vec = make_vector(arena, int, 4);

  for (int i = 0; i < 4; ++i) {
    vec[i] = i;
  }

  foreach(int, vec) {
    printf("%d\n", *it);
  }

  int* vec2 = make_vector(arena, int, 4);

  append(vec2, 5);
  append(vec2, 6);
  append(vec2, 7);
  append(vec2, 8);

  foreach(int, vec2) {
    printf("%d\n", *it);
  }

  free(arena);

  arena = make_arena(2048);

  //println(read_file(arena, s("main.c")));

  println(append_string(arena, s("Hello,"), s("World!")));

  String* strings = make_vector(arena, String, 3);
  append(strings, s("Hello"));
  append(strings, s("World!"));
  append(strings, s("Test!"));

  println(join_strings(arena, s(", "), strings));

  String* split = split_string(arena, s("Hello,World,Test"), ',');

  foreach(String, split) {
    println(*it);
  }

  free(arena);

  printf("Contains: %d\n", substring(s("Hello World"), s("Hello")));
  printf("Contains: %d\n", substring(s("Hello World"), s("World")));
  printf("Contains: %d\n", substring(s("Hello World"), s("Word")));

  return 0;
}
