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
#define LED_BLUE 13


OneButton btn1(BTN_1, true);
OneButton btn2(BTN_2, true);
OneButton btn3(BTN_3, true);
OneButton btn4(BTN_4, true);
OneButton btn5(BTN_5, true);
OneButton btn6(BTN_6, true);
OneButton btn7(BTN_7, true);
OneButton btn8(BTN_8, true);
OneButton btn9(BTN_9, true);
OneButton btn10(BTN_10, true);


enum modes_t {LOOPER};       // modes of operation
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
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GRN, OUTPUT);

  // Buttons:

  btn1.setClickTicks(50);
  btn1.attachClick(oneClick);
  btn1.attachLongPressStart(oneLongPressStart);

  btn2.setClickTicks(50);
  btn2.attachClick(twoClick);
  btn2.attachLongPressStart(twoLongPressStart);

  btn3.setClickTicks(50);
  btn3.attachClick(threeClick);

  btn4.setClickTicks(50);
  btn4.attachClick(fourClick);

  btn5.setClickTicks(50);
  btn5.attachClick(fiveClick);

  btn6.setClickTicks(50);
  btn6.attachClick(sixClick);
  btn6.attachLongPressStart(sixLongPressStart);

  btn7.setClickTicks(50);
  btn7.attachClick(sevenClick);
  btn7.attachLongPressStart(sevenLongPressStart);


  btn8.setClickTicks(50);
  btn8.attachClick(eightClick);

  btn9.setClickTicks(50);
  btn9.attachClick(nineClick);
  btn9.attachLongPressStart(nineLongPressStart);
  
  btn10.setClickTicks(50);
  btn10.attachClick(tenClick);



  // Set MIDI baud rate:
  Serial.begin(31250);

      
  // Looper default state
  LPR_MODE = STOP;

}

void loop() {
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
//  handle_leds();
}

/* ------------------------------------------------- */
/* ---       OneButton Callback Routines          ---*/
/* ------------------------------------------------- */

void oneClick() {
      patchDown();
      flashLED(2, LED_BLUE);
}

void oneLongPressStart() {
      midiCtrlChange(70, 0); // Bypass On
}

void twoClick() {
      patchUp();
      flashLED(2, LED_BLUE);
}

void twoLongPressStart() {
      midiCtrlChange(70, 127); // Bypass Off
}



void threeClick() {
  midiCtrlChange(1,0); // emulate EXP 1 low
}
void threeLongPressStart() {
  midiCtrlChange(1, 127); // emulate EXP 1  High
}

void fiveClick() {
  midiCtrlChange(63,127); //looper undo
}

void fourClick() {
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



void sixClick() {
  midiCtrlChange(69,0); // snapshot 1
}

void sixLongPressStart() {
    midiCtrlChange(52, 0); // FS 4
}


void sevenClick() {
  midiCtrlChange(69,1); //snapshot 2
}

void sevenLongPressStart() {
  midiCtrlChange(53, 0); // FS 5
}


void eightClick() {
  midiCtrlChange(69,2); // snapshot 3
}

void nineClick() {
  midiCtrlChange(61,127); // play looper
}

void nineLongPressStart() {
  midiCtrlChange(62, 127); // play once
}

void tenClick() {
  midiCtrlChange(61,0); //stop looper
}








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
  last_state_r = digitalRead(LED_BLUE);
  last_state_g = digitalRead(LED_GRN);


  for (uint8_t j = 0; j < i; j++) {
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(LED_GRN, HIGH);
    delay(75);
    analogWrite(LED_BLUE, HIGH);
    digitalWrite(LED_GRN, LOW);
    delay(75);
  }
  digitalWrite(LED_BLUE, last_state_r);
  digitalWrite(LED_GRN, last_state_g);
}

//void handle_leds() {
//  static unsigned long next = millis();
//
//  switch (MODE) {
//    case SCROLL:
//      // solid red
//      digitalWrite(LED_GRN, LOW);
//      analogWrite(LED_BLUE, HIGH);
//      //digitalWrite(LED_BLUE, HIGH);
//      break;
//    case SNAPSHOT:
//      // solid green
//      digitalWrite(LED_GRN, HIGH);
//      digitalWrite(LED_BLUE, LOW);
//      break;
//    case FS:
//      // solid green
//      digitalWrite(LED_BLUE, LOW);
//      digitalWrite(LED_GRN, HIGH);
//      break;
//
//    case LOOPER:
//      switch (LPR_MODE) {
//        case STOP:
//          digitalWrite(LED_GRN, LOW);
//          digitalWrite(LED_BLUE, LOW);
//          break;
//        case PLAY:
//          digitalWrite(LED_GRN, HIGH);
//          digitalWrite(LED_BLUE, LOW);
//          break;
//        case RECORD:
//          digitalWrite(LED_GRN, LOW);
//          analogWrite(LED_BLUE, HIGH);
//          break;
//        case OVERDUB:
//          // yellow
//          digitalWrite(LED_GRN, HIGH);
//          analogWrite(LED_BLUE, HIGH);
//          break;
//      }
//      break;
//  }
//}
