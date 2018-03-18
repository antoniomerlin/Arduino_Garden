#include <SPI.h>
#include <time.h>
#include <LiquidCrystal595.h>
#include <NewPing.h>
#include <RTClib.h>

LiquidCrystal595 lcd(2, 3, 4);

#define IsDebugMode false

#define RELAY_TRIGGER_PIN 9
#define TRIGGER_PIN 6
#define ECHO_PIN  5
#define MAX_DISTANCE 200
#define sprinkler_manual_btn 8

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

const char *monthName[12] = {
  "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
  "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

#define Display_Clock_Every_N_Seconds 10
#define Display_ShortHelp_Every_N_Seconds 60
String tz,esp8266_ip;
int hours = 0;
int minutes = 0;
int seconds = 0;
int dates = 0;
int months = 0;
int years = 0;
int ap = 0;
int sprinklerCnt = 0;
int waterlevel = 0;
boolean IsSprinklerEnable = false;

RTC_DS1307 rtc;

class ESPSafeMaster
{
  private:
    uint8_t _ss_pin;
    void _pulseSS()
    {
      digitalWrite(_ss_pin, HIGH);
      delayMicroseconds(5);
      digitalWrite(_ss_pin, LOW);
    }
  public:
    ESPSafeMaster(uint8_t pin): _ss_pin(pin) {}
    void begin()
    {
      pinMode(_ss_pin, OUTPUT);
      _pulseSS();
    }

    uint32_t readStatus()
    {
      _pulseSS();
      SPI.transfer(0x04);
      uint32_t status = (SPI.transfer(0) | ((uint32_t)(SPI.transfer(0)) << 8) | ((uint32_t)(SPI.transfer(0)) << 16) | ((uint32_t)(SPI.transfer(0)) << 24));
      _pulseSS();
      return status;
    }

    void writeStatus(uint32_t status)
    {
      _pulseSS();
      SPI.transfer(0x01);
      SPI.transfer(status & 0xFF);
      SPI.transfer((status >> 8) & 0xFF);
      SPI.transfer((status >> 16) & 0xFF);
      SPI.transfer((status >> 24) & 0xFF);
      _pulseSS();
    }

    void readData(uint8_t * data)
    {
      _pulseSS();
      SPI.transfer(0x03);
      SPI.transfer(0x00);
      for (uint8_t i = 0; i < 32; i++) {
        data[i] = SPI.transfer(0);
      }
      _pulseSS();
    }

    void writeData(uint8_t * data, size_t len)
    {
      uint8_t i = 0;
      _pulseSS();
      SPI.transfer(0x02);
      SPI.transfer(0x00);
      while (len-- && i < 32) {
        SPI.transfer(data[i++]);
      }
      while (i++ < 32) {
        SPI.transfer(0);
      }
      _pulseSS();
    }

    String readData()
    {
      char data[33];
      data[32] = 0;
      readData((uint8_t *)data);
      return String(data);
    }

    void writeData(const char * data)
    {
      writeData((uint8_t *)data, strlen(data));
    }
};

ESPSafeMaster esp(SS);

void send(const char * message)
{
  if (IsDebugMode) Serial.print("Master: ");
  if (IsDebugMode) Serial.println(message);
  esp.writeData(message);
  delay(10);
  if (IsDebugMode) Serial.print("Slave: ");
  String slave_op = esp.readData();
  if (IsDebugMode) Serial.println(slave_op);
  if (message == "ESP8266 IP") {
    esp8266_ip = slave_op;
  } else if (slave_op.length() > 0 && slave_op.toInt() != 0) {
    time_t epoch = slave_op.toInt() + 19800;
  }
}

void setup() {

  pinMode(RELAY_TRIGGER_PIN, OUTPUT);
  digitalWrite(RELAY_TRIGGER_PIN, LOW);
  pinMode(sprinkler_manual_btn, INPUT);
  attachInterrupt(0, start_sprinkler_man, CHANGE);

  lcd.begin(16, 2);

  Serial.begin(115200);
  SPI.begin();
  esp.begin();
  while (!Serial);

  delay(200);

  scroll("WELLCOME TO ARDUINO GRADEN 1.1", 5); if (IsDebugMode) Serial.println("WELLCOME TO ARDUINO GRADEN 1.1");
  scroll("PERFORMING SYSTEM CHECK...", 5);  if (IsDebugMode) Serial.println("PERFORMING SYSTEM CHECK...");

  pinMode(A5, OUTPUT);
  digitalWrite(A5, HIGH);
  pinMode(A4, OUTPUT);
  digitalWrite(A4, LOW);
  // Check for attached module (RTC, ESP8266, LCD, SONAR)
  if (! rtc.begin()) {
    lcd.print("Couldn't find RTC");
    while (1);
  }
  if (! rtc.isrunning()) {
    lcd.print("RTC is NOT running!");
  }

}

void loop() {
  lcd.clear();
  lcd.home();

  waterlevel = sonar.ping_cm();

  if (IsDebugMode) Serial.println(waterlevel);
  lcd.print("water level:");
  lcd.print(waterlevel);
  delay(2000);
  lcd.clear();

  if ((waterlevel >= 15) && (waterlevel <= 20) && (waterlevel > 0)) {
    lcd.home();
    lcd.print("WATER LEVEL: LOW");
    lcd.setCursor(0, 1);
    lcd.print("PLEASE FILL BUCKET");
  } else if ((waterlevel > 20) && (waterlevel > 0)) {
    lcd.clear();
    lcd.home();
    lcd.print("WATER LEVEL: EMPTY");
    lcd.setCursor(0, 1);
    lcd.print("PLEASE FILL BUCKET");
  } else if ((waterlevel > 0) && (waterlevel <= 15)) {
    lcd.clear();
    lcd.home();
    lcd.print("WATER LEVEL:FULL");
  }

  delay(5000);
  lcd.clear();

  DateTime now = rtc.now();
  Serial.println(now.hour());

  hours = now.hour();
  minutes = now.minute();
  seconds = now.second();
  dates = now.day();
  months = now.month();
  years = now.year();

  if (hours < 12)
    tz = "AM";
  else
    tz = "PM";

  lcd.home();
  lcd.print("TIME");
  lcd.setCursor(0, 1);
  lcd.print(hours);
  lcd.print(":");
  lcd.print(minutes);
  lcd.print(":");
  lcd.print(seconds);
  lcd.print(" ");
  lcd.print(tz);
  delay(5000);
  lcd.clear();

  lcd.home();
  lcd.print("DATE");
  lcd.setCursor(0, 1);
  lcd.print(dates);
  lcd.print(" ");
  lcd.print(monthName[months - 1]);
  lcd.print(" ");
  lcd.print(years);
  delay(5000);
  lcd.clear();

  if ((hours == 7) && (minutes == 10) && (!IsSprinklerEnable)) {
    if ((waterlevel <= 20) && (waterlevel > 0)) {
      digitalWrite(9, HIGH);
      IsSprinklerEnable = true;
    }
  }

  if (IsSprinklerEnable) {
    scroll("SPRINKLER IS ON", 5);
    sprinklerCnt++;
    if ((waterlevel > 20) && (waterlevel > 0)) {
      scroll("TURNING SPRINKLER OFF", 0);
      digitalWrite(9, LOW);
      IsSprinklerEnable = false;
      sprinklerCnt = 0;
    }
  } else {
    scroll("SPRINKLER IS OFF", 5);
  }

  if (IsSprinklerEnable && sprinklerCnt == 6) {
    scroll("TURNING OFF SPRINKLER", 0);
    digitalWrite(9, LOW);
    IsSprinklerEnable = false;
    sprinklerCnt = 0;
  }

  send("ESP8266 IP");
  esp8266_ip != "" ? scroll("ESP8266 IP:"+ esp8266_ip ,5) : scroll("ESP8266 NOT CONNECTED",5);
  delay(2000);
  send("ESP8266 GET TIME");
}

void start_sprinkler_man() {
  if (digitalRead(sprinkler_manual_btn)) {
    if ((waterlevel <= 20) && (waterlevel > 0)) {
      digitalWrite(9, HIGH);
      IsSprinklerEnable = true;
    } else {
      scroll("SPRINKLER CANNOT START WATER LEVEL LOW", 5);
      delay(5000);
    }
  }
}

void scroll(String text, int delaySeconds) {
  int Length = text.length();
  String sample = text;
  String firstLine = text;
  String secondLine = ""; //Second line is blank unless the message is over 16 characters
  if (Length > 16) {
    secondLine = text;
    sample.remove(16); //"sample" is the message, except cut off at 16 characters.
    int firstLineCharCount  = sample.lastIndexOf(" "); //Sees where the last complete word was
    firstLine.remove(firstLineCharCount);  //Omits any imcomplete words when the first line is limited to 16 characters.
    secondLine.remove(0, firstLineCharCount + 1); //Omits the part of the message that is already in the first line (plus the space)
  }
  Write(firstLine, secondLine);
  delay(delaySeconds * 1000);
}

bool writing = false;
String scribe;
void Write(String firstLine, String secondLine) {
  lcd.clear();
  for (int line = 0; line < 2; line++) {
    if (line == 0) {
      scribe = firstLine; //When 0, loop is setting first line. When 1, loop is setting second line.
    } else {
      scribe = secondLine;
    }
    int scribeLength = scribe.length();
    if (writing == false) { //Won't begin writing till current write is done
      writing = true; //Tells the code it's already writing so that it doesn't write 2 things at once
      for (int i = 0; i < scribeLength; i++) {
        if (i > 16) {
          lcd.scrollDisplayLeft();
        }
        lcd.setCursor(i, line);
        lcd.write(scribe.charAt(i));
        delay(50);
      }
      writing = false;
    }
    delay(10);
  }
}
