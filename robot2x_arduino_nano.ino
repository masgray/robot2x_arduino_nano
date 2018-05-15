#include <Arduino_FreeRTOS.h>
#include <VL53L0X.h>

#include <Wire.h>

constexpr int PinWheelLeftUp PROGMEM     = 5;
constexpr int PinWheelLeftDown PROGMEM   = 6;
constexpr int PinWheelRightUp PROGMEM    = 9;
constexpr int PinWheelRightDown PROGMEM  = 10;

VL53L0X sensor;

constexpr int MaxPwm = 255;
constexpr int GoPwmLeft = MaxPwm * 65 / 100;
constexpr int GoPwmRight = MaxPwm * 65 / 100;

enum class Distance
{
  Stop,
  TooClose,
  Close,
  Middle,
  Far,
  Free
};

static uint16_t cm = 0;
static Distance barrierDistance = Distance::Free;

void GetDistance()
{
  auto bk = cm;
  auto tmp = sensor.readRangeSingleMillimeters() / 10;
  if (sensor.timeoutOccurred())
  {
    barrierDistance = Distance::Stop;
    sensor.init();
  }
  else
  {
    if (tmp > 80)
      tmp = 80;
    cm = tmp;
    if (cm < 5)
      barrierDistance = Distance::TooClose;
    else if (cm < 10)
      barrierDistance = Distance::Close;
    else if (cm < 15)
      barrierDistance = Distance::Middle;
    else if (cm < 20)
      barrierDistance = Distance::Far;
    else
      barrierDistance = Distance::Free;
  }
}

static void vDistanceTask(void *pvParameters)
{
  for (;;)
  {
    GetDistance();
    taskYIELD();
  }
}

static void vWheelsTask(void *pvParameters)
{
  static bool rotateLeft = false;
  for (;;)
  {
    switch (barrierDistance)
    {
      case Distance::Stop:
        WheelsStop();
        break;
      case Distance::TooClose:
        WheelsBack();
        vTaskDelay(150 / portTICK_PERIOD_MS);
        Rotate(rotateLeft, 255);
        break;
      case Distance::Close:
        Rotate(rotateLeft, 255);
        break;
      case Distance::Middle:
        Rotate(rotateLeft, 200);
        break;
      case Distance::Far:
        Rotate(rotateLeft, 130);
        break;
      case Distance::Free:
        WheelsGo();
        rotateLeft = !rotateLeft;
        break;
    }
    taskYIELD();
  }
}

static void vLedTask(void *pvParameters) 
{
  static bool on = false;
  for (;;)
  {
    digitalWrite(LED_BUILTIN, on);
    on = !on;
    vTaskDelay(cm * 10 / portTICK_PERIOD_MS);
  }
}

void setup() 
{
  pinMode(PinWheelLeftUp, OUTPUT);
  pinMode(PinWheelLeftDown, OUTPUT);
  pinMode(PinWheelRightUp, OUTPUT);
  pinMode(PinWheelRightDown, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  WheelsStop();

  Wire.begin();
  delay(100);

  sensor.init();
  sensor.setTimeout(1000);

//  Serial.begin(115200);
//  delay(500);

  xTaskCreate(vDistanceTask,      "T1", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
  xTaskCreate(vWheelsTask,        "T2", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
  xTaskCreate(vLedTask,           "T3", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
  vTaskStartScheduler();
}

void loop() 
{
}

void Rotate(bool rotateLeft, int ms)
{
//  WheelsBack();
//  vTaskDelay(xDelay1Ms * ms / 2);
  WheelsStop();
  if (rotateLeft)
  {
    WheelsRotationLeft();
  } 
  else
  {
    WheelsRotationRight();
  } 
  delay(ms);
  WheelsStop();
}

void WheelsRotationLeft()
{
  WheelLeftUpOff();
  WheelLeftDownOn();
  WheelRightUpOn();
  WheelRightDownOff();
}

void WheelsRotationRight()
{
  WheelLeftUpOn();
  WheelLeftDownOff();
  WheelRightUpOff();
  WheelRightDownOn();
}

void WheelsStop()
{
  WheelLeftUpOff();
  WheelLeftDownOff();
  WheelRightUpOff();
  WheelRightDownOff();
}

void WheelsGo()
{
  WheelLeftUpOn();
  WheelLeftDownOff();
  WheelRightUpOn();
  WheelRightDownOff();
}

void WheelsBack()
{
  WheelLeftUpOff();
  WheelLeftDownOn();
  WheelRightUpOff();
  WheelRightDownOn();
}

void WheelLeftUpOn()
{
  analogWrite(PinWheelLeftUp, GoPwmLeft);
}

void WheelLeftUpOff()
{
  analogWrite(PinWheelLeftUp, MaxPwm);
}

void WheelLeftDownOn()
{
  analogWrite(PinWheelLeftDown, GoPwmLeft);
}

void WheelLeftDownOff()
{
  analogWrite(PinWheelLeftDown, MaxPwm);
}

void WheelRightUpOn()
{
  analogWrite(PinWheelRightUp, GoPwmRight);
}

void WheelRightUpOff()
{
  analogWrite(PinWheelRightUp, MaxPwm);
}

void WheelRightDownOn()
{
  analogWrite(PinWheelRightDown, GoPwmRight);
}

void WheelRightDownOff()
{
  analogWrite(PinWheelRightDown, MaxPwm);
}

