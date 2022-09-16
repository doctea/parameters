#include "Parameter.h"

//template<class DataType = double>
class FunctionParameter : public DoubleParameter {
    public:
    //using Callback = void(*)(DataType);
    //Callback *callback = nullptr;
    void (*callback)(double) = nullptr;
    FunctionParameter(char *label, void(*callback)(double)) : DoubleParameter(label) {
        this->callback = callback;
    }

    virtual void setParamValue(double value, double range = 1.0) {
        if (this->callback!=nullptr)
            (*this->callback) (value);
    };
};