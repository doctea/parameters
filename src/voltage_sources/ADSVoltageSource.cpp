#include "voltage_sources/ADSVoltageSource.h"
#include "voltage_sources/ADS24vVoltageSource.h"

#ifdef ENABLE_SCREEN
    #include "submenuitem_bar.h"
    #include "menuitems_object.h"

    MenuItem *ADSVoltageSourceBase::makeCalibrationControls(int i) {
        Serial.println("makeCalibrationControls() for an ADS24vVoltageSource!"); Serial.flush();

        char name[20];
        sprintf(name, "Voltage Source %i Calibrator", i);
        SubMenuItemBar *submenu = new SubMenuItemBar(name);

        Serial.println("makeCalibrationControls() creating ctrl1!"); Serial.flush();
        DirectNumberControl<float> *ctrl1 = new DirectNumberControl<float> ("correction_1", &(this->correction_value_1), this->correction_value_1, 1024.0, 1200.0, nullptr);
        ctrl1->step = 0.5;
        ctrl1->float_mult = 1.0;

        Serial.println("makeCalibrationControls() creating ctrl2!"); Serial.flush();
        DirectNumberControl<float> *ctrl2 = new DirectNumberControl<float> ("correction_2", &(this->correction_value_2), this->correction_value_2, 0.020, 0.099, nullptr);
        ctrl2->step = 0.001;
        ctrl2->float_mult = 10000.0;

        Serial.println("makeCalibrationControls() adding to submenu!"); Serial.flush();   
        submenu->add(ctrl1);
        submenu->add(ctrl2);

        DirectNumberControl<double> *current_value_disp = new DirectNumberControl<double> 
            ("current", &this->current_value, this->current_value, -10.0, 10.0, nullptr);
        current_value_disp->readOnly = true;
        submenu->add(current_value_disp);

        return submenu;
    }

#endif
