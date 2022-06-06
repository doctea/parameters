#include "AnalogParameterInput.h"

#include "ads.h"

template<class ADClass, class TargetClass, class DataType = double>
class ADSParameterInput : public AnalogParameterInput<TargetClass, DataType> {
  int inputPin;
  DataType lastValue = 0;
  DataType currentValue = 0;
  DataType minimum_value;
  DataType maximum_value;

  ADClass *ads;
  TargetClass *target_parameter;

  public:
    using Callback = void (*)(DataType);
    Callback callback;

    int sensitivity = 4;

    int channel = 0;

    ADSParameterInput() {};
    ADSParameterInput(ADClass *ads) {
      this->ads = ads;
      this->channel = 0;
    }
    ADSParameterInput(ADClass *ads, int channel, TargetClass *target_parameter) {
      this->ads = ads;
      this->channel = channel;
      this->target_parameter = target_parameter;
    }

    /*ADSParameterInput(ADClass *ads, int channel, TargetClass *target_parameter) { //}, TargetClass *target_parameter) { //TargetType *in_target, DataType minimum_value, DataType maximum_value, int in_sensitivity = 3) {
        this->channel = channel;
        this->ads = ads;
        //callback = in_callback;
        this->sensitivity = in_sensitivity;
        //pinMode(inputPin, INPUT);
        //this->minimum_value = minimum_value;
        //this->maximum_value = maximum_value;
        this->target_parameter = target_parameter;
    }*/

    void setTarget(TargetClass *target_parameter) {
      this->target_parameter = target_parameter;
    }

    virtual void loop() override {
      this->read();
    }

    virtual DataType get_normal_value(double voltage_value) {
      if (this->inverted) {
        /*Serial.print("in ADSParameterInput#get_normal_value(): Inverting ");
        Serial.print(voltage_value);
        Serial.print(" to ");
        Serial.println(5.0f - ((float)voltage_value / 5.0f));*/
        return 1.0f - (voltage_value / 5.0f);
      } else
        return voltage_value / 5.0f;
    }

    virtual void read() override {
      //int currentValue = analogRead(inputPin);
      int intermediate = ads->readADC(channel);
      DataType currentValue = this->ads->toVoltage(intermediate);
      if (this->debug) {
        Serial.printf("ADSParameterInput->read() got intermediate %i, voltage ", intermediate);
        Serial.print(this->ads->toVoltage(intermediate));
        Serial.printf(", final %i", currentValue*1000.0);
        Serial.println();
      }
      
      if (this->is_significant_change(currentValue, lastValue)) {
        lastValue = currentValue;
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
          target_parameter->setParamValue(normal);
        }
        //if (Parameter_number!=0xff)
          //Parameters[Parameter_number]->setParamValueA(get_normal_value(currentValue));
        //return currentValue;
      }
    }

};