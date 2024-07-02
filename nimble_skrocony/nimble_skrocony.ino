/*
  Copyright (C) AC SOFTWARE SP. Z O.O.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#define STATUS_LED_GPIO 2
#define BUTTON_CFG_RELAY_GPIO 0
#include <sstream>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>


#include <SuplaDevice.h>
#include <supla/network/esp_wifi.h>
#include <supla/control/button.h>
#include <supla/control/action_trigger.h>
#include <supla/device/status_led.h>
#include <supla/storage/littlefs_config.h>
#include <supla/network/esp_web_server.h>
#include <supla/network/html/device_info.h>
#include <supla/network/html/protocol_parameters.h>
#include <supla/network/html/status_led_parameters.h>
#include <supla/network/html/wifi_parameters.h>
#include <supla/device/supla_ca_cert.h>
#include <supla/sensor/therm_hygro_meter.h>
#include <supla/sensor/thermometer.h>

#define SCAN_TIME  2 // seconds

const char* xiaomiBleDeviceMacs[] = {  //Lista czujników
"A4:C1:38:4D:BF:11",
"A4:C1:38:F8:C7:B4",
"A4:C1:38:C8:C2:F5",
"A4:C1:38:6F:31:E3",
"A4:C1:38:CE:D0:57",
"A4:C1:38:05:05:05",
"A4:C1:38:06:06:06",
"A4:C1:38:07:07:07",
"A4:C1:38:08:08:08",
"A4:C1:38:09:09:09"
};

const int liczbaElementow = 5; //Liczba czujników
byte od[liczbaElementow]={0};
byte bat[liczbaElementow]={0};

int wilg[liczbaElementow]={-1};
float temp[liczbaElementow]={-275};


unsigned long czas_pomiedzy_skanowaniem;
unsigned long wifimilis;
unsigned long lastReadTime = 0;

boolean METRIC = true; //Set true for metric system; false for imperial

String text01;
String text02;
String text03;
String text04;
String text05;
String text06;
String text07;
String text08;
String text09;
String text10;
String text11;
String text12;
String text13;
String text14;
String text15;
String zmienna;
String MAC_ADR;
byte BLE_DEVICE;
bool wyswietl = 0;

//#include <supla/storage/eeprom.h>
//Supla::Eeprom eeprom;

Supla::ESPWifi wifi;
Supla::LittleFsConfig configSupla;

Supla::Device::StatusLed statusLed(STATUS_LED_GPIO, true); // inverted state
Supla::EspWebServer suplaServer;
//wifi.setSSLEnabled(false);
Supla::Html::DeviceInfo htmlDeviceInfo(&SuplaDevice);
Supla::Html::WifiParameters htmlWifi;
Supla::Html::ProtocolParameters htmlProto;
Supla::Html::StatusLedParameters htmlStatusLed;

byte current_humidity;
byte current_batt;

word val;

NimBLEScan *pBLEScan;

void IRAM_ATTR resetModule(){
    ets_printf("reboot\n");
    esp_restart();
}

float current_temperature;

class MyAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice)
    {
        if (advertisedDevice->haveName() && advertisedDevice->haveServiceData()) {

            int serviceDataCount = advertisedDevice->getServiceDataCount();
            std::string strServiceData = advertisedDevice->getServiceData(0);

            uint8_t cServiceData[100];
            char charServiceData[100];

            strServiceData.copy((char *)cServiceData, strServiceData.length(), 0);

            for (int i=0;i<strServiceData.length();i++) {
                sprintf(&charServiceData[i*2], "%02x", cServiceData[i]);
            }

            std::stringstream ss;
            ss << "fe95" << charServiceData;

            

            text01 = String(charServiceData[0]) + String(charServiceData[1]);      //Serial.print(text1); Serial.print(" ");
            text02 = String(charServiceData[2]) + String(charServiceData[3]);      //Serial.print(text2); Serial.print(" ");
            text03 = String(charServiceData[4]) + String(charServiceData[5]);      //Serial.print(text3); Serial.print(" ");
            text04 = String(charServiceData[6]) + String(charServiceData[7]);      //Serial.print(text4); Serial.print(" ");
            text05 = String(charServiceData[8]) + String(charServiceData[9]);      //Serial.print(text5); Serial.print(" "); 
            text06 = String(charServiceData[10]) + String(charServiceData[11]);    //Serial.print(text6); Serial.print(" "); 
            text07 = String(charServiceData[12]) + String(charServiceData[13]);    //Serial.print(text7); Serial.print(" "); 
            text08 = String(charServiceData[14]) + String(charServiceData[15]);    //Serial.print(text8); Serial.print(" "); 
            text09 = String(charServiceData[16]) + String(charServiceData[17]);    //Serial.print(text9); Serial.print(" "); 
            text10 = String(charServiceData[18]) + String(charServiceData[19]);   //Serial.print(text10); Serial.print(" "); 
            text11 = String(charServiceData[20]) + String(charServiceData[21]);   //Serial.print(text11); Serial.print(" "); 
            text12 = String(charServiceData[22]) + String(charServiceData[23]);   //Serial.print(text12); Serial.print(" "); 
            
            //Serial.println();

            zmienna = text01; Zamien_litery(); text01 = zmienna;
            zmienna = text02; Zamien_litery(); text02 = zmienna;
            zmienna = text03; Zamien_litery(); text03 = zmienna;
            zmienna = text04; Zamien_litery(); text04 = zmienna;
            zmienna = text05; Zamien_litery(); text05 = zmienna;
            zmienna = text06; Zamien_litery(); text06 = zmienna;
            zmienna = text07; Zamien_litery(); text07 = zmienna;
            zmienna = text08; Zamien_litery(); text08 = zmienna;
            zmienna = text09; Zamien_litery(); text09 = zmienna;
            zmienna = text10; Zamien_litery(); text10 = zmienna;
            zmienna = text11; Zamien_litery(); text11 = zmienna;
            zmienna = text12; Zamien_litery(); text12 = zmienna;
            zmienna = text13; Zamien_litery(); text13 = zmienna;
            zmienna = text14; Zamien_litery(); text14 = zmienna;
            zmienna = text15; Zamien_litery(); text15 = zmienna;




            Konwersja();


            
        }
    }
};

class Czujnik : public Supla::Sensor::ThermHygroMeter {
 public:
  explicit Czujnik(int sensorNumber)
      : sensorNumber(sensorNumber),
        temperature(-275),
        humidity(-1)
  {

  }

  double getTemp() {
    temperature = temp[sensorNumber];
    return temperature;
  }

  double getHumi() {
    humidity = wilg[sensorNumber];
    return humidity;
  }

 private:
  void iterateAlways() {
    if (millis() - lastReadTime > 10000) {
      lastReadTime = millis();
      channel.setNewValue(getTemp(), getHumi());
      channel.setBatteryLevel(bat[sensorNumber]);
    }
  }

  void onInit() {
    channel.setNewValue(getTemp(), getHumi());
    channel.setBatteryLevel(bat[sensorNumber]);
  }

 protected:
  const int sensorNumber;
  float temperature;
  int humidity;
};

Czujnik* czujniki[liczbaElementow]; 


void setup() {

  Serial.begin(115200);
  initBluetooth();

  // Channels configuration
  auto buttonCfgRelay = new Supla::Control::Button(BUTTON_CFG_RELAY_GPIO, true, true);
  buttonCfgRelay->configureAsConfigButton(&SuplaDevice);
  
 for (int i = 0; i < liczbaElementow; ++i) {
    czujniki[i] = new Czujnik(i);
    czujniki[i]->getChannel()->setFlag(SUPLA_CHANNELSTATE_FIELD_BATTERYPOWERED);
    czujniki[i]->getChannel()->setFlag(SUPLA_CHANNELSTATE_FIELD_BATTERYLEVEL);
  }
 
  // configure defualt Supla CA certificate
  SuplaDevice.setSuplaCACert(suplaCACert);
  SuplaDevice.setSupla3rdPartyCACert(supla3rdCACert);
  SuplaDevice.setName("Supla Xiaomi");
  //SuplaDevice.addFlags(SUPLA_DEVICE_FLAG_SLEEP_MODE_ENABLED);

  SuplaDevice.begin();
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); 
}

void loop() {
  

  if (millis() - czas_pomiedzy_skanowaniem > 120000) {
    Serial.println(F("skanuje bluetooth "));
    czas_pomiedzy_skanowaniem = millis();
    NimBLEScan* pBLEScan = NimBLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    NimBLEScanResults foundDevices = pBLEScan->start(SCAN_TIME);
    byte count = foundDevices.getCount();
    delay(100);
  for (int i = 0; i < liczbaElementow; i++) {
    od[i] = od[i] + 1;
  }
  }

  SuplaDevice.iterate();


    if (WiFi.status() != WL_CONNECTED) {    
      if (millis() > wifimilis)  {               
        WiFi.begin(); 
        wifimilis = (millis() + 30000) ;
      }
    }
    if (millis() > 86400000){
      ESP.restart();
    }
}
void initBluetooth()
{
    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScan->setInterval(0x50);
    pBLEScan->setWindow(0x30);
}

void Konwersja()
{
  int number;
  String hexstring;
  int t1;
  int t2;


  hexstring = "#" + text07;
  number = (int) strtol( &hexstring[1], NULL, 16);
  t1 = number & 0xFF;                        

  hexstring = "#" + text08;
  number = (int) strtol( &hexstring[1], NULL, 16);
  t2 = number & 0xFF;

  //current_temperature = t1 * 256;
  current_temperature = ( t1 == 255 ? -1 : t1 ) * 256;
  current_temperature = current_temperature + t2;
  current_temperature = current_temperature / 10;
  // Konwersja z Faranheitów na Celsjusze
                                      
  hexstring = "#" + text09;
  number = (int) strtol( &hexstring[1], NULL, 16);
  current_humidity = number & 0xFF;

  hexstring = "#" + text10;
  number = (int) strtol( &hexstring[1], NULL, 16);
  current_batt = number & 0xFF;
  
  MAC_ADR = "";
  MAC_ADR = MAC_ADR + text01 + ":";
  MAC_ADR = MAC_ADR + text02 + ":";
  MAC_ADR = MAC_ADR + text03 + ":";
  MAC_ADR = MAC_ADR + text04 + ":";
  MAC_ADR = MAC_ADR + text05 + ":";
  MAC_ADR = MAC_ADR + text06;

  BLE_DEVICE = 0;
for (int i = 0; i < liczbaElementow; i++) {
  if (MAC_ADR == xiaomiBleDeviceMacs[i]) {
    BLE_DEVICE = i ;
    wyswietl = 1;
    Wyswietl();
    break;  // Przerwij pętlę po znalezieniu pasującego adresu MAC
  }
}
/////////////////////////////////////////////////////////
}
void zerowanie(int index) {
  if (od[index] > 5) {
    temp[index] = -275;
    wilg[index] = -1;
    bat[index] = 0;
    od[index] = 0;
  }
}

void Wyswietl(){
  if(wyswietl == 1){
    Serial.print(F("Urządzenie:"));
    Serial.print(BLE_DEVICE);
    Serial.print(F(" MAC: ")); 
    Serial.print(MAC_ADR);
                                   
    Serial.print(F(" Temp: "));
    Serial.print(current_temperature,1);
    Serial.print(F("stC. Wilgotnosc: "));
    Serial.print(current_humidity);
    Serial.print(F("%. Bateria: "));
    Serial.print(current_batt);
    Serial.println(F("%."));   
  
      temp[BLE_DEVICE] = current_temperature;
      wilg[BLE_DEVICE] = current_humidity;
      bat[BLE_DEVICE] = current_batt;
      od[BLE_DEVICE] = 0;
    

     for (int i = 0; i < liczbaElementow; i++) {
    zerowanie(i);
  }
    wyswietl = 0; 
  }
}

void Zamien_litery()  //Zamieniamy otrzymywane małe litery na duże
{
  zmienna.replace('a', 'A');
  zmienna.replace('b', 'B');
  zmienna.replace('c', 'C');
  zmienna.replace('d', 'D');
  zmienna.replace('e', 'E');
  zmienna.replace('f', 'F');
}