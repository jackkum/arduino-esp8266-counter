#define COUNTER_PIN 5
#define AT24C_ADDR 0x50
#include "at24c.h"
#include <ESP8266WiFi.h>

volatile uint32_t counter = 0;
uint8_t lastState = 0;
char wifi_ssid_private[32] = "foobar";
char wifi_password_private[32] = "";
char clientId[32];

void at24cWrite(uint16_t addr, char *str, uint8_t len) {
  uint8_t data[32];
  memcpy(data, str, len);

  Serial.print("W[");
  Serial.print(len);
  Serial.print("]:");
  for(int i = 0; i < len; i++){
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
  at24c_writeInPage(addr, data, len, true);
}

void at24cRead(uint16_t addr, char *dst, uint8_t len) {
  uint8_t data[32];
  at24c_readBytes(addr, data, len);

  Serial.print("R[");
  Serial.print(len);
  Serial.print("]:");
  for(int i = 0; i < len; i++){
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
  
  memcpy(dst, data, len);
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  pinMode(COUNTER_PIN, INPUT);
  delay(5000);

  Serial.println("Clearing...");
  memset(wifi_ssid_private, 0, sizeof(wifi_ssid_private));
  memset(wifi_password_private, 0, sizeof(wifi_password_private));

  Serial.println("Coping...");
  strcpy(wifi_ssid_private, "str1");
  strcpy(wifi_password_private, "str2");

  Serial.print("SSID: ");
  Serial.print(wifi_ssid_private);
  Serial.print(", pwd: ");
  Serial.print(wifi_password_private);
  Serial.println("");

  Serial.println("Writing to i2c");
  at24cWrite(0, &wifi_ssid_private[0], 5);
  at24cWrite(32, &wifi_password_private[0], 5);

  //Serial.print("Writing... ");
  //memcpy(data, (char *)"HELLO", 5);
  //at24c_writeInPage(0, data, 1, true);

  Serial.println("Waiting...");
  delay(1000);

  Serial.println("Reading...");
  at24cRead(0,  wifi_ssid_private, 32);
  at24cRead(32, wifi_password_private, 32);
  at24cRead(64, clientId, 32);

  Serial.print("SSID: ");
  Serial.print(wifi_ssid_private);
  Serial.print(", pwd: ");
  Serial.print(wifi_password_private);
  Serial.print(", id: ");
  Serial.print(clientId);
  Serial.println("");

  delay(1000);

  //do {
  //  delay(5000);
  //} while( ! startWPSPBC());
}

bool startWPSPBC() {
  Serial.println("WPS config start");

  if( ! wifi_ssid_private[0]){
    Serial.println("Empty eeprom, init default params");
    strcat(wifi_ssid_private, "default");
    strcat(wifi_password_private, "");
  }
  
  // WPS works in STA (Station mode) only -> not working in WIFI_AP_STA !!! 
  Serial.println("Init WIFI_STA");
  WiFi.mode(WIFI_STA);
  delay(1000);

  Serial.print("Start connecting to ");
  Serial.print(wifi_ssid_private);
  Serial.println("");
  
  WiFi.begin(wifi_ssid_private, wifi_password_private); // make a failed connection
  
  Serial.print("Wait status >> ");
  while (WiFi.status() == WL_DISCONNECTED) {
    delay(500);
    Serial.print(".");
  }

  if(WiFi.status() == WL_CONNECTED){
    Serial.println("Connected success");
    return true;
  }

  Serial.println("\nPress WPS button on your router ...");
  Serial.println("Beginf WPS...");
  bool wpsSuccess = WiFi.beginWPSConfig();
  if(wpsSuccess) {
      // Well this means not always success :-/ in case of a timeout we have an empty ssid
      String newSSID = WiFi.SSID();
      if(newSSID.length() > 0) {
        // WPSConfig has already connected in STA mode successfully to the new station. 
        Serial.printf("WPS finished. Connected successfull to SSID '%s'", newSSID.c_str());
        // save to config and use next time or just use - WiFi.begin(WiFi.SSID().c_str(),WiFi.psk().c_str());
        //qConfig.wifiSSID = newSSID;
        //qConfig.wifiPWD = WiFi.psk();
        //saveConfig();

        Serial.println("Wrine new data to eeprome");
        WiFi.SSID().getBytes((unsigned char *)wifi_ssid_private, 32);
        WiFi.psk().getBytes((unsigned char *)wifi_password_private, 32);

        at24c_writeInPage(0, (uint8_t *)wifi_ssid_private, 32, true);
        at24c_writeInPage(32, (uint8_t *)wifi_password_private, 32, true);
        //at24c_writeInPage(64, wifi_ssid_private, 32, true);
  
      } else {
        wpsSuccess = false;
      }
  }
  return wpsSuccess; 
}

void loop() {
  // put your main code here, to run repeatedly:

  if(digitalRead(COUNTER_PIN) == HIGH){
    delay(10);
    if(digitalRead(COUNTER_PIN) == HIGH && ! lastState){
      lastState = 1;

      Serial.println("Pushed");
    }
  }

  if(digitalRead(COUNTER_PIN) == LOW){
    delay(10);
    if(digitalRead(COUNTER_PIN) == LOW && lastState){
      lastState = 0;

      Serial.println("Released");
    }
  }
}

