#include "AnalogParameterInput.h"

#include "ads.h"

template<class ADClass, class TargetClass, class DataType = double>
class ADSParameterInput : public AnalogParameterInput<TargetClass, DataType> {
  int inputPin;
  /*DataType lastValue = 0;
  DataType currentValue = 0;
  DataType minimum_value;
  DataType maximum_value;*/

  ADClass *ads;
  //TargetClass *target_parameter;

  public:
    using Callback = void (*)(DataType);
    Callback callback;

    int sensitivity = 4;

    int channel = 0;

    //bool debug = true;

    ADSParameterInput() {};
    ADSParameterInput(ADClass *ads) {
      this->ads = ads;
      this->channel = 0;
    }
    ADSParameterInput(ADClass *ads, int channel) {
      this->ads = ads;
      this->channel = channel;
      this->target_parameter = nullptr;
      this->max_input_value = 5.0;
    }
    ADSParameterInput(ADClass *ads, int channel, TargetClass *target_parameter) {
      this->ads = ads;
      this->channel = channel;
      this->target_parameter = target_parameter;
      this->max_input_value = 5.0;
    }

    virtual const char* getInputInfo() {
      static char input_info[20] = "                ";

      sprintf(input_info, "ADS@%p %i %s%s", this->ads, this->channel, (this->inverted?"I":""), (this->map_unipolar?"U":""));
      return input_info;
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

    /*void setTarget(TargetClass *target_parameter) {
      this->target_parameter = target_parameter;
    }*/

    virtual void loop() override {
      this->read();
    }

    /*virtual DataType get_normal_value(double voltage_value) {
      if (this->inverted) {
        //Serial.print("in ADSParameterInput#get_normal_value(): Inverting ");
        //Serial.print(voltage_value);
        //Serial.print(" to ");
        //Serial.println(5.0f - ((float)voltage_value / 5.0f));
        return 1.0f - (voltage_value / 5.0f);
      } else
        return voltage_value / 5.0f;
    }*/

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
      if (this->target_parameter==nullptr && this->callback==nullptr)
        return;

      int intermediate = ads->readADC(channel);
      DataType currentValue = this->ads->toVoltage(intermediate);

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

};