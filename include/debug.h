
//#ifndef F
    //#define F(X) X
//#endif

#ifndef Serial_flush
    #ifdef SERIAL_FLUSH_REALLY
        #define Serial_flush() if(Serial)Serial.flush()
    #else
        #define Serial_flush() {}
    #endif
#endif
#ifndef Debug_printf
    #ifdef ENABLE_DEBUG_SERIAL
        #define Debug_println(X)    if(Serial) { Serial.println(X); Serial.flush(); }
        #define Debug_printf(...)   if(Serial) { Serial.printf(__VA_ARGS__); }
        #define Debug_print(X)      if(Serial) { Serial.print(X); }
    #else
        #define Debug_println(X)    {}
        #define Debug_printf(...)     {}
        #define Debug_print(X)      {}
    #endif
#endif
