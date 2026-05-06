#include "Gamepad.h"

#define CMD 3
#define CLK 5

#define DATA1 2 
#define ATT1 4

#define DATA2 6
#define ATT2 8

#define NUM_PADS 2

#define delay 6

//#define DEBUG

// ATT: 20 chars max (including NULL at the end) according to Arduino source code.
// Additionally serial number is used to differentiate arduino projects to have different button maps!
const char *gp_serial = "PSX to USB";

//================================================================================
//================================================================================
//  Gamepad (Gamepad)

Gamepad_ Gamepad[NUM_PADS];

//================================================================================
//================================================================================

void shift(Gamepad_& gamepad, uint8_t _dataOut) {
  gamepad.dataIn = 0;

  for (uint8_t _i = 0; _i < 8; _i++) {
    if (_dataOut & (1 << _i)) 
      digitalWrite(CMD, HIGH);
    else 
      digitalWrite(CMD, LOW);

    digitalWrite(CLK, LOW);
    delayMicroseconds(delay);

    if (digitalRead(gamepad.DATA)) gamepad.dataIn |= (1 << _i);

    digitalWrite(CLK, HIGH);
    delayMicroseconds(delay);
  }
}

void parallel_shift(uint8_t _dataOut) {
  for (uint8_t _i = 0; _i < NUM_PADS; _i++) {
    Gamepad[_i].dataIn = 0;
  }

  for (uint8_t _j = 0; _j < 8; _j++) {
    if (_dataOut & (1 << _j)) 
      digitalWrite(CMD, HIGH);
    else 
      digitalWrite(CMD, LOW);

    digitalWrite(CLK, LOW);
    delayMicroseconds(delay);

    for (uint8_t _i = 0; _i < NUM_PADS; _i++) {
      if (digitalRead(Gamepad[_i].DATA)) Gamepad[_i].dataIn |= (1 << _j);
    }

    digitalWrite(CLK, HIGH);
    delayMicroseconds(delay);
  }
}

void enable_analog(Gamepad_& gamepad) {
  // Additional delays are necessary for some controllers(e.g. SCPH-110, Revision H)
  // These delays will only occur when a new controller is detected

  // Enable config mode
  digitalWrite(gamepad.ATT, LOW); 
  delayMicroseconds(100);
  shift(gamepad, 0x01);  delayMicroseconds(40);
  shift(gamepad, 0x43);  delayMicroseconds(40);
  shift(gamepad, 0x00);  delayMicroseconds(40);
  shift(gamepad, 0x01);  delayMicroseconds(40);
  shift(gamepad, 0x00);  delayMicroseconds(40);
  shift(gamepad, 0x00);  delayMicroseconds(40);
  shift(gamepad, 0x00);  delayMicroseconds(40);
  shift(gamepad, 0x00);  delayMicroseconds(40);
  shift(gamepad, 0x00);
  digitalWrite(gamepad.ATT, HIGH); 
  delayMicroseconds(1000);

  // Enable analog mode
  digitalWrite(gamepad.ATT, LOW); 
  delayMicroseconds(100);
  shift(gamepad, 0x01);  delayMicroseconds(40);
  shift(gamepad, 0x44);  delayMicroseconds(40);
  shift(gamepad, 0x00);  delayMicroseconds(40);
  shift(gamepad, 0x01);  delayMicroseconds(40);
  shift(gamepad, 0x00);  delayMicroseconds(40);
  shift(gamepad, 0x00);  delayMicroseconds(40);
  shift(gamepad, 0x00);  delayMicroseconds(40);
  shift(gamepad, 0x00);  delayMicroseconds(40);
  shift(gamepad, 0x00);
  digitalWrite(gamepad.ATT, HIGH); 
  delayMicroseconds(1000);

  // Disable config
  digitalWrite(gamepad.ATT, LOW); 
  delayMicroseconds(100);
  shift(gamepad, 0x01);  delayMicroseconds(40);
  shift(gamepad, 0x43);  delayMicroseconds(40);
  shift(gamepad, 0x00);  delayMicroseconds(40);
  shift(gamepad, 0x00);  delayMicroseconds(40);
  shift(gamepad, 0x5A);  delayMicroseconds(40);
  shift(gamepad, 0x5A);  delayMicroseconds(40);
  shift(gamepad, 0x5A);  delayMicroseconds(40);
  shift(gamepad, 0x5A);  delayMicroseconds(40);
  shift(gamepad, 0x5A);
  digitalWrite(gamepad.ATT, HIGH); 
  delayMicroseconds(1000);
}

void setup() {
  pinMode(CMD, OUTPUT);
  pinMode(CLK, OUTPUT);
  
  Gamepad[0].DATA = DATA1;
  Gamepad[0].ATT = ATT1;

  Gamepad[1].DATA = DATA2;
  Gamepad[1].ATT = ATT2;

  //Gamepad[2].DATA = DATA3;
  //Gamepad[2].ATT = ATT3;

  for (uint8_t i = 0; i < NUM_PADS; i++) {
    pinMode(Gamepad[i].DATA, INPUT_PULLUP);
    pinMode(Gamepad[i].ATT, OUTPUT);
  }

  #ifdef DEBUG
  Serial.begin(115200);
  #endif
}

void loop() {
  // http://problemkaputt.de/psx-spx.htm#controllerandmemorycardsignals
  
  for (uint8_t i = 0; i < NUM_PADS; i++) {
    digitalWrite(Gamepad[i].ATT, LOW);
  }

  parallel_shift(0x01); // Controller Access
  parallel_shift(0x42); // Receive ID bit0..7
  for (uint8_t i = 0; i < NUM_PADS; i++) {
    Gamepad[i].type = Gamepad[i].dataIn;
  }
  parallel_shift(0x00); // Receive ID bit8..15

  for (uint8_t j = 0; j < 6; j++) {                 // Receive controller state:
    parallel_shift(0x00);                           // j=0: Receive Digital Switches bit0..7  (buttons, inverted)
    for (uint8_t i = 0; i < NUM_PADS; i++) {        // j=1: Receive Digital Switchis bit8..15 (buttons, inverted)
      if (j < 2) {                                  // j=2: Receive Analog Input 0 (right analog X)
        Gamepad[i].data[j] = ~Gamepad[i].dataIn;  // j=3: Receive Analog Input 1 (right analog Y)
      } else {                                      // j=4: Receive Analog Input 2 (left analog X)
        Gamepad[i].data[j] = Gamepad[i].dataIn;   // j=5: Receive Analog Input 3 (left analog Y)
      }
    }
  }

  for (uint8_t i = 0; i < NUM_PADS; i++) { 
    digitalWrite(Gamepad[i].ATT, HIGH);
  }

  // Check if controller has just been connected (i.e. type not 0xFF)
  // If so, enable analog mode
  for (uint8_t i = 0; i < NUM_PADS; i++) {
    if ((!Gamepad[i].connected) && (Gamepad[i].type != 0xFF)) {
      enable_analog(Gamepad[i]);
    }
    Gamepad[i].connected = (Gamepad[i].type != 0xFF);
  }

  // If no analog sticks available, set stick to neutral
  for (uint8_t i = 0; i < NUM_PADS; i++) {
    if (Gamepad[i].type != 0x73 && Gamepad[i].type != 0x53) {
      for (uint8_t j = 2; j < 6; j++) {
        Gamepad[i].data[j] = 127;
      }
    }
  }

  #ifdef DEBUG
  for (uint8_t i = 0; i < NUM_PADS; i++) {
    Serial.print(" Pad "); Serial.print(i+1); Serial.print(":");
    Serial.print(" type: 0x"); Serial.print(Gamepad[i].type, HEX);
    Serial.print(" data: 0x"); Serial.print(Gamepad[i].data[0], HEX);
    Serial.print(" 0x"); Serial.print(Gamepad[i].data[1], HEX);
    Serial.print(" 0x"); Serial.print(Gamepad[i].data[2], HEX);
    Serial.print(" 0x"); Serial.print(Gamepad[i].data[3], HEX);
    Serial.print(" 0x"); Serial.print(Gamepad[i].data[4], HEX);
    Serial.print(" 0x"); Serial.print(Gamepad[i].data[5], HEX);
  }
  Serial.println();
  Serial.flush();
  #endif

  for (uint8_t i = 0; i < NUM_PADS; i++) {
    Gamepad[i]._GamepadReport.buttons = (Gamepad[i].data[0]) | (Gamepad[i].data[1] << 8);
    Gamepad[i]._GamepadReport.right_X = Gamepad[i].data[2];
    Gamepad[i]._GamepadReport.right_Y = Gamepad[i].data[3];
    Gamepad[i]._GamepadReport.left_X  = Gamepad[i].data[4];
    Gamepad[i]._GamepadReport.left_Y  = Gamepad[i].data[5];
  }
  sendState();
}

void sendState()
{
  for (uint8_t i = 0; i < NUM_PADS; i++) {
    Gamepad[i].send();
  }
  __builtin_avr_delay_cycles(16000);
}