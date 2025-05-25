#include "timer.h"

void setupTimer() {
  // Cette fonction est appelée depuis setup() dans le .ino principal.
  // Charger les préférences EEPROM pour le timer (preset, temps manuel)
  byte savedPreset = EEPROM.read(EEPROM_ADDR_PRESET);
  if (savedPreset >= NUM_PRESETS) { 
    currentPresetChoice = 0; 
    EEPROM.update(EEPROM_ADDR_PRESET, currentPresetChoice); 
  } else { 
    currentPresetChoice = savedPreset; 
  }
  // menuPresetIndex = currentPresetChoice; // menuPresetIndex est pour le menu, pas ici

  if (currentPresetChoice == 0) { // Mode Manuel
     EEPROM.get(EEPROM_ADDR_MANUAL_TIME, targetTotalSeconds);
     if (targetTotalSeconds > MAX_TOTAL_SECONDS) { 
        targetTotalSeconds = 0;
        EEPROM.put(EEPROM_ADDR_MANUAL_TIME, targetTotalSeconds);
     }
  } else { // Mode Preset
     if (currentPresetChoice < NUM_PRESETS) { 
        targetTotalSeconds = PRESET_VALUES[currentPresetChoice];
     } else { 
        targetTotalSeconds = 0;
        currentPresetChoice = 0; 
        EEPROM.update(EEPROM_ADDR_PRESET, currentPresetChoice);
     }
  }
  lastPos = targetTotalSeconds / SECOND_INCREMENT;
  // encoder.setPosition(lastPos * STEPS); // L'encodeur est initialisé dans le setup principal

  // Initialiser les variables d'affichage du timer
  displayMIN = targetTotalSeconds / 60;
  displaySEC = targetTotalSeconds % 60;
  displayCS = 0;
  lastDisplayedMIN = -1; 
  lastDisplayedSEC = -1;

  // currentTimerState = STATE_IDLE; // Est déjà initialisé dans le .ino
}


void loopTimer() {
  // Cette fonction est appelée depuis loop() dans le .ino principal lorsque currentMode == MODE_TIMER

  if (currentTimerState == STATE_RUNNING) {
    unsigned long currentTime = millis();
    // Gérer le cas où targetEndTime pourrait ne pas être encore initialisé correctement
    // ou si millis() a fait un rollover et est plus petit que targetEndTime après un long uptime.
    // Pour la robustesse, on vérifie si targetEndTime est supérieur à currentTime avant soustraction.
    if (targetEndTime >= currentTime) {
        remainingMillis = targetEndTime - currentTime;
    } else {
        remainingMillis = 0; // Le temps est écoulé ou erreur de synchronisation
    }

    if (remainingMillis <= 0) { 
        remainingMillis = 0; // Assurer qu'il n'est pas négatif
        timerEnd();          // Gérer la fin du timer
    } else {
      unsigned long totalRemainingSeconds = (remainingMillis + 999) / 1000; // Arrondi supérieur
      int currentMIN_disp = totalRemainingSeconds / 60;
      int currentSEC_disp = totalRemainingSeconds % 60;

      if (currentMIN_disp != displayMIN || currentSEC_disp != displaySEC) {
          displayMIN = currentMIN_disp;
          displaySEC = currentSEC_disp;
           if (displayMIN != lastDisplayedMIN || displaySEC != lastDisplayedSEC) {
               updateStaticDisplay(); 
               lastDisplayedMIN = displayMIN;
               lastDisplayedSEC = displaySEC;
           }
      }
      displayCS = (remainingMillis % 1000) / 10;
      if (currentTime - lastCsUpdateTime >= csUpdateInterval) {
          lastCsUpdateTime = currentTime;
          updateCentisecondsDisplay();
      }
    }
  } 
  else if (isEndSequenceBlinking) { 
    unsigned long currentTime = millis();
    if (currentTime - blinkSequenceStartTime >= blinkSequenceDuration) {
      isEndSequenceBlinking = false;
      LCD.backlight();
      resetActivityTimer(); 
    } else {
      if (currentTime - lastEndBlinkToggleTime >= endBlinkInterval) {
        lastEndBlinkToggleTime = currentTime;
        endBlinkStateIsOn = !endBlinkStateIsOn;
        if (endBlinkStateIsOn) { LCD.backlight(); }
        else { LCD.noBacklight(); }
      }
    }
  } 
  // La vérification de la mise en veille pour MODE_TIMER (quand STATE_IDLE) reste dans le loop() principal
  // car elle est étroitement liée à lastActivityTime qui est global et utilisé par d'autres modes.
}

void timerEnd() {
  currentTimerState = STATE_IDLE;
  displayMIN = 0; displaySEC = 0; displayCS = 0;
  updateStaticDisplay(); 
  updateCentisecondsDisplay(); 

  lastDisplayedMIN = -1; 
  lastDisplayedSEC = -1;
  digitalWrite(RELAY_PIN, HIGH); 

  if (currentPresetChoice == 0) { 
      EEPROM.get(EEPROM_ADDR_MANUAL_TIME, targetTotalSeconds);
      if (targetTotalSeconds > MAX_TOTAL_SECONDS) targetTotalSeconds = 0;
  } else { 
      if (currentPresetChoice < NUM_PRESETS) targetTotalSeconds = PRESET_VALUES[currentPresetChoice];
      else targetTotalSeconds = 0; 
  }
  lastPos = targetTotalSeconds / SECOND_INCREMENT;
  encoder.setPosition(lastPos * STEPS); 

  if (timerMelodyEnabled) { 
    byte melodyIdx = currentMelodyChoice;
    if (melodyIdx < NUM_MELODIES) { // Sécurité
        if (melodyIdx == 0) { playMarioMelody(); }
        else if (melodyIdx == 1) { playImperialMarch(); }
        else if (melodyIdx == 2) { playZeldaTheme(); }
        else if (melodyIdx == 3) { playNokiaTune(); }
        else if (melodyIdx == 4) { playTetrisTheme(); } 
        else if (melodyIdx == 5) { playBipBipMelody(); }
    }
  } 

  if (!blinkDone) { 
      isEndSequenceBlinking = true;
      blinkSequenceStartTime = millis();
      lastEndBlinkToggleTime = millis();
      endBlinkStateIsOn = false; 
      LCD.noBacklight();
  }
  blinkDone = true; 
}

void updateStaticDisplay() { 
  LCD.setCursor(STATUS_COL_START, STATUS_ROW);
    if (currentTimerState == STATE_RUNNING) {
        LCD.print(F("TIMER START         "));
        clearRestOfLine(strlen("TIMER START         "), STATUS_ROW);
    } else if (currentTimerState == STATE_PAUSED) {
        LCD.print(F("PAUSE               "));
        clearRestOfLine(strlen("PAUSE               "), STATUS_ROW);
    }
    else { // STATE_IDLE
        LCD.print(F("TIMER STOP | "));
        int targetMIN_disp = targetTotalSeconds / 60;
        int targetSEC_disp = targetTotalSeconds % 60;
        if (targetMIN_disp < 10) LCD.print("0"); LCD.print(targetMIN_disp);
        LCD.print(":");
        if (targetSEC_disp < 10) LCD.print("0"); LCD.print(targetSEC_disp);
        LCD.print(" "); 
        clearRestOfLine(strlen("TIMER STOP | MM:SS "), STATUS_ROW);
    }

    byte minTens = displayMIN / 10; byte minUnits = displayMIN % 10;
    byte secTens = displaySEC / 10; byte secUnits = displaySEC % 10;
    bigNum.displayLargeNumber(minTens, BIG_M1_COL, BIG_NUM_ROW);
    bigNum.displayLargeNumber(minUnits, BIG_M2_COL, BIG_NUM_ROW);
    LCD.setCursor(COLON_COL, BIG_NUM_ROW); LCD.print(" "); 
    LCD.setCursor(COLON_COL, BIG_NUM_ROW + 1); LCD.print("."); 
    bigNum.displayLargeNumber(secTens, BIG_S1_COL, BIG_NUM_ROW);
    bigNum.displayLargeNumber(secUnits, BIG_S2_COL, BIG_NUM_ROW);
}

void updateCentisecondsDisplay() {
  LCD.setCursor(CS_COL, CS_ROW); LCD.print(".");
  if (displayCS < 10) { LCD.print("0"); } 
  LCD.print(displayCS);
  LCD.print(" "); 
}

void handleTimerEncoderInput(int physicalEncoderPos) {
  // Cette fonction est appelée par handleEncoder() dans le .ino quand currentMode == MODE_TIMER
  if (currentTimerState == STATE_IDLE) {
     newPos = physicalEncoderPos; 

     if (newPos < POSMIN) {
         encoder.setPosition(POSMIN); 
         newPos = POSMIN;
     } else if (newPos > POSMAX) {
         encoder.setPosition(POSMAX); 
         newPos = POSMAX;
     }
    
     if (lastPos != newPos) { 
       playClickSound();
       resetActivityTimer();
       if (currentPresetChoice != 0) {
           currentPresetChoice = 0; 
           saveChoiceToEEPROM(EEPROM_ADDR_PRESET, currentPresetChoice);
           displayStatusLine3(); 
       }
       targetTotalSeconds = newPos * SECOND_INCREMENT; 
       lastPos = newPos; 
      
       displayMIN = targetTotalSeconds / 60;
       displaySEC = targetTotalSeconds % 60;
       displayCS = 0;
       updateStaticDisplay();
       updateCentisecondsDisplay();
       lastDisplayedMIN = displayMIN; 
       lastDisplayedSEC = displaySEC;
     }
  }
}

void handleTimerButtonShortPress() {
  // Cette fonction est appelée par handleButton() dans le .ino
  // pour un appui court quand currentMode == MODE_TIMER
  if (currentTimerState == STATE_RUNNING) { 
      currentTimerState = STATE_PAUSED;
      pausedRemainingMillis = remainingMillis;
      digitalWrite(RELAY_PIN, HIGH); 
      noTone(BUZZER_PIN); 
      updateStaticDisplay(); 
  } else if (currentTimerState == STATE_PAUSED) { 
      currentTimerState = STATE_RUNNING;
      targetEndTime = millis() + pausedRemainingMillis; 
      digitalWrite(RELAY_PIN, LOW); 
      updateStaticDisplay(); 
      lastCsUpdateTime = millis(); 
      remainingMillis = pausedRemainingMillis; 
      unsigned long totalRemainingSeconds = (remainingMillis + 999) / 1000;
      displayMIN = totalRemainingSeconds / 60;
      displaySEC = totalRemainingSeconds % 60;
      displayCS = (remainingMillis % 1000) / 10; // Recalculer displayCS
      updateCentisecondsDisplay(); // Afficher CS
      lastDisplayedMIN = displayMIN; 
      lastDisplayedSEC = displaySEC;
  } else if (currentTimerState == STATE_IDLE) { 
      if (targetTotalSeconds > 0) { 
          targetEndTime = millis() + (unsigned long)targetTotalSeconds * 1000UL;
          remainingMillis = targetEndTime - millis(); 
          if(remainingMillis < 0) remainingMillis = 0; 
          
          unsigned long totalRemainingSeconds = (remainingMillis + 999) / 1000;
          displayMIN = totalRemainingSeconds / 60;
          displaySEC = totalRemainingSeconds % 60;
          displayCS = (remainingMillis % 1000) / 10;
          
          currentTimerState = STATE_RUNNING;
          blinkDone = false; 
          
          if (currentPresetChoice == 0) { 
              EEPROM.put(EEPROM_ADDR_MANUAL_TIME, targetTotalSeconds);
          }
          digitalWrite(RELAY_PIN, LOW); 
          updateStaticDisplay(); 
          updateCentisecondsDisplay();
          displayStatusLine3(); 
          lastDisplayedMIN = displayMIN; 
          lastDisplayedSEC = displaySEC;
          lastCsUpdateTime = millis();
      }
  }
}

void handleTimerButtonLongPress() {
  // Cette fonction est appelée par handleButton() dans le .ino
  // pour un appui long quand currentMode == MODE_TIMER
  if (currentTimerState == STATE_PAUSED) { 
       currentTimerState = STATE_IDLE;
       pausedRemainingMillis = 0; 
       digitalWrite(RELAY_PIN, HIGH); 
       noTone(BUZZER_PIN); 
       
       if (currentPresetChoice == 0) { 
           EEPROM.get(EEPROM_ADDR_MANUAL_TIME, targetTotalSeconds);
           if (targetTotalSeconds > MAX_TOTAL_SECONDS) targetTotalSeconds = 0;
       } else { 
           if (currentPresetChoice < NUM_PRESETS) targetTotalSeconds = PRESET_VALUES[currentPresetChoice];
           else targetTotalSeconds = 0; 
       }
       lastPos = targetTotalSeconds / SECOND_INCREMENT;
       encoder.setPosition(lastPos * STEPS); 
       displayMIN = targetTotalSeconds / 60; 
       displaySEC = targetTotalSeconds % 60;
       displayCS = 0;
       updateStaticDisplay();
       updateCentisecondsDisplay();
       displayStatusLine3();
       lastDisplayedMIN = -1; 
       lastDisplayedSEC = -1; 
  }
  // Si STATE_IDLE, enterMainMenu() est géré dans le .ino principal
  // Si STATE_RUNNING, un appui long ne fait rien
}