#include <Arduino.h>
//#include "ble_uart.h"

//https://github.com/PaulStoffregen/OneWire
#include "OneWire.h"
//https://github.com/milesburton/Arduino-Temperature-Control-Library
#include "DallasTemperature.h"

//温度センサ DS18B20 互換(Amazon)
//参考http://make.bcde.jp/arduino/ds18b201-wire%E3%81%A7%E6%B8%A9%E5%BA%A6%E8%A8%88%E6%B8%AC/

const uint8_t sensor_pin = 23;
OneWire oneWire(sensor_pin);
DallasTemperature sensor(&oneWire);

int ref_temprature = 57;
int temperature;

const uint8_t pwm_pin = 22;
const uint8_t pwm_cannel = 3;

enum class Mode{
    On,
    Off,
    Control
};

Mode mode;

enum class IHCommands : int{
    Kiri = 1024,
    Kanetsu = (int)(1024*0.75),
    Agemono = (int)(1024*0.55),
    Tsuyoku = (int)(1024*0.35),
    Yowaku = (int)(1024*0.15),
    Default = 0
};

//Arduinoがバカすぎてプロトタイプ宣言を書かないとenum型を引数にできない
void SendCommand(IHCommands command);

void SendCommand(IHCommands command){
    ledcWrite(pwm_cannel,(int)command);
    vTaskDelay(300);
    ledcWrite(pwm_cannel,(int)IHCommands::Default);
}

void ControlTask(void* arg) {
  portTickType wakeupTime = xTaskGetTickCount();
  while (1) {
      switch(mode){
          case Mode::On :
            SendCommand(IHCommands::Agemono);
          break;
          case Mode::Off :
            SendCommand(IHCommands::Kiri);
            break;
          case Mode::Control :
            if(temperature >= ref_temprature + 1){
                SendCommand(IHCommands::Kiri);
            }
            if(temperature <= ref_temprature - 1){
                SendCommand(IHCommands::Agemono);
            }
            Serial.println(temperature);
            break;
        }
      vTaskDelayUntil(&wakeupTime, 1000);      
  }
}

void SensorTask(void* arg) {
    portTickType wakeupTime = xTaskGetTickCount();
    while (1) {
        sensor.requestTemperatures();
        Serial.println(sensor.getTempCByIndex(0));
        vTaskDelayUntil(&wakeupTime, 500);
    }
}

void BleTask(void* arg) {
    //ble.Init();
    portTickType wakeupTime = xTaskGetTickCount();
    while (1) {
        //ble.Update();
        vTaskDelayUntil(&wakeupTime, 100);
    }
}

void setup() {
    ledcSetup(pwm_cannel, 1000, 10);// 1000Hz, 10bit
    ledcAttachPin(pwm_pin, pwm_cannel);
    ledcWrite(pwm_cannel,(int)IHCommands::Default);

    sensor.setResolution(9);//9bit

    Serial.begin(115200);
    
    //xTaskCreatePinnedToCore(ControlTask, "Control", 4096, nullptr, 4, nullptr, 0);
    xTaskCreatePinnedToCore(SensorTask, "Sensor", 4096, nullptr, 5, nullptr, 0);
    //xTaskCreatePinnedToCore(BleTask, "Ble", 4096, nullptr, 5, nullptr, 1);    
}

void loop() {   
}
