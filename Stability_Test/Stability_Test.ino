#include <WiFiNINA.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <SerLCD.h> 
#include <Arduino_LSM6DS3.h>
#include "arduino_secrets.h" 

SerLCD lcd; // Initialize the library with default I2C address 0x72

//sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the Wifi radio's status

#define SECRET_CH_ID 925987     //ThingSpeak Channel number
#define SECRET_WRITE_APIKEY "CTXHQ4146KMFW5LS"   //ThingSpeak API key
unsigned long myChannelNumber = SECRET_CH_ID;
const char* myWriteAPIKey = SECRET_WRITE_APIKEY;

WiFiClient  client;
#define Abutt 4 //"A" Button

//variables for tracking test data
float average=0;
int Time=0;
int i=0;
float mx=0;

void setup() {
  Serial.begin(115200);
  
  //lcd setup-------------
  Wire.begin();
  lcd.begin(Wire); //Set up the LCD for I2C communication
  lcd.setBacklight(100, 100, 100); //Set backlight to bright white
  lcd.setContrast(5); //Set contrast. Lower to 0 for higher contrast.
  lcd.clear(); //Clear the display - this moves the cursor to home position as well
//--------

  delay(2500);
  lcd.print("Attempting Wifi");
  lcd.setCursor(0, 1);
  lcd.print("connection...");
   
//----WiFi and Thingspeak
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    lcd.clear();
    lcd.print("Connection Failed!");
    while (true);
  }

  //firmware check and possible prompt for update
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
  
  Serial.print("You're connected to the network");

  printCurrentNet();
  printWifiData();

  IMU.begin();//initialize the IMU
  delay(3000);

  if (!IMU.begin()) {//if the IMU is offline for any reason, let the user know, and they will need to reset
      lcd.clear();
      lcd.print("Accelerometer");
      lcd.setCursor(0,1);
      lcd.print("Failed");
    while (1);
  }

  //prompt test start on LCD screen
  lcd.clear();
  lcd.print("Push button for");
  lcd.setCursor(0,1);
  lcd.print("Stability test!");

}

void loop() {
  float x, y, z, g; //acceleration variables
  float total=0; //total acc to calculate average later
  Time=0;
  i=0;
 if (IMU.accelerationAvailable()) {
    if (digitalRead(Abutt)==HIGH){ //if the user pushes the "A" button to start the test
      lcd.clear();
      lcd.print("Use your device");
      lcd.setCursor(0,1);

      //lcd count down to give user time to prepare their test start
      lcd.print("in 3...");
      delay (500);
       lcd.setCursor(0,1);
       lcd.print("in 2...");
       delay (500);
       lcd.setCursor(0,1);
       lcd.print("in 1...");
       delay (500);
       lcd.clear();
      lcd.print("GO!");
      delay (500);
      lcd.clear();
      
      while(digitalRead(Abutt)==LOW) {//until the A button is pushed again
        i++;
        Time=i*.5;//tracking time to calculate average acceleration
        IMU.readAcceleration(x, y, z);
        g=pow((pow(x+.2,2)+pow(y-.2,2)+pow(z+.98,2)),.5);//g=magnitude of acceleration in all directions
        if(g>mx){
          mx=g;//track max acceleration
        }
        total=total+g;
        average=total/i;
        Serial.println("Reading: "+String(g));
        Serial.println("Average: "+ String(average));
        Serial.println(String(i/2)+" seconds");
        Serial.println("");

        //display data to user during test
        lcd.setCursor(0, 0);
        lcd.print("Acceleration");
        lcd.setCursor(0, 1);
        lcd.print( "Reading: "+ String(g)+"g");
        
        delay(500);
      }
      Serial.println("Final Force: " + String(average));
      FinalStability();//When the A button IS pushed again, the test is over, run the final test function
    }
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

  lcd.clear();
  lcd.print("Connection");
  lcd.setCursor(0,1);
  lcd.print("Strength: "+ String(abs(rssi)));
}

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

void FinalStability() {
  int statusCode = 0;

  //display final measurements to user
  lcd.clear();
  lcd.print("Average: " + String(average)+ "g");
  lcd.setCursor(0,1);
  lcd.print("Maximum: " + String(mx)+ "g");
  delay(3000);
  if(Time<13){//this wait prevents breaching ThingSpeak's data send limit
      Serial.println("Please wait while data is sent");
      delay((13-Time)*1000);
    }
  statusCode = ThingSpeak.writeField(myChannelNumber, 1, average, myWriteAPIKey);

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
  lcd.clear();
  lcd.print("Push button for");
  lcd.setCursor(0,1);
  lcd.print("Stability test!");
    Serial.println("Ready to Test!");
}
