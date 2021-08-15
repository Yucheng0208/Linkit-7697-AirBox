#include <SoftwareSerial.h>
#include "Wire.h"
#include "U8g2lib.h"
#include <LRTC.h>
#include <WiFiUdp.h>
#include <ctime>
#include <LWiFi.h>
#include "LTimer.h"
#include "LWatchDog.h"

#define TIMER0_INTERVAL (30e3) // timer0觸發時間，每觸發一次reboot_counter+1

char _lwifi_ssid[] = ""; //填入Wi-Fi名稱
char _lwifi_pass[] = ""; //填入Wi-Fi密碼

SoftwareSerial myMerial(3, 2);
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);
LTimer timer0(LTIMER_0);      //timer0 進行初始化


long pmat10 = 0;
long pmat25 = 0;
long pmat100 = 0;
long Temp = 0;
long Humid = 0;
char buf[50];
short reboot_counter = 0;     //counter，因為timer0 0.5秒觸發一次counter++，counter因此當counter>=*interval，則reboot
short reboot_interval = 1;    //多久(min)重啟

//----------------------------------
void set_rtc_from_time_string(const String& time_str) {
  // field_index [0,1,2,3,4,5] = [Year,Month,Day,Hour,Minute,Sec]
  int fields[6] = {0};
  sscanf(time_str.c_str(), "%d-%d-%d %d:%d:%d",
          &fields[0], &fields[1], &fields[2],
          &fields[3], &fields[4], &fields[5]);
  LRTC.set(fields[0], fields[1], fields[2],
           fields[3], fields[4], fields[5]);
}

const char *NTP_server = "time.stdtime.gov.tw";
const int NTP_PACKET_SIZE = 48;                   // NTP time stamp is in the first 48 bytes of the message
static byte packetBuffer[NTP_PACKET_SIZE] = {0};  //buffer to hold incoming and outgoing packets
const unsigned int localPort = 2390;              // local port to listen for UDP packets
static WiFiUDP Udp;                               // A UDP instance to let us send and receive packets over UDP

String getNetworkTime() {
  Udp.begin(localPort);
  sendNTPpacket(NTP_server); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  if (Udp.parsePacket()) {
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    const unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    const unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    const unsigned long secsSince1900 = highWord << 16 | lowWord;
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    const unsigned long epoch = secsSince1900 - seventyYears;
    // Taiwan is UTC+8 = 8 * 60 * 60 seconds
    const time_t taiwan_time = epoch + (8 * 60 * 60);
    // const tm* pTime = gmtime(&taiwan_time);
    static char time_text[] = "YYYY-MM-DD HH:MM:SS";
    strftime(time_text, sizeof(time_text), "%Y-%m-%d %H:%M:%S", gmtime(&taiwan_time));
    return String((const char*)time_text);
  }

  return String("Connection error");
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(const char* host) {
  //Serial.println("1");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  //Serial.println("2");
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  //Serial.println("3");

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(host, 123); //NTP requests are to port 123
  //Serial.println("4");
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  //Serial.println("5");
  Udp.endPacket();
  //Serial.println("6");

  return 0;
}

String get_time_from_RTC() {
  // get time from the RTC module
  LRTC.get();

  // format to time string
  static char buffer[] = "YYYY-MM-DD HH:MM:SS";
  sprintf(buffer, "%04ld-%02ld-%02ld %02ld:%02ld:%02ld",
    LRTC.year(),
    LRTC.month(),
    LRTC.day(),
    LRTC.hour(),
    LRTC.minute(),
    LRTC.second());

  return String(buffer);
}


//----------------------------------


void setup() {
  LRTC.begin();
  while (WiFi.begin(_lwifi_ssid, _lwifi_pass) != WL_CONNECTED) { delay(1000); }
  set_rtc_from_time_string(getNetworkTime());
  Serial.begin(9600);
  myMerial.begin(9600);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0,10,"Wait...");
  //u8g2.drawStr(0,25,(String(get_time_from_RTC())).c_str());
  u8g2.sendBuffer();
  
  WiFi.begin(_lwifi_ssid, _lwifi_pass);
  delay(3000); 
  LWatchDog.begin(60); //啟動看門狗，單位：秒
  timer0.begin();
  timer0.start(TIMER0_INTERVAL, LTIMER_ONESHOT_MODE, ISR_TIMER0, NULL);
}

void loop() {
  LWatchDog.feed();  //呼叫看門狗函式庫，清除
  retrievepm25();
  
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0,10,"LinkIt AirBox");
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

/*TIMER0中斷副程式*/
void ISR_TIMER0(void *usr_data) {
  /* 每0.5秒會中斷觸發這個副程式一次，使得reboot_counter++。
   * 若reboot_counter >= (reboot_interval*2)，
   * 則reboot設為TRUE，該reboot旗標告訴loop不可將餵食看門狗(WDT)
   * 並設定看門狗1秒後重啟板子(reboot)
  */
  reboot_counter++;
  if ( (reboot_counter >= (reboot_interval*2))) {
    LWatchDog.end();
    LWatchDog.begin(1);
  } else {
    timer0.start(TIMER0_INTERVAL, LTIMER_ONESHOT_MODE, ISR_TIMER0, NULL);
  }
}
