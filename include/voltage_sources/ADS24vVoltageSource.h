#pragma once

#ifdef ENABLE_CV_INPUT

#include "VoltageSource.h"
#include "ADSVoltageSource.h"

#if __has_include("ADS1X15.h")
    #include "ADS1X15.h"

    // for the Pimoroni +/- 24v 1015 module https://shop.pimoroni.com/products/ads1015-adc-breakout?variant=27859155026003
    // supports 1v/oct
    template<class ADS1X15Type>
    class ADS24vVoltageSource : public ADSVoltageSource<ADS1X15Type> {
        public:
            ADS24vVoltageSource(int global_slot, ADS1X15Type *ads_source, byte channel, float minimum_input_voltage = 0.0, float maximum_input_voltage = 10.0, bool supports_pitch = true, float smooth_alpha = VOLTAGE_SOURCE_DEFAULT_SMOOTHING) :
                ADSVoltageSource<ADS1X15Type>(global_slot, ads_source, channel, minimum_input_voltage, maximum_input_voltage, supports_pitch, smooth_alpha) {
                    // note this defaults to 'true' as the last argument above, because this can support 1v/oct
                    //this->debug = true;
                    this->correction_value_1 = 1182.0;
                    this->correction_value_2 = 0.041;
                    //actually getting close: 1183, 0.0400
                    //          1181, 0.0400
            }

            virtual float adcread_to_voltage(int16_t adcReading) override {
                // for some reason, seem to have to do this calculation differently (than the parent class) for this board?
                //return ads_source->toVoltage(adcReading);
                float voltageCorrected = ((float)adcReading/this->correction_value_1) - 1.0;
                /*
                // tryna implement same logic as https://github.com/pimoroni/ads1015-python/blob/9ae775bc0a4a148771ef1d36a98adb76cc6cf54e/library/ads1015/__init__.py#L375 
                float voltageCorrected = adcReading * 1000.0;
                voltageCorrected /= 2048.0;
                voltageCorrected *= this->ads_source->getGain();
                voltageCorrected /= 1000.0;
                */
                /*if (this->debug) {
                    Serial.printf("after applying correction stage 1, got ");
                    Serial.print(voltageCorrected);
                    Serial.print("\t, ");
                }*/
                return voltageCorrected;
            }

            virtual float get_corrected_voltage(float voltageFromAdc) override {
                float corrected2 = voltageFromAdc / this->correction_value_2;
                /*
                // tryna implement same logic as https://github.com/pimoroni/ads1015-python/blob/9ae775bc0a4a148771ef1d36a98adb76cc6cf54e/library/ads1015/__init__.py#L375 
                float pin_v = voltageFromAdc;
                float input_v = pin_v * ((float)8060000+402000) / (float)8060000;
                input_v += 1.241;
                float corrected2 = input_v;
                */
                /*if (this->debug) {
                    Serial.printf("after applying correction stage 1, got ");
                    Serial.print(corrected2);
                    Serial.print("\t, ");
                }*/
                return corrected2;
            }

            // The Pimoroni regression uses raw ADC integers as samples (y-variable),
            // not the adcread_to_voltage() output, because adcread_to_voltage() already
            // applies cv1: (adc/cv1) - 1.0.  We must return the uncorrupted raw count.
            // _raw_adc_sample is cached by ADSVoltageSource<T>::fetch_current_voltage()
            // before adcread_to_voltage() is called — no extra I2C read needed.
            virtual float fetch_calibration_sample() override {
                return this->_raw_adc_sample;
            }

            // Calibration sweep defaults: derived from minimum/maximum_input_voltage.
            // get_default_calib_start/end/step() inherited from ADSVoltageSourceBase.

            // Inverse of the Pimoroni two-stage correction:
            //   adcread_to_voltage(adc)  = (adc / cv1) - 1.0
            //   get_corrected_voltage(v) = v / cv2
            // Combined: V = ((raw_adc / cv1) - 1.0) / cv2
            virtual float compute_voltage_from_raw_sample(float raw_sample, float cv1, float cv2) override {
                if (cv1 == 0.0f || cv2 == 0.0f) return 0.0f;
                return ((raw_sample / cv1) - 1.0f) / cv2;
            }

            // Inverse: given V, recover raw_adc.
            // From V = ((adc/cv1) - 1) / cv2  =>  adc = cv1 * (cv2 * V + 1)
            virtual float compute_raw_from_voltage(float voltage, float cv1, float cv2) override {
                return cv1 * (cv2 * voltage + 1.0f);
            }

            // Linear regression: adc_sample = a * target_V + b
            // Then cv1 = b (intercept), cv2 = a/b (slope/intercept)
            // Uses ordinary least-squares with x=targets, y=samples.
            virtual bool compute_calibration(int n, const float *targets, const float *samples,
                                             float *out_cv1, float *out_cv2) override {
                if (n < 2) return false;
                double sum_x = 0.0, sum_y = 0.0, sum_xx = 0.0, sum_xy = 0.0;
                for (int i = 0; i < n; i++) {
                    sum_x  += targets[i];
                    sum_y  += samples[i];
                    sum_xx += (double)targets[i] * targets[i];
                    sum_xy += (double)targets[i] * samples[i];
                }
                double denom = (double)n * sum_xx - sum_x * sum_x;
                if (denom == 0.0) return false;
                double slope     = ((double)n * sum_xy - sum_x * sum_y) / denom;
                double intercept = (sum_y - slope * sum_x) / (double)n;
                if (intercept == 0.0) return false;
                *out_cv1 = (float)intercept;
                *out_cv2 = (float)(slope / intercept);
                return true;
            }

            /*#ifdef ENABLE_SCREEN
                virtual MenuItem *makeCalibrationLoadSaveControls(int i) override;
            #endif*/
    };

    #endif

#endif

