#include "voltage_sources/ADSVoltageSource.h"
#include "voltage_sources/ADS24vVoltageSource.h"

#if defined(ENABLE_SCREEN) && defined(ENABLE_CV_INPUT)
    #include "submenuitem_bar.h"
    #include "menuitems_object.h"
    //#include "ParameterManager.h"
    //FLASHMEM 
    MenuItem *ADSVoltageSourceBase::makeCalibrationControls(int global_slot) {
        Debug_println("makeCalibrationControls() for an ADSVoltageSource!"); Serial_flush();

        Debug_printf("MENU_C_MAX is %i\n", MENU_C_MAX);
        char name[MENU_C_MAX];
        snprintf(name, MENU_C_MAX, "Voltage Source %i Calibrator", global_slot);
        SubMenuItemBar *submenu = new SubMenuItemBar(name);

        Debug_println("makeCalibrationControls() creating ctrl1!"); Serial_flush();
        DirectNumberControl<float> *ctrl1 = new DirectNumberControl<float> (
            "correction_1", 
            &(this->correction_value_1), 
            this->correction_value_1, 
            1024.0, 
            1200.0, 
            nullptr
        );
        ctrl1->step = 0.1;
        ctrl1->float_mult = 10.0;
        ctrl1->float_unit = ' ';

        Debug_println("makeCalibrationControls() creating ctrl2!"); Serial_flush();
        DirectNumberControl<float> *ctrl2 = new DirectNumberControl<float> (
            "correction_2", 
            &(this->correction_value_2), 
            this->correction_value_2, 
            0.020, 
            0.099, 
            nullptr
        );
        ctrl2->step = 0.0001;
        ctrl2->float_mult = 100000.0;
        ctrl2->float_unit = ' ';

        Debug_println("makeCalibrationControls() adding to submenu!"); Serial_flush();   
        submenu->add(ctrl1);
        submenu->add(ctrl2);

        Debug_println("makeCalibrationControls() creating current_value_disp control!"); Serial_flush();   
        DirectNumberControl<float> *current_value_disp = new DirectNumberControl<float> 
            ("current", &this->current_value, this->current_value, -10.0, 10.0, nullptr);
        current_value_disp->readOnly = true;
        current_value_disp->float_unit = 'v';
        submenu->add(current_value_disp);

        //Serial.println("makeCalibrationControls() returning!"); Serial_flush();   
        Debug_printf("makeCalibrationControls() returning - i is %i!\n", i); Serial_flush();   
        return submenu;
    }

#endif

#ifdef ENABLE_SD
    #define STORAGE SD
    #include "SdFat.h"
#endif
#ifdef ENABLE_LITTLEFS
    #define STORAGE LittleFS
    #include <LittleFS.h>
#endif

// todo: different implementation depending on what file libraries are enabled?
#if defined(ENABLE_CALIBRATION_STORAGE)
    //#include "SD.h"
    #ifdef ENABLE_SD
        #include "SD.h"
        #define FILEPATH_CALIBRATION_FORMAT       "calibration_voltage_source_%i.txt"
        #define FILE_READ_MODE FILE_READ
        #define FILE_WRITE_MODE FILE_WRITE_BEGIN
    #elif defined (ENABLE_LITTLEFS)
        // LittleFS has maximum filepath length of 31 characters
        #define FILE_READ_MODE "r"
        #define FILE_WRITE_MODE "w"
        #define FILEPATH_CALIBRATION_FORMAT       "calib_volt_src_%i.txt"
    #endif
    
    #ifdef ENABLE_CV_INPUT
    FLASHMEM void ADSVoltageSourceBase::load_calibration() {
        // todo: make VoltageSource know its name so that it knows where to load from
        //Debug_printf("ADSVoltageSourceBase: load_calibration for slot!\n", slot);
        if (Serial) Serial.printf("ADSVoltageSourceBase: load_calibration for slot %i!\n", global_slot);
        //int slot = parameter_manager.find_slot_for_voltage(this);

        //parameter_manager->load_voltage_calibration(slot); //, this);
        File myFile;

        char filename[255] = "";
        sprintf(filename, FILEPATH_CALIBRATION_FORMAT, global_slot); //, preset_number);
        Debug_printf("\tload_calibration() opening '%s' for global slot %i\n", filename, global_slot);
        myFile.setTimeout(0);
        myFile = STORAGE.open(filename, FILE_READ_MODE);

        if (!myFile) {
            //Debug_printf("\tError: Couldn't open '%s' for reading for slot %i!\n", filename, slot);
            if (Serial) Serial.printf("\tError: Couldn't open '%s' for reading for global slot %i!\n", filename, global_slot);
            return; // false;
        }
        String line;
        while (myFile.available()) {
            line = myFile.readStringUntil('\n');
            String key = line.substring(0, line.indexOf("="));
            String value = line.substring(line.indexOf("=")+1);
            Debug_printf("\tfor %s, found value '%s' => %6.6f\n", key.c_str(), value.c_str(), value.toFloat());
            if (key.equals("correction_value_1")) {
                this->correction_value_1 = value.toFloat();
            } else if (key.equals("correction_value_2")) {
                this->correction_value_2 = value.toFloat();
            }
        }
        myFile.close();

        Debug_printf("for slot %i, got calibration values %6.6f : %6.6f\n", global_slot, this->correction_value_1, this->correction_value_2);
    }
    FLASHMEM void ADSVoltageSourceBase::save_calibration() {
        // todo: make VoltageSource know its name so that it knows where to save to
        Debug_printf("ADSVoltageSourceBase: save_calibration for slot %i!\n", global_slot);
        //int slot = parameter_manager.find_slot_for_voltage(this);

        //parameter_manager->save_voltage_calibration(slot);

        Debug_printf("\tfor slot %i, saving calibration values %6.6f : %6.6f\n", global_slot, this->correction_value_1, this->correction_value_2);
       
        char filename[255] = "";
        snprintf(filename, 255, FILEPATH_CALIBRATION_FORMAT, global_slot); //, preset_number);
        Debug_printf("\tsave_calibration() opening %s\n", filename);

        if (STORAGE.exists(filename)) {
            Debug_println("\tfile exists - removing!");
            STORAGE.remove(filename);
        }

        File myFile = STORAGE.open(filename, FILE_WRITE_MODE /*FILE_WRITE_BEGIN*/);
        if (myFile) {
            myFile.printf("correction_value_1=%6.6f\n", this->correction_value_1);
            myFile.printf("correction_value_2=%6.6f\n", this->correction_value_2);
            myFile.close();
            Debug_printf("\tsaved!\n");
            //message_log("Saved calibration!");
        } else {
            Debug_printf("\tError saving calibration!\n");
            //message_log("Error saving calibration!");
        }
        //myFile.close();
        Debug_printf("\tEnd of save_calibration.\n");
    }
    #endif
#endif