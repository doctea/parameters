#pragma once

#ifdef ENABLE_SCREEN

// Include only the lightweight abstract base — avoids the circular dependency:
//   OscillatorTuningControls.h -> CVOutputParameter.h -> ParameterInputMenuItems.h
//   -> ParameterManager.h -> OscillatorTuningControls.h
#include "parameters/CVOutputParameterBase.h"
#include <LinkedList.h>
#include "submenuitem.h"
#include "submenuitem_bar.h"
#include "menuitems_lambda.h"
#include "midi_helpers.h"

#include "mymenu/menuitems_scale.h"

// State object for the oscillator tuning page.
// Holds a list of available CVOutputParameterBase* and manages
// the selected output, pitch A/B, active pitch, and octave position.
class CVOutputTuningState {
public:
    LinkedList<CVOutputParameterBase *> *available_outputs = nullptr;
    int8_t selected_output_index = 0;
    int8_t pitch_a = 48;   // C3
    int8_t pitch_b = 60;   // C4
    bool   active_is_b    = false;
    int8_t octave_position = 4;   // C4 = MIDI 60

    CVOutputTuningState(LinkedList<CVOutputParameterBase *> *outputs)
        : available_outputs(outputs) {}

    CVOutputParameterBase *get_selected_output() {
        if (available_outputs == nullptr || available_outputs->size() == 0)
            return nullptr;
        int8_t idx = constrain(selected_output_index, 0, (int8_t)(available_outputs->size() - 1));
        return available_outputs->get(idx);
    }

    const char *get_selected_label() {
        auto *out = get_selected_output();
        if (out == nullptr) return "(none)";
        return out->get_cv_label();
    }

    void send_note(int8_t pitch) {
        auto *out = get_selected_output();
        if (out == nullptr) return;
        out->tuning_send_note(pitch);
    }

    void toggle_pitches() {
        active_is_b = !active_is_b;
        send_note(active_is_b ? pitch_b : pitch_a);
    }

    // Note at current octave is C (midi = octave_position * 12)
    int8_t current_octave_note() const {
        return (int8_t)(octave_position * 12);
    }

    void next_octave() {
        octave_position++;
        if (octave_position > 10) octave_position = 0;
        send_note(current_octave_note());
    }

    void do_note_off() {
        auto *out = get_selected_output();
        if (out == nullptr) return;
        out->tuning_note_off();
    }

    void zero_output() {
        auto *out = get_selected_output();
        if (out == nullptr) return;
        out->tuning_zero_output();
    }

    void disconnect_modulation() {
        auto *out = get_selected_output();
        if (out == nullptr) return;
        out->tuning_disconnect_modulation();
    }
};


// Build a SubMenuItem for the oscillator tuning page.
FLASHMEM
inline SubMenuItem *makeOscillatorTuningSubMenu(CVOutputTuningState *state) {
    SubMenuItem *root = new SubMenuItem("Osc Tuning", true, false);

    // --- Output selector ---
    auto *output_sel = new LambdaNumberControl<int8_t>(
        "Output",
        [=](int8_t v) -> void {
            state->do_note_off();
            state->selected_output_index = constrain(v, (int8_t)0,
                (int8_t)(state->available_outputs->size() - 1));
        },
        [=]() -> int8_t { return state->selected_output_index; },
        nullptr,
        (int8_t)0,
        (int8_t)(state->available_outputs != nullptr ? state->available_outputs->size() - 1 : 0),
        false,
        false
    );
    // Override display to show the label of the selected output rather than just a number
    root->add(output_sel);

    // --- Disconnect / zero / disconnect-mod buttons in a bar ---
    SubMenuItemBar *disc_bar = new SubMenuItemBar("Disconnect", true, false);
    disc_bar->add(new LambdaActionItem("NoteOff",
        [=]() -> void { state->do_note_off(); }
    ));
    disc_bar->add(new LambdaActionItem("Zero Out",
        [=]() -> void { state->zero_output(); }
    ));
    disc_bar->add(new LambdaActionItem("Disc Mod",
        [=]() -> void { state->disconnect_modulation(); }
    ));
    root->add(disc_bar);

    // --- Pitch A ---
    auto *pitch_a_ctrl = new LambdaScaleNoteMenuItem<int8_t>(
        "Pitch A",
        [=](int8_t v) -> void { state->pitch_a = constrain(v, (int8_t)0, (int8_t)127); },
        [=]() -> int8_t { return state->pitch_a; },
        nullptr,
        (int8_t)0,
        (int8_t)127,
        true,
        true
    );
    root->add(pitch_a_ctrl);

    // --- Pitch B ---
    auto *pitch_b_ctrl = new LambdaScaleNoteMenuItem<int8_t>(
        "Pitch B",
        [=](int8_t v) -> void { state->pitch_b = constrain(v, (int8_t)0, (int8_t)127); },
        [=]() -> int8_t { return state->pitch_b; },
        nullptr,
        (int8_t)0,
        (int8_t)127,
        true,
        true
    );
    root->add(pitch_b_ctrl);

    // --- Pitch action bar ---
    SubMenuItemBar *pitch_bar = new SubMenuItemBar("Pitch Actions", true, false);
    pitch_bar->add(new LambdaActionItem("Send A",
        [=]() -> void { state->active_is_b = false; state->send_note(state->pitch_a); }
    ));
    pitch_bar->add(new LambdaActionItem("Toggle A/B",
        [=]() -> void { state->toggle_pitches(); }
    ));
    pitch_bar->add(new LambdaActionItem("Send B",
        [=]() -> void { state->active_is_b = true; state->send_note(state->pitch_b); }
    ));
    root->add(pitch_bar);

    // --- Octave cycling ---
    SubMenuItemBar *oct_bar = new SubMenuItemBar("Octaves", true, false);
    oct_bar->add(new LambdaActionItem("Next Oct",
        [=]() -> void { state->next_octave(); }
    ));
    // Read-only display of current octave note
    auto *oct_display = new LambdaNumberControl<int8_t>(
        "Oct Note",
        [=](int8_t) -> void {},
        [=]() -> int8_t { return state->current_octave_note(); },
        nullptr,
        (int8_t)0,
        (int8_t)120,
        false,
        false
    );
    oct_display->setReadOnly(true);
    oct_display->selectable = false;
    oct_bar->add(oct_display);
    root->add(oct_bar);

    return root;
}

#endif // ENABLE_SCREEN
