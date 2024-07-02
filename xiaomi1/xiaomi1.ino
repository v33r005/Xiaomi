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

//Parametry
#define STATUS_LED_GPIO 2
#define BUTTON_CFG_RELAY_GPIO 0
#define SCAN_TIME 2                    // seconds
#define SCAN_BT_TIME 120               // seconds
const int liczbaElementow = 8;         //Liczba czujników
const char* xiaomiBleDeviceMacs[] = {  //Lista czujników
  "A4:C1:38:00:00:00",
  "A4:C1:38:00:00:00",
};


//Ładowanie bibiotek
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

#if !CONFIG_BT_NIMBLE_EXT_ADV
#  error Must enable extended advertising, see nimconfig.h file.
#endif

//Deklaracja zmiennych
byte od[liczbaElementow];
byte bat[liczbaElementow];
int wilg[liczbaElementow];
float temp[liczbaElementow];

unsigned long czas_pomiedzy_skanowaniem;
unsigned long wifimilis;
unsigned long lastReadTime = 0;
boolean METRIC = true;  //Set true for metric system; false for imperial
byte current_humidity;
byte current_batt;
float current_temperature;

//#include <supla/storage/eeprom.h>
//Supla::Eeprom eeprom;

Supla::ESPWifi wifi;
Supla::LittleFsConfig configSupla;
Supla::Device::StatusLed statusLed(STATUS_LED_GPIO, true);  // inverted state
Supla::EspWebServer suplaServer;
//wifi.setSSLEnabled(false);
Supla::Html::DeviceInfo htmlDeviceInfo(&SuplaDevice);
Supla::Html::WifiParameters htmlWifi;
Supla::Html::ProtocolParameters htmlProto;
Supla::Html::StatusLedParameters htmlStatusLed;
NimBLEScan* pBLEScan;

void printHexString(const uint8_t* data, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    Serial.print("0x");
    if (data[i] < 0x10) Serial.print("0");  // Dodaj zero przed jednocyfrowe wartości
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

std::string strServiceData;
class MyAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
    if (advertisedDevice->haveName() && advertisedDevice->haveServiceData()) {

      int serviceDataCount = advertisedDevice->getServiceDataCount();
      strServiceData = advertisedDevice->getServiceData(0);
      Serial.println("-----Nowy wpis-----");
      Serial.println("--------Czysty HEX-");
      printHexString(reinterpret_cast<const uint8_t*>(strServiceData.c_str()), strServiceData.length());
      Serial.println("--------MAC--------");
      Serial.println(advertisedDevice->getAddress().toString().c_str());
      Serial.println("--------NAZWA------");
      Serial.println(advertisedDevice->getName().c_str());
      Serial.println("--------RSSI------");
      Serial.println(advertisedDevice->getRSSI());



      String macAddress = advertisedDevice->getAddress().toString().c_str();
      macAddress.toUpperCase();
      for (int i = 0; i < liczbaElementow; i++) {
        if (macAddress == xiaomiBleDeviceMacs[i]) {
          //current_temperature = (float)((strServiceData[7] | (strServiceData[6] << 8)) * 0.1);  //dodać obsługę ujemnych
          int16_t rawTemperature = (strServiceData[7] | (strServiceData[6] << 8));
          current_temperature = rawTemperature * 0.1;
          current_humidity = (float)(strServiceData[8]);
          current_batt = (float)(strServiceData[9]);
          Serial.print(F("Urządzenie:"));
          Serial.print(i);
          Serial.print(F(" MAC: "));
          Serial.print(macAddress);
          Serial.print(F(" Temp: "));
          Serial.print(current_temperature, 1);
          Serial.print(F("stC. Wilgotnosc: "));
          Serial.print(current_humidity);
          Serial.print(F("%. Bateria: "));
          Serial.print(current_batt);
          Serial.println(F("%."));
          temp[i] = current_temperature;
          wilg[i] = current_humidity;
          bat[i] = current_batt;
          od[i] = 0;
          zerowanie(i);
          break;  // Przerwij pętlę po znalezieniu pasującego adresu MAC
        }
      }
    }
  }
};



class Czujnik : public Supla::Sensor::ThermHygroMeter {
public:
  explicit Czujnik(int sensorNumber)
    : sensorNumber(sensorNumber)  //,
                                  //temperature(-275),
                                  //humidity(-1)
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
    channel.setNewValue(temp[sensorNumber], wilg[sensorNumber]);
    channel.setBatteryLevel(bat[sensorNumber]);
  }

protected:
  const int sensorNumber;
  float temperature = -275;
  int humidity = -1;
};

Czujnik* czujniki[liczbaElementow];


void setup() {

  Serial.begin(115200);
  initBluetooth();

  // Channels configuration
  auto buttonCfgRelay = new Supla::Control::Button(BUTTON_CFG_RELAY_GPIO, true, true);
  buttonCfgRelay->configureAsConfigButton(&SuplaDevice);

  for (int i = 0; i < liczbaElementow; ++i) {
    od[i] = 0;
    bat[i] = 0;
    wilg[i] = -1;
    temp[i] = -275;
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

//pętla główna
void loop() {
  if ((millis() - czas_pomiedzy_skanowaniem) > (SCAN_BT_TIME * 1000)) {
    Serial.println(F("skanuje bluetooth "));
    czas_pomiedzy_skanowaniem = millis();
    NimBLEScan* pBLEScan = NimBLEDevice::getScan();  //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);  //active scan uses more power, but get results faster
    NimBLEScanResults foundDevices = pBLEScan->start(SCAN_TIME);
    byte count = foundDevices.getCount();
    delay(100);
    for (int i = 0; i < liczbaElementow; i++) {
      od[i] = od[i] + 1;
    }
  }

  SuplaDevice.iterate();

  if (WiFi.status() != WL_CONNECTED) {
    if (millis() > wifimilis) {
      WiFi.begin();
      wifimilis = (millis() + 30000);
    }
  }
  if (millis() > 86400000) {
    ESP.restart();
  }
}

void initBluetooth() {
  NimBLEDevice::init("");
  pBLEScan = NimBLEDevice::getScan();  //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  //active scan uses more power, but get results faster
  pBLEScan->setInterval(0x50);
  pBLEScan->setWindow(0x30);
}

//zerowanie wartości czujnika o id
void zerowanie(int index) {
  if (od[index] > 5) {
    temp[index] = -275;
    wilg[index] = -1;
    bat[index] = 0;
    od[index] = 0;
  }
}