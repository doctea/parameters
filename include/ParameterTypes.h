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

// Slot mode semantics (all scaled so amount=1.0 represents full parameter span):
// - MOD_SLOT_UNI_RAW:     source 0..1, interpreted as one-sided positive modulation.
// - MOD_SLOT_UNI_CENTERED: source centered around 0 from unipolar input.
// - MOD_SLOT_BI_NATIVE:   source -1..1 from bipolar-capable input.
//
// The UI labels below are intentionally centralized so compact text can be tuned
// without touching menu/widget code.

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

// Set to 1 for lightweight runtime tracing of CV pitch-bend routing and applied values.
#ifndef CV_PITCH_BEND_TRACE
  #define CV_PITCH_BEND_TRACE 0
#endif
