#pragma once

#ifdef ENABLE_SCREEN

#include "voltage_sources/ADSVoltageSource.h"
#include "submenuitem.h"

// Forward declaration — full definition is in ParameterManager.h
// (Not used directly; save callback is passed as a function pointer instead)
#include "submenuitem_bar.h"
#include "menuitems_lambda.h"

// Maximum number of calibration steps supported by the wizard.
// Covers e.g. -5V..+5V @ 0.5V step = 21 steps + 1 spare
#define CV_INPUT_CALIB_MAX_STEPS 22

// Wizard state for walking through a series of known voltages and recording
// the voltage-source's raw measurement at each step.  After collecting samples,
// compute() fits a linear model and the user can apply / revert / save.
class CVInputCalibWizardState {
public:
    // ---- configuration (edit before calling begin()) --------------------
    float start_voltage = 0.0f;
    float end_voltage   = 10.0f;
    float step_size     = 1.0f;

    // ---- runtime state --------------------------------------------------
    ADSVoltageSourceBase *voltage_source = nullptr;
    int8_t step = -1;  // -1 = not started

    float   samples[CV_INPUT_CALIB_MAX_STEPS]         = {};
    float   targets_at_record[CV_INPUT_CALIB_MAX_STEPS] = {};
    bool    is_skipped[CV_INPUT_CALIB_MAX_STEPS]       = {};

    // Number of fetch_calibration_sample() calls to average per recorded step.
    // Higher values reduce noise at the cost of a slightly longer blocking read.
    uint8_t samples_per_step = 8;

    float new_cv1 = 1.0f;
    float new_cv2 = 0.0f;
    bool  results_ready         = false;
    bool  error_too_few_points  = false;
    float max_error_old = 0.0f;  // max |err| across calibration points using old cv1/cv2
    float max_error_new = 0.0f;  // max |err| across calibration points using new cv1/cv2

    float backup_cv1  = 1.0f;
    float backup_cv2  = 0.0f;
    bool  has_backup  = false;

    explicit CVInputCalibWizardState(ADSVoltageSourceBase *src)
        : voltage_source(src)
    {
        if (src != nullptr) {
            start_voltage = src->get_default_calib_start();
            end_voltage   = src->get_default_calib_end();
            step_size     = src->get_default_calib_step();
        }
    }

    // ---- configuration helpers -----------------------------------------

    int8_t num_steps() const {
        if (step_size <= 0.0f) return 1;
        int n = (int)round((end_voltage - start_voltage) / step_size) + 1;
        if (n < 1)  n = 1;
        if (n > CV_INPUT_CALIB_MAX_STEPS) n = CV_INPUT_CALIB_MAX_STEPS;
        return (int8_t)n;
    }

    float current_target_voltage() const {
        return start_voltage + step * step_size;
    }

    bool is_running() const { return step >= 0 && step < num_steps(); }
    bool is_complete() const { return step >= num_steps(); }

    // ---- wizard actions ------------------------------------------------

    // Reset to pre-start state, preserving configuration (start/end/step/samples_per_step).
    void abort() {
        step = -1;
        results_ready        = false;
        error_too_few_points = false;
        for (int i = 0; i < CV_INPUT_CALIB_MAX_STEPS; i++) {
            samples[i]          = 0.0f;
            targets_at_record[i] = 0.0f;
            is_skipped[i]       = false;
        }
    }

    void begin() {
        step = 0;
        results_ready        = false;
        error_too_few_points = false;
        for (int i = 0; i < CV_INPUT_CALIB_MAX_STEPS; i++) {
            samples[i]          = 0.0f;
            targets_at_record[i] = 0.0f;
            is_skipped[i]       = false;
        }
    }

    void record_reading() {
        if (!is_running() || voltage_source == nullptr) return;
        // Average multiple ADC readings to reduce noise.
        float accum = 0.0f;
        const uint8_t n_samp = (samples_per_step > 0) ? samples_per_step : 1;
        for (uint8_t i = 0; i < n_samp; i++)
            accum += voltage_source->fetch_calibration_sample();
        samples[step]          = accum / (float)n_samp;
        targets_at_record[step] = current_target_voltage();
        is_skipped[step]       = false;
        step++;
        if (is_complete()) compute();
    }

    void skip_step() {
        if (!is_running()) return;
        targets_at_record[step] = current_target_voltage();
        is_skipped[step]        = true;
        step++;
        if (is_complete()) compute();
    }

    void undo_last() {
        if (step <= 0) return;
        step--;
        samples[step]           = 0.0f;
        targets_at_record[step] = 0.0f;
        is_skipped[step]        = false;
        results_ready        = false;
        error_too_few_points = false;
    }

    void undo_all() {
        begin();
    }

    void compute() {
        results_ready        = false;
        error_too_few_points = false;

        // Collect valid (non-skipped) points
        float x[CV_INPUT_CALIB_MAX_STEPS];
        float y[CV_INPUT_CALIB_MAX_STEPS];
        int n = 0;
        int8_t ns = num_steps();
        for (int i = 0; i < ns; i++) {
            if (!is_skipped[i]) {
                x[n] = targets_at_record[i];
                y[n] = samples[i];
                n++;
            }
        }

        if (n < 2) {
            error_too_few_points = true;
            return;
        }

        if (voltage_source == nullptr ||
            !voltage_source->compute_calibration(n, x, y, &new_cv1, &new_cv2)) {
            error_too_few_points = true;
            return;
        }

        results_ready = true;

        // Compute max absolute residual for both old and new calibration.
        // (correction_value_1/2 still hold the *old* values here — apply() hasn't been called yet.)
        max_error_old = 0.0f;
        max_error_new = 0.0f;
        if (voltage_source != nullptr) {
            const float cv1_old = voltage_source->correction_value_1;
            const float cv2_old = voltage_source->correction_value_2;
            for (int i = 0; i < n; i++) {
                const float pred_old = voltage_source->compute_voltage_from_raw_sample(y[i], cv1_old, cv2_old);
                const float pred_new = voltage_source->compute_voltage_from_raw_sample(y[i], new_cv1, new_cv2);
                const float err_old = fabsf(pred_old - x[i]);
                const float err_new = fabsf(pred_new - x[i]);
                if (err_old > max_error_old) max_error_old = err_old;
                if (err_new > max_error_new) max_error_new = err_new;
            }
        }
    }

    // ---- apply / revert / save -----------------------------------------

    void apply() {
        if (!results_ready || voltage_source == nullptr) return;
        if (!has_backup) {
            backup_cv1 = voltage_source->correction_value_1;
            backup_cv2 = voltage_source->correction_value_2;
            has_backup = true;
        }
        voltage_source->correction_value_1 = new_cv1;
        voltage_source->correction_value_2 = new_cv2;
    }

    void revert() {
        if (!has_backup || voltage_source == nullptr) return;
        voltage_source->correction_value_1 = backup_cv1;
        voltage_source->correction_value_2 = backup_cv2;
    }

    void save(bool (*save_callback)()) {
        if (save_callback != nullptr)
            save_callback();
    }

    // ---- display helpers -----------------------------------------------

    const char *get_instruction() {
        static char buf[40];
        if (step < 0) {
            return "Press Start";
        }
        if (is_complete()) {
            if (error_too_few_points)  return "ERR: need >=2 pts";
            if (results_ready)         return "Apply to use new cal";
            return "Calculating...";
        }
        snprintf(buf, sizeof(buf), "Apply %.2fV (#%d/%d)",
                 current_target_voltage(), (int)(step + 1), (int)num_steps());
        return buf;
    }

    // Number of ADC reads to average for the live display.  Higher values reduce
    // quantization jitter (e.g. when the input sits on a code boundary) at the cost
    // of a slightly longer blocking read during display.  Default 4.
    uint8_t live_display_averages = 4;

    float get_live_voltage() {
        if (voltage_source == nullptr) return 0.0f;
        // Average multiple reads to damp quantization jitter when the input sits on
        // an ADC code boundary (e.g. 0 V with ADS24v alternates ~±11 mV per count).
        // Each update() triggers one fresh I2C read; averaging N reads gives N-fold
        // noise reduction at a cost of N × one-read latency.
        float sum = 0.0f;
        const uint8_t n = (live_display_averages > 0) ? live_display_averages : 1;
        for (uint8_t i = 0; i < n; i++) {
            voltage_source->update();
            sum += voltage_source->get_voltage();
        }
        return sum / (float)n;
    }

    // Predict what the live voltage would read if new_cv1/new_cv2 were applied.
    // Uses the voltage source's own inverse/forward virtual methods so it works
    // correctly for both ADSVoltageSource (linear) and ADS24vVoltageSource (Pimoroni).
    float get_predicted_voltage() {
        if (!results_ready || voltage_source == nullptr) return 0.0f;
        const float raw = voltage_source->compute_raw_from_voltage(
            voltage_source->get_voltage(),
            voltage_source->correction_value_1,
            voltage_source->correction_value_2);
        return voltage_source->compute_voltage_from_raw_sample(raw, new_cv1, new_cv2);
    }

    // Dump all calibration session data to Serial in CSV format (human- and machine-readable).
    // The header block (lines beginning with #) gives human context;
    // the CSV rows can be imported directly into a spreadsheet.
    void dump_to_serial() {
        Serial.println("# === CV Input Calibration Dump ===");
        if (voltage_source != nullptr) {
            Serial.printf("# Source: slot=%d  channel=%d  cur_cv1=%.6f  cur_cv2=%.6f\n",
                voltage_source->global_slot,
                (int)voltage_source->get_adc_channel(),
                voltage_source->correction_value_1,
                voltage_source->correction_value_2);
        }
        if (has_backup) {
            Serial.printf("# Backup (pre-Apply): cv1=%.6f  cv2=%.6f\n", backup_cv1, backup_cv2);
        }
        Serial.printf("# Config: start=%.4f  end=%.4f  step=%.4f  samples_per_step=%d\n",
            start_voltage, end_voltage, step_size, (int)samples_per_step);
        const int8_t ns = num_steps();
        const int steps_done = (step < 0) ? 0 : (step < ns ? (int)step : (int)ns);
        int n_valid = 0;
        for (int i = 0; i < steps_done; i++)
            if (!is_skipped[i]) n_valid++;
        Serial.printf("# Steps: total=%d  recorded=%d  valid=%d\n", (int)ns, steps_done, n_valid);
        if (results_ready) {
            Serial.printf("# New fit: cv1=%.6f  cv2=%.6f\n", new_cv1, new_cv2);
            Serial.printf("# Max errors: old=%.6fV (%.2fmV)  new=%.6fV (%.2fmV)\n",
                max_error_old, max_error_old * 1000.0f,
                max_error_new, max_error_new * 1000.0f);
        } else {
            Serial.println("# New fit: not ready");
        }
        // Use backup cv1/cv2 for the "before" column if Apply was already pressed,
        // so the old vs new comparison is always meaningful.
        const float ref_cv1 = has_backup ? backup_cv1 : (voltage_source ? voltage_source->correction_value_1 : 1.0f);
        const float ref_cv2 = has_backup ? backup_cv2 : (voltage_source ? voltage_source->correction_value_2 : 0.0f);
        Serial.println("# Columns: step,target_V,raw_sample,skipped,pred_old_V,err_old_V,pred_new_V,err_new_V");
        Serial.println("step,target_V,raw_sample,skipped,pred_old_V,err_old_V,pred_new_V,err_new_V");
        for (int i = 0; i < steps_done; i++) {
            if (is_skipped[i]) {
                Serial.printf("%d,%.4f,,1,,,,\n", i, targets_at_record[i]);
            } else {
                float pred_old = 0.0f, pred_new = 0.0f;
                if (voltage_source != nullptr) {
                    pred_old = voltage_source->compute_voltage_from_raw_sample(samples[i], ref_cv1, ref_cv2);
                    if (results_ready)
                        pred_new = voltage_source->compute_voltage_from_raw_sample(samples[i], new_cv1, new_cv2);
                }
                const float err_old = pred_old - targets_at_record[i];
                if (results_ready) {
                    const float err_new = pred_new - targets_at_record[i];
                    Serial.printf("%d,%.4f,%.6f,0,%.6f,%+.6f,%.6f,%+.6f\n",
                        i, targets_at_record[i], samples[i],
                        pred_old, err_old, pred_new, err_new);
                } else {
                    Serial.printf("%d,%.4f,%.6f,0,%.6f,%+.6f,,\n",
                        i, targets_at_record[i], samples[i], pred_old, err_old);
                }
            }
        }
        Serial.println("# === End Calibration Dump ===");
    }
};


// Build a SubMenuItem wizard for a single ADSVoltageSource calibration.
FLASHMEM
inline SubMenuItem *makeCVInputCalibrationSubMenu(
    CVInputCalibWizardState *state,
    bool (*save_callback)(),
    const char *label = "CV Input Cal")
{
    SubMenuItem *root = new SubMenuItem(label, true, false);

    // ---- Configuration row ---------------------------------------------
    SubMenuItemBar *cfg_bar = new SubMenuItemBar("Range Config", true, false);

    cfg_bar->add(new LambdaNumberControl<float>(
        "V Start",
        [=](float v) -> void { if (!state->is_running()) state->start_voltage = v; },
        [=]() -> float { return state->start_voltage; },
        nullptr, -15.0f, 15.0f, true, false
    ));
    cfg_bar->add(new LambdaNumberControl<float>(
        "V End",
        [=](float v) -> void { if (!state->is_running()) state->end_voltage = v; },
        [=]() -> float { return state->end_voltage; },
        nullptr, -15.0f, 15.0f, true, false
    ));
    cfg_bar->add(new LambdaNumberControl<float>(
        "V Step",
        [=](float v) -> void { if (!state->is_running()) state->step_size = max(0.05f, v); },
        [=]() -> float { return state->step_size; },
        nullptr, 0.05f, 5.0f, true, false
    ));
    cfg_bar->add(new LambdaNumberControl<int8_t>(
        "Samples",
        [=](int8_t v) -> void { if (!state->is_running()) state->samples_per_step = (uint8_t)max(1, (int)v); },
        [=]() -> int8_t { return (int8_t)state->samples_per_step; },
        nullptr, 1, 32, true, false
    ));
    root->add(cfg_bar);

    // ---- Source info (confirms which slot/channel is being calibrated) -
    root->add(new CallbackMenuItem(
        "Source",
        [=]() -> const char* {
            static char buf[40];
            if (state->voltage_source == nullptr) return "Source: (none)";
            snprintf(buf, sizeof(buf), "Slot %d  Ch %d  cv1:%.5f cv2:%.5f",
                (int)state->voltage_source->global_slot,
                (int)state->voltage_source->get_adc_channel(),
                state->voltage_source->correction_value_1,
                state->voltage_source->correction_value_2
            );
            return buf;
        },
        false
    ));

    // ---- Start / Abort controls ----------------------------------------
    {
        SubMenuItemBar *ctrl_bar = new SubMenuItemBar("Control", false, false);
        {
            auto *btn = new LambdaActionItem("Start", [=]() -> void {
                state->begin();
                menu_set_last_message(state->get_instruction(), GREEN);
            }, true);
            ctrl_bar->add(btn);
        }
        {
            auto *btn = new LambdaActionConfirmItem("Abort", [=]() -> void {
                state->abort();
                menu_set_last_message("Aborted", BLUE);
            }, true);
            ctrl_bar->add(btn);
        }
        root->add(ctrl_bar);
    }

    // ---- Persistent instruction display (updates every frame) ----------
    root->add(new CallbackMenuItem(
        "Instr",
        [=]() -> const char* { return state->get_instruction(); },
        [=]() -> uint16_t {
            if (state->is_complete())
                return state->error_too_few_points ? RED : GREEN;
            return state->is_running() ? YELLOW : C_WHITE;
        },
        false
    ));

    // ---- Live voltage readouts (both update every frame via CallbackMenuItem) ----
    // "Old" = current calibration;  "New" = predicted with new cv1/cv2 once ready.
    root->add(new CallbackMenuItem(
        "Old V",
        [=]() -> const char* {
            static char buf[24];
            snprintf(buf, sizeof(buf), "%s: %+.4fV",
                     state->results_ready ? "Old" : "Live",
                     state->get_live_voltage());
            return buf;
        },
        false
    ));
    root->add(new CallbackMenuItem(
        "New V",
        [=]() -> const char* {
            static char buf[24];
            if (!state->results_ready) return "New: (pending)";
            snprintf(buf, sizeof(buf), "New: %+.4fV", state->get_predicted_voltage());
            return buf;
        },
        false
    ));

    // ---- Step controls bar ---------------------------------------------
    SubMenuItemBar *step_bar = new SubMenuItemBar("Step", false, true);
    {
        auto *btn = new LambdaActionItem("Record", [=]() -> void {
            state->record_reading();
            menu_set_last_message(state->get_instruction(), GREEN);
        }, true);
        step_bar->add(btn);
    }
    {
        auto *btn = new LambdaActionItem("Skip", [=]() -> void {
            state->skip_step();
            menu_set_last_message(state->get_instruction(), YELLOW);
        }, true);
        step_bar->add(btn);
    }
    step_bar->add(new LambdaActionItem("Undo Last",
        [=]() -> void { state->undo_last(); }
    ));
    step_bar->add(new LambdaActionConfirmItem("Undo All",
        [=]() -> void { state->undo_all(); }
    ));
    root->add(step_bar);

    // ---- Result preview ------------------------------------------------
    // Use CallbackMenuItems added directly to root (not inside a SubMenuItemBar)
    // to avoid the crash caused by a SubMenuItemBar whose children all have
    // selectable=false (currently_selected walks out of bounds on open).
    root->add(new CallbackMenuItem(
        "New cv1",
        [=]() -> const char* {
            static char buf[24];
            if (!state->results_ready) return "cv1: (pending)";
            snprintf(buf, sizeof(buf), "cv1: %.5f", state->new_cv1);
            return buf;
        },
        false
    ));
    root->add(new CallbackMenuItem(
        "New cv2",
        [=]() -> const char* {
            static char buf[24];
            if (!state->results_ready) return "cv2: (pending)";
            snprintf(buf, sizeof(buf), "cv2: %.5f", state->new_cv2);
            return buf;
        },
        false
    ));
    root->add(new CallbackMenuItem(
        "MaxErr",
        [=]() -> const char* {
            static char buf[36];
            if (!state->results_ready) return "MaxErr: (pending)";
            snprintf(buf, sizeof(buf), "Old:%.4fV  New:%.4fV",
                state->max_error_old, state->max_error_new);
            return buf;
        },
        [=]() -> uint16_t {
            if (!state->results_ready) return C_WHITE;
            // Red if new fit is worse than 5 mV, yellow 1-5 mV, green < 1 mV
            if (state->max_error_new > 0.005f) return RED;
            if (state->max_error_new > 0.001f) return YELLOW;
            return GREEN;
        },
        false
    ));

    // ---- Apply / Revert / Save -----------------------------------------
    SubMenuItemBar *persist_bar = new SubMenuItemBar("Calibration values", false, false);
    persist_bar->add(new LambdaActionConfirmItem("Apply",
        [=]() -> void { state->apply(); }
    ));
    persist_bar->add(new LambdaActionConfirmItem("Revert",
        [=]() -> void { state->revert(); }
    ));
    persist_bar->add(new LambdaActionConfirmItem("Save",
        [=]() -> void { state->save(save_callback); }
    ));
    persist_bar->add(new LambdaActionItem("Dump",
        [=]() -> void { state->dump_to_serial(); }
    ));
    root->add(persist_bar);

    return root;
}

#endif // ENABLE_SCREEN
