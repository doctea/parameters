#pragma once

#include <stdint.h>

enum VALUE_TYPE {
  BIPOLAR,
  UNIPOLAR
};

// Modulation input interpretation mode per modulation slot.
// Keep 0/1 aligned with existing BIPOLAR/UNIPOLAR values for compatibility.
enum MODULATION_SLOT_MODE : uint8_t {
  MOD_SLOT_BI_NATIVE = BIPOLAR,
  MOD_SLOT_UNI_RAW = UNIPOLAR,
  MOD_SLOT_UNI_CENTERED = 2
};

// Compact UI labels for slot modulation modes (easy to redefine project-wide).
#ifndef MOD_SLOT_LABEL_BI_NATIVE
  #define MOD_SLOT_LABEL_BI_NATIVE "B+-"
#endif
#ifndef MOD_SLOT_LABEL_UNI_RAW
  #define MOD_SLOT_LABEL_UNI_RAW "U+ "
#endif
#ifndef MOD_SLOT_LABEL_UNI_CENTERED
  #define MOD_SLOT_LABEL_UNI_CENTERED "U+-"
#endif
