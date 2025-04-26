//***************************************************************
//  Super Minuteur - Code Principal (.ino)
//  - AFFICHAGE STATUT PRESET/MANUEL AJOUTÉ sur ligne 3
//  - Menu sélection Mélodie ET Preset Temps (runtime)
//  - Sauvegarde EEPROM des choix
//  - 3 Mélodies + 3 Presets + Manuel
//  - Clignotement Rétroéclairage en Fin
//  - Ajout deuxième écran de démarrage (Infos Auteur/Version)
//
//  Date de génération : Vendredi 25 avril 2025 22:45:00 CEST
//  Lieu : Montmédy, Grand Est, France
//**************************************************************

#include <EEPROM.h>
#include "Wire.h"
#include <string.h> // Pour strlen (utilisé dans les menus)
#include "LiquidCrystal_I2C.h"
#include "RotaryEncoder.h"
#include "BigNumbers_I2C.h"

#include "conf.h"     // Inclut la configuration générale
#include "melodie.h"  // Inclut les définitions de notes et les fonctions mélodies

// --- Énumération pour les Modes ---
enum Mode {
  MODE_TIMER,          // Fonctionnement normal du timer
  MODE_MENU_MAIN,      // Menu principal (Choix Melodie/Preset/Quitter)
  MODE_MENU_MELODY,    // Sous-menu choix mélodie
  MODE_MENU_PRESET     // Sous-menu choix preset
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
unsigned long targetEndTime = 0;    // Heure de fin calculée
long remainingMillis = 0;           // Temps restant calculé
int lastDisplayedSEC = -1;
int lastDisplayedMIN = -1; // Pour affichage intelligent
unsigned long lastCsUpdateTime = 0;
const unsigned long interval = 1000; // Pour décompte

// Variables pour le clignotement final du rétroéclairage
bool isEndSequenceBlinking = false;
unsigned long blinkSequenceStartTime = 0;
const unsigned long blinkSequenceDuration = 5000; // Peut être dans conf.h
unsigned long lastEndBlinkToggleTime = 0;
const unsigned long endBlinkInterval = 250;       // Peut être dans conf.h
bool endBlinkStateIsOn = true;

// Variables Menu
byte currentMelodyChoice = 1;
byte currentPresetChoice = 0;
byte menuMainIndex = 1;
byte menuMelodyIndex = 1;
byte menuPresetIndex = 0;     // Sélection dans le sous-menu preset (0 à NUM_PRESETS-1)
byte presetMenuScrollOffset = 0; // Décalage pour le scrolling
int lastEncoderMenuPos = 0;   // Pour la navigation dans le menu avec encodeur

const char* mainMenuItems[] = { " Melodie", " Preset ", " Quitter" };
const char* melodyNames[] = { "Mario", "StarWars", "Zelda" };
const char* presetNames[] = { "Manuel", "1 Min", "2 Min", "3 Min" };


// --- Déclarations de fonctions ---
// (Mélodies dans melodie.h)
void blinkBacklightOnTimerEnd(); void updateStaticDisplay(); void updateCentisecondsDisplay();
void handleEncoder(); void handleButton(); void timerEnd(); void enterMainMenu();
void displayMainMenu(); void navigateMainMenu(int diff); void selectMainMenuItem();
void enterMelodyMenu(); void displayMelodyMenu(); void navigateMelodyMenu(int diff); void selectMelodyMenuItem();
void enterPresetMenu(); void displayPresetMenu(); void navigatePresetMenu(int diff); void selectPresetMenuItem();
void exitMenu(); void saveChoiceToEEPROM(int address, byte value);
void displayStatusLine3(); // <<< RENOMMÉE (anciennement displaySelectedMelodyName)
void clearRestOfLine(byte startCol, byte row); // Helper d'effacement

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
  LCD.setCursor(0, 0); LCD.print("Auteur: "); LCD.print(AUTHOR_NAME);       // Utilise constante de conf.h
  LCD.setCursor(0, 1); LCD.print("Version: "); LCD.print(FIRMWARE_VERSION); // Utilise constante de conf.h
  LCD.setCursor(0, 2); LCD.print("Date: "); LCD.print(__DATE__);          // Date de compilation
  LCD.setCursor(0, 3); LCD.print(__TIME__);                               // Heure de compilation

  delay(2500); // Affiche l'écran d'infos pendant 2.5 secondes
  LCD.clear(); // Efface l'écran après l'écran d'infos
  // --- FIN ÉCRAN DE DÉMARRAGE 2 ----

  // --- Lire les choix sauvegardés en EEPROM ---
  byte savedMelody = EEPROM.read(EEPROM_ADDR_MELODY);
  if (savedMelody < 1 || savedMelody > NUM_MELODIES) { currentMelodyChoice = 1; EEPROM.update(EEPROM_ADDR_MELODY, currentMelodyChoice); }
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

  // Affichage initial (Mode Timer)
  currentMode = MODE_TIMER;
  updateStaticDisplay();
  updateCentisecondsDisplay();
  displayStatusLine3(); // <<< APPEL FONCTION RENOMMÉE/MODIFIÉE
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
        isEndSequenceBlinking = false; LCD.backlight();
      } else {
        if (currentTime - lastEndBlinkToggleTime >= endBlinkInterval) {
          lastEndBlinkToggleTime = currentTime; endBlinkStateIsOn = !endBlinkStateIsOn;
          if (endBlinkStateIsOn) { LCD.backlight(); } else { LCD.noBacklight(); }
        }
      }
    }
  }
} // Fin loop()


// --- Fonctions Auxiliaires ---

// Gère l'encodeur (MODIFIÉ pour appeler la bonne fonction d'affichage)
void handleEncoder() {
  if (isEndSequenceBlinking) { isEndSequenceBlinking = false; LCD.backlight(); }
  encoder.tick();

  if (currentMode == MODE_TIMER) {
    if (!timerRunning) {
       int currentEncoderPos = encoder.getPosition();
       newPos = currentEncoderPos * STEPS;
       if (newPos < POSMIN) { encoder.setPosition(POSMIN / STEPS); newPos = POSMIN; }
       else if (newPos > POSMAX) { encoder.setPosition(POSMAX / STEPS); newPos = POSMAX; }
       if (lastPos != newPos) {
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
       switch(currentMode) {
           case MODE_MENU_MAIN: navigateMainMenu(diff); break;
           case MODE_MENU_MELODY: navigateMelodyMenu(diff); break;
           case MODE_MENU_PRESET: navigatePresetMenu(diff); break;
           default: break;
       }
       lastEncoderMenuPos = currentEncoderMenuPos;
     }
  }
}

// Gère le bouton (CORRIGÉ pour arrêt manuel et redémarrage preset)
void handleButton() {
  // Interrompt le clignotement si actif
  if (isEndSequenceBlinking) {
    isEndSequenceBlinking = false;
    LCD.backlight();
  }

  boolean buttonIsUp = digitalRead(BUTTON_PIN); // Lire état actuel

  // Détection appui (front descendant)
  if (buttonWasUp && !buttonIsUp) {
    buttonDownTime = millis();    // Mémorise l'heure de l'appui
    longPressDetected = false;    // Réinitialise le flag d'appui long
  }

  // Détection maintien appui long (pendant que le bouton EST enfoncé)
  if (!buttonWasUp && !longPressDetected && (millis() - buttonDownTime >= longPressDuration)) {
     longPressDetected = true; // Marque l'appui long comme détecté
     // --- Action Appui Long ---
     if (currentMode == MODE_TIMER && !timerRunning) { // Ne peut entrer dans le menu que si le timer est arrêté
        enterMainMenu();
     }
     // Pas d'action spécifique pour appui long dans les menus pour l'instant
  }

  // Détection relâchement (front montant)
  if (!buttonWasUp && buttonIsUp) {
    // Vérifie si ce relâchement correspond à un appui court (non détecté comme long)
    // Et applique l'anti-rebond standard (debounceDelay de conf.h)
    if (!longPressDetected && (millis() - buttonDownTime >= debounceDelay)) {
        // --- Action Appui Court ---
        switch (currentMode) {
            case MODE_TIMER: // Agit comme Start/Stop
                if (timerRunning) { // ****** Bloc Arrêter (MODIFIÉ) ******
                    timerRunning = false; // Arrête le drapeau du timer

                    // Reset affichage à 00:00.00 et met à jour LCD
                    displayMIN = 0; displaySEC = 0; displayCS = 0;
                    updateStaticDisplay();       // Affiche STOP et 00:00
                    updateCentisecondsDisplay();   // Affiche .00
                    displayStatusLine3();        // Affiche Mélodie | Statut (qui n'a pas changé)

                    // Coupe relais et son
                    digitalWrite(RELAY_PIN, HIGH);
                    noTone(BUZZER_PIN); // Arrête le son si on interrompt manuellement

                     // --- MODIFICATION ICI ---
                     // Reset temps cible ou recharge dernier temps manuel/preset
                     if (currentPresetChoice == 0) { // Si c'était un timer manuel qui tournait
                         // Recharge la dernière valeur manuelle depuis EEPROM
                         unsigned int savedManualTime = 0;
                         // Utilise EEPROM.get pour lire les 2 octets de l'unsigned int
                         EEPROM.get(EEPROM_ADDR_MANUAL_TIME, savedManualTime);
                         // Validation simple (ne devrait pas être nécessaire si EEPROM fiable)
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
                     // Remet l'encodeur à 0 dans tous les cas après arrêt manuel
                     lastPos = 0;
                     encoder.setPosition(POSMIN / STEPS);
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
        } // Fin switch(currentMode)
    } // Fin if debounce
    // Réinitialise le flag d'appui long au relâchement, après les vérifications
    longPressDetected = false;
  }
  // Mémorise l'état du bouton pour la prochaine boucle
  buttonWasUp = buttonIsUp;
} // --- Fin handleButton ---


// Gère la fin du timer (MODIFIÉE pour conserver le preset)
void timerEnd() {
  timerRunning = false; // Arrête le drapeau du timer

  // Réinitialise l'affichage à 00:00.00 et met à jour l'écran
  displayMIN = 0; displaySEC = 0; displayCS = 0;
  updateStaticDisplay();       // Affiche STOP et 00:00
  updateCentisecondsDisplay();   // Affiche .00
  displayStatusLine3();        // Réaffiche Mélodie | Statut Preset/Manuel (qui n'a pas changé)
  lastDisplayedMIN = -1;       // Force le rafraîchissement si on redémarre vite
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
  // Remet l'encodeur à 0 pour être prêt
  lastPos = 0;
  encoder.setPosition(POSMIN / STEPS);
  // --- FIN MODIFICATION ---

  // --- Séquence de Fin (Mélodie + Clignotement) ---
  // 1. Joue la mélodie CHOISIE
  if (currentMelodyChoice == 1) { playMarioMelody(); }
  else if (currentMelodyChoice == 2) { playImperialMarch(); }
  else if (currentMelodyChoice == 3) { playZeldaTheme(); }

  // 2. Fait clignoter le rétroéclairage
  if (!blinkDone) {
      blinkBacklightOnTimerEnd();
      blinkDone = true;
  }
}

// --- Fonctions du Menu ---

// Passe en mode Menu Principal
void enterMainMenu() {
    currentMode = MODE_MENU_MAIN;
    timerRunning = false; isEndSequenceBlinking = false; LCD.backlight(); noTone(BUZZER_PIN);
    menuMainIndex = 1; // Commence sur la première option "Melodie"
    lastEncoderMenuPos = encoder.getPosition(); // Synchro encodeur pour éviter sauts
    displayMainMenu();
}

// Affichage Menu Principal (CORRIGÉ)
void displayMainMenu() {
    LCD.clear();
    LCD.setCursor(0, 0); LCD.print("Menu Reglages:");
    for (byte i = 1; i <= 3; i++) { // 3 options
        byte displayRow = i;
        LCD.setCursor(0, displayRow);
        if (i == menuMainIndex) { LCD.print(">"); } else { LCD.print(" "); }
        LCD.print(i); LCD.print(mainMenuItems[i - 1]);

        byte startColClear = 15; // Colonne fixe sûre pour commencer l'effacement
        if (i == 1) { // Ligne Mélodie
            LCD.print(": ");
            if ((currentMelodyChoice - 1) < NUM_MELODIES) LCD.print(melodyNames[currentMelodyChoice - 1]);
            else LCD.print("???");
            // startColClear = LCD.getCursorCol(); // ERREUR SUPPRIMÉE
        } else if (i == 2) { // Ligne Preset
            LCD.print(": ");
            if (currentPresetChoice < NUM_PRESETS) LCD.print(presetNames[currentPresetChoice]);
            else LCD.print("???");
            // startColClear = LCD.getCursorCol(); // ERREUR SUPPRIMÉE
        } else { // Ligne Quitter
             startColClear = 3 + strlen(mainMenuItems[i-1]); // Peut être plus précis ici
             if(startColClear < 15) startColClear = 15; // Assure au moins 15
        }
        clearRestOfLine(startColClear, displayRow); // Efface fin de ligne
    }
}

// Navigue dans le Menu Principal
void navigateMainMenu(int diff) {
    if (diff > 0) { menuMainIndex++; if (menuMainIndex > 3) menuMainIndex = 1; }
    else { menuMainIndex--; if (menuMainIndex < 1) menuMainIndex = 3; }
    displayMainMenu();
}

// Sélectionne un item dans le Menu Principal
void selectMainMenuItem() {
    switch (menuMainIndex) {
        case 1: enterMelodyMenu(); break; // Ouvre sous-menu mélodie
        case 2: enterPresetMenu(); break; // Ouvre sous-menu preset
        case 3: exitMenu(); break;        // Quitte tous les menus
    }
}

// --- Sous-Menu Mélodie ---
void enterMelodyMenu() {
    currentMode = MODE_MENU_MELODY;
    menuMelodyIndex = currentMelodyChoice; // Sélection initiale = choix actuel
    lastEncoderMenuPos = encoder.getPosition(); // Synchro encodeur
    displayMelodyMenu();
}

// Affichage Sous-Menu Mélodie (CORRIGÉ)
void displayMelodyMenu() {
    LCD.clear();
    LCD.setCursor(0, 0); LCD.print(" Choix Melodie:");
    byte displayLines = LCD_ROWS - 1; // Lignes dispo pour items
    for (byte i = 0; i < displayLines; i++) { // Affiche max 3 lignes
       byte displayRow = i + 1;
       byte melodyIndex = i + menuMelodyIndex -1 ; // TODO: Logic might need adjustment for proper scrolling view - keeping simple for now
       // This simple version only shows first 3 - NEEDS SCROLLING like preset menu
       // Let's just display first 3 for now to fix compile error.
       melodyIndex = i + 1; // Show item 1, 2, 3

       LCD.setCursor(0, displayRow);
       if (melodyIndex <= NUM_MELODIES) {
           if (melodyIndex == menuMelodyIndex) { LCD.print(">"); } else { LCD.print(" "); }
           LCD.print(melodyIndex); LCD.print("-");
           LCD.print(melodyNames[melodyIndex - 1]);
           clearRestOfLine(15, displayRow); // Effacement fixe
       } else {
           clearRestOfLine(0, displayRow); // Efface ligne vide
       }
    }
     // Note: Scrolling logic for melody menu is NOT implemented here, only fixed display of first items.
}

// Navigue dans le sous-menu Mélodie (Sans scroll visuel pour l'instant)
void navigateMelodyMenu(int diff) {
    if (diff > 0) { menuMelodyIndex++; if (menuMelodyIndex > NUM_MELODIES) menuMelodyIndex = 1; }
    else { menuMelodyIndex--; if (menuMelodyIndex < 1) menuMelodyIndex = NUM_MELODIES; }
    displayMelodyMenu(); // Redessine (affiche les 3 premiers, mais le '>' bouge)
}

void selectMelodyMenuItem() {
    currentMelodyChoice = menuMelodyIndex; // Valide le choix
    saveChoiceToEEPROM(EEPROM_ADDR_MELODY, currentMelodyChoice); // Sauvegarde
    enterMainMenu(); // Revient au menu principal après sélection mélodie
}

// --- Sous-Menu Preset ---

// Initialise et affiche le sous-menu Preset (MODIFIÉ pour scrolling)
void enterPresetMenu() {
    currentMode = MODE_MENU_PRESET;
    menuPresetIndex = currentPresetChoice; // Sélection initiale = choix actuel

    // Calcule le décalage de défilement initial pour que l'item choisi soit visible
    byte displayLines = LCD_ROWS - 1; // Nombre de lignes pour les items (ex: 3 si 4 lignes LCD)
    if (menuPresetIndex >= displayLines) {
        // Si l'index est au-delà de ce qui est visible, on décale
        presetMenuScrollOffset = menuPresetIndex - displayLines + 1;
    } else {
        presetMenuScrollOffset = 0; // Sinon, on commence au début
    }
    // Assure que le décalage ne dépasse pas les limites possibles
     if (presetMenuScrollOffset > (NUM_PRESETS - displayLines)) {
         if (NUM_PRESETS >= displayLines) {
              presetMenuScrollOffset = NUM_PRESETS - displayLines;
         } else {
              presetMenuScrollOffset = 0; // Moins d'items que de lignes
         }
     }

    lastEncoderMenuPos = encoder.getPosition(); // Synchro encodeur
    displayPresetMenu(); // Affiche le menu avec le bon décalage
}

// Affichage Sous-Menu Preset (CORRIGÉ - utilise scrollOffset)
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

// Helper pour effacer la fin d'une ligne
void clearRestOfLine(byte startCol, byte row) {
    if (startCol >= LCD_COLS) return; // Sécurité
    LCD.setCursor(startCol, row);
    for (byte c = startCol; c < LCD_COLS; c++) { LCD.print(" "); }
}

// Navigue dans le sous-menu Preset (CORRIGÉ - variable inutilisée enlevée)
void navigatePresetMenu(int diff) {
    byte displayLines = LCD_ROWS - 1;
    // byte oldPresetIndex = menuPresetIndex; // SUPPRIMÉ (inutilisé)

    if (diff > 0) { // Down
        menuPresetIndex++;
        if (menuPresetIndex >= NUM_PRESETS) { menuPresetIndex = 0; presetMenuScrollOffset = 0; }
        else if (menuPresetIndex >= (presetMenuScrollOffset + displayLines)) {
            presetMenuScrollOffset++;
             if (presetMenuScrollOffset > (NUM_PRESETS - displayLines)) { if (NUM_PRESETS >= displayLines) { presetMenuScrollOffset = NUM_PRESETS - displayLines; } else { presetMenuScrollOffset = 0; } }
        }
    } else { // Up
        if (menuPresetIndex == 0) {
            menuPresetIndex = NUM_PRESETS - 1;
             if (NUM_PRESETS > displayLines) { presetMenuScrollOffset = NUM_PRESETS - displayLines; } else { presetMenuScrollOffset = 0; }
        } else {
            menuPresetIndex--;
            if (menuPresetIndex < presetMenuScrollOffset) { presetMenuScrollOffset = menuPresetIndex; }
        }
    }
    if (presetMenuScrollOffset > NUM_PRESETS) presetMenuScrollOffset = 0; // Sécurité

    displayPresetMenu();
}

void selectPresetMenuItem() {
    currentPresetChoice = menuPresetIndex; // Valide le choix (0 à 3)
    saveChoiceToEEPROM(EEPROM_ADDR_PRESET, currentPresetChoice); // Sauvegarde

    // Charge le temps du preset choisi (ou 0 si Manuel)
    if (currentPresetChoice > 0 && currentPresetChoice < NUM_PRESETS) {
        targetTotalSeconds = PRESET_VALUES[currentPresetChoice]; // PRESET_VALUES défini dans conf.h
    } else {
        targetTotalSeconds = 0; // Manuel
    }
    // Quitte le menu et applique directement le preset/manuel
    exitMenu();
}

// Quitte le menu (MODIFIÉ pour appeler la bonne fonction d'affichage)
void exitMenu() {
    currentMode = MODE_TIMER;
    LCD.clear(); // Efface le menu

    // Met à jour affichage timer avec preset/manuel chargé
    displayMIN = targetTotalSeconds / 60; displaySEC = targetTotalSeconds % 60; displayCS = 0;
    lastPos = targetTotalSeconds / SECOND_INCREMENT; encoder.setPosition(lastPos * STEPS);
    lastDisplayedMIN = -1; lastDisplayedSEC = -1; // Force rafraîchissement

    updateStaticDisplay();
    updateCentisecondsDisplay();
    displayStatusLine3(); // <<< APPEL FONCTION RENOMMÉE/MODIFIÉE
}

// Sauvegarde EEPROM
void saveChoiceToEEPROM(int address, byte value) { EEPROM.update(address, value); 
} // update ne réécrit que si la valeur est différente

// --- Fonctions d'Affichage (Timer) ---

// Fonction pour faire clignoter le rétroéclairage (BLOQUANTE)
// Utilise les globales corrigées
void blinkBacklightOnTimerEnd() {
   unsigned long startTime = millis();
   LCD.backlight();
   while (millis() - startTime < blinkSequenceDuration) { // Utilise globale
     LCD.noBacklight(); delay(endBlinkInterval); // Utilise globale
     if (millis() - startTime >= blinkSequenceDuration) break;
     LCD.backlight(); delay(endBlinkInterval); // Utilise globale
   }
   LCD.backlight();
}

// Met à jour uniquement la partie statique (Grands chiffres + Statut)
// (Utilise la correction pour l'effacement de la ligne statut)
void updateStaticDisplay() {
LCD.setCursor(STATUS_COL_START, STATUS_ROW); // Ligne 0
    if (timerRunning) {
        LCD.print("TIMER START");
        // Efface la zone où le temps cible serait affiché
        LCD.print("         "); 
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
    // Cette partie utilise displayMIN et displaySEC (qui valent 0 quand timerRunning est false)
    byte minTens = displayMIN / 10; byte minUnits = displayMIN % 10;
    byte secTens = displaySEC / 10; byte secUnits = displaySEC % 10;

    // (Optionnel : Effacement explicite des zones BigNumbers si vous aviez des problèmes)
    // LCD.setCursor(BIG_M1_COL, BIG_NUM_ROW); LCD.print("   "); ... etc ...

    // Affiche les grands chiffres (00:00 quand arrêté)
    bigNum.displayLargeNumber(minTens, BIG_M1_COL, BIG_NUM_ROW);
    bigNum.displayLargeNumber(minUnits, BIG_M2_COL, BIG_NUM_ROW);
    LCD.setCursor(COLON_COL, BIG_NUM_ROW); LCD.print(" "); // Votre style de séparateur
    LCD.setCursor(COLON_COL, BIG_NUM_ROW + 1); LCD.print("."); // Votre style de séparateur
    bigNum.displayLargeNumber(secTens, BIG_S1_COL, BIG_NUM_ROW);
    bigNum.displayLargeNumber(secUnits, BIG_S2_COL, BIG_NUM_ROW);
}

// Affichage Centisecondes (.CS)
void updateCentisecondsDisplay() {
  LCD.setCursor(CS_COL, CS_ROW); LCD.print(".");
  if (displayCS < 10) { LCD.print("0"); } LCD.print(displayCS); LCD.print(" ");
}

// *** RENOMMÉE ET MODIFIÉE : Affiche icône + nom mélodie + statut preset ***
void displayStatusLine3() {
    byte displayRow = MELODY_NAME_ROW; // Devrait être 3 (défini dans conf.h)
    byte startCol = MELODY_NAME_COL;   // Devrait être 1 (défini dans conf.h)

    // 1. Efface la ligne AVANT d'écrire pour éviter les restes
    clearRestOfLine(0, displayRow);

    // 2. Affiche Icône + Nom Mélodie
    LCD.setCursor(startCol - 1, displayRow);
    LCD.print("*"); // Icône simple
    LCD.print(" ");
    byte melodyIndex = currentMelodyChoice - 1;
    if (melodyIndex < NUM_MELODIES) { // Utilise NUM_MELODIES de conf.h
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
    // Pas besoin d'effacer après, la ligne a été effacée au début
}