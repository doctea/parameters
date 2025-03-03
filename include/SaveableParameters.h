// todo: do we need an entire 'serialiser' load/save library?  `file_manager` branch, save load, etc..?
// todo: rename this to something else as calling it '..Parameter' risks confusing it with actual Parameters; maybe "Setting" or "Option" or something?

#pragma once

#include <Arduino.h>
//#include "scales.h"

#include <LinkedList.h>

extern const char *true_label;
extern const char *enable_label;
extern const char *warning_label;
extern const char *nop_label;

class SaveableParameterBase {
    public:
    char label[40];
    const char *category_name = nullptr;
    const char *nice_label = nullptr;

    const char *niceify(const char *label = nullptr) {
        if (this->nice_label==nullptr) {
            if (label==nullptr)
                label = this->label;
            String s = String(label);
            s.replace('_', ' ');
            //s[0] = String(s.charAt(0)).toUpperCase().charAt(0);
            s[0] = toupper(s[0]);
            String *st = new String(s);
            if (st->equals(label)) {
                this->nice_label = label;
                delete st;
            } else
                this->nice_label = st->c_str();
        }
        return this->nice_label;
    }

    SaveableParameterBase(const char *label, const char *category_name, bool *variable_recall_enabled = nullptr, bool *variable_save_enabled = nullptr) :
        //label(label), 
        category_name(category_name),
        variable_recall_enabled(variable_recall_enabled ? variable_recall_enabled : &this->recall_enabled), 
        variable_save_enabled(variable_save_enabled ? variable_save_enabled : &this->save_enabled) {
            strncpy(this->label, label, 40);
        }
        
    virtual String get_line() { return String(nop_label); }
    virtual bool parse_key_value(String key, String value) {
        return false;
    }

    bool recall_enabled = true; // for use when no pointer to variable or function is passed in
    bool save_enabled = true;   // for use when no pointer to variable or function is passed in

    bool *variable_recall_enabled = nullptr;
    bool *variable_save_enabled = nullptr;
    virtual bool is_recall_enabled() { 
        if (variable_recall_enabled==nullptr || (*variable_recall_enabled)) 
            return true; 
        return recall_enabled;
        //return false;
    }
    virtual bool is_save_enabled() { 
        if (variable_save_enabled==nullptr || (*variable_save_enabled)) 
            return true; 
        //return false;
        return save_enabled;
    }

    virtual void set_recall_enabled(bool value) {
        if (variable_recall_enabled==nullptr)
            return;
        *this->variable_recall_enabled = value;
    }
    virtual void set_save_enabled(bool value) {
        if (variable_save_enabled==nullptr)
            return;
        *this->variable_save_enabled = true;
    }
};

template<class TargetClass, class DataType>
class SaveableParameter : virtual public SaveableParameterBase {
    public:
        TargetClass *target = nullptr;
        DataType *variable = nullptr;

        void(TargetClass::*setter_func)(DataType)  = nullptr;
        DataType(TargetClass::*getter_func)()  = nullptr;
        bool(TargetClass::*is_recall_enabled_func)() = nullptr;
        bool(TargetClass::*is_save_enabled_func)() = nullptr;
        void(TargetClass::*set_recall_enabled_func)(bool state) = nullptr;
        void(TargetClass::*set_save_enabled_func)(bool state) = nullptr;

        SaveableParameter(
            const char *label, 
            const char *category_name,
            TargetClass *target, 
            DataType *variable,
            bool *variable_recall_enabled = nullptr,
            bool *variable_save_enabled = nullptr,
            void(TargetClass::*setter_func)(DataType) = nullptr,
            DataType(TargetClass::*getter_func)() = nullptr,
            bool(TargetClass::*is_recall_enabled_func)() = nullptr,
            bool(TargetClass::*is_save_enabled_func)() = nullptr,
            void(TargetClass::*set_recall_enabled_func)(bool state) = nullptr,
            void(TargetClass::*set_save_enabled_func)(bool state) = nullptr
        ) : SaveableParameterBase(label, category_name, variable_recall_enabled, variable_save_enabled), 
            target(target), 
            variable(variable), 
            setter_func(setter_func), 
            getter_func(getter_func), 
            is_recall_enabled_func(is_recall_enabled_func), 
            is_save_enabled_func(is_save_enabled_func),
            set_recall_enabled_func(set_recall_enabled_func),
            set_save_enabled_func(set_save_enabled_func) 
        {
            if (variable_recall_enabled==nullptr)
                variable_recall_enabled = &this->recall_enabled;
            if (variable_save_enabled==nullptr) {
                variable_save_enabled = &this->save_enabled;
            }
        }

        virtual bool is_recall_enabled () override {
            if (this->target!=nullptr && this->is_recall_enabled_func!=nullptr) 
                return (this->target->*is_recall_enabled_func)();
            return SaveableParameterBase::is_recall_enabled();
        }
        virtual bool is_save_enabled () override {
            if (this->target!=nullptr && this->is_save_enabled_func!=nullptr) 
                return (this->target->*is_save_enabled_func)();
            return SaveableParameterBase::is_save_enabled();
        }
        virtual void set_recall_enabled(bool state) override {
            if (this->target!=nullptr && this->set_recall_enabled_func!=nullptr)
                (this->target->*set_recall_enabled_func)(state);
            else
                SaveableParameterBase::set_recall_enabled(state);
        }
        virtual void set_save_enabled(bool state) override {
            if (this->target!=nullptr && this->set_save_enabled_func!=nullptr)
                (this->target->*set_save_enabled_func)(state);
            else
                SaveableParameterBase::set_save_enabled(state);
        }


        virtual String get_line() {
            if (this->target!=nullptr && this->getter_func!=nullptr) {
                //Serial.printf("%s#get_line has target and getter func..", this->label );
                return String(this->label) + String('=') + String((this->target->*getter_func)());
            } else if (this->variable!=nullptr) {
                //Serial.printf("%s#get_line has target variable..", this->label);
                return String(this->label) + String('=') + String(*this->variable);
            } else {
                //Serial.printf("%s#get_line has neither target nor getter func!", this->label);
                return String("; ") + String(this->label) + warning_label;
            }
        }
        virtual bool parse_key_value(String key, String value) {
            if (key.equals(this->label)) {
                this->set((DataType)0,value);
                return true;
            }
            return false;
        }
        // todo: rewrite to use constexpr?
        void setInt(String value) {
            if (setter_func!=nullptr)
                (this->target->*setter_func)(value.toInt());
            else if (variable!=nullptr)
                *this->variable = value.toInt();
        }
        /*void setInt(int value) {
            if (setter_func!=nullptr)
                (this->target->*setter_func)(value);
            else if (variable!=nullptr)
                *this->variable = value;
        }*/
        void setBool(bool value) {
            if (setter_func!=nullptr)
                (this->target->*setter_func)(value);
            else if (variable!=nullptr)
                *this->variable = value;
        }
        void setFloat(float value) {
            if (setter_func!=nullptr)
                (this->target->*setter_func)(value);
            else if (variable!=nullptr)
                *this->variable = value;
        }
        virtual void set(signed int, String value) {
            setInt(value);
        }
        virtual void set(unsigned int, String value) {
            setInt(value);
        }
        virtual void set(signed long, String value) {
            setInt(value);
        }
        virtual void set(unsigned long, String value) {
            setInt(value);
        }
        virtual void set(bool, String value) {
            setBool(value.equals("1") || value.equals(true_label) || value.equals(enable_label));
        }
        virtual void set(float, String value) {
            this->setFloat(value.toFloat());
        }
        /*virtual void set(SCALE, String value) {
            setInt(value);
        }*/
};

#include "functional-vlpp.h"

template<class DataType>
class LSaveableParameter : virtual public SaveableParameterBase {
    public:
        DataType *variable = nullptr;

        using setter_func_def = vl::Func<void(DataType)>;
        using getter_func_def = vl::Func<DataType(void)>;

        setter_func_def setter_func = [this](DataType v) -> void { 
            if (this->variable!=nullptr) 
                *this->variable = v; 
        };
        getter_func_def getter_func = [this]() -> DataType { 
            if (this->variable!=nullptr) 
                return *this->variable; 
            return (DataType) 0; 
        };
        
        LSaveableParameter(
            const char *label,
            const char *category_name,
            DataType *variable
        ) : SaveableParameterBase(label, category_name), variable(variable) 
        {}

        LSaveableParameter(
            const char *label,
            const char *category_name,
            DataType *variable,
            setter_func_def setter_func
        ) : LSaveableParameter(label, category_name, variable) {
            this->setter_func = setter_func;
        }

        LSaveableParameter(
            const char *label, 
            const char *category_name,
            DataType *variable,
            setter_func_def setter_func,
            getter_func_def getter_func
        ) : LSaveableParameter(label, category_name, variable, setter_func)
        {
            this->getter_func = getter_func;
        }

        /*virtual bool is_recall_enabled () override {
            return this->is_recall_enabled_func();
        }
        virtual bool is_save_enabled () override {
            return this->is_save_enabled_func();
        }
        virtual void set_recall_enabled(bool state) override {
            this->set_recall_enabled_func(state);
        }
        virtual void set_save_enabled(bool state) override {
            this->set_save_enabled_func(state);
        }*/

        virtual DataType get() {
            return this->getter_func();
        }

        virtual String get_line() {
            return String(this->label) + String('=') + String(this->getter_func());
        }
        virtual bool parse_key_value(String key, String value) {
            if (key.equals(this->label)) {
                this->set((DataType)0,value);
                return true;
            }
            return false;
        }
        // todo: rewrite to use constexpr?
        void setInt(String value) {
            this->setter_func(value.toInt());
        }
        void setBool(bool value) {
            this->setter_func(value);
        }
        void setFloat(float value) {
            this->setter_func(value);
        }
        virtual void set(signed int, String value) {
            setInt(value);
        }
        virtual void set(unsigned int, String value) {
            setInt(value);
        }
        virtual void set(signed long, String value) {
            setInt(value);
        }
        virtual void set(unsigned long, String value) {
            setInt(value);
        }
        virtual void set(bool, String value) {
            setBool(value.equals("1") || value.equals(true_label) || value.equals(enable_label));
        }
        virtual void set(float, String value) {
            this->setFloat(value.toFloat());
        }
        /*virtual void set(SCALE, String value) {
            setInt(value);
        }*/
};


/*
// this untested, since wrote it then realised that SaveableParameters are only currently used by Sequences, not Projects
template<class TargetClass, class DataType=BaseParameterInput>
class ParameterInputSaveableParameter : public SaveableParameter<TargetClass,DataType> {
    public:    
        virtual String get_line() {
            DataType *source = nullptr;
            if (this->target!=nullptr && this->getter_func!=nullptr) {
                //Serial.printf("%s#get_line has target and getter func..", this->label );
                source = (this->target->*getter_func)();
            } else if (this->variable!=nullptr) {
                //Serial.printf("%s#get_line has target variable..", this->label);
                source = this->variable;
            } else {
                //Serial.printf("%s#get_line has neither target nor getter func!", this->label);
                return String("; ") + String(this->label) + warning_label;
            }
            return String(this->label) + String('=') + (source!=nullptr?source->name:"none");
        }
        virtual bool parse_key_value(String key, String value) {
            if (key.equals(this->label)) {
                //this->set((DataType)0,value);
                DataType *source = parameter_manager->getInputForName((char*)value.c_str());
                return true;
            }
            return false;
        }
};
*/

#ifdef ENABLE_SCREEN
    #include "menuitems_object_multitoggle.h"
    class SaveableParameterOptionToggle : virtual public MultiToggleItemClass<SaveableParameterBase> {
        SaveableParameterBase *target = nullptr;
        public:
            SaveableParameterOptionToggle(SaveableParameterBase *target) : MultiToggleItemClass(target->niceify(), target, &SaveableParameterBase::set_recall_enabled, &SaveableParameterBase::is_recall_enabled)
            {}
    };
#endif

/*
#include "parameters/Parameter.h"
class SaveableParameterWrapper : public SaveableParameterBase {
    FloatParameter *target = nullptr;
    SaveableParameterWrapper(FloatParameter *target) : SaveableParameterBase(target->label) {
        this->target = target;
    }
    virtual String get_line() {
        return String(this->target->label) + String("=") + String(this->target->getCurrentNormalValue());
    }
    virtual bool parse_key_value(String key, String value) {
        if (key.equals(this->target->label)) {
            this->target->updateValueFromNormal(value.toFloat());
            //this->target->upd
        }
    }
};
*/



// todo: really, this should be part of the 'file_manager' branch stuff...?
// and so should implement IParseKeyValueReceiver and ISaveKeyValueSource and donate boilerplate functions to handle that
class ISaveableParameterHost {
    public:

    LinkedList<SaveableParameterBase*> *saveable_parameters = nullptr;
    virtual void setup_saveable_parameters() {
        if (this->saveable_parameters==nullptr) {
            //Debug_println("instantiating saveable_parameters list");
            this->saveable_parameters = new LinkedList<SaveableParameterBase*> ();
        }
    }

    virtual bool load_parse_key_value_saveable_parameters(String key, String value) {
        if (saveable_parameters==nullptr) {
            Debug_printf("WARNING: load_parse_key_value_saveable_parameters(%s,%s) called, but saveable_parameters isn't initialised yet!\n", key.c_str(), value.c_str());
            return false;
        }
        for (uint_fast8_t i = 0 ; i < saveable_parameters->size() ; i++) {
            if (!saveable_parameters->get(i)->is_recall_enabled())
                continue;
            if (saveable_parameters->get(i)->parse_key_value(key, value))
                return true;
        }
        return false;
    }
    virtual void add_save_lines_saveable_parameters(LinkedList<String> *lines) {
        for (uint_fast8_t i = 0 ; i < saveable_parameters->size() ; i++) {
            Debug_printf("ISaveableParameterHost#save_sequence_add_lines_saveable_parameters() processing %i aka '%s'..\n", i, saveable_parameters->get(i)->label);
            if (saveable_parameters->get(i)->is_save_enabled()) {
                Debug_printf("\t\tis_save_enabled() returned true, getting line!");
                lines->add(saveable_parameters->get(i)->get_line());
            }
        }
    }

};
