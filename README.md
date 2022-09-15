# Eurorack Arduino parameters library

Work-in-progress, classes designed for handling Eurorack inputs in Arduino platform projects and mapping values to object-setter functions.

Based on code extracted from my [envor2](https://github.com/doctea/envor2) and [talkie](https://github.com/doctea/talkie) projects, currently used by [sidenhancy](https://github.com/doctea/sidenhancy) and [usb_midi_clocker](https://github.com/doctea/usb_midi_clocker).

## Currently working
- Analogue inputs via [Rob Tillaart's ADS1X15 library](https://github.com/RobTillaart/ADS1X15)
- 1v/oct inputs using the [Pimoroni +/-24v ADC breakout](https://coolcomponents.co.uk/products/ads1015-24v-adc-breakout)
- Convert incoming analogue level to frequency in 1v/oct fashion
- MenuItems for [mymenu](https://github.com/doctea/mymenu) that allow changing Parameter values and ParameterInput-Parameter mappings

## Untested

- analogRead()-based inputs on Arduino analog pins

## Todo
- ~~Make the usual analogRead()-based classes work again~~ think done, just needs testing?
- ~~Make the GateInput classes work again, to trigger on/offs~~ think done, just needs testing?
- Provide scaling logic for eg opamp-scaled inputs
- Make envor2 and talkie use this library
 - Hmm, adapted envor2 to use this, but doesn't work; suspect requires too much memory for the Nano to handle?
- Docs/examples how to use the library in a project from scratch
- Provide a manager class that encapsulates all the annoying stuff like setting up and checking hte values (currently need to add code to do this in your app, see sidenhancy and usb_midi_clocker for examples)

### Total rebuild
- ok, need to change all this, cos its getting too confusing.
- so the Parameter should request the value from the ParameterInput, instead of ParameterInput pushing it to the Parameter.
 - Parameter to be able to pull from multiple ParameterInputs, and choose the mix level, etc.
- each ParameterInput should have a set of options to mark them as bipolar, unipolar, inverted, range, frequency.
- ParameterInput should only send doubles, range -1 to +1?
- ParameterControl should ask the Parameter how to display the value of a double ?
 - But how should ParameterControl know what a knob left/knob right should do to its internal value?
 - Maybe ParameterControl should ask Parameter 'what is X after knob turned right?' etc
- Only Parameter needs to care about what its final output datatype is?

### Classes and structure

- VoltageSources connect directly to ADS or analog pin
- ParameterInputs fetches values from VoltageSources and send them to Parameters
- a Parameter is the final destination of the value (eg, Envelope); objects should inherit from Parameter, or simple wrappers are provided to wrap around objects and provide callbacks

