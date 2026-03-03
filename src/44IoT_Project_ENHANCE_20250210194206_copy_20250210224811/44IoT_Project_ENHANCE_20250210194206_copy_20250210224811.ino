
/* 44IoT Activities development code */
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "credentials.h"//These lines include necessary libraries for WiFi connectivity, MQTT communication, JSON handling, and OLED display control.
#include "parameters.h"
#include "profile.h"
#include "cert.h"
#include <Adafruit_GFX.h>                       // OLED lib
#include <Adafruit_SSD1306.h>                   // OLED lib

#include <Wire.h>                               // I2C lib


/*** Define Board IO Pins ***/
#define LD1 23
#define DoubleLD1 18              // represents green colour in ithe double led
#define DoubleLD2 19             //represent red colour
#define Buzzer 4
#define LDR2 34  //define board io pins
#define Button 25  

#define LD2 15
#define LD3 13                                  // Note GPIO13 has multiple use; controls Vmon, Vext & LD3 *  
#define LDR 39     
#define PB1 0
#define PB2 27
#define A36 36                                  // for measuring of Vmon
#define A13 13                                  // for controlling Vext and Vmon       
#define SPK 2
#define mySensor 35
//OLED Display Configuration
#define SCREEN_WIDTH 128        // OLED display width, in pixels
#define SCREEN_HEIGHT 64        // OLED display height, in pixels
#define OLED_RESET     -1       // Reset pin
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


#define RAD2DEG  57.3  
/*** Define Expansion IO Pins ***/
#define RELAY 5
/*** Global variables ***/
float sensor,ldrsensor;
long timer;                        
bool currPB2Val, oldPB2Val;                     // States of PB2
char publish_topic[100];
char subscribe_topic[100];
bool button;
String state,plant;
int f;
String line1, line2, line3;


/*** WIFI and MQTT Client setup ***/
#if defined(HIVEMQS)||defined(MOSQUITTOS)     // Read 'profile.h' on #ifdef
 WiFiClientSecure espClient;                    // Creates a client with secure TCP (SSL) connection via WIFI
#else
 WiFiClient espClient;                          // Creates a client with TCP connection via WIFI 
#endif
PubSubClient client(espClient);                 // Enable the client to connecf with MQTT

void setup() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));//Initializes the OLED display and sets up GPIO pins, MQTT topics, and serial communication.
    for (;;); // Stop the program if OLED initialization fails
}
  /*** GPIO setup ***/
  pinMode(LD1, OUTPUT); 
  pinMode(DoubleLD1, OUTPUT);
  pinMode(DoubleLD2, OUTPUT);  //for double led
  pinMode(LDR2, INPUT); 
  pinMode(Buzzer, OUTPUT);
  pinMode(Button, INPUT);  

  pinMode(LD2, OUTPUT);
  pinMode(LD3, OUTPUT); 
  pinMode(A36, INPUT);  
  pinMode(LDR, INPUT);
  pinMode(PB2, INPUT);
  pinMode(mySensor, INPUT);
  pinMode(RELAY, OUTPUT);  
  /*** Initialise ***/
  oldPB2Val = digitalRead(PB2);          // PB2 state
  digitalWrite(RELAY, HIGH);             // RELAY pin HIGH to turn off
  digitalWrite(LD1, HIGH);               // all LED off
  digitalWrite(LD2, HIGH);               
  digitalWrite(LD3, HIGH); 
  digitalWrite(RELAY, HIGH);             
  timer= millis();                       // timer
  set_topics();                          // setup MQTT topics

  /*** Setup Serial Communication ***/
  Serial.begin(19200);
  while (!Serial) delay(1);  
  /*** Setup WIFI & MQTT Communication ***/    
  #if defined(Publish)||defined(Subscribe)                              
     setup_wifi();
     client.setServer(mqtt_server, mqtt_port);
     client.setCallback(callback);
     client.setKeepAlive(keepalive);
    #ifdef HIVEMQS 
     espClient.setCACert(root_ca1);       // Read 'cert.h' on MQTTS; Public Key of HIVEMQ
    #endif
    #ifdef MOSQUITTOS 
     espClient.setCACert(root_ca2);       // Read 'cert.h' on MQTTS; Public Key of MOSQUITTO
    #endif
    mqttconnect();
  #endif
}
void Growth()
{ 
  if (ldrsensor < 59      ){                                  
        Serial.println("  / TOO DARK!!!"); 

        digitalWrite(DoubleLD1, LOW);
        digitalWrite(DoubleLD2, LOW);  
                                                                                                                                                             
      }
     else if (ldrsensor > 100      ){                                  
        Serial.println("  / No Need for Growth Light");                
       
        digitalWrite(DoubleLD1, HIGH);
        digitalWrite(DoubleLD2, HIGH);  
        
     }
      else {                                                                                                        
        Serial.println("/ Insuffient Lighting");                                     
        
        digitalWrite(DoubleLD1, LOW);
        digitalWrite(DoubleLD2, HIGH); 
                                                                                                                                                                               
      }

}




void Modes()
{
  button = digitalRead(Button);
    if (button == LOW) {
        f++;
        button = true;
        if (f == 2) {
            f = 0;
        }
    } else {
        return;
    }
}

void loop() {
  //Modes();
  
  if (millis() > timer){                    // check timer to start publishing cycle
    /*** read LDR ***/
    Modes();
    sensor = analogRead(mySensor);               // Read Va binary code
    sensor = (sensor +100)*2048/1988;       // 2-pt compensation
    Serial.print("Sense binary:");                       
    Serial.print((short)sensor);                                     
    sensor = sensor * 0.0008;               // Convert binary to voltage by x3.3/4095 ( = x0.0008)
    Serial.print(" / Sense Voltage Va (V):");                 
    Serial.print(sensor);

    ldrsensor = analogRead(LDR2);               // Read Va binary code
    ldrsensor = (ldrsensor +100)*2048/1988;       // 2-pt compensation
    ldrsensor = ldrsensor * 0.0008;
    ldrsensor = 200/ldrsensor;
    Serial.print("/ ldr value:");
    Serial.print(ldrsensor);

    sensor = ((sensor-0.4)/0.0195);
    Serial.print(" Tempreature : ");
     Serial.print(sensor);

     line1 = "Temperature: " + String(sensor) + "C";  
    line2 = "Plant Type: " + String(plant);  
    line3 = "Brightness: " + String(ldrsensor)+ "LUX"; 



     digitalWrite(DoubleLD1, HIGH);
        digitalWrite(DoubleLD2, HIGH);
        Growth();
  

    
 if (f == 0) {
   Serial.print(" Carrot");
    plant="Carrot";
} else if (f == 1) {
  Serial.print(" Strawberies");
    plant="Strawberies";
} 
oleddisplay();  // Update OLED display

#if defined(Publish)
      /*** JSON Object ***/  
      DynamicJsonDocument doc(256);                         // JSON Object format: {"key1":value1,"key2":value2,...}  
      doc["v"] = (short)(sensor*100);                        // 1st property eg. sense value
      doc["l"] = (short)(ldrsensor*10); 
      doc["m"] = (short)(f+1);
      doc["x"] = (state);
      doc["r"] = (short)! digitalRead (RELAY);                  // 2nd property eg. voltage of sensor
      char mqtt_message[256];
      serializeJson(doc, mqtt_message);
 
      /*** MQTT Publish ***/
      client.publish(publish_topic, mqtt_message, false);
      Serial.println("Published Topic: "+String(publish_topic));
      Serial.println("Published Message: "+String(mqtt_message));
   #endif

  
   /*** Set timer for next Transmit; non-blocking ***/ 
   timer = millis() + max(mintxinterval, 10000);  }         //reset timer; end of publishing cycle
  
    
    /*** client.loop allow the client to process incoming subscribed messages thru callback() ***/
    #if defined(Publish) || defined(Subscribe)                             
      if(!client.loop()) mqttconnect() ;        
    #endif

    /*** Check PB2 press ***/
    if (pb2PressedChk()){
      if (sensor < 100) {
        digitalWrite(LD1, !digitalRead(LD1));
        }                              ;
    }  
}




/******************** Functions **********************/

/*** MQTT Call back Method ***/
void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for (int i = 0; i < length; i++) incommingMessage+=(char)payload[i];
  Serial.println("Message arrived ["+String(topic)+"]"+incommingMessage);
  if( String(topic).equalsIgnoreCase(subscribe_topic)){   
                    //check if incoming topic == subscribe_topic
     if (incommingMessage.equalsIgnoreCase("COOLING FAN ON")) 
          { digitalWrite(RELAY,LOW);
          digitalWrite(Buzzer, HIGH); 
        delay(1000);
        digitalWrite(Buzzer, LOW);
         digitalWrite(LD1, HIGH);
         Serial.println("  / Its scorching HOT!!!");
         state="hot";

                }


         else if(incommingMessage.equalsIgnoreCase("HEATER ON"))
         {
digitalWrite(LD1, LOW);
digitalWrite(RELAY,HIGH);
digitalWrite(Buzzer, HIGH); 
        delay(500);
        digitalWrite(Buzzer, LOW);
        delay(500);
        digitalWrite(Buzzer, HIGH); 
        delay(500);
        digitalWrite(Buzzer, LOW);
        Serial.println("  / Its Too COLD!!!");
        state="cold";
         }
     else 
          {
            digitalWrite(RELAY,HIGH); 
            digitalWrite(LD1, HIGH);
            Serial.println("/ Idel Tempreature"); 
            state="ideal";
                                     }                             
   }
}



/*** Connect to WiFi ***/
void setup_wifi() {
  delay(10);
  Serial.print("\nConnecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("w");
  }  
  Serial.println("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());
}

/*** Connect to MQTT Broker ***/
void mqttconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientid, mqtt_username, mqtt_password)) {
      Serial.println("connected");
       #if defined(Subscribe)
         client.subscribe(subscribe_topic, QoS);        // subscribe topic
       #endif
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");        // Wait 5 seconds before retrying
      delay(5000);
      
    }
  }
}

/*** Topics setup ***/
void set_topics(){
  sprintf(publish_topic, "%s/%s/%s", appname,clientid,device);
  sprintf(subscribe_topic, "DL/%s/%s/%s", appname,clientid,device);
}

/*** Pushbutton check ***/
 bool pb2PressedChk() {                      // returns true if PB is pressed, false if not pressed
 currPB2Val = digitalRead(PB2);
  if(currPB2Val != oldPB2Val) {              // if there is transition
   oldPB2Val = currPB2Val;
    if(currPB2Val == LOW) {                  // if transition is HI to LO
     return true; }
  }
 return false;
 }
void oleddisplay() {
  display.clearDisplay();                   // clear screen
  display.setTextSize(1);                   // set font size
  display.setTextColor(WHITE);              // set color
  display.setCursor(0,0);                   // line1 @row0
  display.println(line1);
  display.setCursor(0,25);                  // line2 @row25
  display.println(line2);
  display.setCursor(0,45);                  // line3 @row45
  display.println(line3);
  display.display();                        // print to screen
 }
 /*** Battery monitor ***/
 float batterymonitor() {                   // For IoT Power Mgt
  digitalWrite(A13, HIGH);                  // High -> Vmon
  delay(1);
  float reading = analogRead(A36);          // ADC read Vmon
  reading = (reading +100)*2048/1988;       // 2-pt compensation
  reading = reading * 0.0008;               // Convert binary to voltage: x3.3/4095 (ie. = x0.0008) 
  reading = (2*reading/Vbatmax) * 100;      // ?
  return reading;
 }