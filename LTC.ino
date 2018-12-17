#include <Arduino.h>
#include "ble_uart.h"

//https://github.com/PaulStoffregen/OneWire
#include "OneWire.h"

//https://github.com/milesburton/Arduino-Temperature-Control-Library
#include "DallasTemperature.h"

//温度センサ DS18B20 互換(Amazon)
//参考http://make.bcde.jp/arduino/ds18b201-wire%E3%81%A7%E6%B8%A9%E5%BA%A6%E8%A8%88%E6%B8%AC/


enum class Mode : uint8_t{
    On,
    Off,
    Control
};

enum class IHCommands : int{
    Kiri = 1024,
    Kanetsu = (int)(1024*0.75),
    Agemono = (int)(1024*0.55),
    Tsuyoku = (int)(1024*0.35),
    Yowaku = (int)(1024*0.15),
    Default = 0
};

BleUart ble;

const uint8_t sensor_pin = 23;
OneWire oneWire(sensor_pin);
DallasTemperature sensor(&oneWire);

float temperature = 0;
float ref_temperature = 0;

const uint8_t pwm_pin = 22;
const uint8_t pwm_cannel = 3;

Mode mode = Mode::Off;


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
            if(temperature >= ref_temperature + 1){
                SendCommand(IHCommands::Kiri);
            }
            if(temperature <= ref_temperature - 1){
                SendCommand(IHCommands::Agemono);
            }
            break;
        }
      vTaskDelayUntil(&wakeupTime, 1000);      
  }
}

void SensorTask(void* arg) {
    portTickType wakeupTime = xTaskGetTickCount();
    while (1) {
        sensor.requestTemperatures();
        auto tmp = sensor.getTempCByIndex(0);
        if(tmp > 0) temperature = tmp; //うまく読めてない場合は負の値が返ってくるから
        vTaskDelayUntil(&wakeupTime, 500);
    }
}

void UpdateTask(void* arg) {
    portTickType wakeupTime = xTaskGetTickCount();
    while (1) {
        uint8_t value = ble.GetRecievedValue();
        if(value < 254){
            ref_temperature = value/2.0f;
            mode = Mode::Control;
        }
        else{
            ref_temperature = 0;
            if(value == 255) mode = Mode::On;
            else if(value == 254) mode = Mode::Off;
        }
        Serial.printf("%03.1f , %03.1f , %d\n",ref_temperature,temperature,value);

        uint8_t data[2]{value,(uint8_t)(temperature*2.0f)};
        ble.write(data,2);
        
        vTaskDelayUntil(&wakeupTime, 500);
    }
}

void BleTask(void* arg) {
    ble.Init();
    portTickType wakeupTime = xTaskGetTickCount();
    while (1) {
        ble.Update();
        vTaskDelayUntil(&wakeupTime, 100);
    }
}

void setup() {
    ledcSetup(pwm_cannel, 1000, 10);// 1000Hz, 10bit
    ledcAttachPin(pwm_pin, pwm_cannel);
    ledcWrite(pwm_cannel,(int)IHCommands::Default);

    sensor.setResolution(9);//9bit

    Serial.begin(115200);
    
    xTaskCreatePinnedToCore(ControlTask, "Control", 4096, nullptr, 4, nullptr, 0);
    xTaskCreatePinnedToCore(SensorTask, "Sensor", 4096, nullptr, 5, nullptr, 0);
    xTaskCreatePinnedToCore(UpdateTask, "Update", 4096, nullptr, 5, nullptr, 0);
    xTaskCreatePinnedToCore(BleTask, "Ble", 4096, nullptr, 5, nullptr, 1);    
}

void loop() {   
}
