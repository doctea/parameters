#ifndef Serial_flush
    #ifdef SERIAL_FLUSH_REALLY
        #define Serial_flush() Serial.flush()
    #else
        #define Serial_flush() {}
    #endif
#endif
