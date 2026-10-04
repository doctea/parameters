
#include "parameter_list.h"
#include "parameters/Parameter.h"

FloatParameter* ParameterList::getByName(const char *name) const {
    for (uint16_t i = 0; i < _count; ++i) {
        if (_items[i] && strcmp(_items[i]->label, name) == 0) {
            return _items[i];
        }
    }
    return nullptr;
}