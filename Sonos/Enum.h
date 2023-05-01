#ifndef ENUM_UTILS_H
#define ENUM_UTILS_H

#include "Debug.h"

#define __ENUM_STATES(...)     \
  enum States { __VA_ARGS__ }; \
  static States states;

#ifdef DEBUG_OR_TRACE

// https://stackoverflow.com/questions/2124339/c-preprocessor-va-args-number-of-arguments

#define PP_NARG(...) PP_NARG_(__VA_ARGS__, PP_RSEQ_N())
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N
#define PP_RSEQ_N() 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

// https://stackoverflow.com/questions/44303189/c-macro-to-create-a-string-array

#define FIRST_(a, ...) a
#define SECOND_(a, b, ...) b

#define FIRST(...) FIRST_(__VA_ARGS__, )
#define SECOND(...) SECOND_(__VA_ARGS__, )

#define EMPTY()

#define EVAL(...) EVAL16(__VA_ARGS__)
#define EVAL16(...) EVAL8(EVAL8(__VA_ARGS__))
#define EVAL8(...) EVAL4(EVAL4(__VA_ARGS__))
#define EVAL4(...) EVAL2(EVAL2(__VA_ARGS__))
#define EVAL2(...) EVAL1(EVAL1(__VA_ARGS__))
#define EVAL1(...) __VA_ARGS__

#define DEFER1(m) m EMPTY()
#define DEFER2(m) m EMPTY EMPTY()()

#define IS_PROBE(...) SECOND(__VA_ARGS__, 0)
#define PROBE() ~, 1

#define CAT(a, b) a##b

#define NOT(x) IS_PROBE(CAT(_NOT_, x))
#define _NOT_0 PROBE()

#define BOOL(x) NOT(NOT(x))

#define IF_ELSE(condition) _IF_ELSE(BOOL(condition))
#define _IF_ELSE(condition) CAT(_IF_, condition)

#define _IF_1(...) __VA_ARGS__ _IF_1_ELSE
#define _IF_0(...) _IF_0_ELSE

#define _IF_1_ELSE(...)
#define _IF_0_ELSE(...) __VA_ARGS__

#define COMMA ,

#define HAS_ARGS(...) BOOL(FIRST(_END_OF_ARGUMENTS_ __VA_ARGS__)())
#define _END_OF_ARGUMENTS_() 0

#define MAP(m, first, ...)                                                       \
  m(first)                                                                       \
      IF_ELSE(HAS_ARGS(__VA_ARGS__))(                                            \
          COMMA DEFER2(_MAP)()(m, __VA_ARGS__))(/* Do nothing, just terminate */ \
      )
#define _MAP() MAP

#define STRINGIZE(x) #x
#define MAGIC_MACRO(...) EVAL(MAP(STRINGIZE, __VA_ARGS__))

#define ENUM_STATES(...)      \
  __ENUM_STATES(__VA_ARGS__); \
  String stateStrings[PP_NARG(__VA_ARGS__)] = {MAGIC_MACRO(__VA_ARGS__)};

#else

#define ENUM_STATES(...)      \
  __ENUM_STATES(__VA_ARGS__); \
  String stateStrings[0] = {};

#endif

#endif
