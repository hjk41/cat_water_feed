int ir_input = 2;
int faucet_control_pos = 3;
int faucet_control_neg = 4;

int last_on_time = 0;
int last_off_time = -100000000;
int faucet_delay = 500;
int check_delay = 3000;
int cooldown = 5*60*1000;
int backoff_period = 2 * cooldown;
bool backoff = false;

void setup() {
  // put your setup code here, to run once:
  pinMode(ir_input, INPUT_PULLUP);
  pinMode(faucet_control_pos, OUTPUT);
  pinMode(faucet_control_neg, OUTPUT);
}

bool faucet_on = false;

void set_faucet(bool on) {
  if (on) {
    if (!faucet_on) {
      // backoff if 
      if (backoff && millis() - last_off_time < backoff_period) {
        return;
      }
      digitalWrite(faucet_control_pos, HIGH);
      digitalWrite(faucet_control_neg, LOW);
      faucet_on = true;
      backoff = false;
      last_on_time = millis();
      delay(faucet_delay);
    }
    else {
      if (faucet_on && millis() - last_on_time > cooldown) {
        // check if on too long
        backoff = true;
        set_faucet(false);
      }
      return;
    }
  } else {
    if (faucet_on) {
      digitalWrite(faucet_control_pos, LOW);
      digitalWrite(faucet_control_neg, HIGH);
      faucet_on = false;
      last_off_time = millis();
      delay(faucet_delay);
    } else {
      return;
    }
  }
  digitalWrite(faucet_control_pos, LOW);
  digitalWrite(faucet_control_neg, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(digitalRead(ir_input) == HIGH) {
    set_faucet(true);
  } else {
    // off
    set_faucet(false);
  }
  delay(check_delay);
}
