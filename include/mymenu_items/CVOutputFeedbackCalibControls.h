#pragma once

#ifdef ENABLE_SCREEN

// CVOutputParameter is included by the parent (CVOutputParameter.h) before this file
#include "submenuitem.h"
#include "submenuitem_bar.h"
#include "menuitems_lambda.h"

// State machine for user-guided (ears/tuner) CV output calibration.
//
// Workflow:
//   1. Press "Cal 0V"  → enter BINARY_SEARCH for the low endpoint
//   2. Listen to oscillator / watch voltmeter, press Too High / Too Low to narrow
//   3. Press Accept → commit, enter FINE_TUNE for 0V endpoint
//   4. Use Fine +/- for small adjustments
//   5. Repeat with "Cal 10V" for the high endpoint
//   6. Press Revert at any time to undo back to original values
//   7. Press Save when happy

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

    // backup taken when first leaving IDLE so we can always revert
    bool     has_backup      = false;
    uint16_t backup_lowest   = 0;
    uint16_t backup_highest  = 65535;

    explicit CVOutputFeedbackCalibState(CVOutputParameter<DACClass, DataType> *out)
        : output(out) {}

    void snapshot_backup() {
        if (!has_backup) {
            backup_lowest  = output->calibrated_lowest_value;
            backup_highest = output->calibrated_highest_value;
            has_backup = true;
        }
    }

    void output_test_value() {
        if (output == nullptr || output->target == nullptr) return;
        output->target->write(output->dac_channel, current_test_value);
    }

    // ---- start calibrating an endpoint ---------------------------------

    void start_calibrate_low() {
        if (output == nullptr) return;
        snapshot_backup();
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
        current_test_value = (uint16_t)(((uint32_t)lo_bracket + hi_bracket) / 2);
        output_test_value();
    }

    void too_low() {
        if (phase != BINARY_SEARCH) return;
        lo_bracket         = current_test_value;
        current_test_value = (uint16_t)(((uint32_t)lo_bracket + hi_bracket) / 2);
        output_test_value();
    }

    void accept_binary() {
        if (phase != BINARY_SEARCH) return;
        if (calibrating_high)
            output->calibrated_highest_value = current_test_value;
        else
            output->calibrated_lowest_value  = current_test_value;
        phase = FINE_TUNE;
    }

    // ---- fine-tune controls (FINE_TUNE phase) ---------------------------

    void nudge_up() {
        if (phase != FINE_TUNE) return;
        if (calibrating_high) {
            output->calibrated_highest_value++;
        } else {
            output->calibrated_lowest_value++;
        }
        // re-output current data value through normal calibrated path
        output->setTargetValueFromData(output->getCurrentDataValue(), true);
        output->process_pending();
    }

    void nudge_down() {
        if (phase != FINE_TUNE) return;
        if (calibrating_high) {
            if (output->calibrated_highest_value > 0) output->calibrated_highest_value--;
        } else {
            if (output->calibrated_lowest_value > 0) output->calibrated_lowest_value--;
        }
        output->setTargetValueFromData(output->getCurrentDataValue(), true);
        output->process_pending();
    }

    // ---- revert & save --------------------------------------------------

    void revert() {
        if (!has_backup || output == nullptr) return;
        output->calibrated_lowest_value  = backup_lowest;
        output->calibrated_highest_value = backup_highest;
        output->setTargetValueFromData(output->getCurrentDataValue(), true);
        output->process_pending();
        phase = IDLE;
    }

    void save() {
        if (output == nullptr) return;
        output->save_calibration();
    }

    // ---- query helpers --------------------------------------------------

    uint16_t get_active_calibrated_value() const {
        if (output == nullptr) return 0;
        return calibrating_high ? output->calibrated_highest_value
                                : output->calibrated_lowest_value;
    }
};


// Build a SubMenuItem "Feedback Calib" for a given CVOutputFeedbackCalibState.
// This is called from inside CVOutputParameter::makeCalibrationControls().
template<class DACClass = DAC8574, class DataType = float>
FLASHMEM
inline SubMenuItem *makeFeedbackCalibrationControls(
    CVOutputFeedbackCalibState<DACClass, DataType> *state)
{
    SubMenuItem *bar = new SubMenuItem("Feedback Calib", true, false);

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
    val_display->selectable = false;
    bar->add(val_display);

    // Start calibration buttons
    SubMenuItemBar *start_bar = new SubMenuItemBar("Start", true, false);
    start_bar->add(new LambdaActionConfirmItem("Cal 0V",
        [=]() -> void { state->start_calibrate_low(); }
    ));
    start_bar->add(new LambdaActionConfirmItem("Cal 10V",
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
