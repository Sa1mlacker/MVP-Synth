#include <Arduino.h>
#include "pitches.h"

const int BUZZER_PIN    = 23;
const int BUZZER_CH     = 0;
const int MODE_BTN_PIN  = 19;   // кнопка режимів
const int POWER_BTN_PIN = 18;   // кнопка вкл/викл звуку

// 0 = off, 1 = scale (Hijaz), 2 = arabic pattern, 3 = improv
int mode = 1;                   // стартовий режим (наприклад 1)
bool soundEnabled = true;       // звук увімкнений

int lastModeBtnState  = HIGH;
int lastPowerBtnState = HIGH;

const int BASE_TEMPO_MS = 1800;

// --- Hijaz / Phrygian dominant на C ---
// Режим 1: гамма
int melodyScale[] = {
  NOTE_C4, NOTE_DB4, NOTE_E4, NOTE_F4,
  NOTE_G4, NOTE_AB4, NOTE_BB4, NOTE_C5,
  NOTE_BB4, NOTE_AB4, NOTE_G4, NOTE_F4,
  NOTE_E4, NOTE_DB4, NOTE_C4
};
int durScale[] = {
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4
};

// Режим 2: фіксований патерн
int melodyArab[] = {
  NOTE_C4,  NOTE_DB4, NOTE_E4,  NOTE_DB4,
  NOTE_C4,  NOTE_G4,  NOTE_F4,  NOTE_E4,
  NOTE_DB4, NOTE_C4,  NOTE_BB4, NOTE_AB4,
  NOTE_G4,  NOTE_F4,  NOTE_E4,  NOTE_DB4,
  NOTE_C4
};
int durArab[] = {
  4, 8, 8, 4,
  2, 8, 8, 4,
  4, 8, 8, 4,
  4, 4, 2
};

// Режим 3: «зважений» Hijaz
int hijazNotes[] = {
  NOTE_C4, NOTE_C4, NOTE_C4,
  NOTE_E4, NOTE_E4,
  NOTE_G4, NOTE_G4,
  NOTE_DB4, NOTE_F4, NOTE_AB4, NOTE_BB4, NOTE_C5
};
const int hijazLen = sizeof(hijazNotes) / sizeof(hijazNotes[0]);

int improvDurDivs[] = {2, 4, 4, 4};
const int improvDurLen = sizeof(improvDurDivs) / sizeof(improvDurDivs[0]);

int idxScale = 0;
int idxArab  = 0;
unsigned long lastNoteTime = 0;
int currentNoteDur = 0;   // мс

void setup() {
  Serial.begin(9600);

  ledcSetup(BUZZER_CH, 2000, 10);
  ledcAttachPin(BUZZER_PIN, BUZZER_CH);

  pinMode(MODE_BTN_PIN,  INPUT_PULLUP);
  pinMode(POWER_BTN_PIN, INPUT_PULLUP);

  randomSeed(analogRead(0));
}

// кнопка режимів: циклічно 1->2->3->1...
void handleModeButton() {
  int reading = digitalRead(MODE_BTN_PIN);

  if (lastModeBtnState == HIGH && reading == LOW) {
    mode++;
    if (mode > 3) mode = 1;   // тільки 1..3, 0 тепер значить "режим є, але звук вимкнений"
    Serial.print("Mode: ");
    Serial.println(mode);

    idxScale = 0;
    idxArab  = 0;
    lastNoteTime   = millis();
    currentNoteDur = 0;
  }

  lastModeBtnState = reading;
}

// кнопка живлення звуку: toggle
void handlePowerButton() {
  int reading = digitalRead(POWER_BTN_PIN);

  if (lastPowerBtnState == HIGH && reading == LOW) {
    soundEnabled = !soundEnabled;
    Serial.print("Sound: ");
    Serial.println(soundEnabled ? "ON" : "OFF");

    if (!soundEnabled) {
      ledcWriteTone(BUZZER_CH, 0);  // миттєво глушимо
    } else {
      lastNoteTime   = millis();
      currentNoteDur = 0;
    }
  }

  lastPowerBtnState = reading;
}

// Режим 1
void stepScale() {
  unsigned long now = millis();
  int length = sizeof(melodyScale) / sizeof(melodyScale[0]);

  if (idxScale >= length) {
    idxScale = 0;
    ledcWriteTone(BUZZER_CH, 0);
    lastNoteTime   = now;
    currentNoteDur = 500;
    return;
  }

  if (now - lastNoteTime >= (unsigned long)currentNoteDur) {
    int note     = melodyScale[idxScale];
    int noteType = durScale[idxScale];

    int noteDuration = BASE_TEMPO_MS / noteType;

    if (note > 0 && soundEnabled) {
      ledcWriteTone(BUZZER_CH, note);
    } else {
      ledcWriteTone(BUZZER_CH, 0);
    }

    currentNoteDur = noteDuration;
    lastNoteTime   = now;
    idxScale++;
  }
}

// Режим 2
void stepArab() {
  unsigned long now = millis();
  int length = sizeof(melodyArab) / sizeof(melodyArab[0]);

  if (idxArab >= length) {
    idxArab = 0;
    ledcWriteTone(BUZZER_CH, 0);
    lastNoteTime   = now;
    currentNoteDur = 700;
    return;
  }

  if (now - lastNoteTime >= (unsigned long)currentNoteDur) {
    int note     = melodyArab[idxArab];
    int noteType = durArab[idxArab];

    int noteDuration = BASE_TEMPO_MS / noteType;

    if (note > 0 && soundEnabled) {
      ledcWriteTone(BUZZER_CH, note);
    } else {
      ledcWriteTone(BUZZER_CH, 0);
    }

    currentNoteDur = noteDuration;
    lastNoteTime   = now;
    idxArab++;
  }
}

// Режим 3: фразова імпровізація в Hijaz
void stepImprov() {
  // фраза: кілька нот поспіль у вибраному напрямку (вгору/вниз), потім пауза
  static int  curIndex      = 0;     // поточна позиція в ладі
  static int  notesInPhrase = 0;     // скільки нот вже зіграно у фразі
  static int  phraseLength  = 0;     // довжина поточної фрази (4–6)
  static int  direction     = 1;     // 1 = вгору, -1 = вниз
  static bool inPause       = false; // зараз в паузі між фразами

  unsigned long now = millis();

  if (now - lastNoteTime < (unsigned long)currentNoteDur) {
    return;
  }

  if (inPause) {
    // пауза між фразами закінчилась -> починаємо нову фразу
    inPause       = false;
    notesInPhrase = 0;
    // новий напрямок (вгору чи вниз)
    direction = random(0, 2) == 0 ? 1 : -1;
    // нова довжина фрази 4–6 нот
    phraseLength = 4 + random(0, 3); // 4,5,6
    // стартова нота: частіше C/E/G
    curIndex = random(0, hijazLen);  // можна залишити як є, бо масив уже «зважений»
  }

  // якщо фраза ще не закінчилась
  if (notesInPhrase < phraseLength) {
    // рухаємося на 0,1 або 2 ступені в обраний бік
    int step = random(0, 3) * direction; // 0, +/-1, +/-2
    curIndex += step;
    if (curIndex < 0) curIndex = 0;
    if (curIndex >= hijazLen) curIndex = hijazLen - 1;

    int note = hijazNotes[curIndex];

    // тривалість ноти: половина чи чверть (іноді половина довша)
    int div = (random(0, 3) == 0) ? 2 : 4;  // 1/2 або 1/4
    int noteDuration = BASE_TEMPO_MS / div;

    if (note > 0 && soundEnabled) {
      ledcWriteTone(BUZZER_CH, note);
    } else {
      ledcWriteTone(BUZZER_CH, 0);
    }

    currentNoteDur = noteDuration;
    lastNoteTime   = now;
    notesInPhrase++;

    // іноді спеціально закінчуємо фразу на "опорі" (C/E/G)
    if (notesInPhrase == phraseLength) {
      int resolveChance = random(0, 3); // ~1/3
      if (resolveChance == 0) {
        // форсуємо тоніку
        ledcWriteTone(BUZZER_CH, NOTE_C4);
        currentNoteDur = BASE_TEMPO_MS / 2; // довша нота
        lastNoteTime   = now;
      }
    }

  } else {
    // фраза закінчилась -> пауза
    ledcWriteTone(BUZZER_CH, 0);
    inPause       = true;
    currentNoteDur = 500 + random(0, 400); // 0.5–0.9 c паузи
    lastNoteTime   = now;
  }
}

void loop() {
  handleModeButton();
  handlePowerButton();

  if (!soundEnabled) {
    ledcWriteTone(BUZZER_CH, 0);  // повна тиша незалежно від режиму
    return;
  }

  if (mode == 1) {
    stepScale();
  } else if (mode == 2) {
    stepArab();
  } else if (mode == 3) {
    stepImprov();
  }
}
