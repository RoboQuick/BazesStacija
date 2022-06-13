/*----------------------- Libaries ------------------ */

#include <SPI.h>
#include <LoRa.h>
#include <SD.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>


/*------------------------- LCD ------------------ */
// LCD pins
const int rs = 33, en = 34, d4 = 35, d5 = 36, d6 = 37, d7 = 39;

// LCD object
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Variables that are displayed
String hour, minute, second, longitude, latitude, altitude, speed, battery_voltage;

/*-------------------------- LoRa --------------------- */

// LoRa pins
const int csPin = 24;
const int resetPin = 26;
const int irqPin = 25;

// Frquency for LoRa in Hz
const long freq = 4339E5; // 433.9 MHz

// Last received message time
unsigned long last_message_time = 0;

/* ----------------------- SD Card ------------------------ */

// File object
File csvFile;

// Name of created csv file
String file_name;

// CSV file header
// NAMES MUST BE IN THE SAME ORDER AS THEY ARE SENT FROM CANSAT
// EACH NAME MUST BE SEPERATED BY A COMMA(,) AND WITHOUT A SPACE BETWEEN
String header = "x,y,z";

/*------------------------ Timing ------------------------- */

unsigned long current_millis, previous_millis;
unsigned long lcd_update_interval = 500;

/*----------------------- Functions ------------------------- */

// Create a new csv file and write the header with names of variables
void create_sd_file()
{
  // Get last index of created csv file
  int current_index = 0;
  EEPROM.get(0, current_index);

  // THIS CODE IS JUST FOR TESTING!!!
  // PLEASE DON'T FORGET TO DELETE THIS!!!
  if (current_index == 255) {
    EEPROM.put(0, 0);
  }

  // Increase index
  current_index += 1;

  // Creates the file name
  file_name = "BaseStationLog" + (String)current_index;

  // If file with that index already exists, increment index until the index isn't taken
  while (SD.exists(file_name.c_str())) {
    current_index += 1;
    file_name = "BaseStationLog" + (String)current_index;
  }

  // Creates a new file
  File csvFile = SD.open(file_name.c_str(), FILE_WRITE);

  // If the file is available, write to it:
  if (csvFile) {
    csvFile.println(header);
    csvFile.close();
  }

  // Store the new index, that just got taken
  EEPROM.put(0, current_index);
}

// Function that writes given message to SD card csv file
void write_to_sd(String message)
{
  // Opens a file
  File csvFile = SD.open(file_name.c_str(), FILE_WRITE);

  // if the file is available, write to it:
  if (csvFile) {
    csvFile.println(message);
    csvFile.close();
  }
}

// Function that runs waits until data package has been received
void onReceive(int packetSize) {
  // If no package received stop function
  if (packetSize == 0) return;

  // String to save incoming package
  String incoming = "";

  // Read package data and append each charachter to string
  while (LoRa.available()) {
    // Turn on LED to see that data has been received
    digitalWrite(14, HIGH);
    incoming += (char)LoRa.read();
  }

  // At the end of the message append the signal strength and signal to noise ratio
  incoming += ",";
  incoming += LoRa.packetRssi();
  incoming += ",";
  incoming += LoRa.packetSnr();
  incoming += "\n";
  
  // Print the message to serial monitor
  Serial.println(incoming);
  int MyP = 0;
  int MyI = 0;
  String array[11];
  int index = 0;
  while (MyI != -1) {
    MyI = incoming.indexOf(",", MyP);
    if (MyI == -1){
      break;
    }
    String s = incoming.substring(MyP, MyI);
    MyP = MyI + 1;
    array[index] = s;
    index = index + 1;
  }
  latitude = array[3];
  longitude = array[4];
  altitude = array[5];
  speed = array[6];
  // Write data to sd card
  //write_to_sd(incoming);
  last_message_time = millis();
  // Turn off LED as data has been parsed
  digitalWrite(14, LOW);
}

// Function that updates the LCD with new data
void update_lcd() {
  // GPS
  lcd.setCursor(0, 0);
  lcd.print(latitude);
  lcd.setCursor(0, 1);
  lcd.print(longitude);

  // Altitude
  lcd.setCursor(11, 0);
  // lcd.print("Alt:");
  lcd.print(altitude);
  
  // Speed
  lcd.setCursor(11, 1);
  // lcd.print("Spd: ");
  lcd.print(speed);

  // Miscellaneous
  lcd.setCursor(0, 3);
  lcd.print("Last M:");
  lcd.print((millis() - last_message_time) / 1000);
  lcd.setCursor(10, 3);
  lcd.print("V:");
  lcd.print(battery_voltage);
}

/*------------- Setup --------------*/
void setup() {
  // Set pins to required mode
  pinMode(14, OUTPUT);

  // Begin serial commucication
  Serial.begin(115200);

  // Tries to start up LoRa module. If it doesn't start, goes into infinite loop
  LoRa.setPins(csPin, resetPin, irqPin);
  if (!LoRa.begin(freq)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // LoRa settings
  // These must match CanSat's settings!!!
  LoRa.setTxPower(20);
  LoRa.setSpreadingFactor(10);
  LoRa.setSignalBandwidth(125E3);
  // With this setting, we won't see any corrupted messages
  // This can be both a good and a bad thing
  // Even if one charachter is corrupted, the entire message is thrown out
  LoRa.enableCrc();

  /*
    // See if the SD card is present and initialize it
    while (!SD.begin(BUILTIN_SDCARD)) {
    delay(1000);
    }
    // Create new csv file to store received data
    // create_sd_file();
  */

  // Begin LCD display
  lcd.begin(16, 4);
  lcd.clear();
  lcd.setCursor(0, 0);
}

/*------------- Loop --------------*/
void loop() {
  // Check if new package has been received
  onReceive(LoRa.parsePacket());

  // Get current millis
  current_millis = millis();

  // If interval time has passed, update displayed data on LCD
  if (current_millis - previous_millis >= lcd_update_interval) {
    // Update displayed data
    update_lcd();

    // Update last time this update has run
    previous_millis = current_millis;
  }
}
