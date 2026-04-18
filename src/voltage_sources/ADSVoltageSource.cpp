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
        current_value_disp->selectable = false;
        current_value_disp->float_unit = 'v';
        submenu->add(current_value_disp);

        //Serial.println("makeCalibrationControls() returning!"); Serial_flush();   
        Debug_printf("makeCalibrationControls() returning - i is %i!\n", i); Serial_flush();   
        return submenu;
    }

#endif
