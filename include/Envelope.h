// Weirdolope - a minimal one-control envelope generator
// (c) 2022 Russell Borogove
// Use at your own risk

// This envelope generator uses floating point math; 
// if your MCU doesn't have floating-point hardware 
// support you're gonna have a bad time.

// Call updateEnvelope() periodically at a rate >= 1000 Hz

// Set envelopeState to ENVELOPE_STATE_ATTACK to 
// begin the envelope and ENVELOPE_STATE_RELEASE to 
// release it.

// envelopeLevel will range from 0.0 to 1.0. 

// Implement setEnvelope() to scale that value 
// appropriately and send it to your DAC. 

// Thanks to "synthesizers, yo" user meem for 
// getting me some nice clean samples of guitars 
// and pianos that I could measure some 
// representative envelope time curves from. 

// Now modified quite a bit by Tristan Rowley with full thanks and royalties ;-) to Russell Borogove for the basis!
//  Seems to work OK on an Arduino Nano

/// NOTE THIS IS NOT USED / NOT WORKING AT PRESENT
#ifdef ENABLE_OFFCUTS

char NEXT_ENVELOPE_NAME = 'A';

class Envelope : public Targetable {
  // attack rate, amplitude change per millisec
  using SetterCallback = void (*)(float,bool);
  using StateChangeCallback = void (*)(int, int);

private:
  float AttackRateTable[9] = 
  {
      1.00000f,     // instant
      0.08333f,     // guitar
      0.04000f,     // bass
      0.01667f,     // piano bass
      0.10000f,     // piano treble
      0.20000f,     // chiff organ
      0.10000f,     // synth lead
      0.00250f,     // synth pad
      0.00050f,     // long ambient pad
  };
  
  float DecayRateTable[9] = 
  {  
      0.96555f,     // instant
      0.99929f,     // guitar
      0.98851f,     // bass
      0.99934f,     // piano bass
      0.98746f,     // piano treble
      0.98000f,     // chiff organ
      0.99644f,     // synth lead
      0.99978f,     // synth pad
      0.99988f,     // long ambient pad   
  };
  
  // End-of-decay level
  float SustainLevelTable[9] = 
  {
      0.70f,     // instant
      0.72f,     // guitar
      0.75f,     // bass
      0.70f,     // piano bass
      0.70f,     // piano treble
      0.70f,     // chiff organ
      0.70f,     // synth lead
      0.80f,     // synth pad
      0.90f,     // long ambient pad   
  };
  
  // Unlike a typical synth ADSR envelope, we use an 
  // exponentially decaying sustain. 
  float SustainRateTable[9] = 
  {
      0.99880f,     // instant
      0.99928f,     // guitar
      0.99957f,     // bass
      0.99913f,     // piano bass
      0.99015f,     // piano treble
      0.99977f,     // chiff organ
      0.99991f,     // synth lead
      1.0f,     // synth pad
      1.0f,     // long ambient pad   
  };
  
  // Unlike a typical synth ADSR envelope, we use an 
  // exponentially decaying sustain
  float ReleaseRateTable[9] = 
  {
      0.99312f,     // instant
      0.99421f,     // guitar
      0.99484f,     // bass
      0.99092f,     // piano bass
      0.97727f,     // piano treble
      0.99500f,     // chiff organ
      0.99500f,     // synth lead
      0.99650f,     // synth pad
      0.99800f,     // long ambient pad   
  };
 
  int EnvA = 0;
  int EnvB = 1;
  float EnvAlpha = 0.0f;

  long lastUpdatedClock = 0l;
    
  float envelopeLevel = 0.0f;
  // silence the envelope when it reaches this level, well below 12 bit dac resolution.
  float envelopeStopLevel = 0.0001f;
  unsigned long nextEnvelopeUpdate = 0;

  float base_level;

  float paramValueA = 0.0f;
  
  float lerp( float y0, float y1, float alpha )
  {
      return (y1-y0)*alpha + y0;
  }

  // callback handlers
  SetterCallback setterCallback;
  StateChangeCallback stateChangeCallback;

  bool inverted = false;
  //float idleSlew = false;
  float slewRate = 0.0f;

  unsigned long stageStartedAt;
  float stageStartLevel = 0.0f;

public:

  char name;

  Envelope() {
    name = NEXT_ENVELOPE_NAME++;
    Serial.print("Instantiated envelope ");
    Serial.println(name);
  }

  static const int ENVELOPE_STATE_IDLE = 0;
  //static const int ENVELOPE_STATE_SLEW_TO_STOP = 1;
  static const int ENVELOPE_STATE_ATTACK =  1;
  static const int ENVELOPE_STATE_DECAY =   2;
  static const int ENVELOPE_STATE_SUSTAIN = 3;
  static const int ENVELOPE_STATE_RELEASE = 4;
  static const int ENVELOPE_STATE_INVERTED_RELEASE = 5;

  int envelopeState = ENVELOPE_STATE_IDLE;

  void begin() {
    envelopeLevel = 0.0f;
    envelopeState = ENVELOPE_STATE_IDLE;
    lastUpdatedClock = 0;
    //Serial.print(name);
    //Serial.println(": Envelope.begin()");
    setEnvelope(0.0f);
    //Serial.println("Finished Envelope.begin()");
  }

  bool debug = false;
  void setDebug() {
    debug = !debug;
  }

  void gate_on() {
    changeState(ENVELOPE_STATE_ATTACK);
  }
  void gate_off() {
    changeState(ENVELOPE_STATE_RELEASE);
  }
  void stop() {
    changeState(ENVELOPE_STATE_IDLE);
  }
  void invert_release() {
    changeState(ENVELOPE_STATE_INVERTED_RELEASE);
  }
  /*void slew_to_stop() {
    changeState(ENVELOPE_STATE_SLEW_TO_STOP);
  }*/
  void registerSetterCallback(SetterCallback c) {
    setterCallback = c;
  }
  void registerStateChangeCallback(StateChangeCallback c) {
    stateChangeCallback = c;
  }
  
  void setInverted(bool in_inverted = true) {
    inverted = in_inverted;
  }
  /*void setIdleSlew(float in_slew = true) {
    Serial.print("Setting idleSlew to ");
    Serial.println(in_slew);
    idleSlew = in_slew;
  }*/
  void setSlewRate(float in_slew = true) {
    slewRate = in_slew;
  }

  void setParamValueA(float paramValue) {
    if (debug) {
      Serial.print(name);
      Serial.print(F(": Setting paramvalue to "));
      Serial.println(paramValue);
    } 
    paramValueA = paramValue;
  }

  void changeState(int new_state) {
    if (envelopeState != new_state) {
      stageStartedAt = bpm_clock();
      stageStartLevel = envelopeLevel;
      if (stateChangeCallback)
        stateChangeCallback(envelopeState, new_state);
    }
    envelopeState = new_state;
  }

  void setBaseLevel(float norm_value) {
    base_level = norm_value;
  }
  
  void setEnvelope(float envelopeLevel, bool force = false)
  {
    // unipolar
    //cvValues[ CV_CHANNEL_ENVELOPE ] = envelopeLevel;
    //Serial.print("setEnvelope passed ");
    //Serial.println(envelopeLevel);

    envelopeLevel += base_level;
    
    if (envelopeLevel>envelopeStopLevel && inverted)
      envelopeLevel = 1.0-envelopeLevel;
      
    //if (envelopeState==ENVELOPE_STATE_IDLE)
    //  envelopeLevel = 0.0;

    if (setterCallback!=NULL) {
      /*if (debug) {
        Serial.print(name);
        Serial.print(F(": calling setterCallback for value "));
        Serial.println(envelopeLevel);
      }*/
      setterCallback(envelopeLevel, force);
    } else if (debug) {
      Serial.print(name);
      Serial.println(F(": setterCallback is NULL?!"));
    }
  }
  
  void updateEnvelope() //int envelopeControl)
  {
      //int envelopeControl = analogRead( PIN_ENVELOPE );
  
      //float x = constrain( 8.0f * (float)(envelopeControl-10) / 1003.1f, 0.0f, 7.999f );
      float x = constrain( 8.0f * paramValueA, 0.0f, 7.999f );
      EnvA = int(x);
      EnvB = EnvA+1;
      EnvAlpha = constrain( x - EnvA, 0.0f, 1.0f );
  
      unsigned long long ttg = bpm_clock() - lastUpdatedClock; //micros();

      float delta = 0.0f;
      float damp = 1.0f;
      float sustainLevel = 0.7f;
      if (ttg > effective_TIME_BETWEEN_UPDATES) {
          //nextEnvelopeUpdate += 1000;
          nextEnvelopeUpdate += 1000; //1000;

          //switch (envelopeState)
          if (envelopeState==ENVELOPE_STATE_IDLE) {
              if (envelopeLevel>envelopeStopLevel && slewRate>0.01) {
                if (debug) {
                  Serial.print(name);
                  Serial.print(F(": stageStartLevel is "));
                  Serial.print(stageStartLevel);
                  Serial.print(F(", with slewRate at "));
                  Serial.print(slewRate);
                  Serial.print(F(" @ time elapsed "));
                  Serial.print((unsigned long)(bpm_clock()-stageStartedAt));
                  Serial.print(F(" after some maths = "));
                  Serial.print(constrain((bpm_clock()-stageStartedAt)/slewRate, 0.0f, 1.0f));
                }
                
                envelopeLevel = lerp(
                  //inverted ? 1.0f-stageStartLevel : stageStartLevel, 
                  stageStartLevel,
                  0.0f, 
                  constrain((bpm_clock()-stageStartedAt)/slewRate, 0.0f, 1.0f)
                );
                if (inverted) envelopeLevel = 1.0 - envelopeLevel;
                if (debug) {
                  Serial.print(F(" gives level "));
                  Serial.println(envelopeLevel);
                }
              } else {
                /*if(debug) {
                  Serial.print(name);
                  Serial.println(F(": ENVELOPE_STATE_IDLE"));
                }*/
                envelopeLevel = 0.0f;
              }
          } else if (envelopeState==ENVELOPE_STATE_ATTACK) {
              if(debug) {
                Serial.print(name);
                Serial.println(F(": ENVELOPE_STATE_ATTACK"));
              }
              delta = lerp( AttackRateTable[EnvA], AttackRateTable[EnvB], EnvAlpha );
              envelopeLevel += delta;
              if (envelopeLevel >= 1.0f)
              {
                  envelopeLevel = 1.0f;
                  changeState(ENVELOPE_STATE_DECAY);
              }
          } else if (envelopeState==ENVELOPE_STATE_DECAY) {
              if(debug) {
                Serial.print(name);
                Serial.println(F(": ENVELOPE_STATE_DECAY"));
              }
              damp = lerp( DecayRateTable[EnvA], DecayRateTable[EnvB], EnvAlpha );
              envelopeLevel *= damp;
              sustainLevel = lerp(SustainLevelTable[EnvA],SustainLevelTable[EnvB],EnvAlpha);
              if (envelopeLevel <= sustainLevel)
              {
                  changeState(ENVELOPE_STATE_SUSTAIN);
              }
          } else if (envelopeState==ENVELOPE_STATE_SUSTAIN) {
              if(debug) {
                Serial.print(name);
                Serial.println(F(": ENVELOPE_STATE_SUSTAIN"));
              }
              damp = lerp( SustainRateTable[EnvA], SustainRateTable[EnvB], EnvAlpha );
              envelopeLevel *= damp;
              if (envelopeLevel <= envelopeStopLevel)
              {
                  changeState(ENVELOPE_STATE_IDLE);
              }
          }  else if (envelopeState==ENVELOPE_STATE_RELEASE) {
              if(debug) {
                Serial.print(name);
                Serial.println(F(": ENVELOPE_STATE_RELEASE"));
              }
              damp = lerp( ReleaseRateTable[EnvA], ReleaseRateTable[EnvB], EnvAlpha );
              envelopeLevel *= damp;
              if (envelopeLevel <= envelopeStopLevel)
              {
                  changeState(ENVELOPE_STATE_IDLE);
              }
          } else if (envelopeState==ENVELOPE_STATE_INVERTED_RELEASE) { 
              // TODO: this doesn't really do what we actually want it to
              if(debug) Serial.println("ENVELOPE_STATE_INVERTED_RELEASE");
              //damp = lerp( ReleaseRateTable[EnvA], ReleaseRateTable[EnvB], EnvAlpha );
              //envelopeLevel *= damp;
              envelopeLevel = constrain(envelopeLevel*slewRate, 0.0f, 1.0f);
              if (envelopeLevel <= envelopeStopLevel || envelopeLevel>=0.9999f)
              {
                  changeState(ENVELOPE_STATE_IDLE);
              }
              //if (inverted) envelopeLevel = 1.0f - envelopeLevel;
          } else  {
              if(debug) {
                Serial.print(name);
                Serial.println(F(": UNKNOWN ENVELOPE STATE?"));
              }
          }
          //Serial.print("envelopeLevel = ");
          //Serial.println(envelopeLevel);


          lastUpdatedClock = bpm_clock();
  
          setEnvelope(envelopeLevel);
          //return;
      } else if (debug) {
        Serial.print(name);
        Serial.print(F(": ttg >0 (is "));
        Serial.print((unsigned long)ttg);
        Serial.print(F(") and bpm_clock is "));
        Serial.println((unsigned long)bpm_clock());
        //Serial.print(F(" and nextEnvelopeUpdate is"));
        //Serial.println(nextEnvelopeUpdate);
      }
  }
};

#endif