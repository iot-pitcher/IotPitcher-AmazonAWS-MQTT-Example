#include <IotPitcher_Simple_LoRa_Transceiver.h>
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include "secrets.h"

#define MAX_MESSAGE_LENGTH 256

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(512);
SoftwareSerial swSer;
const char* aws_topic  = "topic_name";   

//callback to handle mqtt messages
void messageHandler(String &topic, String &payload)  {
  //STM32L0 will handle received message
  Serial.println(payload);
  swSer.print(payload);
}

void on_set_loraconfiguration_centerfrequency(){
    std::string value = pitcher_get_loraconfiguration_centerfrequency();
    StaticJsonDocument<128> JSONdata;
    JSONdata["CenterFrequency"]=value.c_str();
    //send a message   
    char buf[128];
    serializeJson(JSONdata, buf);
    swSer.print(buf);
    Serial.println(buf);
}

void on_set_loraconfiguration_bandwidth(){
    std::string value = pitcher_get_loraconfiguration_bandwidth();
    Serial.println(value.c_str());
    StaticJsonDocument<128> JSONdata;
    JSONdata["Bandwidth"]=value.c_str();
    //send a message   
    char buf[128];
    serializeJson(JSONdata, buf);
    swSer.print(buf);
    Serial.println(buf);
}

void on_set_loraconfiguration_codingrate(){
    std::string value = pitcher_get_loraconfiguration_codingrate();
    Serial.println(value.c_str());
    StaticJsonDocument<128> JSONdata;
    JSONdata["CodingRate"]=value.c_str();
    //send a message   
    char buf[128];
    serializeJson(JSONdata, buf);
    swSer.print(buf);
    Serial.println(buf);
}

void on_set_loraconfiguration_transmitpower(){
    std::string value = pitcher_get_loraconfiguration_transmitpower();
    Serial.println(value.c_str());
    StaticJsonDocument<128> JSONdata;
    JSONdata["TransmitPower"]=value.c_str();
    //send a message   
    char buf[128];
    serializeJson(JSONdata, buf);
    swSer.print(buf);
    Serial.println(buf);
}

void on_set_loraconfiguration_payloadlength(){
    std::string value = pitcher_get_loraconfiguration_payloadlength();
    Serial.println(value.c_str());
    StaticJsonDocument<128> JSONdata;
    JSONdata["PayloadLength"]=value.c_str();
    //send a message   
    char buf[128];
    serializeJson(JSONdata, buf);
    swSer.print(buf);
    Serial.println(buf);
}
volatile bool aws_connected = false;

void connectAWS(){
  Serial.println("Connecting to AWS IOT");
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.println("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

    Serial.println("AWS IoT Connected!");
  // Subscribe to a topic
    client.subscribe(String(aws_topic)+"/sub");
   //subscript to a topic
    Serial.println("MQTT subscribed");
  aws_connected = true;
}

//send a message to a mqtt topic/
void sendMessageData (char *messageData) {
    StaticJsonDocument<256> JSONdata;
    char *dataParse[2];
    int dataPos;
    char buf[64];
    int dataSize=64;
    
    pitcher_take_payloadconfiguration_data1(buf,dataSize);
    dataParse[0]=strtok(buf,"@");
    dataParse[1]=strtok(NULL,"");
    JSONdata[dataParse[0]]=messageData[atoi(dataParse[1])];

    pitcher_take_payloadconfiguration_data2(buf,dataSize);
    dataParse[0]=strtok(buf,"@");
    dataParse[1]=strtok(NULL,"");
    JSONdata[dataParse[0]]=messageData[atoi(dataParse[1])];

    pitcher_take_payloadconfiguration_data3(buf,dataSize);
    dataParse[0]=strtok(buf,"@");
    dataParse[1]=strtok(NULL,"");
    JSONdata[dataParse[0]]=messageData[atoi(dataParse[1])];
    
    //send a message   
    char messageBuf[256];
    serializeJson(JSONdata, messageBuf);
    Serial.println(messageBuf);
    client.publish(String(aws_topic)+"/pub",messageBuf);
}

void setup() {
    pitcher_set_loraconfiguration_centerfrequency_on_set_callback( on_set_loraconfiguration_centerfrequency );
    pitcher_set_loraconfiguration_bandwidth_on_set_callback( on_set_loraconfiguration_bandwidth );
    pitcher_set_loraconfiguration_codingrate_on_set_callback( on_set_loraconfiguration_codingrate );
    pitcher_set_loraconfiguration_transmitpower_on_set_callback( on_set_loraconfiguration_transmitpower );
    pitcher_set_loraconfiguration_payloadlength_on_set_callback( on_set_loraconfiguration_payloadlength );

    pitcher_set_wifi_connect_callback(connectAWS);
    pitcher_start();
    
    Serial.begin (115200);
    swSer.begin(115200, SWSERIAL_8N1, 34, 32, false, 64, 11);

    pinMode(5, OUTPUT);
    digitalWrite(5, LOW);
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH); 
    

  while(pitcher_wifi_is_connected() == false || aws_connected == false){
    printf("...\n");
    delay(500);
  }

  printf("Going to loop aws_connected %d\n",aws_connected);

}

int j=0;
char messageData[MAX_MESSAGE_LENGTH];
bool messageReceiving=false;

void loop() {
  if(swSer.available() > 0) {
    char readVal = swSer.read();
    if(messageReceiving){
      if(readVal=='\n'){
      char tmpData[j+1];
      strcpy(tmpData,messageData);
      tmpData[j]=';';
      Serial.println(tmpData);
      sendMessageData(tmpData);
      j=0;
      messageReceiving=false;
    }else{
      messageData[j]=readVal;
      j++;
    }
    }
    if(readVal=='*'){
      messageReceiving=true;
    }
  }
  if (client.connected ()) {   
     client.loop ();
  } 
}
