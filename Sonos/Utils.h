#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#define NOW millis()
#define MINUTE 60000
#define SLEEP 1800000
#define LOCK 300000
#define WIFI_CHECK 10000
#define SENSOR_CHECK 60000

#define HOLD 1000
#define TAP 40
#define DOUBLE_TAP 100

#define TIMEOUT 1000
#define BACKOFF(count) (1000 * pow(2, count))

#define SUCCESS 200

#define HAPPY_BEEP 800
#define SAD_BEEP 400

#define FAST_BLINK 100

#define TOP_Y 42
#define TEXT_3_STEP 24
#define TEXT_3_CHAR 9
#define TEXT_2_STEP 16
#define MARGIN 5

#define WIDTH 240
#define CENTER 120

#define MESSAGE 50
#define CONTROLS 215
#define PLAY_STATE 110

#ifdef DEBUG
#define DEBUG_OR_TRACE
#define PRINT(...) Serial.print(__VA_ARGS__)
#define PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define PRINT(...)
#define PRINTLN(...)
#endif

#ifdef DEBUG_TRACE
#define DEBUG_OR_TRACE
#define TRACE(...) Serial.print(__VA_ARGS__)
#define TRACELN(...) Serial.println(__VA_ARGS__)
#else
#define TRACE(...)
#define TRACELN(...)
#endif

#ifndef DEBUG_OR_TRACE

#define DEBUG_MACHINE(...)
#define TRACE_MACHINE(...)

#else

#define GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define DEBUG_MACHINE(...) GET_MACRO(__VA_ARGS__, DEBUG_OR_TRACE_MACHINE_STATE, DEBUG_MACHINE_2, DEBUG_MACHINE_1)(__VA_ARGS__)
#define TRACE_MACHINE(...) GET_MACRO(__VA_ARGS__, TRACE_MACHINE_STATE, TRACE_MACHINE_2, TRACE_MACHINE_1)(__VA_ARGS__)

#define TRACE_MACHINE_STATE(type, state, meta) \
  if (this->debug()) {                         \
    TRACE(this->debugName());                  \
    TRACE(" - ");                              \
    TRACE(type);                               \
    TRACE(" - ");                              \
    TRACE(this->stateString(state));           \
    if (String(meta).length()) {               \
      TRACE(" - ");                            \
      TRACE(meta);                             \
    }                                          \
    TRACELN("");                               \
  }

#define DEBUG_MACHINE_STATE(type, state, meta) \
  if (this->debug()) {                         \
    PRINT(this->debugName());                  \
    PRINT(" - ");                              \
    PRINT(type);                               \
    PRINT(" - ");                              \
    PRINT(this->stateString(state));           \
    if (String(meta).length()) {               \
      PRINT(" - ");                            \
      PRINT(meta);                             \
    }                                          \
    PRINTLN("");                               \
  }

#define DEBUG_OR_TRACE_MACHINE_STATE(type, state, meta) \
  if (this->debug()) {                                  \
    if (this->traceState(state)) {                      \
      TRACE_MACHINE_STATE(type, state, meta);           \
    } else {                                            \
      DEBUG_MACHINE_STATE(type, state, meta);           \
    }                                                   \
  }

#define TRACE_MACHINE_2(type, str) \
  if (this->debug()) {             \
    TRACE(this->debugName());      \
    TRACE(" - ");                  \
    TRACE(type);                   \
    TRACE(" - ");                  \
    TRACE(str);                    \
    TRACELN("");                   \
  }

#define DEBUG_MACHINE_2(type, str) \
  if (this->debug()) {             \
    PRINT(this->debugName());      \
    PRINT(" - ");                  \
    PRINT(type);                   \
    PRINT(" - ");                  \
    PRINT(str);                    \
    PRINTLN("");                   \
  }

#define TRACE_MACHINE_1(str)  \
  if (this->debug()) {        \
    TRACE(this->debugName()); \
    TRACE(" - ");             \
    TRACE(str);               \
    TRACELN("");              \
  }

#define DEBUG_MACHINE_1(str)  \
  if (this->debug()) {        \
    PRINT(this->debugName()); \
    PRINT(" - ");             \
    PRINT(str);               \
    PRINTLN("");              \
  }

#endif

#endif