#include <Arduino.h>

const int BUZZER_PIN = 23;
const int BUZZER_CH  = 0;
const int BUTTON_PIN = 19;   // твоя кнопка

void setup() {
  Serial.begin(9600);                     // тільки для перевірки
  pinMode(BUTTON_PIN, INPUT_PULLUP);      // кнопка до GND[web:772][web:636]

  ledcSetup(BUZZER_CH, 2000, 10);
  ledcAttachPin(BUZZER_PIN, BUZZER_CH);
}

void loop() {
  int state = digitalRead(BUTTON_PIN);
  Serial.println(state);                  // дивишся 0/1 при натисканні[web:772]

  if (state == LOW) {                     // натиснута
    ledcWriteTone(BUZZER_CH, 2000);       // пищить[web:663][web:775]
  } else {                                // відпущена
    ledcWriteTone(BUZZER_CH, 0);          // тиша[web:663]
  }

  delay(50);
}
