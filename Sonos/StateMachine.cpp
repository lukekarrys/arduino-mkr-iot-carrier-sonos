#include "StateMachine.h"

#include <Arduino.h>

#include "Utils.h"

StateMachine::StateMachine(String aDebugName, String* aStateStrings, bool aDebug)
    : iDebugName(aDebugName), iStateStrings(aStateStrings), iDebug(aDebug) {
}

StateMachine::StateMachine(String aDebugName, String* aStateStrings)
    : iDebugName(aDebugName), iStateStrings(aStateStrings) {
}

void StateMachine::reset() {
  state = -1;
  prevState = -1;
  this->resetSinceChange();
  DEBUG_MACHINE("RESET");
}

void StateMachine::poll(int state, unsigned long since) {}
void StateMachine::enter(int enterState, int exitState, unsigned long sinceChange) {}
void StateMachine::exit(int exitState, int enterState) {}
void StateMachine::ready() {}

bool StateMachine::debug() {
#ifdef DEBUG_ALL
  return true;
#else
  return iDebug;
#endif
}

bool StateMachine::traceState(int state, String type) {
  return type != "ENTER";
}

String StateMachine::debugName() {
  return iDebugName;
}

String StateMachine::stateString(int index) {
  return iStateStrings[index];
}

void StateMachine::setState(int newState) {
  TRACE_MACHINE("SET_STATE", newState, "");
  state = newState;
}

int StateMachine::getState() {
  return (state);
}

unsigned long StateMachine::getSinceChange() {
  return NOW - stateChange;
}

void StateMachine::resetSinceChange() {
  stateChange = NOW;
}

void StateMachine::nextState(int inititalState) {
  runState = Polling;
  this->resetSinceChange();
  prevState = inititalState;
  DEBUG_MACHINE("NEXT_STATE", prevState, "");
}

void StateMachine::run() {
  const int initialState = state;

  switch (runState) {
    case Polling:
      this->runExit() || this->runEnter(initialState) || this->runPoll();
      break;

    case Exited:
      this->runEnter(initialState) || this->runPoll();
      break;

    case Entered:
      this->nextState(state);
      runPoll();
      break;
  }
}

bool StateMachine::runExit() {
  if (prevState == -1 || prevState == state) {
    return (false);
  }

  DEBUG_MACHINE("EXIT", prevState, "");

  this->exit(prevState, state);

  runState = Exited;

  return (true);
}

bool StateMachine::runEnter(int initialState) {
  if (state == -1 || prevState == state) {
    return (false);
  }

  DEBUG_MACHINE("RUN_STATE", runState == 0 ? "Polling" : runState == 1 ? "Entered"
                                                                       : "Exited");
  DEBUG_MACHINE("ENTER", state, String(this->getSinceChange()) + "ms");

  this->enter(state, prevState, this->getSinceChange());

  if (initialState != -1 && initialState != state) {
    DEBUG_MACHINE("ENTER_EXIT_INITIAL", initialState, "");
    DEBUG_MACHINE("ENTER_EXIT_STATE", state, "");
    this->nextState(initialState);
  } else {
    runState = Entered;
  }

  return (true);
}

bool StateMachine::runPoll() {
  runState = Polling;

  if (state == -1) {
    return (false);
  }

  this->poll(state, this->getSinceChange());

  return (true);
}
