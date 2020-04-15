/**
 * Arduino program for notifying slave I2C processes of RFID permissions
 * Uses MFRC522 with standard pinout
 * Uses LCD with I2C pinout
 * Each node registered at single bit I2C address
 * TODO: Can I fit an RGB LED?
 * 
 * Holds map of allowed users & admin for attached nodes
 * Sends I2C notification to slaves indicating which doors to unlock within node?
 */
#include <EEPROM.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522_I2C.h>
#include <LiquidCrystal_I2C.h>

// WIFI Connection Info
char ssid[] = "SuckItTrebek-2.4";     //  your network SSID (name)
char pass[] = "9253301622";  // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status

// For RFID Reader
#define RST_PIN D3
#define SS_PIN D4

#define SLAVE_ADDRESS 8

byte readCard[4];
char* myTags[100] = {};
int tagsCount = 0;
String tagID = "";
boolean successRead = false;
boolean correctTag = false;
boolean doorOpened = false;

// Create instances
MFRC522 mfrc522(0x28, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display


// Master  keys must be manually added
// Student keys automatically added
// On setup, fetch the list of master keys and allowed keys
// in Loop
// If a master key is scanned once, process normally
// If a master key is scanned twice w/o any other actions, enter add mode
// In add mode, student scans key to be added to spreadsheet. After add, allowed keys is refreshed
// Normal Mode:
// 1. Scan key, accessible drawers unlock and show green for 15 seconds. Inaccessible drawers stay red and locked
// 2. When drawer opens, indicator flashed red/green until drawer closed again. Key scanner inaccessible while drawers are open
// 3. When drawer closes, drawer re-locks, led returns to red
// When wifi down, cannot enter Add mode; Cannot update from spreadsheet

/**
 * Node Program:
 * 1. DB contains 2 collections - 1) associations of readers & tools, 2) association of tools and spreadsheets
 * 2. Need interface to configure DB linking spreadsheets 
 * 3. Receives requests from reader w/ query params of reader ID
 * 4. Queries spreadsheets for associated tools and returns card IDs
 * 
 * 
 * In EEPROM store:
 * 1. Config settings (tools owned by reader) [1 byte tool (unique ID to reader)]
 * 2. Admin card collection [4 byte hex card: 1 byte tool]
 * 3. User card collection [4 byte hex card: 1 byte tool] (do not need to include admin tools since they can open them )
 * 4. Last refesh time
 * 
 * On Boot:
 * 1. Download what tools belong to this reader (can send query param about how many locks it supports)
 * 2. Download admin cards mapped to what tool they can modify
 * 3. Download user cards mapped to what tool they can unlock
 * 4. Initialize slaves with what tools they are managing
 * 
 * On Loop:
 * 1. Every X min, refresh store (amt dependent on allowable latency/ downtime)
 * 2. 
 */
void setup() {
  // Initialize LCD
  lcd.begin(16,2);
  lcd.init();
  lcd.backlight(); // Turn on the backlight.

  // Initialize I2C connections
  Wire.begin(); 

  // Init MFRC522
  mfrc522.PCD_Init();
  showReaderDetails();

  // Start Wifi
  connectWifi();
}

void loop() {
    // If door is closed...
    if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
      return;
    }
    if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
      return;
    }
    tagID = "";
    // The MIFARE PICCs that we use have 4 byte UID
    for ( uint8_t i = 0; i < 4; i++) {  //
      readCard[i] = mfrc522.uid.uidByte[i];
      tagID.concat(String(mfrc522.uid.uidByte[i], HEX)); // Adds the 4 bytes in a single String variable
    }
    tagID.toUpperCase();
    mfrc522.PICC_HaltA(); // Stop reading
    correctTag = false;
    // Checks whether the scanned tag is the master tag
    if (tagID == myTags[0]) {
      lcd.clear();
      lcd.print("Program mode:");
      lcd.setCursor(0, 1);
      lcd.print("Add/Remove Tag");
      while (!successRead) {
        successRead = getID();
        if ( successRead == true) {
          for (int i = 0; i < 100; i++) {
            if (tagID == myTags[i]) {
              myTags[i] = "";
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("  Tag Removed!");
              printNormalModeMessage();
              return;\
            }
          }
          myTags[tagsCount] = strdup(tagID.c_str());
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("   Tag Added!");
          printNormalModeMessage();
          tagsCount++;
          return;
        }
      }
    }
    successRead = false;
    // Checks whether the scanned tag is authorized
    for (int i = 0; i < 100; i++) {
      if (tagID == myTags[i]) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Access Granted!");
        writeToSlave(SLAVE_ADDRESS, 0); // Unlock door
        printNormalModeMessage();
        correctTag = true;
      }
    }
    if (correctTag == false) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" Access Denied!");
      writeToSlave(SLAVE_ADDRESS, 1); // Lock (re-lock) door
      printNormalModeMessage();
    }
}

uint8_t getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  tagID = "";
  for ( uint8_t i = 0; i < 4; i++) {  // The MIFARE PICCs that we use have 4 byte UID
    readCard[i] = mfrc522.uid.uidByte[i];
    tagID.concat(String(mfrc522.uid.uidByte[i], HEX)); // Adds the 4 bytes in a single String variable
  }
  tagID.toUpperCase();
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

void writeToSlave(byte address, byte payload) {
  Wire.beginTransmission(address);
  Wire.write(payload);
  Wire.endTransmission();
}

void printNormalModeMessage() {
  delay(1500);
  lcd.clear();
  lcd.print("-Access Control-");
  lcd.setCursor(0, 1);
  lcd.print(" Scan Your Tag!");
}

void showReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  lcd.clear();
  lcd.print(F("MFRC522 Software Version: 0x"));
  lcd.print(v, HEX);
  if (v == 0x91)
    lcd.print(F(" = v1.0"));
  else if (v == 0x92)
    lcd.print(F(" = v2.0"));
  else
    lcd.print(F(" (unknown)"));
    delay(1000);
  lcd.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    lcd.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
    delay(100000);
  }
}

void connectWifi() {
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    lcd.setCursor(0, 0);
    lcd.println("Attempting to connect to WPA SSID: ");
    lcd.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // you're connected now, so print out the data:
  lcd.println("You're connected to the network");
  IPAddress ip = WiFi.localIP();
  lcd.print("IP Address: ");
  lcd.println(ip);
}
