# Eurorack Arduino parameters library

Work-in-progress, classes designed for handling Eurorack inputs in Arduino platform projects and mapping values to object-setter functions.

Based on code extracted from my [envor2](https://github.com/doctea/envor2) and [talkie](https://github.com/doctea/talkie) projects, currently used by [sidenhancy](https://github.com/doctea/sidenhancy) and [usb_midi_clocker](https://github.com/doctea/usb_midi_clocker).

## Currently working
- Analogue inputs via [Rob Tillaart's ADS1X15 library](https://github.com/RobTillaart/ADS1X15)
- Convert incoming analogue level to frequency in 1v/oct fashion
- MenuItems for [mymenu](https://github.com/doctea/mymenu) that allow changing Parameter values and ParameterInput-Parameter mappings

## Todo
- ~~Make the usual analogRead()-based classes work again~~ think done, just needs testing?
- ~~Make the GateInput classes work again, to trigger on/offs~~ think done, just needs testing?
- Provide scaling logic for eg opamp-scaled inputs
- Make envor2 and talkie use this library
 - Hmm, adapted envor2 to use this, but doesn't work; suspect requires too much memory for the Nano to handle?
- Docs/examples how to use the library in a project from scratch

### Classes and structure

- VoltageSources connect directly to ADS or analog pin
- ParameterInputs fetches values from VoltageSources and send them to Parameters
- a Parameter is the final destination of the value (eg, Envelope); objects should inherit from Parameter, or simple wrappers are provided to wrap around objects and provide callbacks

