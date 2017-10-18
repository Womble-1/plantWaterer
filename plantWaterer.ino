// v 0.2 Moisture sensor
// v0.3 included variables.h,  relay code , made mqtt_port a variable
// v0.4 gets threshold variable from MQTT topic


#include <ESP8266WiFi.h>
#include <PubSubClient.h>    //https://pubsubclient.knolleary.net/api.html 
#include <variables.h>

//variables.h contains the following
//#define wifi_ssid ""
//#define wifi_password ""
//#define mqtt_server ""
//#define mqtt_user ""
//#define mqtt_password ""
//#define mqtt_port ""

#define mqtt_clientID "plant1"
#define HostName "plant1"


#define moisture_topic "plant1/moisture"
#define log_topic "plant1/log"
//#define temperature_topic "plant1/temperature"

float sensorValue;
float MoisturePercentage;
const int relayPin = D1;
float threshold = 30;  // default percentage moisture at which action is taken
int pumpTime = 5000;   //length of time the pump is on before a recheck
int loopTime = 10000;   //length of time to pause before looping
String logout = "";   // used for logging output. to serial and to MQTT

WiFiClient espClient;
PubSubClient client(espClient);



void setup() {
  Serial.begin(115200);
  Serial.println("\n Jonny 5 is Alive");
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);  //for subscribing
  pinMode(LED_BUILTIN, OUTPUT);
  //setup the relay
  pinMode(relayPin, OUTPUT);

}

void setup_wifi() {
  //Set WiFi mode to client not AP
  WiFi.hostname(HostName);
  WiFi.mode(WIFI_STA);
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_clientID, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("plant1/threshold");
      //client.subscribe("plant1/loopTime");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool checkBound(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) &&
         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}


  //grab settings from MQTT
  //boolean subscribe (topic, [qos])
//subscribe to string: https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_basic/mqtt_basic.ino

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  payload[length] = '\0';
  String s = String((char*)payload);
  float f = s.toFloat();

//  String t = String((char*)topic);
//  Serial.print("\nt:");
//  Serial.print(t);
//  if(t = "threshold"){
//  threshold = f;
//}
//else if(t = "loopTime"){
//  loopTime = f;
//  }
  Serial.print("\nf:");
  Serial.print(f);
  Serial.println();

  threshold = f;

}


long lastMsg = 0;
float temp = 0.0;
float hum = 0.0;
float diff = 1.0;
int n = 0;


void loop() {
  if (!client.connected()) {
    Serial.println("Client not connected");
    Serial.print(" Client State:" );
    Serial.print(client.state());
    Serial.println(" ");
    reconnect();
  }
  client.loop();



  sensorValue = analogRead(A0); // read analog input pin 0
  MoisturePercentage = (1 - (sensorValue / 1024)) * 100;

  Serial.print(sensorValue, DEC); // prints the value read
  Serial.print("   ");

  Serial.print(MoisturePercentage);
  Serial.print("%");
  Serial.print("   ");
  Serial.print(threshold);
  Serial.print("%");
  Serial.print("   \n");
  logout = "threshold: "; //+= threshold;
  logout += threshold;
  logout += "\n";
  Serial.print(logout);
  client.publish(log_topic, String(logout).c_str(), true);

  client.publish(moisture_topic, String(MoisturePercentage).c_str(), true);

  if (MoisturePercentage <= threshold) {

    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    logout = "moisturePercentage <= threhold\n";    
    Serial.print(logout);    
    client.publish(log_topic, String(logout).c_str(), true);
    //turn on the relay, then off again
    digitalWrite(relayPin, HIGH); // turn on relay with voltage HIGH
    logout = "Pump Running for ";
    logout += pumpTime/1000;
    logout += " Seconds\n";
    Serial.print(logout);    
    client.publish(log_topic, String(logout).c_str(), true);
    delay(pumpTime);              // pumpRunning
    digitalWrite(relayPin, LOW);  // turn off relay with voltage LOW

  }
  else {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    logout = "moisturePercentage > threhold\n";    
    Serial.print(logout);    
    client.publish(log_topic, String(logout).c_str(), true);
  }

  //      for (int i=0; i <= 10; i++){
  //            delay(10000);
  //      }
  // 600000 = 10 mins

  logout = "Waiting for ";
  logout += loopTime/1000;
  logout += " Seconds\n";
  Serial.print(logout);    
  client.publish(log_topic, String(logout).c_str(), true);
  
  delay(loopTime);
  Serial.print(" \n");
}
