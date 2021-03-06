#ifndef BEHAVIOR_H
#define BEHAVIOR_H

#include <Servicable.h>
#include "FrontInfrared.h"
#include "VirtualBumper.h"
#include "Bumper.h"

namespace behaviors {

typedef struct {
  int8_t enabled;
  int8_t velocity;
  int8_t rotation;
} behavior_command_t;

class Behavior : public Servicable {
protected:
  behavior_command_t command;

public:
  virtual ~Behavior() {
  }
  behavior_command_t *getCommand() {
    return &command;
  }
};

class BumperBehavior : public Behavior {
private:
  typedef enum {
    Inactive,
    Reverse,
    Left,
    Right,
    Stopped,
    Forward,
  } state_t;
  
  ObstructionSensor &sensor;
  state_t state;
  state_t afterReverse;
  uint32_t changedAt;
  uint8_t justChanged;

public:
  BumperBehavior(ObstructionSensor &sensor) : sensor(sensor) {
    state = Inactive;
    afterReverse = Inactive;
    changedAt = 0;
  }

  uint8_t didJustChange() {
    return justChanged;
  }

  uint8_t didJustBump() {
    return didJustChange() && state == Reverse;
  }

  void begin() {
    sensor.begin();
  }

  void service() {
    sensor.service();
    justChanged = false;

serviceAgain:
    switch (state) {
    case Inactive:
      if (sensor.any()) {
        afterReverse = Inactive;
        if (sensor.isCenterOrBothBlocked()) {
          afterReverse = Left;
        }
        else if (sensor.isLeftBlocked()) {
          afterReverse = Right;
        }
        else if (sensor.isRightBlocked()) {
          afterReverse = Left;
        }
        if (afterReverse != Inactive) {
          state = Reverse;
          justChanged = true;
          changedAt = millis();
          printlnf("Inactive -> Reverse");
          goto serviceAgain;
        }
      }
      else {
        command.enabled = false;
      }
      break;
    case Reverse:
      if (millis() - changedAt > 2500) {
        state = afterReverse;
        justChanged = true;
        printlnf("Reverse -> %d", state);
        changedAt = millis();
        goto serviceAgain;
      }
      else {
        command.enabled = true;
        command.rotation = 0;
        command.velocity = -5;
      }
      break;
    case Left:
      if (millis() - changedAt > 1500) {
        state = Stopped;
        justChanged = true;
        printlnf("Left -> Forward");
        changedAt = millis();
        goto serviceAgain;
      }
      command.enabled = true;
      command.rotation = -6;
      command.velocity = 0;
      break;
    case Right:
      if (millis() - changedAt > 1500) {
        state = Stopped;
        justChanged = true;
        printlnf("Right -> Forward");
        changedAt = millis();
        goto serviceAgain;
      }
      command.enabled = true;
      command.rotation = 6;
      command.velocity = 0;
      break;
    case Stopped:
      if (millis() - changedAt > 1000) {
        changedAt = millis();
        state = Inactive;
        justChanged = true;
        printlnf("Stopped -> Inactive");
        goto serviceAgain;
      }
      command.enabled = true;
      command.rotation = 0;
      command.velocity = 0;
      break;
    case Forward: // Skipping for now...
      if (millis() - changedAt > 500) {
        state = Inactive;
        justChanged = true;
        printlnf("Forward -> Inactive");
        changedAt = millis();
        goto serviceAgain;
      }
      command.enabled = true;
      command.rotation = 0;
      command.velocity = 8;
      break;
    }
  }
};

class UserBehavior : public Behavior {
public:
  UserBehavior() {
    command.enabled = true;
    command.rotation = 0;
    command.velocity = 0;
  }

  void begin() {
  }

  void service() {
  }

  void toggle() {
    if (command.velocity > 0) {
      command.velocity = 0;
    }
    else {
      command.velocity = 5;
    }
  }
};

#define LMB_INTERVAL (1000 / 20)
#define LMB_DELTA     50

class LocalMinimumBehavior : public Behavior {
private:
  typedef enum {
    Inactive,
    Turning
  } state_t;
  uint16_t counter;
  uint32_t previous;
  state_t state;
  BumperBehavior &bumper;

public:
  LocalMinimumBehavior(BumperBehavior &bumper) : counter(LMB_DELTA), previous(0), state(Inactive), bumper(bumper) {
    command.enabled = true;
    command.rotation = 0;
    command.velocity = 0;
  }

  void begin() {
  }

  void service() {
    if (bumper.didJustBump()) {
      counter += LMB_DELTA;
      printlnf("LMB: Bump = %d", counter);
    }
    else if (counter > 0) {
      if (millis() - previous > LMB_INTERVAL) {
        counter--;
        if (counter == 0) {
          printlnf("LMB: Zero");
        }
        previous = millis();
      }
    }
    switch (state) {
    case Inactive:
      if (counter > LMB_DELTA * 5) {
        state = Turning;
        printlnf("LMB: Turning %d", counter);
        previous = millis();
      }
      command.enabled = false;
      command.rotation = 0;
      command.velocity = 0;
      break;
    case Turning:
      if (millis() - previous > 4000) {
        state = Inactive;
        counter = 0;
      }
      command.enabled = true;
      command.rotation = -5;
      command.velocity = 0;
      break;
    }
  }

};

}

#endif
