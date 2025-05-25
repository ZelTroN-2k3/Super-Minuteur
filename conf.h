// conf.h - Fichier de configuration pour la minuterie (CORRIGÉ)

#ifndef CONF_H // Protection pour éviter les inclusions multiples
#define CONF_H

#include <Arduino.h> // Pour byte etc.

// --- Énumérations Globales --- 
enum Mode {
  MODE_TIMER,
  MODE_MENU_MAIN,
  MODE_MENU_MELODY,
  MODE_MENU_PRESET,
  MODE_MENU_VEILLE,
  MODE_METRONOME,
  MODE_MENU_TS_METRO, 
  MODE_MENU_TEMPO_PRESET
};

enum TimerRunState { STATE_IDLE, STATE_RUNNING, STATE_PAUSED };
enum MetronomeRunState { METRO_STOPPED, METRO_RUNNING };
// --- Fin Énumérations Globales ---

// --- Informations Auteur/Version ---
#define AUTHOR_NAME "ANCHER.P" // ICI VOTRE NOM
#define FIRMWARE_VERSION "1.8.0_METRO" // Version mise à jour

// --- Configuration Matérielle ---

// Broches Arduino
const byte BUTTON_PIN = 6;
const byte RELAY_PIN  = 10;
const byte BUZZER_PIN = 12; // Utilisé par melodie.h
const byte ENCODER_DT_PIN = 4;  // Broche DT de l'encodeur
const byte ENCODER_CLK_PIN = 2; // Broche CLK de l'encodeur

// Configuration LCD I2C
const byte LCD_ADDR = 0x27; // Adresse I2C de l'écran
const byte LCD_COLS = 20;   // Nombre de colonnes de l'écran
const byte LCD_ROWS = 4;    // Nombre de lignes de l'écran

// --- Constantes pour l'Encodeur et le Temps ---
const byte STEPS   = 1;             // Nombre de pas logiques par "clic" physique de l'encodeur
const byte SECOND_INCREMENT = 10;   // Incrément (en secondes) pour chaque pas logique
const unsigned int MAX_TOTAL_SECONDS = 600; // Temps maximum réglable en secondes (ex: 600s = 10 min) pour le mode MANUEL
const int POSMIN  = 0;             // Position logique minimale de l'encodeur (correspond à 0 secondes)
// Position logique maximale (calculée automatiquement)
const int POSMAX  = MAX_TOTAL_SECONDS / SECOND_INCREMENT;

// --- Constantes de Temporisation ---
const int debounceDelay = 50;        // Délai anti-rebond pour le bouton (en ms)
const unsigned long csUpdateInterval = 50; // Intervalle de rafraîchissement des centisecondes (en ms)
const unsigned long longPressDuration = 1000; // Durée pour appui long pour accéder au menu (ms)
const unsigned long blinkSequenceDuration = 5000; // Durée du clignotement final (ms) <<< AJOUTEZ/DÉCOMMMENTEZ
const unsigned long endBlinkInterval = 250;       // Intervalle de basculement du rétroéclairage (ms) <<< AJOUTEZ/DÉCOMMMENTEZ


// --- Constantes pour la disposition de l'Affichage LCD ---
const byte STATUS_ROW = 0;
const byte STATUS_COL_START = 0;
const byte BIG_NUM_ROW = 1;     // Ligne supérieure des grands chiffres
const byte BIG_M1_COL = 2;      // Colonne début Dizaines Minutes
const byte BIG_M2_COL = 5;      // Colonne début Unités Minutes (M1 + 3)
const byte COLON_COL = 8;       // Colonne du séparateur ":" ou "."
const byte BIG_S1_COL = 9;      // Colonne début Dizaines Secondes
const byte BIG_S2_COL = 12;     // Colonne début Unités Secondes (S1 + 3)
const byte CS_ROW = 2;          // Ligne d'affichage des centisecondes
const byte CS_COL = 15;         // Colonne de départ pour ".CS"
const byte MELODY_NAME_ROW = 3; // Ligne pour afficher le nom de la mélodie
const byte MELODY_NAME_COL = 1; // Colonne de départ pour icône + nom

// --- Configuration Menu & EEPROM ---
const int EEPROM_ADDR_MELODY                  = 0;       // Adresse mémoire pour choix mélodie
const int EEPROM_ADDR_PRESET                  = 1;       // Adresse mémoire pour choix preset/mode (0=Manuel, 1=P1...)
const int EEPROM_ADDR_MANUAL_TIME             = 2;       // Adresse pour temps manuel (prend 2 octets: 2 et 3)
// L'adresse 4 est réservée pour EEPROM_ADDR_SLEEP_DELAY ci-dessous
// --- Configuration Veille (Sleep) ---
const int EEPROM_ADDR_SLEEP_DELAY             = 4;      // Adresse EEPROM suivante disponible
const int EEPROM_ADDR_BUZZER_FEEDBACK         = 5;      // <<< AJOUTER (Adresse après Veille=4)
// --- EEPROM POUR MÉTRONOME ---
const int EEPROM_ADDR_TIMER_MELODY_ENABLED    = 6;      // Adresse pour activer/désactiver la mélodie de fin
const int EEPROM_ADDR_METRONOME_BPM           = 7;      // Prend 1 octet (pour BPM jusqu'à 255) ou 2 pour plus.
const int EEPROM_ADDR_METRONOME_TS_NUM        = 9;      // Numérateur de la signature rythmique (ex: 4 pour 4/4)
const int EEPROM_ADDR_METRONOME_TS_DEN        = 10;     // byte, <<< NOUVELLE ADRESSE EEPROM POUR LE DÉNOMINATEUR

// --- Configuration des Préréglages de Tempo --- <<< NOUVELLE SECTION
const byte NUM_TEMPO_PRESETS  = 8; // Nombre de préréglages de tempo

struct TempoPreset {
  const char* name;       // Nom du tempo (ex: "Allegro")
  const char* frenchDesc; // Description en français (ex: "rapide")
  int bpm;                // Pulsation par minute
};

// Déclaration seulement (la définition = { ... } sera dans le .ino)
extern const TempoPreset tempoPresets[];
// --- Fin Configuration des Préréglages de Tempo ---

const byte NUM_MELODIES = 6;      // Nombre total de mélodies disponibles (Mario, Star Wars, Zelda, Nokia Tune, Tetris Theme (Thème A), Bip-Bip)
const byte NUM_PRESETS = 4;       // Nombre total d'options de preset (Manuel, 1min, 2min, 3min)

// Valeurs des presets en secondes (l'index correspond au choix, ex: PRESET_VALUES[1] = 60)
// LA DÉFINITION (= { ... }) de ce tableau DOIT être dans le .ino
extern const unsigned int PRESET_VALUES[]; // Déclaration seulement



// Nombre d'options de délai de veille (doit correspondre aux tableaux dans le .ino)
const byte NUM_SLEEP_OPTIONS = 4;     // Défini explicitement (Off, 1min, 5min, 10min)

// Déclarations seulement (les définitions = { ... } DOIVENT être dans le .ino)
extern const unsigned int SLEEP_DELAY_VALUES[];
extern const char* const SLEEP_DELAY_NAMES[];

// --- Configuration Feedback Sonore ---
/* const bool ENABLE_BUZZER_FEEDBACK = true;  // Activer (true) ou désactiver (false) les clics */
const unsigned int CLICK_FREQUENCY = 2731;    // Fréquence du clic (Hz) - Ajustez selon vos préférences
const byte CLICK_DURATION = 15;               // Durée très courte du clic (ms) - Ajustez si besoin


// --- CONSTANTES POUR MÉTRONOME ---
const int MIN_BPM = 20;
const int MAX_BPM = 240;
const int DEFAULT_BPM = 120;
const unsigned int METRONOME_CLICK_FREQ = 2000;
const byte METRONOME_CLICK_DURATION = 30;      // ms
const unsigned int METRONOME_ACCENT_FREQ = 2700;
const byte METRONOME_ACCENT_DURATION = 50;     // ms

// Disposition LCD pour Métronome
const byte METRO_STATUS_ROW = 0;
const byte METRO_STATUS_COL = 0;
const byte METRO_BPM_BIG_NUM_ROW = 1;  // Ligne supérieure des grands chiffres pour BPM
const byte METRO_BPM_BIG_NUM_COL = 6;  // Colonne pour afficher 3 chiffres de BPM (centré approx.)
                                       // Chaque grand chiffre prend 3 colonnes. 3*3=9. (20-9)/2 = 5.5 -> 5 ou 6.
const byte METRO_TS_ROW = 0;           // Ligne pour la signature rythmique
const byte METRO_TS_COL = 13;          // Colonne pour afficher "TS: 4/4"
const byte METRO_BEAT_VISUAL_ROW = 3;  // Ligne pour l'indicateur visuel de battement
const byte METRO_BEAT_VISUAL_COL = 9;  // Colonne pour l'indicateur visuel de battement

// Constantes pour l'affichage des temps du métronome
const byte METRO_BEAT_MARKER_CHAR = 1;          // Index du caractère custom 'upperBar'
const byte METRO_BEAT_MARKER_START_COL = 1;     // Colonne de départ pour le premier marqueur de temps
                                                // (laisse la colonne 0 pour le bord de l'écran ou un petit espace)

const byte DEFAULT_TIME_SIGNATURE_NUMERATOR = 4; // ex: 4 pour 4/4
const byte MIN_TIME_SIGNATURE_NUMERATOR = 1;     // 
const byte MAX_TIME_SIGNATURE_NUMERATOR = 16;    // (ex: jusqu'à 16/4, modifiable)
// Le dénominateur est souvent 4 et peut être implicite pour simplifierr
const byte MIN_TIME_SIGNATURE_DENOMINATOR = 1;  // <<< NOUVEAU
const byte MAX_TIME_SIGNATURE_DENOMINATOR = 8;  // <<< NOUVEAU
const byte DEFAULT_TIME_SIGNATURE_DENOMINATOR = 4; // <<< NOUVEAU

#endif // CONF_H