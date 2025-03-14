#pragma once

class ICalibratable {
    public:
    virtual void calibrate() = 0;

    virtual void start_calibration();

    virtual void save_calibration() {}
    virtual void load_calibration() {}

    virtual bool needs_calibration() {
        return false;
    }

    // write the calibration data to the serial port
    virtual void output_calibration_data() {}
};
