// melodie.h - Déclarations des fonctions mélodies et définitions des notes

#ifndef MELODIE_H
#define MELODIE_H

#include <Arduino.h> // Pour les types comme byte (implicite mais bon)

// --- Fréquences des Notes ---
// (Toutes les définitions #define NOTE_... restent ici)
#define NOTE_A4  440
#define NOTE_F4  349
#define NOTE_C5  523
#define NOTE_AS4 466  // A#4 / Bb4
#define NOTE_G4  392
#define NOTE_E4  330
#define NOTE_FS4 370  // F#4 / Gb4
#define NOTE_GS4 415  // G#4 / Ab4
#define NOTE_CS5 554  // C#5 / Db5
#define NOTE_DS5 622  // D#5 / Eb5
#define NOTE_D5  587
#define NOTE_F5  698
#define NOTE_E5  659  // E5
#define NOTE_D4  294  // D4
#define NOTE_CS4 277  // C#4 / Db4
#define NOTE_B4  494  // Pour Zelda
// Notes "Mario" spécifiques
#define NOTE_E6_MARIO  660
#define NOTE_C6_MARIO  510
#define NOTE_G6_MARIO  770
#define NOTE_G5_MARIO  380
#define NOTE_E5_MARIO  320
#define NOTE_A5_MARIO  440
#define NOTE_Bb5_MARIO 480
#define NOTE_A5_ALT_MARIO 450
#define NOTE_G6_ALT_MARIO 760
#define NOTE_A6_MARIO 860
#define NOTE_F6_MARIO 700


// --- Déclarations des Fonctions Mélodies ---
// Les définitions (le code complet) sont maintenant dans melodie.cpp
void playMarioMelody();
void playImperialMarch();
void playZeldaTheme();
void playNokiaTune(); 
void playTetrisTheme(); 
void playBipBipMelody();

#endif // MELODIE_H