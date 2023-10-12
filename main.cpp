#include <EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>

#define COMMON_ANODE

#ifdef COMMON_ANODE
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif

class AccessControlSystem {
public:
  AccessControlSystem() : mfrc522(SS_PIN, RST_PIN) {}

  void begin() {
    pinMode(redLed, OUTPUT);
    pinMode(greenLed, OUTPUT);
    pinMode(blueLed, OUTPUT);
    pinMode(wipeB, INPUT_PULLUP);
    pinMode(relay, OUTPUT);
    digitalWrite(relay, HIGH);
    digitalWrite(redLed, LED_OFF);
    digitalWrite(greenLed, LED_OFF);
    digitalWrite(blueLed, LED_OFF);
    Serial.begin(9600);
    SPI.begin();
    mfrc522.PCD_Init();

    if (EEPROM.read(1) != 143) {
      Serial.println(F("No Master Card Defined"));
      Serial.println(F("Scan A PICC to Define as Master Card"));
      defineMasterCard();
    }

    Serial.println(F("-------------------"));
    Serial.println(F("Master Card's UID"));
    for (uint8_t i = 0; i < 4; i++) {
      masterCard[i] = EEPROM.read(2 + i);
      Serial.print(masterCard[i], HEX);
    }
    Serial.println("");
    Serial.println(F("-------------------"));
    Serial.println(F("Everything is ready"));
    Serial.println(F("Waiting PICCs to be scanned"));
  }

  void run() {
    do {
      successRead = getID();
      if (digitalRead(wipeB) == LOW) {
        wipeEEPROM();
      }
      if (programMode) {
        cycleLeds();
      } else {
        normalModeOn();
      }
    } while (!successRead);

    if (programMode) {
      if (isMaster(readCard)) {
        Serial.println(F("Master Card Scanned"));
        Serial.println(F("Exiting Program Mode"));
        Serial.println(F("-----------------------------"));
        programMode = false;
        return;
      } else {
        if (findID(readCard)) {
          Serial.println(F("I know this PICC, removing..."));
          deleteID(readCard);
          Serial.println("-----------------------------");
          Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
        } else {
          Serial.println(F("I do not know this PICC, adding..."));
          writeID(readCard);
          Serial.println(F("-----------------------------"));
          Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
        }
      }
    } else {
      if (isMaster(readCard)) {
        programMode = true;
        Serial.println(F("Hello Master - Entered Program Mode"));
        uint8_t count = EEPROM.read(0);
        Serial.print(F("I have "));
        Serial.print(count);
        Serial.print(F(" record(s) on EEPROM"));
        Serial.println("");
        Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
        Serial.println(F("Scan Master Card again to Exit Program Mode"));
        Serial.println(F("-----------------------------"));
      } else {
        if (findID(readCard)) {
          Serial.println(F("Welcome, You shall pass"));
          granted();
        } else {
          Serial.println(F("You shall not pass"));
          denied();
        }
      }
    }
  }

private:
  MFRC522 mfrc522;

  uint8_t successRead;
  byte storedCard[4];
  byte readCard[4];
  byte masterCard[4];

  void defineMasterCard() {
    do {
      successRead = getID();
      digitalWrite(blueLed, LED_ON);
      delay(200);
      digitalWrite(blueLed, LED_OFF);
      delay(200);
    } while (!successRead);
    for (uint8_t j = 0; j < 4; j++) {
      EEPROM.write(2 + j, readCard[j]);
    }
    EEPROM.write(1, 143);
    Serial.println(F("Master Card Defined"));
  }

  void wipeEEPROM() {
    digitalWrite(redLed, LED_ON);
    Serial.println(F("Wipe Button Pressed"));
    Serial.println(F("You have 10 seconds to Cancel"));
    bool buttonState = monitorWipeButton(10000);
    if (buttonState == true && digitalRead(wipeB) == LOW) {
      Serial.println(F("Starting Wiping EEPROM"));
      for (uint16_t x = 0; x < EEPROM.length(); x = x + 1) {
        if (EEPROM.read(x) == 0) {
          // do nothing, already clear, go to the next address
        } else {
          EEPROM.write(x, 0);
        }
      }
      Serial.println(F("EEPROM Successfully Wiped"));
      digitalWrite(redLed, LED_OFF);
      delay(200);
      digitalWrite(redLed, LED_ON);
      delay(200);
      digitalWrite(redLed, LED_OFF);
      delay(200);
      digitalWrite(redLed, LED_ON);
      delay(200);
      digitalWrite(redLed, LED_OFF);
    } else {
      Serial.println(F("Wiping Cancelled"));
      digitalWrite(redLed, LED_OFF);
    }
  }

  void cycleLeds() {
    digitalWrite(redLed, LED_OFF);
    digitalWrite(greenLed, LED_ON);
    digitalWrite(blueLed, LED_OFF);
    delay(200);
    digitalWrite(redLed, LED_OFF);
    digitalWrite(greenLed, LED_OFF);
    digitalWrite(blueLed, LED_ON);
    delay(200);
    digitalWrite(redLed, LED_ON);
    digitalWrite(greenLed, LED_OFF);
    digitalWrite(blueLed, LED_OFF);
    delay(200);
  }

  void normalModeOn() {
    digitalWrite(blueLed, LED_ON);
    digitalWrite(redLed, LED_OFF);
    digitalWrite(greenLed, LED_OFF);
    digitalWrite(relay, HIGH);
  }

  uint8_t getID() {
    if (!mfrc522.PICC_IsNewCardPresent()) {
      return 0;
    }
    if (!mfrc522.PICC_ReadCardSerial()) {
      return 0;
    }
    Serial.println(F("Scanned PICC's UID:"));
    for (uint8_t i = 0; i < 4; i++) {
      readCard[i] = mfrc522.uid.uidByte[i];
      Serial.print(readCard[i], HEX);
    }
    Serial.println("");
    mfrc522.PICC_HaltA();
    return 1;
  }

  void granted() {
    digitalWrite(blueLed, LED_OFF);
    digitalWrite(redLed, LED_OFF);
    digitalWrite(greenLed, LED_ON);
    if (relay == LOW) {
      digitalWrite(relay, HIGH);
    } else {
      digitalWrite(relay, LOW);
    }
    delay(1000);
  }

  void denied() {
    digitalWrite(greenLed, LED_OFF);
    digitalWrite(blueLed, LED_OFF);
    digitalWrite(redLed, LED_ON);
    digitalWrite(relay, HIGH);
    delay(1000);
  }

  bool checkTwo(byte a[], byte b[]) {
    if (a[0] != 0)
      match = true;
    for (uint8_t k = 0; k < 4; k++) {
      if (a[k] != b[k])
        match = false;
    }
    if (match) {
      return true;
    } else {
      return false;
    }
  }

  uint8_t findIDSLOT(byte find[]) {
    uint8_t count = EEPROM.read(0);
    for (uint8_t i = 1; i <= count; i++) {
      readID(i);
      if (checkTwo(find, storedCard)) {
        return i;
        break;
      }
    }
  }

  bool findID(byte find[]) {
    uint8_t count = EEPROM.read(0);
    for (uint8_t i = 1; i <= count; i++) {
      readID(i);
      if (checkTwo(find, storedCard)) {
        return true;
        break;
      }
    }
    return false;
  }

  void readID(uint8_t number) {
    uint8_t start = (number * 4) + 2;
    for (uint8_t i = 0; i < 4; i++) {
      storedCard[i] = EEPROM.read(start + i);
    }
  }

  void writeID(byte a[]) {
    if (!findID(a)) {
      uint8_t num = EEPROM.read(0);
      uint8_t start = (num * 4) + 6;
      num++;
      EEPROM.write(0, num);
      for (uint8_t j = 0; j < 4; j++) {
        EEPROM.write(start + j, a[j]);
      }
      successWrite();
      Serial.println(F("Succesfully added ID record to EEPROM"));
    } else {
      failedWrite();
      Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
    }
  }

  void deleteID(byte a[]) {
    if (!findID(a)) {
      failedWrite();
      Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
    } else {
      uint8_t num = EEPROM.read(0);
      uint8_t slot;
      uint8_t start;
      uint8_t looping;
      uint8_t j;
      uint8_t count = EEPROM.read(0);
      slot = findIDSLOT(a);
      start = (slot * 4) + 2;
      looping = ((num - slot) * 4);
      num--;
      EEPROM.write(0, num);
      for (j = 0; j < looping; j++) {
        EEPROM.write(start + j, EEPROM.read(start + 4 + j));
      }
      for (uint8_t k = 0; k < 4; k++) {
        EEPROM.write(start + j + k, 0);
      }
      successDelete();
      Serial.println(F("Succesfully removed ID record from EEPROM"));
    }
  }

  void defineMasterCard() {
    do {
      successRead = getID();
      digitalWrite(blueLed, LED_ON);
      delay(200);
      digitalWrite(blueLed, LED_OFF);
      delay(200);
    } while (!successRead);
    for (uint8_t j = 0; j < 4; j++) {
      EEPROM.write(2 + j, readCard[j]);
    }
    EEPROM.write(1, 143);
    Serial.println(F("Master Card Defined"));
  }

  void successWrite() {
    digitalWrite(blueLed, LED_OFF);
    digitalWrite(redLed, LED_OFF);
    digitalWrite(greenLed, LED_OFF);
    delay(200);
    digitalWrite(greenLed, LED_ON);
    delay(200);
    digitalWrite(greenLed, LED_OFF);
    delay(200);
    digitalWrite(greenLed, LED_ON);
    delay(200);
    digitalWrite(greenLed, LED_OFF);
    delay(200);
    digitalWrite(greenLed, LED_ON);
    delay(200);
  }

  void failedWrite() {
    digitalWrite(blueLed, LED_OFF);
    digitalWrite(redLed, LED_OFF);
    digitalWrite(greenLed, LED_OFF);
    delay(200);
    digitalWrite(redLed, LED_ON);
    delay(200);
    digitalWrite(redLed, LED_OFF);
    delay(200);
    digitalWrite(redLed, LED_ON);
    delay(200);
    digitalWrite(redLed, LED_OFF);
    delay(200);
    digitalWrite(redLed, LED_ON);
    delay(200);
  }

  bool isMaster(byte test[]) {
    if (checkTwo(test, masterCard))
      return true;
    else
      return false;
  }

  bool monitorWipeButton(uint32_t interval) {
    uint32_t now = (uint32_t)millis();
    while ((uint32_t)millis() - now < interval) {
      if (((uint32_t)millis() % 500) == 0) {
        if (digitalRead(wipeB) != LOW)
          return false;
      }
    }
    return true;
  }

  void successDelete() {
    digitalWrite(blueLed, LED_OFF);
    digitalWrite(redLed, LED_OFF);
    digitalWrite(greenLed, LED_OFF);
    delay(200);
    digitalWrite(blueLed, LED_ON);
    delay(200);
    digitalWrite(blueLed, LED_OFF);
    delay(200);
    digitalWrite(blueLed, LED_ON);
    delay(200);
    digitalWrite(blueLed, LED_OFF);
    delay(200);
    digitalWrite(blueLed, LED_ON);
    delay(200);
  }

  void granted() {
    digitalWrite(blueLed, LED_OFF);
    digitalWrite(redLed, LED_OFF);
    digitalWrite(greenLed, LED_ON);
    if (relay == LOW) {
      digitalWrite(relay, HIGH);
    } else {
      digitalWrite(relay, LOW);
    }
    delay(1000);
  }

  void denied() {
    digitalWrite(greenLed, LED_OFF);
    digitalWrite(blueLed, LED_OFF);
    digitalWrite(redLed, LED_ON);
    digitalWrite(relay, HIGH);
    delay(1000);
  }

  uint8_t getID() {
    if (!mfrc522.PICC_IsNewCardPresent()) {
      return 0;
    }
    if (!mfrc522.PICC_ReadCardSerial()) {
      return 0;
    }
    Serial.println(F("Scanned PICC's UID:"));
    for (uint8_t i = 0; i < 4; i++) {
      readCard[i] = mfrc522.uid.uidByte[i];
      Serial.print(readCard[i], HEX);
    }
    Serial.println("");
    mfrc522.PICC_HaltA();
    return 1;
  }

  void ShowReaderDetails() {
    byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    Serial.print(F("MFRC522 Software Version: 0x"));
    Serial.print(v, HEX);
    if (v == 0x91)
      Serial.print(F(" = v1.0"));
    else if (v == 0x92)
      Serial.print(F(" = v2.0"));
    else
      Serial.print(F(" (unknown),probably a chinese clone?"));
    Serial.println("");
    if ((v == 0x00) || (v == 0xFF)) {
      Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
      Serial.println(F("SYSTEM HALTED: Check connections."));
      digitalWrite(greenLed, LED_OFF);
      digitalWrite(blueLed, LED_OFF);
      digitalWrite(redLed, LED_ON);
      while (true);
    }
  }

  void cycleLeds() {
    digitalWrite(redLed, LED_OFF);
    digitalWrite(greenLed, LED_ON);
    digitalWrite(blueLed, LED_OFF);
    delay(200);
    digitalWrite(redLed, LED_OFF);
    digitalWrite(greenLed, LED_OFF);
    digitalWrite(blueLed, LED_ON);
    delay(200);
    digitalWrite(redLed, LED_ON);
    digitalWrite(greenLed, LED_OFF);
    digitalWrite(blueLed, LED_OFF);
    delay(200);
  }

  void normalModeOn() {
    digitalWrite(blueLed, LED_ON);
    digitalWrite(redLed, LED_OFF);
    digitalWrite(greenLed, LED_OFF);
    digitalWrite(relay, HIGH);
  }

  void loop() {
    do {
      successRead = getID();
      if (digitalRead(wipeB) == LOW) {
        digitalWrite(redLed, LED_ON);
        digitalWrite(greenLed, LED_OFF);
        digitalWrite(blueLed, LED_OFF);
        Serial.println(F("Wipe Button Pressed"));
        Serial.println(F("Master Card will be Erased! in 10 seconds"));
        bool buttonState = monitorWipeButton(10000);
        if (buttonState == true && digitalRead(wipeB) == LOW) {
          EEPROM.write(1, 0);
          Serial.println(F("Master Card Erased from device"));
          Serial.println(F("Please reset to re-program Master Card"));
          while (1);
        }
        Serial.println(F("Master Card Erase Cancelled"));
      }
      if (programMode) {
        cycleLeds();
      } else {
        normalModeOn();
      }
    } while (!successRead);
    if (programMode) {
      if (isMaster(readCard)) {
        Serial.println(F("Master Card Scanned"));
        Serial.println(F("Exiting Program Mode"));
        Serial.println(F("-----------------------------"));
        programMode = false;
        return;
      } else {
        if (findID(readCard)) {
          Serial.println(F("I know this PICC, removing..."));
          deleteID(readCard);
          Serial.println("-----------------------------");
          Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
        } else {
          Serial.println(F("I do not know this PICC, adding..."));
          writeID(readCard);
          Serial.println(F("-----------------------------"));
          Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
        }
      }
    } else {
      if (isMaster(readCard)) {
        programMode = true;
        Serial.println(F("Hello Master - Entered Program Mode"));
        uint8_t count = EEPROM.read(0);
        Serial.print(F("I have "));
        Serial.print(count);
        Serial.print(F(" record(s) on EEPROM"));
        Serial.println("");
        Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
        Serial.println(F("Scan Master Card again to Exit Program Mode"));
        Serial.println(F("-----------------------------"));
      } else {
        if (findID(readCard)) {
          Serial.println(F("Welcome, You shall pass"));
          granted();
        } else {
          Serial.println(F("You shall not pass"));
          denied();
        }
      }
    }
  }

  void setup() {
    pinMode(redLed, OUTPUT);
    pinMode(greenLed, OUTPUT);
    pinMode(blueLed, OUTPUT);
    pinMode(wipeB, INPUT_PULLUP);
    pinMode(relay, OUTPUT);
    digitalWrite(relay, HIGH);
    digitalWrite(redLed, LED_OFF);
    digitalWrite(greenLed, LED_OFF);
    digitalWrite(blueLed, LED_OFF);
    Serial.begin(9600);
    SPI.begin();
    mfrc522.PCD_Init();
    Serial.println(F("Access Control Example v0.1"));
    ShowReaderDetails();
    if (digitalRead(wipeB) == LOW) {
      digitalWrite(redLed, LED_ON);
      Serial.println(F("Wipe Button Pressed"));
      Serial.println(F("You have 10 seconds to Cancel"));
      Serial.println(F("This will be remove all records and cannot be undone"));
      bool buttonState = monitorWipeButton(10000);
      if (buttonState == true && digitalRead(wipeB) == LOW) {
        Serial.println(F("Starting Wiping EEPROM"));
        for (uint16_t x = 0; x < EEPROM.length(); x = x + 1) {
          if (EEPROM.read(x) == 0) {
          } else {
            EEPROM.write(x, 0);
          }
        }
        Serial.println(F("EEPROM Successfully Wiped"));
        digitalWrite(redLed, LED_OFF);
        delay(200);
        digitalWrite(redLed, LED_ON);
        delay(200);
        digitalWrite(redLed, LED_OFF);
        delay(200);
        digitalWrite(redLed, LED_ON);
        delay(200);
        digitalWrite(redLed, LED_OFF);
      } else {
        Serial.println(F("Wiping Cancelled"));
        digitalWrite(redLed, LED_OFF);
      }
    }
    if (EEPROM.read(1) != 143) {
      Serial.println(F("No Master Card Defined"));
      Serial.println(F("Scan A PICC to Define as Master Card"));
      do {
        successRead = getID();
        digitalWrite(blueLed, LED_ON);
        delay(200);
        digitalWrite(blueLed, LED_OFF);
        delay(200);
      } while (!successRead);
      for (uint8_t j = 0; j < 4; j++) {
        EEPROM.write(2 + j, readCard[j]);
      }
      EEPROM.write(1, 143);
      Serial.println(F("Master Card Defined"));
    }
    Serial.println(F("-------------------"));
    Serial.println(F("Master Card's UID"));
    for (uint8_t i = 0; i < 4; i++) {
      masterCard[i] = EEPROM.read(2 + i);
      Serial.print(masterCard[i], HEX);
    }
    Serial.println("");
    Serial.println(F("-------------------"));
    Serial.println(F("Everything is ready"));
    Serial.println(F("Waiting PICCs to be scanned"));
    digitalWrite(blueLed, LED_OFF);
  }
};

AccessControlSystem accessControlSystem;

void setup() {
  accessControlSystem.setup();
}

void loop() {
  accessControlSystem.loop();
}
