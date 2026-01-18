#include <Arduino.h>
#include "pitches.h"

const int BUZZER_PIN = 23;
const int BUZZER_CH  = 0;
const int BUTTON_PIN = 19;

// 0 = off, 1 = scale (Hijaz), 2 = arabic pattern
int mode = 0;
int lastButtonState = HIGH;

// --- Темп ---
const int BASE_TEMPO_MS = 1800;   // базова довжина цілої ноти (повільніше, ніж 1000)[web:1063]

// --- Hijaz / Phrygian dominant на C: C, Db, E, F, G, Ab, Bb ---[web:1073][web:1079]

// Режим 1: гамма вгору-вниз
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

// Режим 2: розмірений «арабський» патерн із різними довжинами
int melodyArab[] = {
  NOTE_C4,  NOTE_DB4, NOTE_E4,  NOTE_DB4,
  NOTE_C4,  NOTE_G4,  NOTE_F4,  NOTE_E4,
  NOTE_DB4, NOTE_C4,  NOTE_BB4, NOTE_AB4,
  NOTE_G4,  NOTE_F4,  NOTE_E4,  NOTE_DB4,
  NOTE_C4
};
// 2 = половина, 4 = чверть, 8 = восьма. Більше значення -> коротша нота.
int durArab[] = {
  4, 8, 8, 4,
  2, 8, 8, 4,
  4, 8, 8, 4,
  4, 4, 2
};

// Індекси та таймінг
int idxScale = 0;
int idxArab  = 0;
unsigned long lastNoteTime = 0;
int currentNoteDur = 0;   // у мс

void setup() {
  Serial.begin(9600);

  ledcSetup(BUZZER_CH, 2000, 10);
  ledcAttachPin(BUZZER_PIN, BUZZER_CH);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

// Обробка кнопки: фронт HIGH -> LOW
void handleButton() {
  int reading = digitalRead(BUTTON_PIN);

  if (lastButtonState == HIGH && reading == LOW) {
    mode = (mode + 1) % 3;   // 0 -> 1 -> 2 -> 0
    Serial.print("Mode: ");
    Serial.println(mode);

    // Скидаємо стан мелодій і глушимо звук
    idxScale = 0;
    idxArab  = 0;
    ledcWriteTone(BUZZER_CH, 0);
    lastNoteTime = millis();
    currentNoteDur = 0;
  }

  lastButtonState = reading;
}

// Один крок програвання Hijaz‑гами
void stepScale() {
  unsigned long now = millis();
  int length = sizeof(melodyScale) / sizeof(melodyScale[0]);

  // пауза між циклами
  if (idxScale >= length) {
    idxScale = 0;
    ledcWriteTone(BUZZER_CH, 0);
    lastNoteTime = now;
    currentNoteDur = 500;  // пауза 0.5 с між повтореннями
    return;
  }

  if (now - lastNoteTime >= (unsigned long)currentNoteDur) {
    int note     = melodyScale[idxScale];
    int noteType = durScale[idxScale];           // 4, 8 тощо

    int noteDuration = BASE_TEMPO_MS / noteType; // повільніший темп[web:1063]

    if (note > 0) {
      ledcWriteTone(BUZZER_CH, note);
    } else {
      ledcWriteTone(BUZZER_CH, 0);
    }

    currentNoteDur = noteDuration;
    lastNoteTime   = now;
    idxScale++;
  }
}

// Один крок «арабського» патерну
void stepArab() {
  unsigned long now = millis();
  int length = sizeof(melodyArab) / sizeof(melodyArab[0]);

  if (idxArab >= length) {
    idxArab = 0;
    ledcWriteTone(BUZZER_CH, 0);
    lastNoteTime = now;
    currentNoteDur = 700;  // трохи довша пауза між фразами
    return;
  }

  if (now - lastNoteTime >= (unsigned long)currentNoteDur) {
    int note     = melodyArab[idxArab];
    int noteType = durArab[idxArab];

    int noteDuration = BASE_TEMPO_MS / noteType;

    if (note > 0) {
      ledcWriteTone(BUZZER_CH, note);
    } else {
      ledcWriteTone(BUZZER_CH, 0);
    }

    currentNoteDur = noteDuration;
    lastNoteTime   = now;
    idxArab++;
  }
}

void loop() {
  handleButton();

  if (mode == 0) {
    ledcWriteTone(BUZZER_CH, 0);   // тиша

  } else if (mode == 1) {
    stepScale();

  } else if (mode == 2) {
    stepArab();
  }
}
