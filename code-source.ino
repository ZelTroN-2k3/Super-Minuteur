//***************************************************************
//  Super Minuteur - Code Principal (.ino)
//  - AFFICHAGE STATUT PRESET/MANUEL AJOUTÉ sur ligne 3
//  - Menu sélection Mélodie ET Preset Temps (runtime)
//  - Sauvegarde EEPROM des choix
//  - 3 Mélodies + 3 Presets + Manuel
//  - Clignotement Rétroéclairage en Fin
//  - Ajout deuxième écran de démarrage (Infos Auteur/Version)
//  - AJOUT Fonction Veille réglable via menu
//
//  Date de génération : Vendredi 25 avril 2025 22:45:00 CEST
//  Lieu : Montmédy, Grand Est, France
//  Lien : https://github.com/ZelTroN-2k3/Super-Minuteur
//**************************************************************

#include <EEPROM.h>
#include "Wire.h"
#include <string.h> // Pour strlen (utilisé dans les menus)
#include "LiquidCrystal_I2C.h"
#include "RotaryEncoder.h"
#include "BigNumbers_I2C.h"

#include "conf.h"     // Inclut la configuration générale
#include "melodie.h"  // Inclut les définitions de notes et les fonctions mélodies

#include <avr/sleep.h>      // Pour les fonctions de mise en veille
#include <avr/power.h>      // Pour éteindre des périphériques (optionnel)
#include <avr/interrupt.h>  // Pour les interruptions (cli/sei)

// --- Énumération pour les Modes ---
enum Mode {
  MODE_TIMER,           // Fonctionnement normal du timer
  MODE_MENU_MAIN,       // Menu principal (Choix Melodie/Preset/Veille/Quitter) // <<< MODIF VEILLE >>>
  MODE_MENU_MELODY,     // Sous-menu choix mélodie
  MODE_MENU_PRESET,     // Sous-menu choix preset
  MODE_MENU_VEILLE      // <<< AJOUT VEILLE >>> Sous-menu choix délai veille
};
Mode currentMode = MODE_TIMER; // Mode actuel

// --- Initialisation des Objets Matériels ---
LiquidCrystal_I2C LCD(LCD_ADDR, LCD_COLS, LCD_ROWS);
BigNumbers_I2C bigNum(&LCD);
RotaryEncoder encoder(ENCODER_DT_PIN, ENCODER_CLK_PIN);

// --- Variables Globales ---
bool blinkDone = false; // Pour clignoter une seule fois par cycle de timer
int lastPos = 0;
int newPos = 0; // Pour l'encodeur en mode timer
boolean buttonWasUp = true;
boolean longPressDetected = false; // Pour gérer l'appui long
unsigned long buttonDownTime = 0; // Pour mesurer la durée d'appui

boolean timerRunning = false;
int displaySEC = 0;
int displayMIN = 0;
int displayCS = 0; // Variables d'affichage
unsigned int targetTotalSeconds = 0; // Temps cible réglé
unsigned long targetEndTime = 0;      // Heure de fin calculée
long remainingMillis = 0;           // Temps restant calculé
int lastDisplayedSEC = -1;
int lastDisplayedMIN = -1; // Pour affichage intelligent
unsigned long lastCsUpdateTime = 0;
// const unsigned long interval = 1000; // <<< MODIF VEILLE: Semble inutilisé, commenté >>>

// Variables pour le clignotement final du rétroéclairage
bool isEndSequenceBlinking = false;
unsigned long blinkSequenceStartTime = 0;
const unsigned long blinkSequenceDuration = 5000; // (Durée totale clignotement en ms)
unsigned long lastEndBlinkToggleTime = 0;
const unsigned long endBlinkInterval = 250;      // (Intervalle ON/OFF clignotement en ms)
bool endBlinkStateIsOn = true;

// Variables Menu
byte currentMelodyChoice = 1;
byte currentPresetChoice = 0;
byte menuMainIndex = 0;             // Index de 0 à N-1 maintenant // <<< MODIF VEILLE >>>
byte menuMelodyIndex = 0;           // Index de 0 à N-1 // <<< MODIF VEILLE >>>
byte menuPresetIndex = 0;           // Sélection dans le sous-menu preset (0 à NUM_PRESETS-1)
byte melodyMenuScrollOffset = 0;    // <<< AJOUTER CETTE LIGNE
byte presetMenuScrollOffset = 0;    // Décalage pour le scrolling
int lastEncoderMenuPos = 0;         // Pour la navigation dans le menu avec encodeur
byte menuMainScrollOffset = 0;      // Variable pour le défilement du menu principal

// <<< MODIF VEILLE >>> Mise à jour pour inclure "Veille"
const char* mainMenuItems[] = { " Melodie", " Preset ", " Veille ", " Quitter" };
const byte numMainMenuOptions = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]); // Calcul dynamique

const char* melodyNames[] = { "Mario", "StarWars", "Zelda", "Nokia", "Tetris" };
const char* presetNames[] = { "Manuel", "1 Min", "2 Min", "3 Min" };
// Les noms pour le menu Veille sont dans conf.h (SLEEP_DELAY_NAMES)

// <<< AJOUT VEILLE >>> Variables pour la veille
byte currentSleepSetting = 0;             // Index du délai de veille choisi (0=Off par défaut)
unsigned long configuredSleepDelayMillis = 0; // Délai en ms (0 = Off)
unsigned long lastActivityTime = 0;         // Timestamp de la dernière activité utilisateur
volatile bool awokeByInterrupt = false;     // Indicateur pour savoir si on sort de veille

// Définition (initialisation) effective des tableaux
const unsigned int PRESET_VALUES[NUM_PRESETS] = { 0, 60, 120, 180 };
const unsigned int SLEEP_DELAY_VALUES[NUM_SLEEP_OPTIONS] = { 0, 60, 300, 600 };
const char* const SLEEP_DELAY_NAMES[NUM_SLEEP_OPTIONS] = { "Off", "1 min", "5 min", "10 min" };

// --- Déclarations de fonctions ---

// (Mélodies dans melodie.h)
void resetActivityTimer(); // <<< AJOUT VEILLE >>>
void blinkBacklightOnTimerEnd(); void updateStaticDisplay(); void updateCentisecondsDisplay();
void handleEncoder(); void handleButton(); void timerEnd(); void enterMainMenu();
void displayMainMenu(); void navigateMainMenu(int diff); void selectMainMenuItem();
void enterMelodyMenu(); void displayMelodyMenu(); void navigateMelodyMenu(int diff); void selectMelodyMenuItem();
void enterPresetMenu(); void displayPresetMenu(); void navigatePresetMenu(int diff); void selectPresetMenuItem();
void enterVeilleMenu(); void displayVeilleMenu(); void navigateVeilleMenu(int diff); void selectVeilleMenuItem(); // <<< AJOUT VEILLE >>>
void exitMenu(); void saveChoiceToEEPROM(int address, byte value);
void displayStatusLine3(); // <<< RENOMMÉE (anciennement displaySelectedMelodyName)
void clearRestOfLine(byte startCol, byte row); // Helper d'effacement
void goToSleep(); // <<< AJOUT VEILLE >>>


// --- Fonction d'initialisation ---
void setup() {
  // Configuration des broches
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // États initiaux des sorties
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialisation de l'écran LCD
  LCD.begin();
  LCD.backlight(); // Allume le rétroéclairage par défaut
  LCD.clear();

// --- ÉCRAN DE DÉMARRAGE 1 ---
  LCD.setCursor(3, 1); LCD.print("Super Minuteur");
  LCD.setCursor(1, 2); LCD.print("Initialisation...");
  delay(2000); // Réduit un peu pour laisser du temps au 2ème écran
  LCD.clear();
  // --- FIN ÉCRAN DE DÉMARRAGE 1 ---

  // --- NOUVEAU: ÉCRAN DE DÉMARRAGE 2 (Infos) ---
  LCD.setCursor(0, 0); LCD.print("Auteur: "); LCD.print(AUTHOR_NAME);      // Utilise constante de conf.h
  LCD.setCursor(0, 1); LCD.print("Version: "); LCD.print(FIRMWARE_VERSION); // Utilise constante de conf.h
  LCD.setCursor(0, 2); LCD.print("Date: "); LCD.print(__DATE__);       // Date de compilation
  LCD.setCursor(0, 3); LCD.print(__TIME__);                       // Heure de compilation

  delay(2500); // Affiche l'écran d'infos pendant 2.5 secondes
  LCD.clear(); // Efface l'écran après l'écran d'infos
  // --- FIN ÉCRAN DE DÉMARRAGE 2 ----

  // --- Lire les choix sauvegardés en EEPROM ---
  byte savedMelody = EEPROM.read(EEPROM_ADDR_MELODY);
  // Utilise index 0-based maintenant pour cohérence
  if (savedMelody >= NUM_MELODIES) { currentMelodyChoice = 0; EEPROM.update(EEPROM_ADDR_MELODY, currentMelodyChoice); }
  else { currentMelodyChoice = savedMelody; }
  menuMelodyIndex = currentMelodyChoice; // Init menu sur choix sauvegardé

  // --- Lire le dernier PRESET/MODE utilisé ---
  byte savedPreset = EEPROM.read(EEPROM_ADDR_PRESET);
  if (savedPreset >= NUM_PRESETS) { // Si valeur invalide
      currentPresetChoice = 1; // Défaut = Preset 1 (1 Min)
      EEPROM.update(EEPROM_ADDR_PRESET, currentPresetChoice);
  } else {
      currentPresetChoice = savedPreset;
  }
  menuPresetIndex = currentPresetChoice; // Init menu sur choix sauvegardé

  // <<< AJOUT VEILLE >>> Lire le réglage de veille
  currentSleepSetting = EEPROM.read(EEPROM_ADDR_SLEEP_DELAY);
  if (currentSleepSetting >= NUM_SLEEP_OPTIONS) {
      currentSleepSetting = 0; // Valeur invalide -> par défaut sur Off
      EEPROM.update(EEPROM_ADDR_SLEEP_DELAY, currentSleepSetting); // Sauvegarde valeur par défaut si invalide
  }
  // Mettre à jour la variable du délai en ms
  configuredSleepDelayMillis = (unsigned long)SLEEP_DELAY_VALUES[currentSleepSetting] * 1000;

  // Initialisation de BigNumbers
  bigNum.begin();

  // Initialisation des variables du timer et de l'encodeur
  encoder.setPosition(POSMIN / STEPS); // Commence à 0 logiquement
  targetTotalSeconds = 0; // Sera écrasé si preset ou manuel sauvegardé
  lastPos = 0; // Sera écrasé si preset ou manuel sauvegardé

  // --- Appliquer le preset OU le temps manuel sauvegardé ---
  if (currentPresetChoice == 0) { // Dernier mode était MANUEL
      unsigned int savedManualTime = 0;
      EEPROM.get(EEPROM_ADDR_MANUAL_TIME, savedManualTime);
      if (savedManualTime <= MAX_TOTAL_SECONDS) { targetTotalSeconds = savedManualTime; }
      else { targetTotalSeconds = 0; }
      lastPos = targetTotalSeconds / SECOND_INCREMENT;
      encoder.setPosition(lastPos * STEPS);
  } else { // Dernier mode était un PRESET
      if (currentPresetChoice > 0 && currentPresetChoice < NUM_PRESETS) { targetTotalSeconds = PRESET_VALUES[currentPresetChoice]; }
      else { targetTotalSeconds = 0; }
      lastPos = targetTotalSeconds / SECOND_INCREMENT;
      encoder.setPosition(lastPos * STEPS);
  }

  // Initialise l'affichage basé sur targetTotalSeconds calculé
  displayMIN = targetTotalSeconds / 60;
  displaySEC = targetTotalSeconds % 60;
  displayCS = 0;
  lastDisplayedMIN = -1; // Force l'affichage initial
  lastDisplayedSEC = -1;

  // <<< AJOUT VEILLE >>> Initialiser le timer d'activité APRES avoir tout initialisé
  resetActivityTimer(); // Appel de la nouvelle fonction

  // Affichage initial (Mode Timer)
  currentMode = MODE_TIMER;
  updateStaticDisplay();
  updateCentisecondsDisplay();
  displayStatusLine3(); // Affiche Mélodie | Statut Preset/Manuel
}

// --- Boucle Principale (Gère les modes) ---
void loop() {
  handleEncoder(); // Gère l'encodeur selon le mode
  handleButton();  // Gère le bouton selon le mode

  // Logique spécifique au mode TIMER
  if (currentMode == MODE_TIMER) {
    // Décompte si le timer est en cours
    if (timerRunning) {
      unsigned long currentTime = millis();
      remainingMillis = targetEndTime - currentTime;
      if (remainingMillis <= 0) { timerEnd(); }
      else {
        unsigned long totalRemainingSeconds = remainingMillis / 1000;
        displayMIN = totalRemainingSeconds / 60;
        displaySEC = totalRemainingSeconds % 60;
        displayCS = (remainingMillis % 1000) / 10;
        if (displayMIN != lastDisplayedMIN || displaySEC != lastDisplayedSEC) {
          updateStaticDisplay();
          lastDisplayedMIN = displayMIN; lastDisplayedSEC = displaySEC;
        }
        if (currentTime - lastCsUpdateTime >= csUpdateInterval) {
           lastCsUpdateTime = currentTime; updateCentisecondsDisplay();
        }
      }
    }
    // Gestion du clignotement final si activé
    else if (isEndSequenceBlinking) {
      unsigned long currentTime = millis();
      if (currentTime - blinkSequenceStartTime >= blinkSequenceDuration) {
        isEndSequenceBlinking = false; LCD.backlight(); // S'assure que c'est allumé à la fin
      } else {
        if (currentTime - lastEndBlinkToggleTime >= endBlinkInterval) {
          lastEndBlinkToggleTime = currentTime; endBlinkStateIsOn = !endBlinkStateIsOn;
          if (endBlinkStateIsOn) { LCD.backlight(); } else { LCD.noBacklight(); }
        }
      }
    }
    // <<< AJOUT VEILLE >>> Vérification pour Mise en Veille
    else { // Seulement si timer n'est PAS running ET PAS en séquence de clignotement fin
        if (configuredSleepDelayMillis > 0) { // Si la veille est activée (pas sur "Off")
             if (millis() - lastActivityTime > configuredSleepDelayMillis) {
                  goToSleep(); // Mettre en veille
                  // Le code reprendra ici après le réveil, dans la fonction goToSleep
             }
        }
    }
    // <<< FIN AJOUT VEILLE >>>
  }
  // else { // Si on est dans un mode Menu, on ne fait rien de spécial ici (géré par handleEncoder/Button) }

} // Fin loop()


// --- Fonctions Auxiliaires ---

// <<< AJOUT VEILLE >>> Fonction pour réinitialiser le timer d'inactivité
void resetActivityTimer() {
    lastActivityTime = millis();
    // Optionnel: si l'écran était en veille (pas juste backlight), le rallumer ici ?
    // Pour l'instant, on suppose que seule backlight est coupée.
    // Si l'écran clignotait pour la fin, on arrête ça aussi.
    if (isEndSequenceBlinking) {
      isEndSequenceBlinking = false;
      LCD.backlight();
    }
}

// Gère l'encodeur (MODIFIÉ pour appeler resetActivityTimer)
void handleEncoder() {
  //if (isEndSequenceBlinking) { isEndSequenceBlinking = false; LCD.backlight(); } // Déplacé dans resetActivityTimer
  encoder.tick();

  if (currentMode == MODE_TIMER) {
    if (!timerRunning) {
       int currentEncoderPos = encoder.getPosition();
       newPos = currentEncoderPos * STEPS;
       if (newPos < POSMIN) { encoder.setPosition(POSMIN / STEPS); newPos = POSMIN; }
       else if (newPos > POSMAX) { encoder.setPosition(POSMAX / STEPS); newPos = POSMAX; }
       if (lastPos != newPos) {
         resetActivityTimer(); // <<< AJOUT VEILLE >>> Rotation = Activité
         // Si on tourne l'encodeur, passe en mode Manuel (0) et sauvegarde ce mode
         if (currentPresetChoice != 0) {
             currentPresetChoice = 0;
             saveChoiceToEEPROM(EEPROM_ADDR_PRESET, currentPresetChoice);
             displayStatusLine3(); // <<< MET À JOUR LIGNE 3 pour afficher "Manuel"
         }
         // Calcule et affiche le temps manuel
         targetTotalSeconds = newPos * SECOND_INCREMENT; lastPos = newPos;
         displayMIN = targetTotalSeconds / 60; displaySEC = targetTotalSeconds % 60; displayCS = 0;
         updateStaticDisplay(); updateCentisecondsDisplay();
         // displayStatusLine3(); // Déjà fait au dessus si preset != 0
         lastDisplayedMIN = displayMIN; lastDisplayedSEC = displaySEC;
       }
    }
  }
  else { // Mode Menu
       int currentEncoderMenuPos = encoder.getPosition();
       int diff = currentEncoderMenuPos - lastEncoderMenuPos;
       if(diff != 0) {
         resetActivityTimer(); // <<< AJOUT VEILLE >>> Rotation = Activité (dans les menus aussi)
         switch(currentMode) {
             case MODE_MENU_MAIN: navigateMainMenu(diff); break;
             case MODE_MENU_MELODY: navigateMelodyMenu(diff); break;
             case MODE_MENU_PRESET: navigatePresetMenu(diff); break;
             case MODE_MENU_VEILLE: navigateVeilleMenu(diff); break; // <<< AJOUT VEILLE >>>
             default: break;
         }
         lastEncoderMenuPos = currentEncoderMenuPos;
       }
  }
}

// Gère le bouton (MODIFIÉ pour appeler resetActivityTimer)
void handleButton() {
  // Interrompt le clignotement si actif (déplacé dans resetActivityTimer)
  // if (isEndSequenceBlinking) { resetActivityTimer(); } // Fait par resetActivityTimer appelé plus bas

  boolean buttonIsUp = digitalRead(BUTTON_PIN); // Lire état actuel

  // Détection appui (front descendant)
  if (buttonWasUp && !buttonIsUp) {
    buttonDownTime = millis();      // Mémorise l'heure de l'appui
    longPressDetected = false;      // Réinitialise le flag d'appui long
    resetActivityTimer(); // <<< AJOUT VEILLE >>> Détection appui = Activité
  }

  // Détection maintien appui long (pendant que le bouton EST enfoncé)
  if (!buttonWasUp && !longPressDetected && (millis() - buttonDownTime >= longPressDuration)) {
     longPressDetected = true; // Marque l'appui long comme détecté
     // resetActivityTimer(); // Déjà fait au début de l'appui
     // --- Action Appui Long ---
     if (currentMode == MODE_TIMER && !timerRunning) { // Ne peut entrer dans le menu que si le timer est arrêté
       enterMainMenu();
     }
     // Pas d'action spécifique pour appui long dans les menus pour l'instant
  }

  // Détection relâchement (front montant)
  if (!buttonWasUp && buttonIsUp) {
    // resetActivityTimer(); // <<< AJOUT VEILLE >>> Relâchement = Activité (optionnel, déjà fait à l'appui)
    // Vérifie si ce relâchement correspond à un appui court (non détecté comme long)
    // Et applique l'anti-rebond standard (debounceDelay de conf.h)
    if (!longPressDetected && (millis() - buttonDownTime >= debounceDelay)) {
        // --- Action Appui Court ---
        // resetActivityTimer(); // Déjà fait à l'appui
        switch (currentMode) {
            case MODE_TIMER: // Agit comme Start/Stop
                if (timerRunning) { // ****** Bloc Arrêter (MODIFIÉ) ******
                    timerRunning = false; // Arrête le drapeau du timer

                    // Reset affichage à 00:00.00 et met à jour LCD
                    displayMIN = 0; displaySEC = 0; displayCS = 0;
                    updateStaticDisplay();      // Affiche STOP et 00:00
                    updateCentisecondsDisplay();  // Affiche .00
                    displayStatusLine3();       // Affiche Mélodie | Statut (qui n'a pas changé)

                    // Coupe relais et son
                    digitalWrite(RELAY_PIN, HIGH);
                    noTone(BUZZER_PIN); // Arrête le son si on interrompt manuellement

                     // --- MODIFICATION ICI ---
                     // Reset temps cible ou recharge dernier temps manuel/preset
                     if (currentPresetChoice == 0) { // Si c'était un timer manuel qui tournait
                         // Recharge la dernière valeur manuelle depuis EEPROM
                         unsigned int savedManualTime = 0;
                         EEPROM.get(EEPROM_ADDR_MANUAL_TIME, savedManualTime);
                         if (savedManualTime <= MAX_TOTAL_SECONDS) {
                             targetTotalSeconds = savedManualTime;
                         } else {
                             targetTotalSeconds = 0; // Sécurité
                         }
                     } else { // C'était un preset, on recharge sa valeur
                         if (currentPresetChoice > 0 && currentPresetChoice < NUM_PRESETS) {
                             targetTotalSeconds = PRESET_VALUES[currentPresetChoice];
                         } else {
                             targetTotalSeconds = 0; // Sécurité
                         }
                     }
                     // Remet l'encodeur à la position correspondant au temps chargé
                     lastPos = targetTotalSeconds / SECOND_INCREMENT;
                     encoder.setPosition(lastPos * STEPS);
                     // --- FIN MODIFICATION ---

                    lastDisplayedMIN = -1; // Force rafraîchissement si redémarrage
                    lastDisplayedSEC = -1;

                } else { // ****** Bloc Démarrer (Inchangé) ******
                    // On vérifie si un temps (manuel ou preset) est chargé
                    if (targetTotalSeconds > 0) {
                        targetEndTime = millis() + (unsigned long)targetTotalSeconds * 1000;
                        remainingMillis = targetEndTime - millis(); // Calcul initial
                        displayMIN = (remainingMillis / 1000) / 60; displaySEC = (remainingMillis / 1000) % 60;
                        displayCS = (remainingMillis % 1000) / 10;
                        timerRunning = true; blinkDone = false; // Reset blinkDone flag
                        // Sauvegarde temps manuel SEULEMENT si on démarre en mode manuel
                        if (currentPresetChoice == 0) {
                            EEPROM.put(EEPROM_ADDR_MANUAL_TIME, targetTotalSeconds);
                        }
                        digitalWrite(RELAY_PIN, LOW); updateStaticDisplay(); updateCentisecondsDisplay(); displayStatusLine3();
                        lastDisplayedMIN = displayMIN; lastDisplayedSEC = displaySEC; lastCsUpdateTime = millis();
                    }
                    // else { // targetTotalSeconds == 0, ne rien faire }
                }
                break; // Fin case MODE_TIMER

            case MODE_MENU_MAIN: selectMainMenuItem(); break;
            case MODE_MENU_MELODY: selectMelodyMenuItem(); break;
            case MODE_MENU_PRESET: selectPresetMenuItem(); break;
            case MODE_MENU_VEILLE: selectVeilleMenuItem(); break; // <<< AJOUT VEILLE >>>
        } // Fin switch(currentMode)
    } // Fin if debounce
    // Réinitialise le flag d'appui long au relâchement, après les vérifications
    longPressDetected = false;
  }
  // Mémorise l'état du bouton pour la prochaine boucle
  buttonWasUp = buttonIsUp;
} // --- Fin handleButton ---


// Gère la fin du timer (Inchangé ici)
void timerEnd() {
  timerRunning = false; // Arrête le drapeau du timer

  // Réinitialise l'affichage à 00:00.00 et met à jour l'écran
  displayMIN = 0; displaySEC = 0; displayCS = 0;
  updateStaticDisplay();      // Affiche STOP et 00:00
  updateCentisecondsDisplay();  // Affiche .00
  displayStatusLine3();       // Réaffiche Mélodie | Statut Preset/Manuel (qui n'a pas changé)
  lastDisplayedMIN = -1;      // Force le rafraîchissement si on redémarre vite
  lastDisplayedSEC = -1;

  // Coupe le relais
  digitalWrite(RELAY_PIN, HIGH);

  // --- MODIFICATION ICI : Reset target time ou recharge ---
  if (currentPresetChoice == 0) { // Si c'était un timer manuel
     // Recharge la dernière valeur manuelle depuis EEPROM
     unsigned int savedManualTime = 0;
     EEPROM.get(EEPROM_ADDR_MANUAL_TIME, savedManualTime);
     if (savedManualTime <= MAX_TOTAL_SECONDS) { targetTotalSeconds = savedManualTime; }
     else { targetTotalSeconds = 0; } // Sécurité
  } else { // C'était un preset
     if (currentPresetChoice > 0 && currentPresetChoice < NUM_PRESETS) {
         targetTotalSeconds = PRESET_VALUES[currentPresetChoice]; // Recharge valeur preset
     } else {
         targetTotalSeconds = 0; // Sécurité
     }
  }
  // Remet l'encodeur à la position correspondant au temps chargé
  lastPos = targetTotalSeconds / SECOND_INCREMENT;
  encoder.setPosition(lastPos * STEPS);
  // --- FIN MODIFICATION ---

  // --- Séquence de Fin (Mélodie + Clignotement) ---
  // 1. Joue la mélodie CHOISIE
  byte melodyIdx = currentMelodyChoice; // Utilise index 0-based
  if (melodyIdx == 0) { playMarioMelody(); }
  else if (melodyIdx == 1) { playImperialMarch(); }
  else if (melodyIdx == 2) { playZeldaTheme(); }
  else if (melodyIdx == 3) { playNokiaTune(); }     // <<< AJOUTER
  else if (melodyIdx == 4) { playTetrisTheme(); }   // <<< AJOUTER
  // else { // Aucune mélodie si index invalide }

  // 2. Fait clignoter le rétroéclairage
  if (!blinkDone) {
      isEndSequenceBlinking = true; // Active le drapeau pour le clignotement dans loop()
      blinkSequenceStartTime = millis();
      lastEndBlinkToggleTime = millis();
      endBlinkStateIsOn = false; // Commence éteint
      LCD.noBacklight();
      // blinkBacklightOnTimerEnd(); // Ancienne version bloquante
      blinkDone = true;
  }
}

// --- Fonctions du Menu ---

// Passe en mode Menu Principal (MODIFIÉ pour resetActivityTimer)
void enterMainMenu() {
    resetActivityTimer(); // <<< AJOUT VEILLE >>>
    currentMode = MODE_MENU_MAIN;
    timerRunning = false; // isEndSequenceBlinking = false; LCD.backlight(); // Fait par resetActivityTimer
    noTone(BUZZER_PIN);
    menuMainIndex = 0; // Commence sur la première option (index 0)

    // Initialize scroll offset
    byte displayLines = LCD_ROWS - 1;
    if (menuMainIndex >= displayLines) {
        menuMainScrollOffset = menuMainIndex - displayLines + 1;
    } else {
        menuMainScrollOffset = 0;
    }
    // Ensure offset validity (safety check)
    if (numMainMenuOptions <= displayLines) menuMainScrollOffset = 0;
    else if (menuMainScrollOffset > (numMainMenuOptions - displayLines)) {
         menuMainScrollOffset = numMainMenuOptions - displayLines;
    }

    lastEncoderMenuPos = encoder.getPosition(); // Synchro encodeur pour éviter sauts
    displayMainMenu();
}

// Affichage Menu Principal (MODIFIÉ pour index 0-based et Veille)
// CORRECTION pour affichage complet des réglages dans le menu principal
void displayMainMenu() {
    LCD.clear();
    LCD.setCursor(0, 0); LCD.print("Menu Reglages:");
    byte displayLines = LCD_ROWS - 1; // ex: 3 lignes pour items

    for (byte i = 0; i < displayLines; i++) { // Boucle seulement sur les lignes d'affichage disponibles
        byte displayRow = i + 1;
        byte itemIndexToShow = i + menuMainScrollOffset; // Calcule l'index réel de l'item à afficher

        LCD.setCursor(0, displayRow); // Positionne sur la ligne d'affichage

        if (itemIndexToShow < numMainMenuOptions) { // Vérifie si cet index est valide
            byte startColInfo = 0; // Colonne où commence l'info (après nom item + curseur)
            byte startColClear = 0;  // Colonne où commencer l'effacement

            // Affiche le curseur '>' ou espace
            if (itemIndexToShow == menuMainIndex) { LCD.print(">"); startColInfo = 1;}
            else { LCD.print(" "); startColInfo = 1;}

            // Affiche le texte de l'item du menu
            LCD.print(mainMenuItems[itemIndexToShow]);
            startColInfo += strlen(mainMenuItems[itemIndexToShow]); // Met à jour la position

            // Positionne le curseur pour l'info et initialise startColClear
            LCD.setCursor(startColInfo, displayRow);
            startColClear = startColInfo; // Par défaut, on efface après le nom de l'item

            // Affiche les détails (réglage actuel) et met à jour startColClear
            if (itemIndexToShow == 0) { // Melodie
                LCD.print(": "); startColClear += 2; // Avance après ": "
                if (currentMelodyChoice < NUM_MELODIES) {
                    LCD.print(melodyNames[currentMelodyChoice]);
                    startColClear += strlen(melodyNames[currentMelodyChoice]); // Avance après le nom de la mélodie
                } else { LCD.print("???"); startColClear += 3; }
            } else if (itemIndexToShow == 1) { // Preset
                LCD.print(": "); startColClear += 2;
                if (currentPresetChoice < NUM_PRESETS) {
                    LCD.print(presetNames[currentPresetChoice]);
                    startColClear += strlen(presetNames[currentPresetChoice]); // Avance après le nom du preset
                } else { LCD.print("???"); startColClear += 3; }
            } else if (itemIndexToShow == 2) { // Veille
                LCD.print(": "); startColClear += 2;
                if (currentSleepSetting < NUM_SLEEP_OPTIONS) {
                    LCD.print(SLEEP_DELAY_NAMES[currentSleepSetting]);
                    startColClear += strlen(SLEEP_DELAY_NAMES[currentSleepSetting]); // Avance après le nom du délai
                } else { LCD.print("???"); startColClear += 3; }
            }
            // Pas de détails pour "Quitter" (index 3), startColClear reste après " Quitter"

            // Efface le reste de la ligne à partir de la bonne colonne
            clearRestOfLine(startColClear, displayRow);

        } else {
            // Si l'index à afficher n'est pas valide, efface la ligne
            clearRestOfLine(0, displayRow);
        }
    }
}


// Navigue dans le Menu Principal (MODIFIÉ pour index 0-based)
void navigateMainMenu(int diff) {
    byte displayLines = LCD_ROWS - 1;
    /* byte previousIndex = menuMainIndex; // Inutile ici, peut être enlevé */

    // Update index (wraps around)
    if (diff > 0) { // Down
        menuMainIndex = (menuMainIndex + 1) % numMainMenuOptions;
    } else { // Up
        menuMainIndex = (menuMainIndex - 1 + numMainMenuOptions) % numMainMenuOptions;
    }

    // Update scroll offset based on new index
    if (numMainMenuOptions > displayLines) { // Only scroll if needed
        if (menuMainIndex < menuMainScrollOffset) {
            // Scrolled up past the top visible item
            menuMainScrollOffset = menuMainIndex;
        } else if (menuMainIndex >= (menuMainScrollOffset + displayLines)) {
            // Scrolled down past the bottom visible item
            menuMainScrollOffset = menuMainIndex - displayLines + 1;
        }
        // Handle wrap around scrolling for offset
        if (diff > 0 && menuMainIndex == 0) { // Wrapped from bottom to top
             menuMainScrollOffset = 0;
        } else if (diff < 0 && menuMainIndex == numMainMenuOptions - 1) { // Wrapped from top to bottom
             menuMainScrollOffset = numMainMenuOptions - displayLines;
        }
         // Ensure offset is valid (safety check)
         if (menuMainScrollOffset > (numMainMenuOptions - displayLines)) {
              menuMainScrollOffset = numMainMenuOptions - displayLines;
         }
    } else {
        menuMainScrollOffset = 0; // No scrolling needed if items fit
    }

    displayMainMenu(); // Redraw with new index and offset
}

// Sélectionne un item dans le Menu Principal (MODIFIÉ pour index 0-based et Veille)
void selectMainMenuItem() {
    resetActivityTimer(); // <<< AJOUT VEILLE >>> Sélection = Activité
    switch (menuMainIndex) {
        case 0: enterMelodyMenu(); break;   // Ouvre sous-menu mélodie
        case 1: enterPresetMenu(); break;   // Ouvre sous-menu preset
        case 2: enterVeilleMenu(); break;   // <<< AJOUT VEILLE >>> Ouvre sous-menu veille
        case 3: exitMenu(); break;          // Quitte tous les menus
    }
}

// --- Sous-Menu Mélodie --- (MODIFIÉ pour index 0-based)
void enterMelodyMenu() {
    resetActivityTimer(); // <<< AJOUT VEILLE >>>
    currentMode = MODE_MENU_MELODY;
    menuMelodyIndex = currentMelodyChoice; // Sélection initiale = choix actuel (0-based)
    // Initialize scroll offset
    byte displayLines = LCD_ROWS - 1;
    if (menuMelodyIndex >= displayLines) {
        melodyMenuScrollOffset = menuMelodyIndex - displayLines + 1;
    } else {
        melodyMenuScrollOffset = 0;
    }
    // Ensure offset validity
    // Note : NUM_MELODIES vient de conf.h (doit être 5 maintenant)
    if (NUM_MELODIES <= displayLines) melodyMenuScrollOffset = 0;
    else if (melodyMenuScrollOffset > (NUM_MELODIES - displayLines)) {
         melodyMenuScrollOffset = NUM_MELODIES - displayLines;
    }
   
    lastEncoderMenuPos = encoder.getPosition(); // Synchro encodeur
    // Ajouter logique de scroll offset si beaucoup de mélodies
    displayMelodyMenu();
}

// Affichage Sous-Menu Mélodie (MODIFIÉ pour index 0-based, sans scroll complexe)
void displayMelodyMenu() {
    LCD.clear();
    LCD.setCursor(0, 0); LCD.print(" Choix Melodie:");
    byte displayLines = LCD_ROWS - 1; // Lignes dispo pour items (ex: 3)

    for (byte i = 0; i < displayLines; i++) { // Boucle seulement sur les lignes d'affichage disponibles
        byte displayRow = i + 1;
        byte itemIndexToShow = i + melodyMenuScrollOffset; // Calcule l'index réel de l'item à afficher

        LCD.setCursor(0, displayRow); // Positionne sur la ligne d'affichage

        if (itemIndexToShow < NUM_MELODIES) { // Vérifie si cet index est valide
            // Affiche le curseur '>' si c'est l'item sélectionné
            if (itemIndexToShow == menuMelodyIndex) { LCD.print(">"); } else { LCD.print(" "); }

            // Affiche le texte de l'item du menu (nom de la mélodie)
            // LCD.print(itemIndexToShow + 1); LCD.print("-"); // Optionnel: Numérotation 1-based
            LCD.print(melodyNames[itemIndexToShow]);

            clearRestOfLine(15, displayRow); // Efface le reste de la ligne (ajustez 15 si noms longs)

        } else {
            // Si l'index à afficher n'est pas valide, efface la ligne
            clearRestOfLine(0, displayRow);
        }
    }
}


// Navigue dans le sous-menu Mélodie (MODIFIÉ pour index 0-based)
void navigateMelodyMenu(int diff) {
    byte displayLines = LCD_ROWS - 1;

    // Update index (wraps around, 0-based)
    if (diff > 0) { // Down
        menuMelodyIndex = (menuMelodyIndex + 1) % NUM_MELODIES;
    } else { // Up
        menuMelodyIndex = (menuMelodyIndex - 1 + NUM_MELODIES) % NUM_MELODIES;
    }
    // Update scroll offset
    if (NUM_MELODIES > displayLines) { // Only scroll if needed
        if (menuMelodyIndex < melodyMenuScrollOffset) {
            // Scrolled up past the top visible item
            melodyMenuScrollOffset = menuMelodyIndex;
        } else if (menuMelodyIndex >= (melodyMenuScrollOffset + displayLines)) {
            // Scrolled down past the bottom visible item
            melodyMenuScrollOffset = menuMelodyIndex - displayLines + 1;
        }
        // Handle wrap around scrolling for offset
        if (diff > 0 && menuMelodyIndex == 0) { // Wrapped from bottom to top
             melodyMenuScrollOffset = 0;
        } else if (diff < 0 && menuMelodyIndex == NUM_MELODIES - 1) { // Wrapped from top to bottom
             // Ensure offset is max possible if more items than lines
             if (NUM_MELODIES > displayLines) {
                melodyMenuScrollOffset = NUM_MELODIES - displayLines;
             } else {
                melodyMenuScrollOffset = 0;
             }
        }
         // Ensure offset is valid (safety check) - Important!
         if (melodyMenuScrollOffset > (NUM_MELODIES - displayLines)) {
              if (NUM_MELODIES >= displayLines) { // Avoid negative offset if few items
                  melodyMenuScrollOffset = NUM_MELODIES - displayLines;
              } else {
                  melodyMenuScrollOffset = 0;
              }
         }
         // Ensure offset isn't greater than the current index either
         if (melodyMenuScrollOffset > menuMelodyIndex) melodyMenuScrollOffset = menuMelodyIndex;


    } else {
        melodyMenuScrollOffset = 0; // No scrolling needed if items fit
    }

    displayMelodyMenu(); // Redraw with new index and offset
}

// Sélection Mélodie (MODIFIÉ pour index 0-based)
void selectMelodyMenuItem() {
    resetActivityTimer(); // <<< AJOUT VEILLE >>>
    currentMelodyChoice = menuMelodyIndex; // Valide le choix (0-based)
    saveChoiceToEEPROM(EEPROM_ADDR_MELODY, currentMelodyChoice); // Sauvegarde
    enterMainMenu(); // Revient au menu principal après sélection mélodie
}

// --- Sous-Menu Preset --- (Déjà OK avec index 0-based et scroll)
void enterPresetMenu() {
    resetActivityTimer(); // <<< AJOUT VEILLE >>>
    currentMode = MODE_MENU_PRESET;
    menuPresetIndex = currentPresetChoice; // Sélection initiale = choix actuel

    byte displayLines = LCD_ROWS - 1;
    if (menuPresetIndex >= displayLines) {
        presetMenuScrollOffset = menuPresetIndex - displayLines + 1;
    } else {
        presetMenuScrollOffset = 0;
    }
     if (presetMenuScrollOffset > (NUM_PRESETS - displayLines)) {
        if (NUM_PRESETS >= displayLines) {
            presetMenuScrollOffset = NUM_PRESETS - displayLines;
        } else {
            presetMenuScrollOffset = 0;
        }
     }

    lastEncoderMenuPos = encoder.getPosition(); // Synchro encodeur
    displayPresetMenu(); // Affiche le menu avec le bon décalage
}

// Affichage Sous-Menu Preset (Inchangé, déjà OK)
void displayPresetMenu() {
    LCD.clear();
    LCD.setCursor(0, 0); LCD.print(" Choix Preset:");
    byte displayLines = LCD_ROWS - 1; // Lignes dispo pour items (ex: 3)

    for (byte i = 0; i < displayLines; i++) {
        byte displayRow = i + 1;
        byte presetIndex = i + presetMenuScrollOffset; // Index réel dans le tableau

        LCD.setCursor(0, displayRow);

        if (presetIndex < NUM_PRESETS) { // Si cet index est valide
            if (presetIndex == menuPresetIndex) { LCD.print(">"); } else { LCD.print(" "); }
            LCD.print(presetNames[presetIndex]);
            clearRestOfLine(10, displayRow); // Efface fin de ligne
        } else {
            clearRestOfLine(0, displayRow); // Efface ligne vide
        }
    }
}

// Navigue dans le sous-menu Preset (Inchangé, déjà OK)
void navigatePresetMenu(int diff) {
    byte displayLines = LCD_ROWS - 1;

    if (diff > 0) { // Down
        menuPresetIndex = (menuPresetIndex + 1) % NUM_PRESETS;
        if (menuPresetIndex < presetMenuScrollOffset) { // Wrap around
             presetMenuScrollOffset = menuPresetIndex;
        } else if (menuPresetIndex >= (presetMenuScrollOffset + displayLines)) {
            presetMenuScrollOffset = menuPresetIndex - displayLines + 1;
        }
    } else { // Up
        menuPresetIndex = (menuPresetIndex - 1 + NUM_PRESETS) % NUM_PRESETS;
         if (menuPresetIndex >= (presetMenuScrollOffset + displayLines) || menuPresetIndex < presetMenuScrollOffset) { // Wrap around or scroll up
              presetMenuScrollOffset = menuPresetIndex;
              // Adjust offset if scrolling up into view from bottom
              if (NUM_PRESETS > displayLines && menuPresetIndex == NUM_PRESETS - 1) {
                 presetMenuScrollOffset = NUM_PRESETS - displayLines;
              }
         }
    }
     // Ensure offset is valid
     if (presetMenuScrollOffset > (NUM_PRESETS - displayLines)) {
       if (NUM_PRESETS >= displayLines) presetMenuScrollOffset = NUM_PRESETS - displayLines;
       else presetMenuScrollOffset = 0;
     }
     if (presetMenuScrollOffset > menuPresetIndex) presetMenuScrollOffset = menuPresetIndex; // Don't scroll past selection

    displayPresetMenu();
}


// Sélection Preset (MODIFIÉ pour resetActivityTimer)
void selectPresetMenuItem() {
    resetActivityTimer(); // <<< AJOUT VEILLE >>>
    currentPresetChoice = menuPresetIndex; // Valide le choix (0 à 3)
    saveChoiceToEEPROM(EEPROM_ADDR_PRESET, currentPresetChoice); // Sauvegarde

    // Charge le temps du preset choisi (ou 0 si Manuel)
    if (currentPresetChoice > 0 && currentPresetChoice < NUM_PRESETS) {
        targetTotalSeconds = PRESET_VALUES[currentPresetChoice]; // PRESET_VALUES défini dans conf.h
    } else { // Manuel
        // Si manuel choisi dans menu, charger dernier temps manuel sauvegardé
        EEPROM.get(EEPROM_ADDR_MANUAL_TIME, targetTotalSeconds);
        if (targetTotalSeconds > MAX_TOTAL_SECONDS) targetTotalSeconds = 0; // Validation
    }
    // Quitte le menu et applique directement le preset/manuel
    exitMenu();
}

// <<< AJOUT VEILLE >>> --- Sous-Menu Veille ---
void enterVeilleMenu() {
    resetActivityTimer();
    currentMode = MODE_MENU_VEILLE;
    // menuVeilleIndex = currentSleepSetting; // Utilise directement currentSleepSetting comme index
    lastEncoderMenuPos = encoder.getPosition(); // Synchro encodeur
    // Pas besoin de scroll pour 4 options sur 3 lignes
    displayVeilleMenu();
}

// Affichage Sous-Menu Veille
void displayVeilleMenu() {
    LCD.clear();
    LCD.setCursor(0, 0); LCD.print(" Reglage Veille:");
    byte displayLines = LCD_ROWS - 1;

    for (byte i = 0; i < NUM_SLEEP_OPTIONS; i++) {
        if (i >= displayLines) break; // Ne pas dépasser l'écran

        byte displayRow = i + 1;
        LCD.setCursor(0, displayRow);

        if (i == currentSleepSetting) { LCD.print(">"); } else { LCD.print(" "); }
        LCD.print(SLEEP_DELAY_NAMES[i]); // Utilise noms de conf.h
        clearRestOfLine(10, displayRow);
    }
}

// Navigue dans le sous-menu Veille
void navigateVeilleMenu(int diff) {
    if (diff > 0) { currentSleepSetting = (currentSleepSetting + 1) % NUM_SLEEP_OPTIONS; }
    else { currentSleepSetting = (currentSleepSetting - 1 + NUM_SLEEP_OPTIONS) % NUM_SLEEP_OPTIONS; }
    displayVeilleMenu(); // Redessine
}

// Sélection Délai Veille
void selectVeilleMenuItem() {
    resetActivityTimer();
    // La valeur est déjà dans currentSleepSetting grâce à la navigation
    saveChoiceToEEPROM(EEPROM_ADDR_SLEEP_DELAY, currentSleepSetting); // Sauvegarde
    configuredSleepDelayMillis = (unsigned long)SLEEP_DELAY_VALUES[currentSleepSetting] * 1000; // Met à jour délai actif
    enterMainMenu(); // Revient au menu principal
}
// <<< FIN AJOUT VEILLE >>>


// Quitte le menu (MODIFIÉ pour resetActivityTimer et MAJ affichage)
void exitMenu() {
    resetActivityTimer(); // <<< AJOUT VEILLE >>>
    currentMode = MODE_TIMER;
    LCD.clear(); // Efface le menu

    // Met à jour affichage timer avec preset/manuel chargé
    displayMIN = targetTotalSeconds / 60; displaySEC = targetTotalSeconds % 60; displayCS = 0;
    lastPos = targetTotalSeconds / SECOND_INCREMENT; encoder.setPosition(lastPos * STEPS);
    lastDisplayedMIN = -1; lastDisplayedSEC = -1; // Force rafraîchissement

    updateStaticDisplay();
    updateCentisecondsDisplay();
    displayStatusLine3(); // Affiche la ligne d'info mise à jour
}

// Sauvegarde EEPROM (Inchangé)
void saveChoiceToEEPROM(int address, byte value) { EEPROM.update(address, value);
} // update ne réécrit que si la valeur est différente

// --- Fonctions d'Affichage (Timer) ---

// Fonction pour faire clignoter le rétroéclairage (Non-bloquante maintenant)
// void blinkBacklightOnTimerEnd() { // Remplacé par logique dans loop() }


// Met à jour uniquement la partie statique (Grands chiffres + Statut) (Inchangé)
void updateStaticDisplay() {
LCD.setCursor(STATUS_COL_START, STATUS_ROW); // Ligne 0
    if (timerRunning) {
        LCD.print("TIMER START");
        // Efface la zone où le temps cible serait affiché
        LCD.print("         "); // 9 espaces pour effacer " | 00:00"
    } else { // Timer arrêté
        LCD.print("TIMER STOP ");

        // --- MODIFICATION POUR FORMAT " | 00:00" ---
        LCD.print("| "); // Ajoute le séparateur "|"

        int targetMIN = targetTotalSeconds / 60;
        int targetSEC = targetTotalSeconds % 60;
        if (targetMIN < 10) LCD.print("0");
        LCD.print(targetMIN);
        LCD.print(":");
        if (targetSEC < 10) LCD.print("0");
        LCD.print(targetSEC);
        LCD.print(" "); // Espace final
        // --- FIN MODIFICATION ---
    }

    // --- Affichage des Grands Chiffres (MM:SS du décompte actuel, qui est 00:00 si arrêté) ---
    byte minTens = displayMIN / 10; byte minUnits = displayMIN % 10;
    byte secTens = displaySEC / 10; byte secUnits = displaySEC % 10;

    bigNum.displayLargeNumber(minTens, BIG_M1_COL, BIG_NUM_ROW);
    bigNum.displayLargeNumber(minUnits, BIG_M2_COL, BIG_NUM_ROW);
    LCD.setCursor(COLON_COL, BIG_NUM_ROW); LCD.print(" ");
    LCD.setCursor(COLON_COL, BIG_NUM_ROW + 1); LCD.print(".");
    bigNum.displayLargeNumber(secTens, BIG_S1_COL, BIG_NUM_ROW);
    bigNum.displayLargeNumber(secUnits, BIG_S2_COL, BIG_NUM_ROW);
}

// Affichage Centisecondes (.CS) (Inchangé)
void updateCentisecondsDisplay() {
  LCD.setCursor(CS_COL, CS_ROW); LCD.print(".");
  if (displayCS < 10) { LCD.print("0"); } LCD.print(displayCS); LCD.print(" ");
}

// Affiche icône + nom mélodie + statut preset/manuel (Inchangé)
void displayStatusLine3() {
    byte displayRow = MELODY_NAME_ROW; // Devrait être 3 (défini dans conf.h)
    byte startCol = MELODY_NAME_COL;   // Devrait être 1 (défini dans conf.h)

    // 1. Efface la ligne AVANT d'écrire pour éviter les restes
    clearRestOfLine(0, displayRow);

    // 2. Affiche Icône + Nom Mélodie
    LCD.setCursor(startCol - 1, displayRow);
    LCD.print("*"); // Icône simple
    LCD.print(" ");
    byte melodyIndex = currentMelodyChoice; // Utilise index 0-based
    if (melodyIndex < NUM_MELODIES) {
       LCD.print(melodyNames[melodyIndex]);
    } else { LCD.print("???"); }

    // 3. Affiche Séparateur et Statut Preset/Manuel
    LCD.print(" | "); // Ajoute un séparateur

    // Vérifie le statut du preset actuel (currentPresetChoice)
    if (currentPresetChoice < NUM_PRESETS) { // Utilise NUM_PRESETS de conf.h
        if (currentPresetChoice == 0) {
            LCD.print(presetNames[0]); // Affiche "Manuel"
        } else {
            // Construit et affiche "P1", "P2", ou "P3"
            LCD.print("P");
            LCD.print(currentPresetChoice);
        }
    } else {
        LCD.print("???"); // Si choix preset invalide en mémoire
    }
}

// Helper pour effacer la fin d'une ligne (Inchangé)
void clearRestOfLine(byte startCol, byte row) {
    if (startCol >= LCD_COLS) return; // Sécurité
    LCD.setCursor(startCol, row);
    for (byte c = startCol; c < LCD_COLS; c++) { LCD.print(" "); }
}


// <<< AJOUT VEILLE >>> --- Fonction de Mise en Veille ---
void goToSleep() {
    // DEBUG: Serial.println("Mise en veille..."); // Enlevez Serial si non utilisé
    LCD.setCursor(0, 0); LCD.print("                    ");
    LCD.setCursor(0, 1); LCD.print("     Minuteur en    ");
    LCD.setCursor(0, 2); LCD.print("      veille...     ");
    LCD.setCursor(0, 3); LCD.print("                    ");

    // 1. Préparer la mise en veille
    LCD.noBacklight();            // Éteindre le rétroéclairage
    // Ne pas utiliser lcd.noDisplay() pour l'instant pour simplifier le réveil

    digitalWrite(RELAY_PIN, HIGH); // Assurer que le relais est OFF
    noTone(BUZZER_PIN);            // Assurer que le buzzer est OFF

    // Optionnel : Désactiver des périphériques (attention à TWI/I2C !)
    // power_adc_disable();
    // power_timer1_disable(); // Attention si PWM ou autre utilisé
    // etc.

    delay(100); // Petite pause

    // 2. Configurer le réveil via interruption sur le bouton (D6 = PCINT22)
    cli(); // Désactiver interruptions globales

    PCICR |= (1 << PCIE2);    // Activer interruptions Pin Change groupe 2 (D0-D7)
    PCMSK2 |= (1 << PCINT22); // Activer masque pour PCINT22 (D6)

    set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Choisir mode veille profonde
    sleep_enable();                     // Activer la mise en veille

    sei(); // Réactiver interruptions globales (pour le réveil)

    // 3. Entrer en veille
    sleep_cpu(); // S'endort ici...

    // >>> Le code reprend ICI après le réveil par l'interruption <<<

    // 4. Actions au réveil
    sleep_disable();          // Désactiver la fonction veille
    PCMSK2 &= ~(1 << PCINT22); // Optionnel: Désactiver masque PCINT22 pour éviter réveils non désirés juste après
    // Si PCMSK2 est mis à 0, désactiver aussi PCIE2 ?
    // if (PCMSK2 == 0) PCICR &= ~(1 << PCIE2); // Désactiver groupe si plus de pin surveillée

    // Optionnel : Réactiver les périphériques
    // power_all_enable();

    // IMPORTANT : Réactiver l'écran et rafraîchir
    LCD.backlight(); // Rallumer le rétroéclairage
    // Comme on n'a pas fait lcd.noDisplay(), pas besoin de lcd.begin() normalement.
    // Il faut juste redessiner l'écran car il n'a pas été mis à jour pendant la veille.
    LCD.clear(); // Effacer l'écran pour être sûr

    // DEBUG: Serial.println("Reveil !"); // Enlever si Serial non utilisé

    // Redessiner l'état actuel (qui doit être MODE_TIMER, arrêté)
    currentMode = MODE_TIMER; // Assurer qu'on est en mode timer
    timerRunning = false;     // Assurer qu'il est arrêté
    // Les variables targetTotalSeconds, displayMIN/SEC etc. ont conservé leur valeur d'avant la veille
    // Remettre l'encodeur à la bonne position peut être utile
    lastPos = targetTotalSeconds / SECOND_INCREMENT;
    encoder.setPosition(lastPos * STEPS);
    lastDisplayedMIN = -1; // Forcer redessin complet
    lastDisplayedSEC = -1;
    updateStaticDisplay();
    updateCentisecondsDisplay();
    displayStatusLine3();

    resetActivityTimer(); // Réinitialiser le timer d'inactivité
    awokeByInterrupt = true; // Mettre un flag (peut être utilisé dans loop() si besoin)
}

// <<< AJOUT VEILLE >>> ISR pour le réveil (Doit être courte !)
ISR(PCINT2_vect) {
  // Juste pour réveiller le CPU. Ne rien faire d'autre ici.
  // Le flag awokeByInterrupt est mis après sleep_cpu() pour plus de sûreté.
}
// <<< FIN AJOUT VEILLE >>>
