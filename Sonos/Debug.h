#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

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

#define GET_MACHINE_MACRO(_1, _2, _3, NAME, ...) NAME
#define DEBUG_MACHINE(...) GET_MACHINE_MACRO(__VA_ARGS__, DEBUG_OR_TRACE_MACHINE_STATE, DEBUG_MACHINE_2, DEBUG_MACHINE_1)(__VA_ARGS__)
#define TRACE_MACHINE(...) GET_MACHINE_MACRO(__VA_ARGS__, TRACE_MACHINE_STATE, TRACE_MACHINE_2, TRACE_MACHINE_1)(__VA_ARGS__)

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