// metronome.h
#ifndef METRONOME_H
#define METRONOME_H

#include <Arduino.h>
#include <EEPROM.h> // Important pour les fonctions EEPROM
#include "LiquidCrystal_I2C.h"
#include "BigNumbers_I2C.h"
#include "RotaryEncoder.h" 
#include "conf.h"          

// Références externes aux objets et variables globales définis dans le .ino principal
extern LiquidCrystal_I2C LCD;
extern BigNumbers_I2C bigNum;
extern RotaryEncoder encoder;

// Extern pour les états et variables globales du .ino principal que ce module utilise/modifie
extern enum Mode currentMode; 
extern enum MetronomeRunState currentMetroState; 

extern int currentBPM;
extern unsigned long lastMetroBeatTime;
extern byte currentBeatInMeasure;
extern byte timeSignatureNum;
extern byte timeSignatureDen; // <<< CETTE LIGNE DOIT ÊTRE LÀ
extern int metroEncoderLastPos;
extern int lastEncoderMenuPos; 

// Fonctions utilitaires du .ino principal que ce module appelle
void resetActivityTimer();
void playClickSound();
void enterMainMenu(); 
void saveChoiceToEEPROM(int address, byte value); 
void clearRestOfLine(byte startCol, byte row); 

// Fonctions spécifiques au module Métronome
void setupMetronome(); 
void enterMetronomeMode();
void displayMetronomeScreen();
void handleMetronomeLogic();
void playMetronomeBeatSound(bool isAccent);
void saveBPMToEEPROM(int bpmValue);

// Fonctions pour le sous-menu de la Signature Rythmique (TS) du métronome
void enterTSMetroMenu();
void displayTSMetroMenu();
void navigateTSMetroMenu(int diff);
void selectTSMetroMenuItem();

#endif // METRONOME_H