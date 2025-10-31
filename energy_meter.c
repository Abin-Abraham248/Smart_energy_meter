#define BLYNK_TEMPLATE_ID "template id" 
#define BLYNK_TEMPLATE_NAME "Smart energy meter" 
#define BLYNK_AUTH_TOKEN "auth token" 

#include <WiFi.h> 
#include "time.h" 
#include <WiFiClient.h> 
#include <BlynkSimpleEsp32.h> 
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 
#include "ACS712.h" 
#include <ZMPT101B.h> 
#include <EEPROM.h> 
// Your WiFi credentials. 

char ssid[] = "qwerty"; 
char pass[] = "12345678"; 

BLYNK_CONNECTED() { 
Blynk.syncVirtual(V1); 
Blynk.syncVirtual(V2); 
Blynk.syncVirtual(V3); 
Blynk.syncVirtual(V4); 
} 

ACS712 ACS(34, 3.3, 4095, 10); 
ZMPT101B voltageSensor(35, 50.0); 
LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD I2C address and size 

float unit; 
int volt, current, power; 

#define EEPROM_SIZE 512 
#define UNIT_ADDRESS 0 // Address in EEPROM to store unit value 
#define RELAY_PIN 25 // Renamed to avoid conflict if 'RELAY' is used elsewhere 
#define OVERVOLTAGE_THRESHOLD 245 // Set your desired overvoltage threshold in Volts 

void setup() { 
pinMode(RELAY_PIN, OUTPUT); // Configure the relay pin as an output 
digitalWrite(RELAY_PIN, HIGH); // Assuming HIGH means OFF for your relay (normally closed) 
Serial.begin(115200); 
EEPROM.begin(EEPROM_SIZE); 
// Read the previous unit value from EEPROM 
unit = EEPROM.readFloat(UNIT_ADDRESS); 
if (isnan(unit)) { 
unit = 0.0;  // Initialize to 0 if EEPROM value is invalid 
} 
Serial.print("Previous Unit Value: "); 
Serial.println(unit); 
Serial.print("ACS712_LIB_VERSION: "); 
Serial.println(ACS712_LIB_VERSION); 
ACS.autoMidPoint(); 
Serial.print("MidPoint: "); 
Serial.println(ACS.getMidPoint()); 
Serial.print("Noise mV: "); 
Serial.println(ACS.getNoisemV()); 
  
voltageSensor.setSensitivity(500.0f); 
  
lcd.init(); 
lcd.backlight(); 
lcd.setCursor(0, 0); 
lcd.print("Energy Meter"); 
delay(1000); 
  
Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass); 
} 
void loop() { 
Blynk.run(); 
  
int noise = ACS.getNoisemV(); 
float average = 0; 
for (int i = 0; i < 100; i++) { 
average += ACS.mA_AC()-330; 
} 
float mA = (average / 100.0); 
(mA > 1) ? current = mA : current = 0; 
float voltage = voltageSensor.getRmsVoltage(); 
(voltage > 50) ? volt = voltage : volt = 0; 
  
// --- Overvoltage Protection Logic --- 
if (volt > OVERVOLTAGE_THRESHOLD) { 
digitalWrite(RELAY_PIN,LOW); // Turn OFF the load (assuming LOW 
means ON for your relay) 
lcd.clear(); 
lcd.setCursor(0, 0); 
lcd.print("OVERVOLTAGE!"); 
lcd.setCursor(0, 1); 
lcd.print("Load Disconnected"); 
Serial.println("OVERVOLTAGE DETECTED! Load Disconnected."); 
Blynk.virtualWrite(V5, 1); // Send an alert to Blynk (create V5 for 
overvoltage status) 
delay(2000); // Display message for a while 
return; // Skip the rest of the loop to keep the load off 
} else { 
digitalWrite(RELAY_PIN, HIGH); // Keep the load ON (assuming 
HIGH means OFF for your relay) 
Blynk.virtualWrite(V5, 0); // Clear overvoltage alert 
} 
  
// --- End Overvoltage Protection Logic --- 
  
float watt = voltage * (mA / 1000.0); 
power = watt; 
float interval_s = 0.5; // loop time in seconds 
float kWh = (voltage * current * interval_s) / 3600000.0; 
unit += kWh; 
// Update LCD 
lcd.clear(); 
lcd.setCursor(0, 0); 
lcd.print("V:"); 
lcd.print(volt); 
lcd.print("V C:"); 
lcd.print(current); 
lcd.print("mA"); 
lcd.setCursor(0, 1); 
lcd.print("P:"); 
lcd.print(power); 
lcd.print("W U:"); 
lcd.print(unit, 2); 
lcd.print("kWh"); 
// Print to Serial Monitor 
Serial.print("Voltage: "); 
Serial.print(volt); 
Serial.println(" V"); 
Serial.print("Current: "); 
Serial.print(current); 
Serial.println(" mA"); 
Serial.print("Power: "); 
Serial.print(power); 
Serial.println(" W"); 
Serial.print("Energy: "); 
Serial.print(unit, 4); 
Serial.println(" kWh"); 
delay(500); 
// Save the current unit value to EEPROM  
EEPROM.writeFloat(UNIT_ADDRESS, unit); 
EEPROM.commit();
  
Blynk.virtualWrite(V1, volt); 
Blynk.virtualWrite(V2, current); 
Blynk.virtualWrite(V3, power); 
Blynk.virtualWrite(V4, unit); 
} 
