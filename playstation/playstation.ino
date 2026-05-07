#include "Gamepad.h"

#define CMD 2
#define CLK 3

#define DATA1 4 
#define ATT1  5

#define DATA2 6
#define ATT2  7

//#define DATA3 8
//#define ATT3  9

#define NUM_PADS 2

#define delay 6

//#define DEBUG

// ATT: 20 chars max (including NULL at the end) according to Arduino source code.
// Additionally serial number is used to differentiate arduino projects to have different button maps!
const char *gp_serial = "PSX to USB";

Gamepad_ Gamepad[NUM_PADS];

//================================================================================
//================================================================================

void shift(Gamepad_& gamepad, uint8_t _dataOut) {
  gamepad.dataIn = 0;

  for (uint8_t i = 0; i < 8; i++) {
    if (_dataOut & (1 << i)) 
      digitalWrite(CMD, HIGH);
    else 
      digitalWrite(CMD, LOW);

    digitalWrite(CLK, LOW);
    delayMicroseconds(delay);

    if (digitalRead(gamepad.DATA)) gamepad.dataIn |= (1 << i);

    digitalWrite(CLK, HIGH);
    delayMicroseconds(delay);
  }
}

void parallel_shift(uint8_t _dataOut) {
  for (uint8_t i = 0; i < NUM_PADS; i++) {
    Gamepad[i].dataIn = 0;
  }

  for (uint8_t j = 0; j < 8; j++) {
    if (_dataOut & (1 << j)) 
      digitalWrite(CMD, HIGH);
    else 
      digitalWrite(CMD, LOW);

    digitalWrite(CLK, LOW);
    delayMicroseconds(delay);

    for (uint8_t i = 0; i < NUM_PADS; i++) {
      if (digitalRead(Gamepad[i].DATA)) Gamepad[i].dataIn |= (1 << j);
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

void get_state() {
  // http://problemkaputt.de/psx-spx.htm#controllerandmemorycardsignals
  
  for (uint8_t i = 0; i < NUM_PADS; i++) {
    digitalWrite(Gamepad[i].ATT, LOW);
  }

  parallel_shift(0x01); // Controller Access
  parallel_shift(0x42); // Receive ID bit0..7
  for (uint8_t i = 0; i < NUM_PADS; i++) {
    Gamepad[i].type = Gamepad[i].dataIn;
    Gamepad[i].analog = (Gamepad[i].type == 0x73 || Gamepad[i].type == 0x53);
  }
  parallel_shift(0x00); // Receive ID bit8..15

  // Receive controller state:
  // j=0: Receive Digital Switches bit0..7  (buttons, inverted)
  // j=1: Receive Digital Switchis bit8..15 (buttons, inverted)
  // j=2: Receive Analog Input 0 (right analog X)
  // j=3: Receive Analog Input 1 (right analog Y)
  // j=4: Receive Analog Input 2 (left analog X)
  // j=5: Receive Analog Input 3 (left analog Y)
  for (uint8_t j = 0; j < 6; j++) {
    parallel_shift(0x00);
    for (uint8_t i = 0; i < NUM_PADS; i++) {
      Gamepad[i].data[j] = (j < 2) ? ~Gamepad[i].dataIn : Gamepad[i].dataIn;
    }
  }

  for (uint8_t i = 0; i < NUM_PADS; i++) { 
    digitalWrite(Gamepad[i].ATT, HIGH);
  }
}

//================================================================================
//================================================================================

void setup() {
  pinMode(CMD, OUTPUT);
  pinMode(CLK, OUTPUT);
  
  Gamepad[0].DATA = DATA1;
  Gamepad[0].ATT  = ATT1;

  Gamepad[1].DATA = DATA2;
  Gamepad[1].ATT  = ATT2;

  //Gamepad[2].DATA = DATA3;
  //Gamepad[2].ATT  = ATT3;

  for (uint8_t i = 0; i < NUM_PADS; i++) {
    pinMode(Gamepad[i].DATA, INPUT_PULLUP);
    pinMode(Gamepad[i].ATT, OUTPUT);
  }

  #ifdef DEBUG
  Serial.begin(115200);
  #endif
}

void loop() {
  // Get current button state
  get_state();

  // Check if controller has just been connected (i.e. type not 0xFF)
  // If so, enable analog mode
  for (uint8_t i = 0; i < NUM_PADS; i++) {
    if ((!Gamepad[i].connected) && (Gamepad[i].type != 0xFF)) {
      enable_analog(Gamepad[i]);
    }
    Gamepad[i].connected = (Gamepad[i].type != 0xFF);
  }

  #ifdef DEBUG
  for (uint8_t i = 0; i < NUM_PADS; i++) {
    Serial.print(" Pad "); Serial.print(i+1); Serial.print(":");
    Serial.print(" type: 0x"); Serial.print(Gamepad[i].type, HEX);
    Serial.print(" data:");
    for (uint8_t j = 0; j < 6; j++) {
        Serial.print(" 0x"); Serial.print(Gamepad[i].data[j], HEX);
    }
  }
  Serial.println();
  Serial.flush();
  #endif

  for (uint8_t i = 0; i < NUM_PADS; i++) {
    Gamepad[i]._GamepadReport.buttons = (Gamepad[i].data[0]) | (Gamepad[i].data[1] << 8);
    Gamepad[i]._GamepadReport.right_X = Gamepad[i].analog ? Gamepad[i].data[2] : 127;
    Gamepad[i]._GamepadReport.right_Y = Gamepad[i].analog ? Gamepad[i].data[3] : 127;
    Gamepad[i]._GamepadReport.left_X  = Gamepad[i].analog ? Gamepad[i].data[4] : 127;
    Gamepad[i]._GamepadReport.left_Y  = Gamepad[i].analog ? Gamepad[i].data[5] : 127;
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