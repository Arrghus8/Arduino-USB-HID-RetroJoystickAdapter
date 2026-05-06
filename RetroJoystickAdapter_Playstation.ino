#define CMD 3
#define CLK 5

#define DATA1 2 
#define ATT1 4

#define DATA2 6
#define ATT2 8

#include "HID.h"

#define NUM_PADS 2
#define JOYSTICK1_REPORT_ID  0x03
#define JOYSTICK2_REPORT_ID 0x04
//#define JOYSTICK3_REPORT_ID 0x05

#define JOYSTICK_STATE_SIZE 6

#define delay 6

//#define DEBUG

//================================================================================
//================================================================================
//  Joystick (Gamepad)


#define HIDDESC_MACRO(REPORT_ID) \
    /* Joystick # */ \
    0x05, 0x01,               /* USAGE_PAGE (Generic Desktop) */ \
    0x09, 0x04,               /* USAGE (Joystick) */ \
    0xa1, 0x01,               /* COLLECTION (Application) */ \
    0x85, REPORT_ID,          /* REPORT_ID */ \
    /* 16 Buttons */ \
    0x05, 0x09,               /*   USAGE_PAGE (Button) */ \
    0x19, 0x01,               /*   USAGE_MINIMUM (Button 1) */ \
    0x29, 0x10,               /*   USAGE_MAXIMUM (Button 16) */ \
    0x15, 0x00,               /*   LOGICAL_MINIMUM (0) */ \
    0x25, 0x01,               /*   LOGICAL_MAXIMUM (1) */ \
    0x75, 0x01,               /*   REPORT_SIZE (1) */ \
    0x95, 0x10,               /*   REPORT_COUNT (16) */ \
    0x55, 0x00,               /*   UNIT_EXPONENT (0) */ \
    0x65, 0x00,               /*   UNIT (None) */ \
    0x81, 0x02,               /*   INPUT (Data,Var,Abs) */ \
    /* X and Y Axis */ \
    0x05, 0x01,               /*   USAGE_PAGE (Generic Desktop) */ \
    0x09, 0x01,               /*   USAGE (Pointer) */ \
    0xA1, 0x00,               /*   COLLECTION (Physical) */ \
    0x09, 0x32,               /*     USAGE (Z) */ \
    0x09, 0x35,               /*     USAGE (Rz) */ \
    0x09, 0x30,               /*     USAGE (x) */ \
    0x09, 0x31,               /*     USAGE (y) */ \
    0x15, 0x00,               /*     LOGICAL_MINIMUM (0) */ \
    0x26, 0xff, 0x00,         /*     LOGICAL_MAXIMUM (255) */ \
    0x75, 0x08,               /*     REPORT_SIZE (8) */ \
    0x95, 0x04,               /*     REPORT_COUNT (4) */ \
    0x81, 0x02,               /*     INPUT (Data,Var,Abs) */ \
    0xc0,                     /*   END_COLLECTION */ \
    0xc0                      /* END_COLLECTION */




static const uint8_t hidReportDescriptor[] PROGMEM = {
  HIDDESC_MACRO(JOYSTICK1_REPORT_ID),
  HIDDESC_MACRO(JOYSTICK2_REPORT_ID),
  //HIDDESC_MACRO(JOYSTICK3_REPORT_ID),
};


class Joystick_ {

private:
  uint8_t joystickId;
  uint8_t reportId;
  uint8_t olddata[JOYSTICK_STATE_SIZE];
  uint8_t flag;

public:
  uint8_t type;
  uint8_t data[JOYSTICK_STATE_SIZE];
  uint8_t dataIn;
  bool connected = false;
  uint8_t DATA;
  uint8_t ATT;
  
  Joystick_(uint8_t initJoystickId, uint8_t initReportId) {
    // Setup HID report structure
    static bool usbSetup = false;
  
    if (!usbSetup) {
      static HIDSubDescriptor node(hidReportDescriptor, sizeof(hidReportDescriptor));
      HID().AppendDescriptor(&node);
      usbSetup = true;
    }
    
    // Initalize State
    joystickId = initJoystickId;
    reportId = initReportId;
  
    data[0] = 0;
    data[1] = 0;
    data[2] = 127;
    data[3] = 127;
    data[4] = 127;
    data[5] = 127;
    memcpy(olddata, data, JOYSTICK_STATE_SIZE);
    sendState(1);
  }

  void updateState() {
    if (type != 0x73 && type != 0x53) {
      data[2] = 127;
      data[3] = 127;
      data[4] = 127;
      data[5] = 127;
    }
    if (type == 0x41 || type == 0x73 || type == 0x53) {
      if (memcmp(olddata, data, JOYSTICK_STATE_SIZE)) {    
        memcpy(olddata, data, JOYSTICK_STATE_SIZE);
        flag = 1;
      }
    }
    //sendState();
  }

  void sendState(uint8_t force = 0) {
    if (flag || force) {
      // HID().SendReport(Report number, array of values in same order as HID descriptor, length)
      HID().SendReport(reportId, data, JOYSTICK_STATE_SIZE);
      flag = 0;
    }
  }

};


Joystick_ Joystick[2] =
{
    Joystick_(0, JOYSTICK1_REPORT_ID),
    Joystick_(1, JOYSTICK2_REPORT_ID),
    //Joystick_(2, JOYSTICK3_REPORT_ID),
};

//================================================================================
//================================================================================

void shift(Joystick_& joystick, uint8_t _dataOut) {
  joystick.dataIn = 0;

  for (uint8_t _i = 0; _i < 8; _i++) {
    if (_dataOut & (1 << _i)) 
      digitalWrite(CMD, HIGH);
    else 
      digitalWrite(CMD, LOW);

    digitalWrite(CLK, LOW);
    delayMicroseconds(delay);

    if (digitalRead(joystick.DATA)) joystick.dataIn |= (1 << _i);

    digitalWrite(CLK, HIGH);
    delayMicroseconds(delay);
  }
}

void parallel_shift(uint8_t _dataOut) {
  for (uint8_t _i = 0; _i < NUM_PADS; _i++) {
    Joystick[_i].dataIn = 0;
  }

  for (uint8_t _j = 0; _j < 8; _j++) {
    if (_dataOut & (1 << _j)) 
      digitalWrite(CMD, HIGH);
    else 
      digitalWrite(CMD, LOW);

    digitalWrite(CLK, LOW);
    delayMicroseconds(delay);

    for (uint8_t _i = 0; _i < NUM_PADS; _i++) {
      if (digitalRead(Joystick[_i].DATA)) Joystick[_i].dataIn |= (1 << _j);
    }

    digitalWrite(CLK, HIGH);
    delayMicroseconds(delay);
  }
}

void enable_analog(Joystick_& joystick) {
  // Additional delays are necessary for some controllers(e.g. SCPH-110, Revision H)
  // These delays will only occur when a new controller is detected

  // Enable config mode
  digitalWrite(joystick.ATT, LOW); 
  delayMicroseconds(100);
  shift(joystick, 0x01);  delayMicroseconds(40);
  shift(joystick, 0x43);  delayMicroseconds(40);
  shift(joystick, 0x00);  delayMicroseconds(40);
  shift(joystick, 0x01);  delayMicroseconds(40);
  shift(joystick, 0x00);  delayMicroseconds(40);
  shift(joystick, 0x00);  delayMicroseconds(40);
  shift(joystick, 0x00);  delayMicroseconds(40);
  shift(joystick, 0x00);  delayMicroseconds(40);
  shift(joystick, 0x00);
  digitalWrite(joystick.ATT, HIGH); 
  delayMicroseconds(1000);

  // Enable analog mode
  digitalWrite(joystick.ATT, LOW); 
  delayMicroseconds(100);
  shift(joystick, 0x01);  delayMicroseconds(40);
  shift(joystick, 0x44);  delayMicroseconds(40);
  shift(joystick, 0x00);  delayMicroseconds(40);
  shift(joystick, 0x01);  delayMicroseconds(40);
  shift(joystick, 0x00);  delayMicroseconds(40);
  shift(joystick, 0x00);  delayMicroseconds(40);
  shift(joystick, 0x00);  delayMicroseconds(40);
  shift(joystick, 0x00);  delayMicroseconds(40);
  shift(joystick, 0x00);
  digitalWrite(joystick.ATT, HIGH); 
  delayMicroseconds(1000);

  // Disable config
  digitalWrite(joystick.ATT, LOW); 
  delayMicroseconds(100);
  shift(joystick, 0x01);  delayMicroseconds(40);
  shift(joystick, 0x43);  delayMicroseconds(40);
  shift(joystick, 0x00);  delayMicroseconds(40);
  shift(joystick, 0x00);  delayMicroseconds(40);
  shift(joystick, 0x5A);  delayMicroseconds(40);
  shift(joystick, 0x5A);  delayMicroseconds(40);
  shift(joystick, 0x5A);  delayMicroseconds(40);
  shift(joystick, 0x5A);  delayMicroseconds(40);
  shift(joystick, 0x5A);
  digitalWrite(joystick.ATT, HIGH); 
  delayMicroseconds(1000);
}

void setup() {
  pinMode(CMD, OUTPUT);
  pinMode(CLK, OUTPUT);
  
  Joystick[0].DATA = DATA1;
  Joystick[0].ATT = ATT1;

  Joystick[1].DATA = DATA2;
  Joystick[1].ATT = ATT2;

  //Joystick[2].DATA = DATA3;
  //Joystick[2].ATT = ATT3;

  for (uint8_t i = 0; i < NUM_PADS; i++) {
    pinMode(Joystick[i].DATA, INPUT_PULLUP);
    pinMode(Joystick[i].ATT, OUTPUT);
  }

  #ifdef DEBUG
  Serial.begin(115200);
  #endif
}

void loop() {
  // http://problemkaputt.de/psx-spx.htm#controllerandmemorycardsignals
  
  for (uint8_t i = 0; i < NUM_PADS; i++) {
    digitalWrite(Joystick[i].ATT, LOW);
  }

  parallel_shift(0x01); // Controller Access
  parallel_shift(0x42); // Receive ID bit0..7
  for (uint8_t i = 0; i < NUM_PADS; i++) {
    Joystick[i].type = Joystick[i].dataIn;
  }
  parallel_shift(0x00); // Receive ID bit8..15

  for (uint8_t j = 0; j < 6; j++) {                 // Receive controller state:
    parallel_shift(0x00);                           // j=0: Receive Digital Switches bit0..7  (buttons, inverted)
    for (uint8_t i = 0; i < NUM_PADS; i++) {        // j=1: Receive Digital Switchis bit8..15 (buttons, inverted)
      if (j < 2) {                                  // j=2: Receive Analog Input 0 (left analog)
        Joystick[i].data[j] = ~Joystick[i].dataIn;  // j=3: Receive Analog Input 1 (left analog)
      } else {                                      // j=4: Receive Analog Input 2 (right analog)
        Joystick[i].data[j] = Joystick[i].dataIn;   // j=5: Receive Analog Input 3 (right analog)
      }
    }
  }

  for (uint8_t i = 0; i < NUM_PADS; i++) { 
    digitalWrite(Joystick[i].ATT, HIGH);
  }

  // Check if controller has just been connected (i.e. type not 0xFF)
  // If so, enable analog mode
  for (uint8_t i = 0; i < NUM_PADS; i++) {
    if ((!Joystick[i].connected) && (Joystick[i].type != 0xFF)) {
      enable_analog(Joystick[i]);
    }
    Joystick[i].connected = (Joystick[i].type != 0xFF);
  }

  #ifdef DEBUG
  for (uint8_t i = 0; i < NUM_PADS; i++) {
    Serial.print(" Pad "); Serial.print(i+1); Serial.print(":");
    Serial.print(" type: 0x"); Serial.print(Joystick[i].type, HEX);
    Serial.print(" data: 0x"); Serial.print(Joystick[i].data[0], HEX);
    Serial.print(" 0x"); Serial.print(Joystick[i].data[1], HEX);
    Serial.print(" 0x"); Serial.print(Joystick[i].data[2], HEX);
    Serial.print(" 0x"); Serial.print(Joystick[i].data[3], HEX);
    Serial.print(" 0x"); Serial.print(Joystick[i].data[4], HEX);
    Serial.print(" 0x"); Serial.print(Joystick[i].data[5], HEX);
  }
  Serial.println();
  Serial.flush();
  #endif

  for (uint8_t i = 0; i < NUM_PADS; i++) {
    Joystick[i].updateState();
    Joystick[i].sendState();
  }
  delayMicroseconds(1000);
}
