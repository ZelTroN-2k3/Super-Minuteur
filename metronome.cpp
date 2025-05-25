// metronome.cpp

#include "metronome.h"

// État pour l'édition dans le menu de la signature rythmique
enum TSEditState { EDIT_NUM, EDIT_DEN, CONFIRM_TS };
static TSEditState currentTSEditState; // Garder l'état actuel de l'édition

void setupMetronome() {
  // Charger le BPM depuis l'EEPROM
  int temp_bpm;
  EEPROM.get(EEPROM_ADDR_METRONOME_BPM, temp_bpm);
  if (temp_bpm < MIN_BPM || temp_bpm > MAX_BPM) {
    currentBPM = DEFAULT_BPM;
    EEPROM.put(EEPROM_ADDR_METRONOME_BPM, currentBPM); 
  } else {
    currentBPM = temp_bpm;
  }

  // Charger le numérateur de la signature rythmique depuis l'EEPROM
  byte savedTSNum = EEPROM.read(EEPROM_ADDR_METRONOME_TS_NUM);
  if (savedTSNum < MIN_TIME_SIGNATURE_NUMERATOR || savedTSNum > MAX_TIME_SIGNATURE_NUMERATOR) {
    timeSignatureNum = DEFAULT_TIME_SIGNATURE_NUMERATOR;
    EEPROM.update(EEPROM_ADDR_METRONOME_TS_NUM, timeSignatureNum);
  } else {
    timeSignatureNum = savedTSNum;
  }

  // Charger le dénominateur de la signature rythmique depuis l'EEPROM // <<< NOUVEAU
  byte savedTSDen = EEPROM.read(EEPROM_ADDR_METRONOME_TS_DEN);
  if (savedTSDen < MIN_TIME_SIGNATURE_DENOMINATOR || savedTSDen > MAX_TIME_SIGNATURE_DENOMINATOR) {
    timeSignatureDen = DEFAULT_TIME_SIGNATURE_DENOMINATOR;
    EEPROM.update(EEPROM_ADDR_METRONOME_TS_DEN, timeSignatureDen);
  } else {
    timeSignatureDen = savedTSDen;
  }
}

void enterMetronomeMode() {
    resetActivityTimer();
    currentMode = MODE_METRONOME;
    currentMetroState = METRO_STOPPED;
    // LCD.clear(); // displayMetronomeScreen s'en chargera
    bigNum.begin(); 

    encoder.setPosition(currentBPM); // Initialiser l'encodeur avec le BPM actuel
    metroEncoderLastPos = currentBPM; 

    displayMetronomeScreen();
}

void displayMetronomeScreen() {
    resetActivityTimer(); // Peut-être pas nécessaire ici si appelé seulement au changement d'état
    LCD.clear(); // Effacer pour redessiner

    // Affichage du statut (RUN/STOP) et de la Signature Rythmique (TS) sur METRO_STATUS_ROW (ligne 0)
    LCD.setCursor(METRO_STATUS_COL, METRO_STATUS_ROW);
    if (currentMetroState == METRO_RUNNING) {
        LCD.print(F("METRO RUN "));
    } else {
        LCD.print(F("METRO STOP"));
    }
    LCD.setCursor(METRO_TS_COL, METRO_TS_ROW);
    LCD.print(F("TS:"));
    LCD.print(timeSignatureNum);
    LCD.print(F("/")); // <<< MODIFIÉ
    LCD.print(timeSignatureDen); // <<< MODIFIÉ
    
    // Calcul pour effacer correctement la fin de ligne pour TS
    byte tsTextLen = 3; // "TS:"
    if (timeSignatureNum >= 10) tsTextLen++; 
    tsTextLen++; // Numérateur
    tsTextLen++; // "/"
    if (timeSignatureDen >= 10) tsTextLen++; // Dénominateur (improbable si max 8, mais pour la forme)
    else tsTextLen++;
    clearRestOfLine(METRO_TS_COL + tsTextLen , METRO_TS_ROW);;

    // Affichage du BPM en grands chiffres sur METRO_BPM_BIG_NUM_ROW (lignes 1 et 2)
    if (currentBPM < 10) { // Normalement MIN_BPM est > 10
        bigNum.clearLargeNumber(METRO_BPM_BIG_NUM_COL, METRO_BPM_BIG_NUM_ROW);
        bigNum.clearLargeNumber(METRO_BPM_BIG_NUM_COL + 3, METRO_BPM_BIG_NUM_ROW);
        bigNum.displayLargeNumber(currentBPM, METRO_BPM_BIG_NUM_COL + 6, METRO_BPM_BIG_NUM_ROW);
    } else if (currentBPM < 100) {
        bigNum.clearLargeNumber(METRO_BPM_BIG_NUM_COL, METRO_BPM_BIG_NUM_ROW);
        bigNum.displayLargeNumber(currentBPM / 10, METRO_BPM_BIG_NUM_COL + 3, METRO_BPM_BIG_NUM_ROW);
        bigNum.displayLargeNumber(currentBPM % 10, METRO_BPM_BIG_NUM_COL + 6, METRO_BPM_BIG_NUM_ROW);
    } else {
        bigNum.displayLargeNumber(currentBPM / 100, METRO_BPM_BIG_NUM_COL, METRO_BPM_BIG_NUM_ROW);
        bigNum.displayLargeNumber((currentBPM / 10) % 10, METRO_BPM_BIG_NUM_COL + 3, METRO_BPM_BIG_NUM_ROW);
        bigNum.displayLargeNumber(currentBPM % 10, METRO_BPM_BIG_NUM_COL + 6, METRO_BPM_BIG_NUM_ROW);
    }

    // Gestion de METRO_BEAT_VISUAL_ROW (ligne 3)
    LCD.setCursor(0, METRO_BEAT_VISUAL_ROW);
    for (byte c = 0; c < LCD_COLS; c++) { LCD.print(" "); } // Effacer toute la ligne d'abord

    // --- AJOUT : Affichage du nom du Tempo Classique ---
    const char* currentTempoClassName = nullptr;
    for (byte i = 0; i < NUM_TEMPO_PRESETS; i++) {
        // tempoPresets est défini dans le .ino et déclaré extern dans conf.h
        // NUM_TEMPO_PRESETS est défini dans conf.h
        if (tempoPresets[i].bpm == currentBPM) {
            currentTempoClassName = tempoPresets[i].name;
            break;
        }
    }

    if (currentTempoClassName != nullptr) {
        // Calculer la position de départ pour le nom du tempo
        // Il sera affiché APRÈS les marqueurs de temps pour la signature actuelle.
        // Les marqueurs de temps vont de METRO_BEAT_MARKER_START_COL
        // jusqu'à METRO_BEAT_MARKER_START_COL + timeSignatureNum - 1.
        byte tempoNameStartCol = METRO_BEAT_MARKER_START_COL + timeSignatureNum;

        // Ajouter un espace de séparation si possible
        if (tempoNameStartCol < LCD_COLS -1 ) { // -1 pour laisser de la place au moins pour 1 caractère du nom
            LCD.setCursor(tempoNameStartCol, METRO_BEAT_VISUAL_ROW);
            LCD.print(" "); // Espace séparateur
            tempoNameStartCol++;
        }

        // Vérifier si on a de la place pour afficher le nom
        if (tempoNameStartCol < LCD_COLS) {
            LCD.setCursor(tempoNameStartCol, METRO_BEAT_VISUAL_ROW);
            byte availableSpace = LCD_COLS - tempoNameStartCol;
            String nameToPrint = String(currentTempoClassName);

            if (nameToPrint.length() > availableSpace) {
                nameToPrint = nameToPrint.substring(0, availableSpace); // Troncquer si trop long
            }
            LCD.print(nameToPrint);
        }
    }
    // --- FIN AJOUT ---
        
    // Positionnement du texte "BPM" selon votre modification
    LCD.setCursor(METRO_BEAT_VISUAL_COL + 7, METRO_BEAT_VISUAL_ROW - 1); // Ligne 2, colonne 16
    LCD.print(F("BPM"));

    // Si le métronome est arrêté, les marqueurs sont effacés par le nettoyage de ligne ci-dessus.
    // Si le métronome démarre, handleMetronomeLogic dessinera les marqueurs.
}

void handleMetronomeLogic() {
    if (currentMetroState == METRO_RUNNING) {
        unsigned long currentTime = millis();
        if (currentBPM == 0) return; // Évite la division par zéro
        unsigned long beatInterval = 60000UL / currentBPM;

        if (currentTime - lastMetroBeatTime >= beatInterval) {
            lastMetroBeatTime = currentTime;

            currentBeatInMeasure++;
            if (currentBeatInMeasure > timeSignatureNum || currentBeatInMeasure == 0) { // currentBeatInMeasure == 0 pour le tout premier temps
                currentBeatInMeasure = 1;
                // Début d'une nouvelle mesure : effacer les anciens marqueurs
                for (byte b = 0; b < timeSignatureNum; ++b) {
                    // S'assurer de ne pas écrire par-dessus "BPM" si METRO_BEAT_VISUAL_ROW est la même
                    // et que MAX_TIME_SIGNATURE_NUMERATOR est grand.
                    // Avec "BPM" sur la ligne du dessus, pas de souci ici.
                    LCD.setCursor(METRO_BEAT_MARKER_START_COL + b, METRO_BEAT_VISUAL_ROW);
                    LCD.print(" "); // Efface la position du marqueur
                }
            }

            playMetronomeBeatSound(currentBeatInMeasure == 1); // Joue le son du temps

            // Afficher le marqueur de temps actuel
            int beatDisplayColumn = METRO_BEAT_MARKER_START_COL + (currentBeatInMeasure - 1);
            // Vérifier que la colonne est valide et dans les limites de l'écran
            if (beatDisplayColumn < (METRO_BEAT_MARKER_START_COL + MAX_TIME_SIGNATURE_NUMERATOR) && beatDisplayColumn < LCD_COLS) {
                 LCD.setCursor(beatDisplayColumn, METRO_BEAT_VISUAL_ROW);
                 LCD.write(METRO_BEAT_MARKER_CHAR); // Affiche le caractère upperBar
            }
            resetActivityTimer();
        }
    } else { // currentMetroState == METRO_STOPPED
        // Aucune action dynamique d'affichage des temps n'est nécessaire ici,
        // displayMetronomeScreen() appelée lors de l'arrêt nettoie la ligne.
    }
}

void playMetronomeBeatSound(bool isAccent) {
    noTone(BUZZER_PIN); // Arrêter le son précédent au cas où
    if (isAccent) {
        tone(BUZZER_PIN, METRONOME_ACCENT_FREQ, METRONOME_ACCENT_DURATION);
    } else {
        tone(BUZZER_PIN, METRONOME_CLICK_FREQ, METRONOME_CLICK_DURATION);
    }
}

void saveBPMToEEPROM(int bpmValue) {
    EEPROM.put(EEPROM_ADDR_METRONOME_BPM, bpmValue);
}

// Fonctions pour le menu de réglage de la signature rythmique du métronome
void enterTSMetroMenu() {
    resetActivityTimer();
    currentMode = MODE_MENU_TS_METRO;
    currentTSEditState = EDIT_NUM; // Commencer par éditer le numérateur
    encoder.setPosition(timeSignatureNum); // Initialiser l'encodeur avec la valeur du numérateur
    lastEncoderMenuPos = timeSignatureNum; // Pour la navigation relative si vous l'utilisez ainsi
    displayTSMetroMenu();
}

void displayTSMetroMenu() {
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print(F("Signature Rythmique")); // Titre

    // Affichage Numérateur
    LCD.setCursor(0, 1);
    LCD.print(currentTSEditState == EDIT_NUM ? F(">") : F(" "));
    LCD.print(F("Num: "));
    if (timeSignatureNum < 10) LCD.print(F(" ")); // Alignement
    LCD.print(timeSignatureNum);
    LCD.print(F(" (")); LCD.print(MIN_TIME_SIGNATURE_NUMERATOR);
    LCD.print(F("-")); LCD.print(MAX_TIME_SIGNATURE_NUMERATOR); LCD.print(F(")"));
    clearRestOfLine(strlen(" Num: XX (AA-BB)") + 1, 1);


    // Affichage Dénominateur
    LCD.setCursor(0, 2);
    LCD.print(currentTSEditState == EDIT_DEN ? F(">") : F(" "));
    LCD.print(F("Den: "));
    if (timeSignatureDen < 10) LCD.print(F(" ")); // Alignement
    LCD.print(timeSignatureDen);
    LCD.print(F(" (")); LCD.print(MIN_TIME_SIGNATURE_DENOMINATOR);
    LCD.print(F("-")); LCD.print(MAX_TIME_SIGNATURE_DENOMINATOR); LCD.print(F(")"));
    clearRestOfLine(strlen(" Den: X (A-B)") + 1, 2);

    // Affichage Valider
    LCD.setCursor(0, 3);
    LCD.print(currentTSEditState == CONFIRM_TS ? F(">") : F(" "));
    LCD.print(F("Valider & Quitter"));
    clearRestOfLine(strlen(" Valider & Quitter") + 1, 3);
}

void navigateTSMetroMenu(int diff) { // diff est déjà relatif grâce à la gestion dans handleEncoder
    int tempVal;
    bool changed = false;

    if (currentTSEditState == EDIT_NUM) {
        tempVal = timeSignatureNum + diff;
        if (tempVal < MIN_TIME_SIGNATURE_NUMERATOR) tempVal = MIN_TIME_SIGNATURE_NUMERATOR;
        if (tempVal > MAX_TIME_SIGNATURE_NUMERATOR) tempVal = MAX_TIME_SIGNATURE_NUMERATOR;
        if (timeSignatureNum != (byte)tempVal) {
            timeSignatureNum = (byte)tempVal;
            encoder.setPosition(timeSignatureNum); // Mettre à jour la position de l'encodeur
            changed = true;
        }
    } else if (currentTSEditState == EDIT_DEN) {
        tempVal = timeSignatureDen + diff;
        if (tempVal < MIN_TIME_SIGNATURE_DENOMINATOR) tempVal = MIN_TIME_SIGNATURE_DENOMINATOR;
        if (tempVal > MAX_TIME_SIGNATURE_DENOMINATOR) tempVal = MAX_TIME_SIGNATURE_DENOMINATOR;
        if (timeSignatureDen != (byte)tempVal) {
            timeSignatureDen = (byte)tempVal;
            encoder.setPosition(timeSignatureDen); // Mettre à jour la position de l'encodeur
            changed = true;
        }
    }
    // Si CONFIRM_TS, l'encodeur ne fait rien pour l'instant, ou pourrait naviguer entre "Valider" et "Annuler"

    if (changed) {
        playClickSound();
        resetActivityTimer();
        lastEncoderMenuPos = encoder.getPosition(); // Mettre à jour pour la prochaine diff relative
        displayTSMetroMenu();
    }
}

void selectTSMetroMenuItem() {
    playClickSound();
    resetActivityTimer();

    if (currentTSEditState == EDIT_NUM) {
        currentTSEditState = EDIT_DEN;
        encoder.setPosition(timeSignatureDen); // Préparer l'encodeur pour éditer le dénominateur
        lastEncoderMenuPos = timeSignatureDen; 
    } else if (currentTSEditState == EDIT_DEN) {
        currentTSEditState = CONFIRM_TS;
        // Pas besoin de changer la position de l'encodeur ici, car on ne règle plus de valeur
    } else if (currentTSEditState == CONFIRM_TS) {
        EEPROM.update(EEPROM_ADDR_METRONOME_TS_NUM, timeSignatureNum);
        EEPROM.update(EEPROM_ADDR_METRONOME_TS_DEN, timeSignatureDen);
        enterMainMenu(); // Revenir au menu principal des réglages
        return; // Important pour ne pas juste rafraîchir le menu TS
    }
    displayTSMetroMenu();
}