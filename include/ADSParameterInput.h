#include "AnalogParameterInput.h"

#include "ads.h"

template<class ADClass, class TargetClass, class DataType = double>
class ADSParameterInput : public AnalogParameterInput<TargetClass, DataType> {
  int inputPin;
  DataType lastValue = 0;
  DataType value = 0;
  DataType minimum_value;
  DataType maximum_value;

  ADClass *ads;
  TargetClass *target_parameter;

  public:
    using Callback = void (*)(DataType);
    Callback callback;

    bool inverted = false;
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

    virtual void read() override {
      //int currentValue = analogRead(inputPin);
      int intermediate = ads->readADC(channel);
      DataType currentValue = this->ads->toVoltage(intermediate);
      Serial.printf("ADSParameterInput->read() got intermediate %i, final %i\n", intermediate, currentValue*1000.0);
      //if (abs(currentValue - this->lastValue) > sensitivity) {
      if (this->is_significant_change(currentValue, lastValue)) {
        lastValue = currentValue;
        float normal = this->get_normal_value(currentValue);
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
            Serial.print(F(": calling target setParamValue("));
            Serial.print(normal);
            Serial.print(F(")"));
            if (inverted) Serial.print(F(" - inverted"));
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