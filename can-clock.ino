#include <SPI.h>

#if defined(ESP8266)
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif

#include <RtcDateTime.h>
#include <RtcDS3231.h>
#include <RtcUtility.h>

#include <mcp_can.h>
#include <mcp_can_dfs.h>

#include <Wire.h>

#define DEBUG 0

#define TIMER_STEP 50

#define START_COUNT 8
#define MSG_COUNT 4
#define TEXT_COUNT 12

#define HOUR_BUTTON 3
#define MINUTE_BUTTON 4

RtcDS3231 Rtc;

int timer;

uint32_t start_headers[8][3] = \
{{ 100, 0x50c, 3},
 { 250, 0x3e8, 8},
 {  50, 0x3ef, 8},  
 {  50, 0x3f2, 8},
 { 100, 0x50c, 3},
 {  50, 0x3f2, 8},
 {  50, 0x3f1, 8},
 { 100, 0x50c, 3}
};

byte start_data[8][8] = \
{{ 0x0C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x50c
 { 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x3e8
 { 0x32, 0x32, 0x32, 0x32, 0x03, 0x00, 0x00, 0x00},  // 0x3ef
 { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x60, 0x00, 0x00},  // 0x3f2
 { 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x50c
 { 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00},  // 0x3f2
 { 0xF5, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x3f1
 { 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}   // 0x50c: 1st byte 11 or 01
};

uint32_t msg_headers[4][4] = \
{{   0,  500, 0x50c, 3},  
 { 400, 1000, 0x3e8, 8},
 { 450, 1000, 0x3ef, 8},
 { 500, 1000, 0x3f2, 8},
};

byte msg_data[4][8] = \
{{ 0x11, 0x02, 0x00, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE},   // 0x50c
 { 0x0F, 0x00, 0x29, 0x04, 0x00, 0x00, 0x00, 0x00},   // 0x3e8: 1st byte 01 or 00, 4th byte 04 or 00; 
 //1st byte 01 - AM 02 - FM1 03 - FM2 04 - PHON 05 - SYNC 06 - DVD 07 - AUX 08 - CD 09 - EMPTY 0A - SAT1 0B - SAT2 0C - SAT3 0D - PHON OE - LINE 0F - 2 clocks
 //3th byte volume
 //4th byte - clock length?
 { 0x32, 0x32, 0x32, 0x32, 0x03, 0x00, 0x00, 0x20},   // 0x3ef
 { 0x12, 0x01, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00},   // 0x3f2
};

uint32_t text_headers[12][4] = \
{
 {   0, 0xDE, 0x336, 8},
 {  60, 0xDE, 0x324, 8},
 {  60, 0xDE, 0x337, 8},
 { 100, 0xDE, 0x336, 8},
 {  20, 0xDE, 0x324, 8},
 {  20, 0xDE, 0x337, 8},
 {  20, 0xDE, 0x337, 8},
 {  20, 0xDE, 0x337, 8},
 {  20, 0xDE, 0x337, 8},
 {  20, 0xDE, 0x337, 8},
 {  20, 0xDE, 0x337, 8},
 {  20, 0xDE, 0x337, 8}
};

byte text_data[12][8] = \
{
  { 0x03, 0x01, 0x0A, 0x01, 0xFE, 0x00, 0x00, 0x00 },
  { 0x01, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x06, 0x20, 0x20, 0x40, 0x20, 0x40, 0x20, 0x00 },
  { 0x03, 0x01, 0x05, 0x03, 0x03, 0x00, 0x00, 0x00 },
  { 0x03, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x10, 0x2B, 0x20, 0x20, 0x20, 0x40, 0x20, 0x20 },
  { 0x21, 0x20, 0x4D, 0x65, 0x72, 0x63, 0x40, 0x75 },
  { 0x22, 0x72, 0x79, 0x20, 0x20, 0x20, 0x20, 0x20 },
  { 0x23, 0x20, 0x20, 0x20, 0x20, 0x20, 0x40, 0x20 },
  { 0x24, 0x20, 0x20, 0x4d, 0x61, 0x72, 0x69, 0x6e },
  { 0x25, 0x65, 0x72, 0x20, 0x20, 0x20, 0x20, 0x20 },
  { 0x26, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

const int SPI_CS_PIN = 10;

unsigned char rcvFlag = 0;
unsigned char rcvLen = 0;
unsigned long rcvCanId = 0x0;
unsigned char rcvBuf[8];

MCP_CAN CAN(SPI_CS_PIN); 

void MCP2515_ISR()
{
    rcvFlag = 1;
}

byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void setDS3231time(uint8_t second, uint8_t minute, uint8_t hour)
{
  char strTime[8];
  String time = String(hour) + ":" + String(minute) + ":" + String(second);
  time.toCharArray(strTime, 8);
  RtcDateTime dt = RtcDateTime(__DATE__, strTime);
  Rtc.SetDateTime(dt);
}

void readDS3231time(uint8_t *second,
uint8_t *minute,
uint8_t *hour
)
{
  RtcDateTime now = Rtc.GetDateTime();
  *second = now.Second();
  *minute = now.Minute();
  *hour = now.Hour();
}

void printDebug(int timer, uint32_t headers[], byte data[])
{
  if (DEBUG) {
    Serial.print("Time: ");
    Serial.print(timer);
    Serial.print(" ");
    Serial.print(headers[2], HEX);
    Serial.print(": ");
    for (int j=0;j<headers[3];j++) {
      Serial.print(data[j], HEX);
      Serial.print(" ");
    }
    Serial.println("");
  }
}

void displayText(int strNo, String str)
{
  byte curLine, curChar, numChars;

  switch (strNo) {
    case 0:
      curLine = 2;
      curChar = 2;
      numChars = 3;
      break;
    case 1:
      curLine = 5;
      curChar = 2;
      numChars = 20;
      break;
    case 2:
      curLine = 8;
      curChar = 3;
      numChars = 20;
      break;
  }

  for (byte i=0;i<numChars;i++) {
    if (
          ( curLine == 2 && curChar == 3 ) ||
          ( curLine == 2 && curChar == 5 ) ||
          ( curLine == 5 && curChar == 5 ) ||
          ( curLine == 6 && curChar == 6 ) ||
          ( curLine == 8 && curChar == 6 )
       ) {
      text_data[curLine][curChar] = '@';
      curChar++;
    }

    text_data[curLine][curChar] = i<str.length() ? str[i]:' ';

    curChar++;

    if ( curChar == 8 ) {
      curLine++;
      curChar = 1;
    }
  }
}


void setup() {           

  Serial.begin(115200);
  Rtc.Begin();
#if defined(ESP8266)
  Wire.begin(0, 2);
#endif

  pinMode(HOUR_BUTTON, INPUT); 
  pinMode(MINUTE_BUTTON, INPUT); 
  pinMode(HOUR_BUTTON, INPUT); 
  pinMode(MINUTE_BUTTON, INPUT); 
  digitalWrite(HOUR_BUTTON, HIGH); 
  digitalWrite(MINUTE_BUTTON, HIGH); 

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

  attachInterrupt(0, MCP2515_ISR, FALLING); // start interrupt



  CAN.init_Mask(0, 0, 0x7FF << 18);                         // there are 2 mask in mcp2515, you need to set both of them
  CAN.init_Mask(1, 0, 0x7FF << 18);
  CAN.init_Filt(0, 0, 0x3B5 << 18);   // TPMS data
  CAN.init_Filt(1, 0, 0x423 << 18);   // Speed data
  //CAN.init_Filt(0, 0, 0x2db << 18);   // SYNC buttons
  //CAN.init_Filt(1, 0, 0x398 << 18);   // HVAC

  delay(5000);
  timer = 0; 

  for (int i=0;i<START_COUNT;i++) {
    CAN.sendMsgBuf(start_headers[i][1], 0, start_headers[i][2], start_data[i]);  

    //printDebug(timer, start_headers[i], start_data[i]);

    delay(start_headers[i][0]);
    timer = timer + start_headers[i][0];
  }
  Serial.println("****");
  timer = 0;
  delay(1000);
}

void loop() {
  byte second, minute, hour;

  int hourButtonState = 0;
  int minButtonState = 0;

  String inSerialData;
  if (rcvFlag == 1) {
    rcvFlag = 0;
    while (CAN_MSGAVAIL == CAN.checkReceive()) {

      CAN.readMsgBuf(&rcvLen, rcvBuf);
      rcvCanId = CAN.getCanId();
      switch (rcvCanId) {
        case 0x3b5: {
            String fl = String(rcvBuf[0]);
            String fr = String(rcvBuf[1]);
            String rr = String(rcvBuf[2]);
            String rl = String(rcvBuf[3]);
            displayText(1, "L:" + fl + " R:" + fr + " L:" + rl + " R:" + rr);
        }
          break;
        case 0x423: {
          byte rpm1 = rcvBuf[2];
          byte rpm2 = rcvBuf[3];
          String rpm = String(( ( rpm1 << 8 ) + rpm2 ) / 4);
          
          byte speed1 = rcvBuf[0];
          byte speed2 = rcvBuf[1];
          String speed = String(round((( speed1 << 8) + speed2)/100) - 100);
          if (speed.length() == 1) {
            speed = "  "+speed;
          } else if (speed.length() == 2) {
            speed = " "+speed;
          }
          displayText(0,speed);
          displayText(2,"RPM:" + rpm);
          
        }
          break;
        case 0x398: {
          String b0 = String(rcvBuf[0], HEX);
          String b1 = String(rcvBuf[1], HEX);
          String b2 = String(rcvBuf[2], HEX);
          String b3 = String(rcvBuf[3], HEX);
          String b4 = String(rcvBuf[4], HEX);

          displayText(2, "HVAC " + b0 + " " + b1 + " " + b2 + " " + b3 + " " + b4);
        }
          break;
      }
    }
  }
  if (timer % 250 == 0) {
    minButtonState = digitalRead(MINUTE_BUTTON);
    hourButtonState = digitalRead(HOUR_BUTTON);

    if (hourButtonState != HIGH) {
      readDS3231time(&second, &minute, &hour);
      hour = (hour+1) % 24;
      if (DEBUG) {
        Serial.print("New hour: ");
        Serial.println(decToBcd(hour), HEX);
      }
      setDS3231time(second, minute, hour);
    }

    if (minButtonState != HIGH) {
      readDS3231time(&second, &minute, &hour);
      minute = (minute+1) % 60;
      if (DEBUG) {
        Serial.print("New minute: ");
        Serial.println(decToBcd(minute), HEX);
      }
      setDS3231time(second, minute, hour);
    }
  }
  
  for (int i=0;i<MSG_COUNT;i++) {
    if ( ((timer % msg_headers[i][1]) - msg_headers[i][0]) == 0) {
      if (msg_headers[i][2] == 0x3f2) {
        readDS3231time(&second, &minute, &hour);
        msg_data[i][0] = decToBcd(hour);
        msg_data[i][1] = decToBcd(minute);
      }

      CAN.sendMsgBuf(msg_headers[i][2], 0, msg_headers[i][3], msg_data[i]);  

      printDebug(timer, msg_headers[i], msg_data[i]);
    }
  }

  inSerialData = "";

  while (Serial.available() > 0) {
      char recieved = Serial.read();
      inSerialData += recieved; 

      if (recieved == '\n')
      {
        displayText(inSerialData.substring(0,1).toInt(), inSerialData.substring(1));
        inSerialData = "";
      }
  }

  if ( (timer % 500) == 0) {
    for (int i=0;i<TEXT_COUNT;i++) {
      CAN.sendMsgBuf(text_headers[i][2], 0, text_headers[i][3], text_data[i]);

      printDebug(timer, text_headers[i], text_data[i]);

      delay(text_headers[i][0]);
    }
  }

  delay(TIMER_STEP);
  timer = timer + TIMER_STEP;
  if (timer == 32000) timer = 0;
}



