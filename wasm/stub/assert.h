extern "C" auto assert_here(const char* file, size_t len, size_t line, bool cond) -> void; // implemented in js

#define assert(cond) assert_here(__FILE__, strlen(__FILE__), __LINE__, cond)
