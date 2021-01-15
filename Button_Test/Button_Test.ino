#include <WiFiNINA.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <SerLCD.h> 
SerLCD lcd; // Initialize the library with default I2C address 0x72

/*This Arduino program creates what is essentially an IoT stop watch.
K-12 engineering students use it in their hands on design challenges whenever the success of a test
can be measured by how quickly it is accomplished. 
The time of each test is sent to a ThingSpeak server where it is visualized, 
providing the entire class/workshop access to data on other tests to see how their project compares.
*/

#define Abutt 6
#define Bbutt 2
float Time=0;
int i=0;

#include "arduino_secrets.h" 

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the Wifi radio's status

//Thingspeak info
#define SECRET_CH_ID 925875     // replace 0000000 with your channel number
#define SECRET_WRITE_APIKEY "38D0ZQ08VWO49OJU"   // replace XYZ with your channel write API Key
unsigned long myChannelNumber = SECRET_CH_ID;
const char* myWriteAPIKey = SECRET_WRITE_APIKEY;

WiFiClient  client;//initialize WiFi client

void setup() {
  Serial.begin(115200);
  pinMode(Abutt,INPUT);
  pinMode(Bbutt,INPUT);
  
  //lcd setup-------------
  Wire.begin();
  
  lcd.begin(Wire); //Set up the LCD for I2C communication
  
  lcd.setBacklight(100, 100, 100); //Set backlight to bright white
  lcd.setContrast(5); //Set contrast. Lower to 0 for higher contrast.
  
  lcd.clear(); //Clear the display - this moves the cursor to home position as well
  //--------
  
  delay(2000);
  lcd.print("Attempting Wifi");
  lcd.setCursor(0, 1); //this line will come up, it set's our lcd print to the second line of the screen
  lcd.print("connection...");
     
  //----WiFi and Thingspeak
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    lcd.print("Connection Failed!");
    while (true);
  }

  //firmware initialize and check for update
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
 
  printCurrentNet();
  printWifiData();
  //----------------------end of wifi+thingspeak setup

  delay(3000);
  lcd.clear();
  lcd.print("Ready to Test!");
  lcd.setCursor(0, 1);
  lcd.print("Play");

  Serial.println("Ready to test!"); 
}

void loop() {
  delay(100);
  if(digitalRead(Abutt)==HIGH){//look for button push to start test
  lcd.clear();
  lcd.print("Go!");
  delay(500);
  for(i = 1; i<=2400;i++){
    Time=i*.1; //time of test tracking
    Serial.println("Timer: "+ String(Time));
    delay(10);
    
    lcd.setCursor(0, 0);
    lcd.print("Timer:"+ String(Time) + " sec");//display current time elapse to user
    lcd.setCursor(0, 1);
    lcd.print("Pause       Stop"); //these are aligned with the buttons to indicate how to pause/stop the test
    
    if(digitalRead(Abutt)==HIGH){ //if the A button is pushed again, we know to spause
        Serial.println("Paused");
        lcd.clear();
        lcd.print("Paused");
        delay(400); 
        lcd.clear();
        lcd.print("Paused: "+ String(Time)+ " sec" );
        lcd.setCursor(0, 1);
        lcd.print("Play        Stop");
        delay(250);
        while(digitalRead(Abutt)==LOW){
          delay(100);
          if (digitalRead(Bbutt)==HIGH){
            FinalTime();
            break;
          }
        }
          lcd.clear();
          lcd.print("Go!");
          delay(400); 
    }
   if (digitalRead(Bbutt)==HIGH){//B button stops the test, triggering the FinalTime() function
        //replace all of this with a stop function
        FinalTime();
        break;
   }
  }
 }
}

void FinalTime() {
  int statusCode = 0; //initialize ThingSpeak write status code

 lcd.clear();
 lcd.print("Time: " + String(Time) + " sec"); //display final time to user
 lcd.setCursor(0, 1);
 lcd.print("Please wait..."); //tell user to wait for data to send before testing again
 statusCode = ThingSpeak.writeField(myChannelNumber, 1, Time, myWriteAPIKey);

  //thingspeak feedback for troubleshooting in Serial Monitor, no issues came up during user testing
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
    delay(1000);
  if(Time<13){//we need to wait before the next test can start, otherwise we risk breaching the send rate limit for ThingSpeak
    Serial.println("Please wait...");
    delay((12-Time)*1000);
  }
  lcd.clear();
  Time=0;
  i=0;
  lcd.print("Ready to Test!");
  lcd.setCursor(0, 1);
  lcd.print("Play");
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

  //Putting the strength to the LCD tells the user if they are properly hooked up to Wifi
  lcd.clear();
  lcd.print("Connection");
  lcd.setCursor(0,1);
  lcd.print("Strength: "+ String(abs(rssi)));

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

//prints mac address to serial monitor for troubleshooting
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
