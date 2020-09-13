/**************************************************************************

  BSD 3-Clause License

  Copyright (c) 2020, Matthias Wientapper
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

  3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


  Midi Foot Switch for HX Stomp
  =============================
  - Button Up/Down on pins D2, D3
  - LED green/red          D4, D5
  - requires OneButton lib https://github.com/mathertel/OneButton
  - requires JC_Button lib https://github.com/JChristensen/JC_Button

  We are using two different button libraries for different purposes:
    - OneButton to detect a long press without detecting a short press first.
      This means the action will be evaluated on releasing the button,
      which is ok for most cases but bad for timing critical actions
      like when we are in looper mode.
    - JC_Button to detect a short press as soon as the button is pressed down.
      This button library is used in looper mode only.

  SCROLL Mode:   up/dn switches prog patches
               long press dn: TUNER
               long press up: SNAPSHOT
               up + dn: toggle FS4/FS5 and SCROLL mode
  TUNER Mode:    up or dn back to prev Mode
  SNAPSHOT Mode: up/dn switches snapshot?
               long press dn: TUNER
               long press up: FS mode
  FS Mode:       up/dn emulate FS4/FS5
               long press up: TUNER
               long press dn: back to SCROLL mode

**************************************************************************/

#include <Wire.h>
#include <OneButton.h>
//#include <JC_Button.h>

#include <EEPROM.h>

#define BTN_1 6
#define BTN_2 5
#define BTN_3 4
#define BTN_4 3
#define BTN_5 2
#define BTN_6 7
#define BTN_7 8
#define BTN_8 9
#define BTN_9 10
#define BTN_10 11
#define LED_GRN 12
#define LED_RED 13

// Adjust red LED brightness 0-255 (full on was way too bright for me)
#define LED_RED_BRIGHTNESS 25

OneButton btn1(BTN_1, true);
OneButton btn2(BTN_2, true);
OneButton btn3(BTN_3, true);
OneButton btn3FS(BTN_3, true);
OneButton btn4(BTN_4, true);
OneButton btn5(BTN_5, true);
OneButton btn6(BTN_6, true);
OneButton btn7(BTN_7, true);
OneButton btn8(BTN_8, true);
OneButton btn9(BTN_9, true);
OneButton btn10(BTN_10, true);
//OneButton loop1(BTN_UP, true);
//OneButton loop2(BTN_DN, true);

//Button jc_btnUp(BTN_UP);
//Button jc_btnDn(BTN_DN);


enum modes_t {SCROLL, SNAPSHOT, FS, LOOPER};       // modes of operation
static modes_t MODE;       // current mode
static modes_t LAST_MODE;  // last mode

enum lmodes_t {PLAY, RECORD, OVERDUB, STOP};   // Looper modes
static lmodes_t LPR_MODE;

bool with_looper = true;

bool led = true;

void (*Reset)(void) = 0;

void setup() {

  uint8_t eeprom_val;
  uint8_t looper_val;

  // LEDs
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GRN, OUTPUT);

  // Buttons:

  btn1.setClickTicks(50);
  btn1.attachClick(oneClick);

  btn2.setClickTicks(50);
  btn2.attachClick(twoClick);

  btn3.setClickTicks(50);
  btn3.attachClick(threeClick);

  btn3FS.setClickTicks(50);
  btn3FS.attachClick(threeClickFS);

  btn4.setClickTicks(50);
  btn4.attachClick(fourClick);

  btn5.setClickTicks(50);
  btn5.attachClick(fiveClick);

  btn6.setClickTicks(50);
  btn6.attachClick(sixClick);

  btn7.setClickTicks(50);
  btn7.attachClick(sevenClick);


  btn8.setClickTicks(50);
  btn8.attachClick(eightClick);


  btn9.setClickTicks(50);
  btn9.attachClick(nineClick);
  
  btn10.setClickTicks(50);
  btn10.attachClick(tenClick);



  // Set MIDI baud rate:
  Serial.begin(31250);

      
  // get last used mode from eeprom adr. 0
  eeprom_val = EEPROM.read(0);

  // restore last mode, if possible (= valid value)
  if (eeprom_val < 4)
    MODE = (modes_t)eeprom_val;
  else
    // no valid value in eeprom found. (Maybe this is the first power up ever?)
    MODE = SCROLL;

  // restore mode on HX Stomp as well
  if (MODE == SNAPSHOT)
    midiCtrlChange(71, 3); // set snapshot mode
  else if (MODE == FS)
    midiCtrlChange(71, 0); // set stomp mode
  else if (MODE == SCROLL)
    midiCtrlChange(71, 0); // set stomp mode

  // indicate mode via LEDs
  if (MODE == LOOPER) {
    flashRedGreen(10);
    // we are in looper mode, so we are using the jc_button class for action on button press
    // (OneButton acts on button release)
//    jc_btnUp.begin();
//    jc_btnDn.begin();
  }
  else if (MODE == SCROLL)
    flashLED(10, LED_RED);
  else if (MODE == FS)
    flashLED(10, LED_GRN);

  // Looper default state
  LPR_MODE = STOP;

  // check if btd_dn or btn_up is still pressed on power on
  // and enable disable looper mode availability
  // (buttons are low active)
   if(digitalRead(BTN_2) == 0 && digitalRead(BTN_1) == 1){
      // btn dn pressed
      with_looper = false;
      EEPROM.update(1, with_looper);
      delay(500);
      flashLED(5, LED_RED);
  }
   if(digitalRead(BTN_2) == 1 && digitalRead(BTN_1) == 0){
      // btn up pressed
      with_looper = true;
      EEPROM.update(1, with_looper);
      delay(500);
      flashLED(5, LED_GRN);
  }

  // restore looper config from adr. 1
  looper_val = EEPROM.read(1);
  if (looper_val < 2)
      with_looper = looper_val;
   else
      with_looper = true;
}

void loop() {

//  if (MODE == LOOPER) {
////    jc_btnDn.read();                   // DN Button handled by JC_Button
////    jc_btnUp.read();
////    btnUp.tick();                   // Up Button handled by OneButton
////    jc_dnClick();
////    jc_upClick();
//    loop1.tick();
//    loop2.tick();
//    btn5.tick();
////    if (jc_btnDn.wasPressed() && digitalRead(BTN_UP) == 1)          // attach handler
////      jc_dnClick();
//
//  } else {

  if (MODE == FS) {
    btn1.tick();                   // both buttons handled by OneButton
    btn2.tick();
    btn3FS.tick();
    btn4.tick();
    btn5.tick();
    btn6.tick();
    btn7.tick();
    btn8.tick();
    btn9.tick();
    btn10.tick();
  }
  else {
    btn1.tick();                   // both buttons handled by OneButton
    btn2.tick();
    btn3.tick();
    btn4.tick();
    btn5.tick();
    btn6.tick();
    btn7.tick();
    btn8.tick();
    btn9.tick();
    btn10.tick();
  }

  handle_leds();
}

/* ------------------------------------------------- */
/* ---       OneButton Callback Routines          ---*/
/* ------------------------------------------------- */

void oneClick() {
  switch (MODE)
  {
    case SCROLL:
      patchUp();
      flashLED(2, LED_RED);
      break;
    case SNAPSHOT:
      flashLED(2, LED_RED);
      midiCtrlChange(69, 8);  // next snapshot
      break;
    case FS:
      flashLED(2, LED_RED);
      midiCtrlChange(52, 0); // emulate FS 5
      break;
  }
}

void twoClick() {
  switch (MODE)
  {
    case SCROLL:
      patchDown();
      flashLED(2, LED_RED);
      break;
    case SNAPSHOT:
      midiCtrlChange(69, 9);  // prev snapshot
      flashLED(2, LED_RED);
      break;
    case FS:
      midiCtrlChange(53, 0); // emulate FS 4
      flashLED(2, LED_RED);
      break;
  }
}



void threeClick() {
  switch (LPR_MODE) {
    case STOP:
      LPR_MODE = RECORD;
      midiCtrlChange(60, 127);  // Looper record
      break;
    case RECORD:
      LPR_MODE = PLAY;
      midiCtrlChange(61, 127); // Looper play
      break;
    case PLAY:
      LPR_MODE = OVERDUB;
      midiCtrlChange(60, 0);    // Looper overdub
      break;
    case OVERDUB:
      LPR_MODE = PLAY;
      midiCtrlChange(61, 127); // Looper play
      break;
  }
}

void threeClickFS() {
  midiCtrlChange(52, 0); // emulate FS 4
}

void fourClick() {
  midiCtrlChange(63,127);
}

void fiveClick() {
  midiCtrlChange(69,2);
}



void sixClick() {
  midiCtrlChange(69,0);
}

void sevenClick() {
  midiCtrlChange(69,1);
}

void eightClick() {
  midiCtrlChange(69,2);
}


void nineClick() {
  switch (MODE)
  {
    
    case SCROLL:
      MODE = FS;
      midiCtrlChange(71, 0); // set stomp mode
      break;
    case FS:
      if (with_looper)
       {MODE = LOOPER;
       midiProgChange(106);
       EEPROM.update(0, MODE);
    // reset the device to reboot in new mode
    Reset();
        switch (LPR_MODE) {
          case PLAY:
          case STOP:
            midiCtrlChange(63, 127); // looper undo/redo
            flashLED(3, LED_RED);
            break;
          case RECORD:
          case OVERDUB:
            break;
        }

        }
       else
       {MODE = SCROLL;
       midiCtrlChange(71, 0); // set snapshot mode
       }
      break;
    case LOOPER:
      // make sure to switch off looper
      midiCtrlChange(61, 0); // Looper stop
      MODE = SCROLL;
      midiCtrlChange(71, 0); // set stomp mode
//      break;
//    case SNAPSHOT:
//       MODE = SCROLL;
//       midiCtrlChange(71, 0); // set stomp mode
  }
}


void tenClick() {
  midiCtrlChange(68,0);
}




///* ------------------------------------------------- */
///* ---       JC_Button Callback Routines          ---*/
///* ------------------------------------------------- */
//
//void jc_dnClick() {
//  switch (LPR_MODE) {
//    case STOP:
//      LPR_MODE = RECORD;
//      midiCtrlChange(60, 127);  // Looper record
//      break;
//    case RECORD:
//      LPR_MODE = PLAY;
//      midiCtrlChange(61, 127); // Looper play
//      break;
//    case PLAY:
//      LPR_MODE = OVERDUB;
//      midiCtrlChange(60, 0);    // Looper overdub
//      break;
//    case OVERDUB:
//      LPR_MODE = PLAY;
//      midiCtrlChange(61, 127); // Looper play
//      break;
//  }
//}
//
//void jc_upClick() {
//  midiCtrlChange(63,127);
//}




/* ------------------------------------------------- */
/* ---      Midi Routines                         ---*/
/* ------------------------------------------------- */

// HX stomp does not have a native patch up/dn midi command
// so we are switching to scroll mode and emulating a FS1/2
// button press.
void patchUp() {
  midiCtrlChange(71, 1); // HX scroll mode
  delay(30);
  midiCtrlChange(50, 127); // FS 2 (up)
  midiCtrlChange(71, 0);  // HX stomp mode
}

void patchDown() {
  midiCtrlChange(71, 1);   // HX scroll mode
  delay(30);
  midiCtrlChange(49, 127); // FS 1 (down)
  midiCtrlChange(71, 0);   // HX stomp mode
}

void midiProgChange(uint8_t p) {
  Serial.write(0xc0); // PC message
  Serial.write(p); // prog
}
void midiCtrlChange(uint8_t c, uint8_t v) {
  Serial.write(0xb0); // CC message
  Serial.write(c); // controller
  Serial.write(v); // value
}

/* ------------------------------------------------- */
/* ---      Misc Stuff                            ---*/
/* ------------------------------------------------- */
void flashLED(uint8_t i, uint8_t led)
{
  uint8_t last_state;
  last_state = digitalRead(led);

  for (uint8_t j = 0; j < i; j++) {
    digitalWrite(led, HIGH);
    delay(30);
    digitalWrite(led, LOW);
    delay(30);
  }
  digitalWrite(led, last_state);
}

void flashRedGreen(uint8_t i) {
  uint8_t last_state_r;
  uint8_t last_state_g;
  last_state_r = digitalRead(LED_RED);
  last_state_g = digitalRead(LED_GRN);


  for (uint8_t j = 0; j < i; j++) {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GRN, HIGH);
    delay(75);
    analogWrite(LED_RED, LED_RED_BRIGHTNESS);
    digitalWrite(LED_GRN, LOW);
    delay(75);
  }
  digitalWrite(LED_RED, last_state_r);
  digitalWrite(LED_GRN, last_state_g);
}

void handle_leds() {
  static unsigned long next = millis();

  switch (MODE) {
    case SCROLL:
      // solid red
      digitalWrite(LED_GRN, LOW);
      analogWrite(LED_RED, LED_RED_BRIGHTNESS);
      //digitalWrite(LED_RED, HIGH);
      break;
    case SNAPSHOT:
      // solid green
      digitalWrite(LED_GRN, HIGH);
      digitalWrite(LED_RED, LOW);
      break;
    case FS:
      // solid green
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GRN, HIGH);
      break;

    case LOOPER:
      switch (LPR_MODE) {
        case STOP:
          digitalWrite(LED_GRN, LOW);
          digitalWrite(LED_RED, LOW);
          break;
        case PLAY:
          digitalWrite(LED_GRN, HIGH);
          digitalWrite(LED_RED, LOW);
          break;
        case RECORD:
          digitalWrite(LED_GRN, LOW);
          analogWrite(LED_RED, LED_RED_BRIGHTNESS);
          break;
        case OVERDUB:
          // yellow
          digitalWrite(LED_GRN, HIGH);
          analogWrite(LED_RED, LED_RED_BRIGHTNESS);
          break;
      }
      break;
  }
}
