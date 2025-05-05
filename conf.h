// conf.h - Fichier de configuration pour la minuterie (CORRIGÉ)

#ifndef CONF_H // Protection pour éviter les inclusions multiples
#define CONF_H

#include <Arduino.h> // Pour byte etc.

// --- Informations Auteur/Version ---
#define AUTHOR_NAME "ANCHER.P" // ICI VOTRE NOM
#define FIRMWARE_VERSION "1.4.1"  // ICI LA VERSION

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
const int EEPROM_ADDR_MELODY = 0; // Adresse mémoire pour choix mélodie
const int EEPROM_ADDR_PRESET = 1; // Adresse mémoire pour choix preset/mode (0=Manuel, 1=P1...)
const int EEPROM_ADDR_MANUAL_TIME = 2; // Adresse pour temps manuel (prend 2 octets: 2 et 3)
// L'adresse 4 est réservée pour EEPROM_ADDR_SLEEP_DELAY ci-dessous

const byte NUM_MELODIES = 5;      // Nombre total de mélodies disponibles (Mario, Star Wars, Zelda, Nokia Tune, Tetris Theme (Thème A))
const byte NUM_PRESETS = 4;       // Nombre total d'options de preset (Manuel, 1min, 2min, 3min)

// Valeurs des presets en secondes (l'index correspond au choix, ex: PRESET_VALUES[1] = 60)
// LA DÉFINITION (= { ... }) de ce tableau DOIT être dans le .ino
extern const unsigned int PRESET_VALUES[]; // Déclaration seulement

// --- Configuration Veille (Sleep) ---
const int EEPROM_ADDR_SLEEP_DELAY = 4; // Adresse EEPROM suivante disponible

// Nombre d'options de délai de veille (doit correspondre aux tableaux dans le .ino)
const byte NUM_SLEEP_OPTIONS = 4; // Défini explicitement (Off, 1min, 5min, 10min)

// Déclarations seulement (les définitions = { ... } DOIVENT être dans le .ino)
extern const unsigned int SLEEP_DELAY_VALUES[];
extern const char* const SLEEP_DELAY_NAMES[];

#endif // CONF_H
