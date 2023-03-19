int ir_input = 2;
int faucet_control_pos = 4;
int faucet_control_neg = 5;

int faucet_delay = 500;
int approach_delay = 5000;
int sleep_threshold = 5*60*1000; // 5 minutes
int sleep_backoff = 10*60*1000; // 10 minutes
int check_interval = 1000;

enum FaucetAction {
  TURN_ON,
  TURN_OFF
};

void SetFaucet(FaucetAction action) {
  if (action == TURN_ON) {
    digitalWrite(faucet_control_pos, HIGH);
    digitalWrite(faucet_control_neg, LOW);
  } else {
    digitalWrite(faucet_control_pos, LOW);
    digitalWrite(faucet_control_neg, HIGH);
  }
  delay(faucet_delay);
  digitalWrite(faucet_control_pos, LOW);
  digitalWrite(faucet_control_neg, LOW);
}

// organize as a state machine
enum State {
  INIT,
  APPROACH, // cat approaching
  DRINKING, // drinking
  SLEEPING, // cat is sleeping nearby
  LEAVING,  // cat leaving
  OFF
};

class StateBase {
protected:
  unsigned long enter_time;
  State this_state;
public:
  virtual State DecideTransit(bool ir_on) {
    // derived state should implement this
    // the input is the ir state, output is next state
  }

  virtual void OnEnter() {
    // derived state should implement this
    // called when entering this state
  }

  virtual void OnStay() {
    // derived state should implement this
    // this is called every loop
    // called when staying in this state
  }

  void Enter(State state) {
    this_state = state;
    enter_time = millis();
    OnEnter();
  }

  void Stay() {
    OnStay();
  }

  unsigned long GetEnterTime() {
    return enter_time;
  }

  State GetState() {
    return this_state;
  }
};

// init state
// transitions:
//  if ir on: APPROACH
//  if ir off: OFF
class InitState : public StateBase {
public:
  void OnEnter() {
    SetFaucet(TURN_OFF);
  }

  State DecideTransit(bool ir_on) {
    if (ir_on) {
      return APPROACH;
    } else {
      return OFF;
    }
  }
};

// approach state
// transitions:
//  if ir on and started for approach_delay:
//    switch to DRINKING
//  if ir off:
//    turn off faucet
//    switch to LEAVING
class ApproachState : public StateBase {
public:
  State DecideTransit(bool ir_on) {
    if (ir_on) {
      if (millis() - GetEnterTime() > approach_delay)
        return DRINKING;
      else
        return APPROACH;
    } else {
      return LEAVING;
    }
  }
};

// drinking state
// transitions:
//  if ir on:
//    if started for less than SLEEP_THRESHOLD: DRINKING
//    if started for more than SLEEP_THRESHOLD: SLEEPING
//  if ir off: LEAVING
class DrinkingState : public StateBase {
public:
  State DecideTransit(bool ir_on) {
    if (ir_on) {
      if (millis() - GetEnterTime() > sleep_threshold) {
        return SLEEPING;
      } else {
        return DRINKING;
      }
    } else {
      return LEAVING;
    }
  }

  void OnEnter() {
    SetFaucet(TURN_ON);
  }
};

class SleepingState : public StateBase {
public:
  State DecideTransit(bool ir_on) {
    if (ir_on) {
      if (millis() - GetEnterTime() > sleep_threshold) {
        return APPROACH;
      } else {
        return SLEEPING;
      }
    } else {
      return LEAVING;
    }
  }

  void OnEnter() {
    SetFaucet(TURN_OFF);
  }
};

class LeavingState : public StateBase {
public:
  State DecideTransit(bool ir_on) {
    if (ir_on) {
      return APPROACH;
    } else {
      if (millis() - GetEnterTime() > approach_delay) {
        return OFF;
      } else {
        return LEAVING;
      }
    }
  }
};

class OffState : public StateBase {
public:
  State DecideTransit(bool ir_on) {
    if (ir_on) {
      return APPROACH;
    } else {
      return OFF;
    }
  }

  void OnEnter() {
    SetFaucet(TURN_OFF);
  }
};

StateBase* gstate = nullptr;
void Transit() {
  State next_state = gstate->DecideTransit(digitalRead(ir_input) == HIGH);
  switch(next_state) {
    case INIT:
      Serial.println("INIT");
      break;
    case APPROACH:
      Serial.println("APPROACH");
      break;
    case DRINKING:
      Serial.println("DRINKING");
      break;
    case SLEEPING:
      Serial.println("SLEEPING");
      break;
    case LEAVING:
      Serial.println("LEAVING");
      break;
    case OFF:
      Serial.println("OFF");
      break;
  }
  if (next_state == gstate->GetState()) {
    gstate->Stay();
    return;
  }
  // otherwise, transit
  delete gstate;
  switch(next_state) {
    case INIT:
      gstate = new InitState();
      break;
    case APPROACH:
      gstate = new ApproachState();
      break;
    case DRINKING:
      gstate = new DrinkingState();
      break;
    case SLEEPING:
      gstate = new SleepingState();
      break;
    case LEAVING:
      gstate = new LeavingState();
      break;
    case OFF:
      gstate = new OffState();
      break; 
  };
  gstate->Enter(next_state);
}

void setup() {
  // put your setup code here, to run once:
  pinMode(ir_input, INPUT_PULLUP);
  pinMode(faucet_control_pos, OUTPUT);
  pinMode(faucet_control_neg, OUTPUT);
  SetFaucet(TURN_OFF);
  gstate = new InitState();
}

void loop() {
  // put your main code here, to run repeatedly:
  Transit();
  delay(check_interval);
}
