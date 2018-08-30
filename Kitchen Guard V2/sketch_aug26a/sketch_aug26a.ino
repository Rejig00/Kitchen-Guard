//========================== Libraries==================================//
#include <WiFi.h>
#include <PubSubClient.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include "gas.h"



//==========================================================================//
#define MQ_PWR      16        //Gas Sensor Switch
#define BZ_PWR      25        //Buzzer Switch
#define FN_PWR      4         //Fan Switch
//#define WIFI_PIN  27           
//#define MQTT_PIN  26            
//#define PWR       14

#define MQ_SNS      39
#define BT_SNS      36
#define SW_USR      17

#define MQTT_PUB_DELAY   (60*1000)
#define FAN_START_DELAY  (60*1000)
#define FAN_RUN_DELAY    (10*1000)

//#define _HIGH     LOW
//#define _LOW      HIGH



//===================== Declaring Objects=============================//
WiFiManager   wifiManager;
Preferences   preferences;
WiFiClient    espClient;
PubSubClient  client(espClient);

//============================ Declaring Custom Parameters=================//
//WiFiManagerParameter custom_mqtt_topic("topic", "Mqtt Topic", "", 20);


//======================== Declaring Variables==================================//
String            nvs_ssid;           //For fetching saved SSID
String            nvs_pass;           //For fetching saved Password
//String          nvs_mqtt_topic;     //For fetching saved Topic
String            to_be_saved_ssid;
String            to_be_saved_pass;
String            mqttpayload       = "10000.000";    //ppm
String            MQTT_Topic;

unsigned long     Now               = 0;
unsigned long     Then              = 0;
unsigned long     Fan_start         = 0;
unsigned long     Fan_stop          = 0;
unsigned long     Fan_run_start     = 0;
unsigned long     Fan_run_stop      = 0;

char              ssid[20];
char              pass[20];
char              mqtt_topic[20];

const char*       mqtt_client       = "espClient";
const char*       mqtt_server       = "m14.cloudmqtt.com";
const char*       mqtt_user         = "ksnkwraf";
const char*       mqtt_pass         = "5zsEpwhhNpDF";


//=========== Function Declarations=============//
void initIO(void)
{
  pinMode(MQ_PWR, OUTPUT);        //active high 
  pinMode(BZ_PWR, OUTPUT);        //active high 
  pinMode(FN_PWR, OUTPUT);        //active high
  //pinMode(PWR, OUTPUT);         //active low
  //pinMode(WIFI_PIN, OUTPUT);    //active low 
  //pinMode(MQTT_PIN, OUTPUT);    //active low

  pinMode(MQ_SNS, INPUT);   //analog 
  pinMode(BT_SNS, INPUT);   //analog
  pinMode(SW_USR, INPUT);   //digital interrupt, pulled up, NO  
}

void setInitIO(void)
{
  //digitalWrite(PWR, _LOW);
  //digitalWrite(WIFI_PIN, _LOW);
  //digitalWrite(MQTT_PIN, _LOW);
  digitalWrite(MQ_PWR, LOW);
  digitalWrite(BZ_PWR, LOW);
  digitalWrite(FN_PWR, LOW);
}

void setupNVS(void)
{
  preferences.begin("my-app", false);                 //Begin the preferences. Second parameter should be false.
  //preferences.clear();                              //Clears the preference
}

void fetch_saved_data()
{
  nvs_ssid = preferences.getString("ssid");               //Fetch the saved SSID
  nvs_pass = preferences.getString("pass");               //Fetch the saved Password
  //nvs_mqtt_topic = preferences.getString("mqtt_topic"); //Fetch the saved Topic
}

void saveToNVS()
{
   to_be_saved_ssid = wifiManager.get_ssid();
   to_be_saved_pass = wifiManager.get_pass();
   //strcpy(mqtt_topic, custom_mqtt_topic.getValue());
   //preferences.putString("mqtt_topic", mqtt_topic);
   preferences.putString("ssid", to_be_saved_ssid);
   preferences.putString("pass", to_be_saved_pass);
   //nvs_mqtt_topic = mqtt_topic;           //nvs_topic is used for publishing. So we have copied mqtt_topic(which is a char) to nvs_mqtt_topic(which is a string
}


String getTopic(void)
{
  uint64_t    chipid         = ESP.getEfuseMac();
  uint16_t    MAC_H2B        = (uint16_t)(chipid>>32);
  uint32_t    MAC_L4B        = (uint32_t)(chipid);
  String      MAC_H2STR      = String(MAC_H2B, HEX);
  String      MAC_L4STR      = String(MAC_L4B, HEX);
  MAC_H2STR.toUpperCase();
  MAC_L4STR.toUpperCase();
  String      MAC            = MAC_H2STR + "0" + MAC_L4STR;
  String      topic          = MAC + "/GasPPM";
  return      topic;
}


void beep(int duty=100, int interval=200, int beepNumbers=2)
{
   for(int i=0; i<beepNumbers; i++)
   {
      digitalWrite(BZ_PWR, HIGH);
      delay(duty);
      digitalWrite(BZ_PWR, LOW);
      if(i==beepNumbers) break;
      delay(interval);
   }
}


//================= Setup=======================//
void setup() 
{
  Serial.begin(115200);
  
  //====================Initials=================//
  initIO();                         
  setInitIO();
  setupNVS();
  //====================Initials=================// 

  //digitalWrite(PWR, _HIGH);         //Power LED
  beep(500,100,2);
  //digitalWrite(MQ_PWR, HIGH);
  
  
  //wifiManager.addParameter(&custom_mqtt_topic);       //Declaring custom parameter

  fetch_saved_data();       //Fetch the datas stored in non-volatile-memory

  if(nvs_ssid.length() == 0)    //If no data is saved in non-volatile-memory
  {
    //Serial.println(nvs_ssid);
    wifiManager.setConfigPortalTimeout(180);                              //In case of connection failure during configuration, after 3 minutes the ESP will return to AP mode
    wifiManager.startConfigPortal("Kitchen Guard V1.0.0", "123456789");   //Initial Access point ssid and pass
    saveToNVS();
    Serial.println("Configurations Saved");
  }
  else        //If data is saved to memory
  {
    nvs_ssid.toCharArray(ssid, (nvs_ssid.length()+1));
    nvs_pass.toCharArray(pass, (nvs_pass.length()+1));
    WiFi.begin(ssid , pass);
    Serial.print("Connecting to ");
    Serial.println(ssid);
    while(WiFi.status() != WL_CONNECTED) {  // while being connected
      delay(500);
      Serial.print(".");
    }
  }
  Serial.print("Connected to: ");
  Serial.println(WiFi.SSID());
  //digitalWrite(WIFI_PIN, _HIGH);
  beep(50,50,2);
  beep(50,50,2);
  preferences.end();
  
  client.setServer(mqtt_server, 15755);
  MQTT_Topic = getTopic();
  MQTT_Topic.toCharArray(mqtt_topic, 20);
}


//======================= Start of Loop =====================//



void loop() 
{

  //==================== WiFi Connection Stability====================//
  if(WiFi.status() != WL_CONNECTED){
    //digitalWrite(WIFI_PIN , _LOW);
    //digitalWrite(MQTT_PIN , _LOW);
    reconnectWifi();
  }
  while(WiFi.status() != WL_CONNECTED) {  // while being connected
    delay(500);
    Serial.print(".");
  }
  //digitalWrite(WIFI_PIN, _HIGH);
  if (!client.connected()) {
    //digitalWrite(MQTT_PIN , _LOW);
    reconnectMQTT();
  }
  //===================================================================//
  delay(5*60*1000);
  //========================= Get Gas Level=============================//
  getAvgGasLevel();
  addPPMToPayload();
  if(MQ_LPG_PPM > MQ_LPG_LTH && MQ_LPG_PPM < MQ_LPG_ATH)
    beep(50, 200, 1);
  else if(MQ_LPG_PPM > MQ_LPG_ATH && MQ_LPG_PPM < MQ_LPG_CTH)
    beep(100, 200, 2);
  else if(MQ_LPG_PPM > MQ_LPG_CTH)
    beep(200, 50, 3);

    //==================== Publish to MQTT =======================//
    Now = millis();
  if(Now-Then > MQTT_PUB_DELAY)
  {
    
    Serial.println("Publishing...");
    while(!client.publish(mqtt_topic, mqttpayload.c_str())) 
    Serial.print('>');
    Serial.println(mqttpayload);
    
    Then = Now;
  }

  //====================== Fan Start =================================//

  Fan_start = millis();
  if(Fan_start - Fan_stop > FAN_START_DELAY)
  {
    digitalWrite(FN_PWR, HIGH);
    Fan_stop    =   Fan_start;
    Serial.println("Fan has started");
    Fan_run_start = millis();
    Fan_run_stop  =   Fan_run_start;
  }


  //====================== Fan Stop ===================================//

  Fan_run_start = millis();
  if(Fan_run_start - Fan_run_stop > FAN_RUN_DELAY)
  {
    digitalWrite(FN_PWR, LOW);
    Serial.println("Fan has stopped");
    Fan_run_stop  =   Fan_run_start;
  }
//  Serial.println(digitalRead(SW_USR));
  if(!digitalRead(SW_USR))
  {
    preferences.begin("my-app", false);
    preferences.clear();                              //Clears the preference
    preferences.end();
    ESP.restart();
  }
}

//======================== End of Loop ==========================//


//===================== Payload ===========================================//


void addPPMToPayload(void)
{
  mqttpayload = String(MQ_LPG_PPM,2);
  //mqttpayload = analogRead(AN_SENSOR_0);
}






//================ Reconnect MQTT ===========================//

void reconnectMQTT() {
  // Loop until we're reconnected
  while (!client.connected()) {
    delay(1000);
    if(WiFi.status() != WL_CONNECTED){
    //digitalWrite(WIFI_PIN , _LOW);
    reconnectWifi();
  }
  while(WiFi.status() != WL_CONNECTED) {  // while being connected
    delay(500);
    Serial.print(".");
  }
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_client,mqtt_user,mqtt_pass)) {
      //digitalWrite(MQTT_PIN , _HIGH);
      Serial.println("connected");   
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


//====================== Reconnect WiFi ==============================//

void reconnectWifi() {
  //digitalWrite(WIFI_PIN , _LOW);
  Serial.println("Reconecting WiFi...");
  WiFi.begin(ssid, pass);
}
