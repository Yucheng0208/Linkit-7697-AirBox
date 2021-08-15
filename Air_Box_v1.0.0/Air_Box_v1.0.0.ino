#include <SoftwareSerial.h>
#include "Wire.h"
#include "U8g2lib.h"


SoftwareSerial myMerial(3, 2);
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

//========================================================================
#include <LWiFi.h>

char _lwifi_ssid[] = ""; //填入Wi-Fi名稱
char _lwifi_pass[] = ""; //填入Wi-Fi密碼
//========================================================================

long pmat10 = 0;
long pmat25 = 0;
long pmat100 = 0;
long Temp = 0;
long Humid = 0;
char buf[50];

void setup() {

  Serial.begin(9600);
  myMerial.begin(9600);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0,10,"Wait...");
  u8g2.sendBuffer();
  
  WiFi.begin(_lwifi_ssid, _lwifi_pass);
  delay(3000); 
}

void loop() {
  retrievepm25();
  
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0,10,"LinkIt AirBox");
  //u8g2.drawStr(0,25,"datetime");
  u8g2.sendBuffer();
  delay(1000);
  u8g2.clearBuffer();
  u8g2.drawStr(0,10,("PM1.0 : " + String(pmat10) + " ug/m3").c_str());
  u8g2.drawStr(0,25,("PM2.5 : " + String(pmat25) + " ug/m3").c_str());
  u8g2.drawStr(0,37,("PM10 : " + String(pmat100) + " ug/m3").c_str());
  u8g2.drawStr(0,49,("Temp : " + String(Temp) + " *C").c_str());
  u8g2.drawStr(0,61,("Humid : " + String(Humid) + " %RH").c_str());
  u8g2.sendBuffer();
  delay(2000);
}


void retrievepm25(){
  int count = 0;
  unsigned char c;
  unsigned char high;
  while (myMerial.available()) { 
    c = myMerial.read();
    if((count==0 && c!=0x42) || (count==1 && c!=0x4d)){
      break;
    }
    if(count > 27){ 
      break;
    }
     else if(count == 10 || count == 12 || count == 14 || count == 24 || count == 26) {
      high = c;
    }
    else if(count == 11){
      pmat10 = 256*high + c;
    }
    else if(count == 13){
      pmat25 = 256*high + c;
    }
    else if(count == 15){
      pmat100 = 256*high + c;
    }
     else if(count == 25){        
      Temp = (256*high + c)/10;
    }
    else if(count == 27){            
      Humid = (256*high + c)/10;
    }   
    count++;
  }
  while(myMerial.available()) myMerial.read();
}
