/******************************************************************************
Credit to SparkFun's force sensitive resistors for their example code for proper sensor implementation
  (https://www.sparkfun.com/products/9375)
Jim Lindblom @ SparkFun Electronics
April 28, 2016
******************************************************************************/
#include <SPI.h>
#include <WiFiNINA.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <SerLCD.h>
#include "arduino_secrets.h" //where we store our WiFi info

//I designed these bytes to depict an arrow and a dog on the LCD screen. 1=lit pixel, 0=unlit
byte marker[8] = {
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100
};
byte dog[8] = {
  0b10010,
  0b01111,
  0b01111,
  0b01100,
  0b01100,
  0b01100,
  0b10010,
  0b00000
};

const float VCC = 3.3 ; // Vout from Arduino Nano 33
const float R_DIV = 100000; // Reistor I used
 
SerLCD lcd; // Initialize the library with default I2C address 0x72

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the Wifi radio's status

#define SECRET_CH_ID 904398     // ThingSpeak Channel number
#define SECRET_WRITE_APIKEY "Y5HDMGPMTI8Z8ENL"   // ThingSpeak API Key
#define Abutt 4 //Abutton input pin
#define FSR_PIN  A7 // Pin connected to FSR/resistor divider

unsigned long myChannelNumber = SECRET_CH_ID;
const char* myWriteAPIKey = SECRET_WRITE_APIKEY;

WiFiClient  client;
float avg; //var to store average force over course of test
int statusCode = 0; //ThingSpeak.Write() statuscode 

void setup() {
  
//lcd setup
  Wire.begin();
  lcd.begin(Wire); //Set up the LCD for I2C communication
  lcd.setBacklight(100, 100,100); //Set backlight to bright white
  lcd.setContrast(5); //Set contrast. Lower to 0 for higher contrast.
  lcd.clear(); //Clear the display - this moves the cursor to home position as well
  
  //place our markers
  lcd.createChar(0, marker);
  lcd.createChar(1, dog);
  delay(2000);
  lcd.print("Attempting Wifi");
  lcd.setCursor(0, 1);
  lcd.print("connection...");
  pinMode(FSR_PIN, INPUT);

  Serial.begin(115200);

  ThingSpeak.begin(client);  // Initialize ThingSpeak
  
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    lcd.print("Connection Failed!");
    while (true);
  }

  //firmware check and possible prompt to update
  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to Wifi network:
  if (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }

  //connection info for troubleshooting
  printCurrentNet();
  printWifiData();

  delay(3000);

  //LCD prompt for users to test
  lcd.clear();
  lcd.print("Push Button ");
  lcd.setCursor(0, 1);
  lcd.print("to Test");
}


void loop() {
 int pos = 0;
 
 if (digitalRead(Abutt)==HIGH) // When the A button is pressed...
  {
    lcd.clear();
    lcd.print("Begin");
    lcd.setCursor(0, 1);
    lcd.print("Collecting data");
    delay(2000);
    float total=0;//track sum of force to calculate average later
    float mx=0;//current max force
    for (int i = 1; i <= 10; i++) {
      float push = analogRead(FSR_PIN)*.0285*4; //*.0285 for lbf, then/.25in^2 for psu
      if (push>mx){
        mx=push;
      }
      if(push>10){
        pos=15;
      }
      else{
        pos=push*2.6 + 1; //fit to 1->15 on screen
      }

      total=total+push;
      Serial.println("Reading: "+String(push));
      Serial.println("Average: "+ String(total/i));
      Serial.println(String(i*.7)+" seconds");
      Serial.println("");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Pressure:" + String(push)+"psi");
      lcd.setCursor(0, 1);
      lcd.print("0              5");
      lcd.setCursor(pos,1);//move cursor based on magnitude of current force
      lcd.writeChar(1);
      delay(700);
      //button break  
    }

    avg=total/10; //further convert to 1-10 pressure rating. 5 seems to be just noise
 
    lcd.clear();
    lcd.writeChar(0);
    lcd.setCursor(1,0);
    lcd.print("Average    Max");//data labels for user
    lcd.setCursor(15,0);
    lcd.writeChar(0);
    lcd.setCursor(0,1);
    lcd.print(String(avg) + "psi"); //print average test force
    lcd.setCursor(8,1);
    lcd.print(String(mx) + "psi");//print max test force

  //keep average value in target range
  if(avg>5){
    statusCode = ThingSpeak.writeField(myChannelNumber, 1, 5, myWriteAPIKey);
  }
  else{
    statusCode = ThingSpeak.writeField(myChannelNumber, 1, avg, myWriteAPIKey);
  }
   
  //thingspeak feedback for troubleshooting
  if(statusCode == 200){Serial.println("Channel was updated! OK!");
  }else if(statusCode == 404){
    Serial.println("Incorrect API key (or invalid ThingSpeak server address)");
  }else if(statusCode == -101){
    Serial.println("Value is out of range or string is too long (> 255 characters)");
  }else if(statusCode == -201){
    Serial.println("  Invalid field number specified");
  }else if(statusCode == -210){
    Serial.println("setField() was not called before writeFields()");
  }else if(statusCode == -301){
    Serial.println("Failed to connect to ThingSpeak");
  }else if(statusCode == -302){
    Serial.println("Unexpected failure during write to ThingSpeak");
  }else if(statusCode == -303){
    Serial.println("Unable to parse response");
  }else if(statusCode == -304 ){
    Serial.println("Timeout waiting for server to respond");
  }else if(statusCode == -401){
    Serial.println("Point was not inserted (most probable cause is the rate limit of once every 15 seconds)");
  }else{
    Serial.println("Unexpected error!"); } 
  
  delay(6000);//delay to prevent breaching the send rate to Thingspeak
  Serial.println("");
  lcd.clear();
  
  //reprint starting prompt to LCD screen
  lcd.print("Push Button ");
  lcd.setCursor(0, 1);
  lcd.print("to Test");
  } 
}

void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();

  //print signal strength to user to let them know if we are on WiFi or not
  lcd.clear();
  lcd.print("Connection");
  lcd.setCursor(0,1);
  lcd.print("Strength: "+ String(abs(rssi)));

}

//print mac address for troubleshooting
void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}
