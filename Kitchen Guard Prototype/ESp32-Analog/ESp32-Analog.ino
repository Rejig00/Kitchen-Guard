#include <WiFi.h>
#include  <PubSubClient.h>
#include "gas.h"

#define FAN_PWM_PIN      27
#define BUZZER_DAC_PIN   25
#define STATUS_LED_PIN   2
#define MQTT_PUB_DELAY   (5*1000)

const char* ssid           = "Robi-IOT";
const char* password       = "R0b1IoT#";
const int   mqttPort       = 15755;
const char* mqttUser       = "ksnkwraf";
const char* mqttPassword   = "5zsEpwhhNpDF";
const char* mqttServer     = "m14.cloudmqtt.com";
const char* mqttClientID   = "9C6C03A4AE30";
const char* mqttPubTopic   = "GasMonitor/9C6C03A4AE30/Sensor/MQ/5"; 
const char* mqttSubTopic   = "GasMonitor/9C6C03A4AE30/cmd"; 
    
String mqttpayload         = "10000.000"; //ppm
unsigned long  Now         = 0;
unsigned long  Then        = 0;

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
  pinMode(STATUS_LED_PIN, OUTPUT);
  
  Serial.begin(115200);
  Serial.println("\n\n\n\n___________________________\n");
  beep(500,100,2);
  //===================================

  connectWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}


void loop() 
{
  //=========================
  getAvgGasLevel();
  Serial.print(MQ_LPG_PPM);
  Serial.print(" ");
  Serial.println(MQ_RS_R0);
  addPPMToPayload();
  if(MQ_LPG_PPM > MQ_LPG_LTH && MQ_LPG_PPM < MQ_LPG_ATH)
    beep(50, 200, 1);
  else if(MQ_LPG_PPM > MQ_LPG_ATH && MQ_LPG_PPM < MQ_LPG_CTH)
    beep(100, 200, 2);
  else if(MQ_LPG_PPM > MQ_LPG_CTH)
    beep(200, 50, 3);   
  //=========================
  
  client.loop();
  while(!client.connected())
  {
    reconnect();
    client.subscribe(mqttSubTopic);
  }

  while (!client.connected())
  { 
    reconnect();
  }

  //publish data here
  Now = millis();
  if(Now-Then > MQTT_PUB_DELAY)
  {
    mqttPublishPayload();
    Then = Now;
  }
    
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
void connectWiFi()
{
    uint8_t blip = 0;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("WiFi Connecting.."); 
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(300);
      Serial.print(".");
      digitalWrite(STATUS_LED_PIN, blip);
      blip = !blip;
    }
    Serial.println("CONNECTED!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    digitalWrite(STATUS_LED_PIN, LOW);

    beep(50,50,2);
    beep(50,50,2);
}

/**
 * =======reconnect()=========
 */
void reconnect()
{
  uint8_t blip = 0;
  while (!client.connected()) 
  {
    Serial.println("\nConnecting to CloudMQTT");
    if(client.connect(mqttClientID, mqttUser, mqttPassword))
    {
      delay(10);
      Serial.println("CONNECTED");
      digitalWrite(STATUS_LED_PIN, LOW);
    }
    else 
    {
      Serial.print("Failed!\nSTATE CODE = ");
      Serial.println(client.state());
      Serial.println(" Trying in 1 Sec");
      digitalWrite(STATUS_LED_PIN, blip);
      blip = !blip;
      delay(1000);
    }
  } 
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
}
