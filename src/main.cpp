#include <Arduino.h>
#include <Keyboard.h>

/*
  Big Red Button
    Author: Nick Daria
    Date: 11/07/2024

*/

//  Duration of each loop duration in ms
#define APP_PERIOD_MS           5

//  Button
//    Weak internal pullup, switch normally closed to ground, active high
#define CONFIG_BUTTON_PIN       0
#define CONFIG_BUTTON_PULL      INPUT_PULLUP
#define CONFIG_BUTTON_PUSH_DIR  HIGH
#define CONFIG_BUTTON_PUSH_MS   30

//  Open circuit repeat
#define CONFIG_BUTTON_REPEAT_MS 1000

//  LED
#define CONFIG_LED_PIN          LED_BUILTIN

//  Bidirectional button timer (positive = pressed, negative = released)
int64_t button_ms = 0;

//  Decrementing timer for flashing LED after lock command is sent
uint16_t lock_command_sent_flash_tmr = 0;

void setup() {
  pinMode(CONFIG_LED_PIN, OUTPUT);
  pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP);

  Keyboard.begin();
}

//  Uptime modulo operation for flip-flop functionality  
bool flip_flop(unsigned long duration) {
  return (millis() / duration) % 2;
}

//  Bidirectional timer for tracking button press (positive = pressed, negative = released)
void bidirectional_timer(int64_t* timer, bool state) {
  if (state) {
    if (*timer < 0) {
      *timer = 0;  // Reset to zero if less than 0
    }
    // Increment timer, checking for overflow
    if (*timer <= INT64_MAX - APP_PERIOD_MS) {
      *timer += APP_PERIOD_MS;
    } else {
      *timer = INT64_MAX;  // Cap at INT64_MAX to prevent overflow
    }
  } else {
    if (*timer > 0) {
      *timer = 0;  // Reset to zero if greater than 0
    }
    // Decrement timer, checking for underflow
    if (*timer >= INT64_MIN + APP_PERIOD_MS) {
      *timer -= APP_PERIOD_MS;
    } else {
      *timer = INT64_MIN;  // Cap at INT64_MIN to prevent underflow
    }
  }
}

void lockWindows() {
  //  Flash LED for 250ms
  lock_command_sent_flash_tmr = 250;

  //  Win
  Keyboard.press(KEY_LEFT_GUI);

  //  L
  Keyboard.press('l');

  //  Release all keys
  Keyboard.releaseAll();
}

void program_logic() {
  //  Flashing LED
  bool flash_write = flip_flop(500);

  //  Faster flash if lock command sent recently
  if (lock_command_sent_flash_tmr > 0) {
    flash_write = flip_flop(100);
    lock_command_sent_flash_tmr -= APP_PERIOD_MS;
  }

  //  Write to LED
  digitalWrite(CONFIG_LED_PIN, flash_write);

  //  Monitor button status
  bidirectional_timer(&button_ms, digitalRead(CONFIG_BUTTON_PIN) == CONFIG_BUTTON_PUSH_DIR);

  //  Debounced button signal is pressed
  if(button_ms == CONFIG_BUTTON_PUSH_MS || (button_ms % CONFIG_BUTTON_REPEAT_MS == 0 && button_ms >= 0)) {
    lockWindows();
  }
}

void loop() {
  program_logic();

  //  Wait for next loop
  unsigned long release_timestamp = millis() + APP_PERIOD_MS;
  while (millis() < release_timestamp) {
    delayMicroseconds(100);
  }
}