#include <SPI.h>
#include <Wire.h>

#include <Sodaq_DS3231.h>

#include <mcp_can.h>
#include <mcp_can_dfs.h>



#define DEBUG 0

#define DS3231_I2C_ADDRESS 0x68

#define TIMER_STEP 50

#define START_COUNT 8
#define MSG_COUNT 4

#define HOUR_BUTTON 2
#define MINUTE_BUTTON 3


int timer;

uint32_t start_headers[8][3] = \
{{ 100, 0x50c, 3},
 { 250, 0x3e8, 8},
 {  50, 0x3ef, 8},  
 {  50, 0x3f2, 8},
 { 100, 0x50c, 3},
 {  50, 0x3f2, 8},
 {  50, 0x3f1, 8},
 { 100, 0x50c, 3}};

byte start_data[8][8] = \
{{ 0x0C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x50c
 { 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x3e8
 { 0x32, 0x32, 0x32, 0x32, 0x03, 0x00, 0x00, 0x00},  // 0x3ef
 { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x60, 0x00, 0x00},  // 0x3f2
 { 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x50c
 { 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00},  // 0x3f2
 { 0xF5, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x3f1
 { 0x11, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}; // 0x50c: 1st byte 11 or 01

uint32_t msg_headers[4][4] = \
{{   0,  500, 0x50c, 3},  
 { 400, 1000, 0x3e8, 8},
 { 450, 1000, 0x3ef, 8},
 { 500, 1000, 0x3f2, 8}};

byte msg_data[4][8] = \
{{ 0x11, 0x02, 0x00, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE},   // 0x50c
 { 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00},   // 0x3e8: 1st byte 01 or 00, 4th byte 04 or 00
 { 0x32, 0x32, 0x32, 0x32, 0x03, 0x00, 0x00, 0x20},   // 0x3ef
 { 0x12, 0x01, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00}};  // 0x3f2

const int SPI_CS_PIN = 10;

MCP_CAN CAN(SPI_CS_PIN); 

byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}
void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

void setup() {           

  if (DEBUG) {
    Serial.begin(115200);
  }
  Wire.begin();
  
  pinMode(HOUR_BUTTON, INPUT); 
  pinMode(MINUTE_BUTTON, INPUT); 

START_INIT:
if(CAN_OK == CAN.begin(CAN_125KBPS, MCP_8MHz))
    {
        Serial.println("CAN BUS Shield init ok!");
    }
    else
    {
        Serial.println("CAN BUS Shield init fail");
        Serial.println("Init CAN BUS Shield again");
        delay(100);
        goto START_INIT;
    }
    
  delay(5000);
  timer = 0; 

  for (int i=0;i<START_COUNT;i++) {
    CAN.sendMsgBuf(start_headers[i][1], 0, start_headers[i][2], start_data[i]);  
    
    if (DEBUG) {
      Serial.print("Time: ");
      Serial.print(timer);
      Serial.print("\t");
      Serial.print(start_headers[i][1], HEX);
      Serial.print(" ");
      for (int j=0;j<start_headers[i][2];j++) {
        Serial.print(start_data[i][j], HEX);
        Serial.print(" ");
      }
      Serial.println("");
    }
    delay(start_headers[i][0]);
    timer = timer + start_headers[i][0];
  }
  Serial.println("****");
  timer = 0;
  delay(1000);
}

void loop() {
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

  int hourButtonState = 0;
  int minButtonState = 0;


 /*
  if (timer % 250 == 0) {
    minButtonState = digitalRead(MINUTE_BUTTON);
    hourButtonState = digitalRead(HOUR_BUTTON);

    if (hourButtonState == HIGH) {
      readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
      hour = (hour+1) % 24;
      if (DEBUG) {
        Serial.print("New hour: ");
        Serial.println(hour);
      }
      setDS3231time(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
    }

    if (minButtonState == HIGH) {
      readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
      minute = (minute+1) % 60;
      if (DEBUG) {
        Serial.print("New minute: ");
        Serial.println(minute);
      }
      setDS3231time(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
    }
  }
  */
  for (int i=0;i<MSG_COUNT;i++) {
      if ( ((timer % msg_headers[i][1]) - msg_headers[i][0]) == 0) {
        if (msg_headers[i][2] == 0x3f2) {
          readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
          msg_data[i][0] = minute;
          msg_data[i][1] = second;
        }

        CAN.sendMsgBuf(msg_headers[i][2], 0, msg_headers[i][3], msg_data[i]);  

        if (DEBUG) {
          Serial.print("Time: ");
          Serial.print(timer);
          Serial.print("\t");
          Serial.print(msg_headers[i][2], HEX);
          Serial.print(" ");
          for (int j=0;j<msg_headers[i][3];j++) {
            Serial.print(msg_data[i][j], HEX);
            Serial.print(" ");
          }
          Serial.println("");
        }
      }
    }
  delay(TIMER_STEP);
  timer = timer + TIMER_STEP;
  if (timer == 32000) timer = 0;
}



