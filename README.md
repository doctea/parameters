# Eurorack Teensy/Arduino parameters library

Work-in-progress, classes designed for handling Eurorack inputs in Arduino platform projects and mapping values to object-setter functions.

Based on code extracted from my [envor2](https://github.com/doctea/envor2) and [talkie](https://github.com/doctea/talkie) projects, currently used by ~~[sidenhancy](https://github.com/doctea/sidenhancy) and~~ [usb_midi_clocker](https://github.com/doctea/usb_midi_clocker).

## Currently working
- Teensy 4.1
- 3x analogue inputs using the [Pimoroni +/-24v ADC breakout](https://coolcomponents.co.uk/products/ads1015-24v-adc-breakout) and [Rob Tillaart's ADS1X15 library](https://github.com/RobTillaart/ADS1X15)
- MenuItems for [mymenu](https://github.com/doctea/mymenu) that allow changing Parameter values and ParameterInput-Parameter mappings
  - graph of modulation amount/history
  - selectable 3 ParameterInputs per Parameter
  - change how much each selected ParameterInput affects the Parameter, from -100% to +100%
  - switch between bipolar/unipolar ranges on per-ParameterInput level, for both input and output
- MIDI CC as a modulation source
- Convert incoming analogue level to frequency in 1v/oct fashion and output of corresponding MIDI note

## Probably working, but untested
- Analogue inputs via [Rob Tillaart's ADS1X15 library](https://github.com/RobTillaart/ADS1X15)

# Probably not currently working / untested
- Arduino
- analogRead()-based inputs on Arduino analog pins
- Gate inputs

## Todo

- *todo*: implement corrected voltage calculation based on https://github.com/pimoroni/ads1015-python/blob/9ae775bc0a4a148771ef1d36a98adb76cc6cf54e/library/ads1015/__init__.py#L375
- ~~Make the usual analogRead()-based classes work again~~ think done, just needs testing?
- ~~Make the GateInput classes work again, to trigger on/offs~~ think done, just needs testing?
- ~~Move the MIDICCParameter and MIDICCProxiedParameter stuff out of [usb_midi_clocker](https://github.com/doctea/usb_midi_clocker) into this library~~
- Provide scaling logic for eg opamp-scaled inputs
- Make envor2 and talkie use this library
  - Hmm, adapted envor2 to use this, but doesn't work; suspect requires too much memory for the Nano to handle?
- Docs/examples how to use the library in a project from scratch
- ~~Provide a manager class that encapsulates all the annoying stuff like setting up and checking the values (currently need to add code to do this in your app, see sidenhancy and usb_midi_clocker for examples)~~
- Back port so it can be used by [sidenhancy](https://github.com/doctea/sidenhancy) again
- Inputs from non-voltage sources, eg network, ~~MIDI~~, LFOs

### Classes and structure

- ADCDevices set up a hardware ADC device (eg ADS1x15 library + Pimoroni +/-24v board)
- VoltageSources read from the ADCDevice
- ParameterInputs fetches values from VoltageSources
  - if supports_pitch then can also generate a corresponding note 1v/oct value for the voltage
- Parameters fetch values from VoltageSources, apply amount % modulation and mix together
  - Result sent to target objects (eg MIDI output, envelope, etc)

- ParameterMenuItem is a way to interact directly with that Parameter value.
  - asks the Parameter how to render the value, how to increment/decrement, etc..
  - select from available ParameterInputs and choose depth and mixing
  - ParameterInputViewItem is a graphical display of input
 
