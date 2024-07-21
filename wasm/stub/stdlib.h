extern unsigned char __heap_base;

size_t _heap_position = 0;
size_t _heap_depth = 0;
static constexpr size_t _max_heap_depth = 1; // TODO: depth with list of positions
static constexpr size_t _max_alignment = 16;

auto free(void*) -> void {
    size_t start = reinterpret_cast<size_t>(&__heap_base);

    _heap_position = start;

    if (_heap_position % _max_alignment != 0)
        _heap_position += _max_alignment - (_heap_position % _max_alignment);

    _heap_position -= start;
    _heap_depth = 0;
}

auto malloc(size_t n) -> void* {
    if (_heap_depth++ >= _max_heap_depth)
        return NULL;

    free(nullptr); // Align and set _heap_position

    void* space = &__heap_base + _heap_position;

    _heap_position += n;

    return space;
}
