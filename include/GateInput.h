char NEXT_GATE_INPUT_NAME = 'M';

class GateInput {
  protected:
    using Callback = void (*)();

    Callback gate_on_callback = NULL ;
    Callback gate_off_callback = NULL ;
    Envelope *target;
    
    bool last_read_state = false;
    bool triggerState = false;      // the current state of the output pin
    bool buttonState = false;             // the current reading from the input pin
    bool lastButtonState = LOW;
    
    bool triggered = false;
    
    unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
    unsigned long debounceDelay = 5;    // the debounce time; increase if the output flickers   

    bool debug = false;

  public:
    char name;
    int inputPin;
    
    GateInput() {
      name = NEXT_GATE_INPUT_NAME++;
      Serial.print("Instantiated GateInput named ");
      Serial.print(name);
    }
    GateInput(int in_inputPin, Envelope &in_target) 
      : GateInput () {
      inputPin = in_inputPin;
      pinMode(inputPin, INPUT);

      target = &in_target;
      Serial.print("GateInput instantiated with name ");
      Serial.print(name);
      Serial.print(" and pin ");
      Serial.print(inputPin);
      Serial.print(", passed envelope ");
      Serial.println((uint32_t) target);
      //gate_on_callback = in_gate_on_callback;
      //gate_off_callback = in_gate_off_callback;
    }
    GateInput(int in_inputPin, Callback in_gate_on_callback, Callback in_gate_off_callback) 
      : GateInput () {
      inputPin = in_inputPin;
      pinMode(inputPin, INPUT);
      gate_on_callback = in_gate_on_callback;
      gate_off_callback = in_gate_off_callback;
      Serial.print("GateInput instantiated with name ");
      Serial.print(name);
      Serial.print(" and pin ");
      Serial.print(inputPin);
      Serial.println(", passed callbacks ");
    }

    void setDebug() {
      debug = !debug;
    }

    // read current value directly
    virtual bool read_value(){
      return false;
    };

    void loop() {
      /*if (debug) {
        Serial.print(name);
        Serial.print(": GateInput loop() called ");
        Serial.println((uint32_t) this);
      }*/
      /*if (reading) {
        Serial.println("high?");
      }*/
      bool reading = read_value();
    
      bool changed = false;
    
      // If the switch changed, due to noise or pressing:
      if (reading != last_read_state) {
        // reset the debouncing timer
        lastDebounceTime = millis();
      }
    
      last_read_state = reading;
    
      if ((millis() - lastDebounceTime) > debounceDelay) {
        // whatever the reading is at, it's been there for longer than the debounce
        // delay, so take it as the actual current state:
    
        // if the button state has changed:
        if (reading != buttonState) {
          //Serial.print("changed to ");
          //Serial.println(reading);
          buttonState = reading;
          changed = true;
    
          // only toggle the LED if the new button state is HIGH
          if (buttonState == true) {
            triggered = true;
          } else {
            triggered = false;
          }   
        }
      }
    
      if (changed) {
        if (debug) {
          Serial.print(name);
          Serial.println(F(": GateInput changed!"));
        }
        if (triggered) {
          //Serial.println("==== Gate start");
          if (gate_on_callback != NULL) {
            if (debug) {
              Serial.print(name);
              Serial.println(F(": calling gate_on_callback"));
            }
            gate_on_callback();
          }
          if (target) {
            if (debug) {
              Serial.print(name);
              Serial.print(F(": calling gate_on against "));
              Serial.println((uint32_t) &target);
            }
            target->gate_on();
          }
          //envelopes[1]->gate_on();
        } else {
          //Serial.println("==== Gate stop");
          if (gate_off_callback != NULL) {
            if (debug) {
              Serial.print(name);
              Serial.println(F(": calling gate_off_callback"));
            }
            gate_off_callback();
          }
          if (target) {
            if (debug) {
              Serial.print(name);
              Serial.print(F(": calling gate_off against "));
              Serial.println((uint32_t) &target);
            }
            target->gate_off();
          }
        }
      }
    }
};

class AnalogGateInput : public GateInput {

  public:
    int trigger_level = 768;

    AnalogGateInput(int in_inputPin, Envelope &target)
      : GateInput{in_inputPin, target} {
      Serial.print(F("AnalogGateInput instantiated, passed envelope "));
      Serial.println((uint32_t) &target);
    }
    AnalogGateInput(int in_inputPin, Callback in_gate_on_callback, Callback in_gate_off_callback) 
      : GateInput (in_inputPin, in_gate_on_callback, in_gate_off_callback) 
      {}
    
    bool read_value() {
      int areading = analogRead(inputPin); //>=768;
      if (debug) {
          Serial.print(name);
          Serial.print(F(": areading is "));
          Serial.println(areading);
      }      
      int reading = areading >= trigger_level;
      return reading;    
  }

};

class DigitalGateInput : public GateInput {

  public:
    DigitalGateInput(int in_inputPin, Envelope &target)
      : GateInput{in_inputPin, target } {}
    DigitalGateInput(int in_inputPin, Callback in_gate_on_callback, Callback in_gate_off_callback) 
      : GateInput (in_inputPin, in_gate_on_callback, in_gate_off_callback) 
      {}
    
    bool read_value() {
      int value = digitalRead(inputPin);
      if (debug) {
        Serial.print(name);
        Serial.print(F(": read_value from pin "));
        Serial.print(inputPin);
        Serial.print(F(" is "));
        Serial.println(value);
      }
      return value;
    }
};
