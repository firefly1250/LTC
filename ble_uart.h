/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>

class BleUart : public BLECharacteristicCallbacks, public BLEServerCallbacks, public Print{
    BLEServer *pServer = nullptr;
    BLECharacteristic * pTxCharacteristic;
    bool deviceConnected = false;
    bool oldDeviceConnected = false;

    const char* SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
    const char* CHARACTERISTIC_UUID_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
    const char* CHARACTERISTIC_UUID_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

    uint8_t tx_counter=0;
    uint8_t buffer[100];
    SemaphoreHandle_t semaphore;

    void onWrite(BLECharacteristic *pCharacteristic) override{
        std::string rxValue = pCharacteristic->getValue();

        if (rxValue.length() ==rxValue[0]*2 + 3) {
            Serial.println("*********");
            Serial.print("Set: ");
            Serial.println((int)rxValue[1]);
            Serial.print("Interval: ");
            Serial.println(rxValue[2]/2.0f);
            for(int i=0;i<rxValue[0];i++){
                Serial.print(i);
                Serial.println(":");
                Serial.print("  r: ");
                Serial.print((int)rxValue[3+ i*2]);
                Serial.print(" , theta: ");  
                Serial.println(rxValue[3+ i*2+1]/256.0f*360-180);
            }
            Serial.println("*********");
        }
        else{
          Serial.println("miss");
        }
    }

    void onConnect(BLEServer* pServer) override{
      deviceConnected = true;
    }

    void onDisconnect(BLEServer* pServer) override{
      deviceConnected = false;
    }

public:
    BleUart(){
        semaphore = xSemaphoreCreateMutex();
    }
    void Init(){
        // Create the BLE Device
        BLEDevice::init("molten shooting");

        // Create the BLE Server
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(this);

        // Create the BLE Service
        BLEService *pService = pServer->createService(SERVICE_UUID);

        // Create a BLE Characteristic
        pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX,BLECharacteristic::PROPERTY_NOTIFY);
        pTxCharacteristic->addDescriptor(new BLE2902());
        BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX,BLECharacteristic::PROPERTY_WRITE);
        pRxCharacteristic->setCallbacks(this);

        // Start the service
        pService->start();

        // Start advertising
        pServer->getAdvertising()->start();

    }

    size_t write(uint8_t value) override{
        size_t n = 0;
        if(xSemaphoreTake(semaphore, portMAX_DELAY)){
            if(tx_counter < 100){
                buffer[tx_counter++] = value;
                n++;
            }
            xSemaphoreGive(semaphore);
        }
        return n;
    }
    size_t write(const uint8_t *data, size_t size) override{
        size_t n = 0;
        if(xSemaphoreTake(semaphore, portMAX_DELAY)){
            while(n < size && tx_counter < 100) {
                buffer[tx_counter++] = data[n++];
            }
            xSemaphoreGive(semaphore);
        }
        return n;
    }

    void Update(){
        if (deviceConnected && tx_counter>0) {
            if(xSemaphoreTake(semaphore, portMAX_DELAY)){
                pTxCharacteristic->setValue(buffer,tx_counter);
                tx_counter=0;
                xSemaphoreGive(semaphore);
                pTxCharacteristic->notify();
            }
	    }

        // disconnecting
        if (!deviceConnected && oldDeviceConnected) {
            delay(500); // give the bluetooth stack the chance to get things ready
            pServer->startAdvertising(); // restart advertising
            Serial.println("start advertising");
            oldDeviceConnected = deviceConnected;
        }
        // connecting
        if (deviceConnected && !oldDeviceConnected) {
    		// do stuff here on connecting
            oldDeviceConnected = deviceConnected;
        }
    }
};