# Eurorack Arduino parameters library

Work-in-progress, classes designed for handling Eurorack inputs in Arduino platform projects and mapping values to object-setter functions.

Based on code extracted from my [envor2](https://github.com/doctea/envor2) and [talkie](https://github.com/doctea/talkie) projects, currently used by [https://github.com/doctea/sidenhancy](sidenhancy).

## Currently working
- Analogue inputs via https://github.com/RobTillaart/ADS1X15
- Convert incoming analogue level to frequency in 1v/oct fashion
- MenuItems for [mymenu](https://github.com/doctea/mymenu) that allow changing Parameter values and ParameterInput-Parameter mappings

## Todo
- Make the usual analogRead()-based classes work again
- Make the GateInput classes work to trigger gates on/off
- Make envor2 and talkie use this library
- Docs/examples how to use the library in a project from scratch
