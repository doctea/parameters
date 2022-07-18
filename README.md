# Eurorack Arduino parameters library

Work-in-progress, classes designed for handling Eurorack inputs in Arduino platform projects and mapping values to object-setter functions.

Based on code extracted from my [envor2](https://github.com/doctea/envor2) and [talkie](https://github.com/doctea/talkie) projects, currently used by [sidenhancy](https://github.com/doctea/sidenhancy).

## Currently working
- Analogue inputs via [Rob Tillaart's ADS1X15 library](https://github.com/RobTillaart/ADS1X15)
- 1v/oct inputs using the [Pimoroni +/-24v ADC breakout](https://coolcomponents.co.uk/products/ads1015-24v-adc-breakout)
- Convert incoming analogue level to frequency in 1v/oct fashion
- MenuItems for [mymenu](https://github.com/doctea/mymenu) that allow changing Parameter values and ParameterInput-Parameter mappings

## Todo
- ~~Make the usual analogRead()-based classes work again~~
- Make the GateInput classes work again, to trigger on/offs
- Provide scaling logic for eg opamp-scaled inputs
- Make envor2 and talkie use this library
- Docs/examples how to use the library in a project from scratch
