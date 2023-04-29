#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>

class StateMachine {
 public:
  StateMachine(String debugName, String* stateStrings);
  StateMachine(String debugName, String* stateStrings, bool debug);

  virtual void reset();
  virtual int run();
  virtual void ready();

  virtual void poll(int state, unsigned long since);
  virtual void enter(int enterState, int exitState, unsigned long since);
  virtual void exit(int exitState, int enterState);

  virtual bool traceState(int state, String type);

  virtual bool debug() final;
  virtual String debugName() final;
  virtual String stateString(int stateIndex) final;

  virtual int getState() final;
  virtual int getPrevState() final;
  virtual void setState(int state) final;
  virtual unsigned long getSinceChange() final;
  virtual void resetSinceChange() final;

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

  int returnRunState = -1;
  int state = -1;
  int prevState = -1;
  unsigned long stateChange = 0;

  bool iDebug = false;
  String* iStateStrings;
  String iDebugName;
};

#endif