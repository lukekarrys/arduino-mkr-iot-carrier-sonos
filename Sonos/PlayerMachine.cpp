#include "PlayerMachine.h"

#include "Carrier.h"
#include "Consts.h"

#define ROOMS_WAIT 2000
#define RESULT_WAIT 1000

#ifndef DEBUG_PLAYER
#define DEBUG_PLAYER false
#endif

PlayerMachine::PlayerMachine(DisplayMachine *aDisplayMachine, LedMachine *aLedsMachine, const char *server, uint16_t port)
    : StateMachine("PLAYER", stateStrings, DEBUG_PLAYER),
      displayMachine(aDisplayMachine),
      ledsMachine(aLedsMachine),
      sonosMachine(server, port),
      commandMachine(server, port),
      led0Machine(0),
      led1Machine(1),
      led2Machine(2),
      led3Machine(3),
      led4Machine(4),
      button0Machine(TOUCH0),
      button1Machine(TOUCH1),
      button2Machine(TOUCH2),
      button3Machine(TOUCH3),
      button4Machine(TOUCH4) {
  ledMachines[0] = &led0Machine;
  ledMachines[1] = &led1Machine;
  ledMachines[2] = &led2Machine;
  ledMachines[3] = &led3Machine;
  ledMachines[4] = &led4Machine;
}

PlayerMachine::PlayerMachine(DisplayMachine *aDisplayMachine, LedMachine *aLedsMachine, const char *server, uint16_t port, String aInitialRoom)
    : StateMachine("PLAYER", stateStrings, DEBUG_PLAYER),
      displayMachine(aDisplayMachine),
      ledsMachine(aLedsMachine),
      sonosMachine(server, port, aInitialRoom),
      commandMachine(server, port),
      led0Machine(0),
      led1Machine(1),
      led2Machine(2),
      led3Machine(3),
      led4Machine(4),
      button0Machine(TOUCH0),
      button1Machine(TOUCH1),
      button2Machine(TOUCH2),
      button3Machine(TOUCH3),
      button4Machine(TOUCH4) {
  ledMachines[0] = &led0Machine;
  ledMachines[1] = &led1Machine;
  ledMachines[2] = &led2Machine;
  ledMachines[3] = &led3Machine;
  ledMachines[4] = &led4Machine;
}

void PlayerMachine::reset() {
  StateMachine::reset();
  errorReason = "";
  this->resetAction();
  this->resetButtons();
  this->resetLeds();
  sonosMachine.reset();
  commandMachine.reset();
}

void PlayerMachine::resetAction() {
  if (actionButton != -1) {
    ledMachines[actionButton]->off();
  }
  actionMessage = "";
  actionButton = -1;
  actionError = false;
}

void PlayerMachine::resetButtons() {
  button0Machine.reset();
  button1Machine.reset();
  button2Machine.reset();
  button3Machine.reset();
  button4Machine.reset();
}

void PlayerMachine::resetLeds() {
  led0Machine.reset();
  led1Machine.reset();
  led2Machine.reset();
  led3Machine.reset();
  led4Machine.reset();
}

bool PlayerMachine::isConnected() {
  const int connectedState = this->getState();
  return connectedState != Error && connectedState != Connect;
}

bool PlayerMachine::isSleepy() {
  return this->getSinceChange() > SLEEP && sonosMachine.getSinceChange() > SLEEP && commandMachine.getSinceChange() > SLEEP;
}

void PlayerMachine::ready() {
  this->setState(Connect);
}

void PlayerMachine::readyButtons() {
  button0Machine.ready();
  button1Machine.ready();
  button2Machine.ready();
  button3Machine.ready();
  button4Machine.ready();
}

void PlayerMachine::readyLeds() {
  ledsMachine->off();
  led0Machine.ready();
  led1Machine.ready();
  led2Machine.ready();
  led3Machine.ready();
  led4Machine.ready();
}

void PlayerMachine::run() {
  StateMachine::run();

  button0Machine.run();
  button1Machine.run();
  button2Machine.run();
  button3Machine.run();
  button4Machine.run();

  led0Machine.run();
  led1Machine.run();
  led2Machine.run();
  led3Machine.run();
  led4Machine.run();

  commandMachine.run();
  sonosMachine.run();
}

void PlayerMachine::exit(int state, int enterState) {
  switch (state) {
    case Connected:
      displayMachine->player();
      this->readyLeds();
      this->readyButtons();
      break;

    case Locked:
      displayMachine->drawPlayerControls(ST77XX_WHITE);
      commandMachine.resetSinceChange();
      break;
  }
}

void PlayerMachine::enter(int state, int exitState, unsigned long sinceChange) {
  switch (state) {
    case Connect:
      sonosMachine.ready();
      commandMachine.reset();
      ledsMachine->blink(LedYellow, FAST_BLINK);
      displayMachine->sonos("Sonos:", "Connecting to room:");
      this->resetButtons();
      break;

    case Connected:
      ledsMachine->on(LedYellow);
      commandMachine.ready(sonosMachine.getRoom());
      for (int i = 0; i < sonosMachine.roomSize; i++) {
        displayMachine->printLine((i == sonosMachine.roomIndex ? "* " : "  ") + sonosMachine.rooms[i]);
      }
      break;

    case Ready:
      this->resetAction();
      displayMachine->setPlayerAction(sonosMachine.getRoom());
      break;

    case Action:
      DEBUG_MACHINE("ACTION", actionMessage);
      displayMachine->setPlayerAction(actionMessage);
      break;

    case Result:
      DEBUG_MACHINE("RESULT", String(actionButton) + " / " + String(actionError));
      if (actionError) {
        Buzzer(SAD_BEEP, 20);
      }
      commandMachine.ready();
      displayMachine->setPlayerAction(actionMessage, actionError ? ST77XX_RED : ST77XX_GREEN);
      this->resetAction();
      break;

    case Locked:
      displayMachine->setPlayerAction(sonosMachine.getRoom());
      displayMachine->drawPlayerControls(ST77XX_RED);
      break;
  }
}

void PlayerMachine::poll(int state, unsigned long sinceChange) {
  if (this->isConnected() != sonosMachine.isConnected()) {
    this->setState(this->isConnected() ? Connect : Connected);
    return;
  }

  if (sonosMachine.getState() == SonosMachine::Error) {
    errorReason = sonosMachine.errorReason;
    this->setState(Error);
    return;
  }

  if (sonosMachine.getState() == SonosMachine::Update) {
    displayMachine->setPlayer(
        sonosMachine.title,
        sonosMachine.artist,
        sonosMachine.album,
        sonosMachine.volume,
        sonosMachine.mute,
        sonosMachine.repeat,
        sonosMachine.shuffle,
        sonosMachine.playing);
  }

#ifdef AUTO_LOCK
  if (this->isConnected() && state != Locked && commandMachine.getSinceChange() > LOCK) {
    this->setState(Locked);
    return;
  }
#endif

  switch (state) {
    case Connected:
      if (sinceChange > ROOMS_WAIT) {
        this->setState(Ready);
      }
      break;

    case Ready:
      this->handleButtons();
      this->handleCommand();
      break;

    case Action:
      this->handleCommand();
      if (sinceChange > TIMEOUT) {
        actionError = true;
        this->setState(Result);
      }
      break;

    case Result:
      this->handleButtons();
      this->handleCommand();
      if (commandMachine.getSinceChange() > RESULT_WAIT) {
        this->setState(Ready);
      }
      break;

    case Locked:
      this->handleButtons();
      if (sinceChange > FAST_BLINK && actionButton != -1) {
        this->resetAction();
      }
      break;
  }
}

void PlayerMachine::handleCommand() {
  switch (commandMachine.getState()) {
    case CommandMachine::Connect:
      this->setState(Action);
      break;

    case CommandMachine::Success:
      this->setState(Result);
      break;

    case CommandMachine::Error:
      actionError = true;
      this->setState(Result);
      break;
  }
}

void PlayerMachine::handleButtons() {
  if (actionButton != -1) {
    return;
  }
  if (button4Machine.getState() == ButtonMachine::TapHold) {
    actionButton = 4;
    this->setState(this->getState() == Locked ? Ready : Locked);
  } else if (this->getState() != Locked) {
    if (this->button0(button0Machine.getState())) {
      actionButton = 0;
    } else if (this->button1(button1Machine.getState())) {
      actionButton = 1;
    } else if (this->button2(button2Machine.getState())) {
      actionButton = 2;
    } else if (this->button3(button3Machine.getState())) {
      actionButton = 3;
    } else if (this->button4(button4Machine.getState())) {
      actionButton = 4;
    }
  }
  if (actionButton != -1) {
    DEBUG_MACHINE("HANDLE_BUTTON", actionButton);
    Buzzer(HAPPY_BEEP, 20);
    ledMachines[actionButton]->on(LedBlue);
  }
}

bool PlayerMachine::button0(int action) {
  // XXX: do not use TapHold here. That is captured by AppMachine and thus will
  // never run here.
  switch (action) {
    case ButtonMachine::Tap:
      commandMachine.sendRequest("previous");
      actionMessage = "Prev";
      break;

    case ButtonMachine::DoubleTap:
      commandMachine.sendRequest("trackseek/1");
      actionMessage = "Start";
      break;

    case ButtonMachine::Hold:
      sonosMachine.prevRoom();
      this->ready();
      break;

    default:
      return false;
  }

  return true;
}

bool PlayerMachine::button1(int action) {
  switch (action) {
    case ButtonMachine::Tap:
      commandMachine.sendRequest("volume/-1");
      actionMessage = "Vol -1";
      break;

    case ButtonMachine::DoubleTap:
      commandMachine.sendRequest("volume/-5");
      actionMessage = "Vol -5";
      break;

    case ButtonMachine::Hold:
      commandMachine.sendRequest("togglemute");
      actionMessage = sonosMachine.mute ? "Unmute" : "Mute";
      break;

    default:
      return false;
  }

  return true;
}

bool PlayerMachine::button2(int action) {
  String repeat = sonosMachine.repeat;
  switch (action) {
    case ButtonMachine::Tap:
      commandMachine.sendRequest("playpause");
      actionMessage = sonosMachine.playing ? "Pause" : "Play";
      break;

    case ButtonMachine::DoubleTap:
      commandMachine.sendRequest("shuffle/toggle");
      actionMessage = sonosMachine.shuffle ? "Shuf Off" : "Shuf On";
      break;

    case ButtonMachine::Hold:
      commandMachine.sendRequest("repeat/toggle");
      actionMessage = "Rep " + String(repeat == "all" ? "One" : repeat == "one" ? "None"
                                                                                : "All");
      break;

    default:
      return false;
  }

  return true;
}

bool PlayerMachine::button3(int action) {
  switch (action) {
    case ButtonMachine::Tap:
      commandMachine.sendRequest("volume/+1");
      actionMessage = "Vol +1";
      break;

    case ButtonMachine::DoubleTap:
      commandMachine.sendRequest("volume/+5");
      actionMessage = "Vol +5";
      break;

    case ButtonMachine::Hold:
      commandMachine.sendRequest("togglemute");
      actionMessage = sonosMachine.mute ? "Unmute" : "Mute";
      break;

    default:
      return false;
  }

  return true;
}

bool PlayerMachine::button4(int action) {
  // XXX: do not use TapHold here. That is captured by the lock condition in
  // handleButtons and thus will never run here.
  switch (action) {
    case ButtonMachine::Tap:
      commandMachine.sendRequest("next");
      actionMessage = "Next";
      break;

    case ButtonMachine::Hold:
      sonosMachine.nextRoom();
      this->ready();
      break;

    default:
      return false;
  }

  return true;
}
