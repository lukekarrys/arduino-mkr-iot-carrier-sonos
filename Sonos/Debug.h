#ifdef DEBUG
  #define PRINT(...) Serial.print(__VA_ARGS__)
  #define PRINTLN(...) Serial.println(__VA_ARGS__)
#else
  #define PRINT(...)
  #define PRINTLN(...)
#endif

#ifdef TRACE
  #define TRACE(...) Serial.print(__VA_ARGS__)
  #define TRACELN(...) Serial.println(__VA_ARGS__)
  #define DELAY(...) delay(__VA_ARGS__)
  #define TRACE_STATE_CHANGE(name, prev, current, change) {\
    Serial.print(name);\
    Serial.print(" STATE CHANGE: ");\
    Serial.print(static_cast<int>(prev));\
    Serial.print(" -> ");\
    Serial.print(static_cast<int>(current));\
    Serial.print(" - ");\
    Serial.print(NOW - change);\
    Serial.println("ms");\
  }
#else
  #define TRACE(...)
  #define TRACELN(...)
  #define DELAY(...)
  #define TRACE_STATE_CHANGE(...)
#endif

#ifdef PERF
  unsigned long PERF_LAST_LOOP = 0;
  unsigned long PERF_LAST_COMMAND = 0;
  unsigned long PERF_LAST_MACHINE = 0;
  unsigned long PERF_LOOP_COUNT = 0;
  #define PERF_LOOP() {\
    if (PERF_LAST_LOOP > 0) {\
      const unsigned long latest = NOW - PERF_LAST_LOOP;\
      if (latest > 16) {\
        Serial.print("LOOP PERF: ");\
        Serial.print(latest);\
        Serial.print("ms - COUNT: ");\
        Serial.println(PERF_LOOP_COUNT);\
        PERF_LOOP_COUNT = 0;\
      }\
    }\
    PERF_LOOP_COUNT++;\
    PERF_LAST_LOOP = NOW;\
  }
  #define PERF_COMMAND_START() {\
    PERF_LAST_COMMAND = NOW;\
  }
  #define PERF_COMMAND_END(msg) {\
    const unsigned long latest = NOW - PERF_LAST_COMMAND;\
    Serial.print("COMMAND PERF ");\
    Serial.print(msg);\
    Serial.print(": ");\
    Serial.println(latest);\
    PERF_LAST_COMMAND = 0;\
  }
  #define PERF_MACHINE_START() {\
    PERF_LAST_MACHINE = NOW;\
  }
  #define PERF_MACHINE_END(msg, slow) {\
    const unsigned long latest = NOW - PERF_LAST_MACHINE;\
    if (latest >= slow) {\
      Serial.print("MACHINE PERF ");\
      Serial.print(msg);\
      Serial.print(": ");\
      Serial.println(latest);\
    }\
    PERF_LAST_MACHINE = 0;\
  }
#else
  #define PERF_LOOP()
  #define PERF_COMMAND_START()
  #define PERF_COMMAND_END(...)
  #define PERF_MACHINE_START(...)
  #define PERF_MACHINE_END(...)
#endif
