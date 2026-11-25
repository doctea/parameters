const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const header = fs.readFileSync(path.join(__dirname, 'include/parameters/Parameter.h'), 'utf8');
const implementation = fs.readFileSync(path.join(__dirname, 'src/parameters/parameters.cpp'), 'utf8');
const extract = (text, expression) => {
    const match = text.match(expression);
    if (!match) throw new Error(`Could not find ${expression}`);
    return match[0];
};
const slotConfig = extract(header, /#ifndef MAX_SLOT_CONNECTIONS\s+#define MAX_SLOT_CONNECTIONS 3\s+#endif/);
const connection = extract(header, /struct ParameterToInputConnection \{[\s\S]*?\n\};/);
const reset = extract(header, /virtual void reset_mappings\(\) \{[\s\S]*?\n    \}/);
const evaluation = extract(implementation, /bool FloatParameter::is_modulation_slot_active\(int slot\) \{[\s\S]*?(?=    \/\/ Called on every clock tick)/);
const persistence = extract(fs.readFileSync(path.join(__dirname, 'src/parameters/Parameter.cpp'), 'utf8'),
    /class ModulationSlotsSaveableSetting : public SaveableSettingBase \{[\s\S]*?\n    \};/);
const reorder = extract(fs.readFileSync(path.join(__dirname, 'src/parameters/Parameter.cpp'), 'utf8'),
    /bool FloatParameter::swap_slots\(int from, int to\) \{[\s\S]*?\n\}/) + '\n' +
    extract(fs.readFileSync(path.join(__dirname, 'src/parameters/Parameter.cpp'), 'utf8'),
        /bool FloatParameter::move_slot\(int from, int to\) \{[\s\S]*?\n\}/);
const graph = extract(fs.readFileSync(path.join(__dirname, 'include/mymenu_items/ParameterMenuItems.h'), 'utf8'),
    /class ParameterOutputMenuItem : public ParameterValueMenuItem \{[\s\S]*?\n\};/);
const mixerHeader = fs.readFileSync(path.join(__dirname, 'include/parameter_inputs/MixerParameterInput.h'), 'utf8');
const mixerCopy = extract(mixerHeader, /bool copy_from\(const VirtualMixerParameterInput \*source\) \{[\s\S]*?\n        \}/);
const mixerPresets = extract(mixerHeader, /bool preset_path\(int preset,[\s\S]*?bool load_preset\(int preset\) \{[\s\S]*?\n            \}/);
const selector = extract(fs.readFileSync(path.join(__dirname, 'include/mymenu_items/ParameterInputMenuItems.h'), 'utf8'),
    /class ParameterSlotInputSelectorControl : public ParameterInputSelectorControl<FloatParameter> \{[\s\S]*?\n\};/);
const source = `
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using byte = uint8_t;
using std::abs;
#define F(value) value
#define Debug_printf(...) ((void)0)
#define MODULATION_THRESHOLD 0.01f
#define MOD_SLOT_SUM 0
#define MOD_SLOT_MULT 1
enum { MOD_SLOT_BI_NATIVE, MOD_SLOT_UNI_RAW, MOD_SLOT_UNI_CENTERED };
enum SHMode { SH_OFF, SH_ON };
struct BaseParameterInput {
    const char *name = "Group:One";
    float get_normal_value_unipolar() { return 0.75f; }
    float get_normal_value_bipolar() { return 0.5f; }
};
const char *get_sh_mode_save_string(SHMode mode) { return mode == SH_ON ? "on" : "off"; }
SHMode get_sh_mode_from_save_string(const char *mode) { return strcmp(mode, "on") == 0 ? SH_ON : SH_OFF; }
#define SL_MAX_LINE 512
struct SaveableSettingBase {
    char linebuf[SL_MAX_LINE];
    void set_label(const char *) {}
    void set_category(const char *) {}
    virtual const char *get_line() = 0;
    virtual bool parse_key_value(const char *, const char *) = 0;
};
${slotConfig}
${connection}
class FloatParameter {
public:
    ParameterToInputConnection connections[MAX_SLOT_CONNECTIONS];
    bool debug = false;
    const char *label = "test";
    bool is_valid_slot(int slot) { return slot >= 0 && slot < MAX_SLOT_CONNECTIONS; }
    void set_slot_amount(int slot, float amount) { connections[slot].amount = amount; }
    float get_amount_for_slot(int slot) { return connections[slot].amount; }
    const char *get_input_group_and_name_for_slot(int slot) { return connections[slot].parameter_input->name; }
    ${reset}
    bool is_modulation_slot_active(int slot);
    float get_modulation_value();
    bool swap_slots(int from, int to);
    bool move_slot(int from, int to);
};
${evaluation}
struct ParameterManager {
    BaseParameterInput *input = nullptr;
    FloatParameter *awaiting_midi_learn = nullptr;
    int cancellations = 0;
    void stop_midi_learn() { awaiting_midi_learn = nullptr; ++cancellations; }
    BaseParameterInput *getInputForGroupAndName(const char *name) {
        return input && strcmp(name, input->name) == 0 ? input : nullptr;
    }
} manager;
ParameterManager *parameter_manager = &manager;
${reorder}
${persistence}
int main() {
    FloatParameter parameter;
    BaseParameterInput input;
    const int last = MAX_SLOT_CONNECTIONS - 1;
    parameter.connections[last].parameter_input = &input;
    parameter.connections[last].amount = 0.8f;
    assert(std::fabs(parameter.get_modulation_value() - 0.6f) < 0.00001f);
    parameter.reset_mappings();
    assert(parameter.connections[last].amount == 0.0f);
    assert(parameter.get_modulation_value() == 0.0f);
    manager.input = &input;
    parameter.connections[0].parameter_input = &input;
    parameter.connections[0].amount = 0.5f;
    parameter.connections[0].polar_mode = MOD_SLOT_UNI_CENTERED;
    parameter.connections[0].sh_mode = SH_ON;
    parameter.connections[0].operation_mode = MOD_SLOT_MULT;
    parameter.connections[last].parameter_input = nullptr;
    ModulationSlotsSaveableSetting setting(&parameter);
    char saved[SL_MAX_LINE];
    strcpy(saved, setting.get_line());
    assert(strcmp(saved, "modslots=Group:One,0.5,uni_centered,on,mult") == 0);
    parameter.connections[last].parameter_input = &input;
    parameter.connections[last].amount = 1.0f;
    assert(setting.parse_key_value("modslots", saved + strlen("modslots=")));
    assert(parameter.connections[0].parameter_input == &input);
    assert(parameter.connections[0].polar_mode == MOD_SLOT_UNI_CENTERED);
    assert(parameter.connections[0].sh_mode == SH_ON);
    assert(parameter.connections[0].operation_mode == MOD_SLOT_MULT);
    assert(parameter.connections[last].parameter_input == nullptr && parameter.connections[last].amount == 0);
    assert(setting.parse_key_value("modslots", "Group:One,0.25,bipolar"));
    assert(parameter.connections[0].polar_mode == MOD_SLOT_BI_NATIVE);
    assert(parameter.connections[0].sh_mode == SH_OFF && parameter.connections[0].operation_mode == MOD_SLOT_SUM);
    parameter.connections[last].amount = 0.3f;
    strcpy(saved, setting.get_line());
    assert(setting.parse_key_value("modslots", saved + strlen("modslots=")));
    assert(parameter.connections[last].parameter_input == nullptr);
    assert(std::fabs(parameter.connections[last].amount - 0.3f) < 0.00001f);
    assert(setting.parse_key_value("modslots", ""));
    assert(parameter.connections[0].parameter_input == nullptr && parameter.connections[last].amount == 0);
    for (int slot = 0; slot < MAX_SLOT_CONNECTIONS; ++slot) {
        parameter.connections[slot].amount = (float)(slot + 1);
        parameter.connections[slot].sh_last_tick = slot + 10;
    }
    manager.awaiting_midi_learn = &parameter;
    assert(!parameter.move_slot(-1, last) && manager.awaiting_midi_learn == &parameter);
    assert(!parameter.move_slot(256, last) && !parameter.swap_slots(0, 256));
    assert(parameter.move_slot(0, last));
    #ifdef ENABLE_PARAMETERS_MIDI_LEARN
        assert(manager.awaiting_midi_learn == nullptr && manager.cancellations == 1);
    #endif
    assert(parameter.connections[last].amount == 1 && parameter.connections[last].sh_last_tick == 10);
    assert(parameter.connections[0].amount == 2 && parameter.connections[0].sh_last_tick == 11);
    assert(parameter.move_slot(last, 0));
    assert(parameter.connections[0].amount == 1 && parameter.connections[last].amount == last + 1);
    assert(parameter.swap_slots(0, last));
    assert(parameter.connections[0].amount == last + 1 && parameter.connections[last].amount == 1);
}
`;

const graphSource = `
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <new>
static int allocations = 0;
void *operator new[](size_t size, const std::nothrow_t &) noexcept { ++allocations; return std::malloc(size); }
void operator delete[](void *memory) noexcept { if (memory) { --allocations; std::free(memory); } }
#define IF_MENU_PERF_PARTIAL_UPDATES(body) body
#define BLACK 0
#define C_WHITE 1
#define RED 2
#define GREEN 3
struct FloatParameter { float value = 0; float getLastOutputNormalValue() { return value; } };
struct Display {
    int x = 20, y = 30, lines = 0, fills = 0, order = 0, text_order = 0, line_order = 0, last_color = 0;
    int characterWidth() { return 2; }
    int getSingleRowHeight() { return 4; }
    int height() { return 100; }
    int getCursorX() { return x; }
    int getCursorY() { return y; }
    void setCursor(int new_x, int new_y) { x = new_x; y = new_y; }
    void fillRect(int left, int top, int width, int height, int) { assert(left == 20 && top == 30 && width == 6 && height == 8); ++fills; }
    void drawLine(int x1, int y1, int x2, int y2, int color) {
        assert(x1 >= 20 && x1 < 26 && x2 >= 20 && x2 < 26);
        assert(y1 >= 30 && y1 < 38 && y2 >= 30 && y2 < 38);
        ++lines;
        last_color = color; line_order = ++order;
    }
} display;
class ParameterValueMenuItem {
public:
    FloatParameter **parameter;
    Display *tft = &display;
    int text_calls = 0;
    ParameterValueMenuItem(char *, FloatParameter **p) : parameter(p) {}
    virtual ~ParameterValueMenuItem() {}
    void set_show_output_mode() {}
    void request_redraw() {}
    virtual bool action_opened() { return true; }
    virtual void update_ticks(unsigned long) {}
    virtual void on_hidden() {}
    virtual int renderValue(bool, bool, uint16_t) { ++text_calls; display.text_order = ++display.order; return 34; }
};
${graph}
int main() {
    FloatParameter first, second;
    FloatParameter *current = &first;
    {
        ParameterOutputMenuItem output(&current);
        assert(allocations == 0 && output.renderValue(false, false, 3) == 38 && allocations == 1);
        assert(output.text_calls == 1 && display.fills == 0 && display.lines == 0);
        first.value = -1; output.update_ticks(1);
        first.value = 2; output.update_ticks(2);
        display.x = 20; display.y = 30; display.order = display.lines = 0;
        output.renderValue(false, false, 3);
        assert(display.lines == 2 && display.fills == 0 && display.last_color == GREEN);
        assert(display.text_order < display.line_order);
        current = &second;
        display.lines = 0; display.x = 20; display.y = 30;
        output.renderValue(false, false, 3);
        assert(display.lines == 0);
        output.on_hidden();
        assert(allocations == 0);
        display.x = 20; display.y = 100;
        assert(output.renderValue(false, false, 3) == 108 && allocations == 0);
        display.x = 20; display.y = 30;
        output.renderValue(false, false, 3);
        assert(allocations == 1);
        output.action_opened();
        assert(allocations == 0);
        const int text_calls = output.text_calls;
        assert(output.renderValue(false, false, 3) == 34 && output.text_calls == text_calls + 1);
        output.action_opened();
        display.x = 20; display.y = 30;
        output.renderValue(false, false, 3);
        assert(allocations == 1 && display.fills == 1 && output.text_calls == text_calls + 1);
        output.action_opened();
        display.x = 20; display.y = 30; display.order = 0;
        output.renderValue(false, false, 3);
        assert(display.text_order < display.line_order && display.fills == 1);
    }
    assert(allocations == 0);
}
`;

const mixerSource = `
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#define ENABLE_STORAGE
#define ENABLE_SD
#define ENABLE_PARAMETERS_MIDI_LEARN
#define MAX_SLOT_CONNECTIONS 4
#define MIXER_PRESET_DIRECTORY "mixer_presets"
#define MIXER_PRESET_PATH_FORMAT MIXER_PRESET_DIRECTORY "/preset%d.txt"
#define SL_SCOPE_SCENE 4
struct Connection { int input = 0; float amount = 0; float sh_last_sample = 0; uint32_t sh_last_tick = UINT32_MAX; bool midi_learn_active = false; };
struct Parameter {
    Connection connections[4]; float minimum = 0, maximum = 1, value = 0;
    float getRangeMinimumLimit() { return minimum; }
    float getRangeMaximumLimit() { return maximum; }
    float getCurrentDataValue() { return value; }
    void setRangeMinimumLimit(float v) { minimum = v; }
    void setRangeMaximumLimit(float v) { maximum = v; }
    void updateValueFromData(float v) { value = v; }
};
struct Manager { Parameter *awaiting_midi_learn = nullptr; int cancellations = 0; void stop_midi_learn() { awaiting_midi_learn = nullptr; ++cancellations; } } manager;
Manager *parameter_manager = &manager;
struct Filesystem {
    std::set<std::string> paths; bool mkdir_ok = true, remove_ok = true; int removals = 0;
    bool exists(const char *path) { return paths.count(path); }
    bool mkdir(const char *path) { if (!mkdir_ok) return false; paths.insert(path); return true; }
    bool remove(const char *path) { if (!remove_ok) return false; ++removals; paths.erase(path); return true; }
} SD;
class VirtualMixerParameterInput;
bool sl_save_to_file(VirtualMixerParameterInput *, const char *, int);
bool sl_load_from_file_into(VirtualMixerParameterInput *, const char *, int);
class VirtualMixerParameterInput {
public:
    Parameter *underlying_parameter; int input_type = 0; bool inverted = false;
    void setInverted(bool value) { inverted = value; }
    
    ${mixerCopy}
    ${mixerPresets}
};
VirtualMixerParameterInput *last_target = nullptr;
bool sl_save_to_file(VirtualMixerParameterInput *target, const char *path, int scope) { assert(scope == 4); last_target = target; SD.paths.insert(path); return true; }
bool sl_load_from_file_into(VirtualMixerParameterInput *target, const char *path, int scope) { assert(scope == 4); last_target = target; return SD.exists(path); }
int main() {
    Parameter first, second;
    VirtualMixerParameterInput source{&first}, target{&second};
    source.input_type = 2; source.inverted = true; first.minimum = -0.5f; first.maximum = 0.8f; first.value = 0.3f;
    first.connections[3] = {7, 0.6f, 0.9f, 42, true}; manager.awaiting_midi_learn = &second;
    assert(!target.copy_from(nullptr) && !target.copy_from(&target));
    assert(target.copy_from(&source) && manager.cancellations == 1);
    assert(target.input_type == 2 && target.inverted && second.minimum == -0.5f && second.maximum == 0.8f && second.value == 0.3f);
    assert(second.connections[3].input == 7 && second.connections[3].amount == 0.6f);
    assert(second.connections[3].sh_last_sample == 0 && second.connections[3].sh_last_tick == UINT32_MAX && !second.connections[3].midi_learn_active);
    assert(first.connections[3].sh_last_sample == 0.9f);
    char tiny[2]; assert(!target.preset_path(0, tiny, sizeof(tiny)) && !target.preset_exists(-1) && !target.save_preset(8));
    SD.mkdir_ok = false; assert(!target.save_preset(0)); SD.mkdir_ok = true;
    assert(target.save_preset(0) && target.preset_exists(0) && last_target == &target);
    SD.remove_ok = false; assert(!target.save_preset(0)); SD.remove_ok = true;
    assert(target.save_preset(0) && SD.removals == 1);
    assert(source.load_preset(0) && last_target == &source && !source.load_preset(1));
}
`;

const selectorSource = `
#include <cassert>
struct BaseParameterInput {};
struct FloatParameter {
    BaseParameterInput *inputs[4] = {};
    bool is_valid_slot(int slot) { return slot >= 0 && slot < 4; }
    BaseParameterInput *get_slot_input(int slot) { return inputs[slot]; }
    void set_slot_input(int slot, BaseParameterInput *input) { inputs[slot] = input; }
};
template<class T> struct GenericList {};
template<class T> class ParameterInputSelectorControl {
    T **target;
protected:
    virtual BaseParameterInput *read_target_input(T *) { return nullptr; }
    virtual void write_target_input(T *, BaseParameterInput *) {}
public:
    ParameterInputSelectorControl(const char *, T **object, void(T::*)(BaseParameterInput *), BaseParameterInput *(T::*)(), GenericList<BaseParameterInput *> *, BaseParameterInput *) : target(object) {}
    BaseParameterInput *read() { return read_target_input(*target); }
    void write(BaseParameterInput *input) { write_target_input(*target, input); }
};
${selector}
int main() {
    FloatParameter parameter; FloatParameter *proxy = &parameter; GenericList<BaseParameterInput *> inputs;
    BaseParameterInput first, second;
    ParameterSlotInputSelectorControl third("third", &proxy, 2, &inputs), fourth("fourth", &proxy, 3, &inputs), invalid("invalid", &proxy, 4, &inputs);
    third.write(&first); fourth.write(&second); invalid.write(&first);
    assert(third.read() == &first && fourth.read() == &second && invalid.read() == nullptr);
    fourth.write(nullptr); assert(third.read() == &first && fourth.read() == nullptr);
    proxy = nullptr; fourth.write(&second); assert(fourth.read() == nullptr && parameter.inputs[3] == nullptr);
}
`;

const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'modslot-count-'));
try {
    const sourcePath = path.join(temporary, 'test.cpp');
    const binary = path.join(temporary, process.platform === 'win32' ? 'test.exe' : 'test');
    fs.writeFileSync(sourcePath, source);
    for (const flags of [[], ['-DMAX_SLOT_CONNECTIONS=4', '-DENABLE_PARAMETERS_MIDI_LEARN']]) {
        const compile = spawnSync(process.env.CXX || 'g++', ['-std=c++11', ...flags, sourcePath, '-o', binary], { stdio: 'inherit' });
        if (compile.error || compile.status !== 0) throw compile.error || new Error('Compilation failed');
        const result = spawnSync(binary, [], { stdio: 'inherit' });
        if (result.error || result.status !== 0) throw result.error || new Error('Slot count test failed');
    }
    console.log('Three- and four-slot modulation, persistence, and reordering checks passed.');
    fs.writeFileSync(sourcePath, graphSource);
    const graphCompile = spawnSync(process.env.CXX || 'g++', ['-std=c++11', sourcePath, '-o', binary], { stdio: 'inherit' });
    if (graphCompile.error || graphCompile.status !== 0) throw graphCompile.error || new Error('Graph compilation failed');
    const graphResult = spawnSync(binary, [], { stdio: 'inherit' });
    if (graphResult.error || graphResult.status !== 0) throw graphResult.error || new Error('Graph test failed');
    console.log('Output graph allocation, proxy, and drawing checks passed.');
    fs.writeFileSync(sourcePath, mixerSource);
    const mixerCompile = spawnSync(process.env.CXX || 'g++', ['-std=c++14', '-fsanitize=undefined', sourcePath, '-o', binary], { stdio: 'inherit' });
    if (mixerCompile.error || mixerCompile.status !== 0) throw mixerCompile.error || new Error('Mixer compilation failed');
    const mixerResult = spawnSync(binary, [], { stdio: 'inherit' });
    if (mixerResult.error || mixerResult.status !== 0) throw mixerResult.error || new Error('Mixer test failed');
    console.log('Mixer copy and preset path checks passed.');
    fs.writeFileSync(sourcePath, selectorSource);
    const selectorCompile = spawnSync(process.env.CXX || 'g++', ['-std=c++14', '-fsanitize=undefined', sourcePath, '-o', binary], { stdio: 'inherit' });
    if (selectorCompile.error || selectorCompile.status !== 0) throw selectorCompile.error || new Error('Selector compilation failed');
    const selectorResult = spawnSync(binary, [], { stdio: 'inherit' });
    if (selectorResult.error || selectorResult.status !== 0) throw selectorResult.error || new Error('Slot selector test failed');
    console.log('Indexed slot selector checks passed.');
} finally {
    fs.rmSync(temporary, { recursive: true, force: true });
}