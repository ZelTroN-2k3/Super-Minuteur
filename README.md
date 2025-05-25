# Super Minuteur & Métronome Arduino Avancé v1.8.0

![Super Minuteur Biotier](./images/IMG_20250426_120038.jpg)

Ce projet est une minuterie/compte à rebours avancé et un métronome basé sur Arduino (testé sur Nano, compatible avec d'autres AVR). Il utilise un encodeur rotatif avec bouton poussoir pour l'interaction, un écran LCD I2C 20x4 pour l'affichage, et inclut des fonctionnalités étendues comme l'affichage en grands chiffres, les temps préréglés, la sélection de mélodies de fin (y compris "Bip-Bip"), la sauvegarde des préférences en EEPROM et une gestion de l'énergie par mise en veille. La structure du code a été refactorisée en modules (timer, metronome) pour une meilleure lisibilité et maintenance.

## Fonctionnalités Principales

![Super Minuteur Ecran](./images/IMG_20250426_120127.jpg)

* **Compte à Rebours Polyvalent :**
    * Réglage manuel du temps par paliers de 10 secondes (configurable dans `conf.h`) via l'encodeur rotatif.
    * Limite de temps manuel configurable (`MAX_TOTAL_SECONDS` dans `conf.h`).
    * Sélection de temps préréglés (ex: 1min, 2min, 3min + mode Manuel) via un menu de configuration.
    * Sauvegarde en EEPROM du dernier mode utilisé (Manuel ou Preset) et de la dernière valeur manuelle réglée.
* **Mode Métronome :**
    * Réglage du BPM (Battements Par Minute) via l'encodeur (lorsque le métronome est arrêté), affiché en grands chiffres et sauvegardé en EEPROM.
    * Sélection du BPM via un menu de préréglages de tempo classiques (ex: Grave, Largo, Allegro) qui règle le BPM.
    * Plage de BPM configurable (ex: 20-240 BPM, ajusté pour les presets).
    * Signature rythmique X/Y (Numérateur ET Dénominateur) entièrement réglable via le menu "Metro.Rythm", affichée et sauvegardée en EEPROM.
    * Indicateur visuel du battement sur l'écran LCD (séquence de barres `upperBar`).
    * Signal sonore distinct pour le premier temps (accent) et les autres temps.
    * Affichage du nom du tempo classique sélectionné sur l'écran principal du métronome si le BPM correspond.
* **Affichage Amélioré :**
    * Écran LCD I2C 20x4.
    * Affichage du temps MM:SS (Minuterie) ou du BPM (Métronome) en grands chiffres sur 2 lignes grâce à la bibliothèque `BigNumbers_I2C` (fournie).
    * Affichage des centièmes de seconde (.CS) en taille normale pendant le décompte de la minuterie.
    * Ligne de statut indiquant "TIMER START", "TIMER STOP | MM:SS" (temps cible), "METRO RUN", ou "METRO STOP".
    * Ligne d'information (ligne 3 de l'écran principal du minuteur) indiquant la mélodie sélectionnée (ou `*Mel. Off` si désactivée) et le mode Preset/Manuel actif (ex: `*StarWars | P2` ou `*Mel. Off | Manuel`).
    * Affichage de l'état `On/Off` et des valeurs actuelles (BPM, Signature Rythmique X/Y) pour les options de menu configurables.
    * Écran de démarrage (Boot Screen) en deux étapes avec titre, puis infos auteur/version/date.
* **Alertes de Fin de Minuterie Configurables :**
    * Joue une mélodie sélectionnable à la fin du décompte (6 options incluant Mario, Star Wars, Zelda, Nokia, Tetris, Bip-Bip implémentés dans `melodie.h`).
    * Choix de la mélodie via le menu de configuration, sauvegardé en EEPROM.
    * Option pour activer/désactiver globalement la mélodie de fin via le menu "Melodie O/F" (état `On/Off` sauvegardé en EEPROM).
    * Clignotement non-bloquant du rétroéclairage pendant 5 secondes après la mélodie.
* **Contrôles et Interface Utilisateur :**
    * Interface utilisateur simple via encodeur rotatif (régler temps / naviguer menu) et bouton poussoir (Start/Stop/Pause/Resume / Sélection Menu / Entrer Menu via appui long).
    * Navigation dans les menus améliorée : défilement fonctionnel pour toutes les options (y compris le menu "Veille"), retour au menu principal des réglages après sélection d'un "Preset" ou sortie du mode Métronome.
    * Sortie Relais (configurable dans `conf.h`) activée (LOW) pendant le décompte de la minuterie.
    * Sortie Buzzer (configurable dans `conf.h`) pour les mélodies, les clics du métronome et le feedback sonore de l'interface.
    * Option "FeedbackSon: On/Off" pour activer/désactiver les clics sonores de l'interface, sauvegardée en EEPROM.
* **Gestion de l'Énergie :**
    * Mode veille automatique après une période d'inactivité configurable (uniquement lorsque la minuterie et le métronome sont arrêtés).
    * Le décompte de veille est réinitialisé après la fin complète d'un cycle de minuterie (mélodie et clignotement inclus).
    * Délai avant mise en veille réglable via le menu (ex: Off, 1min, 5min, 10min).
    * Réveil instantané par appui sur le bouton de l'encodeur.
    * Réglage de veille sauvegardé en EEPROM.
* **Configuration Facile :**
    * Fichier `conf.h` pour centraliser la configuration des broches, de l'écran LCD, des limites de temps, des valeurs de presets, des adresses EEPROM, des options de veille, des paramètres du métronome (y compris les plages pour le numérateur et le dénominateur de la signature rythmique, et les préréglages de tempo), etc.
    * **Attention aux adresses EEPROM :** `EEPROM_ADDR_METRONOME_BPM` (type `int`) utilise 2 octets. Assurez-vous que `EEPROM_ADDR_METRONOME_TS_NUM`, `EEPROM_ADDR_METRONOME_TS_DEN` et les adresses suivantes (comme `EEPROM_ADDR_TIMER_MELODY_ENABLED`) sont correctement décalées pour éviter tout chevauchement. Vérifiez leur séquence dans `conf.h`.
    * Fichier `melodie.h` pour les définitions des notes et l'ajout/modification facile des mélodies.

## Matériel Requis

* Carte Arduino (Nano, Uno, ou compatible AVR avec suffisamment de mémoire)
* Encodeur Rotatif avec Bouton Poussoir (KY-040 ou similaire)
* Écran LCD I2C 20x04 (avec adaptateur PCF8574 ou similaire)
* Buzzer Actif ou Passif (le code utilise `tone()`)
* Module Relais 5V (ou une LED avec résistance pour test)
* Câblage Dupont / Breadboard
* Alimentation appropriée pour l'Arduino

## Bibliothèques Requises

* `<EEPROM.h>` (Intégrée à l'IDE Arduino)
* `<Wire.h>` (Intégrée à l'IDE Arduino)
* `<string.h>` (Intégrée, utilisée pour `strlen` dans les menus)
* **LiquidCrystal\_I2C :** À installer via le gestionnaire de bibliothèques de l'IDE Arduino (cherchez "LiquidCrystal I2C", plusieurs versions existent, celle de Frank de Brabander ou John Rickman sont courantes).
* **RotaryEncoder :** À installer via le gestionnaire de bibliothèques (cherchez "RotaryEncoder" par Matthias Hertel).
* **BigNumbers\_I2C :** Les fichiers `BigNumbers_I2C.h` et `BigNumbers_I2C.cpp` sont inclus dans ce dépôt. Placez-les dans le dossier de votre sketch ou dans votre dossier `libraries` Arduino.
* *(Note : Les fonctions de veille utilisent `<avr/sleep.h>`, `<avr/power.h>`, `<avr/interrupt.h>` qui font partie de la toolchain AVR-GCC standard et ne nécessitent pas d'installation séparée).*

## Installation et Configuration

1.  **Connectez le matériel** en suivant les définitions de broches dans le fichier `conf.h` (`BUTTON_PIN`, `RELAY_PIN`, `BUZZER_PIN`, `ENCODER_DT_PIN`, `ENCODER_CLK_PIN`) ainsi que les broches I2C (SDA, SCL) de votre Arduino à l'écran LCD.
2.  **Installez les bibliothèques** `LiquidCrystal_I2C` et `RotaryEncoder` via le gestionnaire de bibliothèques de l'IDE Arduino si elles ne sont pas déjà présentes.
3.  **Placez les fichiers** `BigNumbers_I2C.h` et `BigNumbers_I2C.cpp` dans le dossier de votre sketch ou dans le dossier `libraries` de votre installation Arduino.
4.  **Placez les fichiers** `conf.h`, `melodie.h`, `melodie.cpp`, `timer.h`, `timer.cpp`, `metronome.h`, `metronome.cpp`, et le fichier `.ino` principal dans le même dossier de sketch.
5.  **Ouvrez le fichier `.ino`** avec l'IDE Arduino.
6.  **(Important)** Modifiez le fichier `conf.h` pour :
    * Définir votre nom dans `AUTHOR_NAME`.
    * Définir un numéro de version dans `FIRMWARE_VERSION` (ex: "1.8.0_METRO").
    * Vérifier et ajuster si nécessaire les numéros de broches (`BUTTON_PIN`, etc.).
    * Vérifier l'adresse I2C de votre écran (`LCD_ADDR`).
    * Ajuster `MAX_TOTAL_SECONDS`, `SECOND_INCREMENT`, `PRESET_VALUES`, `NUM_MELODIES`, `SLEEP_DELAY_VALUES`, `NUM_SLEEP_OPTIONS`, les paramètres du métronome (`MIN_BPM`, `MAX_BPM`, `MIN_TIME_SIGNATURE_NUMERATOR`, `MAX_TIME_SIGNATURE_NUMERATOR`, `MIN_TIME_SIGNATURE_DENOMINATOR`, `MAX_TIME_SIGNATURE_DENOMINATOR`, `NUM_TEMPO_PRESETS`, etc.) si désiré.
    * **Vérifiez attentivement la séquence et l'unicité des adresses EEPROM** (ex: `EEPROM_ADDR_METRONOME_BPM` utilise 2 octets, donc `EEPROM_ADDR_METRONOME_TS_NUM`, `EEPROM_ADDR_METRONOME_TS_DEN` doivent suivre, puis `EEPROM_ADDR_TIMER_MELODY_ENABLED`, etc.).
7.  **Compilez et téléversez** le code sur votre Arduino.

## Utilisation

* **Démarrage :** L'appareil affiche deux écrans de démarrage, puis l'interface principale du minuteur en mode arrêté, chargé avec le dernier preset utilisé ou le dernier temps manuel sauvegardé. La ligne du bas indique la mélodie active (ou "Mel. Off") et le mode (Manuel/Px.Min).
* **Réglage Manuel (Minuterie) :** Lorsque le minuteur est arrêté, tournez l'encodeur pour régler le temps. L'affichage MM:SS cible apparaît sur la ligne 0, et le statut en bas passe à "Manuel".
* **Démarrage Minuterie :** Appuyez brièvement sur le bouton lorsque du temps est affiché. "TIMER START" s'affiche, le relais s'active.
* **Pause/Reprise Minuterie :** Un appui court pendant le décompte met en Pause. Un autre appui court reprend le décompte.
* **Arrêt Minuterie (depuis Pause) :** Un appui long pendant que la minuterie est en Pause l'arrête complètement et réinitialise au temps cible.
* **Fin du Timer :** Mélodie (si activée), puis clignotement du rétroéclairage. Le temps cible est rechargé.
* **Mise en Veille Automatique :** Si configurée et que l'appareil est inactif (minuterie et métronome arrêtés), il se met en veille. Réveil par appui bouton.
* **Menu Réglages :** Lorsque le minuteur ou le métronome est arrêté, faites un **appui long** sur le bouton pour entrer dans le menu.
* **Navigation Menu :** Tournez l'encodeur pour sélectionner une option dans la liste (ex: "Melodie", "Preset", ..., "Metro.Rythm", "Tempo Class.", "Quitter"). Les valeurs actuelles ou états (On/Off, BPM, X/Y) sont affichés directement dans ce menu.
* **Sélection Menu :** Appuyez brièvement sur le bouton pour :
    * Entrer dans le sous-menu correspondant ("Melodie", "Preset", "Veille", "Metro.Rythm", "Tempo Class.").
    * Basculer l'état pour "FeedbackSon" ou "Melodie O/F" (affiche On/Off, sauvegarde en EEPROM, et revient au menu principal des réglages).
    * Entrer en mode Métronome ("Metronome").
    * Quitter le menu ("Quitter") pour revenir au mode Minuterie.
* **Sous-Menus (Melodie, Preset, Veille, Metro.Rythm, Tempo Class.) :** Tournez l'encodeur pour choisir l'option ou la valeur, appuyez brièvement pour valider.
    * Valider une mélodie, un preset, un délai de veille, une signature rythmique (après avoir réglé numérateur et dénominateur), ou un préréglage de tempo sauvegarde le choix et revient au menu principal des réglages (ou directement au mode métronome pour le préréglage de tempo).
* **Mode Métronome :**
    * Accès via "Menu Réglages" -> "Metronome".
    * Lorsque "METRO STOP" est affiché, tournez l'encodeur pour régler le BPM. La modification est sauvegardée en EEPROM. La signature rythmique (X/Y) et le nom du tempo classique (si applicable) sont affichés.
    * La signature rythmique X/Y est réglable via le menu "Metro.Rythm".
    * Les préréglages de tempo classiques sont sélectionnables via le menu "Tempo Class.".
    * Appuyez brièvement sur le bouton pour Démarrer ("METRO RUN") ou Arrêter ("METRO STOP") le métronome.
    * Un appui long sur le bouton en mode métronome (arrêté ou en marche) quitte le mode métronome et retourne au menu principal des réglages.

## Fichiers du Projet

* `*.ino` : Code principal Arduino gérant la logique de haut niveau, les états, et l'interaction principale.
* `conf.h` : Fichier de configuration pour les broches, constantes, adresses EEPROM, etc.
* `melodie.h` / `melodie.cpp`: Définitions des notes et fonctions pour les mélodies.
* `timer.h` / `timer.cpp`: Logique et fonctions spécifiques à la minuterie.
* `metronome.h` / `metronome.cpp`: Logique et fonctions spécifiques au métronome.
* `BigNumbers_I2C.h` / `BigNumbers_I2C.cpp` : Bibliothèque pour l'affichage des grands chiffres (fournie).

## Ecran Boot Screen 1:

![Ecran principal](./images/IMG_20250426_120122.jpg)

## Ecran Boot Screen 2:

![Ecran principal](./images/IMG_20250426_120125.jpg)

## Ecran Menu Reglages:

![Ecran principal](./images/IMG_20250426_120138.jpg)
*(Note : Cette image pourrait ne pas refléter les toutes dernières options de menu comme "Melodie O/F", "Metro.Rythm", "Tempo Class." ou les états/valeurs affichés à côté des items).*

## Auteur

* [ANCHER.P - à définir dans conf.h]

## Licence

* GNU General Public License V3 (GPLv3)