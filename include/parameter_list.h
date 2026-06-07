#pragma once

#include <stdlib.h>
#include <stdint.h>

// Forward declaration — avoids pulling in the full Parameter.h header.
class FloatParameter;

// Flat realloc-based list of FloatParameter* pointers.
// Drop-in replacement for LinkedList<FloatParameter*>* in per-object parameter storage.
// Avoids the ~16 bytes of malloc overhead per node that LinkedList<T> incurs.
// API surface deliberately matches the subset used by existing code:
//   add(p), get(idx), size(), empty(), clear(), begin(), end()
class ParameterList {
    FloatParameter** _items   = nullptr;
    uint16_t         _count    = 0;
    uint16_t         _capacity = 0;

    void grow() {
        uint16_t new_cap = (_capacity == 0) ? 4 : (_capacity + 4);
        FloatParameter** n = (FloatParameter**)realloc(_items, new_cap * sizeof(FloatParameter*));
        if (n) { _items = n; _capacity = new_cap; }
    }

public:
    ParameterList() = default;
    ~ParameterList() { free(_items); }

    ParameterList(const ParameterList&)            = delete;
    ParameterList& operator=(const ParameterList&) = delete;

    void add(FloatParameter* item) {
        if (_count >= _capacity) grow();
        if (_count < _capacity) _items[_count++] = item;
    }

    FloatParameter* get(int idx) const { return _items[idx]; }

    uint16_t size()  const { return _count; }
    bool     empty() const { return _count == 0; }
    void     clear()       { _count = 0; }

    // Range-for support
    FloatParameter** begin() { return _items; }
    FloatParameter** end()   { return _items + _count; }
    FloatParameter* const* begin() const { return _items; }
    FloatParameter* const* end()   const { return _items + _count; }
};
