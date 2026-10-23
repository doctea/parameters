#pragma once

#ifdef ENABLE_SCREEN

// CVOutputParameter is included by the parent (CVOutputParameter.h) before this file
#include "submenuitem.h"
#include "submenuitem_bar.h"
#include "menuitems_lambda.h"

// State machine for user-guided (ears/tuner) CV output calibration.
//
// Workflow:
//   1. Press cal_low_label (e.g. "Cal 0V") → starts binary search for the low endpoint.
//      Output begins at DAC midpoint (~5V). Hardware inversion is handled automatically.
//   2. Watch voltmeter / listen to oscillator.
//      Press "Too High" / "Too Low" to halve the search range each time.
//   3. Press "Accept" → commits calibrated_lowest_value, enters FINE_TUNE.
//      Output immediately shows the calibrated endpoint voltage.
//   4. Use "Fine +" / "Fine -" for single-step adjustments; each press re-outputs the endpoint.
//   5. Repeat with cal_high_label (e.g. "Cal 10V") for the high endpoint.
//   6. Press "Save" to persist. Press "Revert" at any time to restore original values.
//
//   Note: modulation and external MIDI to this output are blocked while calibration is active.

template<class DACClass = DAC8574, class DataType = float>
class CVOutputFeedbackCalibState {
public:
    enum Phase { IDLE, BINARY_SEARCH, FINE_TUNE };

    CVOutputParameter<DACClass, DataType> *output = nullptr;
    Phase  phase          = IDLE;
    bool   calibrating_high = false;

    // binary search brackets (DAC raw values, 0–65535)
    uint16_t lo_bracket      = 0;
    uint16_t hi_bracket      = 65535;
    uint16_t current_test_value = 32768;
    uint16_t fine_step = 1;

    // backup taken when first leaving IDLE so we can always revert
    bool     has_backup      = false;
    uint16_t backup_lowest   = 0;
    uint16_t backup_highest  = 65535;

    // current voltage set by the range-check control ("Check V")
    float    check_voltage   = 0.0f;

    // Labels use physical voltage limits (e.g. "Cal 0V"/"Cal 10V" or "Cal -5V"/"Cal +5V").
    char cal_low_label[12]  = "Cal Low";
    char cal_high_label[12] = "Cal High";

    explicit CVOutputFeedbackCalibState(CVOutputParameter<DACClass, DataType> *out)
        : output(out) {
        if (out != nullptr) {
            snprintf(cal_low_label,  sizeof(cal_low_label),  "Cal %.0fV", (double)out->minimumDataLimit);
            snprintf(cal_high_label, sizeof(cal_high_label), "Cal %.0fV", (double)out->maximumDataLimit);
        }
    }

    void snapshot_backup() {
        if (!has_backup) {
            backup_lowest  = output->calibrated_lowest_value;
            backup_highest = output->calibrated_highest_value;
            has_backup = true;
        }
    }

    void output_test_value() {
        if (output == nullptr || output->target == nullptr) return;
        // current_test_value lives in "uninverted DAC space".
        // Apply hardware inversion here so the binary search brackets work correctly
        // for both normal and inverted hardware: "Too High" always means "voltage too high".
        const uint16_t raw = output->inverted
            ? (uint16_t)((uint32_t)__UINT16_MAX__ - current_test_value)
            : current_test_value;
        output->write_calibration_dac_value(raw);
    }

    // During FINE_TUNE: output the floor or ceiling voltage through the fully calibrated
    // (+ inversion) path so the user sees the result of each nudge immediately.
    void output_calibrated_endpoint() {
        if (output == nullptr || output->target == nullptr) return;
        const uint16_t value = get_active_calibrated_value();
        const uint16_t raw = output->inverted ? (uint16_t)(65535 - value) : value;
        output->write_calibration_dac_value(raw);
    }

    // ---- start calibrating an endpoint ---------------------------------

    void start_calibrate_low() {
        if (output == nullptr) return;
        snapshot_backup();
        output->set_output_locked(true);
        calibrating_high   = false;
        lo_bracket         = 0;
        hi_bracket         = 65535;
        current_test_value = 32768;
        phase              = BINARY_SEARCH;
        output_test_value();
    }

    void start_calibrate_high() {
        if (output == nullptr) return;
        snapshot_backup();
        output->set_output_locked(true);
        calibrating_high   = true;
        lo_bracket         = 0;
        hi_bracket         = 65535;
        current_test_value = 32768;
        phase              = BINARY_SEARCH;
        output_test_value();
    }

    // ---- binary search controls ----------------------------------------

    void too_high() {
        if (phase != BINARY_SEARCH) return;
        hi_bracket         = current_test_value;
        if (lo_bracket >= current_test_value && current_test_value > 0)
            lo_bracket = current_test_value - 1;
        current_test_value = (uint16_t)(((uint32_t)lo_bracket + hi_bracket) / 2);
        output_test_value();
    }

    void too_low() {
        if (phase != BINARY_SEARCH) return;
        lo_bracket         = current_test_value;
        if (hi_bracket <= current_test_value && current_test_value < 65535)
            hi_bracket = current_test_value + 1;
        current_test_value = (uint16_t)(((uint32_t)lo_bracket + hi_bracket + 1) / 2);
        output_test_value();
    }

    void accept_binary() {
        if (phase != BINARY_SEARCH) return;
        if (calibrating_high)
            output->calibrated_highest_value = current_test_value;
        else
            output->calibrated_lowest_value  = current_test_value;
        phase = FINE_TUNE;
        // Immediately output the endpoint voltage so the user can verify the result
        // and start fine-tuning with visual/audio feedback.
        output_calibrated_endpoint();
    }

    // ---- fine-tune controls (FINE_TUNE phase) ---------------------------

    void begin_fine_tune() {
        if (phase == IDLE && output != nullptr) {
            snapshot_backup();
            output->set_output_locked(true);
            phase = FINE_TUNE;
        } else if (phase == BINARY_SEARCH) {
            accept_binary();
        }
    }

    void nudge_up() {
        begin_fine_tune();
        if (phase != FINE_TUNE) return;
        const uint16_t value = (uint16_t)min((uint32_t)65535,
            (uint32_t)get_active_calibrated_value() + fine_step);
        if (calibrating_high) {
            output->calibrated_highest_value = value;
        } else {
            output->calibrated_lowest_value = value;
        }
        output_calibrated_endpoint();
    }

    void nudge_down() {
        begin_fine_tune();
        if (phase != FINE_TUNE) return;
        const uint16_t current = get_active_calibrated_value();
        const uint16_t value = current > fine_step ? current - fine_step : 0;
        if (calibrating_high) {
            output->calibrated_highest_value = value;
        } else {
            output->calibrated_lowest_value = value;
        }
        output_calibrated_endpoint();
    }

    // ---- revert & save --------------------------------------------------

    // Restore normal operation: clear calibration lock, re-output current value, return to IDLE.
    void finish() {
        if (output == nullptr) return;
        output->set_output_locked(false);
        phase = IDLE;
        has_backup = false;
    }

    void revert() {
        if (!has_backup || output == nullptr) return;
        output->calibrated_lowest_value  = backup_lowest;
        output->calibrated_highest_value = backup_highest;
        finish();
    }

    void save() {
        if (output == nullptr) return;
        output->save_calibration();
        finish();
    }

    // ---- query helpers --------------------------------------------------

    uint16_t get_active_calibrated_value() const {
        if (output == nullptr) return 0;
        return calibrating_high ? output->calibrated_highest_value
                                : output->calibrated_lowest_value;
    }

    // Returns a short instruction string for the current phase (shown live on screen).
    const char *get_status() const {
        static char buf[44];
        switch (phase) {
            case IDLE:
                snprintf(buf, sizeof(buf), "Press %s or %s to start",
                         cal_low_label, cal_high_label);
                return buf;
            case BINARY_SEARCH:
                if (current_test_value == 0 || current_test_value == 65535) {
                    snprintf(buf, sizeof(buf), "DAC limit: reverse, Accept or fine-tune");
                    return buf;
                }
                snprintf(buf, sizeof(buf), "%s search: High/Low, then Accept",
                         calibrating_high ? cal_high_label : cal_low_label);
                return buf;
            case FINE_TUNE:
                snprintf(buf, sizeof(buf), "%s fine-tune: +/- then %s",
                         calibrating_high ? cal_high_label : cal_low_label,
                         calibrating_high ? "Save" : cal_high_label);
                return buf;
        }
        return "";
    }

    ~CVOutputFeedbackCalibState() {
        // Safety net: clear calibration lock if the menu is destroyed without Save/Revert.
        if (output != nullptr) output->set_output_locked(false);
    }

    // ---- range-check / verification ----------------------------------------

    // Output a specific voltage directly so the user can verify calibration
    // across the full range. Only active in IDLE phase (not during binary search
    // or fine-tune) to avoid interfering with an active calibration session.
    void output_check_voltage(float v) {
        if (output == nullptr || output->target == nullptr) return;
        if (phase != IDLE) return;
        check_voltage = v;
        output->set_calibration_voltage(v);
    }
};


// Build a SubMenuItem "Feedback Calib" for a given CVOutputFeedbackCalibState.
// This is called from inside CVOutputParameter::makeCalibrationControls().
template<class DACClass = DAC8574, class DataType = float>
FLASHMEM
inline SubMenuItem *makeFeedbackCalibrationControls(
    CVOutputFeedbackCalibState<DACClass, DataType> *state)
{
    SubMenuItem *bar = new SubMenuItem("Simple", true, false);

    // Live status line: phase-coloured instruction that updates every frame
    bar->add(new CallbackMenuItem(
        "Status",
        [=]() -> const char* { return state->get_status(); },
        [=]() -> uint16_t {
            switch (state->phase) {
                case CVOutputFeedbackCalibState<DACClass,DataType>::BINARY_SEARCH: return YELLOW;
                case CVOutputFeedbackCalibState<DACClass,DataType>::FINE_TUNE:     return GREEN;
                default: return C_WHITE;
            }
        },
        false  // no separate header — the text IS the content
    ));

    // Monitor bar: Lock Output | Check V | Test Val — grouped to save screen space.
    SubMenuItemBar *monitor_bar = new SubMenuItemBar("Monitor", true, false);

    // Lock toggle: prevents modulation from overwriting the output so the user can
    // step through check voltages without interference, even when not actively calibrating.
    monitor_bar->add(new LambdaToggleControl(
        "Lock Output",
        [=](bool v) -> void {
            if (state->output == nullptr) return;
            if (!v && state->phase != CVOutputFeedbackCalibState<DACClass,DataType>::IDLE) {
                menu_set_last_message("Save or Revert calibration first", ORANGE);
                return;
            }
            state->output->set_output_locked(v);
        },
        [=]() -> bool {
            return state->output != nullptr && state->output->calibration_mode;
        }
    ));

    // Check voltage: step through whole-volt values to verify calibration across the full range.
    // Works only in IDLE phase; lock the output first so modulation doesn't overwrite it.
    {
        const float check_min = state->output != nullptr ? (float)state->output->minimumDataLimit : 0.0f;
        const float check_max = state->output != nullptr ? (float)state->output->maximumDataLimit : 10.0f;
        auto *check_ctrl = new LambdaNumberControl<float>(
            "Check V",
            [=](float v) -> void { state->output_check_voltage(v); },
            [=]() -> float { return state->check_voltage; },
            nullptr,
            check_min,
            check_max,
            false,
            false
        );
        check_ctrl->step = 1.0f;
        monitor_bar->add(check_ctrl);
    }

    // Read-only: current test DAC value (during search) or committed value (after accept)
    auto *val_display = new LambdaNumberControl<uint16_t>(
        "Test Val",
        [=](uint16_t) -> void {},
        [=]() -> uint16_t {
            if (state->phase == CVOutputFeedbackCalibState<DACClass,DataType>::BINARY_SEARCH)
                return state->current_test_value;
            return state->get_active_calibrated_value();
        },
        nullptr,
        (uint16_t)0,
        (uint16_t)65535,
        false,
        false
    );
    val_display->setReadOnly(true);
    val_display->flags.selectable = false;
    monitor_bar->add(val_display);

    bar->add(monitor_bar);

    // Start calibration buttons
    SubMenuItemBar *start_bar = new SubMenuItemBar("Start", true, false);
    start_bar->add(new LambdaActionConfirmItem(state->cal_low_label,
        [=]() -> void { state->start_calibrate_low(); }
    ));
    start_bar->add(new LambdaActionConfirmItem(state->cal_high_label,
        [=]() -> void { state->start_calibrate_high(); }
    ));
    bar->add(start_bar);

    // Binary search controls
    SubMenuItemBar *search_bar = new SubMenuItemBar("Adjust", true, false);
    search_bar->add(new LambdaActionItem("Too High",
        [=]() -> void { state->too_high(); }
    ));
    search_bar->add(new LambdaActionItem("Accept",
        [=]() -> void { state->accept_binary(); }
    ));
    search_bar->add(new LambdaActionItem("Too Low",
        [=]() -> void { state->too_low(); }
    ));
    bar->add(search_bar);

    // Fine-tune controls
    SubMenuItemBar *fine_bar = new SubMenuItemBar("Fine Tune", true, false);
    fine_bar->add(new LambdaNumberControl<uint16_t>("Step",
        [=](uint16_t v) { state->fine_step = v; },
        [=]() -> uint16_t { return state->fine_step; },
        nullptr, (uint16_t)1, (uint16_t)4096, false, true
    ));
    fine_bar->add(new LambdaActionItem("Fine -",
        [=]() -> void { state->nudge_down(); }
    ));
    fine_bar->add(new LambdaActionItem("Fine +",
        [=]() -> void { state->nudge_up(); }
    ));
    bar->add(fine_bar);

    // Revert and save
    SubMenuItemBar *persist_bar = new SubMenuItemBar("Persist", true, false);
    persist_bar->add(new LambdaActionConfirmItem("Revert",
        [=]() -> void { state->revert(); }
    ));
    persist_bar->add(new LambdaActionConfirmItem("Save",
        [=]() -> void { state->save(); }
    ));
    bar->add(persist_bar);

    return bar;
}

#endif // ENABLE_SCREEN
