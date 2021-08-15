/* ---------------------------------

Date:20210815
Note:
設備相關請看RENAME.md
以下程式為G5T(內涵PM1.0、PM2.5、PM10.0、溫溼度)的感測功能。

 -----------------------------------*/
#include <SoftwareSerial.h>

SoftwareSerial myMerial(3, 2);
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
  delay(3000); 
}

void loop() {
  retrievepm25();
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
  Serial.println(pmat10);
  delay(1000); 
  Serial.println(pmat25);
  delay(1000); 
  Serial.println(pmat100);
  delay(1000); 
  Serial.println(Temp);
  delay(1000); 
}
