#pragma once

// Lightweight abstract interface for CV output parameters.
// Kept in its own header so that OscillatorTuningControls.h can include it
// without pulling in the full CVOutputParameter.h template chain, which would
// create a circular include dependency through ParameterInputMenuItems.h.

class CVOutputParameterBase {
public:
    // Send a note to this CV output (NoteOff previous, NoteOn new pitch).
    virtual void tuning_send_note(uint8_t pitch) = 0;
    // Send NoteOff for the currently held note (if any).
    virtual void tuning_note_off() = 0;
    // Set output voltage to the floor (0 V for unipolar).
    virtual void tuning_zero_output() = 0;
    // Disconnect all modulation slot connections.
    virtual void tuning_disconnect_modulation() = 0;
    // Return the human-readable label for this output.
    virtual const char *get_cv_label() const = 0;
    virtual ~CVOutputParameterBase() = default;
};
