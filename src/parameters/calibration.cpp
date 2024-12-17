#ifdef ENABLE_CV_OUTPUT

#include "parameters/calibration.h"
#ifdef USE_ATOMIC
    #include "SimplyAtomic.h"
#endif
#include "ParameterManager.h"

// todo: possibly move this whole thing into CVOutputParameter, or ICalibratable...?
// or like, a DACDevice family of classes 

#include "DAC8574.h"
extern DAC8574 *dac_output;

uint16_t calibrate_find_dac_value_for(int channel, VoltageParameterInput *src, float intended_voltage, bool inverted) {
    if (src==nullptr) {
        Serial.printf("calibrate_find_dac_value_for(channel=%u) passed a null VoltageParameterInput!\n", channel);
        return intended_voltage * 65535.0;
    }
    parameter_manager->update_voltage_sources();
    parameter_manager->update_inputs();
    src->read();
    parameter_manager->update_voltage_sources();
    parameter_manager->update_inputs();
    src->read();
    src->get_voltage();

    //float intended_voltage = 0.0;
    int guess_din = (intended_voltage/10.0) * 65535;
    if (guess_din==0) 
        guess_din = 3000;
    else if (guess_din==65535)
        guess_din = 64000;
    int last_guess_din = guess_din;
    float actual_read = 0.0f;
    float last_read = actual_read;

    bool overshot = false;
    float tolerance = 0.001;

    Serial.printf("----\nStarting calibrate_find_dac_value_for(%i, '%s', %3.3f)\n", channel, src->name, intended_voltage);
    Serial.printf("Starting with guess_din of %i.\n", guess_din);

    //if (inverted) intended_voltage = 10.0 - intended_voltage;

    #ifdef USE_ATOMIC
    ATOMIC()
    #endif
    {
        do {
            dac_output->write(
                channel, 
                //inverted ? (65535 - guess_din) : 
                guess_din
            );
            //delay(1);
            /*
            parameter_manager->update_voltage_sources();
            parameter_manager->update_inputs();
            src->read();
            parameter_manager->update_voltage_sources();
            parameter_manager->update_inputs();
            src->read();
            */
            actual_read = src->fetch_current_voltage();
            actual_read = src->fetch_current_voltage();
            actual_read = src->fetch_current_voltage();
            actual_read = inverted ? 10.0 - actual_read : actual_read;   // INVERT THE *READING*

            if (actual_read > intended_voltage) {
                if (last_guess_din < guess_din) {
                    Serial.println("OVERSHOT 1!");
                    overshot = true;
                    tolerance *= 1.25;
                    Serial.printf("Set tolerance to %3.3f\n", tolerance);
                }
                last_guess_din = guess_din;

                guess_din--;
            } else if (actual_read < intended_voltage) {
                if (last_guess_din > guess_din) {
                    Serial.println("OVERSHOT 2!");
                    overshot = true;
                    tolerance *= 1.25;
                    Serial.printf("Set tolerance to %3.3f\n", tolerance);
                }
                last_guess_din = guess_din;

                guess_din++;
            } //else {
            //}

            /*if (overshot && fabs(actual_read - intended_voltage) > fabs(last_read - intended_voltage)) {
                Serial.println("Breaking because changing found a less-accurate number 1!\n");
                guess_din = last_guess_din;
                break;
            } else if (overshot) {
                Serial.println("Breaking because changing found a less-accurate number 2!\n");
                break;
            }*/

            //last_guess_din = guess_din;

            last_read = actual_read;

            Serial.printf("Finding %3.3f:\t", intended_voltage);
            Serial.printf("Tried %i\t", last_guess_din);
            Serial.printf("And got result %3.3f", last_read);
            if (inverted) Serial.printf(" (inverted)");
            Serial.printf("\t(making difference of %3.3f)", fabs(max(actual_read,intended_voltage) - min(actual_read,intended_voltage)));
            Serial.println();

            //Serial.printf(" (wanted %3.3f)\n", intended_voltage);

        } while (fabs(max(actual_read,intended_voltage) - min(actual_read,intended_voltage)) > tolerance);

        Serial.printf("got actual_read=%3.3f ", actual_read);
        Serial.printf("(%3.3f distance from intended %3.3f, with final tolerance %3.3f) ", fabs(actual_read) - fabs(intended_voltage), intended_voltage, tolerance);
        Serial.printf("and last_guess_din=%i\n", last_guess_din);           
    }

    //return (actual_read/10.0) * 65535.0;
    return guess_din;
}

uint16_t calibrate_find_dac_value_for(int channel, char *input_name, float intended_voltage, bool inverted) {
    VoltageParameterInput *src = (VoltageParameterInput*)parameter_manager->getInputForName(input_name);

    return calibrate_find_dac_value_for(channel, src, intended_voltage, inverted);
}


#endif