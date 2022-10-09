#ifdef ENABLE_SCREEN

    class MenuItem;

    #include "voltage_sources/VoltageSource.h"
    #include "submenuitem_bar.h"

    FLASHMEM MenuItem *VoltageSourceBase::makeCalibrationLoadSaveControls(int i) {
        Serial.println(F("makeCalibrationLoadSaveControls() in VoltageSourceBase!")); Serial.flush();
        //Serial.printf(F("\tpassed i=%i!\n"), i);

        char name[MENU_C_MAX];
        sprintf(name, "Recall Voltage Source %i", i+1);
        //Serial.printf(F("Creating submenu control for '%s'..\n"), name); Serial.flush();
        SubMenuItemBar *submenu = new SubMenuItemBar(name);
        
        //Serial.printf(F("Creating load control for '%s'..\n"), name); Serial.flush();
        ObjectActionConfirmItem<VoltageSourceBase> *load = new ObjectActionConfirmItem<VoltageSourceBase> (
            "Load", 
            this,
            &VoltageSourceBase::load_calibration
        );
        submenu->add(load);

        //Serial.printf(F("Creating save control for '%s'\n"), name);Serial.flush();
        ObjectActionConfirmItem<VoltageSourceBase> *save = new ObjectActionConfirmItem<VoltageSourceBase> (
            "Save", 
            this,
            &VoltageSourceBase::save_calibration
        );
        submenu->add(save);

        //Serial.println(F("makeCalibrationLoadSaveControls() returning!")); Serial.flush();
        return submenu;
    }   

#endif