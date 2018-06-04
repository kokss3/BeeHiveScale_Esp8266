#define TINY_GSM_MODEM_SIM800
//#define TINY_GSM_MODEM_SIM900
//#define TINY_GSM_MODEM_A6
//#define TINY_GSM_MODEM_A7

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

// Increase RX buffer
#define TINY_GSM_RX_BUFFER 600

// or Software Serial on Uno, Nano
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(4,5); // RX, TX

// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn_tmobile[]  = "internet.ht.hr"; //t-mobile
const char apn_tele2[] = "internet.tele2.hr"; //tele2
const char apn_vip[] = "gprs0.vipnet.hr";     //vip
const char apn_tomato[] = "tomato";           //tomato
const char user[] = "";
const char pass[] = "";

// Name of the server we want to connect to
const char server[] = "dweet.io";
const int  port     = 80;

// Path to download (this is the bit after the hostname in the URL)
String resource = "/dweet/for/testis2?";

String forSending = "";
String deviceInfo;
bool isOk = 0;

extern "C" {
#include <espnow.h>
#include <user_interface.h>
}

#define minutes 2650     //expressed in seconds 2650 for 60 mins
unsigned long startTime;
unsigned long millisPassed;
bool isOK;
 uint8_t mac[6] {0x68, 0xC6, 0x3A, 0xCA, 0x82, 0x76}; 
//prepare GSM
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
HttpClient http(client, server, port);

void setup() {

  startTime = millis();
  // Set console baud rate
  Serial.begin(115200);    
  Serial.println("Serial started");

  // Set GSM module baud rate
  SerialAT.begin(9600);
  Serial.println("Modem started");
  wifi_set_macaddr(0, const_cast<uint8*>(mac));
  delay(100);

  WiFi.mode(WIFI_AP_STA); // Station mode for esp-now sensor node
  WiFi.disconnect();
  Serial.print("This mac:  ");
  Serial.println(WiFi.macAddress().c_str());

  //initialize espNow
  if (esp_now_init() != 0) {
    Serial.println("*** ESP_Now init failed");
  }
}

void loop() {
  if (isOK) {
    forSending = resource;
    forSending += deviceInfo;

    if (forSending != "" && forSending.length() > 0 && forSending.charAt(forSending.length() - 1) == '&') {
      forSending = forSending.substring(0, forSending.length() - 1);
    }

    Serial.print("Preparing for sending! ");
    Serial.println(forSending);
    sendToNet(forSending);
    forSending = "";
    deviceInfo = "";
    isOK = false;
  }
  espInitNow();

  millisPassed += (millis() - startTime);
  startTime = millis();

  if (millisPassed > minutes * 1000) {
    millisPassed = 0;
    isOK = true;
  } else isOK = false;
}

void espInitNow() {
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_recv_cb([](uint8_t *mac, uint8_t *sendStatus, uint8_t len) {

    //id,battery,mass
    deviceInfo += "UNIT_";
    deviceInfo += String(sendStatus[0]);
    deviceInfo += "=";

    deviceInfo += String(sendStatus[1]);
    deviceInfo += ";";

    //final line
    deviceInfo += String(sendStatus[2]);
    deviceInfo += "&";
    Serial.print("Got value: ");
    Serial.println(deviceInfo);
  });
}

void sendToNet (String values) {
  esp_now_deinit();
  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  modem.restart();
  delay(2000);

  String modemInfo = modem.getModemInfo();
  Serial.print("Modem: ");
  Serial.println(modemInfo);
  yield();
  Serial.print(F("Waiting for network..."));
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    delay(3000);
    return;
  }
  Serial.println("OK");

  //treba unjeti uvijet za ostale APNove
  Serial.print(F("Connecting to APN "));
  
  if(modem.gprsConnect(apn_tmobile, user, pass)) {
    Serial.println("T-mobile OK");
  }
  //}else if(modem.gprsConnect(apn_tele2, user, pass)) {
  //  http.get(values);
  //  Serial.println("Tele2 OK");
  //if(modem.gprsConnect(apn_vip, user, pass)) {
  //  Serial.println("Vip OK");
  //}
  //if(modem.gprsConnect(apn_tomato, user, pass)) {
  //  Serial.println("Tomato OK");
  //}
  
  Serial.print(F("Performing HTTP GET request... "));
  int err = http.get(values);
  if (err != 0) {
    Serial.println("Failed");
    delay(2500);
    return;
  }
/*  Serial.println("OK");
  int status = http.responseStatusCode();
  Serial.println("Stuck Here 3");
  Serial.println(status);
  if (!status) {
   delay(1000);
    return;
  }
*/  Serial.println("Stuck Here 4");
  http.stop();
  esp_now_init();
  modem.gprsDisconnect();
  Serial.println("GPRS disconnected");
}


