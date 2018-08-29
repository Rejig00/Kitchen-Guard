#include <WiFi.h>
#include  <PubSubClient.h>
#include "gas.h"

#define FAN_PWM_PIN      27
#define BUZZER_DAC_PIN   32
#define WIFI_PIN         2
#define MQTT_PIN         15
#define MQTT_PUB_DELAY   (5*1000)
#define FAN_START_DELAY  (30*1000)
#define FAN_RUN_DELAY    (5*1000)

const char* ssid              = "PEACE";
const char* password          = "home0987";
//const char* ssid              = "Robi-IOT";
//const char* password          = "R0b1IoT#";
//const char* ssid              = "Redmi";
//const char* password          = "Omi10004659"; 
const int   mqttPort            = 15755;
const char* mqttUser            = "ksnkwraf";
const char* mqttPassword        = "5zsEpwhhNpDF";
const char* mqttServer          = "m14.cloudmqtt.com";
const char* mqttClientID        = "9C6C03A4AE30";
const char* mqttPubTopic        = "GasMonitor/9C6C03A4AE30/Sensor/MQ/5"; 
const char* mqttSubTopic        = "GasMonitor/9C6C03A4AE30/cmd"; 
    
String mqttpayload              = "10000.000"; //ppm
unsigned long  Now              = 0;
unsigned long  Then             = 0;
unsigned long  Fan_start        = 0;
unsigned long  Fan_stop         = 0;
unsigned long  Fan_run_start    = 0;
unsigned long  Fan_run_stop     = 0;

WiFiClient                   espClient;
PubSubClient                 client(espClient);


//============forward declarations=============

void beep(int duty=100, int interval=200, int beepNumbers=2)
{
   for(int i=0; i<beepNumbers; i++)
   {
      digitalWrite(BUZZER_DAC_PIN, HIGH);
      delay(duty);
      digitalWrite(BUZZER_DAC_PIN, LOW);
      if(i==beepNumbers) break;
      delay(interval);
   }
}


//======================SETUP===========================
void setup() 
{
  //===================================
  pinMode(AN_SENSOR_0,     INPUT);
  pinMode(BUZZER_DAC_PIN, OUTPUT);
  pinMode(FAN_PWM_PIN,    OUTPUT);
  pinMode(WIFI_PIN, OUTPUT);
  pinMode(MQTT_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("\n\n\n\n___________________________\n");
  beep(500,100,2);
  //===================================

  WiFi.mode(WIFI_STA);                  //Put the NodeMCU in Station mode.
  delay(1000);
  WiFi.disconnect();                    //Disconnect from previous connections.
  Serial.println("Scan Start");
  int n = WiFi.scanNetworks();          //Scan the number of available networks
  if(n==0){
    Serial.println("No Networks Found"); 
  }
  else{
    Serial.print(n);
    Serial.println(" Networks Found");
  }
  Serial.println("List of surrounding Network SSIDs…:");    
  for (int i = 0; i < n; i++)
  {
    Serial.print(i+1);
    Serial.print(".");
    Serial.println(WiFi.SSID(i));
  }
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  while(WiFi.status() != WL_CONNECTED) {  // while being connected
    delay(500);
    Serial.print(".");
  }
  digitalWrite(WIFI_PIN,HIGH);
  digitalWrite(FAN_PWM_PIN, HIGH);
  beep(50,50,2);
  beep(50,50,2);
  Serial.println();
  Serial.print("Connected to: ");
  Serial.println(WiFi.SSID());
  //Serial.print(". IP Address: ");
  //Serial.println(WiFi.localIP());
  //Serial.print("Current Signal Strength: ");
  //Serial.println(WiFi.RSSI());
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}


void loop() 
{
  //=========================
  if(WiFi.status() != WL_CONNECTED){
    digitalWrite(WIFI_PIN , LOW);
    digitalWrite(MQTT_PIN , LOW);
    reconnectWifi();
  }
  while(WiFi.status() != WL_CONNECTED) {  // while being connected
    delay(500);
    Serial.print(".");
  }
  digitalWrite(WIFI_PIN,HIGH);

  if (!client.connected()) {
    digitalWrite(MQTT_PIN , LOW);
    reconnectMQTT();
  }
  getAvgGasLevel();
  //Serial.print(MQ_LPG_PPM);
  //Serial.print(" ");
  //Serial.println(MQ_RS_R0);
  addPPMToPayload();
  if(MQ_LPG_PPM > MQ_LPG_LTH && MQ_LPG_PPM < MQ_LPG_ATH)
    beep(50, 200, 1);
  else if(MQ_LPG_PPM > MQ_LPG_ATH && MQ_LPG_PPM < MQ_LPG_CTH)
    beep(100, 200, 2);
  else if(MQ_LPG_PPM > MQ_LPG_CTH)
    beep(200, 50, 3);   
  //=========================

  //Fan Stop
  /*Fan_run_start = millis();
    if(Fan_run_start - Fan_run_stop > FAN_RUN_DELAY)
    {
      digitalWrite(FAN_PWM_PIN, LOW);
      Fan_run_stop = Fan_run_start;
    }*/

  //publish data here
  Now = millis();
  if(Now-Then > MQTT_PUB_DELAY)
  {
    mqttPublishPayload();
    Then = Now;
  }

  //Turn fan on and off
  /*Fan_start = millis();
  if(Fan_start - Fan_stop > FAN_START_DELAY)
  {
    digitalWrite(FAN_PWM_PIN, HIGH);
    Fan_stop = Fan_start;
  }*/
    
}//==============EOF LOOP=======================


/**
 * mqttpublish()
 */

void mqttPublishPayload(void)
{
  Serial.println("Publishing...");
  while(!client.publish(mqttPubTopic, mqttpayload.c_str())) 
    Serial.print('>');
  Serial.println(mqttpayload);
}




/**
 * ======connectWiFi()==========
 */
void reconnectWifi() {
  digitalWrite(WIFI_PIN , LOW);
  Serial.println("Reconecting WiFi...");
  WiFi.begin(ssid, password);
}

/**
 * =======reconnect()=========
 */
void reconnectMQTT() {
  // Loop until we're reconnected
  while (!client.connected()) {
    delay(1000);
    if(WiFi.status() != WL_CONNECTED){
    digitalWrite(WIFI_PIN , LOW);
    reconnectWifi();
  }
  while(WiFi.status() != WL_CONNECTED) {  // while being connected
    delay(500);
    Serial.print(".");
  }
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqttClientID, mqttUser, mqttPassword)) {
      digitalWrite(MQTT_PIN , HIGH);
      Serial.println("connected");   
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  client.subscribe(mqttSubTopic);
}
/**
 * =========callback===========
 */
void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Recieved:");
  Serial.println(topic); 
  Serial.print("Value:");
  for (int i = 0; i < length; i++) 
  {
    Serial.print((char)payload[i]);
  } 
}

void addPPMToPayload(void)
{
  mqttpayload = String(MQ_LPG_PPM,2);
  //mqttpayload = analogRead(33);
}
