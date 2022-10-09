#ifdef ENABLE_SCREEN

    class MenuItem;

    #include "voltage_sources/VoltageSource.h"
    #include "submenuitem_bar.h"

    MenuItem *VoltageSourceBase::makeCalibrationLoadSaveControls(int i) {
        Serial.println("makeCalibrationControls() for an ADS24vVoltageSource!"); Serial.flush();

        char name[20];
        sprintf(name, "%i Calibration", i);
        SubMenuItemBar *submenu = new SubMenuItemBar(name);

        //    ObjectActionConfirmItem(const char *label, TargetClass *target_object, setter_def setter, getter_def getter, const char *button_label_true, const char *button_label_false = nullptr) 

        ObjectActionConfirmItem<VoltageSourceBase> *load = new ObjectActionConfirmItem<VoltageSourceBase> (
            "Load", 
            this,
            &VoltageSourceBase::load_calibration
        );
        submenu->add(load);

        ObjectActionConfirmItem<VoltageSourceBase> *save = new ObjectActionConfirmItem<VoltageSourceBase> (
            "Save", 
            this,
            &VoltageSourceBase::save_calibration
            //(bool(VoltageSourceBase::*))nullptr,
            //(char*)nullptr
        );
        submenu->add(save);

        return submenu;
    }   

#endif