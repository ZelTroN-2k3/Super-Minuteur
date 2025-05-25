//***************************************************************
//  Super Minuteur & Métronome - Code Principal (.ino)
//  - AFFICHAGE STATUT PRESET/MANUEL AJOUTÉ sur ligne 3
//  - AJOUT : Menu sélection Mélodie ET Preset Temps (runtime)
//  - AJOUT : Sauvegarde EEPROM des choix.
//  - AJOUT : 6 Mélodies (incluant "Bip-Bip") + 3 Presets + Manuel. // Mis à jour nombre de mélodies
//  - AJOUT : Clignotement Rétroéclairage en Fin (Non-bloquant)
//  - AJOUT : deuxième écran de démarrage (Infos Auteur/Version)
//  - AJOUT : Fonction Veille réglable via menu (Wake on Button) et logique de réveil améliorée.
//  - AJOUT : Indicateurs de Scrolling (Flèches Custom ↑/↓) dans menus et correction du défilement (ex: menu Veille).
//  - AJOUT : Fonction Pause/Reprise (Appui Court = Start/Pause/Resume, Appui Long en Pause = Stop)
//  - AJOUT : du Mode Métronome avec indicateur visuel par barres.
//  - AMÉLIORATION : Affichage "On/Off" pour "FeedbackSon" dans "Menu Réglages" et gestion de l'effacement de ligne.
//  - AJOUT : Option "Melodie O/F" dans "Menu Réglages" pour activer/désactiver la mélodie de fin.
//  - AMÉLIORATION : Affichage de "*Melodie Off" (ou "*Mel. Off") sur l'écran principal si la sonnerie de fin est désactivée.
//  - AMÉLIORATION : Navigation dans les menus (ex: retour au menu principal après sélection Preset ou sortie du Métronome).
//  - AJOUT : Option "Metro.Rythm" dans "Menu Réglages" pour modifier le numérateur ET le dénominateur (X/Y) et affichage de la valeur.
//  - AMÉLIORATION : Affichage du BPM actuel à côté de l'item "Metronome" dans "Menu Réglages".
//  - AJOUT : Menu "Tempo Class." pour sélectionner des préréglages de tempo classiques (ex: Grave, Allegro) qui règlent le BPM.
//  - AMÉLIORATION : Affichage du nom du tempo classique sélectionné sur l'écran principal du métronome.
//  - RÉORGANISATION : Logique du métronome déplacée vers metronome.h/.cpp.
//  - RÉORGANISATION : Logique du minuteur déplacée vers timer.h/.cpp.
//
//  Date de génération : Dimanche 25 mai 2025 CEST (MAJ 25/05/2025)
//  Lieu : Montmédy, Grand Est, France
//  Lien : https://github.com/ZelTroN-2k3/Super-Minuteur
//**************************************************************

#include <EEPROM.h>
#include "Wire.h"
#include <string.h> // Pour strlen
#include "LiquidCrystal_I2C.h"
#include "RotaryEncoder.h"
#include "BigNumbers_I2C.h"

#include "conf.h"    // Contient maintenant les enums et constantes globales
#include "melodie.h"
#include "metronome.h" 
#include "timer.h"     

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>

// Définition des tableaux déclarés extern dans conf.h
const unsigned int PRESET_VALUES[NUM_PRESETS] = { 0, 60, 120, 180 }; // Manuel, 1min, 2min, 3min
const unsigned int SLEEP_DELAY_VALUES[NUM_SLEEP_OPTIONS] = { 0, 60, 300, 600 }; // Off, 1min, 5min, 10min (en secondes)
const char* const SLEEP_DELAY_NAMES[NUM_SLEEP_OPTIONS] = { "Off", "1 min", "5 min", "10 min" };

// --- Définition des Préréglages de Tempo --- <<< NOUVEAU (après les autres tableaux)
const TempoPreset tempoPresets[NUM_TEMPO_PRESETS] = {
  {"Grave",       "tres lent",          20},
  {"Largo",       "large",              50}, // J'utilise "Largo" pour garder le nom court
  {"Adagio",      "lent, a l'aise",     70}, // "Adagio" pour la brièveté
  {"Andante",     "allant",            108}, // "Andante"
  {"Moderato",    "modere",            120},
  {"Allegro",     "rapide",            168},
  {"Presto",      "tres rapide",       188},
  {"Prestissimo", "extreme. rapide",   208}  // "extreme. rapide" pour "extrêmement rapide"
};

// Définitions des variables globales qui utilisent les enums de conf.h
Mode currentMode = MODE_TIMER;
TimerRunState currentTimerState = STATE_IDLE; 
MetronomeRunState currentMetroState = METRO_STOPPED;

// --- Initialisation des Objets Matériels ---
LiquidCrystal_I2C LCD(LCD_ADDR, LCD_COLS, LCD_ROWS);
BigNumbers_I2C bigNum(&LCD);
RotaryEncoder encoder(ENCODER_DT_PIN, ENCODER_CLK_PIN);

// Définition caractères flèches custom
byte arrowUp_Pattern[8] = { B00100, B01110, B11111, B00100, B00100, B00100, B00000, B00000 };
byte arrowDown_Pattern[8] = { B00000, B00100, B00100, B00100, B11111, B01110, B00100, B00000 };
const byte ARROW_UP_CHAR_CODE = 0; 
const byte ARROW_DOWN_CHAR_CODE = 1; 

// --- Variables Globales (celles qui restent dans le .ino principal ou sont partagées) ---
// Variables Timer (maintenant utilisées via extern dans timer.h/timer.cpp)
bool blinkDone = false; 
int lastPos = 0;        
int newPos = 0;         
boolean buttonWasUp = true;
boolean longPressDetected = false;
unsigned long buttonDownTime = 0;

int displaySEC = 0;
int displayMIN = 0;
int displayCS = 0;
unsigned int targetTotalSeconds = 0;
unsigned long targetEndTime = 0;
long remainingMillis = 0;
unsigned long pausedRemainingMillis = 0; 
int lastDisplayedSEC = -1;
int lastDisplayedMIN = -1;
unsigned long lastCsUpdateTime = 0;

bool isEndSequenceBlinking = false;
unsigned long blinkSequenceStartTime = 0;
unsigned long lastEndBlinkToggleTime = 0;
bool endBlinkStateIsOn = true;

// Variables Menu (restent ici car gèrent les menus principaux)
byte currentMelodyChoice = 0;
byte currentPresetChoice = 0;
byte menuMainIndex = 0;
byte menuMelodyIndex = 0;
byte menuPresetIndex = 0;
byte presetMenuScrollOffset = 0;
byte menuMainScrollOffset = 0;
byte melodyMenuScrollOffset = 0;
byte veilleMenuScrollOffset = 0;
byte menuTempoPresetIndex = 0;        // <<< NOUVEAU : Index pour le menu des presets de tempo
byte tempoPresetMenuScrollOffset = 0; // <<< NOUVEAU : Offset de défilement pour ce menu

int lastEncoderMenuPos = 0; 

const char* mainMenuItems[] = { " Melodie", " Preset ", " Veille ", " FeedbackSon", " Melodie O/F", " Metronome", " Metro.Rythm", " Tempo Class.", " Quitter" };
const byte numMainMenuOptions = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
const char* melodyNames[] = { "Mario    ", "StarWars ", "Zelda    ", "Nokia    ", "Tetris   ", "Bip-Bip  " }; 
const char* presetNames[] = { "Manuel", "1 Min", "2 Min", "3 Min" };

// Variables Veille (restent ici car goToSleep est ici)
byte currentSleepSetting = 0;
unsigned long configuredSleepDelayMillis = 0;
unsigned long lastActivityTime = 0;
volatile bool awokeByInterrupt = false; 
bool buzzerFeedbackEnabled = true;
bool timerMelodyEnabled = true;

// Variables Globales pour MÉTRONOME (définies ici, extern dans metronome.h)
int currentBPM = DEFAULT_BPM;
unsigned long lastMetroBeatTime = 0;
byte currentBeatInMeasure = 0;
byte timeSignatureNum = DEFAULT_TIME_SIGNATURE_NUMERATOR;
byte timeSignatureDen = DEFAULT_TIME_SIGNATURE_DENOMINATOR;
int metroEncoderLastPos = 0; 

// --- Déclarations de fonctions qui restent dans le .ino principal ---
void resetActivityTimer();
void handleEncoder();
void handleButton();
void enterMainMenu();
void displayMainMenu();
void navigateMainMenu(int diff);
void selectMainMenuItem();
void enterMelodyMenu();
void displayMelodyMenu();
void navigateMelodyMenu(int diff);
void selectMelodyMenuItem();
void enterPresetMenu();
void displayPresetMenu();
void navigatePresetMenu(int diff);
void selectPresetMenuItem();
void enterVeilleMenu();
void displayVeilleMenu();
void navigateVeilleMenu(int diff);
void selectVeilleMenuItem();
void exitMenu();
void saveChoiceToEEPROM(int address, byte value); 
void displayStatusLine3(); 
void clearRestOfLine(byte startCol, byte row);
void playClickSound();
void goToSleep();
void enterTempoPresetMenu();    // <<< NOUVELLE FONCTION
void displayTempoPresetMenu();  // <<< NOUVELLE FONCTION
void navigateTempoPresetMenu(int diff); // <<< NOUVELLE FONCTION
void selectTempoPresetMenuItem(); // <<< NOUVELLE FONCTION

// --- Fonction d'initialisation ---
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); 
  digitalWrite(BUZZER_PIN, LOW); 

  LCD.begin();
  LCD.backlight();
  LCD.clear();

  // Boot screen 1
  LCD.setCursor(0, 1); LCD.print(F("   Super Minuteur   "));
  LCD.setCursor(0, 2); LCD.print(F("    & Metronome     "));
  delay(1000);
  LCD.setCursor(0, 2); LCD.print(F("  Initialisation... "));
  delay(1500); LCD.clear();

  // Boot screen 2
  LCD.setCursor(0, 0); LCD.print(F("Auteur: ")); LCD.print(AUTHOR_NAME);
  LCD.setCursor(0, 1); LCD.print(F("Vers: ")); LCD.print(FIRMWARE_VERSION);
  LCD.setCursor(0, 2); LCD.print(F("Date: ")); LCD.print(F(__DATE__)); 
  LCD.setCursor(0, 3); LCD.print(F("Time: ")); LCD.print(F(__TIME__)); 
  delay(1500); LCD.clear();

  // Charger les préférences EEPROM
  byte savedMelody = EEPROM.read(EEPROM_ADDR_MELODY);
  if (savedMelody >= NUM_MELODIES) { currentMelodyChoice = 0; EEPROM.update(EEPROM_ADDR_MELODY, currentMelodyChoice); }
  else { currentMelodyChoice = savedMelody; }
  menuMelodyIndex = currentMelodyChoice; 

  // currentPresetChoice est initialisé dans setupTimer() maintenant
  // menuPresetIndex = currentPresetChoice; // Sera basé sur currentPresetChoice après setupTimer()

  currentSleepSetting = EEPROM.read(EEPROM_ADDR_SLEEP_DELAY);
  if (currentSleepSetting >= NUM_SLEEP_OPTIONS) { currentSleepSetting = 0; EEPROM.update(EEPROM_ADDR_SLEEP_DELAY, currentSleepSetting); }
  configuredSleepDelayMillis = (unsigned long)SLEEP_DELAY_VALUES[currentSleepSetting] * 1000UL;

  byte savedFeedbackState = EEPROM.read(EEPROM_ADDR_BUZZER_FEEDBACK);
  if (savedFeedbackState == 0) { buzzerFeedbackEnabled = false; }
  else { buzzerFeedbackEnabled = true; if (savedFeedbackState > 1 && savedFeedbackState != 0xFF ) { EEPROM.update(EEPROM_ADDR_BUZZER_FEEDBACK, 1); } }
  
  byte savedTimerMelodyState = EEPROM.read(EEPROM_ADDR_TIMER_MELODY_ENABLED);
  if (savedTimerMelodyState == 0) { timerMelodyEnabled = false; }
  else { timerMelodyEnabled = true; if (savedTimerMelodyState != 1 && savedTimerMelodyState != 0 && savedTimerMelodyState != 0xFF) { EEPROM.update(EEPROM_ADDR_TIMER_MELODY_ENABLED, 1); } }

  setupMetronome(); 
  setupTimer();     // <<< APPEL À L'INITIALISATION DU TIMER

  menuPresetIndex = currentPresetChoice; // Initialiser l'index du menu preset après setupTimer

  bigNum.begin(); 
  encoder.setPosition(lastPos * STEPS); // Initialiser la position de l'encodeur pour le timer

  resetActivityTimer(); 

  currentMode = MODE_TIMER; 
  currentTimerState = STATE_IDLE;
  updateStaticDisplay();      // Appel à la fonction maintenant dans timer.cpp
  updateCentisecondsDisplay(); // Appel à la fonction maintenant dans timer.cpp
  displayStatusLine3();
}

// --- Boucle Principale (Gère les modes) ---
void loop() {
  handleEncoder(); 
  handleButton();  

  if (currentMode == MODE_TIMER) {
    loopTimer(); // Appel à la logique de boucle du timer (dans timer.cpp)

    // La gestion de la veille pour MODE_TIMER et STATE_IDLE reste ici pour l'instant
    if (currentTimerState == STATE_IDLE) { 
        if (configuredSleepDelayMillis > 0 && !isEndSequenceBlinking) {
             if (millis() - lastActivityTime > configuredSleepDelayMillis) {
                  goToSleep();
             }
        }
    } 
  } 
  else if (currentMode == MODE_METRONOME) {
    handleMetronomeLogic(); 
    if (currentMetroState == METRO_STOPPED && configuredSleepDelayMillis > 0) {
        if (millis() - lastActivityTime > configuredSleepDelayMillis) {
            goToSleep();
        }
    }
  } 
}


// --- Fonctions Auxiliaires (qui restent dans le .ino principal) ---
void resetActivityTimer() {
    lastActivityTime = millis();
    if (isEndSequenceBlinking) {
      isEndSequenceBlinking = false;
      LCD.backlight(); 
    }
}

void handleEncoder() {
  encoder.tick(); 
  int currentEncoderPhysicalPos = encoder.getPosition(); 

  if (currentMode == MODE_TIMER) {
    handleTimerEncoderInput(currentEncoderPhysicalPos); // <<< APPEL À LA FONCTION DANS timer.cpp
  } else if (currentMode == MODE_METRONOME) {
    if (currentMetroState == METRO_STOPPED) {
        int newBPM = currentEncoderPhysicalPos;
        if (newBPM < MIN_BPM) { encoder.setPosition(MIN_BPM); newBPM = MIN_BPM; }
        if (newBPM > MAX_BPM) { encoder.setPosition(MAX_BPM); newBPM = MAX_BPM; }
        if (currentBPM != newBPM) { 
            playClickSound();
            resetActivityTimer();
            currentBPM = newBPM;
            displayMetronomeScreen();   
            saveBPMToEEPROM(currentBPM); 
        }
    }
  } else { 
       int diff = currentEncoderPhysicalPos - lastEncoderMenuPos;
       if(diff != 0) { 
         playClickSound();
         resetActivityTimer();
         switch(currentMode) {
             case MODE_MENU_MAIN:   navigateMainMenu(diff); break;
             case MODE_MENU_MELODY: navigateMelodyMenu(diff); break;
             case MODE_MENU_PRESET: navigatePresetMenu(diff); break;
             case MODE_MENU_VEILLE: navigateVeilleMenu(diff); break;
             case MODE_MENU_TS_METRO: navigateTSMetroMenu(diff); break; 
             case MODE_MENU_TEMPO_PRESET: navigateTempoPresetMenu(diff); break;
             default: break;
         }
         lastEncoderMenuPos = currentEncoderPhysicalPos; 
       }
  }
}

void handleButton() {
  boolean buttonIsUp = digitalRead(BUTTON_PIN);

  if (buttonWasUp && !buttonIsUp) {
    buttonDownTime = millis();
    longPressDetected = false; 
    resetActivityTimer();      
  }

  if (!buttonWasUp && !longPressDetected && (millis() - buttonDownTime >= longPressDuration)) {
     longPressDetected = true;
     playClickSound(); 

     if (currentMode == MODE_TIMER) {
         if (currentTimerState == STATE_IDLE) { 
             enterMainMenu();
         } else { // STATE_PAUSED (RUNNING ne fait rien sur appui long)
             handleTimerButtonLongPress(); // <<< APPEL À LA FONCTION DANS timer.cpp
         }
     }  else if (currentMode == MODE_METRONOME) {
        // exitMenu(); // <<< ANCIENNE LIGNE : Retournait directement au mode Timer
        enterMainMenu(); // <<< NOUVELLE LIGNE : Retourne au "Menu Réglages"
     }
  }

  if (!buttonWasUp && buttonIsUp) {
    if (!longPressDetected && (millis() - buttonDownTime >= debounceDelay)) { 
        playClickSound(); 
        switch (currentMode) {
            case MODE_TIMER:
                handleTimerButtonShortPress(); // <<< APPEL À LA FONCTION DANS timer.cpp
                break;
            case MODE_METRONOME: 
                if (currentMetroState == METRO_STOPPED) {
                    currentMetroState = METRO_RUNNING;
                    lastMetroBeatTime = millis(); 
                    currentBeatInMeasure = 0;     
                    for (byte b = 0; b < timeSignatureNum; ++b) {
                         LCD.setCursor(METRO_BEAT_MARKER_START_COL + b, METRO_BEAT_VISUAL_ROW);
                         LCD.print(" ");
                    }
                } else { 
                    currentMetroState = METRO_STOPPED;
                    noTone(BUZZER_PIN); 
                }
                displayMetronomeScreen(); 
                break;
            case MODE_MENU_MAIN:   selectMainMenuItem(); break;
            case MODE_MENU_MELODY: selectMelodyMenuItem(); break;
            case MODE_MENU_PRESET: selectPresetMenuItem(); break;
            case MODE_MENU_VEILLE: selectVeilleMenuItem(); break;
            case MODE_MENU_TS_METRO: selectTSMetroMenuItem(); break; 
            case MODE_MENU_TEMPO_PRESET: selectTempoPresetMenuItem(); break;
        }
    }
    longPressDetected = false; 
  }
  buttonWasUp = buttonIsUp; 
}


// --- Fonctions de Menu (restent dans le .ino principal) ---
void enterMainMenu() {
    resetActivityTimer();
    currentMode = MODE_MENU_MAIN;
    if (currentMetroState == METRO_RUNNING) { 
        currentMetroState = METRO_STOPPED;    
        noTone(BUZZER_PIN);                   
    }
    LCD.createChar(ARROW_UP_CHAR_CODE, arrowUp_Pattern);
    LCD.createChar(ARROW_DOWN_CHAR_CODE, arrowDown_Pattern);
    byte displayLines = LCD_ROWS - 1; 
    if (menuMainIndex >= numMainMenuOptions) menuMainIndex = 0; 
    if (menuMainIndex < menuMainScrollOffset) { menuMainScrollOffset = menuMainIndex; }
    else if (menuMainIndex >= menuMainScrollOffset + displayLines) { menuMainScrollOffset = menuMainIndex - displayLines + 1; }
    if (numMainMenuOptions <= displayLines) { menuMainScrollOffset = 0; }
    else { if (menuMainScrollOffset > numMainMenuOptions - displayLines) { menuMainScrollOffset = numMainMenuOptions - displayLines; } }
    lastEncoderMenuPos = encoder.getPosition(); 
    displayMainMenu();
}


void displayMainMenu() {
    LCD.clear();
    LCD.setCursor(0, 0); LCD.print(F("Menu Reglages:"));
    byte displayLines = LCD_ROWS - 1;
    for (byte i = 0; i < displayLines; i++) {
        byte displayRow = i + 1;
        byte itemIndexToShow = i + menuMainScrollOffset;
        LCD.setCursor(0, displayRow);
        if (itemIndexToShow < numMainMenuOptions) {
            byte startColInfo = 1; 
            if (itemIndexToShow == menuMainIndex) { LCD.print(">");}
            else { LCD.print(" ");}
            LCD.print(mainMenuItems[itemIndexToShow]);
            startColInfo += strlen(mainMenuItems[itemIndexToShow]);
            LCD.setCursor(startColInfo, displayRow);

            if (strcmp(mainMenuItems[itemIndexToShow], " Melodie") == 0) {
                LCD.print(F(": "));
                if (currentMelodyChoice < NUM_MELODIES) { LCD.print(melodyNames[currentMelodyChoice]); }
                else { LCD.print(F("???")); }
            } else if (strcmp(mainMenuItems[itemIndexToShow], " Preset ") == 0) {
                LCD.print(F(": "));
                if (currentPresetChoice < NUM_PRESETS) { LCD.print(presetNames[currentPresetChoice]); }
                else { LCD.print(F("???")); }
            } else if (strcmp(mainMenuItems[itemIndexToShow], " Veille ") == 0) {
                LCD.print(F(": "));
                if (currentSleepSetting < NUM_SLEEP_OPTIONS) { LCD.print(SLEEP_DELAY_NAMES[currentSleepSetting]); }
                else { LCD.print(F("???")); }
            } else if (strcmp(mainMenuItems[itemIndexToShow], " FeedbackSon") == 0) {
                LCD.print(F(": "));
                if (buzzerFeedbackEnabled) { LCD.print(F("On ")); }
                else { LCD.print(F("Off")); }
            } else if (strcmp(mainMenuItems[itemIndexToShow], " Melodie O/F") == 0) { 
                LCD.print(F(": ")); 
                if (timerMelodyEnabled) { LCD.print(F("On ")); }
                else { LCD.print(F("Off")); }
            } else if (strcmp(mainMenuItems[itemIndexToShow], " Metronome") == 0) { 
               LCD.print(F(": "));
               LCD.print(currentBPM);
            } else if (strcmp(mainMenuItems[itemIndexToShow], " Metro.Rythm") == 0) {
                LCD.print(F(": ")); 
                LCD.print(timeSignatureNum);
                LCD.print(F("/")); // <<< MODIFIÉ
                LCD.print(timeSignatureDen); // <<< MODIFIÉ
            } else if (strcmp(mainMenuItems[itemIndexToShow], " Quitter") == 0) {
            }
            clearRestOfLine(18, displayRow); 
            } else { 
            clearRestOfLine(0, displayRow);
        }
    }
    LCD.setCursor(LCD_COLS - 1, 1); LCD.print(" "); 
    LCD.setCursor(LCD_COLS - 1, displayLines); LCD.print(" "); 
    if (numMainMenuOptions > displayLines) { 
        if (menuMainScrollOffset > 0) { LCD.setCursor(LCD_COLS - 1, 1); LCD.write(byte(ARROW_UP_CHAR_CODE)); }
        if ((menuMainScrollOffset + displayLines) < numMainMenuOptions) { LCD.setCursor(LCD_COLS - 1, displayLines); LCD.write(byte(ARROW_DOWN_CHAR_CODE)); }
    }
}

void navigateMainMenu(int diff) {
    byte displayLines = LCD_ROWS - 1;
    int tempMenuIndex = menuMainIndex;
    if (diff > 0) { tempMenuIndex++; if(tempMenuIndex >= numMainMenuOptions) tempMenuIndex = 0; }
    else if (diff < 0) { if(tempMenuIndex == 0) tempMenuIndex = numMainMenuOptions; tempMenuIndex--; }
    menuMainIndex = tempMenuIndex;
    if (numMainMenuOptions > displayLines) { 
        if (menuMainIndex < menuMainScrollOffset) { menuMainScrollOffset = menuMainIndex; }
        else if (menuMainIndex >= menuMainScrollOffset + displayLines) { menuMainScrollOffset = menuMainIndex - displayLines + 1; }
        if (diff > 0 && menuMainIndex == 0) { menuMainScrollOffset = 0; }
        else if (diff < 0 && menuMainIndex == (numMainMenuOptions - 1) ) { 
             if (numMainMenuOptions > displayLines) { menuMainScrollOffset = numMainMenuOptions - displayLines; }
             else { menuMainScrollOffset = 0; }
        }
    } else { menuMainScrollOffset = 0; }
    displayMainMenu(); 
}

void selectMainMenuItem() {
    resetActivityTimer();
    const char* selectedOption = mainMenuItems[menuMainIndex]; 
    if (strcmp(selectedOption, " Melodie") == 0) { enterMelodyMenu(); }
    else if (strcmp(selectedOption, " Preset ") == 0) { enterPresetMenu(); }
    else if (strcmp(selectedOption, " Veille ") == 0) { enterVeilleMenu(); } 
    else if (strcmp(selectedOption, " FeedbackSon") == 0) {
        buzzerFeedbackEnabled = !buzzerFeedbackEnabled;
        saveChoiceToEEPROM(EEPROM_ADDR_BUZZER_FEEDBACK, buzzerFeedbackEnabled ? 1 : 0);
        displayMainMenu(); 
    } else if (strcmp(selectedOption, " Melodie O/F") == 0) { 
        timerMelodyEnabled = !timerMelodyEnabled;
        saveChoiceToEEPROM(EEPROM_ADDR_TIMER_MELODY_ENABLED, timerMelodyEnabled ? 1 : 0);
        displayMainMenu(); 
    } else if (strcmp(selectedOption, " Metronome") == 0) {
        enterMetronomeMode(); 
    } else if (strcmp(selectedOption, " Metro.Rythm") == 0) { 
        enterTSMetroMenu(); 
    } else if (strcmp(selectedOption, " Tempo Class.") == 0) { // <<< NOUVEAU CAS (utilisez le nom exact que vous avez mis dans mainMenuItems)
        enterTempoPresetMenu();    
    } else if (strcmp(selectedOption, " Quitter") == 0) {
        LCD.clear(); exitMenu(); 
    }
}

// --- NOUVELLES FONCTIONS POUR LE MENU DES PRÉRÉGLAGES DE TEMPO ---

void enterTempoPresetMenu() {
    resetActivityTimer();
    currentMode = MODE_MENU_TEMPO_PRESET;
    // Initialiser l'index du menu sur le preset actuel si possible, ou 0
    // Pour la simplicité, on commence à 0 ou au dernier réglé si on sauvegardait l'index.
    // Ici, on va juste commencer à 0 et ajuster le scroll.
    menuTempoPresetIndex = 0; // Ou trouver le preset le plus proche du BPM actuel si souhaité.

    byte displayLines = LCD_ROWS - 1; // Ligne 0 pour le titre
    // Ajuster le scrollOffset pour que l'item sélectionné (menuTempoPresetIndex) soit visible
    if (menuTempoPresetIndex < tempoPresetMenuScrollOffset) { 
        tempoPresetMenuScrollOffset = menuTempoPresetIndex;
    } else if (menuTempoPresetIndex >= tempoPresetMenuScrollOffset + displayLines) { 
        tempoPresetMenuScrollOffset = menuTempoPresetIndex - displayLines + 1;
    }
   
    // S'assurer que scrollOffset est valide
    if (NUM_TEMPO_PRESETS <= displayLines) {
        tempoPresetMenuScrollOffset = 0;
    } else {
        if (tempoPresetMenuScrollOffset > NUM_TEMPO_PRESETS - displayLines) {
            tempoPresetMenuScrollOffset = NUM_TEMPO_PRESETS - displayLines;
        }
    }
   
    lastEncoderMenuPos = encoder.getPosition(); 
    displayTempoPresetMenu();
}

void displayTempoPresetMenu() {
    LCD.clear();
    LCD.setCursor(0, 0); LCD.print(F("Tempo Classique:"));
    byte displayLines = LCD_ROWS - 1; // 3 lignes pour les items

    for (byte i = 0; i < displayLines; i++) {
        byte displayRow = i + 1;
        byte itemIndexToShow = i + tempoPresetMenuScrollOffset;
        LCD.setCursor(0, displayRow);

        if (itemIndexToShow < NUM_TEMPO_PRESETS) {
            if (itemIndexToShow == menuTempoPresetIndex) { LCD.print(">"); }
            else { LCD.print(" "); }
            
            LCD.print(tempoPresets[itemIndexToShow].name);
            LCD.print(F(" ("));
            LCD.print(tempoPresets[itemIndexToShow].bpm);
            // LCD.print(F(" BPM)")); // "BPM" pourrait rendre la ligne trop longue
            LCD.print(F(")"));

            // La description française n'est pas affichée ici pour garder la liste concise
            // On pourrait l'afficher sur une 2ème ligne pour l'item sélectionné,
            // mais cela réduirait le nombre d'items visibles.
            
            // Calcul approximatif de la longueur pour clearRestOfLine
            byte currentLength = 1; // Pour ">" ou " "
            currentLength += strlen(tempoPresets[itemIndexToShow].name);
            currentLength += 3; // Pour " (" et ")"
            if (tempoPresets[itemIndexToShow].bpm >= 100) currentLength += 3;
            else if (tempoPresets[itemIndexToShow].bpm >= 10) currentLength += 2;
            else currentLength += 1;
            // if (strlen(" BPM)") > 0) currentLength += strlen(" BPM)");

            clearRestOfLine(currentLength, displayRow);
        } else {
            clearRestOfLine(0, displayRow); // Ligne vide
        }
    }
  
    // Afficher les flèches de défilement
    LCD.setCursor(LCD_COLS - 1, 1); LCD.print(" "); 
    LCD.setCursor(LCD_COLS - 1, displayLines); LCD.print(" "); 
    if (NUM_TEMPO_PRESETS > displayLines) { 
        if (tempoPresetMenuScrollOffset > 0) { 
            LCD.setCursor(LCD_COLS - 1, 1); 
            LCD.write(byte(ARROW_UP_CHAR_CODE)); 
        }
        if ((tempoPresetMenuScrollOffset + displayLines) < NUM_TEMPO_PRESETS) { 
            LCD.setCursor(LCD_COLS - 1, displayLines); 
            LCD.write(byte(ARROW_DOWN_CHAR_CODE)); 
        }
    }
}

void navigateTempoPresetMenu(int diff) {
    byte displayLines = LCD_ROWS - 1;
    int tempMenuIndex = menuTempoPresetIndex;

    if (diff > 0) { 
        tempMenuIndex++;
        if(tempMenuIndex >= NUM_TEMPO_PRESETS) tempMenuIndex = 0; 
    } else if (diff < 0) { 
        if(tempMenuIndex == 0) tempMenuIndex = NUM_TEMPO_PRESETS; 
        tempMenuIndex--;
    }
    menuTempoPresetIndex = tempMenuIndex;

    if (NUM_TEMPO_PRESETS > displayLines) { 
        if (menuTempoPresetIndex < tempoPresetMenuScrollOffset) { 
            tempoPresetMenuScrollOffset = menuTempoPresetIndex;
        } else if (menuTempoPresetIndex >= tempoPresetMenuScrollOffset + displayLines) { 
            tempoPresetMenuScrollOffset = menuTempoPresetIndex - displayLines + 1;
        }
        if (diff > 0 && menuTempoPresetIndex == 0) { 
             tempoPresetMenuScrollOffset = 0;
        } else if (diff < 0 && menuTempoPresetIndex == (NUM_TEMPO_PRESETS - 1) ) { 
             if (NUM_TEMPO_PRESETS > displayLines) { 
                tempoPresetMenuScrollOffset = NUM_TEMPO_PRESETS - displayLines;
             } else {
                tempoPresetMenuScrollOffset = 0;
             }
        }
    } else { 
        tempoPresetMenuScrollOffset = 0;
    }
    displayTempoPresetMenu(); 
}

void selectTempoPresetMenuItem() {
    resetActivityTimer();
    
    currentBPM = tempoPresets[menuTempoPresetIndex].bpm;
    saveBPMToEEPROM(currentBPM);       // Sauvegarder le nouveau BPM
    encoder.setPosition(currentBPM);   // Synchroniser l'encodeur avec le nouveau BPM
    metroEncoderLastPos = currentBPM;  // Pour la gestion de l'encodeur en mode métronome
    
    playClickSound(); 
    // Optionnel: afficher brièvement le tempo sélectionné avant de changer de mode
    // LCD.clear();
    // LCD.setCursor(0,1); LCD.print(F("Tempo selectionne:"));
    // LCD.setCursor(0,2); LCD.print(tempoPresets[menuTempoPresetIndex].name);
    // LCD.print(F(" (")); LCD.print(currentBPM); LCD.print(F(" BPM)"));
    // delay(1500);

    enterMetronomeMode(); // Aller directement au mode métronome avec le BPM réglé
}

void enterMelodyMenu() {
    resetActivityTimer(); currentMode = MODE_MENU_MELODY;
    menuMelodyIndex = currentMelodyChoice; 
    byte displayLines = LCD_ROWS - 1;
    if (menuMelodyIndex < melodyMenuScrollOffset) { melodyMenuScrollOffset = menuMelodyIndex; }
    else if (menuMelodyIndex >= melodyMenuScrollOffset + displayLines) { melodyMenuScrollOffset = menuMelodyIndex - displayLines + 1; }
    if (NUM_MELODIES <= displayLines) { melodyMenuScrollOffset = 0; } 
    else if (melodyMenuScrollOffset > NUM_MELODIES - displayLines) { melodyMenuScrollOffset = NUM_MELODIES - displayLines; } 
    lastEncoderMenuPos = encoder.getPosition();
    displayMelodyMenu();
}

void displayMelodyMenu() {
    LCD.clear(); LCD.setCursor(0, 0); LCD.print(F(" Choix Melodie:"));
    byte displayLines = LCD_ROWS - 1;
    for (byte i = 0; i < displayLines; i++) {
        byte displayRow = i + 1;
        byte itemIndexToShow = i + melodyMenuScrollOffset;
        LCD.setCursor(0, displayRow);
        if (itemIndexToShow < NUM_MELODIES) {
            if (itemIndexToShow == menuMelodyIndex) { LCD.print(">"); }
            else { LCD.print(" "); }
            LCD.print(melodyNames[itemIndexToShow]);
            clearRestOfLine(1 + strlen(melodyNames[itemIndexToShow]) +1, displayRow);
        } else {
            clearRestOfLine(0, displayRow); 
        }
    }
    LCD.setCursor(LCD_COLS - 1, 1); LCD.print(" ");
    LCD.setCursor(LCD_COLS - 1, displayLines); LCD.print(" ");
    if (NUM_MELODIES > displayLines) {
        if (melodyMenuScrollOffset > 0) { LCD.setCursor(LCD_COLS - 1, 1); LCD.write(byte(ARROW_UP_CHAR_CODE)); }
        if ((melodyMenuScrollOffset + displayLines) < NUM_MELODIES) { LCD.setCursor(LCD_COLS - 1, displayLines); LCD.write(byte(ARROW_DOWN_CHAR_CODE)); }
    }
}

void navigateMelodyMenu(int diff) {
    byte displayLines = LCD_ROWS - 1;
    int tempMenuIndex = menuMelodyIndex;
    if (diff > 0) { tempMenuIndex++; if(tempMenuIndex >= NUM_MELODIES) tempMenuIndex = 0; }
    else if (diff < 0) { if(tempMenuIndex == 0) tempMenuIndex = NUM_MELODIES; tempMenuIndex--; }
    menuMelodyIndex = tempMenuIndex;
    if (NUM_MELODIES > displayLines) {
        if (menuMelodyIndex < melodyMenuScrollOffset) { melodyMenuScrollOffset = menuMelodyIndex; }
        else if (menuMelodyIndex >= (melodyMenuScrollOffset + displayLines)) { melodyMenuScrollOffset = menuMelodyIndex - displayLines + 1; }
        if (diff > 0 && menuMelodyIndex == 0) melodyMenuScrollOffset = 0;
        else if (diff < 0 && menuMelodyIndex == (NUM_MELODIES - 1)) {
            if (NUM_MELODIES > displayLines) melodyMenuScrollOffset = NUM_MELODIES - displayLines;
            else melodyMenuScrollOffset = 0;
        }
    } else { melodyMenuScrollOffset = 0; }
    displayMelodyMenu();
}

void selectMelodyMenuItem() {
    resetActivityTimer();
    currentMelodyChoice = menuMelodyIndex;
    saveChoiceToEEPROM(EEPROM_ADDR_MELODY, currentMelodyChoice);
    playClickSound(); 
    enterMainMenu(); 
}

void enterPresetMenu() {
    resetActivityTimer(); currentMode = MODE_MENU_PRESET;
    menuPresetIndex = currentPresetChoice;
    byte displayLines = LCD_ROWS - 1;
    if (menuPresetIndex < presetMenuScrollOffset) { presetMenuScrollOffset = menuPresetIndex; }
    else if (menuPresetIndex >= presetMenuScrollOffset + displayLines) { presetMenuScrollOffset = menuPresetIndex - displayLines + 1; }
    if (NUM_PRESETS <= displayLines) { presetMenuScrollOffset = 0; }
    else if (presetMenuScrollOffset > NUM_PRESETS - displayLines) { presetMenuScrollOffset = NUM_PRESETS - displayLines; }
    lastEncoderMenuPos = encoder.getPosition();
    displayPresetMenu();
}

void displayPresetMenu() {
    LCD.clear(); LCD.setCursor(0, 0); LCD.print(F(" Choix Preset:"));
    byte displayLines = LCD_ROWS - 1;
    for (byte i = 0; i < displayLines; i++) {
        byte displayRow = i + 1;
        byte itemIndexToShow = i + presetMenuScrollOffset;
        LCD.setCursor(0, displayRow);
        if (itemIndexToShow < NUM_PRESETS) {
            if (itemIndexToShow == menuPresetIndex) { LCD.print(">"); }
            else { LCD.print(" "); }
            LCD.print(presetNames[itemIndexToShow]);
            clearRestOfLine(1 + strlen(presetNames[itemIndexToShow]) + 1, displayRow);
        } else {
            clearRestOfLine(0, displayRow);
        }
    }
    LCD.setCursor(LCD_COLS - 1, 1); LCD.print(" ");
    LCD.setCursor(LCD_COLS - 1, displayLines); LCD.print(" ");
    if (NUM_PRESETS > displayLines) {
        if (presetMenuScrollOffset > 0) { LCD.setCursor(LCD_COLS - 1, 1); LCD.write(byte(ARROW_UP_CHAR_CODE)); }
        if ((presetMenuScrollOffset + displayLines) < NUM_PRESETS) { LCD.setCursor(LCD_COLS - 1, displayLines); LCD.write(byte(ARROW_DOWN_CHAR_CODE)); }
    }
}

void navigatePresetMenu(int diff) {
    byte displayLines = LCD_ROWS - 1;
    int tempMenuIndex = menuPresetIndex;
    if (diff > 0) { tempMenuIndex++; if(tempMenuIndex >= NUM_PRESETS) tempMenuIndex = 0; }
    else if (diff < 0) { if(tempMenuIndex == 0) tempMenuIndex = NUM_PRESETS; tempMenuIndex--; }
    menuPresetIndex = tempMenuIndex;
    if (NUM_PRESETS > displayLines) {
        if (menuPresetIndex < presetMenuScrollOffset) { presetMenuScrollOffset = menuPresetIndex; }
        else if (menuPresetIndex >= (presetMenuScrollOffset + displayLines)) { presetMenuScrollOffset = menuPresetIndex - displayLines + 1; }
        if (diff > 0 && menuPresetIndex == 0) presetMenuScrollOffset = 0;
        else if (diff < 0 && menuPresetIndex == (NUM_PRESETS - 1)) {
             if (NUM_PRESETS > displayLines) presetMenuScrollOffset = NUM_PRESETS - displayLines;
             else presetMenuScrollOffset = 0;
        }
    } else { presetMenuScrollOffset = 0; }
    displayPresetMenu();
}

void selectPresetMenuItem() {
    resetActivityTimer();
    currentPresetChoice = menuPresetIndex;
    saveChoiceToEEPROM(EEPROM_ADDR_PRESET, currentPresetChoice); 
    if (currentPresetChoice == 0) { 
        EEPROM.get(EEPROM_ADDR_MANUAL_TIME, targetTotalSeconds);
        if (targetTotalSeconds > MAX_TOTAL_SECONDS) targetTotalSeconds = 0;
    } else if (currentPresetChoice < NUM_PRESETS) { 
        targetTotalSeconds = PRESET_VALUES[currentPresetChoice];
    } else { 
        targetTotalSeconds = 0; currentPresetChoice = 0;
        saveChoiceToEEPROM(EEPROM_ADDR_PRESET, currentPresetChoice); 
    }
    lastPos = targetTotalSeconds / SECOND_INCREMENT;
    encoder.setPosition(lastPos * STEPS);
    playClickSound();
    enterMainMenu(); 
}

void enterVeilleMenu() {
    resetActivityTimer(); currentMode = MODE_MENU_VEILLE;
    byte displayLines = LCD_ROWS - 1;
    if (currentSleepSetting >= NUM_SLEEP_OPTIONS) currentSleepSetting = 0;
    if (currentSleepSetting < veilleMenuScrollOffset) { veilleMenuScrollOffset = currentSleepSetting; }
    else if (currentSleepSetting >= veilleMenuScrollOffset + displayLines) { veilleMenuScrollOffset = currentSleepSetting - displayLines + 1; }
    if (NUM_SLEEP_OPTIONS <= displayLines) { veilleMenuScrollOffset = 0; }
    else { if (veilleMenuScrollOffset > NUM_SLEEP_OPTIONS - displayLines) { veilleMenuScrollOffset = NUM_SLEEP_OPTIONS - displayLines; } }
    // if (veilleMenuScrollOffset < 0) veilleMenuScrollOffset = 0; // Redondant pour byte
    lastEncoderMenuPos = encoder.getPosition();
    displayVeilleMenu();
}

void displayVeilleMenu() {
    LCD.clear(); LCD.setCursor(0, 0); LCD.print(F(" Reglage Veille:"));
    byte displayLines = LCD_ROWS - 1;
    for (byte i = 0; i < displayLines; i++) { 
        byte displayRow = i + 1; 
        byte itemIndexToShow = i + veilleMenuScrollOffset; 
        LCD.setCursor(0, displayRow);
        if (itemIndexToShow < NUM_SLEEP_OPTIONS) { 
            if (itemIndexToShow == currentSleepSetting) { LCD.print(">"); }
            else { LCD.print(" "); }
            LCD.print(SLEEP_DELAY_NAMES[itemIndexToShow]); 
            clearRestOfLine(1 + strlen(SLEEP_DELAY_NAMES[itemIndexToShow]) + 1, displayRow);
        } else {
            clearRestOfLine(0, displayRow);
        }
    }
    LCD.setCursor(LCD_COLS - 1, 1); LCD.print(" ");
    LCD.setCursor(LCD_COLS - 1, displayLines); LCD.print(" ");
    if (NUM_SLEEP_OPTIONS > displayLines) { 
        if (veilleMenuScrollOffset > 0) { LCD.setCursor(LCD_COLS - 1, 1); LCD.write(byte(ARROW_UP_CHAR_CODE)); }
        if ((veilleMenuScrollOffset + displayLines) < NUM_SLEEP_OPTIONS) { LCD.setCursor(LCD_COLS - 1, displayLines); LCD.write(byte(ARROW_DOWN_CHAR_CODE)); }
    }
}

void navigateVeilleMenu(int diff) {
    byte displayLines = LCD_ROWS - 1; 
    int tempSetting = currentSleepSetting;
    if (diff > 0) { tempSetting++; if (tempSetting >= NUM_SLEEP_OPTIONS) tempSetting = 0; }
    else if (diff < 0) { if (tempSetting == 0) tempSetting = NUM_SLEEP_OPTIONS; tempSetting--; }
    currentSleepSetting = tempSetting; 
    if (NUM_SLEEP_OPTIONS > displayLines) { 
        if (currentSleepSetting < veilleMenuScrollOffset) { veilleMenuScrollOffset = currentSleepSetting; }
        else if (currentSleepSetting >= veilleMenuScrollOffset + displayLines) { veilleMenuScrollOffset = currentSleepSetting - displayLines + 1; }
        if (diff > 0 && currentSleepSetting == 0) veilleMenuScrollOffset = 0;
        else if (diff < 0 && currentSleepSetting == (NUM_SLEEP_OPTIONS - 1) ) { 
             if (NUM_SLEEP_OPTIONS > displayLines) veilleMenuScrollOffset = NUM_SLEEP_OPTIONS - displayLines;
             else veilleMenuScrollOffset = 0;
        }
    } else { veilleMenuScrollOffset = 0; }
    if (NUM_SLEEP_OPTIONS > displayLines) { 
        if (veilleMenuScrollOffset > NUM_SLEEP_OPTIONS - displayLines) veilleMenuScrollOffset = NUM_SLEEP_OPTIONS - displayLines;
        // if (veilleMenuScrollOffset < 0) veilleMenuScrollOffset = 0; // Redondant pour byte
    } else { veilleMenuScrollOffset = 0; }
    displayVeilleMenu(); 
}

void selectVeilleMenuItem() {
    resetActivityTimer();
    saveChoiceToEEPROM(EEPROM_ADDR_SLEEP_DELAY, currentSleepSetting);
    configuredSleepDelayMillis = (unsigned long)SLEEP_DELAY_VALUES[currentSleepSetting] * 1000UL;
    playClickSound();
    enterMainMenu();
}

void exitMenu() {
    resetActivityTimer();
    bigNum.begin(); 
    currentMode = MODE_TIMER; 
    if (currentPresetChoice == 0) { 
        EEPROM.get(EEPROM_ADDR_MANUAL_TIME, targetTotalSeconds);
        if (targetTotalSeconds > MAX_TOTAL_SECONDS) targetTotalSeconds = 0;
    } else { 
        if (currentPresetChoice < NUM_PRESETS) { targetTotalSeconds = PRESET_VALUES[currentPresetChoice]; }
        else { targetTotalSeconds = 0; currentPresetChoice = 0; EEPROM.update(EEPROM_ADDR_PRESET, currentPresetChoice); }
    }
    displayMIN = targetTotalSeconds / 60;
    displaySEC = targetTotalSeconds % 60;
    displayCS = 0; 
    lastPos = targetTotalSeconds / SECOND_INCREMENT;
    encoder.setPosition(lastPos * STEPS); 
    lastDisplayedMIN = -1; 
    lastDisplayedSEC = -1;
    updateStaticDisplay();      // Appel à la fonction maintenant dans timer.cpp
    updateCentisecondsDisplay(); // Appel à la fonction maintenant dans timer.cpp
    displayStatusLine3();
}

void saveChoiceToEEPROM(int address, byte value) {
    EEPROM.update(address, value); 
}

void displayStatusLine3() {
    byte displayRow = MELODY_NAME_ROW;
    byte startCol = MELODY_NAME_COL;
    clearRestOfLine(0, displayRow);
    LCD.setCursor(startCol - 1, displayRow); 
    LCD.print(F("*"));
    if (!timerMelodyEnabled) {
        LCD.print(F("Mel. Off")); 
    } else {
        if (currentMelodyChoice < NUM_MELODIES) {
            LCD.print(melodyNames[currentMelodyChoice]);
        } else {
            LCD.print(F("???"));
        }
    }
    LCD.print(F(" | ")); // Séparateur

    // Afficher le statut du preset (Manuel, 1.Min, 2.Min, etc.)
    if (currentPresetChoice == 0) {
        LCD.print(presetNames[0]); // Affiche "Manuel"
    } else if (currentPresetChoice < NUM_PRESETS) {
        LCD.print(currentPresetChoice); LCD.print(F(".Min"));  // Affiche "1.Min", "2.Min", etc.
    } else {
        LCD.print(F("???")); //
    }
}

void clearRestOfLine(byte startCol, byte row) {
    if (startCol >= LCD_COLS) return; 
    LCD.setCursor(startCol, row);
    for (byte c = startCol; c < LCD_COLS; c++) {
        LCD.print(" ");
    }
}

void goToSleep() {
    LCD.clear(); 
    LCD.setCursor(0, 1); LCD.print(F("    Mode Veille     "));
    LCD.setCursor(0, 2); LCD.print(F("   Appuyez Btn...   "));
    LCD.noBacklight();
    digitalWrite(RELAY_PIN, HIGH); 
    noTone(BUZZER_PIN);            
    delay(100); 
    cli(); 
    PCICR |= (1 << PCIE2);    
    PCMSK2 |= (1 << PCINT22); 
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); 
    sleep_enable();                      
    sei();                               
    sleep_cpu();                         
    sleep_disable();                     
    PCMSK2 &= ~(1 << PCINT22);           
    LCD.backlight(); 
    awokeByInterrupt = true; 
    if (currentMode == MODE_TIMER) {
        if (currentPresetChoice == 0) {
            EEPROM.get(EEPROM_ADDR_MANUAL_TIME, targetTotalSeconds);
            if (targetTotalSeconds > MAX_TOTAL_SECONDS) targetTotalSeconds = 0;
        } else {
            if (currentPresetChoice < NUM_PRESETS) targetTotalSeconds = PRESET_VALUES[currentPresetChoice];
            else { targetTotalSeconds = 0; currentPresetChoice = 0; EEPROM.update(EEPROM_ADDR_PRESET, currentPresetChoice); }
        }
        lastPos = targetTotalSeconds / SECOND_INCREMENT;
        encoder.setPosition(lastPos * STEPS); 
        displayMIN = targetTotalSeconds / 60;
        displaySEC = targetTotalSeconds % 60;
        displayCS = 0;
        lastDisplayedMIN = -1; 
        lastDisplayedSEC = -1;
        updateStaticDisplay();      // Appel à la fonction maintenant dans timer.cpp
        updateCentisecondsDisplay(); // Appel à la fonction maintenant dans timer.cpp
        displayStatusLine3();
    } else if (currentMode == MODE_METRONOME) {
        encoder.setPosition(currentBPM); 
        metroEncoderLastPos = currentBPM; // Assurez-vous que metroEncoderLastPos est aussi mis à jour si utilisé
        displayMetronomeScreen();
    } else if (currentMode >= MODE_MENU_MAIN && currentMode <= MODE_MENU_TS_METRO) { 
        bigNum.begin(); 
        LCD.createChar(ARROW_UP_CHAR_CODE, arrowUp_Pattern);
        LCD.createChar(ARROW_DOWN_CHAR_CODE, arrowDown_Pattern);
        lastEncoderMenuPos = encoder.getPosition(); 
        if(currentMode == MODE_MENU_MAIN) displayMainMenu();
        else if(currentMode == MODE_MENU_MELODY) displayMelodyMenu();
        else if(currentMode == MODE_MENU_PRESET) displayPresetMenu();
        else if(currentMode == MODE_MENU_VEILLE) displayVeilleMenu();
        else if(currentMode == MODE_MENU_TS_METRO) displayTSMetroMenu();
    }
    resetActivityTimer(); 
}

ISR(PCINT2_vect) {
    // Juste pour le réveil
}

void playClickSound() {
  if (buzzerFeedbackEnabled) {
    tone(BUZZER_PIN, CLICK_FREQUENCY, CLICK_DURATION);
  }
}

// Les fonctions spécifiques au métronome sont maintenant dans metronome.cpp et déclarées dans metronome.h

// Les fonctions spécifiques au timer (timerEnd, updateStaticDisplay, updateCentisecondsDisplay)
// seront déplacées dans timer.cpp lors de la prochaine phase. (MAINTENANT FAIT CI-DESSUS)