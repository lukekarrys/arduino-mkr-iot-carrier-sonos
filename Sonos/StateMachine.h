#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>

#include "Debug.h"

class StateMachine {
 public:
  StateMachine(String debugName, String* stateStrings, bool debug);

  virtual void reset();
  virtual void run();
  virtual void ready();

  virtual int getState() final;
  virtual unsigned long getSinceChange() final;
  virtual void resetSinceChange() final;

 protected:
  virtual void poll(int state, unsigned long since);
  virtual void enter(int enterState, int exitState, unsigned long since);
  virtual void exit(int exitState, int enterState);

#ifdef DEBUG_OR_TRACE
  virtual bool debug() final;
  virtual String debugName() final;
  virtual String stateString(int stateIndex) final;
  virtual bool traceState(int state);
#endif

  virtual void setState(int state) final;

 private:
  bool runPoll();
  bool runEnter(int initialState);
  bool runExit();
  void nextState(int inititalState);

  enum RunStates {
    Polling,
    Entered,
    Exited
  };
  RunStates runState = Polling;

  int state = -1;
  int prevState = -1;
  unsigned long stateChange = 0;

  bool iDebug = false;
  String* iStateStrings;
  String iDebugName;
};

#endif