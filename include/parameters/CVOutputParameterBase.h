#pragma once

// Lightweight abstract interface for CV output parameters.
// Kept in its own header so that OscillatorTuningControls.h can include it
// without pulling in the full CVOutputParameter.h template chain, which would
// create a circular include dependency through ParameterInputMenuItems.h.

// Per-channel voltage range and inversion for a CV output device.
// Defined here (not in cv_output.h) so the parameters library owns the type.
struct cv_channel_range_t {
    float floor    = 0.0f;
    float ceil     = 10.0f;
    bool  inverted = false;  // set true for hardware where lower DAC => higher voltage
};

// Convenience initializers for use in platformio.ini CONFIG_CV_OUTPUT macros.
// Mix per-channel freely, e.g. {{0,10,false},{-5,5,false},{0,10,true},{-5,5,true}}
#define CV_ALL_UNIPOLAR     {{0.0f,10.0f,false},{0.0f,10.0f,false},{0.0f,10.0f,false},{0.0f,10.0f,false}}
#define CV_ALL_BIPOLAR      {{-5.0f,5.0f,false},{-5.0f,5.0f,false},{-5.0f,5.0f,false},{-5.0f,5.0f,false}}
#define CV_ALL_UNIPOLAR_INV {{0.0f,10.0f,true},{0.0f,10.0f,true},{0.0f,10.0f,true},{0.0f,10.0f,true}}
#define CV_ALL_BIPOLAR_INV  {{-5.0f,5.0f,true},{-5.0f,5.0f,true},{-5.0f,5.0f,true},{-5.0f,5.0f,true}}

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
