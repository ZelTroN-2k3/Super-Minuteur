// melodie.cpp - Définitions (code) des fonctions mélodies

#include "melodie.h" // Inclut les déclarations et les définitions de notes
#include "conf.h"    // Requis pour la constante BUZZER_PIN

// --- Définitions des Fonctions Mélodies ---

// Fonction pour jouer la mélodie Mario (BLOQUANTE - version rythmée)
void playMarioMelody() {
  // Utilise les notes spécifiques définies dans melodie.h
  int tempo = 120; 
  int quarterNote = 60000 / tempo; 
  int eighthNote = quarterNote / 2;
  int halfNote = quarterNote * 2; 
  int dotQuarterNote = quarterNote * 1.5;
  int pause = eighthNote / 3; 
  int noteDur = 0; 
  int delayTime = 0;

  auto playNote = [&](int note, int durationType) {
    noteDur = durationType - pause; if(noteDur < 10) noteDur = 10;
    delayTime = durationType;
    tone(BUZZER_PIN, note, noteDur); // BUZZER_PIN vient de conf.h via melodie.h
    delay(delayTime);
  };

  playNote(NOTE_E6_MARIO, eighthNote); playNote(NOTE_E6_MARIO, eighthNote); playNote(NOTE_E6_MARIO, eighthNote);
  playNote(NOTE_C6_MARIO, eighthNote); playNote(NOTE_E6_MARIO, quarterNote); playNote(NOTE_G6_MARIO, quarterNote);
  playNote(NOTE_G5_MARIO, halfNote);
  delay(quarterNote);
  playNote(NOTE_C6_MARIO, quarterNote); playNote(NOTE_G5_MARIO, quarterNote); playNote(NOTE_E5_MARIO, quarterNote);
  playNote(NOTE_A5_MARIO, eighthNote); playNote(NOTE_Bb5_MARIO, eighthNote); playNote(NOTE_A5_ALT_MARIO, eighthNote);
  playNote(NOTE_G5_MARIO, dotQuarterNote); playNote(NOTE_E6_MARIO, eighthNote); playNote(NOTE_G6_ALT_MARIO, eighthNote);
  playNote(NOTE_A6_MARIO, quarterNote); playNote(NOTE_F6_MARIO, eighthNote); playNote(NOTE_G6_ALT_MARIO, halfNote);
  noTone(BUZZER_PIN);
}

// Fonction pour jouer la Marche Impériale (BLOQUANTE)
void playImperialMarch() {
  // Utilise les notes spécifiques définies dans melodie.h
  int tempo = 110; int quarterNote = 60000 / tempo; int eighthNote = quarterNote / 2;
  int dottedEighthNote = eighthNote * 1.5; int sixteenthNote = quarterNote / 4;
  int halfNote = quarterNote * 2; int pause = eighthNote / 2;

  tone(BUZZER_PIN, NOTE_A4, quarterNote); delay(quarterNote + pause);
  tone(BUZZER_PIN, NOTE_A4, quarterNote); delay(quarterNote + pause);
  tone(BUZZER_PIN, NOTE_A4, quarterNote); delay(quarterNote + pause);
  tone(BUZZER_PIN, NOTE_F4, dottedEighthNote); delay(dottedEighthNote + pause);
  tone(BUZZER_PIN, NOTE_C5, sixteenthNote); delay(sixteenthNote + pause);
  tone(BUZZER_PIN, NOTE_A4, quarterNote); delay(quarterNote + pause);
  tone(BUZZER_PIN, NOTE_F4, dottedEighthNote); delay(dottedEighthNote + pause);
  tone(BUZZER_PIN, NOTE_C5, sixteenthNote); delay(sixteenthNote + pause);
  tone(BUZZER_PIN, NOTE_A4, halfNote); delay(halfNote + pause);
  delay(quarterNote);
  tone(BUZZER_PIN, NOTE_DS5, quarterNote); delay(quarterNote + pause);
  tone(BUZZER_PIN, NOTE_DS5, quarterNote); delay(quarterNote + pause);
  tone(BUZZER_PIN, NOTE_DS5, quarterNote); delay(quarterNote + pause);
  tone(BUZZER_PIN, NOTE_F5, dottedEighthNote); delay(dottedEighthNote + pause);
  tone(BUZZER_PIN, NOTE_C5, sixteenthNote); delay(sixteenthNote + pause);
  tone(BUZZER_PIN, NOTE_GS4, quarterNote); delay(quarterNote + pause);
  tone(BUZZER_PIN, NOTE_F4, dottedEighthNote); delay(dottedEighthNote + pause);
  tone(BUZZER_PIN, NOTE_C5, sixteenthNote); delay(sixteenthNote + pause);
  tone(BUZZER_PIN, NOTE_A4, halfNote); delay(halfNote + pause);
  noTone(BUZZER_PIN);
}

// Fonction pour jouer thème Zelda (BLOQUANTE - Exemple simple)
// Fonction pour jouer thème Zelda (BLOQUANTE - Exemple simple corrigé)
void playZeldaTheme() {
  int tempo = 130;
  int quarterNote = 60000 / tempo;
  int halfNote = quarterNote * 2;
  // int wholeNote = quarterNote * 4; // SUPPRIMÉ (inutilisé)
  int pause = quarterNote / 4;
  int noteDur = 0;

  noteDur = quarterNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_G4, noteDur); delay(quarterNote);
  tone(BUZZER_PIN, NOTE_A4, noteDur); delay(quarterNote);
  tone(BUZZER_PIN, NOTE_B4, noteDur); delay(quarterNote);
  noteDur = halfNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_C5, noteDur); delay(halfNote); // Note plus longue
  noTone(BUZZER_PIN);
}

// --- Nouvelle Mélodie : Nokia Tune ---
void playNokiaTune() {
  int tempo = 180; // Tempo rapide
  int eighthNote = (60000 / tempo) / 2;
  int quarterNote = eighthNote * 2;
  // int dottedQuarterNote = quarterNote * 1.5;
  int pause = eighthNote / 4; // Petite pause entre notes
  int noteDur = 0;

  // Séquence simplifiée mais reconnaissable
  noteDur = eighthNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_E5, noteDur); delay(eighthNote);
  tone(BUZZER_PIN, NOTE_D5, noteDur); delay(eighthNote);
  noteDur = quarterNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_FS4, noteDur); delay(quarterNote);
  tone(BUZZER_PIN, NOTE_GS4, noteDur); delay(quarterNote);
  noteDur = eighthNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_CS5, noteDur); delay(eighthNote);
  tone(BUZZER_PIN, NOTE_B4, noteDur); delay(eighthNote);
  noteDur = quarterNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_D4, noteDur); delay(quarterNote);
  tone(BUZZER_PIN, NOTE_E4, noteDur); delay(quarterNote); // Note E4 déjà définie ? Sinon ajoutez #define NOTE_E4 330
  noteDur = eighthNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_B4, noteDur); delay(eighthNote);
  tone(BUZZER_PIN, NOTE_A4, noteDur); delay(eighthNote);
  noteDur = quarterNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_CS4, noteDur); delay(quarterNote);
  tone(BUZZER_PIN, NOTE_E4, noteDur); delay(quarterNote); // Note E4 déjà définie ?
  noteDur = quarterNote * 2 - pause; if(noteDur<10) noteDur=10; // Note longue finale
  tone(BUZZER_PIN, NOTE_A4, noteDur); delay(quarterNote * 2);

  noTone(BUZZER_PIN);
}

// --- Nouvelle Mélodie : Tetris Theme (Thème A) ---
void playTetrisTheme() {
  int tempo = 145;
  int quarterNote = 60000 / tempo;
  int eighthNote = quarterNote / 2;
  int halfNote = quarterNote * 2;
  int pause = eighthNote / 4;
  int noteDur = 0;

  // Mesure 1
  noteDur = quarterNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_E5, noteDur); delay(quarterNote);
  noteDur = eighthNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_B4, noteDur); delay(eighthNote);
  tone(BUZZER_PIN, NOTE_C5, noteDur); delay(eighthNote);
  noteDur = quarterNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_D5, noteDur); delay(quarterNote);
  noteDur = eighthNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_C5, noteDur); delay(eighthNote);
  tone(BUZZER_PIN, NOTE_B4, noteDur); delay(eighthNote);

  // Mesure 2
  noteDur = quarterNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_A4, noteDur); delay(quarterNote);
  // tone(BUZZER_PIN, NOTE_A4, noteDur); delay(quarterNote); // Répétition A4 enlevée pour simplifier
  tone(BUZZER_PIN, NOTE_C5, noteDur); delay(quarterNote);
  tone(BUZZER_PIN, NOTE_E5, noteDur); delay(quarterNote);
  noteDur = quarterNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_D5, noteDur); delay(quarterNote);
  noteDur = eighthNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_C5, noteDur); delay(eighthNote);
  tone(BUZZER_PIN, NOTE_B4, noteDur); delay(eighthNote);

  // Mesure 3 (identique mesure 1)
  noteDur = quarterNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_E5, noteDur); delay(quarterNote);
  noteDur = eighthNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_B4, noteDur); delay(eighthNote);
  tone(BUZZER_PIN, NOTE_C5, noteDur); delay(eighthNote);
  noteDur = quarterNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_D5, noteDur); delay(quarterNote);
  noteDur = eighthNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_C5, noteDur); delay(eighthNote);
  tone(BUZZER_PIN, NOTE_B4, noteDur); delay(eighthNote);

  // Mesure 4
  noteDur = quarterNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_A4, noteDur); delay(quarterNote);
  // tone(BUZZER_PIN, NOTE_A4, noteDur); delay(quarterNote); // Répétition A4 enlevée
  tone(BUZZER_PIN, NOTE_C5, noteDur); delay(quarterNote);
  noteDur = halfNote - pause; if(noteDur<10) noteDur=10; // Note longue
  tone(BUZZER_PIN, NOTE_E5, noteDur); delay(halfNote);

  // Petite variation pour finir ?
  delay(quarterNote); // Pause
  noteDur = quarterNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_D5, noteDur); delay(quarterNote);
  noteDur = halfNote - pause; if(noteDur<10) noteDur=10;
  tone(BUZZER_PIN, NOTE_C5, noteDur); delay(halfNote); // Fin sur C5

  noTone(BUZZER_PIN);
}
// --- FIN des définitions ---
