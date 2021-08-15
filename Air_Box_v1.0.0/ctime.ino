#include <stdarg.h>
#include <LWiFi.h>
#include <LWiFiUdp.h>

#define BAUDRATE 115200

// 請修改成你的WiFi名稱與密碼
#define WIFI_AP "Ryan"
#define WIFI_PASSWD "chang20020208"

// NTP伺服器
#define NTP_SERVER "time.stdtime.gov.tw" // (118, 163, 81, 61)
IPAddress ntpServerIP;
#define ntpServerPort 123

#define LOCAL_PORT 2390

#define NTP_PACKET_SIZE 48

// UDP客戶端
LWiFiUDP udpcli;

// 類似於printf
void pf(const char *fmt, ... ){
    char tmp[128]; // limited to 128 chars
    va_list args;
    va_start(args, fmt);
    vsnprintf(tmp, 128, fmt, args);
    va_end(args);
    Serial.print(tmp);
}

// 根據WiFi模組狀態，回傳容易看懂的字串
const char * const wifi_status_str(LWifiStatus ws){
  switch(ws){
    case LWIFI_STATUS_DISABLED:
      return "WiFi module status: disabled";
    break;
    case LWIFI_STATUS_DISCONNECTED:
      return "WiFi module status: disconnected";
    break;
    case LWIFI_STATUS_CONNECTED:
      return "WiFi module status: connected";
    break;
  }
  return "WiFi status: error, no such status";
}

void setup(){
  Serial.begin(BAUDRATE);
 
  udpcli.begin(LOCAL_PORT); // 綁定某的通訊埠
}

// 送出詢問時間的UDP封包，詳情請看RFC 5905 unsigned long sendNTPpacket(IPAddress &address)
{
  byte buf[NTP_PACKET_SIZE];
  memset(buf, 0, NTP_PACKET_SIZE); // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  buf[0] = 0b11100011;   // LI, Version, Mode
  buf[1] = 0;     // Stratum, or type of clock
  buf[2] = 6;     // Polling Interval
  buf[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  buf[12]  = 49;
  buf[13]  = 0x4E;
  buf[14]  = 49;
  buf[15]  = 52;

  // send a packet requesting a timestamp:
  udpcli.beginPacket(address, ntpServerPort);
  udpcli.write(buf, NTP_PACKET_SIZE);
  udpcli.endPacket();
}

// 拿到秒數（從1900年1月1日 0時0分0秒），印出
// 轉成Unix epoch time，印出
// 捨棄年月日，印出時分秒，HH:MM:SS void printTime(unsigned long secsSince1900){
  pf("Seconds since Jan 1 1900: %u\n", secsSince1900);
 
  unsigned long epoch = secsSince1900 - 2208988800UL; // seventy years
  pf("Unix time: %u\n", epoch); // 00:00:00 1 January 1970, UTC
 
  epoch %= 86400L;
  unsigned long hh = epoch / 3600;
  epoch %= 3600;
  unsigned mm = epoch / 60;
  epoch %= 60;
  unsigned ss = epoch;
  pf("The UTC time: %02u:%02u:%02u\n", hh, mm, ss);
}

// 主迴圈，從序列埠接收控制字元，做該做的事情 void loop(){
  if(Serial.available()){
    char d = Serial.read();
    switch(d){
      case 'l':{ // 搜尋WiFi網路
        int i;
        int n_ap = LWiFi.scanNetworks();
        pf("Number of WiFi AP found: %d\n", n_ap);
        for(i = 0; i < n_ap; i++){
            pf("%d, %s, RSSI: %d\n", i, LWiFi.SSID(i), LWiFi.RSSI(i));
        }
      }
      break;
      case 's':{ // 印出狀態
        LWifiStatus ws = LWiFi.status();
        Serial.println(wifi_status_str(ws));
       
        uint8_t ma[VM_WLAN_WNDRV_MAC_ADDRESS_LEN] = {0};
        LWiFi.macAddress(ma);
        Serial.print("MAC address: ");
        int i;
        for(i = 0; i < VM_WLAN_WNDRV_MAC_ADDRESS_LEN-1; i++){
            pf("%02X:", ma[i]);
        }
        pf("%02X\n", ma[i]);
       
        if(ws == LWIFI_STATUS_CONNECTED){
          IPAddress ipa = LWiFi.localIP();
          Serial.print("localIP: ");
          ipa.printTo(Serial);
          Serial.println("");
         
          Serial.print("subnetMask: ");
          ipa = LWiFi.subnetMask();
          ipa.printTo(Serial);
          Serial.println("");
         
          Serial.print("gatewayIP: ");
          ipa = LWiFi.gatewayIP();
          ipa.printTo(Serial);
          Serial.println("");
        }
      }
      break;
      case 'b':{ // 開啟WiFi模組
        Serial.println("LWiFi.begin() turn on WiFi module");
        LWiFi.begin();
      }
      break;
      case 'e':{ // 關閉WiFi模組
        Serial.println("LWiFi.end() turn off WiFi module");
        LWiFi.end();
      }
      break;
      case 'c':{ // 連接WiFi AP
        Serial.print("Connecting...");
        if(LWiFi.connect(WIFI_AP, LWiFiLoginInfo(LWIFI_WPA, WIFI_PASSWD)) > 0){
          Serial.println(" succeed");
        }
        else{
          Serial.println(" failed");
        }
      }
      break;
      case 'd':{ // 切斷與WiFi AP的連線
        LWiFi.disconnect();
        Serial.println("Disconnected");
      }
      break;
      case 't':{ // 向NTP伺服器詢問時間
        if(ntpServerIP == INADDR_NONE){
          if(LWiFi.hostByName(NTP_SERVER, ntpServerIP) != 1){
            pf("DNS lookup failed: %s", NTP_SERVER);
          }
        }
        if(ntpServerIP != INADDR_NONE){
          pf("Send request to NTP server %s\n", NTP_SERVER);
          sendNTPpacket(ntpServerIP);
        }
      }
      break;
    }
  }
 
  if(udpcli.parsePacket()){ // 收到NTP伺服器傳回來的時間
    byte packetBuffer[NTP_PACKET_SIZE];
    udpcli.read(packetBuffer, NTP_PACKET_SIZE);
    const unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    const unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    const unsigned long secsSince1900 = highWord << 16 | lowWord;
    printTime(secsSince1900);
  }
}
