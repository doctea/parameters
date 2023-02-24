#include "Parameter.h"

//template<class DataType = float>
class FunctionParameter : public FloatParameter {
    public:
    //using Callback = void(*)(DataType);
    //Callback *callback = nullptr;
    void (*callback)(float) = nullptr;
    FunctionParameter(char *label, void(*callback)(float)) : FloatParameter(label) {
        this->callback = callback;
    }

    virtual void setParamValue(float value, float range = 1.0) {
        if (this->callback!=nullptr)
            (*this->callback) (value);
    };
};