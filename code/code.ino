#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include "RDM6300.h"
#include "LiquidCrystal_I2C.h"
#include "Int64String.h"

int static memory_1 = 0;
int static memory_2 = 10;
int static memory_3 = 20;
int static memory_4 = 30;
int static memory_5 = 40;

const int buttonCard1 = 3;
const int buttonCard2 = 4;
const int buttonCard3 = 5;
const int buttonCard4 = 6;
const int buttonCard5 = 7;
const int buttonCloneCard = 8;

const int led_ready = 10;
const int led_clone = 11;

int coil_pin = 9;
static uint32_t emu_card;

uint8_t hex_keys[16] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};

SoftwareSerial rdm_serial(2, 12);
RDM6300<SoftwareSerial> rdm(&rdm_serial);

LiquidCrystal_I2C lcd(0x27, 16, 2);

void readCard(int memoryLocation) {
  digitalWrite(led_ready, HIGH);

  uint64_t temp_card = EEPROMReadlong(memoryLocation);

  rdm.print_int64(temp_card); Serial.println();
  if (temp_card != 0000000000) {
    Serial.println("Loaded card");
    emu_card = generate_card(temp_card);

    lcd.setCursor(0, 0);
    lcd.print("Loaded CardID");
    lcd.setCursor(0, 1);
    lcd.print(int64String(temp_card, HEX, true));
  }
}
void cloneCard(int memoryLocation) {
  digitalWrite(led_clone, HIGH);
  Serial.println("Waiting to read: ");
  while (0000000000 != rdm.read()) {
    EEPROMWritelong(memoryLocation, rdm.read());
    Serial.println("Writed");

    uint64_t temp_card = EEPROMReadlong(memoryLocation);
    Serial.print("Saved: ");
    rdm.print_int64(temp_card); Serial.println();
    emu_card = generate_card(temp_card);
    digitalWrite(led_clone, LOW);

    lcd.setCursor(0, 0);
    lcd.print("Cloned CardID");
    lcd.setCursor(0, 1);
    lcd.print(int64String(temp_card, HEX, true));

    break;
  }
  digitalWrite(led_ready, HIGH);
}
void saveCard(int memoryLocation, uint64_t saved_card) {
  digitalWrite(led_clone, HIGH);
  Serial.println("Saving card");

  EEPROMWritelong(memoryLocation, saved_card);
  Serial.println("Writed card");

  uint64_t temp_card = EEPROMReadlong(memoryLocation);
  Serial.print("Saved: ");
  rdm.print_int64(temp_card); Serial.println();
  emu_card = generate_card(temp_card);
  digitalWrite(led_clone, LOW);

  lcd.setCursor(0, 0);
  lcd.print("Read CardID");
  lcd.setCursor(0, 1);
  lcd.print(int64String(temp_card, HEX, true));

  digitalWrite(led_ready, HIGH);
}
void EEPROMWritelong(int address, uint64_t value) {
  for (int i = address, j = 0; i < address + 10; i++, j++) {
    byte b = (value >> 36 - j * 4) & 0xFF;
    EEPROM.write(i, b);
  }
}
uint64_t EEPROMReadlong(int address) {
  uint64_t card = 0;
  for (int i = address, b = 0; i < address + 10; i++, b++) {
    uint64_t tmp = EEPROM.read(i);
    tmp = tmp << (36 - b * 4);
    card |= tmp;
  }
  return card;
}

void set_pin_manchester(int clock_half, int signal) {
  int man_encoded = clock_half ^ signal;
  if (man_encoded == 1) {
    digitalWrite(coil_pin, LOW);
  }
  else {
    digitalWrite(coil_pin, HIGH);
  }
}

uint32_t generate_card(uint64_t Hex_IDCard) {
  static uint32_t data_card[64];
  static uint32_t card_id[10];

  for (int i = 0; i < 10; i++) card_id[i] = (Hex_IDCard >> 36 - i * 4 ) & 0xF;

  for (int i = 0; i < 9; i++) data_card[i] = 1;

  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 4; j++) data_card[9 + i * 5 + j] = card_id[i] >> (3 - j) & 1;
    data_card[9 + i * 5 + 4] = ( data_card[9 + i * 5 + 0] + data_card[9 + i * 5 + 1] + data_card[9 + i * 5 + 2] + data_card[9 + i * 5 + 3]) % 2;
  }

  for (int i = 0; i < 4; i++) {
    int checksum = 0;
    for (int j = 0; j < 10; j++) checksum += data_card[9 + i + j * 5];
    data_card[i + 59] = checksum % 2;
  }
  data_card[63] = 0;

  return data_card;
}
void emulateCard(uint32_t *data) {
  for (int i = 0; i < 64; i++) {
    set_pin_manchester(0, data[i]);
    delayMicroseconds(255);

    set_pin_manchester(1, data[i]);
    delayMicroseconds(255);
  }
}

bool isPressed(int button) {
  if (digitalRead(button) == HIGH) {
    return false;
  }
  return true;
}
uint64_t manualEnterCard() {
  unsigned long startMillis = millis();
  unsigned long currentMillis;
  const unsigned long period = 2000;

  lcd.setCursor(0, 0);
  lcd.print("Enter CardID");
  lcd.setCursor(0, 1);
  lcd.print("_");
  lcd.setCursor(0, 1);
  uint64_t new_card = 0;
  uint64_t tmp_card[11];
  int key_pressed = 0;
  int card_id_place = 0;

  while (card_id_place != 10) {
    currentMillis = millis();

    if (currentMillis - startMillis >= period) {

      if (key_pressed == 0) {
        key_pressed = 16;
      }
      else {
        key_pressed--;
      }
      lcd.print(hex_keys[key_pressed], HEX);

      tmp_card[card_id_place] = hex_keys[key_pressed];
      card_id_place++;
      key_pressed = 0;
      startMillis = currentMillis;
      lcd.setCursor(card_id_place, 1);
      lcd.print("_");
      lcd.setCursor(card_id_place, 1);
    }
    else {
      if (isPressed(buttonCloneCard)) {
        startMillis = currentMillis;

        lcd.print(hex_keys[key_pressed], HEX);
        key_pressed++;
        if (key_pressed > 16) {
          key_pressed = 0;
        }
        lcd.setCursor(card_id_place, 1);
        delay(200);

        startMillis = currentMillis;
      }
    }
  }

  lcd.clear();
  lcd.noBlink();
  lcd.setCursor(0, 0);
  lcd.print("Entered");
  lcd.setCursor(0, 1);

  for (int i = 0, b = 0; i < 10; i++, b++) {
    uint64_t tmp = tmp_card[i];
    tmp = tmp << (36 - b * 4);
    new_card |= tmp;
  }

  return new_card;
}
void setup() {
  Serial.begin(57600);

  pinMode(buttonCard1, INPUT_PULLUP);
  pinMode(buttonCard2, INPUT_PULLUP);
  pinMode(buttonCard3, INPUT_PULLUP);
  pinMode(buttonCard4, INPUT_PULLUP);
  pinMode(buttonCard5, INPUT_PULLUP);
  pinMode(buttonCloneCard, INPUT_PULLUP);
  pinMode(coil_pin, OUTPUT);
  digitalWrite(coil_pin, LOW);

  pinMode(led_ready, OUTPUT);
  pinMode(led_clone, OUTPUT);

  lcd.begin();
  lcd.backlight();

  if (isPressed(buttonCard1)) {
    if (isPressed(buttonCloneCard)) {
      Serial.println("CLONE CARD 1");
      cloneCard(memory_1);
    }
    else {
      Serial.println("LOAD CARD 1");
      readCard(memory_1);
    }
  }
  else if (isPressed(buttonCard2)) {
    if (isPressed(buttonCloneCard)) {
      Serial.println("CLONE CARD 2");
      cloneCard(memory_2);
    }
    else {
      Serial.println("LOAD CARD 2");
      readCard(memory_2);
    }
  }
  else if (isPressed(buttonCard3)) {
    if (isPressed(buttonCloneCard)) {
      Serial.println("CLONE CARD 3");
      cloneCard(memory_3);
    }
    else {
      Serial.println("LOAD CARD 3");
      readCard(memory_3);
    }
  }
  else if (isPressed(buttonCard4)) {
    if (isPressed(buttonCloneCard)) {
      Serial.println("CLONE CARD 4");
      cloneCard(memory_4);
    }
    else {
      Serial.println("LOAD CARD 5");
      readCard(memory_4);
    }
  }
  else if (isPressed(buttonCard5)) {
    if (isPressed(buttonCloneCard)) {
      Serial.println("CLONE CARD 5");
      cloneCard(memory_5);
    }
    else {
      Serial.println("LOAD CARD 5");
      readCard(memory_5);
    }
  }
  else {
    uint64_t new_card = manualEnterCard();

    Serial.print("Entered: ");
    rdm.print_int64(new_card); Serial.println();
    emu_card = generate_card(new_card);
    lcd.print(int64String(new_card, HEX, true));

    if (isPressed(buttonCard1)) {
      Serial.println("LOAD CARD 1");
      saveCard(memory_1, new_card);
    }
    else if (isPressed(buttonCard2)) {
      Serial.println("LOAD CARD 2");
      saveCard(memory_2, new_card);
    }
    else if (isPressed(buttonCard3)) {
      Serial.println("LOAD CARD 3");
      saveCard(memory_3, new_card);
    }
    else if (isPressed(buttonCard4)) {
      Serial.println("LOAD CARD 4");
      saveCard(memory_4, new_card);
    }
    else if (isPressed(buttonCard5)) {
      Serial.println("LOAD CARD 5");
      saveCard(memory_5, new_card);
    }
  }
}
  void loop() {
    emulateCard(emu_card);
  }
