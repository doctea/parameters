#!/usr/bin/env python3
"""Compile production CV conversion/calibration methods with mock DAC hardware."""

from pathlib import Path
import subprocess
import tempfile


def extract_body(text, signature):
    start = text.index(signature)
    opening = text.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


def main():
    root = Path(__file__).resolve().parent
    cv = (root / "include" / "parameters" / "CVOutputParameter.h").read_text()
    parameter = (root / "include" / "parameters" / "Parameter.h").read_text()
    feedback = (root / "include" / "mymenu_items" / "CVOutputFeedbackCalibControls.h").read_text()
    calibration = (root / "src" / "parameters" / "calibration.cpp").read_text()
    calibrate_function = extract_body(
        calibration,
        "uint16_t calibrate_find_dac_value_for(DAC8574 *dac_output, int channel, VoltageParameterInput *src,",
    )
    methods = "\n".join(extract_body(cv, signature) for signature in (
        "void calibrate() override",
        "virtual DataType getCurrentDataValue() override",
        "virtual DataType map(",
        "virtual uint16_t get_calibrated_dac_value(",
        "virtual uint16_t get_dac_value_for_voltage(",
        "void write_calibration_dac_value(",
        "void set_calibration_voltage(",
        "virtual void set_output_locked(",
        "virtual void setTargetValueFromData(",
        "virtual void process_pending(",
        "virtual void sendNoteOn(",
        "virtual void sendNoteOff(",
        "void set_midi_notes_enabled(",
        "void set_park_enabled(",
        "void park_pitch(",
        "virtual float get_voltage_for_pitch(",
        "virtual void refresh_pitch_output(",
        "virtual void apply_pitch_bend_delta_to_current_output(",
        "virtual void set_channel_pitch_bend_semitones(",
        "virtual void set_omni_pitch_bend_semitones(",
    )).replace(" override", "")
    methods += "\n" + "\n".join(extract_body(parameter, signature) for signature in (
        "virtual DataType get_effective_minimum_data_value(",
        "virtual DataType get_effective_maximum_data_value(",
        "virtual DataType normalToData(",
        "virtual float dataToNormal(",
        "virtual DataType constrainDataToLimit(",
        "virtual DataType constrainDataToRange(",
        "virtual float constrainNormal(",
    ))
    state = extract_body(feedback, "template<class DACClass = DAC8574, class DataType = float>") + ";"
    source = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>
using std::min;
using std::max;
struct SerialMock {
    explicit operator bool() const { return false; }
    int printf(const char *, ...) __attribute__((format(printf, 2, 3))) { return 0; }
    void println(const char * = "") {}
} Serial;
#define Serial_printf(...) Serial.printf(__VA_ARGS__)
#define Serial_println(...) ((void)0)
constexpr int NOTE_OFF = -1;
bool is_valid_note(int note) { return note >= 0 && note <= 127; }
const char *get_note_name_c(int) { return "note"; }
unsigned micros() { return 0; }
template<class T> T constrain(T v, T lo, T hi) {
    return std::max(lo, std::min(v, hi));
}
struct DAC8574 {
    uint16_t last = 0;
    float floor = 0, ceiling = 10;
    bool inverted = false;
    unsigned writes = 0;
    int lastError() { return 0; }
    uint16_t read(uint8_t) { return last; }
    void write(uint8_t, int value) {
        assert(value >= 0 && value <= 65535);
        assert(++writes < 1000000);
        last = value;
    }
};
struct GateMock {
    bool active = false;
    void send_gate_on(int, int) { active = true; }
    void send_gate_off(int, int) { active = false; }
};
struct VoltageParameterInput {
    DAC8574 *dac;
    const char *name = "feedback";
    void read() {}
    float get_voltage() { return fetch_current_voltage(); }
    float fetch_current_voltage() {
        const float logical_raw = dac->inverted ? 65535 - dac->last : dac->last;
        return dac->floor + (logical_raw - 2000) * (dac->ceiling - dac->floor) / 61000;
    }
};
struct ParameterManagerMock {
    void update_voltage_sources() {}
    void update_inputs() {}
} manager;
ParameterManagerMock *parameter_manager = &manager;
''' + calibrate_function + r'''
template<class DACClass = DAC8574, class DataType = float>
struct CVOutputParameter {
    DataType minimumDataLimit = 0, maximumDataLimit = 10;
    DataType minimumDataRange = 0, maximumDataRange = 10;
    float minimumNormalValue = 0, maximumNormalValue = 1;
    uint16_t calibrated_lowest_value = 8, calibrated_highest_value = 65023;
    bool inverted = false, debug = false, calibration_mode = false;
    bool midi_notes_enabled = true, park_enabled = true, note_is_active = false, gate_is_on = false;
    int8_t current_pitch_note = NOTE_OFF;
    float omni_pitch_bend_semitones = 0, channel_pitch_bend_semitones = 0;
    DataType currentDataValue = 0;
    DataType &current = currentDataValue;
    float modulation = 0;
    DataType output_voltage = 0;
    bool is_pending_value = false;
    uint16_t pending_value = 0, last_value = 0;
    int gate_bank = 0, gate_number = 0;
    GateMock gate;
    GateMock *gate_manager = &gate;
    bool get_gate_output_enabled() { return true; }
    float get_pitch_bend_semitones() { return omni_pitch_bend_semitones + channel_pitch_bend_semitones; }
    const char *label = "test";
    DACClass *target = nullptr;
    uint8_t dac_channel = 0;
    VoltageParameterInput *calibration_input = nullptr;
    const char *get_cv_label() const { return label; }
    void save_calibration() {}
    float getCurrentNormalValue() { return dataToNormal(getCurrentDataValue()); }
    float get_modulation_value() { return modulation; }
    void updateValueFromData(DataType value) {
        if (getCurrentDataValue() == value) return;
        current = value;
        setTargetValueFromNormal(getCurrentNormalValue() + modulation, true);
    }
    void setTargetValueFromNormal(float normal, bool force) {
        setTargetValueFromData(normalToData(normal), force);
    }
''' + methods + "\n};\n" + state + r'''
void check_range(float floor, float ceiling, bool inverted) {
    DAC8574 dac;
    dac.floor = floor;
    dac.ceiling = ceiling;
    dac.inverted = inverted;
    VoltageParameterInput input{&dac};
    CVOutputParameter<> output;
    output.target = &dac;
    output.minimumDataLimit = output.minimumDataRange = floor;
    output.maximumDataLimit = output.maximumDataRange = ceiling;
    output.inverted = inverted;
    auto raw = [inverted](uint16_t value) -> uint16_t {
        return inverted ? 65535 - value : value;
    };
    assert(output.get_dac_value_for_voltage(floor) == raw(8));
    assert(output.get_dac_value_for_voltage(ceiling) == raw(65023));
    assert(output.get_dac_value_for_voltage((floor + ceiling) / 2) == raw(32515));
    assert(output.get_dac_value_for_voltage(floor - 100) == raw(8));
    assert(output.get_dac_value_for_voltage(ceiling + 100) == raw(65023));
    uint16_t previous = raw(8);
    for (int i = 0; i <= 1000; ++i) {
        const float source_bipolar = -1.0f + i / 500.0f;
        const float normal = 0.5f + 0.5f * source_bipolar;
        const float voltage = output.normalToData(normal);
        assert(std::fabs(voltage - (floor + (ceiling - floor) * normal)) < 0.00001f);
        assert(std::fabs(output.dataToNormal(voltage) - normal) < 0.00001f);
        const auto value = output.get_dac_value_for_voltage(voltage);
        assert(inverted ? value <= previous : value >= previous);
        const auto expected = raw(static_cast<uint16_t>(8 + 65015 * normal));
        assert(std::abs(int(value) - int(expected)) <= 1);
        previous = value;
    }
    if (floor == -5 && ceiling == 5) {
        assert(output.normalToData(0.25f) == -2.5f);
        assert(output.get_dac_value_for_voltage(-2.5f) == raw(16261));
        assert(output.get_dac_value_for_voltage(2.5f) == raw(48769));
        for (int i = 0; i < 1000; ++i) {
            const float source = std::sin(i * 6.2831853f / 1000);
            const auto value = output.get_dac_value_for_voltage(
                output.normalToData(0.5f + source * 0.5f));
            if (source < -0.01f) assert(inverted ? value > raw(32515) : value < raw(32515));
        }
    }
    // User range limits affect modulation, not physical calibration.
    output.minimumDataRange = floor + (ceiling - floor) * 0.25f;
    output.maximumDataRange = floor + (ceiling - floor) * 0.75f;
    assert(output.get_dac_value_for_voltage(floor) == raw(8));
    assert(output.get_dac_value_for_voltage(ceiling) == raw(65023));
    assert(output.get_dac_value_for_voltage(output.normalToData(0)) == raw(16261));
    assert(output.get_dac_value_for_voltage(output.normalToData(1)) == raw(48769));

    CVOutputFeedbackCalibState<> state(&output);
    char expected[12];
    std::snprintf(expected, sizeof(expected), "Cal %.0fV", double(floor));
    assert(std::strcmp(state.cal_low_label, expected) == 0);
    std::snprintf(expected, sizeof(expected), "Cal %.0fV", double(ceiling));
    assert(std::strcmp(state.cal_high_label, expected) == 0);
    state.output_calibrated_endpoint();
    assert(dac.last == raw(8));
    state.calibrating_high = true;
    state.output_calibrated_endpoint();
    assert(dac.last == raw(65023));
    state.start_calibrate_low();
    output.sendNoteOn(60, 100, 1);
    assert(output.current_pitch_note == NOTE_OFF);
    for (int i = 0; i < 20; ++i) state.too_low();
    assert(state.current_test_value == 65535);
    state.too_high();
    assert(state.current_test_value == 65534);
    state.nudge_down();  // Fine controls accept a search automatically.
    assert(state.phase == decltype(state)::FINE_TUNE);
    assert(output.calibrated_lowest_value == 65533);
    assert(dac.last == raw(65533));
    output.calibrated_lowest_value = 65535;
    state.nudge_up();
    assert(output.calibrated_lowest_value == 65535);
    output.calibrated_lowest_value = 0;
    state.nudge_down();
    assert(output.calibrated_lowest_value == 0);
    state.revert();
    assert(!output.calibration_mode);
    assert(output.calibrated_lowest_value == 8);
    state.start_calibrate_high();
    for (int i = 0; i < 20; ++i) state.too_high();
    assert(state.current_test_value == 0);
    state.too_low();
    assert(state.current_test_value == 1);
    state.revert();
    state.fine_step = 64;
    state.nudge_down();  // Also works without starting a binary search.
    assert(output.calibrated_highest_value == 64959);
    assert(dac.last == raw(64959));
    state.revert();
    assert(output.calibrated_highest_value == 65023);
    state.output_check_voltage((floor + ceiling) / 2);
    assert(output.calibration_mode);
    assert(dac.last == raw(32515));
    output.setTargetValueFromData(ceiling, true);
    output.process_pending();
    assert(dac.last == raw(32515));
    assert(output.output_voltage == (floor + ceiling) / 2);
    output.set_output_locked(false);
    // Missing feedback must not overwrite existing endpoint calibration.
    output.calibrate();
    assert(output.calibrated_lowest_value == 8 && output.calibrated_highest_value == 65023);
    output.calibration_input = &input;
    output.calibrate();
    assert(std::abs(int(output.calibrated_lowest_value) - 2000) <= 10);
    assert(std::abs(int(output.calibrated_highest_value) - 63000) <= 10);
    for (float voltage : {floor, (floor + ceiling) / 2, ceiling}) {
        dac.write(0, output.get_dac_value_for_voltage(voltage));
        assert(std::fabs(input.fetch_current_voltage() - voltage) < 0.002f);
    }
}
void check_midi_isolation() {
    DAC8574 dac;
    CVOutputParameter<> output;
    output.target = &dac;
    output.minimumDataLimit = output.minimumDataRange = -5;
    output.maximumDataLimit = output.maximumDataRange = 5;
    output.sendNoteOn(48, 100, 1);
    assert(output.current == 4 && output.gate.active);
    output.sendNoteOff(40, 0, 1);
    assert(output.current_pitch_note == 48 && output.note_is_active && output.gate.active);
    output.set_calibration_voltage(-2);
    const auto test_raw = dac.last;
    output.sendNoteOn(36, 100, 1);
    output.park_pitch(60);
    output.set_channel_pitch_bend_semitones(12);
    output.process_pending();
    assert(output.current == 4 && dac.last == test_raw);
    assert(output.output_voltage == -2);
    output.set_output_locked(false);
    output.modulation = 0.1f;
    output.set_midi_notes_enabled(false);
    assert(output.current == 0 && !output.note_is_active && !output.gate.active);
    output.sendNoteOn(60, 100, 1);
    output.park_pitch(60);
    output.set_channel_pitch_bend_semitones(12);
    output.set_omni_pitch_bend_semitones(12);
    assert(output.current == 0);
    assert(output.channel_pitch_bend_semitones == 0 && output.omni_pitch_bend_semitones == 0);
    assert(std::fabs(output.output_voltage - 1.0f) < 0.001f);
    output.set_midi_notes_enabled(true);
    assert(output.getCurrentDataValue() == 0); // Held pitch does not return on re-enable.
    output.set_midi_notes_enabled(false);
    output.updateValueFromData(4); // A scene/UI base edit cannot offset MIDI-Off output.
    assert(output.getCurrentDataValue() == 0);
    output.process_pending();
    assert(std::fabs(output.output_voltage - 1.0f) < 0.001f);
    output.set_midi_notes_enabled(true);
    output.sendNoteOn(36, 100, 1);
    output.set_park_enabled(false);
    assert(output.current == 3);
    output.sendNoteOff(36, 0, 1);
    assert(output.current == 0);
}
int main() {
    check_midi_isolation();
    for (bool inverted : {false, true}) {
        check_range(0, 10, inverted);
        check_range(-5, 5, inverted);
        check_range(-2, 8, inverted);
    }
    std::puts("CV output mapping, bipolar modulation, inversion, and calibration checks passed");
}
'''
    with tempfile.TemporaryDirectory(prefix="cvoutput-test-") as directory:
        source_path = Path(directory) / "test.cpp"
        executable = Path(directory) / "test"
        source_path.write_text(source)
        subprocess.run([
            "g++", "-std=c++14", "-Wall", "-Wextra", "-Werror",
            "-Wno-unused-but-set-variable", "-Wno-unused-parameter", "-Wno-format",
            "-fsanitize=undefined,float-cast-overflow", "-fno-sanitize-recover=all",
            str(source_path), "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
