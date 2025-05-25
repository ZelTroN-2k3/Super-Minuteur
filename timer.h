#ifndef TIMER_H
#define TIMER_H

#include <Arduino.h>
#include <EEPROM.h>
#include "LiquidCrystal_I2C.h"
#include "BigNumbers_I2C.h"
#include "RotaryEncoder.h"
#include "conf.h"    // Pour les constantes (RELAY_PIN, etc.) et les types enum si besoin
#include "melodie.h" // Pour les déclarations des play...Melody()

// Références externes aux objets et variables globales définis dans le .ino principal
extern LiquidCrystal_I2C LCD;
extern BigNumbers_I2C bigNum;
extern RotaryEncoder encoder; 

// Extern pour les états et variables globales du .ino principal que ce module utilise/modifie
extern enum Mode currentMode;           
extern enum TimerRunState currentTimerState;

extern bool blinkDone;
extern int lastPos;
extern int newPos; 

extern int displaySEC;
extern int displayMIN;
extern int displayCS;
extern unsigned int targetTotalSeconds;
extern unsigned long targetEndTime;
extern long remainingMillis;
extern unsigned long pausedRemainingMillis;
extern int lastDisplayedSEC;
extern int lastDisplayedMIN;
extern unsigned long lastCsUpdateTime;

extern bool isEndSequenceBlinking;
extern unsigned long blinkSequenceStartTime;
extern unsigned long lastEndBlinkToggleTime;
extern bool endBlinkStateIsOn;

extern byte currentMelodyChoice;
extern byte currentPresetChoice;
extern bool timerMelodyEnabled;

// Fonctions utilitaires du .ino principal que ce module appelle
void resetActivityTimer();
void playClickSound(); 
void displayStatusLine3();
void saveChoiceToEEPROM(int address, byte value); 
extern void clearRestOfLine(byte startCol, byte row); // <<< AJOUT: DÉCLARATION EXTERN

// Fonctions spécifiques au module Timer
void setupTimer(); 
void loopTimer();  
void timerEnd();
void updateStaticDisplay();       
void updateCentisecondsDisplay(); 

void handleTimerEncoderInput(int physicalEncoderPos);
void handleTimerButtonShortPress();
void handleTimerButtonLongPress();

#endif // TIMER_H