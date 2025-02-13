/*****************************************************************

  Some debug macros for Arduino codes..

  put in '#define DEBUG' in your main code 
  before including this code.

******************************************************************/
#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef DEBUG

  // #include <Serial.h>

  #ifndef Debug
  #define Debug(...)          Serial.print(__VA_ARGS__)
  #endif

  #ifndef Debugln
  #define Debugln(...)        Serial.println(__VA_ARGS__)
  #endif

  #ifndef DebugBegin
  #define DebugBegin(...)     Serial.begin(__VA_ARGS__)
  #endif 

  #ifndef DebugFlush
  #define DebugFlush()        Serial.flush()
  #endif

#else 

  #define Debug(...)          
  #define Debugln(...)        
  #define DebugBegin(...)  
  #define DebugFlush()      
  
#endif /* Debug */

#endif /* Include Guard */