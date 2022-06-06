#include <Arduino.h>

// an object that can be targeted by a ParameterInput, calls setter method on final target object
template<class TargetClass, class DataType = double>
class Parameter {
    private:
        DataType current_value;
        DataType last_value;

        TargetClass *target;
        void(TargetClass::*setter_func)(DataType value);// setter_func;

        /*void (test::*func)(DataType);
        void (test::*func)(float);
        void (test::*func)(int);*/

    public:
        bool debug = false;

        Parameter(TargetClass *target, void(TargetClass::*setter_func)(DataType)) {
            this->target = target;
            this->setter_func = setter_func;
        }

        void setParamValue(DataType value) {
            last_value = current_value;
            current_value = value;
            //this->func(value);
            (target->*setter_func)(value);
        }
        /*void setParamValue(float value) {
            last_value = current_value;
            current_value = value;
            this->func(value);
        }
        void setParamValue(int value) {
            last_value = current_value;
            current_value = value;
            this->func(value);
        }*/
        /*Targetable(TargetClass *target, TargetClass::*setter_func) {
            this->target = target;
            this->setter_func = setter_func;
        }*/

        DataType getCurrentValue() {
            return current_value;
        }
        DataType getLastValue() {
            return last_value;
        }

        void set_target_object(TargetClass *target) {
            this->target = target;
        }
        void set_target_func(void(TargetClass::*fp)(DataType)) {
            this->setter_func = fp;
        }
};

