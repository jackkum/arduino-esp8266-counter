#define COUNTER_PIN 5
#define BUZZER_PIN 4
#define AT24C_ADDR 0x57
#define OFFSET 1024
#include "at24c.h"
#include <ESP8266WiFi.h>

extern "C" {
#include "user_interface.h"
}

volatile uint32_t counter = 0;
uint8_t lastState = 0, buzzerState = 0, makeBeep = 0;
char wifi_ssid_private[32] = "foobar";
char wifi_password_private[32] = "";
char clientId[32];
os_timer_t halfSecTimer;

bool ICACHE_FLASH_ATTR at24cWrite(uint16_t addr, char *str, uint8_t len) {
  uint8_t data[32];
  memcpy(data, str, len);

  int8_t w = at24c_writeInPage(addr + OFFSET, data, len, false);
  //Serial.print("W => ");
  //Serial.print(w, DEC);
  //Serial.print(" => ");
  return len == (uint8_t)w;
}

void ICACHE_FLASH_ATTR at24cRead(uint16_t addr, char *dst, uint8_t len) {
  uint8_t data[32];
  at24c_readBytes(addr + OFFSET, data, len);
  
  memcpy(dst, data, len);
}

bool ICACHE_FLASH_ATTR startWPSPBC() {
  Serial.println("WPS config start");
  
  // WPS works in STA (Station mode) only -> not working in WIFI_AP_STA !!! 
  Serial.println("Init WIFI_STA");
  WiFi.mode(WIFI_STA);
  delay(1000);

  Serial.print("Start connecting to: ");
  Serial.print(wifi_ssid_private);
  Serial.print(", with password: ");
  Serial.print(wifi_password_private);
  Serial.println("");

  if(wifi_password_private[0]){
    Serial.println("Begin with password");
    WiFi.begin(wifi_ssid_private, wifi_password_private); // make a failed connection
  } else {
    Serial.println("Begin without password");
    WiFi.begin(wifi_ssid_private); // make a failed connection
  }
  
  Serial.print("Wait status >> ");
  while (WiFi.status() == WL_DISCONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("Connected success");
    return true;
  }

  Serial.println("\nPress WPS button on your router ...");
  Serial.println("Beginf WPS...");
  makeBeep = 1;
  bool wpsSuccess = WiFi.beginWPSConfig();
  if(wpsSuccess) {
      // Well this means not always success :-/ in case of a timeout we have an empty ssid
      String newSSID = WiFi.SSID();
      if(newSSID.length() > 0) {
        makeBeep = 0;
        // WPSConfig has already connected in STA mode successfully to the new station. 
        Serial.printf("WPS finished. Connected successfull to SSID '%s' \n", newSSID.c_str());
        // save to config and use next time or just use - WiFi.begin(WiFi.SSID().c_str(),WiFi.psk().c_str());
        //qConfig.wifiSSID = newSSID;
        //qConfig.wifiPWD = WiFi.psk();
        //saveConfig();

        Serial.println("Save congigurations...");
        WiFi.SSID().getBytes((unsigned char *)wifi_ssid_private, 32);
        WiFi.psk().getBytes((unsigned char *)wifi_password_private, 32);

        Serial.print("SSID: ");
        Serial.print(wifi_ssid_private);
        Serial.print(", pwd: ");
        Serial.print(wifi_password_private);
        Serial.println("");
      
        Serial.print("Writing ssid: ");
        Serial.print(wifi_ssid_private);
        Serial.print(" => ");
        Serial.println(at24cWrite(0, wifi_ssid_private, 32) ? "OK" : "Failed");

        delay(300);
      
        Serial.print("Writing pwd: ");
        Serial.print(wifi_password_private);
        Serial.print(" => ");
        Serial.println(at24cWrite(32, wifi_password_private, 32) ? "OK" : "Failed");
      } else {
        wpsSuccess = false;
      }
  }
  return wpsSuccess; 
}

// start of timerCallback
void timerCallback(void *pArg) {
  if(makeBeep){
    if(buzzerState){
      buzzerState = 0;
      tone(BUZZER_PIN, 800);
    } else {
      buzzerState = 1;
      noTone(BUZZER_PIN);
    }
  }
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  pinMode(COUNTER_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  os_timer_setfn(&halfSecTimer, timerCallback, NULL);
  os_timer_arm(&halfSecTimer, 500, true);
  
  delay(3000);

  Serial.println("Clearing...");
  memset(wifi_ssid_private, 0, sizeof(wifi_ssid_private));
  memset(wifi_password_private, 0, sizeof(wifi_password_private));

  /*
  strcpy(wifi_ssid_private, "WIFI85");
  strcpy(wifi_password_private, "12345678");

  Serial.print("Writing pwd: ");
  Serial.print(wifi_password_private);
  Serial.print(" => ");
  Serial.println(at24cWrite(32, wifi_password_private, 32) ? "OK" : "Failed");

  delay(300);
  
  Serial.print("Writing ssid: ");
  Serial.print(wifi_ssid_private);
  Serial.print(" => ");
  Serial.println(at24cWrite(0, wifi_ssid_private, 32) ? "OK" : "Failed");

  delay(1000);
  */
  
  Serial.println("Reading...");
  at24cRead(0,  wifi_ssid_private, sizeof(wifi_ssid_private));
  at24cRead(32, wifi_password_private, sizeof(wifi_password_private));
  at24cRead(64, clientId, sizeof(clientId));

  eeStrClean(wifi_ssid_private, sizeof(wifi_ssid_private));
  eeStrClean(wifi_password_private, sizeof(wifi_password_private));
  eeStrClean(clientId, sizeof(clientId));

  Serial.print("SSID: ");
  Serial.print(wifi_ssid_private);
  Serial.print(", pwd: ");
  Serial.print(wifi_password_private);
  Serial.print(", id: ");
  Serial.print(clientId);
  Serial.println("");

  WiFi.persistent(false);
  do {
    delay(5000);
  } while( ! startWPSPBC());
}

void loop() {
  // put your main code here, to run repeatedly:

  if(digitalRead(COUNTER_PIN) == HIGH){
    delay(10);
    if(digitalRead(COUNTER_PIN) == HIGH && ! lastState){
      lastState = 1;
      counter++;

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

void eeStrClean(char *str, uint8_t len){
  for(uint8_t i = 0; i < len; i++){
    if(str[i] == 0xFF){
      str[i] = 0x00;
    }
  }
}
