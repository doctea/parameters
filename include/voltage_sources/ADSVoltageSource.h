#pragma once

#ifdef ENABLE_CV_INPUT

#include "debug.h"

#include "VoltageSource.h"
#ifdef ENABLE_STORAGE
    #include "saveload_settings.h"
#endif
#if __has_include("ADS1X15.h")
    #include "ADS1X15.h"

#ifdef ENABLE_SCREEN
    //#include "menuitems.h"
    class MenuItem;
    class Menu;
#endif

class ADSVoltageSourceBase : public VoltageSourceBase
    #ifdef ENABLE_STORAGE
        , virtual public SHDynamic<0, 2>
    #endif
{
    public:
        float correction_value_1 = 0.976937;
        float correction_value_2 = 0.0123321;

        ADSVoltageSourceBase(int global_slot, float maximum_input_voltage = 5.0, bool supports_pitch = false) 
            : VoltageSourceBase(global_slot, maximum_input_voltage, supports_pitch) {
            #ifdef ENABLE_STORAGE
                this->set_path_segment_fmt("volt_src_%i", global_slot);
            #endif
        }
        ADSVoltageSourceBase(int global_slot, bool supports_pitch = false) 
            : VoltageSourceBase(global_slot, supports_pitch) {
            #ifdef ENABLE_STORAGE
                this->set_path_segment_fmt("volt_src_%i", global_slot);
            #endif
        }

        #if defined(ENABLE_SCREEN)
            FLASHMEM virtual MenuItem *makeCalibrationControls(int i) override;
            //virtual MenuItem *makeCalibrationLoadSaveControls(int i) override;
        #endif

        #ifdef ENABLE_STORAGE
            void setup_saveable_settings() override {
                register_setting(new VarSetting<float>(
                    "correction_value_1", "Calibration", &this->correction_value_1
                ), SL_SCOPE_SYSTEM);
                register_setting(new VarSetting<float>(
                    "correction_value_2", "Calibration", &this->correction_value_2
                ), SL_SCOPE_SYSTEM);
            }
            ISaveableSettingHost* as_saveable_host() override { return this; }
        #endif

        virtual bool needs_calibration() override {
            return true;
        }

        // No-RTTI cast helper: returns this as ADSVoltageSourceBase*
        virtual ADSVoltageSourceBase* as_ads_source() override { return this; }

        virtual void output_calibration_data() override {
            Serial.printf("ADSVoltageSource calibration data for slot %i: correction_value_1=%6.6f : correction_value_2=%6.6f\n", global_slot, this->correction_value_1, this->correction_value_2);
            Serial.printf("correction_value_1=%6.6f\n", this->correction_value_1);
            Serial.printf("correction_value_2=%6.6f\n", this->correction_value_2);
        }

        // Returns an intermediate (pre-correction) sample value for calibration regression.
        // Default fallback: return the current corrected voltage (subclasses override).
        virtual float fetch_calibration_sample() {
            return this->fetch_current_voltage();
        }

        // Given n (target_voltage, sample) pairs, compute new cv1/cv2 without applying them.
        // Returns true on success (requires at least 2 valid points).
        // Subclasses override to implement their specific calibration model.
        virtual bool compute_calibration(int n, const float *targets, const float *samples,
                                         float *out_cv1, float *out_cv2) {
            (void)n; (void)targets; (void)samples; (void)out_cv1; (void)out_cv2;
            return false;
        }

        // Default calibration sweep range.  Subclasses override to suit their input range.
        virtual float get_default_calib_start() const { return 0.0f; }
        virtual float get_default_calib_end()   const { return 5.0f; }
        virtual float get_default_calib_step()  const { return 1.0f; }

        // Convert a raw calibration sample back to a voltage using the given cv1/cv2.
        // Default implements the linear model: V = cv1 * raw_sample + cv2.
        // ADS24vVoltageSource overrides with the Pimoroni formula.
        virtual float compute_voltage_from_raw_sample(float raw_sample, float cv1, float cv2) {
            return cv1 * raw_sample + cv2;
        }

        // Inverse of compute_voltage_from_raw_sample: given a corrected voltage, recover the
        // raw calibration sample that would produce it under the given cv1/cv2.
        // Default implements the linear inverse: raw = (V - cv2) / cv1.
        // ADS24vVoltageSource overrides with the Pimoroni inverse.
        virtual float compute_raw_from_voltage(float voltage, float cv1, float cv2) {
            if (cv1 == 0.0f) return 0.0f;
            return (voltage - cv2) / cv1;
        }

        // Returns the physical ADC channel number this source reads from.
        virtual uint8_t get_adc_channel() const { return 0; }
};

// for generic 1115 ADC modules with 5v range
template<class ADS1X15Type>
class ADSVoltageSource : public ADSVoltageSourceBase {
    public:
        byte channel = 0;
        ADS1X15Type *ads_source;

        ADSVoltageSource(int global_slot, ADS1X15Type *ads_source, byte channel, float maximum_input_voltage = 5.0, bool supports_pitch = false) 
            : ADSVoltageSourceBase(global_slot, maximum_input_voltage, supports_pitch) {
            this->ads_source = ads_source;
            this->channel = channel;
        }

        // ask the ADC for its current voltage
        /*virtual float fetch_current_voltage() {
            //int16_t value = ads_source->readADC(channel);
            int16_t value1 = ads_source->readADC(channel);
            int16_t value2 = ads_source->readADC(channel);
            int16_t value3 = ads_source->readADC(channel);

            int value = (value1+value2+value3) / 3;

            float voltageFromAdc = ads_source->toVoltage(value);
            if ((int)voltageFromAdc==ADS1X15_INVALID_VOLTAGE)
                return 0.0;
            
            return this->get_corrected_voltage(voltageFromAdc);
        }*/
        bool already_succeeded = false;
        virtual float fetch_current_voltage() override {
            if (this->debug) {
                Debug_println(F("in ADSVoltageSource#fetch_current_voltage().."));
                Debug_printf(F("\tads_source is @%p, reading from channel %i\n"), this->ads_source, this->channel);
            }            
            if (!already_succeeded) 
                Debug_printf(F("ADSVoltageSource#fetch_current_voltage reading from channel %i, check you're using correct address ADC board if crash here!\n"), this->channel);

            if (!ads_source->isConnected()) {
                if (this->debug) {
                    Serial.printf("ADSVoltageSource#fetch_current_voltage(): underlying ADC isn't connected!\n");
                }
                return 0.0f;
            }

            #ifndef FAST_VOLTAGE_READS
                // do three readings from ADC and average them
                int16_t value1 = ads_source->readADC(channel);
                int16_t value2 = ads_source->readADC(channel);
                int16_t value3 = ads_source->readADC(channel);

                int adcReading = (value1+value2+value3) / 3;
            #else
                int adcReading = ads_source->readADC(channel);
            #endif

            if (!already_succeeded) 
                Debug_printf(F("ADSVoltageSource#fetch_current_voltage didn't crash on first read, so address is probably ok!\n"), this->channel);
            already_succeeded = true;

            if (this->debug) {
                Debug_printf(F("ADSVoltageSource channel %i read ADC voltageFromAdc %i\t :"), channel, adcReading); Serial_flush();
            }

            float voltageFromAdc = this->adcread_to_voltage(adcReading);

            float voltageCorrected = this->get_corrected_voltage(voltageFromAdc);

            if (this->debug) {
                Debug_print(F(" after correction stage 2 got "));
                Debug_println(voltageCorrected);
            }

            if (this->debug) Debug_printf(F("in ADSVoltageSource#fetch_current_voltage() finishing (and returning %f)\n"), voltageCorrected);

            return voltageCorrected;
        }

        virtual float adcread_to_voltage(int16_t adcReading) {
            float voltageFromAdc = ads_source->toVoltage(adcReading);
            if ((int)voltageFromAdc==ADS1X15_INVALID_VOLTAGE)
                return 0.0;
            return voltageFromAdc;
        }

        virtual uint8_t get_adc_channel() const override { return this->channel; }

        // correct for non-linearity
        virtual float get_corrected_voltage(float voltageFromAdc) {
            // TODO: what is the maths behind this?  make configurable, etc 
            // from empirical measuring of received voltage and asked wolfram alpha to figure it out:-
            //  1v: v=1008        = 0.99206349206
            //  2v: v=2031-2034   = 0.98473658296
            //  3v: v=3060        = 0.98039215686
            //  4v: v=4086-4089   = 0.9789525208
            // https://www.wolframalpha.com/input?i=linear+fit+%7B%7B1.008%2C1%7D%2C+%7B2.034%2C2%7D%2C+%7B3.063%2C3%7D%2C+%7B4.086%2C4%7D%2C+%7B5.1%2C5%7D%7D
            return (voltageFromAdc * correction_value_1) + correction_value_2;
        };

        // Returns pre-correction library voltage: adcread_to_voltage(avg of 3 reads).
        // This is the x-variable for the ADSVoltageSource calibration model:
        //   V_corrected = cv1 * adcread_to_voltage(adc) + cv2
        virtual float fetch_calibration_sample() override {
            if (!ads_source->isConnected()) return 0.0f;
            int16_t v1 = ads_source->readADC(this->channel);
            int16_t v2 = ads_source->readADC(this->channel);
            int16_t v3 = ads_source->readADC(this->channel);
            return this->adcread_to_voltage((int16_t)((v1 + v2 + v3) / 3));
        }

        // Linear regression: target = cv1 * sample + cv2
        // (sample = adcread_to_voltage result, target = known voltage)
        // Uses ordinary least-squares with x=samples, y=targets.
        virtual bool compute_calibration(int n, const float *targets, const float *samples,
                                         float *out_cv1, float *out_cv2) override {
            if (n < 2) return false;
            double sum_x = 0.0, sum_y = 0.0, sum_xx = 0.0, sum_xy = 0.0;
            for (int i = 0; i < n; i++) {
                sum_x  += samples[i];
                sum_y  += targets[i];
                sum_xx += (double)samples[i] * samples[i];
                sum_xy += (double)samples[i] * targets[i];
            }
            double denom = (double)n * sum_xx - sum_x * sum_x;
            if (denom == 0.0) return false;
            double slope     = ((double)n * sum_xy - sum_x * sum_y) / denom;
            double intercept = (sum_y - slope * sum_x) / (double)n;
            *out_cv1 = (float)slope;
            *out_cv2 = (float)intercept;
            return true;
        }

};

#endif

#endif
