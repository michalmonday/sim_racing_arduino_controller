// This sketch programs Arduino Pro Micro microcontroller to read Logitech pedals,
// Logitech shifter, and a handbrake as analog input pins. After programming, the microcontroller  
// and acts as a controller for sim racing games on PC (works in all modern games like Assetto Corsa, 
// ACC, iRacing, Forza Horizon 5, BeamNG, F1 23).

// A nice feature of this setup is that a single USB connector provides the following inputs to a PC:
// - pedals
// - shifter
// - handbrake

// Another nice feature is that it could be easily extended with any Arduino sensors/buttons for custom controls.

#include <Joystick.h>



#include "Joystick.h"

#define PIN_SHIFTER_X A6
#define PIN_SHIFTER_Y A7
#define PIN_SHIFTER_REVERSE 14

#define PIN_PEDALS_CLUTCH A2
#define PIN_PEDALS_BRAKE A1
#define PIN_PEDALS_THROTTLE A3

#define PIN_HANDBRAKE A0
#define PIN_ANALOG_INPUT_UNUSED A10

#define BUTTON_COUNT 8


// MIN and MAX values constrain and scale analog read inputs of pedals. 
// (analog inputs should read values between 0 and 1023 but in reality that's not the case)
//
// The purpose of MIN value is to prevent detecting pedal press when it's physically released.
// When pedals are released the input values should be 0, but in reality are something like 50-90 (depending on pedal).
// Setting MIN values process is as follows:
// - open serial monitor
// - see analog read value of a pedal while it's fully released (press and release the pedal few times)
// - set MIN to the highest observed value (plus some added value for safety, if the highest observed value was 50, then MIN value should be 60-90, no less than 50 in any case)
// Setting MIN value too low will brake automatically without pressing the pedal.
// Setting MIN value too high will ignore pedal being pressed until it reaches certain point.

// Similarily, the purpose of MAX is to allow full range of input even if pressing a pedal does not result in 1023 value being registered.
// If we press the pedal fully, and the analog read returns less than 1023 (which is typically the case), the MAX value
// will scale the highest registered value (e.g. 990) to 1023.
// Setting MAX values process is as follows:
// - open serial monitor
// - see analog read value of a pedal while it's fully pressed
// - set MAX to the lowest observed value (minus some for safety, if the highest observed value was 990, then MAX value should be 940-970, no more than 990 in any case)
// Setting too high MAX value will prevent us from accellerating with 100% power. 
// Ssetting too low MAX value will set acceleration to 100% too early (e.g. when the pedal is pressed far from its full range).
#define CLUTCH_MIN 120
#define CLUTCH_MAX 940

#define BRAKE_MIN 80
#define BRAKE_MAX 880

#define THROTTLE_MIN 110
#define THROTTLE_MAX 950

#define HANDBRAKE_MIN 560
#define HANDBRAKE_MAX 730


Joystick_ pedals(
  0x03,                       // Default, don't change
  JOYSTICK_TYPE_MULTI_AXIS,   // Type
  BUTTON_COUNT,               // Button Count
  0,                          // HatSwitch Count
  false,                      // includeXAxis
  false,                      // includeYAxis
  false,                      // includeZAxis
  false,                      // includeRxAxis
  false,                      // includeRyxis
  false,                      // includeRzxis
  false,                       // includeRudder
  true,                       // includeThrottle
  true,                       // includeAccelerator
  true,                       // includeBrake
  true                      // includeSteering
  
);

/*
//Joystick_(0x03, JOYSTICK_TYPE_GAMEPAD, 4, 2, true, true, false, false, false, false, false, false, false, false, false),
Joystick_ gamepad(
  0x04,                       // Default, don't change
  JOYSTICK_TYPE_GAMEPAD,      // Type
  0,                          // Button Count
  2,                          // HatSwitch Count
  false,                       // includeXAxis
  false,                       // includeYAxis
  false,                       // includeZAxis
  true,                      // includeRxAxis
  true,                      // includeRyxis
  false,                      // includeRzxis
  false,                      // includeRudder
  false,                      // includeThrottle
  false,                      // includeAccelerator
  false,                      // includeBrake
  false                       // includeSteering
);*/

#define BTN_GEAR_NEUTRAL 0
#define BTN_GEAR_1 1
#define BTN_GEAR_2 2
#define BTN_GEAR_3 3
#define BTN_GEAR_4 4
#define BTN_GEAR_5 5
#define BTN_GEAR_6 6
#define BTN_GEAR_REVERSE 7

constexpr long int RANGE_LIMIT{ 1023 };

int last_gear = -1;

void reset_shifter_buttons() {
  for (int i=0; i < BUTTON_COUNT; i++) {
     pedals.setButton(i, LOW);
  }
}

int shifter_pos_to_btn_gear(int x, int y) {
  if (x < 436) {
    if (y < 363) return BTN_GEAR_2;
    if (y > 733) return BTN_GEAR_1;
    return BTN_GEAR_NEUTRAL;
  } 

  // x right
  if (x > 621) {
    if (y < 363) return BTN_GEAR_6;
    if (y > 733) return BTN_GEAR_5;
    return BTN_GEAR_NEUTRAL;
  }

  // x middle
  if (y < 363) return BTN_GEAR_4;
  if (y > 733) return BTN_GEAR_3;
  return BTN_GEAR_NEUTRAL;
}


void setup() {
    Serial.begin(9600);
    pedals.setThrottleRange(0, RANGE_LIMIT);
    pedals.setRudderRange(0, RANGE_LIMIT);
    pedals.setAcceleratorRange(0, RANGE_LIMIT);
    pedals.setBrakeRange(0, RANGE_LIMIT);
    pedals.setSteeringRange(0, RANGE_LIMIT);
    
    pedals.begin();

    //gamepad.begin();

    pinMode(PIN_SHIFTER_REVERSE, INPUT);
    pinMode(PIN_SHIFTER_X, INPUT_PULLUP); 
    pinMode(PIN_SHIFTER_Y, INPUT_PULLUP); 

    pinMode(PIN_HANDBRAKE, INPUT_PULLUP);
}

void loop() {
    
    int rawClutchPosition = constrain( analogRead(PIN_PEDALS_CLUTCH), CLUTCH_MIN, CLUTCH_MAX); 
    
//    Serial.print("Clutch - ");
//    Serial.print(rawClutchPosition, DEC);
    int mappedClutchPosition = map(rawClutchPosition, CLUTCH_MIN, CLUTCH_MAX, RANGE_LIMIT, 0);
    pedals.setAccelerator(mappedClutchPosition);
//    Serial.print(" ("+String(mappedClutchPosition)+") ");


    int rawAcceleratorPosition = constrain( analogRead(PIN_PEDALS_THROTTLE), THROTTLE_MIN, THROTTLE_MAX );
//    Serial.print(" | Accelerator - ");
//    Serial.print(rawAcceleratorPosition, DEC);
    int mappedAcceleratorPostion = map(rawAcceleratorPosition, THROTTLE_MIN, THROTTLE_MAX, RANGE_LIMIT, 0);
    pedals.setThrottle(mappedAcceleratorPostion);
//    Serial.print(" ("+String(mappedAcceleratorPostion)+") ");

    int rawBrakePosition = constrain( analogRead(PIN_PEDALS_BRAKE), BRAKE_MIN, BRAKE_MAX );
//    Serial.print(" | Brake - ");
//    Serial.print(rawBrakePosition, DEC);
    int mappedBrakePosition = map(rawBrakePosition, BRAKE_MIN, BRAKE_MAX, RANGE_LIMIT, 0);
    pedals.setBrake(mappedBrakePosition);
//    Serial.print(" ("+String(mappedBrakePosition)+") ");

    int gear = shifter_pos_to_btn_gear(analogRead(PIN_SHIFTER_X), analogRead(PIN_SHIFTER_Y));
    if (gear == BTN_GEAR_6 && digitalRead(PIN_SHIFTER_REVERSE)) {
      gear = BTN_GEAR_REVERSE;
    }

//    Serial.print(" | Shifter - ");
//    Serial.print(analogRead(PIN_SHIFTER_X), DEC);
//    Serial.print(" x ");
//    Serial.print(analogRead(PIN_SHIFTER_Y), DEC);
    

//    if (gear == BTN_GEAR_REVERSE) {
//      Serial.print("(gear reverse)");
//    } else {
//      Serial.print("(gear " + String(gear) + ")");
//    }

    int rawHandbrake = constrain(analogRead(PIN_HANDBRAKE), HANDBRAKE_MIN, HANDBRAKE_MAX);
    int mappedHandbrakePosition = map(rawHandbrake, HANDBRAKE_MAX, HANDBRAKE_MIN, RANGE_LIMIT, 0);
    pedals.setSteering(mappedHandbrakePosition);
//    Serial.print(" | Handbrake - " + String(rawHandbrake) + " (" + String(mappedHandbrakePosition) + ")");
//    
//    Serial.println(" ");


    if (last_gear != gear) {
      reset_shifter_buttons();
      pedals.setButton(gear, HIGH);
      Serial.println("GEAR CHANGED FROM " + String(last_gear) + " TO " + String(gear));
      last_gear = gear;
    }
    
    //delay(20);
}
