#include "voltage_sources/ADSVoltageSource.h"
#include "voltage_sources/ADS24vVoltageSource.h"

#include "SdFat.h"

#ifdef ENABLE_SCREEN
    #include "submenuitem_bar.h"
    #include "menuitems_object.h"
    //#include "ParameterManager.h"

    FLASHMEM MenuItem *ADSVoltageSourceBase::makeCalibrationControls(int i) {
        Serial.println("makeCalibrationControls() for an ADS24vVoltageSource!"); Serial.flush();

        Serial.printf("MENU_C_MAX is %i\n", MENU_C_MAX);
        char name[MENU_C_MAX];
        sprintf(name, "Voltage Source %i Calibrator", i);
        SubMenuItemBar *submenu = new SubMenuItemBar(name);

        Serial.println("makeCalibrationControls() creating ctrl1!"); Serial.flush();
        DirectNumberControl<float> *ctrl1 = new DirectNumberControl<float> ("correction_1", &(this->correction_value_1), this->correction_value_1, 1024.0, 1200.0, nullptr);
        ctrl1->step = 0.25;
        ctrl1->float_mult = 10.0;

        Serial.println("makeCalibrationControls() creating ctrl2!"); Serial.flush();
        DirectNumberControl<float> *ctrl2 = new DirectNumberControl<float> ("correction_2", &(this->correction_value_2), this->correction_value_2, 0.020, 0.099, nullptr);
        ctrl2->step = 0.00025;
        ctrl2->float_mult = 10000.0;

        Serial.println("makeCalibrationControls() adding to submenu!"); Serial.flush();   
        submenu->add(ctrl1);
        submenu->add(ctrl2);

        Serial.println("makeCalibrationControls() creating current_value_disp control!"); Serial.flush();   
        DirectNumberControl<double> *current_value_disp = new DirectNumberControl<double> 
            ("current", &this->current_value, this->current_value, -10.0, 10.0, nullptr);
        current_value_disp->readOnly = true;
        submenu->add(current_value_disp);

        //Serial.println("makeCalibrationControls() returning!"); Serial.flush();   
        Serial.printf("makeCalibrationControls() returning - i is %i!\n", i); Serial.flush();   
        return submenu;
    }

#endif


// todo: different implementation depending on whether file stuff is available or not?
#ifndef DISABLE_CALIBRATION_STORAGE
    #include "SD.h"
    #define FILEPATH_CALIBRATION_FORMAT       "calibration_voltage_source_%i.txt"
    
    void ADSVoltageSourceBase::load_calibration() {
        // todo: make VoltageSource know its name so that it knows where to load from
        Serial.printf("ADSVoltageSourceBase: load_calibration for slot!\n", slot);
        //int slot = parameter_manager.find_slot_for_voltage(this);

        //parameter_manager->load_voltage_calibration(slot); //, this);
        File myFile;

        char filename[255] = "";
        sprintf(filename, FILEPATH_CALIBRATION_FORMAT, slot); //, preset_number);
        Serial.printf("\tload_calibration() opening '%s' for slot %i\n", filename, slot);
        myFile = SD.open(filename, FILE_READ);
        myFile.setTimeout(0);

        if (!myFile) {
            Serial.printf("Error: Couldn't open %s for reading!\n", filename);
            return; // false;
        }
        String line;
        while (line = myFile.readStringUntil('\n')) {
            String key = line.substring(0, line.indexOf("="));
            String value = line.substring(line.indexOf("=")+1);
            Serial.printf("\tfor %s, found value '%s' => %6.6f\n", key, value, value.toFloat());
            if (key.equals("correction_value_1")) {
                this->correction_value_1 = value.toFloat();
            } else if (key.equals("correct_value_2")) {
                this->correction_value_2 = value.toFloat();
            }
        }
        myFile.close();

        Serial.printf("for slot %i, got calibration values %6.6f : %6.6f\n", slot, this->correction_value_1, this->correction_value_2);
    }
    void ADSVoltageSourceBase::save_calibration() {
        // todo: make VoltageSource know its name so that it knows where to save to
        Serial.printf("ADSVoltageSourceBase: load_calibration for slot %i!", slot);
        //int slot = parameter_manager.find_slot_for_voltage(this);

        //parameter_manager->save_voltage_calibration(slot);

        Serial.printf("\tfor slot %i, saving calibration values %6.6f : %6.6f\n", slot, this->correction_value_1, this->correction_value_2);
       
        File myFile;

        char filename[255] = "";
        sprintf(filename, FILEPATH_CALIBRATION_FORMAT, slot); //, preset_number);
        Serial.printf("\tsave_calibration() opening %s\n", filename);
        myFile = SD.open(filename, FILE_WRITE_BEGIN);

        myFile.printf("correction_value_1=%6.6f\n", this->correction_value_1);
        myFile.printf("correction_value_2=%6.6f\n", this->correction_value_2);

        myFile.close();
    }

#endif
