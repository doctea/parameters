/*#include "AnalogParameterInput.h"

#include "ads.h"

template<class ADClass, class TargetClass, class DataType = double>
class ADSParameterInput : public AnalogParameterInput<TargetClass, DataType> {
  int inputPin;

  ADClass *ads;

  public:
    using Callback = void (*)(DataType);
    Callback callback;

    int sensitivity = 4;

    int channel = 0;

    //bool debug = true;

    //ADSParameterInput() : AnalogParameterInput() {};
    ADSParameterInput(char name, ADClass *ads) {
      this->name = name; //++NEXT_PARAMETER_NAME;
      this->ads = ads;
      this->channel = 0;
      this->max_input_value = 5.0;
    }
    ADSParameterInput(char name, ADClass *ads, int channel, float max_input_value) : ADSParameterInput(name, ads) {
      //this->name = ++NEXT_PARAMETER_NAME;
      //this->ads = ads;
      this->channel = channel;
      this->target_parameter = nullptr;
      this->max_input_value = max_input_value;
    }
    ADSParameterInput(char name, ADClass *ads, int channel, float max_input_value, TargetClass *target_parameter) : ADSParameterInput(name, ads, max_input_value, channel) {
      this->target_parameter = target_parameter;
    }

    virtual const char* getInputInfo() {
      static char input_info[20] = "                ";

      sprintf(input_info, "ADS@%p %i %s%s", this->ads, this->channel, (this->inverted?"I":""), (this->map_unipolar?"U":""));
      return input_info;
    }

    virtual void loop() override {
      this->read();
    }

    #define NUM_AVERAGE_READS 3

    DataType reads[NUM_AVERAGE_READS];

    void add_average(DataType value) {
      static byte index = 0;
      reads[index++] = value;
      if (index>=NUM_AVERAGE_READS) index = 0;
    }

    DataType get_average() {
      DataType count = 0.0;
      for (int i = 0 ; i < NUM_AVERAGE_READS ; i++) {
        count += reads[i];
      }
      return count / (DataType)NUM_AVERAGE_READS;
    }

    virtual void read() override {
      //int currentValue = analogRead(inputPin);

      // don't do anything if we haven't got a target to send the value to
      //if (this->target_parameter==nullptr && this->callback==nullptr)
      //  return;

      //int intermediate = ads->readADC(channel);
      int intermediate = ads_values[channel];
      //DataType currentValue = this->ads->toVoltage(intermediate);

      DataType currentValue = current_adc_voltage[channel];

      add_average(currentValue);
      currentValue = get_average();
      
      if (this->is_significant_change(currentValue, this->lastValue)) {
        this->currentValue = currentValue;
        if (this->debug) {
          Serial.printf("%s: ADSParameterInput->read() got intermediate %i, voltage ", this->name, intermediate);
          Serial.print((uint32_t) this->ads->toVoltage(intermediate));
          Serial.printf(", final %i", (uint32_t) currentValue*1000.0);
          Serial.println();
        }
        this->lastValue = currentValue;
        DataType normal = this->get_normal_value(currentValue);
        if (callback != nullptr) {
          if (this->debug) {
            Serial.print(this->name);
            Serial.print(F(": calling callback("));
            Serial.print(normal);
            Serial.println(F(")"));
          }      
          callback(normal);
        }
        if (this->target_parameter!=nullptr) {
          if (this->debug) {
            Serial.print(this->name);
            Serial.print(F(": calling target from normal setParamValue("));
            Serial.print(normal);
            Serial.print(F(")"));
            Serial.print(" from currentValue ");
            Serial.print(currentValue);
            if (this->inverted) Serial.print(F(" - inverted"));
            Serial.println();
          }
          //target->setParamValueA(normal);
          this->target_parameter->setParamValue(normal);
        }
        //if (Parameter_number!=0xff)
          //Parameters[Parameter_number]->setParamValueA(get_normal_value(currentValue));
        //return currentValue;
      }
    }

};*/