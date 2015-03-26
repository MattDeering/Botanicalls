// Botanicalls allows plants to ask for human help.
// http://www.botanicalls.com
// rewrite of the original program to use MQTT to send messages. Only data points are sent
// to the MQTT message server.  Software elsewhere can then interpret this data and
// pass the results to any number of other agents, e.g., Twitter, Pachube, Nimbits, ThingSpeak, etc.
// Modified by Mark Schulz to support MQTT
// Modified by Matthew Deering to support Chirp Soild Moisture Sensor
// Last Update: Feb 13, 2013
//
// Botanicalls is a project with Kati London, Rob Faludi and Kate Hartman

#define VERSION "0.1" // use with 2.2 leaf board hardware

#include <UIPEthernet.h> // Ethernet library
#include <PubSubClient.h>  // MQTT library
#include "BotanicallsMQTT.h"
#include <I2C.h> //I2C

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[]    = { 0x90, 0xA2, 0xDA, 0x00, 0x00, 0x15  };
byte mqttServer[] = { 130, 102, 129, 175 };  // winter.ceit.uq.edu.au
byte ip[]     = { 130, 102, 86, 55 };  // botanicalsMQTT.ceit.uq.edu.au
byte dnsServer[] = { 130, 102, 128, 43 };  //cuscus.cc.uq.edu.au
byte gateway[] = { 130, 102, 86, 1 };
byte subnet[] = { 255, 255, 255, 128 };

// MQTT data
char *clientID = "botanicallsMQTT2";
char *topicData = "Plant/data";

//Sensor pins for I2C use
#define MOISTURE 0
#define LIGHT 4
#define TEMP 5

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

// buffer in which to build printable strings
char msg[100];

EthernetClient ethClient;
PubSubClient client(mqttServer, 1883, callback, ethClient);  // No subscribe at the moment.

void setup() { 
  // start Ethernet
  Ethernet.begin(mac, ip, dnsServer, gateway, subnet);
  delay(1000);

  Serial.begin(115200);   // set the data rate for the hardware serial port
  sprintf(msg, "Botanicalls v%s", VERSION);
  Serial.println(msg);
 
  sprintf(msg, "mac: %.2X:%.2X:%.2X:%.2X:%.2X:%.2X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(msg);
  
  Serial.println("Starting Botanicalls->MQTT transfer");
  
  
}

//writeI2CRegister8bit
//
//Write data via I2c to Chirp
//
//Return: void
void writeI2CRegister8bit(int addr, int value) {
  I2c.begin();
  I2c.write(addr, value);
  I2c.end();
}

//readI2CRegister16bit
//
//Read I2c data from Chirp
//
//Return: unsigned int
unsigned int readI2CRegister16bit(int addr, int reg) {
  I2c.begin();
  I2c.write(addr, reg);
  delay(20);
  I2c.read(addr, 2);
  unsigned int t = I2c.receive() << 8;
  t = t | I2c.receive();
  I2c.end();
  return t;
}

// readStats
//
// Take a mositure, light and temperature reading from the sensor.
//
// Return: moisture reading as a long.

char* readStats() {
  int array[3];
  static char buffer[80];
  
  array[0] = readI2CRegister16bit(0x20, MOISTURE); //moisture
  writeI2CRegister8bit(0x20, 3); //request light measurement 
  delay(1000);                   //this can take a while
  array[1] = readI2CRegister16bit(0x20, LIGHT); //light
  array[2] = readI2CRegister16bit(0x20, TEMP); //temperature
  
  sprintf(buffer, "{\"moisture\":%i,\"light\":%i,\"temperature\":%i}", array[0], array[1], array[2]);
  
  return buffer; 
}

void loop()       // main loop of the program     
{
  char* buffer = readStats();
  if (client.loop() == 0) {
    // Try to reconnect, and check that we are still subscribed.
    // You need to name your device as something other than LCDarduino, especially
    // if you are on  my server.
    if (client.connect(clientID)) {
        Serial.println("connect");
      client.publish(topicData, buffer); 
    } else 
      Serial.println("disconnected");
  } else {
    client.publish(topicData, buffer);
    Serial.println("sent");
  }
  
  client.loop(); // keep the connection alive ??
  delay(1000);
}

