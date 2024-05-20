#ifndef BASIC_H_
#define BASIC_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef struct {
  u64 size;
  u64 position;
  max_align_t memory;
} Arena;

Arena* make_arena(u32 size);
void* allocate_impl(Arena* arena, u32 size, u32 alignment);

#define allocate(arena, T) allocate_impl(arena, sizeof(T), alignof(T))
#define allocate_n(arena, T, n) allocate_impl(arena, sizeof(T) * n, alignof(T))

typedef struct {
  u64 length;
  u64 append;
  max_align_t data;
} Vector;

void* make_vector_impl(Arena* arena, u32 type_size, u32 length);
Vector* get_vector(void* v);

#define make_vector(arena, T, n) make_vector_impl(arena, sizeof(T), n)
#define append(v, x) v[get_vector(v)->append++] = x
#define foreach(T, v) for (T* it = v; (u64){ it - v } < get_vector(v)->length; ++it)

typedef struct {
  char* data;
  u64 length;
} String;

// TODO: fmt
String read_file(Arena* arena, String file);
String append_string(Arena* arena, String this, String other);
String* split_string(Arena* arena, String string, char separator);
String join_strings(Arena* arena, String separator, String* strings);
bool string_equals(String this, String other);
bool substring(String this, String other);
u32 count_characters(String string, char character);
char* cstr(Arena* arena, String string);
void println(String string);

#define s(cstr) (String){ .data = cstr, .length = strlen(cstr) }

#endif

#ifdef BASIC_IMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

Arena* make_arena(u32 size) {
  Arena* arena = malloc(sizeof(Arena) - sizeof(max_align_t) + size);

  arena->size = size;
  arena->position = 0;

  return arena;
}

void* allocate_impl(Arena* arena, u32 size, u32 alignment) {
  arena->position += arena->position % alignment;

  assert(arena->position + size <= arena->size);

  void* address = &arena->memory + arena->position;

  arena->position += size;

  return address;
}

void* make_vector_impl(Arena* arena, u32 type_size, u32 length) {
  Vector* vector =
    allocate_impl(arena,
                  sizeof(Vector) - sizeof(max_align_t) + type_size * length,
                  alignof(Vector));

  vector->length = length;
  vector->append = 0;

  return &vector->data;
}

Vector* get_vector(void* v) {
  return (void*){ (char*){ v } - offsetof(Vector, data) };
}

String read_file(Arena* arena, String file) {
  Arena* scratch = make_arena(255);

  char* file_cstr = cstr(scratch, file);

  FILE* f = fopen(file_cstr, "rb");

  free(scratch);

  assert(f);

  assert(fseek(f, 0, SEEK_END) >= 0);

  String string = {
    .length = ftell(f)
  };

  assert(fseek(f, 0, SEEK_SET) >= 0);

  string.data = allocate_n(arena, char, string.length);

  fread(string.data, 1, string.length, f);

  assert(ferror(f) == 0);

  fclose(f);

  return string;
}

String append_string(Arena* arena, String this, String other) {
  String string = {
    .length = this.length + other.length
  };

  string.data = allocate_n(arena, char, string.length);

  memcpy(string.data, this.data, this.length);
  memcpy(string.data + this.length, other.data, other.length);

  return string;
}

String* split_string(Arena* arena, String string, char separator) {
  u32 separator_count = count_characters(string, separator);
  String* strings = make_vector(arena, String, separator_count + 1);

  u64 position = 0;

  for (u64 i = 0; i < string.length; ++i) {
    if (string.data[i] == separator) {
      append(strings, ((String){ string.data + position, i - position }));
      position = i + 1;
    }

    if (i + 1 == string.length)
      append(strings, ((String){ string.data + position, string.length - position }));
  }

  return strings;
}

String join_strings(Arena* arena, String separator, String* strings) {
  String string = {
    .length = 0
  };

  foreach(String, strings) {
    string.length += it->length + separator.length;
  }

  string.length -= separator.length;

  string.data = allocate_n(arena, char, string.length);

  u64 position = 0;

  foreach(String, strings) {
    memcpy(string.data + position, it->data, it->length);
    position += it->length;

    if (position < string.length) {
      // Also copy separator
      memcpy(string.data + position, separator.data, separator.length);
      position += separator.length;
    }
  }

  return string;
}

bool string_equals(String this, String other) {
  return memcmp(this.data, other.data, this.length) == 0;
}

bool substring(String this, String other) {
  if (other.length > this.length)
    return false;

  u64 match = 0;

  for (u64 i = 0; i < this.length; ++i) {
    if (this.data[i] == other.data[match]) {
      ++match;
    } else {
      match = this.data[i] == other.data[0] ? 1 : 0;
    }

    if (match == other.length)
      return true;
  }

  return false;
}

u32 count_characters(String string, char character) {
  u32 count = 0;

  for (u64 i = 0; i < string.length; ++i) {
    if (string.data[i] == character)
      ++count;
  }

  return count;
}

char* cstr(Arena* arena, String string) {
  char* new = allocate_n(arena, char, string.length + 1);

  memcpy(new, string.data, string.length);

  new[string.length] = '\0';

  return new;
}

void println(String string) {
  assert(write(STDOUT_FILENO, string.data, string.length) >= 0);
  assert(write(STDOUT_FILENO, "\n", 1) >= 0);
}

#endif
