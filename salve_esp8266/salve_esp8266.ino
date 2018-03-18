#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "SPISlave.h"

#define IsDebugMode true

char ssid[] = "*************";
char pass[] = "*************";
char timestamp[33];
char esp8266IP[33];

unsigned int localPort = 2390;

IPAddress timeServerIP;
const char* ntpServerName = "***.***.***.***"; //NTP SERVER NAME OR IP
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[ NTP_PACKET_SIZE];
WiFiUDP udp;

void setup()
{
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  if (IsDebugMode) Serial.print("Connecting to ");
  if (IsDebugMode) Serial.println(ssid);
  
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (IsDebugMode) Serial.print(".");
  }
  
  if (IsDebugMode) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  
  udp.begin(localPort);
  
  SPISlave.onData([](uint8_t * data, size_t len) {
    String message = String((char *)data);
   
    if (message.equals("ESP8266 PRESENT")) {
      SPISlave.setData("TRUE");
      
    }else if (message.equals("ESP8266 IP")) {
      if (isConnected()){
        SPISlave.setData(esp8266IP);
      }else{
        SPISlave.setData("ESP8266 NOT CONNECTED");    
      }
      
    } else if (message.equals("ESP8266 GET TIME")) {
      if (isConnected) {
        if (IsDebugMode) Serial.println(timestamp);
        SPISlave.setData(timestamp);
        
      }else {
        SPISlave.setData("ESP8266 NOT CONNECTED");    
      }
    } else {
      SPISlave.setData("CMD NOT DEFINE");
    }
  });
  SPISlave.begin();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.localIP().toString().toCharArray(esp8266IP, 33);
    WiFi.hostByName(ntpServerName, timeServerIP);
    sendNTPpacket(timeServerIP);
    int cb = udp.parsePacket();
    if (!cb) {
      sprintf(timestamp, "%s", "time request fail");
    }
    else {
      udp.read(packetBuffer, NTP_PACKET_SIZE);
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      const unsigned long seventyYears = 2208988800UL;
      unsigned long epoch = secsSince1900 - seventyYears;
      sprintf(timestamp, "%ld", epoch);
    }
  }
}

boolean isConnected(){
  if (WiFi.status() == WL_CONNECTED)
    return true;
  else
    return false;
}

unsigned long sendNTPpacket(IPAddress& address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  udp.beginPacket(address, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
  delay(1000);
}
