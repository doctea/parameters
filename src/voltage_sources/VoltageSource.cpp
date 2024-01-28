#ifdef ENABLE_SCREEN

    class MenuItem;

    #include "voltage_sources/VoltageSource.h"
    #include "submenuitem_bar.h"
    #include "menuitems_lambda.h"
    #include "functional-vlpp.h"

    #if defined(ENABLE_CALIBRATION_STORAGE)
        FLASHMEM 
        MenuItem *VoltageSourceBase::makeCalibrationLoadSaveControls(int i) {
            if (Serial) Serial.println(F("makeCalibrationLoadSaveControls() in VoltageSourceBase!")); Serial_flush();
            //Serial.printf(F("\tpassed i=%i!\n"), i);

            char name[MENU_C_MAX];
            snprintf(name, MENU_C_MAX, "Recall Voltage Source %i", i+1);
            //Serial.printf(F("Creating submenu control for '%s'..\n"), name); Serial_flush();
            SubMenuItemBar *submenu = new SubMenuItemBar(name);
            
            //Serial.printf(F("Creating load control for '%s'..\n"), name); Serial_flush();
            /*ObjectActionConfirmItem<VoltageSourceBase> *load = new ObjectActionConfirmItem<VoltageSourceBase> (
                "Load", 
                this,
                &VoltageSourceBase::load_calibration
            );*/
            LambdaActionConfirmItem *load = new LambdaActionConfirmItem("Load", [=]() -> void { this->load_calibration(); });
            //load->show_header = false;
            submenu->add(load);

            //Serial.printf(F("Creating save control for '%s'\n"), name);Serial_flush();
            /*ObjectActionConfirmItem<VoltageSourceBase> *save = new ObjectActionConfirmItem<VoltageSourceBase> (
                "Save", 
                this,
                &VoltageSourceBase::save_calibration
            );*/
            LambdaActionConfirmItem *save = new LambdaActionConfirmItem("Save", [=]() -> void { this->save_calibration(); });
            //save->show_header = false;
            submenu->add(save);

            submenu->show_header = false;
            submenu->show_sub_headers = false;

            //Serial.println(F("makeCalibrationLoadSaveControls() returning!")); Serial_flush();
            return submenu;
        }   
    #endif

#endif