#include <ESP8266WiFi.h>
  extern "C" {
#include <espnow.h>
  }
#include <HX711.h>

//individual scale data
#define ID 1
#define zeroFactor 193572
#define offset 172800

int secondsToSleep = 3600;
//68:C6:3A:CA:82:76
uint8_t remoteMac[] = {0x68, 0xC6, 0x3A, 0xCA, 0x82, 0x76};

HX711 scale(4, 5);  //DT, SCL

void setup(){
  long startTime = millis();
  Serial.begin(115200);
  delay(500);
 
  WiFi.mode(WIFI_STA); // Station mode for esp-now sensor node
  WiFi.disconnect();

  if(getVolt()<184){
    ESP.deepSleep(7890000000000); 
  }

  //gather all data and ready it to be sent
  uint8_t bs[] = {ID,getVolt(),getMass()};

  Serial.print("ID, volt, mass: ");
  Serial.println(bs[0]);
  Serial.println(bs[1]);
  Serial.println(bs[2]);
 
  //send payload via espNOW
  if (esp_now_init() != 0) Serial.println("*** ESP_Now init failed");
  
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_send(remoteMac, bs, sizeof(bs)); 
  
  for (int i = 0; i < sizeof(bs); i++) {
    Serial.println();
    Serial.print(bs[i]);
    Serial.print(" ");
    }
    Serial.println();
  
    ESP.deepSleep(1000*((1000*secondsToSleep)-(millis()-startTime))); 
  }

void loop(){}

//  place is mass in kgs we do not need to change this value
//  but it is just int so resolution is +/- 1kg
uint8_t getMass(){
  scale.power_up();
  int mass;
  long val;  
  float avr;
  
  for (int a = 0; a<10; a++){
  val = scale.read();
   avr += ((val - zeroFactor)/(0.1*offset));
  }
  
  mass = avr/10;
  scale.power_down();
  return mass;
}
  
//  place is voltage in 8bit
//  ie. on ADC 10bit in max range of 1V
//  so 3,9 / 5 = 0,78 and ADC reads 799(0,78*1024)
//  then we map it to 8bit value
//  volt = map(val, 0, 1023, 0, 255);
uint8_t getVolt(){
  int volt = 0;

  for (int i = 0; i<10; i++) volt += map((analogRead(A0)+125),0,1023,0,255);
  volt = volt / 10;
  return volt;
  }
  
